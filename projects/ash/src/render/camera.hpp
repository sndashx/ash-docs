#pragma once
/// Phase 01 step 0107: World→screen camera with smooth follow.
#include <cstdint>

#include "core/math.hpp"

namespace ash {
namespace render {

struct Camera {
    using Fixed = core::math::Fixed;

    core::math::Vec2<Fixed> world_pos{};
    uint16_t width_cells        = 80;
    uint16_t height_cells       = 24;
    uint16_t map_width_cells    = 80;
    uint16_t map_height_cells   = 24;

    float lerp_factor = 0.25f;

    Camera() = default;

    void center_on(core::math::Vec2<Fixed> target);
    void move(core::math::Vec2<Fixed> delta);

    core::math::IVec2 world_to_screen(core::math::Vec2<Fixed> world) const;
    core::math::Vec2<Fixed> screen_to_world(core::math::IVec2 screen) const;
};

}  // namespace render
}  // namespace ash