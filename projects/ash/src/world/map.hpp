#pragma once
/// Phase 07: Map = walkability + collision grid + door states.
/// The Map owns an 80x50 grid of tile flags (per pillar D65 dimensions).
/// Phase 7 uses the grid for pathfinding, line of sight, and cover. The
/// rexpaint layers themselves are still in Phase 3 — this Map is the
/// in-memory representation the combat layer actually queries. Later
/// phases can rehydrate the grid from `.xp` layer data; for Phase 7 the
/// Map is built programmatically via the helper factory functions.
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "core/ids.hpp"
#include "world/tile_flags.hpp"

namespace ash {
namespace world {

inline constexpr int kMapWidth  = 80;
inline constexpr int kMapHeight = 50;
inline constexpr int kMapCellCount = kMapWidth * kMapHeight;

inline constexpr int idx(int x, int y) noexcept {
    return y * kMapWidth + x;
}

struct Map {
    core::MapId                                id{};
    std::string                                name{};
    /// Tile flags indexed by `(y * kMapWidth + x)`.
    std::array<std::uint16_t, kMapCellCount>   tiles{};
    /// Door state per (x,y). `true` = open (walkable), `false` = closed
    /// (counts as TF_BLOCKING unless the entity has the `CanOpen` flag).
    std::unordered_map<int, bool>              doors{};

    /// True if the cell is in-bounds and has the TF_WALKABLE flag, or
    /// is a closed door whose occupant is openable (callers pass
    /// `can_open=true` for the entity checking).
    bool is_walkable(int x, int y, bool can_open_doors = false) const noexcept;

    /// True if the cell is in-bounds and has the TF_OPAQUE flag. Closed
    /// doors are also opaque (they're physical objects).
    bool is_opaque(int x, int y) const noexcept;

    /// True if the cell is in-bounds and has the TF_BLOCKING flag. Closed
    /// doors block movement. Walls block movement.
    bool is_blocking(int x, int y, bool can_open_doors = false) const noexcept;

    /// Toggle a door. `x`/`y` must refer to a door tile.
    void set_door_open(int x, int y, bool open) noexcept;

    /// Returns a quick hash of the current door-state layout. Used by the
    /// A* cache (Step 0706.00.02): path caches are invalid whenever the
    /// door-state hash changes. Maps with no doors return 0.
    std::uint64_t door_state_hash() const noexcept;
};

/// Build an empty `w x h` map with all cells walkable.
Map make_open_map(int w, int h) noexcept;

/// Build a rectangular wall-bordered arena: borders are blocking, the
/// inside is walkable. Useful for combat arenas and tests.
Map make_arena(int w, int h) noexcept;

}  // namespace world
}  // namespace ash
