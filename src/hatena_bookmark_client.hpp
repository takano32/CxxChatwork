#pragma once
#include "hatena_bookmark.hpp"
#include "hatena_client.hpp"
#include <string>
#include <vector>

namespace chatwork {

enum class BookmarkResult { Added, Updated };

class HatenaBookmarkClient : public HatenaClient {
public:
    using HatenaClient::HatenaClient;
    void process(const std::string& url) override;
    BookmarkResult post_bookmark(const std::string& url,
                                 const std::string& comment = {},
                                 const std::vector<std::string>& tags = {});
    HatenaBookmark get_bookmark(const std::string& url);
};

} // namespace chatwork
