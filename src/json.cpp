#include "json.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace chatwork {
namespace {

class Parser {
public:
    explicit Parser(std::string_view text) : _text(text) {}

    JSON parse() {
        skip_whitespace();
        JSON value = parse_value();
        skip_whitespace();
        if (_pos != _text.size()) {
            throw std::runtime_error("JSON: trailing characters after value");
        }
        return value;
    }

private:
    std::string_view _text;
    std::size_t _pos = 0;

    void skip_whitespace() {
        while (_pos < _text.size() && std::isspace(static_cast<unsigned char>(_text[_pos]))) {
            ++_pos;
        }
    }

    char peek() const {
        if (_pos >= _text.size()) {
            throw std::runtime_error("JSON: unexpected end of input");
        }
        return _text[_pos];
    }

    char next() {
        const char c = peek();
        ++_pos;
        return c;
    }

    JSON parse_value() {
        skip_whitespace();
        switch (peek()) {
        case '{': return parse_object();
        case '[': return parse_array();
        case '"': return JSON(JSON::Value(parse_string()));
        case 't': case 'f': return parse_boolean();
        case 'n': return parse_null();
        default: return parse_number();
        }
    }

    JSON parse_object() {
        ++_pos; // consume '{'
        JSON::Object object;
        skip_whitespace();
        if (peek() == '}') {
            ++_pos;
            return JSON(JSON::Value(std::move(object)));
        }
        while (true) {
            skip_whitespace();
            if (peek() != '"') {
                throw std::runtime_error("JSON: expected string key");
            }
            std::string key = parse_string();
            skip_whitespace();
            if (next() != ':') {
                throw std::runtime_error("JSON: expected ':' after key");
            }
            object.insert_or_assign(std::move(key), parse_value());
            skip_whitespace();
            const char c = next();
            if (c == '}') break;
            if (c != ',') {
                throw std::runtime_error("JSON: expected ',' or '}' in object");
            }
        }
        return JSON(JSON::Value(std::move(object)));
    }

    JSON parse_array() {
        ++_pos; // consume '['
        JSON::Array array;
        skip_whitespace();
        if (peek() == ']') {
            ++_pos;
            return JSON(JSON::Value(std::move(array)));
        }
        while (true) {
            array.push_back(parse_value());
            skip_whitespace();
            const char c = next();
            if (c == ']') break;
            if (c != ',') {
                throw std::runtime_error("JSON: expected ',' or ']' in array");
            }
        }
        return JSON(JSON::Value(std::move(array)));
    }

    std::string parse_string() {
        ++_pos; // consume opening '"'
        std::string value;
        while (true) {
            if (_pos >= _text.size()) {
                throw std::runtime_error("JSON: unterminated string");
            }
            const char c = _text[_pos++];
            if (c == '"') return value;
            if (c != '\\') {
                value.push_back(c);
                continue;
            }
            if (_pos >= _text.size()) {
                throw std::runtime_error("JSON: unterminated escape");
            }
            const char esc = _text[_pos++];
            switch (esc) {
            case '"': case '\\': case '/': value.push_back(esc); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            case 'u':
                value.append("\\u");
                if (_pos + 4 > _text.size()) {
                    throw std::runtime_error("JSON: invalid \\u escape");
                }
                value.append(_text.substr(_pos, 4));
                _pos += 4;
                break;
            default: value.push_back(esc); break;
            }
        }
    }

    JSON parse_boolean() {
        if (_text.compare(_pos, 4, "true") == 0) {
            _pos += 4;
            return JSON(JSON::Value(true));
        }
        if (_text.compare(_pos, 5, "false") == 0) {
            _pos += 5;
            return JSON(JSON::Value(false));
        }
        throw std::runtime_error("JSON: invalid boolean literal");
    }

    JSON parse_null() {
        if (_text.compare(_pos, 4, "null") == 0) {
            _pos += 4;
            return JSON();
        }
        throw std::runtime_error("JSON: invalid null literal");
    }

    JSON parse_number() {
        const std::size_t start = _pos;
        while (_pos < _text.size()) {
            const char c = _text[_pos];
            if (c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E'
                || std::isdigit(static_cast<unsigned char>(c))) {
                ++_pos;
            } else {
                break;
            }
        }
        if (_pos == start) {
            throw std::runtime_error("JSON: invalid number");
        }
        const std::string_view token = _text.substr(start, _pos - start);
        const std::string number(token);
        const bool is_real = token.find_first_of(".eE") != std::string_view::npos;
        if (!is_real) {
            try {
                return JSON(JSON::Value(static_cast<std::int64_t>(std::stoll(number))));
            } catch (const std::out_of_range&) {
                // value does not fit in std::int64_t; fall back to double
            }
        }
        return JSON(JSON::Value(std::stod(number)));
    }
};

} // namespace

JSON::JSON() : _value(nullptr) {}
JSON::JSON(Value value) : _value(std::move(value)) {}

JSON JSON::parse(std::string_view text) {
    return Parser(text).parse();
}

bool JSON::is_null() const { return std::holds_alternative<std::nullptr_t>(_value); }
bool JSON::is_boolean() const { return std::holds_alternative<bool>(_value); }
bool JSON::is_integer() const { return std::holds_alternative<std::int64_t>(_value); }
bool JSON::is_double() const { return std::holds_alternative<double>(_value); }
bool JSON::is_number() const { return is_integer() || is_double(); }
bool JSON::is_string() const { return std::holds_alternative<std::string>(_value); }
bool JSON::is_array() const { return std::holds_alternative<Array>(_value); }
bool JSON::is_object() const { return std::holds_alternative<Object>(_value); }

bool JSON::boolean() const { return std::get<bool>(_value); }

std::int64_t JSON::integer() const {
    if (std::holds_alternative<double>(_value)) {
        return static_cast<std::int64_t>(std::get<double>(_value));
    }
    return std::get<std::int64_t>(_value);
}

double JSON::number() const {
    if (std::holds_alternative<std::int64_t>(_value)) {
        return static_cast<double>(std::get<std::int64_t>(_value));
    }
    return std::get<double>(_value);
}

const std::string& JSON::string() const { return std::get<std::string>(_value); }
const JSON::Array& JSON::array() const { return std::get<Array>(_value); }
const JSON::Object& JSON::json() const { return std::get<Object>(_value); }

bool JSON::contains(std::string_view key) const {
    const Object& object = std::get<Object>(_value);
    return object.find(std::string(key)) != object.end();
}

const JSON& JSON::at(std::string_view key) const {
    const Object& object = std::get<Object>(_value);
    const auto it = object.find(std::string(key));
    if (it == object.end()) {
        throw std::out_of_range("JSON: key not found: " + std::string(key));
    }
    return it->second;
}

const JSON& JSON::at(std::size_t index) const {
    return std::get<Array>(_value).at(index);
}

JSON::operator const std::string&() const { return std::get<std::string>(_value); }

} // namespace chatwork
