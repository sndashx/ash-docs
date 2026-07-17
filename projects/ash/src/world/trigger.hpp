#pragma once
/// Phase 07: trigger volumes. Phase 7 doesn't yet have a real trigger
/// system (Phase 8/9 hook into quest state); the placeholder API exists
/// so combat encounters can declare a "begin combat when player enters"
/// trigger without depending on the full quest runtime.
#include "core/math.hpp"
#include "world/world.hpp"

namespace ash {
namespace world {

/// One rectangular trigger volume. When the player crosses into it,
/// `on_enter` is fired.
struct Trigger {
    core::IVec2  origin{};
    int          width{0};
    int          height{0};
    /// Script tag — "enter_combat:bandit_camp" etc. Filled in by Phase 9.
    const char*  tag{nullptr};
};

/// Returns true if `cell` lies inside `t`.
inline bool trigger_contains(Trigger const& t, core::IVec2 cell) noexcept {
    return cell.x >= t.origin.x && cell.x < t.origin.x + t.width &&
           cell.y >= t.origin.y && cell.y < t.origin.y + t.height;
}

}  // namespace world
}  // namespace ash
