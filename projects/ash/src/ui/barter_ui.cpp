#include "ui/barter_ui.hpp"

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

void draw_panel(render::Buffer& buf, Rect r, std::string const& title_text,
                std::vector<TradeItem> const& items, int cursor,
                Theme const& t, bool active) {
    fill_rect(buf, r, t.panel_bg);
    draw_border_color(buf, r, active ? t.accent : t.panel_border, t.panel_bg);
    draw_string(buf, r.x + 2, r.y, title_text,
                active ? t.title_fg : t.disabled_fg, t.panel_bg);
    int rows = r.h - 2;
    int inner_x = r.x + 1;
    int inner_y = r.y + 2;
    int inner_w = r.w - 2;
    for (int row = 0; row < rows; ++row) {
        int idx = row;
        if (idx >= static_cast<int>(items.size())) break;
        bool sel = active && (cursor == idx);
        bool chosen = items[static_cast<std::size_t>(idx)].selected;
        auto fg = sel ? t.selected_fg : (chosen ? t.sp_fg : t.text_fg);
        auto bg = sel ? t.selected_bg : t.panel_bg;
        TradeItem const& it = items[static_cast<std::size_t>(idx)];
        std::string line;
        line.reserve(it.name.size() + 8);
        line.push_back(chosen ? '*' : ' ');
        line.push_back(' ');
        line += it.name;
        char cost[12];
        std::snprintf(cost, sizeof(cost), " %d", it.value);
        line += cost;
        if (static_cast<int>(line.size()) > inner_w) {
            line.resize(static_cast<std::size_t>(inner_w));
        }
        int y = inner_y + row;
        for (int x = 0; x < inner_w; ++x) {
            uint32_t g = uint32_t(' ');
            if (x < static_cast<int>(line.size())) {
                g = static_cast<uint32_t>(static_cast<unsigned char>(line[static_cast<std::size_t>(x)]));
            }
            put(buf, inner_x + x, y, mk(g, fg, bg));
        }
    }
}

}  // namespace

BarterScreen::BarterScreen(std::vector<TradeItem> player_items,
                           std::vector<TradeItem> merchant_items)
    : player_(std::move(player_items)),
      merchant_(std::move(merchant_items)) {}

void BarterScreen::set_player(std::vector<TradeItem> items) {
    player_ = std::move(items);
    if (player_cursor_ >= static_cast<int>(player_.size())) player_cursor_ = 0;
}

void BarterScreen::set_merchant(std::vector<TradeItem> items) {
    merchant_ = std::move(items);
    if (merchant_cursor_ >= static_cast<int>(merchant_.size())) merchant_cursor_ = 0;
}

int BarterScreen::totals(int& player_total, int& merchant_total) const {
    player_total = 0;
    merchant_total = 0;
    for (auto const& it : player_)   if (it.selected) player_total += it.value;
    for (auto const& it : merchant_) if (it.selected) merchant_total += it.value;
    return player_total - merchant_total;
}

void BarterScreen::render(render::Buffer& buf, Rect viewport) {
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
    draw_title(buf, title, "Barter", t);

    int gap = 1;
    int side_w = (panel.w - 4 - gap) / 2;
    int top = panel.y + 2;
    int bot = panel.y + panel.h - 3;
    int side_h = bot - top;
    if (side_h < 5) side_h = 5;
    if (side_w < 14) side_w = 14;

    Rect left{panel.x + 2, top, side_w, side_h};
    Rect right{panel.x + 2 + side_w + gap, top, side_w, side_h};
    draw_panel(buf, left, "You", player_, player_cursor_, t, active_panel_ == 0);
    draw_panel(buf, right, "Merchant", merchant_, merchant_cursor_, t, active_panel_ == 1);

    int pt = 0;
    int mt = 0;
    totals(pt, mt);
    char foot[96];
    std::snprintf(foot, sizeof(foot),
                  "Your total: %dgp   Their total: %dgp   Net: %+d",
                  pt, mt, pt - mt);
    int fx = panel.x + (panel.w - static_cast<int>(std::char_traits<char>::length(foot))) / 2;
    draw_string(buf, fx, panel.y + panel.h - 2, foot, t.title_fg, t.panel_bg);

    std::string hint = "Tab switch   Up/Down pick   Enter offer   Esc leave";
    int hx = panel.x + (panel.w - static_cast<int>(hint.size())) / 2;
    draw_string(buf, hx, panel.y + panel.h - 1, hint, t.disabled_fg, t.panel_bg);
}

ScreenResult BarterScreen::handle_key(KeyEvent const& ev, ScreenContext& /*ctx*/) {
    if (ev.key == Key::Esc) return ScreenResult::Pop;
    if (ev.key == Key::Tab) {
        active_panel_ = (active_panel_ + 1) % 2;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Left)  { active_panel_ = 0; return ScreenResult::Stay; }
    if (ev.key == Key::Right) { active_panel_ = 1; return ScreenResult::Stay; }

    if (active_panel_ == 0) {
        int n = static_cast<int>(player_.size());
        if (ev.key == Key::Up && n > 0)
            player_cursor_ = (player_cursor_ - 1 + n) % n;
        else if (ev.key == Key::Down && n > 0)
            player_cursor_ = (player_cursor_ + 1) % n;
        else if (ev.key == Key::Enter || ev.key == Key::Space) {
            if (n > 0)
                player_[static_cast<std::size_t>(player_cursor_)].selected =
                    !player_[static_cast<std::size_t>(player_cursor_)].selected;
        } else return ScreenResult::Stay;
        return ScreenResult::Stay;
    } else {
        int n = static_cast<int>(merchant_.size());
        if (ev.key == Key::Up && n > 0)
            merchant_cursor_ = (merchant_cursor_ - 1 + n) % n;
        else if (ev.key == Key::Down && n > 0)
            merchant_cursor_ = (merchant_cursor_ + 1) % n;
        else if (ev.key == Key::Enter || ev.key == Key::Space) {
            if (n > 0)
                merchant_[static_cast<std::size_t>(merchant_cursor_)].selected =
                    !merchant_[static_cast<std::size_t>(merchant_cursor_)].selected;
        } else return ScreenResult::Stay;
        return ScreenResult::Stay;
    }
}

}  // namespace ui
}  // namespace ash