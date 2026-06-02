#pragma once
#include "oauth.hpp"
#include <string>

namespace chatwork {

class HatenaClient {
public:
    explicit HatenaClient(OAuth oauth);
    virtual ~HatenaClient() = default;

protected:
    OAuth _oauth;
};

} // namespace chatwork
