#include "combat/line_of_sight.hpp"

#include <cstdlib>
#include <vector>

namespace ash {
namespace combat {

namespace {

/// Bresenham-style 2D line, axis-aligned. Generalized for any slope.
void bresenham(int x0, int y0, int x1, int y1,
               std::vector<core::IVec2>& out) noexcept {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int x = x0;
    int y = y0;
    while (true) {
        out.emplace_back(x, y);
        if (x == x1 && y == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

}  // namespace

std::vector<core::IVec2> bresenham_line(core::IVec2 from, core::IVec2 to) noexcept {
    std::vector<core::IVec2> out;
    bresenham(from.x, from.y, to.x, to.y, out);
    return out;
}

LosResult trace_line_of_sight(world::Map const& map,
                              core::IVec2       from,
                              core::IVec2       to) noexcept {
    LosResult r{};
    r.path = bresenham_line(from, to);
    r.blocker = to;
    if (r.path.empty()) {
        return r;
    }
    /// The starting cell does not block (we're shooting from it).
    for (std::size_t i = 1; i < r.path.size(); ++i) {
        auto cell = r.path[i];
        if (map.is_opaque(cell.x, cell.y)) {
            r.visible = false;
            r.blocker = cell;
            return r;
        }
    }
    r.visible = true;
    r.blocker = to;
    return r;
}

}  // namespace combat
}  // namespace ash
