#pragma once
/// Phase 01 step 0105: Buffer diff → list of dirty rectangles.
#include <cstdint>
#include <vector>

#include "render/buffer.hpp"

namespace ash {
namespace render {

struct DirtyRect {
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t w = 0;
    uint16_t h = 0;
};

std::vector<DirtyRect> buffer_diff(Buffer const& old_buf, Buffer const& new_buf);

}  // namespace render
}  // namespace ash