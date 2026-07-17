#pragma once
/// Phase 04: In-game editor.
///
/// The `Editor` is a stateful object that owns a `Map`, an `UndoStack`,
/// a `Clipboard`, and the six tool sub-states. `App` pushes an Editor
/// onto the mode stack when the user presses `e` from the menu (or
/// `F7` per the design doc, but we expose `e` for the placeholder
/// menu). It dispatches a small `EditorInput` event to `handle()`
/// and asks `render_status_line()` for an ASCII HUD line each frame.
///
/// The six tools (Paint, Select, Pick, Fill, Entity, Warp) are all
/// represented as small data members plus free-function helpers in
/// editor.cpp. This keeps the header small and makes it cheap to
/// inline-test from the ctests.
#include "editor/clipboard.hpp"
#include "editor/map.hpp"
#include "editor/undo.hpp"

#include "render/cell.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace ash {
namespace editor {

/// Which tool is currently armed. The integer values are part of the
/// wire format for the menu hotkeys (encoded as a single ASCII
/// letter), so do not reorder without a migration.
enum class Tool : std::uint8_t {
    Paint   = 0,
    Select  = 1,
    Pick    = 2,
    Fill    = 3,
    Entity  = 4,
    Warp    = 5,
    Count   = 6,
};

/// Compact key event the editor consumes. Real input plumbing lives
/// in Phase 5; for now App calls `handle` directly.
struct EditorInput {
    enum class Kind : std::uint8_t {
        Char,        // Printable ASCII glyph (no control modifier).
        Digit,       // '1'..'9' (treated as layer hotkey).
        KeyEsc,
        KeyEnter,
        KeyBackspace,
        KeyTab,
        KeyUp, KeyDown, KeyLeft, KeyRight,
        KeyHome, KeyEnd,
        KeyPageUp, KeyPageDown,
        MouseMove, MouseClick, MouseRelease,
        Ctrl,        // Ctrl+letter (letter in `ctrl_letter`).
        Shift,       // Shift+letter (uppercase ASCII in `letter`).
        F5,
        F7,
        Quit,
    };
    Kind kind = Kind::Char;
    char letter = 0;            // for Char/Shift/Ctrl.
    int  ctrl_letter = 0;       // lowercase letter (for Ctrl).
    int  mouse_x = 0;
    int  mouse_y = 0;
    bool shift = false;
    bool ctrl  = false;
    bool alt   = false;
};

/// Outcome of `handle()` so the app loop can react (close the editor,
/// save the map, etc.).
enum class EditorAction : std::uint8_t {
    None   = 0,
    Exit,         // User pressed Esc / Ctrl+Q — pop back to menu.
    Saved,        // User pressed Ctrl+S — map saved to disk.
    Reloaded,     // User pressed F5 — map reloaded from disk.
};

/// Top-level editor state. App owns one and toggles its `active`
/// flag when entering / leaving editor mode.
struct Editor {
    bool active = false;

    Tool current_tool = Tool::Paint;
    int  active_layer = 1;             // 1..9.

    /// Currently armed paint cell (glyph + fg + bg). Edits with the
    /// paint tool stamp this onto cells under the cursor.
    render::Cell paint_cell = render::Cell{ uint32_t('#'), 220, 220, 220 };

    /// Selection state (Select tool).
    bool        selecting  = false;
    IVec2       sel_anchor { 0, 0 };
    IRect       selection  {};

    /// Pick tool: last sampled cell.
    render::Cell picked_cell = render::CELL_BLANK;

    /// Fill tool: replacement cell. Defaults to a wall glyph so the
    /// first fill is meaningful.
    render::Cell fill_cell   = render::Cell{ uint32_t('#'), 200, 200, 200 };

    /// Entity tool: kind + content id used by the next placement.
    EntityKind entity_kind  = EntityKind::NPC;
    std::string entity_type_id = "npc_bandit_01";
    /// Monotonic counter for `EntitySpec::id`.
    std::uint64_t next_entity_id = 1;

    /// Warp tool: target cell; `0,0` means "unset".
    IVec2 warp_target { -1, -1 };

    /// Brush size for the paint tool. `1` is a single cell; `3` is a
    /// 3x3 square; etc.
    int brush_size = 1;

    /// Cursor (last known mouse / arrow-key position).
    IVec2 cursor { 0, 0 };

    /// Underlying map and editor systems.
    Map        map;
    UndoStack  undo;
    Clipboard  clipboard;

    /// Runtime overlay layer (layer 0 in the design doc) is held
    /// separately so save() can decide whether to write it back.
    bool overlay_enabled = true;

    Editor() {
        // Phase 4 default map: 80x25 blank — matches terminal size.
        map.resize(80, 25);
    }

    /// Construct against a pre-populated map.
    explicit Editor(Map m) : map(std::move(m)) {}

    /// Dispatch one input event. Returns the action the app loop
    /// should take as a side effect.
    EditorAction handle(EditorInput const& ev);

    /// Begin editing — resets tool state.
    void init();

    /// Helper to push an already-built command through `undo`.
    void push(std::unique_ptr<Command> cmd) { undo.push(std::move(cmd), map); }

    /// Render a single-line status bar describing the current tool,
    /// paint cell, layer, and undo count.
    std::string status_line() const;

    /// Render the full editor HUD as multi-line ASCII. Returned
    /// string is exactly `height` lines separated by '\n'. The HUD
    /// is intentionally low-tech — the design doc says the editor is
    /// REXPaint-like and ASCII is enough.
    std::string hud_text(int viewport_w, int viewport_h) const;

    /// Save current map state to `path` via the in-memory layer
    /// writer (see `editor/map_io.cpp`). Returns true on success.
    bool save_to(std::string const& path) const;

    /// Reload from disk (resets undo + clipboard).
    bool load_from(std::string const& path);
};

/// Stable ASCII labels for `Tool`. Used in the HUD.
char const* tool_name(Tool t) noexcept;
char        tool_hotkey(Tool t) noexcept;

}  // namespace editor
}  // namespace ash