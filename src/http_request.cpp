#include "http_request.hpp"

#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <sys/socket.h>

namespace chatwork {
namespace {

std::optional<std::size_t> content_length(std::string_view headers) {
    constexpr std::string_view key = "content-length:";

    std::size_t line_start = 0;
    while (line_start < headers.size()) {
        const std::size_t line_end = headers.find("\r\n", line_start);
        std::string_view line = headers.substr(
            line_start,
            line_end == std::string_view::npos ? std::string_view::npos : line_end - line_start);

        std::string lower;
        lower.reserve(line.size());
        for (char c : line) {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        if (lower.starts_with(key)) {
            line.remove_prefix(key.size());
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
                line.remove_prefix(1);
            }
            std::size_t value = 0;
            const auto result = std::from_chars(line.data(), line.data() + line.size(), value);
            if (result.ec == std::errc{}) {
                return value;
            }
        }

        if (line_end == std::string_view::npos) break;
        line_start = line_end + 2;
    }
    return std::nullopt;
}

} // namespace

HttpRequest::HttpRequest(int fd) {
    std::array<char, 4096> buffer{};
    std::optional<std::size_t> expected_body_size;
    _body_offset = std::string::npos;

    while (true) {
        const ssize_t n = ::recv(fd, buffer.data(), buffer.size(), 0);
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::string("recv failed: ") + std::strerror(errno));
        }
        _raw.append(buffer.data(), static_cast<std::size_t>(n));

        if (_body_offset == std::string::npos) {
            const std::size_t header_end = _raw.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                _body_offset = header_end + 4;
                expected_body_size = content_length(std::string_view(_raw).substr(0, header_end));
                if (!expected_body_size.has_value()) break;
            }
        }

        if (expected_body_size.has_value() && _raw.size() >= _body_offset + *expected_body_size) {
            break;
        }
    }
}

std::string_view HttpRequest::body() const {
    if (_body_offset == std::string::npos || _body_offset > _raw.size()) return {};
    return std::string_view(_raw).substr(_body_offset);
}

} // namespace chatwork
