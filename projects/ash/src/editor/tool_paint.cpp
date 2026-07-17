#include "editor/tool_paint.hpp"

namespace ash {
namespace editor {

std::size_t paint_stamp(Map& m, UndoStack& undo,
                        int layer, int cx, int cy,
                        int brush_size,
                        render::Cell paint)
{
    if (!m.valid_layer(layer) || brush_size < 1) return 0;
    int const half = brush_size / 2;
    IRect r;
    r.x0 = cx - half;
    r.y0 = cy - half;
    r.x1 = cx + (brush_size - 1 - half);
    r.y1 = cy + (brush_size - 1 - half);
    r = r.normalize();

    // Single rect command for the entire stamp — one undo step.
    undo.push(std::make_unique<PaintRectCommand>(layer, r, paint), m);
    return static_cast<std::size_t>(r.area());
}

}  // namespace editor
}  // namespace ash
