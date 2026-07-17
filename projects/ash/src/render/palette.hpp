#pragma once
/// Phase 01 step 0110: 256-color and accessibility palettes.
#include <array>
#include <cstdint>
#include <string>

namespace ash {
namespace render {

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Palette {
    std::array<Color, 256> colors{};
};

Palette default_256();
Palette palette_for_mode(std::string mode);
Palette high_contrast_palette();
Palette no_color_palette();

}  // namespace render
}  // namespace ash