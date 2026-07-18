#pragma once
/// Phase 11: in-game HUD overlay (HP/VP/SP bars, minimap corner, clock,
/// location name, contextual hints).
///
/// This is intentionally NOT a `Screen`; it is a static overlay drawn
/// on top of the gameplay viewport. The application calls
/// `HudOverlay::render(buf, viewport, ctx)` every frame after the game
/// world renders but before any modal screens.
#include <string>

#include "render/buffer.hpp"
#include "ui/screen.hpp"

namespace ash {
namespace ui {

class HudOverlay {
public:
    /// Draw the HUD into `buf` using the full `viewport`. Safe to call
    /// with any viewport size; elements shrink gracefully.
    void render(render::Buffer& buf, Rect viewport, ScreenContext const& ctx) const;
};

}  // namespace ui
}  // namespace ash