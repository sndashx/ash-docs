#pragma once
/// Phase 01 step 0110: 256-entry color palette with colorblind remappings
/// and accessibility variants.
#include <array>
#include <cstdint>
#include <string>

namespace ash {
namespace render {

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    constexpr Color() noexcept : r(0), g(0), b(0) {}
    constexpr Color(std::uint8_t rv, std::uint8_t gv, std::uint8_t bv) noexcept
        : r(rv), g(gv), b(bv) {}

    constexpr bool operator==(Color const& o) const noexcept {
        return r == o.r && g == o.g && b == o.b;
    }
    constexpr bool operator!=(Color const& o) const noexcept { return !(*this == o); }
};

struct Palette {
    std::array<Color, 256> colors{};

    Color const& operator[](std::size_t i) const noexcept { return colors[i]; }
    Color&       operator[](std::size_t i)       noexcept { return colors[i]; }
};

/// xterm-compatible 256-color palette.
Palette default_256();

/// Colorblind remapping. Recognized modes:
///   "protan", "deutan", "tritan", "high_contrast", "no_color".
/// Anything else returns the default palette.
Palette palette_for_mode(std::string const& mode);

/// High-contrast palette (wider hue separation).
Palette high_contrast_palette();

/// Luminance-only palette (R=G=B=Y for each entry).
Palette no_color_palette();

}  // namespace render
}  // namespace ash