#pragma once
/// Phase 04: Selection rectangle tool.
///
/// Two-phase: press sets the anchor; release (or click+drag) updates
/// `selection`. The selection is consumed by Ctrl+C / Ctrl+X / Del.
#include "editor/editor.hpp"
#include "editor/map.hpp"

namespace ash {
namespace editor {

/// Begin a new selection at (x, y) on the current layer.
void select_begin(Editor& ed, int x, int y) noexcept;

/// Update the live selection extent. `x`/`y` may exceed the map
/// bounds; they are clamped internally.
void select_update(Editor& ed, int x, int y) noexcept;

/// Commit the current selection (mouse release). After commit the
/// selection is "final" until the next click.
void select_commit(Editor& ed) noexcept;

/// Erase the cells in the current selection (or, if empty, the cell
/// under the cursor). Each cleared cell is wrapped in a
/// PaintCellCommand for undo.
void select_delete(Editor& ed);

}  // namespace editor
}  // namespace ash