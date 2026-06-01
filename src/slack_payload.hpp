#pragma once
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
    std::string _body;
};

} // namespace chatwork
