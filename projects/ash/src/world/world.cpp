#include "world/world.hpp"

namespace ash {
namespace world {

core::EntityId World::add(Entity e) noexcept {
    if (e.id.value == 0) {
        e.id = core::EntityId{next_entity_id++};
    } else if (e.id.value >= next_entity_id) {
        next_entity_id = e.id.value + 1;
    }
    core::EntityId const out = e.id;
    entities.push_back(std::move(e));
    return out;
}

bool World::remove(core::EntityId id) noexcept {
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        if (it->id == id) {
            entities.erase(it);
            return true;
        }
    }
    return false;
}

Entity* World::find(core::EntityId id) noexcept {
    for (auto& e : entities) {
        if (e.id == id) {
            return &e;
        }
    }
    return nullptr;
}

Entity const* World::find(core::EntityId id) const noexcept {
    for (auto const& e : entities) {
        if (e.id == id) {
            return &e;
        }
    }
    return nullptr;
}

void World::rebuild_occupancy() noexcept {
    cell_occupant.clear();
    cell_occupant.resize(static_cast<std::size_t>(kMapCellCount), core::EntityId{});
    for (auto const& e : entities) {
        if (!e.has_position || !e.alive) {
            continue;
        }
        int const x = e.position.cell.x;
        int const y = e.position.cell.y;
        if (x < 0 || x >= kMapWidth || y < 0 || y >= kMapHeight) {
            continue;
        }
        cell_occupant[static_cast<std::size_t>(idx(x, y))] = e.id;
    }
}

}  // namespace world
}  // namespace ash
