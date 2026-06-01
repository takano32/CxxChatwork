#include "http_response.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/socket.h>

namespace chatwork {
namespace {

void send_all(int fd, std::string_view data) {
    while (!data.empty()) {
        const ssize_t sent = ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::string("send failed: ") + std::strerror(errno));
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
}

} // namespace

void HttpResponse::ok(int fd, std::string_view body) {
    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + std::string(body);
    send_all(fd, response);
}

} // namespace chatwork
