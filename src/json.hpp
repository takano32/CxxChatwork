#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace chatwork {

std::optional<std::string> find_json_string_value(std::string_view json, std::string_view key);
std::optional<std::string> slack_message_text(std::string_view body);

} // namespace chatwork
