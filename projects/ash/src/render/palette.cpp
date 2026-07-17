#include "render/palette.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace ash {
namespace render {

namespace {

inline uint8_t clamp8(int v) noexcept {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<uint8_t>(v);
}

// sRGB-ish luminance (BT.601).
inline uint8_t luminance(uint8_t r, uint8_t g, uint8_t b) noexcept {
    int y = (299 * static_cast<int>(r) + 587 * static_cast<int>(g) + 114 * static_cast<int>(b)) / 1000;
    return clamp8(y);
}

inline std::string lowercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}  // namespace

Palette default_256() {
    Palette p{};
    // 0..15: xterm / ANSI 16 (RGB approximations for our purposes)
    static constexpr uint8_t ansi16[16][3] = {
        {0,0,0},        {128,0,0},      {0,128,0},      {128,128,0},
        {0,0,128},      {128,0,128},    {0,128,128},    {192,192,192},
        {128,128,128},  {255,0,0},      {0,255,0},      {255,255,0},
        {0,0,255},      {255,0,255},    {0,255,255},    {255,255,255},
    };
    for (std::size_t i = 0; i < 16; ++i) {
        p.colors[i] = { ansi16[i][0], ansi16[i][1], ansi16[i][2] };
    }
    // 16..231: 6×6×6 color cube.
    for (std::size_t i = 16; i < 232; ++i) {
        std::size_t idx = i - 16;
        std::size_t b_idx = idx % 6;
        std::size_t g_idx = (idx / 6) % 6;
        std::size_t r_idx = idx / 36;
        int r = r_idx ? 55 + static_cast<int>(r_idx) * 40 : 0;
        int g = g_idx ? 55 + static_cast<int>(g_idx) * 40 : 0;
        int bv = b_idx ? 55 + static_cast<int>(b_idx) * 40 : 0;
        p.colors[i] = { static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(bv) };
    }
    // 232..255: 24-step grayscale.
    for (std::size_t i = 232; i < 256; ++i) {
        uint8_t v = static_cast<uint8_t>(8 + (static_cast<int>(i) - 232) * 10);
        if (v > 238) v = 255;
        p.colors[i] = { v, v, v };
    }
    return p;
}

namespace {

inline Color remap_protan(Color c) {
    // Brettel/Viénot/Mollon simplified protanopia simulation.
    double r = c.r, g = c.g, b = c.b;
    double nr = 0.567 * r + 0.433 * g + 0.0     * b;
    double ng = 0.558 * r + 0.442 * g + 0.0     * b;
    double nb = 0.0   * r + 0.242 * g + 0.758  * b;
    return { clamp8(static_cast<int>(nr)),
             clamp8(static_cast<int>(ng)),
             clamp8(static_cast<int>(nb)) };
}

inline Color remap_deutan(Color c) {
    double r = c.r, g = c.g, b = c.b;
    double nr = 0.625 * r + 0.375 * g + 0.0     * b;
    double ng = 0.7   * r + 0.3   * g + 0.0     * b;
    double nb = 0.0   * r + 0.3   * g + 0.7     * b;
    return { clamp8(static_cast<int>(nr)),
             clamp8(static_cast<int>(ng)),
             clamp8(static_cast<int>(nb)) };
}

inline Color remap_tritan(Color c) {
    double r = c.r, g = c.g, b = c.b;
    double nr = 0.95  * r + 0.05  * g + 0.0     * b;
    double ng = 0.0   * r + 0.433 * g + 0.567   * b;
    double nb = 0.0   * r + 0.475 * g + 0.525   * b;
    return { clamp8(static_cast<int>(nr)),
             clamp8(static_cast<int>(ng)),
             clamp8(static_cast<int>(nb)) };
}

}  // namespace

Palette palette_for_mode(std::string mode) {
    Palette base = default_256();
    std::string m = lowercase(mode);
    if (m == "default" || m == "none" || m.empty()) return base;
    Palette out{};
    auto apply = [&](Color (*fn)(Color)) {
        for (std::size_t i = 0; i < 256; ++i) out.colors[i] = fn(base.colors[i]);
    };
    if      (m == "protan"   || m == "protanopia")   apply(remap_protan);
    else if (m == "deutan"   || m == "deuteranopia") apply(remap_deutan);
    else if (m == "tritan"   || m == "tritanopia")   apply(remap_tritan);
    else if (m == "high_contrast" || m == "high-contrast") return high_contrast_palette();
    else if (m == "no_color" || m == "no-color" || m == "luminance") return no_color_palette();
    else return base;
    return out;
}

Palette high_contrast_palette() {
    Palette base = default_256();
    Palette out{};
    // Push each color to the extreme of its hue: maximize distance to grey.
    for (std::size_t i = 0; i < 256; ++i) {
        Color c = base.colors[i];
        int r = static_cast<int>(c.r) - 128;
        int g = static_cast<int>(c.g) - 128;
        int b = static_cast<int>(c.b) - 128;
        r = (r == 0) ? 0 : (r > 0 ? 255 : 0);
        g = (g == 0) ? 0 : (g > 0 ? 255 : 0);
        b = (b == 0) ? 0 : (b > 0 ? 255 : 0);
        out.colors[i] = { static_cast<uint8_t>(r),
                          static_cast<uint8_t>(g),
                          static_cast<uint8_t>(b) };
    }
    return out;
}

Palette no_color_palette() {
    Palette out{};
    Palette base = default_256();
    for (std::size_t i = 0; i < 256; ++i) {
        uint8_t y = luminance(base.colors[i].r,
                              base.colors[i].g,
                              base.colors[i].b);
        out.colors[i] = { y, y, y };
    }
    return out;
}

}  // namespace render
}  // namespace ash