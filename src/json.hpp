#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace chatwork {

class JSON {
public:
    using Object = std::unordered_map<std::string, JSON>;
    using Array = std::vector<JSON>;
    using Value = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, Array, Object>;

    JSON();
    explicit JSON(Value value);

    static JSON parse(std::string_view text);

    bool is_null() const;
    bool is_boolean() const;
    bool is_number() const;
    bool is_integer() const;
    bool is_double() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    bool boolean() const;
    std::int64_t integer() const;
    double number() const;
    const std::string& string() const;
    const Array& array() const;
    const Object& json() const;

    bool contains(std::string_view key) const;
    const JSON& at(std::string_view key) const;

private:
    Value _value;
};

} // namespace chatwork
