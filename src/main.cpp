#include "hatena_bookmark_client.hpp"
#include "oauth.hpp"
#include "server.hpp"

#include <memory>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

std::uint16_t parse_listen_port() {
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

std::string parse_listen_address() {
    const char* env = std::getenv("CXX_CHATWORK_LISTEN");
    if (env == nullptr || *env == '\0') {
        return "0.0.0.0";
    }
    return env;
}

chatwork::OAuth make_oauth() {
    const char* key = std::getenv("HATENA_CONSUMER_KEY");
    if (key == nullptr || *key == '\0') {
        throw std::runtime_error("HATENA_CONSUMER_KEY is required");
    }
    const char* secret = std::getenv("HATENA_CONSUMER_SECRET");
    if (secret == nullptr || *secret == '\0') {
        throw std::runtime_error("HATENA_CONSUMER_SECRET is required");
    }
    return chatwork::OAuth(key, secret);
}

} // namespace

int main() {
    try {
        chatwork::install_signal_handlers();

        chatwork::OAuth oauth = make_oauth();
        oauth.authorize();

        const std::uint16_t listen_port = parse_listen_port();
        const std::string listen_address = parse_listen_address();
        std::vector<std::unique_ptr<chatwork::HatenaClient>> clients;
        clients.push_back(std::make_unique<chatwork::HatenaBookmarkClient>(std::move(oauth)));
        chatwork::Server server(std::move(clients), listen_address, listen_port);

        std::cout << "Listening on " << listen_address << ":" << listen_port << std::endl;

        server.run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }

    return 0;
}
