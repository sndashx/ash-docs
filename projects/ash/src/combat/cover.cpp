#include "combat/cover.hpp"

#include "combat/line_of_sight.hpp"

namespace ash {
namespace combat {

char const* cover_name(Cover c) noexcept {
    switch (c) {
        case Cover::None:          return "none";
        case Cover::Half:          return "half";
        case Cover::ThreeQuarters: return "three_quarters";
        case Cover::Full:          return "full";
    }
    return "none";
}

namespace {

/// Returns the number of the 8 neighbors of `c` that are blocking
/// (TF_BLOCKING + closed doors).
int count_blocking_neighbors(world::Map const& map, core::IVec2 c) noexcept {
    static constexpr int dx[8] = {-1,  0,  1, -1, 1, -1, 0, 1};
    static constexpr int dy[8] = {-1, -1, -1,  0, 0,  1, 1, 1};
    int n = 0;
    for (int i = 0; i < 8; ++i) {
        if (map.is_blocking(c.x + dx[i], c.y + dy[i], false)) {
            ++n;
        }
    }
    return n;
}

}  // namespace

Cover compute_cover(world::Map const& map,
                    core::IVec2       attacker,
                    core::IVec2       target) noexcept {
    /// First: if the line of sight passes through a wall, the target is
    /// in full cover behind it. (This is a "tucked behind wall" situation.)
    LosResult r = trace_line_of_sight(map, attacker, target);
    if (!r.visible) {
        return Cover::Full;
    }
    /// Even with clear LoS, count blocking neighbors of the target.
    int neighbors = count_blocking_neighbors(map, target);
    if (neighbors >= 3) return Cover::ThreeQuarters;
    if (neighbors >= 1) return Cover::Half;
    return Cover::None;
}

}  // namespace combat
}  // namespace ash