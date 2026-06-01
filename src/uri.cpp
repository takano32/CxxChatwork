#include "uri.hpp"
#include <regex>

namespace chatwork {

std::vector<std::string> URI::extract(std::string_view text) {
    static const std::regex pattern(
        R"([a-zA-Z][a-zA-Z0-9+\-.]*://[^\s"'<>]+)");

    std::vector<std::string> result;
    auto it = std::cregex_iterator(text.data(), text.data() + text.size(), pattern);
    const auto end = std::cregex_iterator{};

    for (; it != end; ++it) {
        std::string uri = it->str();
        // trailing punctuation that belongs to surrounding sentence, not the URI
        while (!uri.empty() && (uri.back() == '.' || uri.back() == ',' ||
                                uri.back() == ')' || uri.back() == ']')) {
            uri.pop_back();
        }
        if (!uri.empty()) {
            result.push_back(std::move(uri));
        }
    }

    return result;
}

} // namespace chatwork
