#include "slack_payload.hpp"

#include <cctype>

namespace chatwork {
namespace {

std::optional<std::string> parse_json_string_at(std::string_view json, std::size_t quote_pos) {
    if (quote_pos >= json.size() || json[quote_pos] != '"') {
        return std::nullopt;
    }

    std::string value;
    for (std::size_t i = quote_pos + 1; i < json.size(); ++i) {
        const char c = json[i];
        if (c == '"') return value;
        if (c != '\\') { value.push_back(c); continue; }

        if (++i >= json.size()) return std::nullopt;

        switch (json[i]) {
        case '"': case '\\': case '/': value.push_back(json[i]); break;
        case 'b': value.push_back('\b'); break;
        case 'f': value.push_back('\f'); break;
        case 'n': value.push_back('\n'); break;
        case 'r': value.push_back('\r'); break;
        case 't': value.push_back('\t'); break;
        case 'u':
            value.append("\\u");
            if (i + 4 >= json.size()) return std::nullopt;
            value.append(json.substr(i + 1, 4));
            i += 4;
            break;
        default: value.push_back(json[i]); break;
        }
    }
    return std::nullopt;
}

std::optional<std::string> find_json_string_value(std::string_view json, std::string_view key) {
    const std::string quoted_key = "\"" + std::string(key) + "\"";
    std::size_t pos = 0;
    while ((pos = json.find(quoted_key, pos)) != std::string_view::npos) {
        std::size_t colon = pos + quoted_key.size();
        while (colon < json.size() && std::isspace(static_cast<unsigned char>(json[colon]))) {
            ++colon;
        }
        if (colon >= json.size() || json[colon] != ':') {
            pos += quoted_key.size();
            continue;
        }
        std::size_t value_pos = colon + 1;
        while (value_pos < json.size() && std::isspace(static_cast<unsigned char>(json[value_pos]))) {
            ++value_pos;
        }
        if (value_pos < json.size() && json[value_pos] == '"') {
            return parse_json_string_at(json, value_pos);
        }
        pos += quoted_key.size();
    }
    return std::nullopt;
}

} // namespace

SlackPayload::SlackPayload(std::string_view body) : _body(body) {}

std::optional<std::string> SlackPayload::challenge() const {
    return find_json_string_value(_body, "challenge");
}

std::optional<std::string> SlackPayload::text() const {
    if (auto pos = _body.find("\"event\""); pos != std::string::npos) {
        if (auto t = find_json_string_value(std::string_view(_body).substr(pos), "text")) {
            return t;
        }
    }
    if (auto pos = _body.find("\"message\""); pos != std::string::npos) {
        if (auto t = find_json_string_value(std::string_view(_body).substr(pos), "text")) {
            return t;
        }
    }
    return find_json_string_value(_body, "text");
}

} // namespace chatwork
