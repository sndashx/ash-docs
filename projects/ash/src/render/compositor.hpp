#pragma once
/// Phase 01 step 0109: Layer compositor + per-cell light application.
#include <array>
#include <cstdint>

#include "render/buffer.hpp"
#include "render/cell.hpp"
#include "render/light.hpp"

namespace ash {
namespace render {

struct CompositeInput {
    std::array<Buffer, 9> layers{};
    LightGrid const*      light = nullptr;
    uint8_t               ambient = 0;
};

Buffer composite(CompositeInput const& input);

}  // namespace render
}  // namespace ash