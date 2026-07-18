#include "ui/spell_ui.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>

#include "ui/widgets.hpp"

namespace ash {
namespace ui {

namespace {

void put(render::Buffer& buf, int x, int y, render::Cell c) {
    if (x >= 0 && y >= 0 && x < static_cast<int>(buf.width) && y < static_cast<int>(buf.height)) {
        buf.set(x, y, c);
    }
}

render::Cell mk(uint32_t g, render::Color fg, render::Color bg) {
    render::Cell c;
    c.glyph = g;
    c.fg_r = fg.r; c.fg_g = fg.g; c.fg_b = fg.b;
    c.bg_r = bg.r; c.bg_g = bg.g; c.bg_b = bg.b;
    return c;
}

void draw_string(render::Buffer& buf, int x, int y, std::string const& s,
                 render::Color fg, render::Color bg) {
    for (std::size_t i = 0; i < s.size(); ++i) {
        put(buf, x + static_cast<int>(i), y,
            mk(static_cast<uint32_t>(static_cast<unsigned char>(s[i])), fg, bg));
    }
}

void fill_rect(render::Buffer& buf, Rect r, render::Color bg) {
    for (int y = r.y; y < r.y + r.h; ++y) {
        for (int x = r.x; x < r.x + r.w; ++x) {
            put(buf, x, y, mk(uint32_t(' '), bg, bg));
        }
    }
}

}  // namespace

SpellScreen::SpellScreen(std::vector<SpellEntry> spells)
    : spells_(std::move(spells)) {}

void SpellScreen::set_spells(std::vector<SpellEntry> spells) {
    spells_ = std::move(spells);
    if (selected_ >= static_cast<int>(spells_.size())) selected_ = 0;
}

void SpellScreen::render(render::Buffer& buf, Rect viewport) {
    Theme t = Theme::derive(/*high_contrast=*/false,
                            /*no_color=*/false,
                            /*palette_variant=*/"");

    int panel_w = std::min<int>(viewport.w - 2, 60);
    int panel_h = std::min<int>(viewport.h - 2, 24);
    int px = viewport.x + (viewport.w - panel_w) / 2;
    int py = viewport.y + (viewport.h - panel_h) / 2;
    Rect panel{px, py, panel_w, panel_h};
    fill_rect(buf, panel, t.panel_bg);
    draw_border(buf, panel, t);
    Rect title{panel.x, panel.y, panel.w, 1};
    draw_title(buf, title, "Spells", t);

    std::vector<ListItem> items;
    items.reserve(spells_.size());
    for (auto const& s : spells_) {
        std::string label = "[" + s.school + "] " + s.name;
        char cost[16];
        std::snprintf(cost, sizeof(cost), "%d SP", s.cost_sp);
        items.push_back(ListItem{label, cost, true, nullptr});
    }
    if (selected_ >= static_cast<int>(items.size())) selected_ = 0;
    if (selected_ < 0) selected_ = 0;
    Rect inner{panel.x + 2, panel.y + 2, panel.w - 4, panel.h - 4};
    if (inner.h > 0) {
        int max_scroll = std::max(0, static_cast<int>(items.size()) - inner.h);
        if (scroll_ > max_scroll) scroll_ = max_scroll;
        if (selected_ < scroll_) scroll_ = selected_;
        if (selected_ >= scroll_ + inner.h) scroll_ = selected_ - inner.h + 1;
        draw_list(buf, inner, items, selected_, scroll_, t);
    }

    std::string hint = "Up/Down select   Enter cast   Esc close";
    int hx = panel.x + (panel.w - static_cast<int>(hint.size())) / 2;
    draw_string(buf, hx, panel.y + panel.h - 1, hint, t.disabled_fg, t.panel_bg);
}

ScreenResult SpellScreen::handle_key(KeyEvent const& ev, ScreenContext& /*ctx*/) {
    int n = static_cast<int>(spells_.size());
    if (ev.key == Key::Esc) return ScreenResult::Pop;
    if (ev.key == Key::Up) {
        if (n > 0) selected_ = (selected_ - 1 + n) % n;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Down) {
        if (n > 0) selected_ = (selected_ + 1) % n;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::PageUp) {
        if (scroll_ > 0) --scroll_;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::PageDown) {
        ++scroll_;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Enter || ev.key == Key::Space) {
        return ScreenResult::Pop;
    }
    return ScreenResult::Stay;
}

}  // namespace ui
}  // namespace ash