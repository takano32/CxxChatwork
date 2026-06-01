#include "server.hpp"
#include "http.hpp"
#include "json.hpp"
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

void handle_client(int client_fd) {
    const std::string request = read_http_request(client_fd);
    const std::string_view body = http_body(request);

    if (const auto challenge = find_json_string_value(body, "challenge")) {
        respond_ok(client_fd, *challenge);
        return;
    }

    const auto source = slack_message_text(body).value_or(std::string(body));
    for (const auto& uri : URI::extract(source)) {
        std::cout << uri << "\n";
    }

    respond_ok(client_fd);
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

Server::Server(std::uint16_t port) {
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
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (::bind(s.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error(std::string("bind failed: ") + std::strerror(errno));
    }

    if (::listen(s.get(), SOMAXCONN) < 0) {
        throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));
    }

    socket_ = std::move(s);
}

void Server::run() {
    while (keep_running) {
        sockaddr_in client_address{};
        socklen_t client_size = sizeof(client_address);
        Socket client(::accept(socket_.get(), reinterpret_cast<sockaddr*>(&client_address), &client_size));
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
}

} // namespace chatwork
