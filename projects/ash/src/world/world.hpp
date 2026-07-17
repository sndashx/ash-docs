#pragma once
/// Phase 07: World = the container of maps + entities + (later) scripts.
/// Phase 7 only needs the minimum: the active map, the entity list, and
/// lookup helpers used by the combat layer. Save/load and scripted
/// triggers are stubbed and scheduled for later phases.
#include <vector>

#include "core/ids.hpp"
#include "world/components.hpp"
#include "world/map.hpp"

namespace ash {
namespace world {

struct World {
    Map                active_map{};
    std::vector<Entity> entities{};
    core::EntityId     player_id{};
    /// Monotonically increasing id allocator; incremented by `add`.
    std::size_t        next_entity_id{1};

    /// Add a pre-constructed entity. Assigns it a fresh EntityId if its
    /// `id.value == 0`. Returns the (now valid) EntityId.
    core::EntityId add(Entity e) noexcept;

    /// Remove the entity with the given id. Returns true if a removal
    /// actually happened (no-op otherwise).
    bool remove(core::EntityId id) noexcept;

    /// Look up an entity by id. Returns nullptr if missing.
    Entity*       find(core::EntityId id) noexcept;
    Entity const* find(core::EntityId id) const noexcept;

    /// Index-by-cell occupancy grid for fast neighbor lookups. Rebuilt
    /// by `rebuild_occupancy`.
    std::vector<core::EntityId> cell_occupant{};

    /// Refresh `cell_occupant` from current entity positions. Called by
    /// combat after every move step so AI can do a cheap scan.
    void rebuild_occupancy() noexcept;
};

}  // namespace world
}  // namespace ash
