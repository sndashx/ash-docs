#pragma once
/// Phase 01 step 0105: Dirty-rectangle diff between two buffers.
/// Naive O(n) scan that groups consecutive differing cells in a row into
/// horizontal DirtyRect runs. Sufficient for the terminal-redraw cadence
/// (60fps × a few hundred cells = trivial). Replace with a smarter
/// bucket algorithm only if profiling demands it.
#include <cstdint>
#include <vector>

#include "render/buffer.hpp"

namespace ash {
namespace render {

struct DirtyRect {
    std::uint16_t x;
    std::uint16_t y;
    std::uint16_t w;
    std::uint16_t h;
};

/// Returns the list of rectangular regions in `new_buf` whose cells
/// differ from `old`. Empty result means nothing to redraw.
std::vector<DirtyRect> diff(Buffer const& old_buf, Buffer const& new_buf);

}  // namespace render
}  // namespace ash