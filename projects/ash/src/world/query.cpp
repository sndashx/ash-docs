#include "world/query.hpp"

#include <algorithm>
#include <cmath>

namespace ash {
namespace world {

int chebyshev(core::IVec2 a, core::IVec2 b) noexcept {
    int const dx = std::abs(a.x - b.x);
    int const dy = std::abs(a.y - b.y);
    return std::max(dx, dy);
}

std::vector<Entity const*> entities_in_radius(World const& w,
                                              core::IVec2 center,
                                              int radius_cells) noexcept {
    std::vector<Entity const*> out;
    if (radius_cells < 0) {
        return out;
    }
    for (auto const& e : w.entities) {
        if (!e.has_position || !e.alive) {
            continue;
        }
        int const d = chebyshev(e.position.cell, center);
        if (d <= radius_cells) {
            out.push_back(&e);
        }
    }
    return out;
}

Entity const* occupant_at(World const& w, core::IVec2 c) noexcept {
    if (c.x < 0 || c.x >= kMapWidth || c.y < 0 || c.y >= kMapHeight) {
        return nullptr;
    }
    if (w.cell_occupant.empty()) {
        for (auto const& e : w.entities) {
            if (e.has_position && e.alive && e.position.cell == c) {
                return &e;
            }
        }
        return nullptr;
    }
    core::EntityId oid = w.cell_occupant[static_cast<std::size_t>(idx(c.x, c.y))];
    if (oid.value == 0) {
        return nullptr;
    }
    return w.find(oid);
}

}  // namespace world
}  // namespace ash
