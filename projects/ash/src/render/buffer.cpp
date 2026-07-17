#include "render/buffer.hpp"

#include <algorithm>

namespace ash {
namespace render {

Buffer::Buffer(std::uint16_t w, std::uint16_t h)
    : cells(static_cast<std::size_t>(w) * h, CELL_BLANK), width(w), height(h) {}

Buffer::Buffer(Terminal const& t)
    : Buffer(t.width_cells, t.height_cells) {}

void Buffer::clear(Cell c) {
    std::fill(cells.begin(), cells.end(), c);
}

void Buffer::set(int x, int y, Cell c) {
    assert(x >= 0 && y >= 0
           && x < static_cast<int>(width)
           && y < static_cast<int>(height)
           && "Buffer::set out of bounds");
    if (x < 0 || y < 0
        || x >= static_cast<int>(width)
        || y >= static_cast<int>(height)) {
        return;  // release no-op
    }
    cells[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)] = c;
}

Cell Buffer::get(int x, int y) const {
    if (x < 0 || y < 0
        || x >= static_cast<int>(width)
        || y >= static_cast<int>(height)) {
        return CELL_BLANK;
    }
    return cells[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
}

void Buffer::fill_rect(int x, int y, int w, int h, Cell c) {
    if (w <= 0 || h <= 0) return;
    int const x0 = std::max(x, 0);
    int const y0 = std::max(y, 0);
    int const x1 = std::min(x + w, static_cast<int>(width));
    int const y1 = std::min(y + h, static_cast<int>(height));
    for (int yy = y0; yy < y1; ++yy) {
        auto* row = cells.data() + static_cast<std::size_t>(yy) * width;
        for (int xx = x0; xx < x1; ++xx) {
            row[xx] = c;
        }
    }
}

void Buffer::resize(std::uint16_t w, std::uint16_t h) {
    width = w;
    height = h;
    cells.assign(static_cast<std::size_t>(w) * h, CELL_BLANK);
}

}  // namespace render
}  // namespace ash