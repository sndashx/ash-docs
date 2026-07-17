#include "render/buffer.hpp"

#include <algorithm>
#include <cassert>

namespace ash {
namespace render {

Buffer::Buffer(uint16_t w, uint16_t h) : width(w), height(h) {
    cells.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), CELL_BLANK);
}

Buffer::Buffer(Terminal const& t)
    : width(t.width_cells), height(t.height_cells) {
    cells.resize(static_cast<std::size_t>(width) *
                 static_cast<std::size_t>(height), CELL_BLANK);
}

void Buffer::clear(Cell c) {
    std::fill(cells.begin(), cells.end(), c);
}

void Buffer::set(int x, int y, Cell c) {
    assert(x >= 0 && y >= 0 && x < width && y < height);
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    cells[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)] = c;
}

Cell Buffer::get(int x, int y) const noexcept {
    if (x < 0 || y < 0 || x >= width || y >= height) return CELL_BLANK;
    return cells[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)];
}

void Buffer::fill_rect(int x, int y, int w, int h, Cell c) {
    int x0 = std::max(x, 0);
    int y0 = std::max(y, 0);
    int x1 = std::min(x + w, static_cast<int>(width));
    int y1 = std::min(y + h, static_cast<int>(height));
    for (int yy = y0; yy < y1; ++yy) {
        for (int xx = x0; xx < x1; ++xx) {
            cells[static_cast<std::size_t>(yy) * width + static_cast<std::size_t>(xx)] = c;
        }
    }
}

void Buffer::resize(uint16_t w, uint16_t h) {
    width = w;
    height = h;
    cells.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), CELL_BLANK);
}

}  // namespace render
}  // namespace ash