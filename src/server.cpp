#include "server.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "slack_payload.hpp"
#include "uri.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>

namespace chatwork {
namespace {

volatile std::sig_atomic_t keep_running = 1;

void handle_signal(int) {
    keep_running = 0;
}

} // namespace

void install_signal_handlers() {
    struct sigaction action {};
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (::sigaction(SIGINT, &action, nullptr) < 0 || ::sigaction(SIGTERM, &action, nullptr) < 0) {
        throw std::runtime_error(std::string("sigaction failed: ") + std::strerror(errno));
    }
}

Server::Server(std::vector<std::unique_ptr<HatenaClient>> clients,
               const std::string& listen_address,
               std::uint16_t listen_port)
    : _clients(std::move(clients)) {
    Socket s(::socket(AF_INET, SOCK_STREAM, 0));
    if (s.get() < 0) {
        throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
    }

    int yes = 1;
    if (::setsockopt(s.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        throw std::runtime_error(std::string("setsockopt failed: ") + std::strerror(errno));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    if (::inet_pton(AF_INET, listen_address.c_str(), &address.sin_addr) != 1) {
        throw std::runtime_error("invalid listen address: " + listen_address);
    }
    address.sin_port = htons(listen_port);

    if (::bind(s.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error(std::string("bind failed: ") + std::strerror(errno));
    }

    if (::listen(s.get(), SOMAXCONN) < 0) {
        throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));
    }

    _socket = std::move(s);
}

void Server::handle_client(int client_fd) {
    const HttpRequest request(client_fd);
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

void Server::run() {
    while (keep_running) {
        sockaddr_in client_address{};
        socklen_t client_size = sizeof(client_address);
        Socket client(::accept(_socket.get(), reinterpret_cast<sockaddr*>(&client_address), &client_size));
        if (client.get() < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::string("accept failed: ") + std::strerror(errno));
        }

        try {
            handle_client(client.get());
        } catch (const std::exception& error) {
            std::cerr << "request error: " << error.what() << std::endl;
        }
    }
}

} // namespace chatwork
