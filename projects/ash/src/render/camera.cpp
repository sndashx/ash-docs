#include "render/camera.hpp"

#include <algorithm>
#include <cmath>

namespace ash {
namespace render {

namespace {

inline core::math::Fixed to_fixed(float v) noexcept {
    return core::math::from_float(v);
}

inline core::math::Vec2<core::math::Fixed> lerp_v(
    core::math::Vec2<core::math::Fixed> cur,
    core::math::Vec2<core::math::Fixed> tgt,
    float t) noexcept {
    auto mix = [t](core::math::Fixed a, core::math::Fixed b) -> core::math::Fixed {
        float fa = core::math::to_float(a);
        float fb = core::math::to_float(b);
        return core::math::from_float(fa + (fb - fa) * t);
    };
    return core::math::Vec2<core::math::Fixed>{
        mix(cur.x, tgt.x),
        mix(cur.y, tgt.y)
    };
}

}  // namespace

void Camera::center_on(core::math::Vec2<Fixed> target) {
    // Desired camera position so that `target` is in the center of the viewport.
    Fixed half_w = core::math::from_int(width_cells / 2);
    Fixed half_h = core::math::from_int(height_cells / 2);
    core::math::Vec2<Fixed> desired{target.x - half_w, target.y - half_h};

    // Clamp to map bounds: world_pos ∈ [0, map_dim - view_dim].
    Fixed max_x = core::math::from_int(
        (map_width_cells  > width_cells  ? map_width_cells  - width_cells  : 0));
    Fixed max_y = core::math::from_int(
        (map_height_cells > height_cells ? map_height_cells - height_cells : 0));

    if (desired.x < core::math::Fixed{0}) desired.x = core::math::Fixed{0};
    if (desired.y < core::math::Fixed{0}) desired.y = core::math::Fixed{0};
    if (desired.x > max_x) desired.x = max_x;
    if (desired.y > max_y) desired.y = max_y;

    world_pos = lerp_v(world_pos, desired, lerp_factor);
}

void Camera::move(core::math::Vec2<Fixed> delta) {
    world_pos = world_pos + delta;
    if (world_pos.x < core::math::Fixed{0}) world_pos.x = core::math::Fixed{0};
    if (world_pos.y < core::math::Fixed{0}) world_pos.y = core::math::Fixed{0};
    Fixed max_x = core::math::from_int(
        (map_width_cells  > width_cells  ? map_width_cells  - width_cells  : 0));
    Fixed max_y = core::math::from_int(
        (map_height_cells > height_cells ? map_height_cells - height_cells : 0));
    if (world_pos.x > max_x) world_pos.x = max_x;
    if (world_pos.y > max_y) world_pos.y = max_y;
}

core::math::IVec2 Camera::world_to_screen(core::math::Vec2<Fixed> world) const {
    int sx = core::math::to_int(world.x - world_pos.x);
    int sy = core::math::to_int(world.y - world_pos.y);
    return { sx, sy };
}

core::math::Vec2<Camera::Fixed> Camera::screen_to_world(core::math::IVec2 screen) const {
    return { world_pos.x + core::math::from_int(screen.x),
             world_pos.y + core::math::from_int(screen.y) };
}

}  // namespace render
}  // namespace ash