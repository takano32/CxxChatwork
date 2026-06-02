#include "uri.hpp"
#include <regex>
#include <unordered_set>

namespace chatwork {

std::vector<std::string> URI::extract_url(std::string_view text) {
    // Slack はリンクを <URL|表示テキスト> 形式で送るため、'|' も URL の区切りとして除外する
    static const std::regex pattern(
        R"(https?://[^\s"'<>|]+)");

    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    auto it = std::cregex_iterator(text.data(), text.data() + text.size(), pattern);
    const auto end = std::cregex_iterator{};

    for (; it != end; ++it) {
        std::string url = it->str();
        // trailing punctuation that belongs to surrounding sentence, not the URL
        while (!url.empty() && (url.back() == '.' || url.back() == ',' ||
                                url.back() == ')' || url.back() == ']')) {
            url.pop_back();
        }
        if (!url.empty() && seen.insert(url).second) {
            result.push_back(std::move(url));
        }
    }

    return result;
}

} // namespace chatwork
