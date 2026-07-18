#pragma once
/// Phase 11: Input events for UI screens.
#include <cstdint>

namespace ash {
namespace ui {

enum class Key : std::uint16_t {
    None      = 0,
    Esc       = 27,
    Enter     = 13,
    Tab       = 9,
    Backspace = 8,
    Space     = 32,
    Up        = 256,
    Down,
    Left,
    Right,
    Home,
    End,
    PageUp,
    PageDown,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
};

struct KeyEvent {
    Key         key{Key::None};
    char        ch{0};        /// printable ASCII if key is a glyph
    bool        ctrl{false};
    bool        shift{false};
    bool        alt{false};
};

enum class MouseButton : std::uint8_t { None = 0, Left, Right, Middle };

struct MouseEvent {
    MouseButton button{MouseButton::None};
    int         x{0};
    int         y{0};
    bool        pressed{false};   /// true=down, false=up
    bool        drag{false};      /// true when a drag is in progress
};

}  // namespace ui
}  // namespace ash
