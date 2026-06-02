#pragma once
#include "hatena_client.hpp"
#include "socket.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace chatwork {

void install_signal_handlers();

class Server {
public:
    Server(std::vector<std::unique_ptr<HatenaClient>> clients,
           const std::string& listen_address,
           std::uint16_t listen_port);
    void run();

private:
    Socket _socket;
    std::vector<std::unique_ptr<HatenaClient>> _clients;

    void handle_client(int client_fd);
    void handle_get(int client_fd, std::string_view target);
};

} // namespace chatwork
