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

std::optional<std::string> text_under(const JSON& root, std::string_view container) {
    if (root.is_object() && root.contains(container)) {
        return string_at(root.at(container), "text");
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

SlackPayload::SlackPayload(std::string_view body) : _json(parse_body(body)) {}

std::optional<std::string> SlackPayload::challenge() const {
    return string_at(_json, "challenge");
}

std::optional<std::string> SlackPayload::text() const {
    return text_under(_json, "event")
        .or_else([this] { return text_under(_json, "message"); })
        .or_else([this] { return string_at(_json, "text"); });
}

} // namespace chatwork
