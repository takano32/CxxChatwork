#pragma once
#include <cstddef>
#include <string>
#include <string_view>

namespace chatwork {

class HttpRequest {
public:
    explicit HttpRequest(int fd);
    std::string_view method() const;
    std::string_view target() const;
    std::string_view body() const;

private:
    std::string _raw;
    std::string _method;
    std::string _target;
    std::size_t _body_offset{0};
};

} // namespace chatwork
