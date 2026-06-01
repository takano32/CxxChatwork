#include "http.hpp"

#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>

namespace chatwork {

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

void respond_ok(int client_fd, std::string_view body) {
    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + std::string(body);
    send_all(client_fd, response);
}

} // namespace chatwork
