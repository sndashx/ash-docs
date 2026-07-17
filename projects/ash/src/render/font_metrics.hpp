#pragma once
/// Phase 01 step 0112: Terminal font metrics.
///
/// Today this is just the cell dimensions in pixels (8x16 default).
/// The platform layer will eventually override query() with values from
/// OSC queries or fallback probing.
#include <cstdint>

namespace ash {
namespace render {

struct FontMetrics {
    std::uint16_t cell_w{8};
    std::uint16_t cell_h{16};
};

/// Returns the current font metrics. Phase-0 default: 8x16.
FontMetrics query();

}  // namespace render
}  // namespace ash