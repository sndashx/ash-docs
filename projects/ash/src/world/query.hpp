#pragma once
/// Phase 07: spatial queries. Phase 7 only needs `entities_in_radius` for
/// AI sensing and the iterator interface for the cell occupancy grid.
#include <vector>

#include "core/ids.hpp"
#include "core/math.hpp"
#include "world/world.hpp"

namespace ash {
namespace world {

/// Returns every entity whose position is within `radius_cells` of
/// `center` (Chebyshev / king-move distance, which matches grid combat).
std::vector<Entity const*> entities_in_radius(World const& w,
                                              core::IVec2 center,
                                              int radius_cells) noexcept;

/// Return the entity currently occupying cell `c`, or nullptr.
Entity const* occupant_at(World const& w, core::IVec2 c) noexcept;

/// Distance helper (Chebyshev). 0 = same cell.
int chebyshev(core::IVec2 a, core::IVec2 b) noexcept;

}  // namespace world
}  // namespace ash
