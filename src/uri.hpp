#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace chatwork {

class URI {
public:
    static std::vector<std::string> extract(std::string_view text);
};

} // namespace chatwork
