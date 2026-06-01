#pragma once
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace chatwork {

class HatenaClient {
public:
    HatenaClient(const std::string& consumer_key, const std::string& consumer_secret);
    void oauth();
    void post_bookmark(const std::string& url,
                       const std::string& comment = {},
                       const std::vector<std::string>& tags = {});

private:
    std::string consumer_key;
    std::string consumer_secret;
    std::string request_token;
    std::string request_token_secret;

    std::string oauth_authorization_header(
        std::string_view method,
        std::string_view url,
        const std::map<std::string, std::string>& body_params,
        const std::map<std::string, std::string>& extra_oauth_params) const;
};

} // namespace chatwork
