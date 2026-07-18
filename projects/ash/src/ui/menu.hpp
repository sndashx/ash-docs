#pragma once
/// Phase 11: Main menu screen (per 16-phase-11-polish 11.00.006).
///
/// New Game / Continue / Load / Settings / Quit.
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ui/screen.hpp"
#include "ui/widgets.hpp"

namespace ash {
namespace ui {

enum class MenuAction : std::uint8_t {
    None,
    NewGame,
    Continue,
    Load,
    Settings,
    Quit,
};

class MainMenu : public Screen {
public:
    using ActionCallback = std::function<void(MenuAction)>;

    explicit MainMenu(ActionCallback cb) : on_action_(std::move(cb)) {}

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return "main menu"; }

private:
    ActionCallback on_action_;
    std::vector<ListItem> items_;
    int selected_{0};
    Theme theme_;

    void rebuild_items();
};

}  // namespace ui
}  // namespace ash