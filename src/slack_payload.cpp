#include "slack_payload.hpp"

#include <exception>

namespace chatwork {
namespace {

std::optional<std::string> string_at(const JSON& node, std::string_view key) {
    if (node.is_object() && node.contains(key)) {
        const JSON& value = node.at(key);
        if (value.is_string()) {
            return value.string();
        }
    }
    return std::nullopt;
}

std::optional<std::string> value_under(const JSON& root, std::string_view container,
                                       std::string_view key) {
    if (root.is_object() && root.contains(container)) {
        return string_at(root.at(container), key);
    }
    return std::nullopt;
}

JSON parse_body(std::string_view body) {
    try {
        return JSON::parse(body);
    } catch (const std::exception&) {
        return JSON();
    }
}

} // namespace

SlackPayload::SlackPayload(std::string_view body) : _body(body) {
    const JSON json = parse_body(body);

    _challenge = string_at(json, "challenge");

    // text フィールドが無ければ body 全体を対象とする
    _text = value_under(json, "event", "text")
                .or_else([&] { return value_under(json, "message", "text"); })
                .or_else([&] { return string_at(json, "text"); })
                .value_or(_body);

    // 送信者が無ければ空文字列
    _user_id = value_under(json, "event", "user")
                   .or_else([&] { return value_under(json, "message", "user"); })
                   .or_else([&] { return string_at(json, "user"); })
                   .value_or(std::string{});
}

const std::string& SlackPayload::body() const { return _body; }
const std::optional<std::string>& SlackPayload::challenge() const { return _challenge; }
const std::string& SlackPayload::text() const { return _text; }
const std::string& SlackPayload::user_id() const { return _user_id; }

} // namespace chatwork
