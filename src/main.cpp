#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace {

volatile std::sig_atomic_t keep_running = 1;

void handle_signal(int) {
    keep_running = 0;
}

void install_signal_handlers() {
    struct sigaction action {};
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (::sigaction(SIGINT, &action, nullptr) < 0 || ::sigaction(SIGTERM, &action, nullptr) < 0) {
        throw std::runtime_error(std::string("sigaction failed: ") + std::strerror(errno));
    }
}

class Socket {
public:
    explicit Socket(int fd = -1) : fd_(fd) {}

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    ~Socket() {
        close();
    }

    int get() const {
        return fd_;
    }

private:
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    int fd_;
};

std::uint16_t parse_port() {
    const char* env = std::getenv("CXX_CHATWORK_PORT");
    if (env == nullptr || *env == '\0') {
        throw std::runtime_error("CXX_CHATWORK_PORT is required");
    }

    unsigned int value = 0;
    const std::string_view input(env);
    const auto* begin = input.data();
    const auto* end = input.data() + input.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end || value == 0 || value > 65535) {
        throw std::runtime_error("CXX_CHATWORK_PORT must be an integer from 1 to 65535");
    }

    return static_cast<std::uint16_t>(value);
}

std::optional<std::size_t> content_length(std::string_view headers) {
    constexpr std::string_view key = "content-length:";

    std::size_t line_start = 0;
    while (line_start < headers.size()) {
        const std::size_t line_end = headers.find("\r\n", line_start);
        std::string_view line = headers.substr(
            line_start,
            line_end == std::string_view::npos ? std::string_view::npos : line_end - line_start);

        std::string lower;
        lower.reserve(line.size());
        for (char c : line) {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        if (lower.starts_with(key)) {
            line.remove_prefix(key.size());
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
                line.remove_prefix(1);
            }

            std::size_t value = 0;
            const auto* begin = line.data();
            const auto* end = line.data() + line.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec == std::errc{}) {
                return value;
            }
        }

        if (line_end == std::string_view::npos) {
            break;
        }
        line_start = line_end + 2;
    }

    return std::nullopt;
}

std::string read_http_request(int client_fd) {
    std::string request;
    std::array<char, 4096> buffer{};
    std::optional<std::size_t> expected_body_size;
    std::size_t body_start = std::string::npos;

    while (true) {
        const ssize_t n = ::recv(client_fd, buffer.data(), buffer.size(), 0);
        if (n == 0) {
            break;
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("recv failed: ") + std::strerror(errno));
        }

        request.append(buffer.data(), static_cast<std::size_t>(n));

        if (body_start == std::string::npos) {
            const std::size_t header_end = request.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                body_start = header_end + 4;
                expected_body_size = content_length(std::string_view(request).substr(0, header_end));
                if (!expected_body_size.has_value()) {
                    break;
                }
            }
        }

        if (expected_body_size.has_value() && request.size() >= body_start + *expected_body_size) {
            break;
        }
    }

    return request;
}

std::string_view http_body(std::string_view request) {
    const std::size_t header_end = request.find("\r\n\r\n");
    if (header_end == std::string_view::npos) {
        return {};
    }
    return request.substr(header_end + 4);
}

std::optional<std::string> parse_json_string_at(std::string_view json, std::size_t quote_pos) {
    if (quote_pos >= json.size() || json[quote_pos] != '"') {
        return std::nullopt;
    }

    std::string value;
    for (std::size_t i = quote_pos + 1; i < json.size(); ++i) {
        const char c = json[i];
        if (c == '"') {
            return value;
        }
        if (c != '\\') {
            value.push_back(c);
            continue;
        }

        if (++i >= json.size()) {
            return std::nullopt;
        }

        switch (json[i]) {
        case '"':
        case '\\':
        case '/':
            value.push_back(json[i]);
            break;
        case 'b':
            value.push_back('\b');
            break;
        case 'f':
            value.push_back('\f');
            break;
        case 'n':
            value.push_back('\n');
            break;
        case 'r':
            value.push_back('\r');
            break;
        case 't':
            value.push_back('\t');
            break;
        case 'u':
            value.append("\\u");
            if (i + 4 >= json.size()) {
                return std::nullopt;
            }
            value.append(json.substr(i + 1, 4));
            i += 4;
            break;
        default:
            value.push_back(json[i]);
            break;
        }
    }

    return std::nullopt;
}

std::optional<std::string> find_json_string_value(std::string_view json, std::string_view key) {
    const std::string quoted_key = "\"" + std::string(key) + "\"";
    std::size_t pos = 0;
    while ((pos = json.find(quoted_key, pos)) != std::string_view::npos) {
        std::size_t colon = pos + quoted_key.size();
        while (colon < json.size() && std::isspace(static_cast<unsigned char>(json[colon]))) {
            ++colon;
        }
        if (colon >= json.size() || json[colon] != ':') {
            pos += quoted_key.size();
            continue;
        }

        std::size_t value_pos = colon + 1;
        while (value_pos < json.size() && std::isspace(static_cast<unsigned char>(json[value_pos]))) {
            ++value_pos;
        }

        if (value_pos < json.size() && json[value_pos] == '"') {
            return parse_json_string_at(json, value_pos);
        }

        pos += quoted_key.size();
    }

    return std::nullopt;
}

std::optional<std::string> slack_message_text(std::string_view body) {
    if (auto event = body.find("\"event\""); event != std::string_view::npos) {
        if (auto text = find_json_string_value(body.substr(event), "text")) {
            return text;
        }
    }

    if (auto message = body.find("\"message\""); message != std::string_view::npos) {
        if (auto text = find_json_string_value(body.substr(message), "text")) {
            return text;
        }
    }

    return find_json_string_value(body, "text");
}

void send_all(int fd, std::string_view data) {
    while (!data.empty()) {
        const ssize_t sent = ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("send failed: ") + std::strerror(errno));
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
}

void respond_ok(int client_fd, std::string_view body = "ok\n") {
    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + std::string(body);
    send_all(client_fd, response);
}

void handle_client(int client_fd) {
    const std::string request = read_http_request(client_fd);
    const std::string_view body = http_body(request);

    if (const auto challenge = find_json_string_value(body, "challenge")) {
        respond_ok(client_fd, *challenge);
        return;
    }

    if (const auto text = slack_message_text(body)) {
        std::cout << *text << std::endl;
    } else if (!body.empty()) {
        std::cout << body << std::endl;
    }

    respond_ok(client_fd);
}

Socket create_server_socket(std::uint16_t port) {
    Socket server(::socket(AF_INET, SOCK_STREAM, 0));
    if (server.get() < 0) {
        throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
    }

    int yes = 1;
    if (::setsockopt(server.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        throw std::runtime_error(std::string("setsockopt failed: ") + std::strerror(errno));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (::bind(server.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error(std::string("bind failed: ") + std::strerror(errno));
    }

    if (::listen(server.get(), SOMAXCONN) < 0) {
        throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));
    }

    return server;
}

} // namespace

int main() {
    try {
        install_signal_handlers();

        const std::uint16_t port = parse_port();
        Socket server = create_server_socket(port);

        std::cout << "Listening on port " << port << std::endl;

        while (keep_running) {
            sockaddr_in client_address{};
            socklen_t client_size = sizeof(client_address);
            Socket client(::accept(server.get(), reinterpret_cast<sockaddr*>(&client_address), &client_size));
            if (client.get() < 0) {
                if (errno == EINTR) {
                    continue;
                }
                throw std::runtime_error(std::string("accept failed: ") + std::strerror(errno));
            }

            try {
                handle_client(client.get());
            } catch (const std::exception& error) {
                std::cerr << "request error: " << error.what() << std::endl;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }

    return 0;
}
