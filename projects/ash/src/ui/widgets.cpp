#include "ui/widgets.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

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

}  // namespace

Theme Theme::derive(bool high_contrast, bool no_color,
                    std::string const& palette_variant) {
    Theme t;
    if (no_color) {
        // Luminance-only theme. Foreground = white-ish, background = dark.
        t.text_fg = {230, 230, 230};
        t.text_bg = {0, 0, 0};
        t.panel_fg = {230, 230, 230};
        t.panel_bg = {0, 0, 0};
        t.panel_border = {160, 160, 160};
        t.title_fg = {255, 255, 255};
        t.accent = {255, 255, 255};
        t.selected_fg = {0, 0, 0};
        t.selected_bg = {255, 255, 255};
        t.disabled_fg = {120, 120, 120};
        t.hp_fg = {255, 255, 255};
        t.vp_fg = {200, 200, 200};
        t.sp_fg = {140, 140, 140};
        t.bar_bg = {60, 60, 60};
        return t;
    }
    if (high_contrast) {
        t.text_fg = {255, 255, 255};
        t.text_bg = {0, 0, 0};
        t.panel_fg = {255, 255, 255};
        t.panel_bg = {0, 0, 0};
        t.panel_border = {255, 255, 0};
        t.title_fg = {255, 255, 0};
        t.accent = {0, 255, 255};
        t.selected_fg = {0, 0, 0};
        t.selected_bg = {255, 255, 0};
        t.disabled_fg = {128, 128, 128};
        t.hp_fg = {255, 80, 80};
        t.vp_fg = {80, 200, 255};
        t.sp_fg = {220, 140, 255};
        t.bar_bg = {40, 40, 40};
        return t;
    }
    if (palette_variant == "deuteranopia") {
        // Deutan: shift greens toward blue/cyan.
        t.hp_fg = {255, 80, 80};
        t.vp_fg = {80, 180, 255};   // water-blue instead of green
        t.sp_fg = {200, 130, 255};
        t.accent = {120, 200, 255};
        return t;
    }
    if (palette_variant == "protanopia") {
        // Protan: shift reds toward yellow/olive.
        t.hp_fg = {240, 180, 60};
        t.vp_fg = {80, 180, 220};
        t.sp_fg = {200, 130, 255};
        t.accent = {120, 220, 200};
        return t;
    }
    if (palette_variant == "tritanopia") {
        // Tritan: shift blues toward red/cyan.
        t.hp_fg = {220, 60, 100};
        t.vp_fg = {120, 220, 180};
        t.sp_fg = {200, 100, 240};
        t.accent = {120, 220, 255};
        return t;
    }
    return t;  // defaults
}

void draw_border(render::Buffer& buf, Rect r, Theme const& t) {
    if (r.w < 2 || r.h < 2) return;
    auto bg = t.panel_bg;
    auto fg = t.panel_border;
    // Top + bottom.
    for (int x = r.x; x < r.x + r.w; ++x) {
        put(buf, x, r.y, mk(uint32_t('-'), fg, bg));
        put(buf, x, r.y + r.h - 1, mk(uint32_t('-'), fg, bg));
    }
    // Left + right.
    for (int y = r.y; y < r.y + r.h; ++y) {
        put(buf, r.x, y, mk(uint32_t('|'), fg, bg));
        put(buf, r.x + r.w - 1, y, mk(uint32_t('|'), fg, bg));
    }
    // Corners.
    put(buf, r.x,             r.y,             mk(uint32_t('+'), fg, bg));
    put(buf, r.x + r.w - 1,   r.y,             mk(uint32_t('+'), fg, bg));
    put(buf, r.x,             r.y + r.h - 1,   mk(uint32_t('+'), fg, bg));
    put(buf, r.x + r.w - 1,   r.y + r.h - 1,   mk(uint32_t('+'), fg, bg));
}

void draw_title(render::Buffer& buf, Rect r, std::string const& text, Theme const& t) {
    if (r.w <= 0 || r.h <= 0) return;
    auto bg = t.panel_bg;
    auto fg = t.title_fg;
    int available = r.w - 4;
    if (available <= 0) return;
    std::string s = text;
    if (static_cast<int>(s.size()) > available) {
        s.resize(static_cast<std::size_t>(available));
    }
    auto s_len = static_cast<int>(s.size());
    int pad = std::max(0, (r.w - s_len) / 2);
    for (int x = 0; x < r.w; ++x) {
        if (x >= pad && x < pad + s_len) {
            std::size_t idx = static_cast<std::size_t>(x - pad);
            put(buf, r.x + x, r.y, mk(uint32_t(static_cast<unsigned char>(s[idx])), fg, bg));
        } else {
            put(buf, r.x + x, r.y, mk(uint32_t(' '), fg, bg));
        }
    }
}

void draw_hbar(render::Buffer& buf, Rect r, int value, int max,
               render::Color fg, render::Color bg, Theme const& t) {
    if (r.w <= 0 || r.h <= 0) return;
    int filled = 0;
    if (max > 0) {
        filled = (std::clamp(value, 0, max) * r.w) / max;
    }
    for (int x = 0; x < r.w; ++x) {
        for (int y = 0; y < r.h; ++y) {
            if (x < filled) {
                put(buf, r.x + x, r.y + y, mk(uint32_t('#'), fg, bg));
            } else {
                put(buf, r.x + x, r.y + y, mk(uint32_t('.'), t.bar_bg, t.bar_bg));
            }
        }
    }
}

Rect draw_list(render::Buffer& buf, Rect r,
               std::vector<ListItem> const& items,
               int selected, int scroll, Theme const& t) {
    Rect result{};
    if (r.w <= 0 || r.h <= 0) return result;
    int const row_h = 1;
    int const rows = r.h / row_h;
    for (int row = 0; row < rows; ++row) {
        int idx_int = scroll + row;
        if (idx_int < 0) continue;
        std::size_t idx = static_cast<std::size_t>(idx_int);
        if (idx >= items.size()) continue;
        bool is_sel = (idx_int == selected);
        auto fg = is_sel ? t.selected_fg : (items[idx].enabled ? t.text_fg : t.disabled_fg);
        auto bg = is_sel ? t.selected_bg : t.panel_bg;
        int y = r.y + row * row_h;
        std::string label = items[idx].label;
        auto label_len = static_cast<int>(label.size());
        if (label_len > r.w) {
            label.resize(static_cast<std::size_t>(r.w));
            label_len = r.w;
        }
        std::string detail = items[idx].detail;
        int detail_col = -1;
        if (!detail.empty()) {
            auto detail_len = static_cast<int>(detail.size());
            if (detail_len > r.w / 2) {
                detail.resize(static_cast<std::size_t>(r.w / 2));
                detail_len = r.w / 2;
            }
            detail_col = r.w - detail_len;
            if (detail_col < label_len + 2) detail_col = -1;
        }
        for (int x = 0; x < r.w; ++x) {
            uint32_t g = uint32_t(' ');
            if (x < label_len) {
                std::size_t ci = static_cast<std::size_t>(x);
                g = uint32_t(static_cast<unsigned char>(label[ci]));
            } else if (detail_col >= 0 && x >= detail_col) {
                std::size_t ci = static_cast<std::size_t>(x - detail_col);
                g = uint32_t(static_cast<unsigned char>(detail[ci]));
            }
            put(buf, r.x + x, y, mk(g, fg, bg));
        }
        if (is_sel) {
            result = Rect{r.x, y, r.w, row_h};
        }
    }
    return result;
}

int draw_text(render::Buffer& buf, Rect r, std::string const& text, Theme const& t) {
    if (r.w <= 0 || r.h <= 0) return 0;
    auto lines = word_wrap(text, r.w);
    int n = std::min<int>(static_cast<int>(lines.size()), r.h);
    auto fg = t.text_fg;
    auto bg = t.panel_bg;
    for (int row = 0; row < n; ++row) {
        auto const& ln = lines[static_cast<std::size_t>(row)];
        auto ln_len = static_cast<int>(ln.size());
        for (int x = 0; x < r.w; ++x) {
            uint32_t g = uint32_t(' ');
            if (x < ln_len) {
                std::size_t ci = static_cast<std::size_t>(x);
                g = uint32_t(static_cast<unsigned char>(ln[ci]));
            }
            put(buf, r.x + x, r.y + row, mk(g, fg, bg));
        }
    }
    return n;
}

std::vector<std::string> word_wrap(std::string const& text, int width) {
    std::vector<std::string> out;
    if (width <= 0) {
        out.push_back(text);
        return out;
    }
    std::istringstream iss(text);
    std::string line;
    std::string word;
    while (iss >> word) {
        if (line.empty()) {
            line = word;
            continue;
        }
        if (static_cast<int>(line.size()) + 1 + static_cast<int>(word.size()) <= width) {
            line.push_back(' ');
            line += word;
        } else {
            out.push_back(line);
            line = word;
        }
    }
    if (!line.empty()) out.push_back(line);
    return out;
}

}  // namespace ui
}  // namespace ash