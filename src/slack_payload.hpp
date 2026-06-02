#pragma once
#include "json.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace chatwork {

class SlackPayload {
public:
    explicit SlackPayload(std::string_view body);
    const std::string& body() const;
    const std::optional<std::string>& challenge() const;
    const std::string& text() const;
    const std::string& user_id() const;

private:
    std::string _body;
    std::optional<std::string> _challenge;
    std::string _text;
    std::string _user_id;
};

} // namespace chatwork
