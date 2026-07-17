#include "editor/tool_entity.hpp"

namespace ash {
namespace editor {

std::uint64_t entity_place(Editor& ed, int x, int y) {
    if (!ed.map.in_bounds(x, y)) return 0;
    EntitySpec spec;
    spec.id       = ed.next_entity_id++;
    spec.kind     = ed.entity_kind;
    spec.type_id  = ed.entity_type_id;
    spec.pos      = { x, y };
    spec.layer    = 4;
    ed.push(std::make_unique<PlaceEntityCommand>(spec));
    return spec.id;
}

void entity_cycle_kind(Editor& ed, int delta) noexcept {
    int k = static_cast<int>(ed.entity_kind);
    int n = static_cast<int>(EntityKind::Count);
    k = (k + delta + n) % n;
    ed.entity_kind = static_cast<EntityKind>(k);
}

}  // namespace editor
}  // namespace ash