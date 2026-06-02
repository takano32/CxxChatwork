#pragma once
#include "json.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace chatwork {

class SlackPayload {
public:
    explicit SlackPayload(std::string_view body);
    std::optional<std::string> challenge() const;
    std::optional<std::string> text() const;

private:
    JSON _json;
};

} // namespace chatwork
