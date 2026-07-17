#pragma once
/// Phase 01 step 0104: Front/back buffer of Cells.
#include <cassert>
#include <cstdint>
#include <vector>

#include "render/cell.hpp"
#include "render/terminal.hpp"

namespace ash {
namespace render {

struct Buffer {
    std::vector<Cell> cells;
    uint16_t width  = 0;
    uint16_t height = 0;

    Buffer() = default;
    Buffer(uint16_t w, uint16_t h);
    explicit Buffer(Terminal const& t);

    void clear(Cell c);
    void set(int x, int y, Cell c);
    Cell get(int x, int y) const noexcept;
    void fill_rect(int x, int y, int w, int h, Cell c);
    void resize(uint16_t w, uint16_t h);
};

}  // namespace render
}  // namespace ash