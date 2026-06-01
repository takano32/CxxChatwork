#pragma once
#include <string_view>

namespace chatwork {

class HttpResponse {
public:
    static void ok(int fd, std::string_view body = "ok\n");
};

} // namespace chatwork
