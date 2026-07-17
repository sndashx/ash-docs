#pragma once
/// Phase 04: Pipette (color picker).
///
/// On click, samples the cell under the cursor on the current layer
/// and copies it onto the editor's `paint_cell` so the next paint
/// stamps the same glyph + fg + bg.
#include "editor/editor.hpp"
#include "editor/map.hpp"

#include "render/cell.hpp"

namespace ash {
namespace editor {

/// Sample the cell at (x, y) on `ed`'s active layer into
/// `ed.paint_cell`. Also stores it on `ed.picked_cell` so the HUD
/// can show what was last picked.
void pick_at(Editor& ed, int x, int y);

}  // namespace editor
}  // namespace ash