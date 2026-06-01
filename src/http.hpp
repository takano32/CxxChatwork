#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <cstddef>

namespace chatwork {

std::optional<std::size_t> content_length(std::string_view headers);
std::string read_http_request(int client_fd);
std::string_view http_body(std::string_view request);
void send_all(int fd, std::string_view data);
void respond_ok(int client_fd, std::string_view body = "ok\n");

} // namespace chatwork
