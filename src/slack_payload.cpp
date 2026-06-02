#include "slack_payload.hpp"

#include <cstdlib>
#include <unordered_map>

namespace chatwork {
namespace {

std::string url_decode(std::string_view s) {
    std::string result;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            const char hex[3] = {s[i + 1], s[i + 2], '\0'};
            result += static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2;
        } else if (s[i] == '+') {
            result += ' ';
        } else {
            result += s[i];
        }
    }
    return result;
}

std::unordered_map<std::string, std::string> parse_form(std::string_view body) {
    std::unordered_map<std::string, std::string> fields;
    std::size_t pos = 0;
    while (pos < body.size()) {
        const std::size_t amp = body.find('&', pos);
        const std::string_view pair =
            body.substr(pos, amp == std::string_view::npos ? std::string_view::npos : amp - pos);
        const std::size_t eq = pair.find('=');
        if (eq != std::string_view::npos) {
            fields.insert_or_assign(url_decode(pair.substr(0, eq)), url_decode(pair.substr(eq + 1)));
        }
        if (amp == std::string_view::npos) break;
        pos = amp + 1;
    }
    return fields;
}

std::string field_or_empty(const std::unordered_map<std::string, std::string>& fields,
                           const std::string& key) {
    const auto it = fields.find(key);
    return it != fields.end() ? it->second : std::string{};
}

// Slack はテキスト中の & < > を &amp; &lt; &gt; にエスケープして送るため戻す。
// & を最後に戻すことで &amp;lt; のような二重エスケープも正しく復元する。
std::string unescape_html(std::string s) {
    const auto replace_all = [&s](std::string_view from, char to) {
        std::size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), 1, to);
            pos += 1;
        }
    };
    replace_all("&lt;", '<');
    replace_all("&gt;", '>');
    replace_all("&amp;", '&');
    return s;
}

} // namespace

SlackPayload::SlackPayload(std::string_view body) {
    const auto fields = parse_form(body);
    _text = unescape_html(field_or_empty(fields, "text"));
    _user_id = field_or_empty(fields, "user_id");
    _token = field_or_empty(fields, "token");
}

const std::string& SlackPayload::text() const { return _text; }
const std::string& SlackPayload::user_id() const { return _user_id; }
const std::string& SlackPayload::token() const { return _token; }

} // namespace chatwork
