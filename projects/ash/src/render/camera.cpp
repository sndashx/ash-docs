#include "render/camera.hpp"

#include <algorithm>

namespace ash {
namespace render {

void Camera::center_on(core::Vec2<core::Fixed> target) noexcept {
    // Camera tracks so the target lands at the center of the viewport.
    // Desired top-left = target - viewport/2.
    core::Fixed const half_w = core::fixed_from_int(width_cells / 2);
    core::Fixed const half_h = core::fixed_from_int(height_cells / 2);
    core::Vec2<core::Fixed> desired{
        target.x - half_w,
        target.y - half_h,
    };
    // Smooth lerp toward desired.
    world_pos.x = core::fixed_lerp(world_pos.x, desired.x, lerp_);
    world_pos.y = core::fixed_lerp(world_pos.y, desired.y, lerp_);
    clamp_to_map();
}

void Camera::move(core::Vec2<core::Fixed> delta) noexcept {
    world_pos += delta;
    clamp_to_map();
}

core::IVec2 Camera::world_to_screen(core::Vec2<core::Fixed> p) const noexcept {
    return core::IVec2{
        core::fixed_to_int(p.x - world_pos.x),
        core::fixed_to_int(p.y - world_pos.y),
    };
}

core::Vec2<core::Fixed> Camera::screen_to_world(core::IVec2 s) const noexcept {
    return core::Vec2<core::Fixed>{
        world_pos.x + core::fixed_from_int(s.x),
        world_pos.y + core::fixed_from_int(s.y),
    };
}

void Camera::clamp_to_map() noexcept {
    int const max_x = std::max(0,
        static_cast<int>(map_width_cells) - static_cast<int>(width_cells));
    int const max_y = std::max(0,
        static_cast<int>(map_height_cells) - static_cast<int>(height_cells));
    core::Fixed const cx = core::fixed_from_int(std::clamp(core::fixed_to_int(world_pos.x), 0, max_x));
    core::Fixed const cy = core::fixed_from_int(std::clamp(core::fixed_to_int(world_pos.y), 0, max_y));
    world_pos.x = cx;
    world_pos.y = cy;
}

void Camera::set_lerp_factor(core::Fixed t) noexcept {
    if (t < 0) t = 0;
    if (t > core::FIXED_ONE) t = core::FIXED_ONE;
    lerp_ = t;
}

}  // namespace render
}  // namespace ash