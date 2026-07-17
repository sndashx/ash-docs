#include "render/buffer_diff.hpp"

#include <algorithm>

namespace ash {
namespace render {

std::vector<DirtyRect> buffer_diff(Buffer const& old_buf, Buffer const& new_buf) {
    std::vector<DirtyRect> out;
    const uint16_t w = std::min(old_buf.width, new_buf.width);
    const uint16_t h = std::min(old_buf.height, new_buf.height);

    for (uint16_t y = 0; y < h; ++y) {
        int run_start = -1;
        for (uint16_t x = 0; x < w; ++x) {
            Cell const& a = old_buf.cells[static_cast<std::size_t>(y) * old_buf.width + x];
            Cell const& b = new_buf.cells[static_cast<std::size_t>(y) * new_buf.width + x];
            bool diff = !(a.glyph == b.glyph &&
                          a.fg_r == b.fg_r && a.fg_g == b.fg_g && a.fg_b == b.fg_b &&
                          a.bg_r == b.bg_r && a.bg_g == b.bg_g && a.bg_b == b.bg_b &&
                          a.flags == b.flags);
            if (diff && run_start < 0) {
                run_start = x;
            } else if (!diff && run_start >= 0) {
                out.push_back({static_cast<uint16_t>(run_start), y,
                               static_cast<uint16_t>(x - run_start), 1});
                run_start = -1;
            }
        }
        if (run_start >= 0) {
            out.push_back({static_cast<uint16_t>(run_start), y,
                           static_cast<uint16_t>(w - run_start), 1});
        }
    }

    // If the buffers differ in size, the extra cells in `new_buf` are also dirty.
    if (new_buf.height > old_buf.height) {
        DirtyRect r{0, h, new_buf.width,
                    static_cast<uint16_t>(new_buf.height - old_buf.height)};
        out.push_back(r);
    }
    if (new_buf.width > old_buf.width) {
        uint16_t yy_start = 0;
        uint16_t yy_end   = std::min(new_buf.height, old_buf.height);
        DirtyRect r{w, yy_start,
                    static_cast<uint16_t>(new_buf.width - old_buf.width),
                    static_cast<uint16_t>(yy_end - yy_start)};
        out.push_back(r);
    }
    return out;
}

}  // namespace render
}  // namespace ash