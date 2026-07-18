#include "ui/dialogue_ui.hpp"

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

}  // namespace

DialogueScreen::DialogueScreen(std::string npc_name,
                               std::string greeting,
                               std::vector<DialogueTopic> topics,
                               TopicCallback on_topic)
    : npc_name_(std::move(npc_name)),
      greeting_(std::move(greeting)),
      topics_(std::move(topics)),
      on_topic_(std::move(on_topic)) {}

void DialogueScreen::render(render::Buffer& buf, Rect viewport) {
    Theme t = Theme::derive(/*high_contrast=*/false,
                            /*no_color=*/false,
                            /*palette_variant=*/"");

    int const port_w = 8;
    int const port_h = 8;
    int panel_w = std::min<int>(viewport.w - 2, 80);
    int panel_h = std::min<int>(viewport.h - 2, std::max(port_h + 8, 22));
    if (panel_w < 30) panel_w = std::max(10, viewport.w);
    if (panel_h < 12) panel_h = std::max(8, viewport.h);
    int px = viewport.x + (viewport.w - panel_w) / 2;
    int py = viewport.y + (viewport.h - panel_h) / 2;
    Rect panel{px, py, panel_w, panel_h};
    fill_rect(buf, panel, t.panel_bg);
    draw_border(buf, panel, t);
    Rect title{panel.x, panel.y, panel.w, 1};
    draw_title(buf, title, "Dialogue", t);

    Rect port{panel.x + 2, panel.y + 2, port_w, port_h};
    fill_rect(buf, port, t.bar_bg);
    draw_border_color(buf, port, t.panel_border, t.panel_bg);
    char initial = npc_name_.empty() ? '?' : npc_name_[0];
    char buf2[2] = {initial, 0};
    int ix = port.x + (port.w - 1) / 2;
    int iy = port.y + (port.h - 1) / 2;
    draw_string(buf, ix, iy, buf2, t.title_fg, t.bar_bg);

    int greet_x = port.x + port.w + 2;
    int greet_y = port.y + 2;
    int greet_w = std::max(10, panel.x + panel.w - greet_x - 2);
    int greet_h = port_h - 2;
    Rect greet{greet_x, greet_y, greet_w, greet_h};
    draw_text(buf, greet, greeting_, t);

    draw_string(buf, greet_x, port.y, npc_name_, t.title_fg, t.panel_bg);

    int topics_y = panel.y + port_h + 3;
    int topics_h = panel.y + panel.h - topics_y - 2;
    if (topics_h < 3) topics_h = 3;
    Rect topics_rect{panel.x + 2, topics_y, panel.w - 4, topics_h};
    draw_border_color(buf, topics_rect, t.panel_border, t.panel_bg);

    std::vector<ListItem> items;
    items.reserve(topics_.size());
    for (auto const& tp : topics_) {
        items.push_back(ListItem{tp.text, "", true, nullptr});
    }
    if (selected_ >= static_cast<int>(items.size())) selected_ = 0;
    if (selected_ < 0) selected_ = 0;
    Rect inner{topics_rect.x + 1, topics_rect.y + 1, topics_rect.w - 2, topics_rect.h - 2};
    if (inner.h > 0) {
        int max_scroll = std::max(0, static_cast<int>(items.size()) - inner.h);
        if (scroll_ > max_scroll) scroll_ = max_scroll;
        if (selected_ < scroll_) scroll_ = selected_;
        if (selected_ >= scroll_ + inner.h) scroll_ = selected_ - inner.h + 1;
        draw_list(buf, inner, items, selected_, scroll_, t);
    }

    int foot_y = panel.y + panel.h - 1;
    std::string hint = "Up/Down select   Enter speak   Esc leave";
    int hx = panel.x + (panel.w - static_cast<int>(hint.size())) / 2;
    draw_string(buf, hx, foot_y, hint, t.disabled_fg, t.panel_bg);

    [[maybe_unused]] int pulse = (anim_t_ms_ / 250) % 2;
    if (pulse && !items.empty() && selected_ < static_cast<int>(items.size())) {
        int row_y = inner.y + (selected_ - scroll_);
        if (row_y >= inner.y && row_y < inner.y + inner.h) {
            put(buf, inner.x, row_y, mk(uint32_t('>'), t.accent, t.selected_bg));
        }
    }
}

ScreenResult DialogueScreen::handle_key(KeyEvent const& ev, ScreenContext& ctx) {
    if (ev.key == Key::Esc) {
        return ScreenResult::Pop;
    }
    if (topics_.empty()) {
        if (ev.key == Key::Enter || ev.key == Key::Space) return ScreenResult::Pop;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Up) {
        selected_ = (selected_ - 1 + static_cast<int>(topics_.size())) %
                    static_cast<int>(topics_.size());
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Down) {
        selected_ = (selected_ + 1) % static_cast<int>(topics_.size());
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
        int idx = selected_;
        if (topics_[static_cast<std::size_t>(idx)].on_select) {
            topics_[static_cast<std::size_t>(idx)].on_select(ctx);
        }
        if (on_topic_) on_topic_(idx, ctx);
        return ScreenResult::Pop;
    }
    return ScreenResult::Stay;
}

}  // namespace ui
}  // namespace ash