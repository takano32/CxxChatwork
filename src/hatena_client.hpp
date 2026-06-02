#pragma once
#include "oauth.hpp"
#include <string>

namespace chatwork {

class HatenaClient {
public:
    explicit HatenaClient(OAuth oauth);
    virtual ~HatenaClient() = default;

    virtual void process(const std::string& url) = 0;

protected:
    OAuth _oauth;
};

} // namespace chatwork
