#pragma once
/// Phase 01 step 0104: 2D cell grid (front or back buffer).
///
/// Layout: row-major `std::vector<Cell>` of size `width * height`. Cell at
/// (x, y) is at index `y * width + x`. Storage is owned (value semantics).
///
/// OOB policy:
///   - `set` asserts in debug, is a release no-op (caller bug surface must
///     not corrupt state in shipped builds).
///   - `get` returns `CELL_BLANK` so callers can iterate freely.
#include <cassert>
#include <cstdint>
#include <vector>

#include "render/cell.hpp"
#include "render/terminal.hpp"

namespace ash {
namespace render {

struct Buffer {
    std::vector<Cell> cells;
    std::uint16_t     width{0};
    std::uint16_t     height{0};

    Buffer() = default;
    Buffer(std::uint16_t w, std::uint16_t h);
    explicit Buffer(Terminal const& t);

    void   clear(Cell c = CELL_BLANK);
    void   set(int x, int y, Cell c);
    Cell   get(int x, int y) const;
    void   fill_rect(int x, int y, int w, int h, Cell c);
    void   resize(std::uint16_t w, std::uint16_t h);

    /// Total cell count.
    std::size_t size() const noexcept { return cells.size(); }

    /// Linear index helper (debug-bounds-checked).
    std::size_t index_of(int x, int y) const noexcept {
        assert(x >= 0 && y >= 0 && x < width && y < height);
        return static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x);
    }
};

}  // namespace render
}  // namespace ash