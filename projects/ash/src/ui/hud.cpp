#include "ui/hud.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

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

void pad_to(render::Buffer& buf, int x, int y, int width, render::Color bg) {
    for (int i = 0; i < width; ++i) {
        put(buf, x + i, y, mk(uint32_t(' '), bg, bg));
    }
}

std::string format_clock(int minutes_of_day) {
    int m = minutes_of_day;
    if (m < 0) m = 0;
    int const day = 24 * 60;
    m = m % day;
    int hh = m / 60;
    int mm = m % 60;
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
    return std::string(buf);
}

}  // namespace

void HudOverlay::render(render::Buffer& buf, Rect viewport, ScreenContext const& ctx) const {
    if (viewport.w <= 0 || viewport.h <= 0) return;
    Theme t = Theme::derive(ctx.settings.high_contrast,
                            ctx.settings.no_color,
                            ctx.settings.palette_variant);

    int const top_row    = viewport.y;
    int const bot_row    = viewport.y + viewport.h - 1;

    int bar_row    = top_row;
    int bar_total  = std::max(8, (viewport.w - 24) / 3);
    int bar_label  = 2;
    int bar_width  = std::max(4, bar_total - bar_label - 7);
    int hp_x = viewport.x + 1;
    int vp_x = hp_x + bar_total + 1;
    int sp_x = vp_x + bar_total + 1;

    draw_string(buf, hp_x, bar_row, "HP", t.title_fg, t.panel_bg);
    Rect hp_bar{hp_x + bar_label, bar_row, bar_width, 1};
    draw_hbar(buf, hp_bar, ctx.world.hp, ctx.world.hp_max, t.hp_fg, t.bar_bg, t);
    char hpbuf[16];
    std::snprintf(hpbuf, sizeof(hpbuf), "%d/%d", ctx.world.hp, ctx.world.hp_max);
    draw_string(buf, hp_x + bar_label + bar_width + 1, bar_row, hpbuf, t.text_fg, t.panel_bg);

    draw_string(buf, vp_x, bar_row, "VP", t.title_fg, t.panel_bg);
    Rect vp_bar{vp_x + bar_label, bar_row, bar_width, 1};
    draw_hbar(buf, vp_bar, ctx.world.vp, ctx.world.vp_max, t.vp_fg, t.bar_bg, t);
    char vpbuf[16];
    std::snprintf(vpbuf, sizeof(vpbuf), "%d/%d", ctx.world.vp, ctx.world.vp_max);
    draw_string(buf, vp_x + bar_label + bar_width + 1, bar_row, vpbuf, t.text_fg, t.panel_bg);

    draw_string(buf, sp_x, bar_row, "SP", t.title_fg, t.panel_bg);
    Rect sp_bar{sp_x + bar_label, bar_row, bar_width, 1};
    draw_hbar(buf, sp_bar, ctx.world.sp, ctx.world.sp_max, t.sp_fg, t.bar_bg, t);
    char spbuf[16];
    std::snprintf(spbuf, sizeof(spbuf), "%d/%d", ctx.world.sp, ctx.world.sp_max);
    draw_string(buf, sp_x + bar_label + bar_width + 1, bar_row, spbuf, t.text_fg, t.panel_bg);

    std::string clock = format_clock(ctx.world.time_of_day_minutes);
    int clock_x = viewport.x + viewport.w - static_cast<int>(clock.size()) - 1;
    draw_string(buf, clock_x, top_row, clock, t.accent, t.panel_bg);

    std::string loc = ctx.world.location_name;
    if (static_cast<int>(loc.size()) > viewport.w - clock_x - 1) {
        loc.resize(static_cast<std::size_t>(std::max(0, viewport.w - clock_x - 1)));
    }
    draw_string(buf, clock_x, top_row + 1, loc, t.text_fg, t.panel_bg);

    int const minimap_w = 7;
    int const minimap_h = 3;
    int mm_x = viewport.x + 1;
    int mm_y = top_row + 2;
    if (mm_y + minimap_h < bot_row && viewport.w > minimap_w + 4) {
        for (int row = 0; row < minimap_h; ++row) {
            for (int col = 0; col < minimap_w; ++col) {
                uint32_t g = uint32_t('.');
                if (row == minimap_h / 2 && col == minimap_w / 2) g = uint32_t('@');
                put(buf, mm_x + col, mm_y + row, mk(g, t.accent, t.panel_bg));
            }
        }
        std::string label = "MAP";
        draw_string(buf, mm_x, mm_y - 1, label, t.disabled_fg, t.panel_bg);
    }

    pad_to(buf, viewport.x, bot_row, viewport.w, t.panel_bg);
    std::string hint = "[I]Inv [M]Map [J]Jrnl [C]Char [Sp]Spells [B]Bk [Esc]Menu";
    int hint_x = viewport.x + std::max(0, (viewport.w - static_cast<int>(hint.size())) / 2);
    if (hint_x + static_cast<int>(hint.size()) > viewport.x + viewport.w) {
        hint_x = viewport.x;
    }
    draw_string(buf, hint_x, bot_row, hint, t.disabled_fg, t.panel_bg);
}

}  // namespace ui
}  // namespace ash