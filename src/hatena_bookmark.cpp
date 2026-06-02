#include "hatena_bookmark.hpp"

#include <utility>

namespace chatwork {

HatenaBookmark::HatenaBookmark(std::string url, std::string comment, std::vector<std::string> tags)
    : _url(std::move(url)), _comment(std::move(comment)), _tags(std::move(tags)) {}

const std::string& HatenaBookmark::url() const { return _url; }
const std::string& HatenaBookmark::comment() const { return _comment; }
const std::vector<std::string>& HatenaBookmark::tags() const { return _tags; }

} // namespace chatwork
