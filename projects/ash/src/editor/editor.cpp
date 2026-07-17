#include "editor/editor.hpp"

#include "editor/clipboard.hpp"
#include "editor/map_io.hpp"
#include "editor/tool_entity.hpp"
#include "editor/tool_fill.hpp"
#include "editor/tool_paint.hpp"
#include "editor/tool_pick.hpp"
#include "editor/tool_select.hpp"
#include "editor/tool_warp.hpp"

#include <cstdio>
#include <string>

namespace ash {
namespace editor {

namespace {

char const* kToolNames[static_cast<int>(Tool::Count)] = {
    "Paint", "Select", "Pick", "Fill", "Entity", "Warp"
};
char const kToolHotkeys[static_cast<int>(Tool::Count)] = {
    'P', 'S', 'I', 'F', 'E', 'W'
};

constexpr bool is_digit_layer(char c) noexcept {
    return c >= '1' && c <= '9';
}

}  // namespace

char const* tool_name(Tool t) noexcept {
    int i = static_cast<int>(t);
    if (i < 0 || i >= static_cast<int>(Tool::Count)) return "?";
    return kToolNames[i];
}

char tool_hotkey(Tool t) noexcept {
    int i = static_cast<int>(t);
    if (i < 0 || i >= static_cast<int>(Tool::Count)) return '?';
    return kToolHotkeys[i];
}

void Editor::init() {
    current_tool  = Tool::Paint;
    active_layer  = 1;
    paint_cell    = render::Cell{ uint32_t('#'), 220, 220, 220 };
    fill_cell     = render::Cell{ uint32_t('.'), 100, 100, 100 };
    brush_size    = 1;
    selecting     = false;
    selection     = {};
    cursor        = { 0, 0 };
    undo.clear();
    clipboard.clear();
}

EditorAction Editor::handle(EditorInput const& ev) {
    using K = EditorInput::Kind;
    switch (ev.kind) {
    case K::Quit: return EditorAction::Exit;
    case K::KeyEsc: return EditorAction::Exit;

    case K::Char:
        // Tool hotkeys.
        switch (ev.letter) {
        case 'p': case 'P': current_tool = Tool::Paint;  return EditorAction::None;
        case 's': case 'S': current_tool = Tool::Select; return EditorAction::None;
        case 'i': case 'I': current_tool = Tool::Pick;   return EditorAction::None;
        case 'f': case 'F': current_tool = Tool::Fill;   return EditorAction::None;
        case 'e': case 'E': current_tool = Tool::Entity; return EditorAction::None;
        case 'w': case 'W': current_tool = Tool::Warp;   return EditorAction::None;
        }
        if (ev.letter == '+' || ev.letter == '=') {
            brush_size = (brush_size >= 7) ? 7 : brush_size + 2;
            return EditorAction::None;
        }
        if (ev.letter == '-' || ev.letter == '_') {
            brush_size = (brush_size <= 1) ? 1 : brush_size - 2;
            return EditorAction::None;
        }
        if (ev.letter == 'k') { entity_cycle_kind(*this, +1); return EditorAction::None; }
        if (ev.letter == 'K') { entity_cycle_kind(*this, -1); return EditorAction::None; }
        break;

    case K::Digit:
        if (ev.letter >= '1' && ev.letter <= '9') {
            active_layer = ev.letter - '0';
        } else if (ev.letter == '0') {
            overlay_enabled = !overlay_enabled;
        }
        return EditorAction::None;

    case K::KeyUp:    if (cursor.y > 0) cursor.y--;                  return EditorAction::None;
    case K::KeyDown:  if (cursor.y + 1 < map.height) cursor.y++;      return EditorAction::None;
    case K::KeyLeft:  if (cursor.x > 0) cursor.x--;                  return EditorAction::None;
    case K::KeyRight: if (cursor.x + 1 < map.width) cursor.x++;       return EditorAction::None;
    case K::KeyHome:  cursor = { 0, 0 };                             return EditorAction::None;
    case K::KeyEnd:   cursor = { map.width - 1, map.height - 1 };    return EditorAction::None;
    case K::KeyPageUp:
        cursor.y = (cursor.y >= 5) ? cursor.y - 5 : 0;
        return EditorAction::None;
    case K::KeyPageDown:
        cursor.y = (cursor.y + 5 < map.height) ? cursor.y + 5 : map.height - 1;
        return EditorAction::None;

    case K::MouseMove:
        cursor = { ev.mouse_x, ev.mouse_y };
        if (selecting) select_update(*this, ev.mouse_x, ev.mouse_y);
        return EditorAction::None;

    case K::MouseClick:
        cursor = { ev.mouse_x, ev.mouse_y };
        switch (current_tool) {
        case Tool::Paint:
            paint_stamp(map, undo, active_layer, cursor.x, cursor.y, brush_size, paint_cell);
            break;
        case Tool::Select:
            select_begin(*this, cursor.x, cursor.y);
            break;
        case Tool::Pick:
            pick_at(*this, cursor.x, cursor.y);
            break;
        case Tool::Fill:
            fill_at(*this, cursor.x, cursor.y);
            break;
        case Tool::Entity:
            entity_place(*this, cursor.x, cursor.y);
            break;
        case Tool::Warp:
            warp_to(*this, cursor.x, cursor.y);
            break;
        default: break;
        }
        return EditorAction::None;

    case K::MouseRelease:
        if (selecting) select_commit(*this);
        return EditorAction::None;

    case K::Ctrl:
        // Ctrl+letter combos.
        switch (ev.ctrl_letter) {
        case 'z': case 'Z': undo.undo(map); return EditorAction::None;
        case 'y': case 'Y': undo.redo(map); return EditorAction::None;
        case 'c': case 'C':
            if (!selection.empty())
                clipboard.copy(map, selection, active_layer);
            return EditorAction::None;
        case 'x': case 'X': {
            if (!selection.empty()) {
                // push a PaintRectCommand for clearing the source so
                // the cut is reversible in one undo step.
                undo.push(std::make_unique<PaintRectCommand>(
                    active_layer, selection, render::CELL_BLANK), map);
                clipboard.copy(map, selection, active_layer);
                // Snapshot ids in the region first; pushing commands
                // mutates map.entities, which would invalidate the
                // range-for iterator.
                IRect r = selection.normalize();
                std::vector<std::uint64_t> ids;
                ids.reserve(map.entities.size());
                for (auto const& e : map.entities) {
                    if (r.contains(e.pos.x, e.pos.y) && e.id != 0) {
                        ids.push_back(e.id);
                    }
                }
                for (auto id : ids) {
                    undo.push(std::make_unique<DeleteEntityCommand>(id), map);
                }
            }
            return EditorAction::None;
        }
        case 'v': case 'V':
            clipboard.paste(map, cursor.x, cursor.y, undo);
            return EditorAction::None;
        case 's': case 'S':
            // App-level save; here we just notify via the action enum.
            return EditorAction::Saved;
        case 'q': case 'Q': return EditorAction::Exit;
        }
        return EditorAction::None;

    case K::F5: return EditorAction::Reloaded;
    case K::F7: return EditorAction::Exit;
    case K::KeyBackspace:
        undo.undo(map);
        return EditorAction::None;
    case K::KeyEnter:
    case K::KeyTab:
    case K::Shift:
        return EditorAction::None;
    }
    return EditorAction::None;
}

std::string Editor::status_line() const {
    char buf[160];
    std::snprintf(buf, sizeof(buf),
        "EDITOR [tool=%-6s layer=%d/9 brush=%d] undo=%zu redo=%zu clip=%s",
        tool_name(current_tool),
        active_layer, brush_size,
        undo.undo_size(), undo.redo_size(),
        clipboard.has_data ? "ready" : "empty");
    return std::string(buf);
}

std::string Editor::hud_text(int viewport_w, int viewport_h) const {
    (void)viewport_w;
    (void)viewport_h;
    char line[160];
    std::string out;
    out += "--- ASH Editor ---\n";
    out += "Tools: ";
    for (int i = 0; i < static_cast<int>(Tool::Count); ++i) {
        Tool t = static_cast<Tool>(i);
        char tag[20];
        if (t == current_tool)
            std::snprintf(tag, sizeof(tag), "[%c]%s* ",
                          tool_hotkey(t), tool_name(t));
        else
            std::snprintf(tag, sizeof(tag), "[%c]%s  ",
                          tool_hotkey(t), tool_name(t));
        out += tag;
    }
    out += "\n";
    out += "Layers: ";
    for (int l = 1; l <= kLayerCount; ++l) {
        if (l == active_layer)
            std::snprintf(line, sizeof(line), "[%d]*", l);
        else
            std::snprintf(line, sizeof(line), "[%d] ", l);
        out += line;
    }
    out += overlay_enabled ? "  [0]overlay=on" : "  [0]overlay=off";
    out += "\n";
    std::snprintf(line, sizeof(line),
        "Cursor: (%d,%d)  paint='%c'(%u,%u,%u)  fill='%c'  clip=%s  size=%dx%d",
        cursor.x, cursor.y,
        static_cast<char>(paint_cell.glyph),
        paint_cell.fg_r, paint_cell.fg_g, paint_cell.fg_b,
        static_cast<char>(fill_cell.glyph),
        clipboard.has_data ? "yes" : "no",
        map.width, map.height);
    out += line;
    out += "\n";
    out += "Keys: Ctrl+Z/Y undo/redo, Ctrl+C/X/V clip, + - brush, 1-9 layer, 0 overlay, Esc quit, F5 reload, Ctrl+S save\n";
    return out;
}

bool Editor::save_to(std::string const& path) const {
    return save_map(map, path);
}

bool Editor::load_from(std::string const& path) {
    Map next;
    if (!load_map(next, path)) return false;
    map = std::move(next);
    undo.clear();
    clipboard.clear();
    return true;
}

}  // namespace editor
}  // namespace ash
