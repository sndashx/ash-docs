#pragma once
/// Phase 04: Brush / paint tool.
///
/// Paints `Editor::paint_cell` (glyph + fg + bg) onto every cell of
/// an NxN square centered at (x, y) on `Editor::active_layer`. The
/// paint itself is wrapped in a single `PaintRectCommand` so the
/// whole brush stroke is one undo step.
#include "editor/map.hpp"
#include "editor/undo.hpp"

#include "render/cell.hpp"

namespace ash {
namespace editor {

/// Stamp an NxN brush centered at (cx, cy) onto the map. Returns
/// the number of cells actually painted (clipped to map bounds).
std::size_t paint_stamp(Map& m, UndoStack& undo,
                         int layer, int cx, int cy,
                         int brush_size,
                         render::Cell paint);

}  // namespace editor
}  // namespace ash