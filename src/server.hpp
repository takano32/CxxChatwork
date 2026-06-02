#pragma once
#include "hatena_bookmark_client.hpp"
#include "socket.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace chatwork {

void install_signal_handlers();

class Server {
public:
    Server(HatenaBookmarkClient client,
           const std::string& listen_address,
           std::uint16_t listen_port);
    void run();

private:
    Socket _socket;
    HatenaBookmarkClient _client;

    void handle_client(int client_fd);
    void handle_get(int client_fd, std::string_view target);
};

} // namespace chatwork
