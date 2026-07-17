#include "render/buffer_diff.hpp"

namespace ash {
namespace render {

namespace {

inline bool cell_eq(Cell const& a, Cell const& b) noexcept {
    return a.glyph == b.glyph
        && a.fg_r  == b.fg_r
        && a.fg_g  == b.fg_g
        && a.fg_b  == b.fg_b
        && a.bg_r  == b.bg_r
        && a.bg_g  == b.bg_g
        && a.bg_b  == b.bg_b
        && a.flags == b.flags;
}

}  // namespace

std::vector<DirtyRect> diff(Buffer const& old_buf, Buffer const& new_buf) {
    std::vector<DirtyRect> out;
    std::uint16_t const w = new_buf.width;
    std::uint16_t const h = new_buf.height;
    if (w == 0 || h == 0) return out;
    if (old_buf.width != w || old_buf.height != h) {
        // Size mismatch: every cell is dirty.
        out.push_back({0, 0, w, h});
        return out;
    }

    out.reserve(16);
    for (std::uint16_t y = 0; y < h; ++y) {
        std::uint16_t run_start = 0;
        bool in_run = false;
        for (std::uint16_t x = 0; x < w; ++x) {
            std::size_t const i = static_cast<std::size_t>(y) * w + x;
            bool const changed = !cell_eq(old_buf.cells[i], new_buf.cells[i]);
            if (changed && !in_run) {
                run_start = x;
                in_run = true;
            } else if (!changed && in_run) {
                out.push_back({run_start, y,
                               static_cast<std::uint16_t>(x - run_start), 1});
                in_run = false;
            }
        }
        if (in_run) {
            out.push_back({run_start, y,
                           static_cast<std::uint16_t>(w - run_start), 1});
        }
    }
    return out;
}

}  // namespace render
}  // namespace ash