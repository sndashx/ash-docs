#include "ui/menu.hpp"

#include <algorithm>
#include <utility>

namespace ash {
namespace ui {

void MainMenu::rebuild_items() {
    items_.clear();
    items_.push_back(ListItem{"New Game",  "", true, nullptr});
    items_.push_back(ListItem{"Continue",  "", true, nullptr});
    items_.push_back(ListItem{"Load",      "", true, nullptr});
    items_.push_back(ListItem{"Settings",  "", true, nullptr});
    items_.push_back(ListItem{"Quit",      "", true, nullptr});
}

void MainMenu::render(render::Buffer& buf, Rect viewport) {
    theme_ = Theme::derive(/*high_contrast=*/false,
                           /*no_color=*/false,
                           /*palette_variant=*/"");
    if (items_.empty()) rebuild_items();

    int panel_w = std::min<int>(viewport.w - 4, 40);
    int panel_h = static_cast<int>(items_.size()) + 4;
    int px = viewport.x + (viewport.w - panel_w) / 2;
    int py = viewport.y + (viewport.h - panel_h) / 2;
    Rect panel{px, py, panel_w, panel_h};
    draw_border(buf, panel, theme_);
    Rect title{panel.x, panel.y, panel.w, 1};
    draw_title(buf, title, "ASH", theme_);
    Rect list{panel.x + 2, panel.y + 2, panel.w - 4, panel.h - 4};
    draw_list(buf, list, items_, selected_, 0, theme_);

    auto fg = theme_.accent;
    auto bg = theme_.text_bg;
    std::string hint = "Up/Down select   Enter confirm   Esc quit";
    int hx = viewport.x + (viewport.w - static_cast<int>(hint.size())) / 2;
    int hy = viewport.y + viewport.h - 1;
    if (hx >= 0 && hy >= 0 && hx + static_cast<int>(hint.size()) <= viewport.x + viewport.w) {
        for (std::size_t i = 0; i < hint.size(); ++i) {
            render::Cell c;
            c.glyph = static_cast<uint32_t>(static_cast<unsigned char>(hint[i]));
            c.fg_r = fg.r; c.fg_g = fg.g; c.fg_b = fg.b;
            c.bg_r = bg.r; c.bg_g = bg.g; c.bg_b = bg.b;
            buf.set(hx + static_cast<int>(i), hy, c);
        }
    }
}

ScreenResult MainMenu::handle_key(KeyEvent const& ev, ScreenContext& ctx) {
    if (items_.empty()) rebuild_items();
    if (ev.key == Key::Up) {
        selected_ = (selected_ - 1 + static_cast<int>(items_.size())) %
                    static_cast<int>(items_.size());
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Down) {
        selected_ = (selected_ + 1) % static_cast<int>(items_.size());
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Enter || ev.key == Key::Space) {
        MenuAction a = MenuAction::None;
        switch (selected_) {
            case 0: a = MenuAction::NewGame;  break;
            case 1: a = MenuAction::Continue; break;
            case 2: a = MenuAction::Load;     break;
            case 3: a = MenuAction::Settings; break;
            case 4: a = MenuAction::Quit;     break;
        }
        if (on_action_) on_action_(a);
        if (a == MenuAction::Quit) {
            ctx.quit_requested = true;
            return ScreenResult::Quit;
        }
        return ScreenResult::Pop;
    }
    if (ev.key == Key::Esc) {
        if (on_action_) on_action_(MenuAction::Quit);
        ctx.quit_requested = true;
        return ScreenResult::Quit;
    }
    if (ev.ch == 'n' || ev.ch == 'N') { selected_ = 0; return handle_key(KeyEvent{Key::Enter, 0, false, false, false}, ctx); }
    if (ev.ch == 'c' || ev.ch == 'C') { selected_ = 1; return handle_key(KeyEvent{Key::Enter, 0, false, false, false}, ctx); }
    if (ev.ch == 'l' || ev.ch == 'L') { selected_ = 2; return handle_key(KeyEvent{Key::Enter, 0, false, false, false}, ctx); }
    if (ev.ch == 's' || ev.ch == 'S') { selected_ = 3; return handle_key(KeyEvent{Key::Enter, 0, false, false, false}, ctx); }
    if (ev.ch == 'q' || ev.ch == 'Q') { selected_ = 4; return handle_key(KeyEvent{Key::Enter, 0, false, false, false}, ctx); }
    return ScreenResult::Stay;
}

}  // namespace ui
}  // namespace ash
