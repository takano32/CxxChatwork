#include "hatena_client.hpp"

namespace chatwork {

HatenaClient::HatenaClient(OAuth oauth) : _oauth(std::move(oauth)) {}

} // namespace chatwork
