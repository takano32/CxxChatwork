#pragma once
#include "socket.hpp"
#include <cstdint>

namespace chatwork {

void install_signal_handlers();

class Server {
public:
    explicit Server(std::uint16_t port);
    void run();

private:
    Socket socket_;
};

} // namespace chatwork
