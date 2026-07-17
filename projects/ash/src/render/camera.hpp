#pragma once
/// Phase 01 step 0107: World<->screen coordinate transform with smooth
/// follow and clamped scrolling.
#include <cstdint>

#include "core/math.hpp"

namespace ash {
namespace render {

struct Camera {
    core::Vec2<core::Fixed> world_pos{core::fixed_from_int(0),
                                      core::fixed_from_int(0)};
    std::uint16_t width_cells{80};
    std::uint16_t height_cells{24};
    std::uint16_t map_width_cells{80};
    std::uint16_t map_height_cells{24};

    /// Lerp world_pos toward target, clamped to the map bounds.
    void center_on(core::Vec2<core::Fixed> target) noexcept;

    /// Apply a delta to world_pos, clamped to map bounds.
    void move(core::Vec2<core::Fixed> delta) noexcept;

    /// Convert a world position to a screen-relative cell offset.
    core::IVec2 world_to_screen(core::Vec2<core::Fixed> p) const noexcept;

    /// Inverse of world_to_screen (screen offset back to world coord).
    core::Vec2<core::Fixed> screen_to_world(core::IVec2 s) const noexcept;

    /// Clamp world_pos so the viewport stays inside the map.
    void clamp_to_map() noexcept;

    /// Set the lerp factor used by center_on (0..1, fixed-point).
    void set_lerp_factor(core::Fixed t) noexcept;
    core::Fixed lerp_factor() const noexcept { return lerp_; }

private:
    core::Fixed lerp_{core::fixed_from_float(0.25f)};
};

}  // namespace render
}  // namespace ash