#pragma once
/// Phase 11: UI mode stack + Screen interface (per 16-phase-11-polish).
///
/// Screens push/pop onto the stack. Input dispatches to the top screen.
/// Each screen renders into a render::Buffer provided by the application.
///
/// The Screen interface is intentionally small: `update` advances state,
/// `render` draws into a Buffer, and `handle_key` / `handle_mouse` consume
/// input. Screens return a `Result` enum so the App can decide whether to
/// pop them, push a child, or stay.
#include <cstdint>
#include <string>
#include <vector>

#include "render/buffer.hpp"
#include "render/cell.hpp"
#include "settings/settings.hpp"
#include "ui/input_event.hpp"
#include "ui/screen_context.hpp"

namespace ash {
namespace ui {

enum class ScreenResult {
    Stay,        /// screen still active, keep rendering
    Pop,         /// pop this screen off the stack
    PushChild,   /// a child was pushed, render order will handle it
    Quit,        /// user requested application quit
};

/// A rect in cell coordinates. Screens use this for layout.
struct Rect {
    int x{0}, y{0}, w{0}, h{0};
    constexpr Rect() = default;
    constexpr Rect(int x_, int y_, int w_, int h_) : x{x_}, y{y_}, w{w_}, h{h_} {}

    constexpr bool contains(int px, int py) const noexcept {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

class Screen {
public:
    virtual ~Screen() = default;

    /// One-time init after push. May load data, build widgets.
    virtual void on_push(ScreenContext& /*ctx*/) {}

    /// One-shot teardown before pop. Save any pending state.
    virtual void on_pop() {}

    /// Advance simulation/animation by `dt_ms`.
    virtual void update(int /*dt_ms*/) {}

    /// Render the screen into `buf`. `viewport` is the screen's bounding
    /// rect; screens may use the full window or a sub-rect.
    virtual void render(render::Buffer& buf, Rect viewport) = 0;

    /// Consume a key event. Default: ESC pops the screen.
    virtual ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx);

    /// Consume a mouse event (in absolute cell coordinates). Default: ignored.
    virtual ScreenResult handle_mouse(MouseEvent const& ev, ScreenContext& ctx);

    /// Human-readable title for debug overlay / status line.
    virtual std::string title() const = 0;
};

}  // namespace ui
}  // namespace ash
