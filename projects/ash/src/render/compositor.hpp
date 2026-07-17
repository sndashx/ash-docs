#pragma once
/// Phase 01 step 0109: Layer compositor.
///
/// Merges 9 buffers (REXPaint layer 0..8) into a single buffer according
/// to cell flags: invisible cells skipped, opaque cells overwrite,
/// transparent cells with non-blank bg alpha-blend, then apply light.
#include <array>
#include <cstdint>

#include "render/buffer.hpp"
#include "render/cell.hpp"
#include "render/light.hpp"

namespace ash {
namespace render {

namespace cell_flag {
inline constexpr std::uint8_t INVISIBLE  = 1;
inline constexpr std::uint8_t OPAQUE     = 2;
inline constexpr std::uint8_t TRANSPARENT= 4;
}  // namespace cell_flag

struct CompositeInput {
    std::array<Buffer, 9> layers{};
    LightGrid const*      light{nullptr};
    std::uint8_t          ambient{0};
};

/// Composite 9 layers + lighting into a fresh buffer sized to the layers.
Buffer composite(CompositeInput const& in);

}  // namespace render
}  // namespace ash