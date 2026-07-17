#pragma once
/// Phase 01 step 0112: Cell pixel-size query (stub).
#include <cstdint>

namespace ash {
namespace render {

struct FontMetrics {
    uint16_t cell_w = 8;
    uint16_t cell_h = 16;
};

inline FontMetrics font_metrics_query() noexcept {
    return FontMetrics{8, 16};
}

}  // namespace render
}  // namespace ash