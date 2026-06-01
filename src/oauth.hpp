#pragma once
#include <map>
#include <string>
#include <string_view>

namespace chatwork {

class OAuth {
public:
    OAuth(const std::string& consumer_key, const std::string& consumer_secret);
    void authorize();
    void set_tokens(const std::string& token, const std::string& token_secret);
    const std::string& token() const;
    const std::string& token_secret() const;
    std::string authorization_header(
        std::string_view method,
        std::string_view url,
        const std::map<std::string, std::string>& body_params = {},
        const std::map<std::string, std::string>& extra_oauth_params = {}) const;

private:
    std::string _consumer_key;
    std::string _consumer_secret;
    std::string _token;
    std::string _token_secret;
};

} // namespace chatwork
