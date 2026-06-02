#pragma once
#include <string>
#include <vector>

namespace chatwork {

class HatenaBookmark {
public:
    HatenaBookmark(std::string url, std::string comment, std::vector<std::string> tags);

    const std::string& url() const;
    const std::string& comment() const;
    const std::vector<std::string>& tags() const;

private:
    std::string _url;
    std::string _comment;
    std::vector<std::string> _tags;
};

} // namespace chatwork
