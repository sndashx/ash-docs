#pragma once
/// Phase 04: Place-entity tool.
///
/// Click drops an `EntitySpec` of the currently-selected kind at the
/// clicked cell. Deleting picks an entity up by id via
/// `editor::tool_select::select_delete` followed by explicit
/// DeleteEntityCommand.
#include "editor/editor.hpp"
#include "editor/map.hpp"

namespace ash {
namespace editor {

/// Place the currently-armed entity at (x, y). Returns the id of the
/// newly-placed entity (0 if the placement was rejected, e.g. the
/// cell was out of bounds).
std::uint64_t entity_place(Editor& ed, int x, int y);

/// Rotate through the entity kinds (1..Count). Useful when the
/// picker menu is not yet wired up.
void entity_cycle_kind(Editor& ed, int delta) noexcept;

}  // namespace editor
}  // namespace ash