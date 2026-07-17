#pragma once
/// Phase 01 step 0102: Terminal capability/size descriptor.
/// Pure-data struct describing a terminal: its dimensions in cells and
/// the color depth. Constructed by the platform layer; consumed by Buffer
/// constructors and the renderer.
#include <cstdint>

namespace ash {
namespace render {

struct Terminal {
    std::uint16_t width_cells{80};
    std::uint16_t height_cells{24};
    bool supports_truecolor{true};
};

}  // namespace render
}  // namespace ash