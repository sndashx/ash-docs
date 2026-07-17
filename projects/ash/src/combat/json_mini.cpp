#include "combat/json_mini.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace ash {
namespace json {

namespace {

struct Cursor {
    char const* p{};
    char const* end{};
    int         line{1};
    int         col{1};

    bool eof() const noexcept { return p >= end; }
    char peek() const noexcept { return eof() ? '\0' : *p; }

    char take() {
        if (eof()) return '\0';
        char c = *p++;
        if (c == '\n') { ++line; col = 1; } else { ++col; }
        return c;
    }

    void skip_ws() {
        while (!eof()) {
            char c = *p;
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                take();
            } else if (c == '/' && p + 1 < end && *(p + 1) == '/') {
                while (!eof() && *p != '\n') take();
            } else {
                break;
            }
        }
    }

    void expect(char c) {
        skip_ws();
        if (peek() != c) {
            std::ostringstream os;
            os << "json: expected '" << c << "' at " << line << ":" << col
               << ", got '" << (eof() ? '?' : peek()) << "'";
            throw std::runtime_error(os.str());
        }
        take();
    }

    std::string read_string() {
        expect('"');
        std::string out;
        while (!eof()) {
            char c = take();
            if (c == '"') return out;
            if (c == '\\') {
                char e = take();
                switch (e) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'n':  out += '\n'; break;
                    case 't':  out += '\t'; break;
                    case 'r':  out += '\r'; break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    default:   out += e;
                }
            } else {
                out += c;
            }
        }
        throw std::runtime_error("json: unterminated string");
    }

    std::string read_keyword(char const* kw) {
        for (; *kw; ++kw) {
            if (take() != *kw) {
                throw std::runtime_error(std::string("json: expected keyword ") + kw);
            }
        }
        return kw;
    }

    Value read_value();
    Value read_object() {
        expect('{');
        Object o;
        skip_ws();
        if (peek() == '}') { take(); return Value{std::move(o)}; }
        while (true) {
            skip_ws();
            std::string key = read_string();
            skip_ws();
            expect(':');
            Value v = read_value();
            o.emplace(std::move(key), std::move(v));
            skip_ws();
            char c = peek();
            if (c == ',') { take(); continue; }
            if (c == '}') { take(); break; }
            throw std::runtime_error("json: expected ',' or '}' in object");
        }
        return Value{std::move(o)};
    }
    Value read_array() {
        expect('[');
        Array a;
        skip_ws();
        if (peek() == ']') { take(); return Value{std::move(a)}; }
        while (true) {
            a.push_back(read_value());
            skip_ws();
            char c = peek();
            if (c == ',') { take(); continue; }
            if (c == ']') { take(); break; }
            throw std::runtime_error("json: expected ',' or ']' in array");
        }
        return Value{std::move(a)};
    }
    Value read_number() {
        std::string buf;
        if (peek() == '-') buf += take();
        while (!eof()) {
            char c = peek();
            if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' ||
                c == '+' || c == '-') {
                buf += take();
            } else break;
        }
        try {
            return Value{std::stod(buf)};
        } catch (...) {
            throw std::runtime_error("json: bad number '" + buf + "'");
        }
    }
};

Value Cursor::read_value() {
    skip_ws();
    char c = peek();
    if (c == '{') return read_object();
    if (c == '[') return read_array();
    if (c == '"') return Value{read_string()};
    if (c == 't') { read_keyword("true");  return Value{true}; }
    if (c == 'f') { read_keyword("false"); return Value{false}; }
    if (c == 'n') { read_keyword("null");  return Value{}; }
    if (c == '-' || (c >= '0' && c <= '9')) return read_number();
    throw std::runtime_error("json: unexpected character");
}

}  // namespace

Value parse(std::string const& text) {
    Cursor c{text.data(), text.data() + text.size(), 1, 1};
    c.skip_ws();
    Value v = c.read_value();
    c.skip_ws();
    if (!c.eof()) {
        throw std::runtime_error("json: trailing content after root value");
    }
    return v;
}

}  // namespace json
}  // namespace ash