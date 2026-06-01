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

private:
    std::uint16_t _listen_port;
    std::string _listen_address;
    std::string _hatena_consumer_key;
    std::string _hatena_consumer_secret;
};

} // namespace chatwork
