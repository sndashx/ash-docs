#pragma once
/// Phase 01 step 0103: Fixed-point arithmetic, vec2, rect.
/// `Fixed` is a 16.16 signed integer (int32_t) — chosen for deterministic
/// behavior, fast integer math, and easy serialization. Sufficient range
/// for world coordinates up to ~32767 cells with sub-cell precision.
#include <cstdint>
#include <type_traits>

namespace ash {
namespace core {

using Fixed = std::int32_t;

inline constexpr Fixed FIXED_ONE = 1 << 16;
inline constexpr Fixed FIXED_HALF = 1 << 15;

inline constexpr Fixed fixed_from_int(int v) noexcept { return v << 16; }
inline constexpr int   fixed_to_int(Fixed v) noexcept { return v >> 16; }
inline constexpr float fixed_to_float(Fixed v) noexcept { return static_cast<float>(v) / 65536.0f; }
inline constexpr Fixed fixed_from_float(float v) noexcept { return static_cast<Fixed>(v * 65536.0f); }

inline constexpr Fixed fixed_mul(Fixed a, Fixed b) noexcept {
    return static_cast<Fixed>((static_cast<std::int64_t>(a) * b) >> 16);
}
inline constexpr Fixed fixed_div(Fixed a, Fixed b) noexcept {
    return static_cast<Fixed>((static_cast<std::int64_t>(a) << 16) / b);
}

template <typename T>
struct Vec2 {
    T x{};
    T y{};

    constexpr Vec2() = default;
    constexpr Vec2(T xv, T yv) noexcept : x(xv), y(yv) {}

    constexpr Vec2 operator+(Vec2 const& o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 const& o) const noexcept { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(T s) const noexcept { return {x * s, y * s}; }
    constexpr Vec2 operator/(T s) const noexcept { return {x / s, y / s}; }
    constexpr Vec2& operator+=(Vec2 const& o) noexcept { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(Vec2 const& o) noexcept { x -= o.x; y -= o.y; return *this; }
    constexpr bool operator==(Vec2 const& o) const noexcept { return x == o.x && y == o.y; }
    constexpr bool operator!=(Vec2 const& o) const noexcept { return !(*this == o); }
};

struct IVec2 {
    int x{};
    int y{};

    constexpr IVec2() = default;
    constexpr IVec2(int xv, int yv) noexcept : x(xv), y(yv) {}

    constexpr IVec2 operator+(IVec2 const& o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr IVec2 operator-(IVec2 const& o) const noexcept { return {x - o.x, y - o.y}; }
    constexpr IVec2 operator*(int s) const noexcept { return {x * s, y * s}; }
    constexpr bool operator==(IVec2 const& o) const noexcept { return x == o.x && y == o.y; }
    constexpr bool operator!=(IVec2 const& o) const noexcept { return !(*this == o); }
};

struct Rect {
    int x{};
    int y{};
    int w{};
    int h{};

    constexpr Rect() = default;
    constexpr Rect(int xv, int yv, int wv, int hv) noexcept : x(xv), y(yv), w(wv), h(hv) {}

    constexpr int x2() const noexcept { return x + w; }
    constexpr int y2() const noexcept { return y + h; }
    constexpr bool contains(int px, int py) const noexcept {
        return px >= x && px < x2() && py >= y && py < y2();
    }
    constexpr bool intersects(Rect const& o) const noexcept {
        return !(x >= o.x2() || o.x >= x2() || y >= o.y2() || o.y >= y2());
    }
};

/// Linear interpolation between two Fixed values.
inline constexpr Fixed fixed_lerp(Fixed a, Fixed b, Fixed t) noexcept {
    return a + fixed_mul(b - a, t);
}

}  // namespace core
}  // namespace ash