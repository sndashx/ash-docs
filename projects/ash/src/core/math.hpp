#pragma once
/// Phase 01 step 0100: Fixed-point arithmetic, Vec2, Rect.
#include <cmath>
#include <cstdint>

namespace ash {
namespace core {
namespace math {

struct Fixed {
    int32_t raw;

    constexpr Fixed() noexcept : raw(0) {}
    constexpr explicit Fixed(int32_t r) noexcept : raw(r) {}

    constexpr Fixed(int32_t whole, int32_t frac) noexcept
        : raw((whole << 16) | (frac & 0xFFFF)) {}

    constexpr explicit operator int32_t() const noexcept { return raw >> 16; }
};

inline constexpr int FIXED_SHIFT = 16;
inline constexpr Fixed FIXED_ONE{1, 0};

constexpr Fixed make_fixed(int32_t whole, int32_t frac = 0) noexcept {
    return Fixed{whole, frac};
}

constexpr Fixed from_int(int32_t v) noexcept {
    return Fixed{static_cast<int32_t>(static_cast<uint32_t>(v) << 16)};
}
constexpr int32_t to_int(Fixed f) noexcept { return static_cast<int32_t>(f); }
constexpr float to_float(Fixed f) noexcept {
    return static_cast<float>(f.raw) / 65536.0f;
}
constexpr Fixed from_float(float v) noexcept {
    return Fixed{static_cast<int32_t>(v * 65536.0f)};
}

constexpr Fixed operator+(Fixed a, Fixed b) noexcept { return Fixed{a.raw + b.raw}; }
constexpr Fixed operator-(Fixed a, Fixed b) noexcept { return Fixed{a.raw - b.raw}; }
constexpr Fixed operator*(Fixed a, Fixed b) noexcept {
    int64_t r = static_cast<int64_t>(a.raw) * static_cast<int64_t>(b.raw);
    r >>= 16;
    if (r > 0x7FFFFFFF)  return Fixed{0x7FFFFFFF};
    if (r < -0x80000000LL) return Fixed{-0x7FFFFFFF - 1};
    return Fixed{static_cast<int32_t>(r)};
}
constexpr Fixed operator/(Fixed a, Fixed b) noexcept {
    if (b.raw == 0) return Fixed{0};
    int64_t r = (static_cast<int64_t>(a.raw) << 16) / b.raw;
    if (r > 0x7FFFFFFF)  return Fixed{0x7FFFFFFF};
    if (r < -0x80000000LL) return Fixed{-0x7FFFFFFF - 1};
    return Fixed{static_cast<int32_t>(r)};
}

constexpr Fixed& operator+=(Fixed& a, Fixed b) noexcept { a.raw += b.raw; return a; }
constexpr Fixed& operator-=(Fixed& a, Fixed b) noexcept { a.raw -= b.raw; return a; }

constexpr Fixed operator-(Fixed a) noexcept { return Fixed{-a.raw}; }

constexpr bool operator==(Fixed a, Fixed b) noexcept { return a.raw == b.raw; }
constexpr bool operator!=(Fixed a, Fixed b) noexcept { return a.raw != b.raw; }
constexpr bool operator<(Fixed a, Fixed b) noexcept  { return a.raw <  b.raw; }
constexpr bool operator<=(Fixed a, Fixed b) noexcept { return a.raw <= b.raw; }
constexpr bool operator>(Fixed a, Fixed b) noexcept  { return a.raw >  b.raw; }
constexpr bool operator>=(Fixed a, Fixed b) noexcept { return a.raw >= b.raw; }

template <typename T>
struct Vec2 {
    T x{};
    T y{};

    constexpr Vec2() = default;
    constexpr Vec2(T x_, T y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(Vec2 const& o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 const& o) const noexcept { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(T s) const noexcept { return {x * s, y * s}; }
    constexpr Vec2 operator/(T s) const noexcept { return {x / s, y / s}; }
    constexpr Vec2& operator+=(Vec2 const& o) noexcept { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(Vec2 const& o) noexcept { x -= o.x; y -= o.y; return *this; }
    constexpr bool operator==(Vec2 const& o) const noexcept { return x == o.x && y == o.y; }
    constexpr bool operator!=(Vec2 const& o) const noexcept { return !(*this == o); }
};

template <typename T>
constexpr T dot(Vec2<T> a, Vec2<T> b) noexcept {
    return a.x * b.x + a.y * b.y;
}

template <typename T>
T length(Vec2<T> a) noexcept {
    T sq = dot(a, a);
    return static_cast<T>(std::sqrt(static_cast<double>(sq)));
}

template <typename T>
Vec2<T> normalize(Vec2<T> a) noexcept {
    T len = length(a);
    if (len == T{}) return a;
    return { a.x / len, a.y / len };
}

struct IVec2 {
    int x{};
    int y{};

    constexpr IVec2() = default;
    constexpr IVec2(int x_, int y_) : x(x_), y(y_) {}

    constexpr IVec2 operator+(IVec2 const& o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr IVec2 operator-(IVec2 const& o) const noexcept { return {x - o.x, y - o.y}; }
    constexpr IVec2 operator*(int s) const noexcept { return {x * s, y * s}; }
    constexpr IVec2 operator/(int s) const noexcept { return {x / s, y / s}; }
    constexpr bool operator==(IVec2 const& o) const noexcept { return x == o.x && y == o.y; }
    constexpr bool operator!=(IVec2 const& o) const noexcept { return !(*this == o); }
};

struct IRect {
    int x{};
    int y{};
    int w{};
    int h{};

    constexpr IRect() = default;
    constexpr IRect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}

    constexpr bool contains(IVec2 p) const noexcept {
        return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h;
    }

    constexpr bool intersects(IRect const& o) const noexcept {
        return !(x + w <= o.x || o.x + o.w <= x || y + h <= o.y || o.y + o.h <= y);
    }

    constexpr IRect intersection(IRect const& o) const noexcept {
        int nx = x > o.x ? x : o.x;
        int ny = y > o.y ? y : o.y;
        int rx = (x + w) < (o.x + o.w) ? (x + w) : (o.x + o.w);
        int rb = (y + h) < (o.y + o.h) ? (y + h) : (o.y + o.h);
        if (rx <= nx || rb <= ny) return {};
        return { nx, ny, rx - nx, rb - ny };
    }

    constexpr IRect union_rect(IRect const& o) const noexcept {
        int nx = x < o.x ? x : o.x;
        int ny = y < o.y ? y : o.y;
        int rx = (x + w) > (o.x + o.w) ? (x + w) : (o.x + o.w);
        int rb = (y + h) > (o.y + o.h) ? (y + h) : (o.y + o.h);
        return { nx, ny, rx - nx, rb - ny };
    }
};

}  // namespace math
}  // namespace core
}  // namespace ash