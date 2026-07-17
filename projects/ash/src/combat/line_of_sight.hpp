#pragma once
/// Phase 07: line of sight (Step 0707).
/// Bresenham from `from` to `to`. The endpoint cell is included in the
/// trace (so the function tells you "the line ends in cell (x,y)"). A
/// closed door blocks LoS (closed doors are physical objects). Walls
/// (TF_OPAQUE) block LoS. Open doors and walkable terrain do not.
#include <cstdint>
#include <optional>
#include <vector>

#include "core/math.hpp"
#include "world/map.hpp"

namespace ash {
namespace combat {

struct LosResult {
    bool                       visible{false};
    /// The cell where the trace stopped. If `visible`, this is `to`.
    /// Otherwise this is the first opaque cell the ray hit.
    core::IVec2                blocker{};
    /// Every cell the ray crossed (including the endpoint).
    std::vector<core::IVec2>   path{};
};

/// Trace a line from `from` to `to`. Returns the result struct.
LosResult trace_line_of_sight(world::Map const& map,
                              core::IVec2       from,
                              core::IVec2       to) noexcept;

/// Convenience: is `to` visible from `from`?
inline bool has_line_of_sight(world::Map const& map,
                              core::IVec2       from,
                              core::IVec2       to) noexcept {
    return trace_line_of_sight(map, from, to).visible;
}

/// Return every cell in the Bresenham line from `from` to `to`,
/// inclusive of both endpoints.
std::vector<core::IVec2> bresenham_line(core::IVec2 from, core::IVec2 to) noexcept;

}  // namespace combat
}  // namespace ash
