#pragma once
/// Phase 10: Accessibility apply helpers.
///
/// Translates the boolean/quantitative settings into renderer and UI
/// inputs. Phase 11 wires these into the actual palette/text/motion
/// systems; for Phase 10 the helpers are pure functions that return
/// derived values the renderer / UI can consult.
#include "settings/settings.hpp"

namespace ash {
namespace settings {

/// Returns a glyph that replaces a color swatch for "no-color" mode.
/// ASCII-friendly fallbacks so the engine still renders something
/// meaningful when ANSI is suppressed.
const char* glyph_for_color(int r, int g, int b);

/// Apply accessibility settings to derive the renderer-friendly
/// `RenderAccessibility` snapshot. Renderer reads these to swap
/// palettes / drop color / skip motion.
struct RenderAccessibility {
    bool   use_color{true};
    bool   high_contrast{false};
    std::string palette_variant{"default"};
    bool   palette_swap{false};   /// colorblind palette flavor
    bool   glyphs_for_color{false};
};

RenderAccessibility derive_render(const Settings& s);

/// UI accessibility snapshot.
struct UiAccessibility {
    int     text_speed_ms{30};
    bool    reduce_motion{false};
    bool    screen_shake{true};
    int     subtitle_size{1};
    bool    dyslexic_font{false};
    float   audio_cue_volume{1.0f};
};

UiAccessibility derive_ui(const Settings& s);

}  // namespace settings
}  // namespace ash