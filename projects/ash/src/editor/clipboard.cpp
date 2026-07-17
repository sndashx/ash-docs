#include "editor/clipboard.hpp"

#include <algorithm>

namespace ash {
namespace editor {

bool Clipboard::copy(Map const& m, IRect region_in, int layer_in) {
    if (!m.valid_layer(layer_in)) return false;
    IRect r = region_in.normalize();
    if (r.empty()) return false;

    cells.clear();
    entities.clear();
    has_data = false;
    region  = r;
    layer   = layer_in;

    // Clip against the map bounds; cells outside get CELL_BLANK.
    for (int y = r.y0; y <= r.y1; ++y) {
        for (int x = r.x0; x <= r.x1; ++x) {
            cells.push_back(m.at(layer_in, x, y));
        }
    }
    for (auto const& e : m.entities) {
        if (r.contains(e.pos.x, e.pos.y)) entities.push_back(e);
    }
    has_data = true;
    return true;
}

void Clipboard::cut(Map& m, IRect region_in, int layer_in) {
    if (!copy(m, region_in, layer_in)) return;
    // After the copy we erase the source cells with CELL_BLANK. The
    // caller decides whether to wrap this in a command (typical cut
    // pushes a single PaintRectCommand for the region).
    IRect r = region;
    for (int y = r.y0; y <= r.y1; ++y) {
        for (int x = r.x0; x <= r.x1; ++x) {
            if (!m.in_bounds(x, y)) continue;
            m.cell(layer_in, x, y) = render::CELL_BLANK;
        }
    }
    // Remove in-region entities; their prior state lives on the
    // caller's DeleteEntityCommand if one was pushed.
    m.entities.erase(std::remove_if(m.entities.begin(), m.entities.end(),
        [&](EntitySpec const& e) { return r.contains(e.pos.x, e.pos.y); }),
        m.entities.end());
}

std::size_t Clipboard::paste(Map& m, int x, int y, UndoStack& undo) {
    if (!has_data || region.empty()) return 0;
    std::size_t writes = 0;
    int const W = region.width();
    int const H = region.height();

    // Cells.
    auto make_paint_cell = [&](int lx, int ly, render::Cell new_c) {
        if (!m.in_bounds(lx, ly) || !m.valid_layer(layer)) return;
        render::Cell old_c = m.at(layer, lx, ly);
        undo.push(std::make_unique<PaintCellCommand>(
            layer, lx, ly, old_c, new_c), m);
        ++writes;
    };

    for (int row = 0; row < H; ++row) {
        for (int col = 0; col < W; ++col) {
            int lx = x + col;
            int ly = y + row;
            std::size_t idx = static_cast<std::size_t>(row) * static_cast<std::size_t>(W) + static_cast<std::size_t>(col);
            render::Cell c = cells[idx];
            // Skip blanks? No — paste puts blanks too so the user
            // can use paste as a "clear stamp".
            make_paint_cell(lx, ly, c);
        }
    }

    // Entities — re-id and re-position relative to (x, y).
    int dx_off = x - region.x0;
    int dy_off = y - region.y0;
    for (auto e : entities) {
        // Skip the player sentinel: pasting a player would violate
        // the uniqueness of (id == 0).
        if (e.id == 0) continue;
        e.pos.x += dx_off;
        e.pos.y += dy_off;
        e.id = next_entity_id++;
        undo.push(std::make_unique<PlaceEntityCommand>(std::move(e)), m);
    }
    return writes;
}

void Clipboard::clear() noexcept {
    cells.clear();
    entities.clear();
    has_data = false;
    region = IRect{};
    layer = 0;
}

}  // namespace editor
}  // namespace ash