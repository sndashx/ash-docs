#pragma once
/// Phase 04: Flood-fill tool.
///
/// 4-connected BFS that replaces every cell matching the seed (glyph
/// + flags + fg + bg) on the active layer with `ed.fill_cell`. The
/// whole fill is a single `FillCommand` so one Ctrl+Z reverts it.
#include "editor/editor.hpp"
#include "editor/map.hpp"
#include "editor/undo.hpp"

namespace ash {
namespace editor {

/// Run the fill at (x, y). Returns the number of cells affected.
std::size_t fill_at(Editor& ed, int x, int y);

}  // namespace editor
}  // namespace ash