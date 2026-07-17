#include "editor/tool_select.hpp"

#include <vector>

namespace ash {
namespace editor {

void select_begin(Editor& ed, int x, int y) noexcept {
    ed.sel_anchor = { x, y };
    ed.selection  = { x, y, x, y };
    ed.selecting  = true;
}

void select_update(Editor& ed, int x, int y) noexcept {
    ed.selection.x0 = ed.sel_anchor.x;
    ed.selection.y0 = ed.sel_anchor.y;
    ed.selection.x1 = x;
    ed.selection.y1 = y;
    ed.selection    = ed.selection.normalize();
}

void select_commit(Editor& ed) noexcept {
    ed.selecting = false;
}

void select_delete(Editor& ed) {
    IRect r = ed.selection.empty()
                ? IRect{ ed.cursor.x, ed.cursor.y, ed.cursor.x, ed.cursor.y }
                : ed.selection.normalize();
    if (r.empty()) return;
    for (int y = r.y0; y <= r.y1; ++y) {
        for (int x = r.x0; x <= r.x1; ++x) {
            if (!ed.map.in_bounds(x, y)) continue;
            render::Cell old_c = ed.map.at(ed.active_layer, x, y);
            if (old_c.glyph == render::CELL_BLANK.glyph
                && old_c.fg_r == 0 && old_c.fg_g == 0 && old_c.fg_b == 0
                && old_c.bg_r == 0 && old_c.bg_g == 0 && old_c.bg_b == 0) {
                continue;  // already blank — don't push a no-op.
            }
            ed.push(std::make_unique<PaintCellCommand>(
                ed.active_layer, x, y, old_c, render::CELL_BLANK));
        }
    }
    // Snapshot ids in the region first; pushing commands mutates
    // ed.map.entities and would invalidate the range-for iterator.
    std::vector<std::uint64_t> ids;
    ids.reserve(ed.map.entities.size());
    for (auto const& e : ed.map.entities) {
        if (r.contains(e.pos.x, e.pos.y) && e.id != 0) {
            ids.push_back(e.id);
        }
    }
    for (auto id : ids) {
        ed.push(std::make_unique<DeleteEntityCommand>(id));
    }
}

}  // namespace editor
}  // namespace ash
