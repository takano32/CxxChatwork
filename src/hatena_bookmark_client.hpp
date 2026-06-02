#pragma once
#include "hatena_bookmark.hpp"
#include "hatena_client.hpp"
#include <string>
#include <tuple>
#include <vector>

namespace chatwork {

enum class HatenaBookmarkResult { Added, Updated, Got };

class HatenaBookmarkClient : public HatenaClient {
public:
    using HatenaClient::HatenaClient;
    std::tuple<HatenaBookmark, HatenaBookmarkResult> post_bookmark(
        const std::string& url,
        const std::string& comment = {},
        const std::vector<std::string>& tags = {});
    std::tuple<HatenaBookmark, HatenaBookmarkResult> get_bookmark(const std::string& url);
};

} // namespace chatwork
