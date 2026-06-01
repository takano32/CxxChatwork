#pragma once
#include <utility>
#include <unistd.h>

namespace chatwork {

class Socket {
public:
    explicit Socket(int fd = -1) : _fd(fd) {}

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : _fd(std::exchange(other._fd, -1)) {}

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();
            _fd = std::exchange(other._fd, -1);
        }
        return *this;
    }

    ~Socket() { close(); }

    int get() const { return _fd; }

private:
    void close() {
        if (_fd >= 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    int _fd;
};

} // namespace chatwork
