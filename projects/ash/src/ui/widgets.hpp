#pragma once
/// Phase 11: small UI widget helpers for ASCII panels, lists, and bars.
///
/// These are pure functions operating on a `render::Buffer` and a `Rect`.
/// They never own state, allocate a backing store, or call out to other
/// systems — the goal is to keep screen code declarative.
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "render/buffer.hpp"
#include "render/cell.hpp"
#include "render/palette.hpp"
#include "ui/screen.hpp"

namespace ash {
namespace ui {

/// 8-color theme used by the polish layer. Phase 12+ content may swap
/// palettes per region; the theme values below are the engine defaults.
struct Theme {
    render::Color text_fg{220, 220, 220};
    render::Color text_bg{20, 20, 28};
    render::Color panel_fg{220, 220, 220};
    render::Color panel_bg{16, 16, 24};
    render::Color panel_border{120, 120, 160};
    render::Color title_fg{255, 220, 120};
    render::Color accent{120, 220, 255};
    render::Color selected_fg{0, 0, 0};
    render::Color selected_bg{255, 220, 120};
    render::Color disabled_fg{120, 120, 120};
    render::Color hp_fg{220, 60, 60};
    render::Color vp_fg{80, 160, 220};
    render::Color sp_fg{180, 120, 220};
    render::Color bar_bg{60, 60, 80};

    /// Apply accessibility: drop color in no_color mode; swap colors for
    /// colorblind palettes; bump contrast for high_contrast.
    static Theme derive(bool high_contrast, bool no_color,
                        std::string const& palette_variant);
};

/// Draw a single-line border around `r` using the given cell pattern.
void draw_border(render::Buffer& buf, Rect r, Theme const& t);

/// Draw a centered title at the top of `r` (one row, single line).
void draw_title(render::Buffer& buf, Rect r, std::string const& text, Theme const& t);

/// Draw a horizontal bar (used for HP/VP/SP). `value` and `max` are
/// integers; the bar fills the rect's width proportionally.
void draw_hbar(render::Buffer& buf, Rect r, int value, int max,
               render::Color fg, render::Color bg, Theme const& t);

/// Draw a vertical list of strings. The list is scrollable; the cursor
/// marks the selected row. Returns the screen-space rect of the
/// selection (or empty rect if nothing visible).
struct ListItem {
    std::string                            label;
    std::string                            detail;     /// optional second column
    bool                                   enabled{true};
    std::function<void(ScreenContext&)>    on_select;  /// optional callback
};

Rect draw_list(render::Buffer& buf, Rect r,
               std::vector<ListItem> const& items,
               int selected, int scroll, Theme const& t);

/// Draw a wrapped paragraph of text inside `r`. Returns the number of
/// lines actually drawn. Honors an optional cursor row for highlighting.
int draw_text(render::Buffer& buf, Rect r, std::string const& text, Theme const& t);

/// Word-wrap a string into lines of at most `width` columns.
std::vector<std::string> word_wrap(std::string const& text, int width);

}  // namespace ui
}  // namespace ash
