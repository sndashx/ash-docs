#include "editor/tool_warp.hpp"

namespace ash {
namespace editor {

void warp_to(Editor& ed, int x, int y) {
    if (!ed.map.in_bounds(x, y)) return;
    ed.push(std::make_unique<WarpPlayerCommand>(IVec2{ x, y }));
}

}  // namespace editor
}  // namespace ash