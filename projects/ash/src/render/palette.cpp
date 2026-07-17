#include "render/palette.hpp"

#include <algorithm>
#include <cctype>

namespace ash {
namespace render {

namespace {

inline std::uint8_t clamp8(int v) noexcept {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<std::uint8_t>(v);
}

inline Color to_luma(Color c) noexcept {
    // Rec. 709 luma weights.
    std::uint8_t y = static_cast<std::uint8_t>(
        0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b);
    return {y, y, y};
}

inline Color protan_remap(Color c) noexcept {
    // Approximate Brettel-Viénot-Mollon projection for protanopia.
    double r = 0.567 * c.r + 0.433 * c.g + 0.000 * c.b;
    double g = 0.558 * c.r + 0.442 * c.g + 0.000 * c.b;
    double b = 0.000 * c.r + 0.242 * c.g + 0.758 * c.b;
    return {clamp8(static_cast<int>(r)),
            clamp8(static_cast<int>(g)),
            clamp8(static_cast<int>(b))};
}

inline Color deutan_remap(Color c) noexcept {
    double r = 0.625 * c.r + 0.375 * c.g + 0.000 * c.b;
    double g = 0.700 * c.r + 0.300 * c.g + 0.000 * c.b;
    double b = 0.000 * c.r + 0.300 * c.g + 0.700 * c.b;
    return {clamp8(static_cast<int>(r)),
            clamp8(static_cast<int>(g)),
            clamp8(static_cast<int>(b))};
}

inline Color tritan_remap(Color c) noexcept {
    double r = 0.950 * c.r + 0.050 * c.g + 0.000 * c.b;
    double g = 0.000 * c.r + 0.433 * c.g + 0.567 * c.b;
    double b = 0.000 * c.r + 0.475 * c.g + 0.525 * c.b;
    return {clamp8(static_cast<int>(r)),
            clamp8(static_cast<int>(g)),
            clamp8(static_cast<int>(b))};
}

inline Color high_contrast_remap(Color c) noexcept {
    // Push saturation toward extremes.
    int r = c.r - 128;
    int g = c.g - 128;
    int b = c.b - 128;
    r = r * 2 + 128;
    g = g * 2 + 128;
    b = b * 2 + 128;
    return {clamp8(r), clamp8(g), clamp8(b)};
}

inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

}  // namespace

Palette default_256() {
    Palette p{};
    // 0..15: xterm system colors (approx).
    constexpr Color system16[16] = {
        {0,0,0},       {128,0,0},     {0,128,0},     {128,128,0},
        {0,0,128},     {128,0,128},   {0,128,128},   {192,192,192},
        {128,128,128}, {255,0,0},     {0,255,0},     {255,255,0},
        {0,0,255},     {255,0,255},   {0,255,255},   {255,255,255},
    };
    for (int i = 0; i < 16; ++i) p.colors[static_cast<std::size_t>(i)] = system16[i];
    // 16..231: 6x6x6 color cube.
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 6; ++g) {
            for (int b = 0; b < 6; ++b) {
                int idx = 16 + 36 * r + 6 * g + b;
                p.colors[static_cast<std::size_t>(idx)] = {
                    static_cast<std::uint8_t>(r ? 55 + r * 40 : 0),
                    static_cast<std::uint8_t>(g ? 55 + g * 40 : 0),
                    static_cast<std::uint8_t>(b ? 55 + b * 40 : 0),
                };
            }
        }
    }
    // 232..255: greyscale ramp.
    for (int i = 0; i < 24; ++i) {
        std::uint8_t v = static_cast<std::uint8_t>(8 + i * 10);
        p.colors[static_cast<std::size_t>(232 + i)] = {v, v, v};
    }
    return p;
}

Palette high_contrast_palette() {
    Palette p = default_256();
    for (auto& c : p.colors) c = high_contrast_remap(c);
    return p;
}

Palette no_color_palette() {
    Palette p = default_256();
    for (auto& c : p.colors) c = to_luma(c);
    return p;
}

Palette palette_for_mode(std::string const& mode) {
    std::string m = lower(mode);
    if (m == "protan") {
        Palette p = default_256();
        for (auto& c : p.colors) c = protan_remap(c);
        return p;
    }
    if (m == "deutan") {
        Palette p = default_256();
        for (auto& c : p.colors) c = deutan_remap(c);
        return p;
    }
    if (m == "tritan") {
        Palette p = default_256();
        for (auto& c : p.colors) c = tritan_remap(c);
        return p;
    }
    if (m == "high_contrast" || m == "high-contrast" || m == "contrast") {
        return high_contrast_palette();
    }
    if (m == "no_color" || m == "monochrome" || m == "luminance") {
        return no_color_palette();
    }
    return default_256();
}

}  // namespace render
}  // namespace ash