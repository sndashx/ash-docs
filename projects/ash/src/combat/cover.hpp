#pragma once
/// Phase 07: cover calculation (Step 0708).
/// The cover hit penalty is:
///   * None             -> +0
///   * Half cover       -> +2 to hit
///   * Three-Quarters   -> +5 to hit
///   * Full cover       -> +10 to hit
/// Cover is decided by tracing a line from attacker to target and
/// checking the geometry around the target cell: how many blocking
/// neighbors it has, and whether the ray passes between solid walls.
#include <cstdint>

#include "core/math.hpp"
#include "world/map.hpp"

namespace ash {
namespace combat {

enum class Cover : std::uint8_t {
    None          = 0,
    Half          = 1,  /// 1-2 blocking neighbors.
    ThreeQuarters = 2,  /// 3 blocking neighbors or corner cell.
    Full          = 3,  /// Behind a solid wall on the attacker's line.
};

char const* cover_name(Cover c) noexcept;

/// Compute the cover value from `attacker` toward `target`. The caller
/// is responsible for passing the correct positions (the function does
/// not look up entities).
Cover compute_cover(world::Map const& map,
                    core::IVec2       attacker,
                    core::IVec2       target) noexcept;

/// Hit-chance penalty added to the attacker's roll. Always non-negative.
inline int cover_to_hit_penalty(Cover c) noexcept {
    switch (c) {
        case Cover::None:          return 0;
        case Cover::Half:          return 2;
        case Cover::ThreeQuarters: return 5;
        case Cover::Full:          return 10;
    }
    return 0;
}

}  // namespace combat
}  // namespace ash
