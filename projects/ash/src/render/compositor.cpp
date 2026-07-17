#include "render/compositor.hpp"

#include <algorithm>

namespace ash {
namespace render {

namespace {

inline bool cell_blank_bg(Cell const& c) noexcept {
    return c.bg_r == 0 && c.bg_g == 0 && c.bg_b == 0;
}

inline Cell alpha_over(Cell const& top, Cell const& bot) noexcept {
    // Simplification: if the lower layer has any non-zero bg, blend bg
    // channels by average; glyph and fg always come from top.
    Cell out = top;
    if (!cell_blank_bg(bot)) {
        out.bg_r = static_cast<std::uint8_t>((top.bg_r + bot.bg_r) / 2);
        out.bg_g = static_cast<std::uint8_t>((top.bg_g + bot.bg_g) / 2);
        out.bg_b = static_cast<std::uint8_t>((top.bg_b + bot.bg_b) / 2);
    }
    return out;
}

}  // namespace

Buffer composite(CompositeInput const& in) {
    // Determine output size from the first non-empty layer.
    std::uint16_t out_w = 0;
    std::uint16_t out_h = 0;
    for (auto const& l : in.layers) {
        if (l.width != 0 && l.height != 0) {
            out_w = l.width;
            out_h = l.height;
            break;
        }
    }
    Buffer out(out_w, out_h);

    for (std::size_t li = 0; li < in.layers.size(); ++li) {
        Buffer const& layer = in.layers[li];
        if (layer.width != out_w || layer.height != out_h) continue;

        for (std::uint16_t y = 0; y < out_h; ++y) {
            for (std::uint16_t x = 0; x < out_w; ++x) {
                Cell const& src = layer.cells[static_cast<std::size_t>(y) * out_w + x];
                if (src.flags & cell_flag::INVISIBLE) continue;
                if (src.glyph == 0x20 && cell_blank_bg(src)) continue;  // empty slot

                Cell& dst = out.cells[static_cast<std::size_t>(y) * out_w + x];
                if (src.flags & cell_flag::OPAQUE) {
                    dst = src;
                } else if (src.flags & cell_flag::TRANSPARENT || !cell_blank_bg(src)) {
                    dst = alpha_over(src, dst);
                } else {
                    // Plain glyph with blank bg — only paint if dst is blank.
                    if (dst.glyph == 0x20 && cell_blank_bg(dst)) {
                        dst = src;
                    }
                }
                (void)li;
            }
        }
    }

    // Apply lighting last.
    if (in.light != nullptr) {
        for (std::uint16_t y = 0; y < out_h; ++y) {
            for (std::uint16_t x = 0; x < out_w; ++x) {
                in.light->apply_to(x, y, out.cells[static_cast<std::size_t>(y) * out_w + x], in.ambient);
            }
        }
    }

    return out;
}

}  // namespace render
}  // namespace ash