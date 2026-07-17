#include "editor/tool_pick.hpp"

namespace ash {
namespace editor {

void pick_at(Editor& ed, int x, int y) {
    if (!ed.map.in_bounds(x, y) || !ed.map.valid_layer(ed.active_layer)) return;
    render::Cell c = ed.map.at(ed.active_layer, x, y);
    ed.picked_cell = c;
    ed.paint_cell  = c;
}

}  // namespace editor
}  // namespace ash