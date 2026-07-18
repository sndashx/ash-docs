#include "core/json.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <stdexcept>

namespace ash {
namespace json {

namespace {

std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x",
                                  static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void dump_pretty(std::ostream& os, const Value& v, int indent);

void dump_compact(std::ostream& os, const Value& v) {
    switch (v.type()) {
        case Value::Kind::Null:    os << "null"; break;
        case Value::Kind::Bool:    os << (v.as_bool() ? "true" : "false"); break;
        case Value::Kind::Int:     os << v.as_int(); break;
        case Value::Kind::Double: {
            double d = v.as_double();
            if (std::isfinite(d)) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.17g", d);
                os << buf;
            } else {
                os << "null";
            }
            break;
        }
        case Value::Kind::String:  os << '"' << escape(v.as_string()) << '"'; break;
        case Value::Kind::Arr: {
            os << '[';
            bool first = true;
            for (auto const& e : v.as_array()) {
                if (!first) os << ',';
                first = false;
                dump_compact(os, e);
            }
            os << ']';
            break;
        }
        case Value::Kind::Obj: {
            os << '{';
            bool first = true;
            for (auto const& [k, e] : v.as_object()) {
                if (!first) os << ',';
                first = false;
                os << '"' << escape(k) << "\":";
                dump_compact(os, e);
            }
            os << '}';
            break;
        }
    }
}

void indent_str(std::ostream& os, int n) {
    for (int i = 0; i < n; ++i) os << ' ';
}

void dump_pretty_inner(std::ostream& os, const Value& v, int indent, bool at_root) {
    (void)at_root;
    switch (v.type()) {
        case Value::Kind::Null:    os << "null"; break;
        case Value::Kind::Bool:    os << (v.as_bool() ? "true" : "false"); break;
        case Value::Kind::Int:     os << v.as_int(); break;
        case Value::Kind::Double: {
            double d = v.as_double();
            if (std::isfinite(d)) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.17g", d);
                os << buf;
            } else {
                os << "null";
            }
            break;
        }
        case Value::Kind::String:  os << '"' << escape(v.as_string()) << '"'; break;
        case Value::Kind::Arr: {
            auto& a = v.as_array();
            if (a.empty()) { os << "[]"; break; }
            os << "[\n";
            for (std::size_t i = 0; i < a.size(); ++i) {
                indent_str(os, indent + 2);
                dump_pretty_inner(os, a[i], indent + 2, false);
                if (i + 1 < a.size()) os << ',';
                os << '\n';
            }
            indent_str(os, indent);
            os << ']';
            break;
        }
        case Value::Kind::Obj: {
            auto& o = v.as_object();
            if (o.empty()) { os << "{}"; break; }
            os << "{\n";
            std::size_t i = 0;
            for (auto const& [k, e] : o) {
                indent_str(os, indent + 2);
                os << '"' << escape(k) << "\": ";
                dump_pretty_inner(os, e, indent + 2, false);
                if (++i < o.size()) os << ',';
                os << '\n';
            }
            indent_str(os, indent);
            os << '}';
            break;
        }
    }
}

void dump_pretty(std::ostream& os, const Value& v, int indent) {
    dump_pretty_inner(os, v, indent, true);
}

// ---- Parser ----

struct Cursor {
    const std::string& s;
    std::size_t i = 0;
    int line = 1;
    int col = 1;

    void skip() {
        while (i < s.size()) {
            char c = s[i];
            if (c == ' ' || c == '\t') { ++i; ++col; }
            else if (c == '\n')        { ++i; ++line; col = 1; }
            else if (c == '\r')        { ++i; col = 1; }
            else break;
        }
    }

    bool eof() { skip(); return i >= s.size(); }

    [[noreturn]] void die(const std::string& msg) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "json parse error at %d:%d: %s",
                      line, col, msg.c_str());
        throw std::runtime_error(buf);
    }

    char peek() { skip(); if (i >= s.size()) die("unexpected eof"); return s[i]; }
    char take() { skip(); if (i >= s.size()) die("unexpected eof"); char c = s[i++]; if (c=='\n') {++line; col=1;} else ++col; return c; }
    bool eat(char c) { skip(); if (i < s.size() && s[i] == c) { take(); return true; } return false; }

    std::string parse_string() {
        skip();
        if (i >= s.size() || s[i] != '"') die("expected string");
        ++i; ++col;
        std::string out;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\') {
                if (i + 1 >= s.size()) die("bad escape");
                char nx = s[i + 1];
                switch (nx) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        if (i + 5 >= s.size()) die("bad \\u escape");
                        unsigned cp = 0;
                        for (int k = 0; k < 4; ++k) {
                            char h = s[i + static_cast<std::size_t>(2) + static_cast<std::size_t>(k)];
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp |= static_cast<unsigned>(h - '0');
                            else if (h >= 'a' && h <= 'f') cp |= static_cast<unsigned>(10 + (h - 'a'));
                            else if (h >= 'A' && h <= 'F') cp |= static_cast<unsigned>(10 + (h - 'A'));
                            else die("bad hex digit");
                        }
                        // Encode as UTF-8.
                        if (cp < 0x80) {
                            out += static_cast<char>(cp);
                        } else if (cp < 0x800) {
                            out += static_cast<char>(0xC0 | (cp >> 6));
                            out += static_cast<char>(0x80 | (cp & 0x3F));
                        } else {
                            out += static_cast<char>(0xE0 | (cp >> 12));
                            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        i += 4;
                        break;
                    }
                    default: die("unknown escape");
                }
                i += 2; col += 2;
            } else if (static_cast<unsigned char>(s[i]) < 0x20) {
                die("control char in string");
            } else {
                out += s[i++]; ++col;
            }
        }
        if (i >= s.size()) die("unterminated string");
        ++i; ++col;
        return out;
    }

    Value parse_number() {
        skip();
        std::size_t start = i;
        if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
        bool is_double = false;
        while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])))) ++i;
        if (i < s.size() && s[i] == '.') { is_double = true; ++i;
            while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])))) ++i;
        }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
            is_double = true; ++i;
            if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
            while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])))) ++i;
        }
        if (i == start) die("expected number");
        std::string tok = s.substr(start, i - start);
        col += static_cast<int>(i - start);
        try {
            if (is_double) {
                return Value(std::stod(tok));
            }
            return Value(static_cast<std::int64_t>(std::stoll(tok)));
        } catch (...) {
            die("bad number: " + tok);
        }
    }

    Value parse_value() {
        skip();
        if (i >= s.size()) die("unexpected eof");
        char c = s[i];
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return Value(parse_string());
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        return parse_number();
    }

    Value parse_object() {
        ++i; ++col; // consume {
        Object o;
        skip();
        if (i < s.size() && s[i] == '}') { ++i; ++col; return Value(std::move(o)); }
        while (true) {
            skip();
            if (i >= s.size() || s[i] != '"') die("expected key string");
            auto k = parse_string();
            if (!eat(':')) die("expected :");
            o.emplace(std::move(k), parse_value());
            skip();
            if (i < s.size() && s[i] == ',') { take(); continue; }
            if (i < s.size() && s[i] == '}') { take(); break; }
            die("expected , or }");
        }
        return Value(std::move(o));
    }

    Value parse_array() {
        ++i; ++col;
        Array a;
        skip();
        if (i < s.size() && s[i] == ']') { ++i; ++col; return Value(std::move(a)); }
        while (true) {
            a.push_back(parse_value());
            skip();
            if (i < s.size() && s[i] == ',') { take(); continue; }
            if (i < s.size() && s[i] == ']') { take(); break; }
            die("expected , or ]");
        }
        return Value(std::move(a));
    }

    Value parse_bool() {
        if (i + 4 <= s.size() && s.compare(i, 4, "true") == 0) { i+=4; col+=4; return Value(true); }
        if (i + 5 <= s.size() && s.compare(i, 5, "false") == 0) { i+=5; col+=5; return Value(false); }
        die("expected bool");
    }

    Value parse_null() {
        if (i + 4 <= s.size() && s.compare(i, 4, "null") == 0) { i+=4; col+=4; return Value(nullptr); }
        die("expected null");
    }
};

}  // namespace

std::string dump(const Value& v, bool pretty) {
    std::ostringstream os;
    if (pretty) {
        dump_pretty(os, v, 0);
        os << '\n';
    } else {
        dump_compact(os, v);
    }
    return os.str();
}

Value parse(const std::string& s) {
    Cursor c{s};
    Value v = c.parse_value();
    if (!c.eof()) c.die("trailing content");
    return v;
}

}  // namespace json
}  // namespace ash
