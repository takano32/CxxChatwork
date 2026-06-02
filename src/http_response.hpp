#pragma once
#include <string_view>

namespace chatwork {

class HttpResponse {
public:
    static void ok(int fd, std::string_view body = "ok\n");
    static void unauthorized(int fd, std::string_view body = "unauthorized\n");
    static void not_found(int fd, std::string_view body = "not found\n");
};

} // namespace chatwork
