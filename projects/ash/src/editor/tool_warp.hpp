#pragma once
/// Phase 04: Warp-to tool (debug/dev).
///
/// Click anywhere on the map to teleport the player (entity id=0) to
/// that cell. Revert restores the prior position.
#include "editor/editor.hpp"
#include "editor/map.hpp"

namespace ash {
namespace editor {

/// Move the player to (x, y). Pushes `WarpPlayerCommand` onto the
/// editor's UndoStack so Ctrl+Z restores the original position.
void warp_to(Editor& ed, int x, int y);

}  // namespace editor
}  // namespace ash