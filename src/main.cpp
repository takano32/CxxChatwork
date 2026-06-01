#include "server.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

std::uint16_t parse_port() {
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

} // namespace

int main() {
    try {
        chatwork::install_signal_handlers();

        const std::uint16_t port = parse_port();
        chatwork::Server server(port);

        std::cout << "Listening on port " << port << std::endl;

        server.run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }

    return 0;
}
