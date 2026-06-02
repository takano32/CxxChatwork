#pragma once
#include <string>
#include <string_view>

namespace chatwork {

// Slack Outgoing Webhook (application/x-www-form-urlencoded) のペイロード。
class SlackPayload {
public:
    explicit SlackPayload(std::string_view body);
    const std::string& text() const;
    const std::string& user_id() const;
    const std::string& token() const;

private:
    std::string _text;
    std::string _user_id;
    std::string _token;
};

} // namespace chatwork
