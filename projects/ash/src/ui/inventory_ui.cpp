#include "ui/inventory_ui.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>

#include "ui/widgets.hpp"

namespace ash {
namespace ui {

namespace {

char const* category_name(InventoryCategory c) {
    switch (c) {
        case InventoryCategory::Weapon:  return "Weapon";
        case InventoryCategory::Apparel: return "Apparel";
        case InventoryCategory::Misc:    return "Misc";
        case InventoryCategory::Count:   break;
    }
    return "?";
}

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

InventoryScreen::InventoryScreen(std::vector<InventoryItem> items)
    : items_(std::move(items)) {
    normalize_cursors();
}

void InventoryScreen::set_items(std::vector<InventoryItem> items) {
    items_ = std::move(items);
    normalize_cursors();
}

int InventoryScreen::col_size(int col) const {
    int n = 0;
    for (auto const& it : items_) {
        if (static_cast<int>(it.category) == col) ++n;
    }
    return n;
}

int InventoryScreen::col_offset(int col) const {
    int n = 0;
    for (int c = 0; c < col; ++c) n += col_size(c);
    return n;
}

void InventoryScreen::normalize_cursors() {
    for (int c = 0; c < 3; ++c) {
        int sz = col_size(c);
        if (sz <= 0) col_cursor_[c] = 0;
        else if (col_cursor_[c] < 0) col_cursor_[c] = 0;
        else if (col_cursor_[c] >= sz) col_cursor_[c] = sz - 1;
    }
    if (active_col_ < 0) active_col_ = 0;
    if (active_col_ > 2) active_col_ = 2;
}

void InventoryScreen::render(render::Buffer& buf, Rect viewport) {
    Theme t = Theme::derive(/*high_contrast=*/false,
                            /*no_color=*/false,
                            /*palette_variant=*/"");

    int panel_w = std::min<int>(viewport.w - 2, 90);
    int panel_h = std::min<int>(viewport.h - 2, 24);
    int px = viewport.x + (viewport.w - panel_w) / 2;
    int py = viewport.y + (viewport.h - panel_h) / 2;
    Rect panel{px, py, panel_w, panel_h};
    fill_rect(buf, panel, t.panel_bg);
    draw_border(buf, panel, t);
    Rect title{panel.x, panel.y, panel.w, 1};
    draw_title(buf, title, "Inventory", t);

    int gap = 1;
    int col_w = (panel.w - 4 - gap * 2) / 3;
    if (col_w < 8) col_w = 8;
    int top = panel.y + 2;
    int bot = panel.y + panel.h - 3;
    int col_h = bot - top;
    if (col_h < 4) col_h = 4;

    for (int c = 0; c < 3; ++c) {
        int cx = panel.x + 2 + c * (col_w + gap);
        Rect col{cx, top, col_w, col_h};
        bool active = (c == active_col_);
        draw_border_color(buf, col, active ? t.accent : t.panel_border, t.panel_bg);
        std::string header = std::string("[") + category_name(static_cast<InventoryCategory>(c)) + "]";
        int hx = col.x + (col.w - static_cast<int>(header.size())) / 2;
        draw_string(buf, hx, col.y + 1, header,
                    active ? t.title_fg : t.disabled_fg, t.panel_bg);

        int n = col_size(c);
        int base = col_offset(c);
        int rows = col.h - 3;
        for (int row = 0; row < rows; ++row) {
            int idx = base + row;
            if (idx >= base + n) break;
            InventoryItem const& it = items_[static_cast<std::size_t>(idx)];
            bool sel = active && (col_cursor_[c] == row);
            auto fg = sel ? t.selected_fg : t.text_fg;
            auto bg = sel ? t.selected_bg : t.panel_bg;
            int y = col.y + 2 + row;
            std::string line;
            line.reserve(it.label.size() + 6);
            if (it.equipped) line.push_back('*');
            else line.push_back(' ');
            line.push_back(' ');
            line += it.label;
            if (it.count > 1) {
                char nbuf[16];
                std::snprintf(nbuf, sizeof(nbuf), " x%d", it.count);
                line += nbuf;
            }
            if (static_cast<int>(line.size()) > col.w - 2) {
                line.resize(static_cast<std::size_t>(col.w - 2));
            }
            for (int x = 0; x < col.w - 2; ++x) {
                uint32_t g = uint32_t(' ');
                if (x < static_cast<int>(line.size())) {
                    g = static_cast<uint32_t>(static_cast<unsigned char>(line[static_cast<std::size_t>(x)]));
                }
                put(buf, col.x + 1 + x, y, mk(g, fg, bg));
            }
        }
        if (n == 0) {
            std::string empty = "(empty)";
            int ex = col.x + (col.w - static_cast<int>(empty.size())) / 2;
            draw_string(buf, ex, col.y + 3, empty, t.disabled_fg, t.panel_bg);
        }
    }

    std::string hint = "[Tab/<- ->] column   Up/Down item   Enter use   Esc close";
    int hx = panel.x + (panel.w - static_cast<int>(hint.size())) / 2;
    draw_string(buf, hx, panel.y + panel.h - 1, hint, t.disabled_fg, t.panel_bg);
}

ScreenResult InventoryScreen::handle_key(KeyEvent const& ev, ScreenContext& /*ctx*/) {
    if (ev.key == Key::Esc) return ScreenResult::Pop;
    if (ev.key == Key::Left) {
        active_col_ = (active_col_ + 2) % 3;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Right || ev.key == Key::Tab) {
        active_col_ = (active_col_ + 1) % 3;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Up) {
        int n = col_size(active_col_);
        if (n > 0) col_cursor_[active_col_] = (col_cursor_[active_col_] - 1 + n) % n;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Down) {
        int n = col_size(active_col_);
        if (n > 0) col_cursor_[active_col_] = (col_cursor_[active_col_] + 1) % n;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Enter || ev.key == Key::Space) {
        return ScreenResult::Pop;
    }
    return ScreenResult::Stay;
}

}  // namespace ui
}  // namespace ash