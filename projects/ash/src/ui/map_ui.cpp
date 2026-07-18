#include "ui/map_ui.hpp"

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

void draw_centered(render::Buffer& buf, int cx, int cy, std::string const& s,
                   render::Color fg, render::Color bg) {
    int x = cx - static_cast<int>(s.size()) / 2;
    draw_string(buf, x, cy, s, fg, bg);
}

}  // namespace

MapScreen::MapScreen(std::vector<Region> regions)
    : regions_(std::move(regions)) {}

void MapScreen::set_regions(std::vector<Region> regions) {
    regions_ = std::move(regions);
    if (selected_ >= static_cast<int>(regions_.size())) selected_ = 0;
}

void MapScreen::render(render::Buffer& buf, Rect viewport) {
    Theme t = Theme::derive(/*high_contrast=*/false,
                            /*no_color=*/false,
                            /*palette_variant=*/"");

    int panel_w = std::min<int>(viewport.w - 2, 70);
    int panel_h = std::min<int>(viewport.h - 2, 24);
    int px = viewport.x + (viewport.w - panel_w) / 2;
    int py = viewport.y + (viewport.h - panel_h) / 2;
    Rect panel{px, py, panel_w, panel_h};
    fill_rect(buf, panel, t.panel_bg);
    draw_border(buf, panel, t);
    Rect title{panel.x, panel.y, panel.w, 1};
    draw_title(buf, title, "Map", t);

    int grid_x = panel.x + 4;
    int grid_y = panel.y + 3;
    int cell_w = 14;
    int cell_h = 5;
    for (int gy = 0; gy < 3; ++gy) {
        for (int gx = 0; gx < 3; ++gx) {
            int idx = gy * 3 + gx;
            Rect cell{grid_x + gx * cell_w, grid_y + gy * cell_h, cell_w, cell_h};
            bool discovered = (idx < static_cast<int>(regions_.size()))
                              && regions_[static_cast<std::size_t>(idx)].discovered;
            bool sel = (idx == selected_) && idx < static_cast<int>(regions_.size());
            draw_border_color(buf, cell, sel ? t.accent : t.panel_border, t.panel_bg);
            if (idx < static_cast<int>(regions_.size())) {
                Region const& r = regions_[static_cast<std::size_t>(idx)];
                std::string label = discovered ? r.name : std::string("?");
                draw_centered(buf, cell.x + cell.w / 2, cell.y + cell.h / 2,
                              label, discovered ? t.title_fg : t.disabled_fg, t.panel_bg);
                if (discovered) {
                    std::string id = "[" + r.id + "]";
                    draw_centered(buf, cell.x + cell.w / 2, cell.y + cell.h / 2 + 1,
                                  id, t.disabled_fg, t.panel_bg);
                }
            } else {
                draw_centered(buf, cell.x + cell.w / 2, cell.y + cell.h / 2,
                              "--", t.disabled_fg, t.panel_bg);
            }
        }
    }

    int list_y = grid_y + 3 * cell_h + 1;
    int list_h = panel.y + panel.h - list_y - 2;
    if (list_h < 3) list_h = 3;
    Rect list{panel.x + 2, list_y, panel.w - 4, list_h};
    draw_border_color(buf, list, t.panel_border, t.panel_bg);

    std::vector<ListItem> items;
    items.reserve(regions_.size());
    for (auto const& r : regions_) {
        std::string detail = r.discovered ? "known" : "unknown";
        items.push_back(ListItem{r.discovered ? r.name : std::string("(undiscovered)"),
                                  detail, true, nullptr});
    }
    if (selected_ >= static_cast<int>(items.size())) selected_ = 0;
    Rect inner{list.x + 1, list.y + 1, list.w - 2, list.h - 2};
    if (inner.h > 0) {
        int max_scroll = std::max(0, static_cast<int>(items.size()) - inner.h);
        if (scroll_ > max_scroll) scroll_ = max_scroll;
        if (selected_ < scroll_) scroll_ = selected_;
        if (selected_ >= scroll_ + inner.h) scroll_ = selected_ - inner.h + 1;
        draw_list(buf, inner, items, selected_, scroll_, t);
    }

    std::string hint = "Arrows select   Enter close   Esc close";
    int hx = panel.x + (panel.w - static_cast<int>(hint.size())) / 2;
    draw_string(buf, hx, panel.y + panel.h - 1, hint, t.disabled_fg, t.panel_bg);
}

ScreenResult MapScreen::handle_key(KeyEvent const& ev, ScreenContext& /*ctx*/) {
    int n = static_cast<int>(regions_.size());
    if (ev.key == Key::Esc) return ScreenResult::Pop;
    if (ev.key == Key::Up || ev.key == Key::Left) {
        if (n > 0) {
            selected_ = (selected_ - 1 + n) % n;
            scroll_ = selected_;
        }
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Down || ev.key == Key::Right) {
        if (n > 0) {
            selected_ = (selected_ + 1) % n;
            scroll_ = selected_;
        }
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