#include "quest/flag_store.hpp"

#include <cstdio>

#include <sstream>
#include <stdexcept>

namespace ash {
namespace quest {

namespace {

// JSON escaper: escapes \, ", control chars for strings; emits "null" for
// missing values. Kept intentionally tiny — we control both ends.
std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
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

std::string value_to_json(const FlagValue& v) {
    return std::visit([](auto&& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, bool>) {
            return x ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return std::to_string(x);
        } else {
            return "\"" + json_escape(x) + "\"";
        }
    }, v);
}

// Minimal hand-rolled JSON parser for our own output: keys are strings,
// values are int/bool/string, no nested arrays/objects beyond the top
// {"flag": {"type": "int", "value": 7}} structure.
struct Cursor {
    std::string s;
    std::size_t i = 0;
    void skip_ws() {
        while (i < s.size() &&
               (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) {
            ++i;
        }
    }
    bool eat(char c) {
        skip_ws();
        if (i < s.size() && s[i] == c) { ++i; return true; }
        return false;
    }
    bool peek(char c) {
        skip_ws();
        return i < s.size() && s[i] == c;
    }
    bool eof() { skip_ws(); return i >= s.size(); }
    std::string parse_string() {
        skip_ws();
        if (i >= s.size() || s[i] != '"') throw std::runtime_error("expected string");
        ++i;
        std::string out;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                char nx = s[i + 1];
                switch (nx) {
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    default:   out += nx;   break;
                }
                i += 2;
            } else {
                out += s[i++];
            }
        }
        if (i >= s.size()) throw std::runtime_error("unterminated string");
        ++i;
        return out;
    }
    std::int64_t parse_int() {
        skip_ws();
        bool neg = false;
        if (i < s.size() && s[i] == '-') { neg = true; ++i; }
        std::int64_t v = 0;
        bool any = false;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            v = v * 10 + (s[i] - '0');
            ++i;
            any = true;
        }
        if (!any) throw std::runtime_error("expected int");
        return neg ? -v : v;
    }
    bool parse_bool() {
        skip_ws();
        if (i + 4 <= s.size() && s.compare(i, 4, "true") == 0)  { i += 4; return true;  }
        if (i + 5 <= s.size() && s.compare(i, 5, "false") == 0) { i += 5; return false; }
        throw std::runtime_error("expected bool");
    }
};

}  // namespace

void FlagStore::set_int(const std::string& key, std::int64_t v) {
    auto prev = flags_.count(key) ? std::optional<FlagValue>{flags_[key]}
                                  : std::nullopt;
    flags_[key] = v;
    notify(key, prev, flags_[key]);
}

void FlagStore::set_bool(const std::string& key, bool v) {
    auto prev = flags_.count(key) ? std::optional<FlagValue>{flags_[key]}
                                  : std::nullopt;
    flags_[key] = v;
    notify(key, prev, flags_[key]);
}

void FlagStore::set_string(const std::string& key, std::string v) {
    auto prev = flags_.count(key) ? std::optional<FlagValue>{flags_[key]}
                                  : std::nullopt;
    flags_[key] = std::move(v);
    notify(key, prev, flags_[key]);
}

void FlagStore::set(const std::string& key, FlagValue v) {
    auto prev = flags_.count(key) ? std::optional<FlagValue>{flags_[key]}
                                  : std::nullopt;
    flags_[key] = std::move(v);
    notify(key, prev, flags_[key]);
}

void FlagStore::clear(const std::string& key) {
    auto it = flags_.find(key);
    if (it == flags_.end()) return;
    auto prev = it->second;
    flags_.erase(it);
    notify(key, prev, FlagValue{std::int64_t{0}});
}

void FlagStore::increment(const std::string& key, std::int64_t delta) {
    std::int64_t cur = 0;
    auto it = flags_.find(key);
    if (it != flags_.end()) {
        std::visit([&](auto&& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, bool>) {
                cur = x ? 1 : 0;
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                cur = x;
            } else {
                try { cur = static_cast<std::int64_t>(std::stoll(x)); }
                catch (...) { cur = 0; }
            }
        }, it->second);
    }
    auto prev = (it != flags_.end()) ? std::optional<FlagValue>{it->second}
                                     : std::nullopt;
    flags_[key] = cur + delta;
    notify(key, prev, flags_[key]);
}

void FlagStore::decrement(const std::string& key, std::int64_t delta) {
    increment(key, -delta);
}

void FlagStore::toggle(const std::string& key) {
    bool cur = false;
    auto it = flags_.find(key);
    if (it != flags_.end()) {
        std::visit([&](auto&& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, bool>) {
                cur = x;
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                cur = x != 0;
            } else {
                cur = !x.empty();
            }
        }, it->second);
    }
    auto prev = (it != flags_.end()) ? std::optional<FlagValue>{it->second}
                                     : std::nullopt;
    flags_[key] = !cur;
    notify(key, prev, flags_[key]);
}

std::optional<std::int64_t> FlagStore::get_int(const std::string& key) const {
    auto it = flags_.find(key);
    if (it == flags_.end()) return std::nullopt;
    if (auto* p = std::get_if<std::int64_t>(&it->second)) return *p;
    return std::nullopt;
}

std::optional<bool> FlagStore::get_bool(const std::string& key) const {
    auto it = flags_.find(key);
    if (it == flags_.end()) return std::nullopt;
    if (auto* p = std::get_if<bool>(&it->second)) return *p;
    return std::nullopt;
}

std::optional<std::string> FlagStore::get_string(const std::string& key) const {
    auto it = flags_.find(key);
    if (it == flags_.end()) return std::nullopt;
    if (auto* p = std::get_if<std::string>(&it->second)) return *p;
    return std::nullopt;
}

std::optional<FlagValue> FlagStore::get(const std::string& key) const {
    auto it = flags_.find(key);
    if (it == flags_.end()) return std::nullopt;
    return it->second;
}

bool FlagStore::has(const std::string& key) const {
    return flags_.count(key) > 0;
}

void FlagStore::reset_all() {
    flags_.clear();
    observers_.clear();
}

std::size_t FlagStore::add_observer(FlagObserver obs) {
    observers_.push_back({std::move(obs), true});
    return observers_.size() - 1;
}

void FlagStore::remove_observer(std::size_t handle) {
    if (handle < observers_.size()) observers_[handle].second = false;
}

void FlagStore::notify(const std::string& key,
                       const std::optional<FlagValue>& previous,
                       const FlagValue& current) {
    FlagChange ch{key, previous, current};
    for (auto& [obs, alive] : observers_) {
        if (alive) obs(ch);
    }
}

std::string FlagStore::to_json() const {
    std::ostringstream os;
    os << "{";
    bool first = true;
    for (auto& [k, v] : flags_) {
        if (!first) os << ",";
        first = false;
        os << "\"" << json_escape(k) << "\":";
        std::string type;
        std::visit([&](auto&& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, bool>) type = "bool";
            else if constexpr (std::is_same_v<T, std::int64_t>) type = "int";
            else type = "string";
        }, v);
        os << "{\"type\":\"" << type << "\",\"value\":" << value_to_json(v) << "}";
    }
    os << "}";
    return os.str();
}

void FlagStore::from_json(const std::string& blob) {
    flags_.clear();
    Cursor c{blob};
    if (!c.eat('{')) throw std::runtime_error("expected {");
    while (!c.eat('}')) {
        auto k = c.parse_string();
        if (!c.eat(':')) throw std::runtime_error("expected :");
        if (!c.eat('{')) throw std::runtime_error("expected {");
        auto type_key = c.parse_string();   // "type"
        if (!c.eat(':')) throw std::runtime_error("expected :");
        auto type = c.parse_string();
        if (!c.eat(',')) throw std::runtime_error("expected ,");
        auto val_key = c.parse_string();    // "value"
        if (!c.eat(':')) throw std::runtime_error("expected :");
        if (type == "int") {
            auto v = c.parse_int();
            flags_[k] = v;
        } else if (type == "bool") {
            auto v = c.parse_bool();
            flags_[k] = v;
        } else if (type == "string") {
            auto v = c.parse_string();
            flags_[k] = v;
        } else {
            throw std::runtime_error("unknown flag type: " + type);
        }
        if (!c.eat('}')) throw std::runtime_error("expected }");
        if (!c.eof()) c.eat(',');  // optional trailing comma
        (void)type_key;
        (void)val_key;
    }
}

}  // namespace quest
}  // namespace ash