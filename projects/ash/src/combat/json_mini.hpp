#pragma once
/// Phase 07: minimal JSON parser for the small, well-shaped content files
/// used by combat (`weapons.json`, `armor.json`). Pulled in only when
/// the engine wants to load content — the rest of the codebase stays
/// free of JSON dependencies. Supports: objects, arrays, strings,
/// numbers, booleans, null. Sufficient for our content schemas; not a
/// general-purpose parser.
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace ash {
namespace json {

struct Value;
using Object = std::map<std::string, Value>;
using Array  = std::vector<Value>;

struct Value {
    using Storage = std::variant<std::monostate, bool, double, std::string, Array, Object>;
    Storage data{};

    bool        is_null()   const noexcept { return std::holds_alternative<std::monostate>(data); }
    bool        is_bool()   const noexcept { return std::holds_alternative<bool>(data); }
    bool        is_number() const noexcept { return std::holds_alternative<double>(data); }
    bool        is_string() const noexcept { return std::holds_alternative<std::string>(data); }
    bool        is_array()  const noexcept { return std::holds_alternative<Array>(data); }
    bool        is_object() const noexcept { return std::holds_alternative<Object>(data); }

    bool        as_bool()   const { return std::get<bool>(data); }
    double      as_number() const { return std::get<double>(data); }
    int         as_int()    const { return static_cast<int>(std::get<double>(data)); }
    std::string const& as_string() const { return std::get<std::string>(data); }
    Array const&       as_array()  const { return std::get<Array>(data); }
    Object const&      as_object() const { return std::get<Object>(data); }

    /// Convenience: look up a key when this value is an object. Returns
    /// nullptr if the value is not an object or the key is absent.
    Value const* get(char const* key) const noexcept;
    Value const* get(std::string const& key) const noexcept;
};

/// Parse a UTF-8 JSON document. Throws std::runtime_error on malformed input.
Value parse(std::string const& text);

/// Convenience helpers used by content loaders.
inline Value const* Value::get(char const* key) const noexcept {
    if (!is_object()) return nullptr;
    auto const& o = std::get<Object>(data);
    auto it = o.find(key);
    if (it == o.end()) return nullptr;
    return &it->second;
}
inline Value const* Value::get(std::string const& key) const noexcept {
    if (!is_object()) return nullptr;
    auto const& o = std::get<Object>(data);
    auto it = o.find(key);
    if (it == o.end()) return nullptr;
    return &it->second;
}

}  // namespace json
}  // namespace ash