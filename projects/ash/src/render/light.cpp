#include "render/light.hpp"

#include <algorithm>
#include <cmath>

namespace ash {
namespace render {

void LightGrid::resize(std::uint16_t w, std::uint16_t h) {
    width = w;
    height = h;
    intensity.assign(static_cast<std::size_t>(w) * h, std::uint8_t{0});
}

void LightGrid::clear() noexcept {
    std::fill(intensity.begin(), intensity.end(), std::uint8_t{0});
}

void LightGrid::set(int x, int y, std::uint8_t v) {
    if (x < 0 || y < 0
        || x >= static_cast<int>(width)
        || y >= static_cast<int>(height)) return;
    intensity[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)] = v;
}

std::uint8_t LightGrid::get(int x, int y) const noexcept {
    if (x < 0 || y < 0
        || x >= static_cast<int>(width)
        || y >= static_cast<int>(height)) return 0;
    return intensity[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
}

void LightGrid::add_light(int x, int y, int radius, std::uint8_t base_intensity) {
    if (radius < 0 || base_intensity == 0) return;
    int const x0 = std::max(0, x - radius);
    int const y0 = std::max(0, y - radius);
    int const x1 = std::min(static_cast<int>(width)  - 1, x + radius);
    int const y1 = std::min(static_cast<int>(height) - 1, y + radius);
    for (int yy = y0; yy <= y1; ++yy) {
        for (int xx = x0; xx <= x1; ++xx) {
            int const dx = xx - x;
            int const dy = yy - y;
            // Bresenham-like distance: chebyshev (max of |dx|,|dy|) gives a
            // square falloff which matches "intensity(d) = max(0, base - d)".
            int const d = std::max(std::abs(dx), std::abs(dy));
            if (d > radius) continue;
            int const lit = static_cast<int>(base_intensity) - d;
            if (lit <= 0) continue;
            auto v = static_cast<std::uint8_t>(std::min(255, lit));
            auto& cell = intensity[static_cast<std::size_t>(yy) * width + static_cast<std::size_t>(xx)];
            if (v > cell) cell = v;
        }
    }
}

void LightGrid::add_flicker(int x, int y, int radius, std::uint8_t base_intensity,
                            core::rng::Xoshiro256pp& rng) {
    int const span = std::max(1, static_cast<int>(base_intensity) / 5);
    int const lo = std::max(0, static_cast<int>(base_intensity) - span);
    int const hi = std::min(255, static_cast<int>(base_intensity) + span);
    int const actual = rng.next_int(lo, hi);
    add_light(x, y, radius, static_cast<std::uint8_t>(actual));
}

void LightGrid::propagate() {
    if (width == 0 || height == 0 || intensity.empty()) return;
    std::vector<std::uint8_t> scratch(intensity.size(), std::uint8_t{0});
    for (int pass = 0; pass < 4; ++pass) {
        for (std::uint16_t y = 0; y < height; ++y) {
            for (std::uint16_t x = 0; x < width; ++x) {
                std::uint8_t m = intensity[static_cast<std::size_t>(y) * width + x];
                if (x > 0)              m = std::max<std::uint8_t>(m, intensity[static_cast<std::size_t>(y) * width + (x - 1)]);
                if (x + 1 < width)      m = std::max<std::uint8_t>(m, intensity[static_cast<std::size_t>(y) * width + (x + 1)]);
                if (y > 0)              m = std::max<std::uint8_t>(m, intensity[static_cast<std::size_t>((y - 1)) * width + x]);
                if (y + 1 < height)     m = std::max<std::uint8_t>(m, intensity[static_cast<std::size_t>((y + 1)) * width + x]);
                scratch[static_cast<std::size_t>(y) * width + x] = m;
            }
        }
        intensity.swap(scratch);
    }
}

namespace {

inline std::uint8_t scale_u8(std::uint8_t v, float k) noexcept {
    float f = static_cast<float>(v) * k;
    if (f < 0.0f)   f = 0.0f;
    if (f > 255.0f) f = 255.0f;
    return static_cast<std::uint8_t>(f);
}

}  // namespace

void LightGrid::apply_to(int x, int y, Cell& c, std::uint8_t ambient_override) const noexcept {
    std::uint8_t const i = get(x, y);
    float const k = (static_cast<float>(i) + static_cast<float>(ambient_override)) / 30.0f;
    if (k >= 1.0f) return;  // no darkening
    c.fg_r = scale_u8(c.fg_r, k);
    c.fg_g = scale_u8(c.fg_g, k);
    c.fg_b = scale_u8(c.fg_b, k);
    c.bg_r = scale_u8(c.bg_r, k);
    c.bg_g = scale_u8(c.bg_g, k);
    c.bg_b = scale_u8(c.bg_b, k);
}

}  // namespace render
}  // namespace ash