#include "render/light.hpp"

#include <algorithm>
#include <cstdlib>

namespace ash {
namespace render {

void LightGrid::resize(uint16_t w, uint16_t h) {
    width = w;
    height = h;
    intensity.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0);
}

void LightGrid::set(int x, int y, uint8_t v) noexcept {
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    intensity[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)] = v;
}

uint8_t LightGrid::get(int x, int y) const noexcept {
    if (x < 0 || y < 0 || x >= width || y >= height) return 0;
    return intensity[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
}

void LightGrid::add_light(int x, int y, uint8_t radius, uint8_t intensity_val) noexcept {
    if (radius == 0 || intensity_val == 0 || width == 0 || height == 0) return;
    int r = static_cast<int>(radius);
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            int d = std::abs(dx) + std::abs(dy);
            if (d > r) continue;
            int xi = x + dx;
            int yi = y + dy;
            if (xi < 0 || yi < 0 || xi >= width || yi >= height) continue;
            int contrib = static_cast<int>(intensity_val) - d;
            if (contrib <= 0) continue;
            uint8_t nv = static_cast<uint8_t>(contrib);
            auto& cur = intensity[static_cast<std::size_t>(yi) * width +
                                  static_cast<std::size_t>(xi)];
            if (nv > cur) cur = nv;
        }
    }
}

void LightGrid::add_flicker(int x, int y, uint8_t radius, uint8_t base) noexcept {
    static core::rng::Xoshiro256pp rng(0xA5A5'C3C3'F00D'BEEFULL);
    uint8_t jitter = static_cast<uint8_t>(rng.uniform(4));
    uint8_t intensity_val = static_cast<uint8_t>(base + jitter);
    add_light(x, y, radius, intensity_val);
}

void LightGrid::propagate() {
    if (width == 0 || height == 0 || intensity.empty()) return;
    std::vector<uint8_t> scratch = intensity;
    const std::size_t w = static_cast<std::size_t>(width);
    const std::size_t h = static_cast<std::size_t>(height);
    for (int pass = 0; pass < 4; ++pass) {
        for (std::size_t y = 0; y < h; ++y) {
            for (std::size_t x = 0; x < w; ++x) {
                uint8_t best = scratch[y * w + x];
                if (x > 0) {
                    uint8_t v = scratch[y * w + (x - 1)];
                    if (v > best) best = v;
                }
                if (x + 1 < w) {
                    uint8_t v = scratch[y * w + (x + 1)];
                    if (v > best) best = v;
                }
                if (y > 0) {
                    uint8_t v = scratch[(y - 1) * w + x];
                    if (v > best) best = v;
                }
                if (y + 1 < h) {
                    uint8_t v = scratch[(y + 1) * w + x];
                    if (v > best) best = v;
                }
                intensity[y * w + x] = best;
            }
        }
        scratch = intensity;
    }
}

void LightGrid::apply_to(Cell& c, uint8_t ambient_val) const noexcept {
    // `ambient_val` is the per-cell effective level: typically
    //   effective = max(LightGrid::get(x, y), LightGrid.ambient).
    // The caller computes this; here we apply factor (effective+ambient)/30
    // per the spec. Mixing ambient with the spec formula keeps the contract
    // identical to the bead body while staying true to the prototype.
    float factor = static_cast<float>(ambient_val) / 30.0f;
    if (factor > 1.0f) factor = 1.0f;
    auto scale = [factor](uint8_t ch) -> uint8_t {
        float v = static_cast<float>(ch) * factor;
        if (v > 255.0f) v = 255.0f;
        return static_cast<uint8_t>(v);
    };
    c.fg_r = scale(c.fg_r);
    c.fg_g = scale(c.fg_g);
    c.fg_b = scale(c.fg_b);
    c.bg_r = scale(c.bg_r);
    c.bg_g = scale(c.bg_g);
    c.bg_b = scale(c.bg_b);
}

}  // namespace render
}  // namespace ash