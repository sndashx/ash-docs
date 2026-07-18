#include "ui/screen.hpp"

namespace ash {
namespace ui {

ScreenResult Screen::handle_key(KeyEvent const& ev, ScreenContext& /*ctx*/) {
    if (ev.key == Key::Esc) return ScreenResult::Pop;
    return ScreenResult::Stay;
}

ScreenResult Screen::handle_mouse(MouseEvent const& /*ev*/, ScreenContext& /*ctx*/) {
    return ScreenResult::Stay;
}

}  // namespace ui
}  // namespace ash
