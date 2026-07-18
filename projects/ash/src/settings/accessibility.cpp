#include "settings/accessibility.hpp"

#include <algorithm>

namespace ash {
namespace settings {

const char* glyph_for_color(int r, int g, int b) {
    if (r == 0 && g == 0 && b == 0)         return "@";
    int maxc = std::max({r, g, b});
    int minc = std::min({r, g, b});
    int delta = maxc - minc;
    if (delta < 24 && maxc < 64)            return ".";
    if (delta < 32 && maxc >= 64 && maxc < 160) return "o";
    if (delta < 32 && maxc >= 160)          return "*";
    if (r > g && r > b) {
        if (delta < 64) return "+";
        return "R";
    }
    if (g > r && g >= b) return "G";
    if (b > r && b >= g) return "B";
    if (delta < 64) return "x";
    return "#";
}

RenderAccessibility derive_render(const Settings& s) {
    RenderAccessibility r;
    r.use_color         = !s.no_color;
    r.high_contrast     = s.high_contrast;
    r.palette_variant   = s.palette_variant;
    r.palette_swap      = s.colorblind_mode || s.palette_variant != "default";
    r.glyphs_for_color  = s.no_color;
    return r;
}

UiAccessibility derive_ui(const Settings& s) {
    UiAccessibility u;
    u.text_speed_ms     = s.text_speed_ms;
    u.reduce_motion     = s.reduce_motion;
    u.screen_shake      = s.screen_shake;
    u.subtitle_size     = s.subtitle_size;
    u.dyslexic_font     = s.dyslexic_font;
    u.audio_cue_volume  = s.audio_cue_volume;
    return u;
}

}  // namespace settings
}  // namespace ash