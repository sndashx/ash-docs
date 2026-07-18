#pragma once
/// Phase 11: Mode stack (Phase 5 spec).
///
/// A LIFO stack of `Screen` pointers. The topmost screen receives all
/// input. Screens push child screens (modal sub-dialogs) which also
/// receive input until popped.
///
/// The mode enum below mirrors the spec names so call sites can refer
/// to the conceptual "menu" / "game" / "inventory" mode, while the
/// stack itself is just a vector.
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

enum class Mode : std::uint8_t {
    None,
    Menu,
    Game,
    Inventory,
    Map,
    Journal,
    Character,
    Dialogue,
    Spells,
    Book,
    Barter,
    Settings,
    Save,
    Load,
    Death,
};

std::string const& mode_name(Mode m) noexcept;

class ModeStack {
public:
    ModeStack() = default;

    /// Push a new screen. The screen becomes topmost and receives input.
    void push(std::unique_ptr<Screen> s, Mode mode, ScreenContext& ctx);

    /// Pop the topmost screen. Returns false if the stack was empty.
    bool pop(ScreenContext& ctx);

    /// Pop everything down to (and including) the named mode. No-op if
    /// the named mode is not on the stack.
    void pop_to(Mode mode, ScreenContext& ctx);

    /// Replace the entire stack with a single screen.
    void reset(std::unique_ptr<Screen> s, Mode mode, ScreenContext& ctx);

    /// True if there are no screens on the stack.
    bool empty() const noexcept { return stack_.empty(); }

    /// Current top mode (Mode::None if empty).
    Mode top_mode() const noexcept;

    /// Title of the topmost screen (or empty if empty).
    std::string top_title() const noexcept;

    /// Update all screens (top-down, so children advance first).
    void update(int dt_ms);

    /// Render top-down; only the topmost screen draws (others are dimmed
    /// background). The screen's render() takes a viewport rect.
    void render(render::Buffer& buf, Rect viewport);

    /// Dispatch a key event to the topmost screen. Honors the result.
    ScreenResult dispatch_key(KeyEvent const& ev, ScreenContext& ctx);

    /// Dispatch a mouse event to the topmost screen.
    ScreenResult dispatch_mouse(MouseEvent const& ev, ScreenContext& ctx);

    /// Number of screens on the stack.
    std::size_t size() const noexcept { return stack_.size(); }

    /// Raw access for debug / overlay.
    std::vector<std::pair<Mode, std::string>> entries() const;

private:
    struct Entry {
        Mode                  mode{Mode::None};
        std::unique_ptr<Screen> screen;
    };
    std::vector<Entry> stack_;

    void apply_pop(ScreenContext& ctx);
};

}  // namespace ui
}  // namespace ash