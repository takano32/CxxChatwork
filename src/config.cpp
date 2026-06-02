#include "config.hpp"

#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace chatwork {

Config::Config() {
    const char* port_env = std::getenv("CXX_CHATWORK_PORT");
    if (port_env == nullptr || *port_env == '\0') {
        throw std::runtime_error("CXX_CHATWORK_PORT is required");
    }
    unsigned int port_value = 0;
    const std::string_view port_input(port_env);
    const auto result = std::from_chars(
        port_input.data(), port_input.data() + port_input.size(), port_value);
    if (result.ec != std::errc{} || result.ptr != port_input.data() + port_input.size()
        || port_value == 0 || port_value > 65535) {
        throw std::runtime_error("CXX_CHATWORK_PORT must be an integer from 1 to 65535");
    }
    _listen_port = static_cast<std::uint16_t>(port_value);

    const char* listen_env = std::getenv("CXX_CHATWORK_LISTEN");
    _listen_address = (listen_env != nullptr && *listen_env != '\0') ? listen_env : "0.0.0.0";

    const char* key_env = std::getenv("HATENA_CONSUMER_KEY");
    if (key_env == nullptr || *key_env == '\0') {
        throw std::runtime_error("HATENA_CONSUMER_KEY is required");
    }
    _hatena_consumer_key = key_env;

    const char* secret_env = std::getenv("HATENA_CONSUMER_SECRET");
    if (secret_env == nullptr || *secret_env == '\0') {
        throw std::runtime_error("HATENA_CONSUMER_SECRET is required");
    }
    _hatena_consumer_secret = secret_env;

    const char* token_env = std::getenv("HATENA_ACCESS_TOKEN");
    if (token_env != nullptr && *token_env != '\0') {
        _hatena_access_token = token_env;
    }

    const char* token_secret_env = std::getenv("HATENA_ACCESS_TOKEN_SECRET");
    if (token_secret_env != nullptr && *token_secret_env != '\0') {
        _hatena_access_token_secret = token_secret_env;
    }

    const char* slack_token_env = std::getenv("SLACK_OUTGOING_TOKEN");
    if (slack_token_env == nullptr || *slack_token_env == '\0') {
        throw std::runtime_error("SLACK_OUTGOING_TOKEN is required");
    }
    _slack_outgoing_token = slack_token_env;
}

std::uint16_t Config::listen_port() const { return _listen_port; }
const std::string& Config::listen_address() const { return _listen_address; }
const std::string& Config::hatena_consumer_key() const { return _hatena_consumer_key; }
const std::string& Config::hatena_consumer_secret() const { return _hatena_consumer_secret; }
const std::string& Config::hatena_access_token() const { return _hatena_access_token; }
const std::string& Config::hatena_access_token_secret() const { return _hatena_access_token_secret; }
const std::string& Config::slack_outgoing_token() const { return _slack_outgoing_token; }

} // namespace chatwork
