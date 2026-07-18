#include "ui/journal_ui.hpp"

#include <algorithm>
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

void draw_border_color(render::Buffer& buf, Rect r, render::Color fg, render::Color bg) {
    if (r.w < 2 || r.h < 2) return;
    for (int x = r.x; x < r.x + r.w; ++x) {
        put(buf, x, r.y, mk(uint32_t('-'), fg, bg));
        put(buf, x, r.y + r.h - 1, mk(uint32_t('-'), fg, bg));
    }
    for (int y = r.y; y < r.y + r.h; ++y) {
        put(buf, r.x, y, mk(uint32_t('|'), fg, bg));
        put(buf, r.x + r.w - 1, y, mk(uint32_t('|'), fg, bg));
    }
    put(buf, r.x,             r.y,             mk(uint32_t('+'), fg, bg));
    put(buf, r.x + r.w - 1,   r.y,             mk(uint32_t('+'), fg, bg));
    put(buf, r.x,             r.y + r.h - 1,   mk(uint32_t('+'), fg, bg));
    put(buf, r.x + r.w - 1,   r.y + r.h - 1,   mk(uint32_t('+'), fg, bg));
}

char const* tab_label(int t) {
    switch (t) {
        case 0: return "Active";
        case 1: return "Completed";
        case 2: return "All";
    }
    return "?";
}

}  // namespace

JournalScreen::JournalScreen(std::vector<JournalEntry> entries)
    : entries_(std::move(entries)) {}

void JournalScreen::set_entries(std::vector<JournalEntry> entries) {
    entries_ = std::move(entries);
}

std::vector<JournalEntry const*> JournalScreen::filtered() const {
    std::vector<JournalEntry const*> out;
    for (auto const& e : entries_) {
        if (tab_ == 2) out.push_back(&e);
        else if (tab_ == 0 && !e.completed) out.push_back(&e);
        else if (tab_ == 1 && e.completed) out.push_back(&e);
    }
    return out;
}

void JournalScreen::render(render::Buffer& buf, Rect viewport) {
    Theme t = Theme::derive(/*high_contrast=*/false,
                            /*no_color=*/false,
                            /*palette_variant=*/"");

    int panel_w = std::min<int>(viewport.w - 2, 90);
    int panel_h = std::min<int>(viewport.h - 2, 26);
    int px = viewport.x + (viewport.w - panel_w) / 2;
    int py = viewport.y + (viewport.h - panel_h) / 2;
    Rect panel{px, py, panel_w, panel_h};
    fill_rect(buf, panel, t.panel_bg);
    draw_border(buf, panel, t);
    Rect title{panel.x, panel.y, panel.w, 1};
    draw_title(buf, title, "Journal", t);

    int tabs_y = panel.y + 2;
    int cur_x = panel.x + 2;
    for (int i = 0; i < 3; ++i) {
        std::string tab = std::string("[") + tab_label(i) + "]";
        auto fg = (i == tab_) ? t.selected_fg : t.text_fg;
        auto bg = (i == tab_) ? t.selected_bg : t.panel_bg;
        draw_string(buf, cur_x, tabs_y, tab, fg, bg);
        cur_x += static_cast<int>(tab.size()) + 2;
    }

    int list_x = panel.x + 2;
    int list_y = tabs_y + 2;
    int list_w = panel.w / 3;
    int list_h = panel.y + panel.h - list_y - 2;
    if (list_w < 16) list_w = 16;
    if (list_h < 4) list_h = 4;
    Rect list{list_x, list_y, list_w, list_h};
    draw_border_color(buf, list, t.panel_border, t.panel_bg);

    auto filt = filtered();
    std::vector<ListItem> items;
    items.reserve(filt.size());
    for (auto const* e : filt) {
        std::string mark = e->completed ? "[x] " : "[ ] ";
        items.push_back(ListItem{mark + e->title, "", true, nullptr});
    }
    if (selected_ >= static_cast<int>(items.size())) selected_ = 0;
    if (selected_ < 0) selected_ = 0;
    Rect inner{list.x + 1, list.y + 1, list.w - 2, list.h - 2};
    if (inner.h > 0) {
        int max_scroll = std::max(0, static_cast<int>(items.size()) - inner.h);
        if (scroll_ > max_scroll) scroll_ = max_scroll;
        if (selected_ < scroll_) scroll_ = selected_;
        if (selected_ >= scroll_ + inner.h) scroll_ = selected_ - inner.h + 1;
        draw_list(buf, inner, items, selected_, scroll_, t);
    }

    int desc_x = list.x + list.w + 2;
    int desc_y = list_y;
    int desc_w = panel.x + panel.w - desc_x - 2;
    int desc_h = list_h;
    Rect desc{desc_x, desc_y, desc_w, desc_h};
    draw_border_color(buf, desc, t.panel_border, t.panel_bg);
    if (!filt.empty() && selected_ < static_cast<int>(filt.size())) {
        JournalEntry const& e = *filt[static_cast<std::size_t>(selected_)];
        Rect inner_desc{desc.x + 1, desc.y + 1, desc.w - 2, 1};
        draw_string(buf, inner_desc.x, inner_desc.y, e.title, t.title_fg, t.panel_bg);
        Rect body{desc.x + 1, desc.y + 2, desc.w - 2, desc.h - 3};
        draw_text(buf, body, e.text, t);
    }

    std::string hint = "[1/2/3] tabs   Arrows select   Enter/Esc close";
    int hx = panel.x + (panel.w - static_cast<int>(hint.size())) / 2;
    draw_string(buf, hx, panel.y + panel.h - 1, hint, t.disabled_fg, t.panel_bg);

    [[maybe_unused]] int pulse = (anim_t_ms_ / 250) % 2;
    if (pulse && !filt.empty()) {
        int row_y = inner.y + (selected_ - scroll_);
        if (row_y >= inner.y && row_y < inner.y + inner.h) {
            put(buf, inner.x, row_y, mk(uint32_t('>'), t.accent, t.selected_bg));
        }
    }
}

ScreenResult JournalScreen::handle_key(KeyEvent const& ev, ScreenContext& /*ctx*/) {
    if (ev.key == Key::Esc) return ScreenResult::Pop;
    if (ev.ch == '1') { tab_ = 0; selected_ = 0; scroll_ = 0; return ScreenResult::Stay; }
    if (ev.ch == '2') { tab_ = 1; selected_ = 0; scroll_ = 0; return ScreenResult::Stay; }
    if (ev.ch == '3') { tab_ = 2; selected_ = 0; scroll_ = 0; return ScreenResult::Stay; }
    if (ev.key == Key::Tab) {
        tab_ = (tab_ + 1) % 3;
        selected_ = 0;
        scroll_ = 0;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Up) {
        auto filt = filtered();
        int n = static_cast<int>(filt.size());
        if (n > 0) selected_ = (selected_ - 1 + n) % n;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Down) {
        auto filt = filtered();
        int n = static_cast<int>(filt.size());
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