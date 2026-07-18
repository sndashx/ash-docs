#include "ui/screen_context.hpp"

namespace ash {
namespace ui {

void ScreenContext::refresh_accessibility() {
    render_acc = settings::derive_render(settings);
    ui_acc     = settings::derive_ui(settings);
    if (render_acc.palette_variant == "high_contrast" ||
        settings.high_contrast) {
        palette = render::high_contrast_palette();
    } else if (render_acc.palette_variant == "no_color" ||
               render_acc.glyphs_for_color ||
               settings.no_color) {
        palette = render::no_color_palette();
    } else if (render_acc.palette_variant != "default") {
        palette = render::palette_for_mode(render_acc.palette_variant);
    } else {
        palette = render::default_256();
    }
}

}  // namespace ui
}  // namespace ash
