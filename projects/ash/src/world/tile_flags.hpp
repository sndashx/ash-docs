#pragma once
/// Phase 07: tile/cell flags used by collision, line of sight, and cover.
/// A tile is identified by its `(x, y)` grid position in a `Map` (defined
/// in `world/map.hpp`) and by a 16-bit flag bitmask. The bits are designed
/// to be ORed together: a single tile can simultaneously be opaque AND
/// blocking AND walkable-for-friendlies.
#include <cstdint>

namespace ash {
namespace world {

enum TileFlag : std::uint16_t {
    TF_NONE      = 0,
    TF_WALKABLE  = 1u << 0,  /// Creature pathing may enter.
    TF_OPAQUE    = 1u << 1,  /// Blocks line of sight and ranged attacks.
    TF_BLOCKING  = 1u << 2,  /// Solid wall: stops movement AND LoS AND cover.
    TF_DOOR      = 1u << 3,  /// Door tile; see `door_open` in Map.
    TF_HAZARD    = 1u << 4,  /// Damaging terrain (fire, spikes, ...).
    TF_LIGHT     = 1u << 5,  /// Tile emits light (Phase 1 renderer will read this).
    TF_WATER     = 1u << 6,
    TF_CHEST     = 1u << 7,  /// Interactable container.
    TF_STAIR     = 1u << 8,
};

inline constexpr std::uint16_t operator|(TileFlag a, TileFlag b) noexcept {
    return static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b);
}
inline constexpr std::uint16_t operator|(std::uint16_t a, TileFlag b) noexcept {
    return a | static_cast<std::uint16_t>(b);
}
inline constexpr bool has_flag(std::uint16_t f, TileFlag b) noexcept {
    return (f & static_cast<std::uint16_t>(b)) != 0;
}

}  // namespace world
}  // namespace ash
