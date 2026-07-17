#include "render/compositor.hpp"

#include <algorithm>

namespace ash {
namespace render {

namespace {

// "Transparent" sentinel: a Cell with glyph 0 in layer slots that should be
// treated as see-through. We treat any cell whose glyph is exactly 0 as
// transparent, and as opaque otherwise. RGB-only cells (glyph != 0, but
// same as CELL_BLANK glyph of 0x20) are still opaque.
constexpr bool is_transparent(Cell const& c) noexcept { return c.glyph == 0; }

inline void blend_alpha_over(Cell& dst, Cell const& src) noexcept {
    // Treat the upper layer's bg as a translucent wash (alpha 50%).
    auto mix = [](uint8_t a, uint8_t b) -> uint8_t {
        int r = (static_cast<int>(a) + static_cast<int>(b)) / 2;
        return static_cast<uint8_t>(r);
    };
    dst.bg_r = mix(dst.bg_r, src.bg_r);
    dst.bg_g = mix(dst.bg_g, src.bg_g);
    dst.bg_b = mix(dst.bg_b, src.bg_b);
}

}  // namespace

Buffer composite(CompositeInput const& input) {
    // Pick layer 0's dimensions as the target framebuffer.
    Buffer const& base = input.layers[0];
    Buffer out(base.width, base.height);

    for (uint16_t y = 0; y < out.height; ++y) {
        for (uint16_t x = 0; x < out.width; ++x) {
            Cell acc = CELL_BLANK;
            for (std::size_t li = 0; li < input.layers.size(); ++li) {
                Buffer const& layer = input.layers[li];
                if (layer.width == 0 || layer.height == 0) continue;
                if (x >= layer.width || y >= layer.height) continue;
                Cell const& c = layer.cells[
                    static_cast<std::size_t>(y) * layer.width + x];
                if (c.glyph == 0) continue;  // invisible
                if (is_transparent(c)) {
                    blend_alpha_over(acc, c);
                } else {
                    acc = c;
                }
            }

            if (input.light) {
                uint8_t intensity = input.light->get(x, y);
                uint8_t effective = static_cast<uint8_t>(std::max<int>(intensity, input.ambient));
                input.light->apply_to(acc, effective);
            }
            out.set(x, y, acc);
        }
    }
    return out;
}

}  // namespace render
}  // namespace ash