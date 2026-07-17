#include "editor/tool_fill.hpp"

namespace ash {
namespace editor {

std::size_t fill_at(Editor& ed, int x, int y) {
    if (!ed.map.in_bounds(x, y) || !ed.map.valid_layer(ed.active_layer)) return 0;
    auto cmd = std::make_unique<FillCommand>(ed.active_layer, x, y, ed.fill_cell);
    // Apply first so we can read the affected count for the HUD.
    cmd->apply(ed.map);
    std::size_t count = cmd->affected_count();
    // Move the (already-applied) command directly onto the undo
    // stack without re-applying.
    ed.undo.push_already_applied(std::move(cmd));
    return count;
}

}  // namespace editor
}  // namespace ash