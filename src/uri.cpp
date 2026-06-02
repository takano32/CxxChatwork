#include "uri.hpp"
#include <regex>

namespace chatwork {

std::vector<std::string> URI::extract_url(std::string_view text) {
    static const std::regex pattern(
        R"(https?://[^\s"'<>]+)");

    std::vector<std::string> result;
    auto it = std::cregex_iterator(text.data(), text.data() + text.size(), pattern);
    const auto end = std::cregex_iterator{};

    for (; it != end; ++it) {
        std::string url = it->str();
        // trailing punctuation that belongs to surrounding sentence, not the URL
        while (!url.empty() && (url.back() == '.' || url.back() == ',' ||
                                url.back() == ')' || url.back() == ']')) {
            url.pop_back();
        }
        if (!url.empty()) {
            result.push_back(std::move(url));
        }
    }

    return result;
}

} // namespace chatwork
