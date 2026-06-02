#pragma once
#include <cstdint>
#include <string>

namespace chatwork {

class Config {
public:
    Config();
    std::uint16_t listen_port() const;
    const std::string& listen_address() const;
    const std::string& hatena_consumer_key() const;
    const std::string& hatena_consumer_secret() const;
    const std::string& hatena_access_token() const;
    const std::string& hatena_access_token_secret() const;
    const std::string& slack_outgoing_token() const;

private:
    std::uint16_t _listen_port;
    std::string _listen_address;
    std::string _hatena_consumer_key;
    std::string _hatena_consumer_secret;
    std::string _hatena_access_token;
    std::string _hatena_access_token_secret;
    std::string _slack_outgoing_token;
};

} // namespace chatwork
