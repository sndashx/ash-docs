#include "ui/character_ui.hpp"

#include <algorithm>
#include <cstdio>
#include <numeric>
#include <utility>

#include "ui/widgets.hpp"

namespace ash {
namespace ui {

namespace {

char const* const ATTR_NAMES[9] = {
    "Str", "End", "Agi", "Wil",
    "Int", "Wit", "Cha", "Luc", "Voi",
};

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

CharacterScreen::CharacterScreen(std::string player_name,
                                 std::string class_hint,
                                 Attributes attrs,
                                 Skills skills,
                                 DerivedStats derived)
    : player_name_(std::move(player_name)),
      class_hint_(std::move(class_hint)),
      attrs_(attrs),
      skills_(skills),
      derived_(derived) {}

void CharacterScreen::render(render::Buffer& buf, Rect viewport) {
    Theme t = Theme::derive(/*high_contrast=*/false,
                            /*no_color=*/false,
                            /*palette_variant=*/"");

    int panel_w = std::min<int>(viewport.w - 2, 80);
    int panel_h = std::min<int>(viewport.h - 2, 28);
    int px = viewport.x + (viewport.w - panel_w) / 2;
    int py = viewport.y + (viewport.h - panel_h) / 2;
    Rect panel{px, py, panel_w, panel_h};
    fill_rect(buf, panel, t.panel_bg);
    draw_border(buf, panel, t);
    Rect title{panel.x, panel.y, panel.w, 1};
    draw_title(buf, title, "Character", t);

    int header_y = panel.y + 2;
    std::string head = player_name_ + "  -  " + class_hint_;
    draw_string(buf, panel.x + 2, header_y, head, t.title_fg, t.panel_bg);

    int col_y = header_y + 2;
    int left_x = panel.x + 2;
    int left_w = (panel.w - 6) / 2;
    int right_x = left_x + left_w + 2;
    int right_w = panel.x + panel.w - right_x - 2;

    Rect left_col{left_x, col_y, left_w, panel.h - (col_y - panel.y) - 3};
    draw_border_color(buf, left_col, t.panel_border, t.panel_bg);
    draw_string(buf, left_col.x + 2, left_col.y, "Attributes", t.accent, t.panel_bg);

    int ax = left_col.x + 2;
    int ay = left_col.y + 2;
    for (int i = 0; i < 9; ++i) {
        char line[24];
        std::snprintf(line, sizeof(line), "%-3s %3d", ATTR_NAMES[i], attrs_[static_cast<std::size_t>(i)]);
        draw_string(buf, ax, ay + i, line, t.text_fg, t.panel_bg);
    }

    Rect right_col{right_x, col_y, right_w, left_col.h};
    draw_border_color(buf, right_col, t.panel_border, t.panel_bg);
    draw_string(buf, right_col.x + 2, right_col.y, "Derived", t.accent, t.panel_bg);

    char buf2[32];
    int ry = right_col.y + 2;
    std::snprintf(buf2, sizeof(buf2), "HP max: %d", derived_.hp_max);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.hp_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "VP max: %d", derived_.vp_max);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.vp_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "SP max: %d", derived_.sp_max);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.sp_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "Crit:   %d%%", derived_.crit_chance_pct);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.text_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "Carry:  %d", derived_.carry_capacity);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.text_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "Speed:  %d c/s", derived_.speed_cells_per_sec);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.text_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "Barter: %d%%", derived_.barter_discount);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.text_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "Pick:   %d", derived_.pick_skill);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.text_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "Id:     %d", derived_.identify_skill);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.text_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "Pers:   %d", derived_.persuade_skill);
    draw_string(buf, right_col.x + 2, ry++, buf2, t.text_fg, t.panel_bg);
    std::snprintf(buf2, sizeof(buf2), "Status: %s", derived_.encumbered ? "encumbered" : "ok");
    draw_string(buf, right_col.x + 2, ry++, buf2,
                derived_.encumbered ? t.hp_fg : t.text_fg, t.panel_bg);

    int total_skills = std::accumulate(skills_.begin(), skills_.end(), 0);
    char foot[64];
    std::snprintf(foot, sizeof(foot),
                  "Skills sum: %d    Press Enter or Esc to close",
                  total_skills);
    int fx = panel.x + (panel.w - static_cast<int>(std::char_traits<char>::length(foot))) / 2;
    draw_string(buf, fx, panel.y + panel.h - 1, foot, t.disabled_fg, t.panel_bg);
}

ScreenResult CharacterScreen::handle_key(KeyEvent const& ev, ScreenContext& /*ctx*/) {
    if (ev.key == Key::Esc || ev.key == Key::Enter || ev.key == Key::Space) {
        return ScreenResult::Pop;
    }
    return ScreenResult::Stay;
}

}  // namespace ui
}  // namespace ash