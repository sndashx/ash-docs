#pragma once
/// Phase 10: minimal JSON library for save/settings/config files.
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace ash {
namespace json {

class Value;
using Object = std::map<std::string, Value>;
using Array  = std::vector<Value>;

class Value {
public:
    enum class Kind { Null, Bool, Int, Double, String, Arr, Obj };

    Value() noexcept : kind_(Kind::Null) {}
    Value(std::nullptr_t) noexcept : kind_(Kind::Null) {}
    Value(bool b) noexcept : kind_(Kind::Bool), bool_(b) {}
    Value(int i) noexcept : kind_(Kind::Int), int_(i) {}
    Value(long i) noexcept : kind_(Kind::Int), int_(static_cast<std::int64_t>(i)) {}
    Value(long long i) noexcept : kind_(Kind::Int), int_(static_cast<std::int64_t>(i)) {}
    Value(unsigned i) noexcept : kind_(Kind::Int), int_(static_cast<std::int64_t>(i)) {}
    Value(unsigned long i) noexcept : kind_(Kind::Int), int_(static_cast<std::int64_t>(i)) {}
    Value(unsigned long long i) noexcept : kind_(Kind::Int), int_(static_cast<std::int64_t>(i)) {}
    Value(double d) noexcept : kind_(Kind::Double), dbl_(d) {}
    Value(const char* s) : kind_(Kind::String), str_(s) {}
    Value(std::string s) : kind_(Kind::String), str_(std::move(s)) {}
    Value(Array a) : kind_(Kind::Arr), arr_(std::move(a)) {}
    Value(Object o) : kind_(Kind::Obj), obj_(std::move(o)) {}

    Kind type() const noexcept { return kind_; }

    bool is_null()   const noexcept { return kind_ == Kind::Null; }
    bool is_bool()   const noexcept { return kind_ == Kind::Bool; }
    bool is_int()    const noexcept { return kind_ == Kind::Int; }
    bool is_double() const noexcept { return kind_ == Kind::Double || kind_ == Kind::Int; }
    bool is_string() const noexcept { return kind_ == Kind::String; }
    bool is_array()  const noexcept { return kind_ == Kind::Arr; }
    bool is_object() const noexcept { return kind_ == Kind::Obj; }

    bool         as_bool()                 const { if (kind_==Kind::Bool) return bool_; throw std::runtime_error("json: not bool"); }
    std::int64_t as_int()                  const { if (kind_==Kind::Int) return int_; if (kind_==Kind::Double) return static_cast<std::int64_t>(dbl_); if (kind_==Kind::Bool) return bool_?1:0; throw std::runtime_error("json: not int"); }
    double       as_double()               const { if (kind_==Kind::Double) return dbl_; if (kind_==Kind::Int) return static_cast<double>(int_); throw std::runtime_error("json: not number"); }
    const std::string& as_string()          const { if (kind_==Kind::String) return str_; throw std::runtime_error("json: not string"); }
    const Array&  as_array()               const { if (kind_==Kind::Arr) return arr_; throw std::runtime_error("json: not array"); }
    const Object& as_object()              const { if (kind_==Kind::Obj) return obj_; throw std::runtime_error("json: not object"); }
    Array&        as_array()                     { if (kind_!=Kind::Arr) { kind_=Kind::Arr; arr_={}; } return arr_; }
    Object&       as_object()                    { if (kind_!=Kind::Obj) { kind_=Kind::Obj; obj_={}; } return obj_; }

    bool contains(const std::string& k) const {
        if (kind_ != Kind::Obj) return false;
        return obj_.count(k) > 0;
    }
    const Value& at(const std::string& k) const {
        if (kind_ != Kind::Obj) throw std::runtime_error("json: not object");
        auto it = obj_.find(k);
        if (it == obj_.end()) throw std::runtime_error("json: missing key " + k);
        return it->second;
    }
    Value& at(const std::string& k) {
        if (kind_ != Kind::Obj) { kind_=Kind::Obj; obj_={}; }
        return obj_[k];
    }
    Value& operator[](const std::string& k) { return at(k); }
    const Value& operator[](const std::string& k) const {
        static thread_local Value null_v;
        if (kind_ != Kind::Obj) return null_v;
        auto it = obj_.find(k);
        if (it == obj_.end()) return null_v;
        return it->second;
    }

    std::int64_t int_or(std::int64_t dflt) const {
        if (kind_ == Kind::Int) return int_;
        if (kind_ == Kind::Double) return static_cast<std::int64_t>(dbl_);
        if (kind_ == Kind::Bool) return bool_ ? 1 : 0;
        return dflt;
    }
    bool bool_or(bool dflt) const {
        if (kind_ == Kind::Bool) return bool_;
        if (kind_ == Kind::Int) return int_ != 0;
        return dflt;
    }
    std::string str_or(const std::string& dflt) const {
        if (kind_ == Kind::String) return str_;
        return dflt;
    }
    double dbl_or(double dflt) const {
        if (kind_ == Kind::Double) return dbl_;
        if (kind_ == Kind::Int) return static_cast<double>(int_);
        return dflt;
    }

private:
    Kind         kind_;
    bool         bool_{false};
    std::int64_t int_{0};
    double       dbl_{0.0};
    std::string  str_{};
    Array        arr_{};
    Object       obj_{};
};

/// Serialize a JSON value to a string.
std::string dump(const Value& v, bool pretty = false);

/// Parse a JSON string into a Value. Throws std::runtime_error on malformed.
Value parse(const std::string& s);

inline Value parse(const char* s) { return parse(std::string(s)); }

}  // namespace json
}  // namespace ash