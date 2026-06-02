#include "server.hpp"
#include "hatena_bookmark_client.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "slack_payload.hpp"
#include "uri.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>

namespace chatwork {
namespace {

volatile std::sig_atomic_t keep_running = 1;

void handle_signal(int) {
    keep_running = 0;
}

std::string url_decode(std::string_view s) {
    std::string result;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            const char hex[3] = {s[i + 1], s[i + 2], '\0'};
            result += static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2;
        } else if (s[i] == '+') {
            result += ' ';
        } else {
            result += s[i];
        }
    }
    return result;
}

std::string query_param(std::string_view query, std::string_view key) {
    std::size_t pos = 0;
    while (pos < query.size()) {
        const std::size_t amp = query.find('&', pos);
        const std::string_view pair =
            query.substr(pos, amp == std::string_view::npos ? std::string_view::npos : amp - pos);
        const std::size_t eq = pair.find('=');
        if (eq != std::string_view::npos && pair.substr(0, eq) == key) {
            return url_decode(pair.substr(eq + 1));
        }
        if (amp == std::string_view::npos) break;
        pos = amp + 1;
    }
    return {};
}

} // namespace

void install_signal_handlers() {
    struct sigaction action {};
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (::sigaction(SIGINT, &action, nullptr) < 0 || ::sigaction(SIGTERM, &action, nullptr) < 0) {
        throw std::runtime_error(std::format("sigaction failed: {}", std::strerror(errno)));
    }
}

Server::Server(std::vector<std::unique_ptr<HatenaClient>> clients,
               const std::string& listen_address,
               std::uint16_t listen_port)
    : _clients(std::move(clients)) {
    Socket s(::socket(AF_INET, SOCK_STREAM, 0));
    if (s.get() < 0) {
        throw std::runtime_error(std::format("socket failed: {}", std::strerror(errno)));
    }

    int yes = 1;
    if (::setsockopt(s.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        throw std::runtime_error(std::format("setsockopt failed: {}", std::strerror(errno)));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    if (::inet_pton(AF_INET, listen_address.c_str(), &address.sin_addr) != 1) {
        throw std::runtime_error(std::format("invalid listen address: {}", listen_address));
    }
    address.sin_port = htons(listen_port);

    if (::bind(s.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error(std::format("bind failed: {}", std::strerror(errno)));
    }

    if (::listen(s.get(), SOMAXCONN) < 0) {
        throw std::runtime_error(std::format("listen failed: {}", std::strerror(errno)));
    }

    _socket = std::move(s);
}

void Server::handle_client(int client_fd) {
    const HttpRequest request(client_fd);

    if (request.method() == "GET") {
        handle_get(client_fd, request.target());
        return;
    }

    const SlackPayload payload(request.body());

    if (const auto challenge = payload.challenge()) {
        HttpResponse::ok(client_fd, *challenge);
        return;
    }

    const auto text = payload.text().value_or(std::string(request.body()));
    for (const auto& url : URI::extract_url(text)) {
        for (const auto& client : _clients) {
            client->process(url);
        }
    }

    HttpResponse::ok(client_fd);
}

void Server::handle_get(int client_fd, std::string_view target) {
    const std::size_t query_start = target.find('?');
    const std::string_view path = target.substr(0, query_start);
    const std::string url = (query_start == std::string_view::npos)
                                ? std::string{}
                                : query_param(target.substr(query_start + 1), "url");

    if (path != "/bookmark" || url.empty()) {
        HttpResponse::ok(client_fd);
        return;
    }

    for (const auto& client : _clients) {
        if (auto* bookmark_client = dynamic_cast<HatenaBookmarkClient*>(client.get())) {
            const auto [bookmark, result] = bookmark_client->get_bookmark(url);
            std::string joined_tags;
            for (const auto& tag : bookmark.tags()) {
                joined_tags += '[' + tag + ']';
            }
            HttpResponse::ok(client_fd,
                             std::format("{}, {}, {}\n", joined_tags, bookmark.url(),
                                         bookmark.comment()));
            return;
        }
    }

    HttpResponse::ok(client_fd);
}

void Server::run() {
    while (keep_running) {
        sockaddr_in client_address{};
        socklen_t client_size = sizeof(client_address);
        Socket client(::accept(_socket.get(), reinterpret_cast<sockaddr*>(&client_address), &client_size));
        if (client.get() < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::format("accept failed: {}", std::strerror(errno)));
        }

        try {
            handle_client(client.get());
        } catch (const std::exception& error) {
            std::cerr << "request error: " << error.what() << std::endl;
        }
    }
}

} // namespace chatwork
