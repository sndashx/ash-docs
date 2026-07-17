#include "app/app.hpp"

#include "core/log.hpp"
#include "core/version.hpp"
#include "platform/signal.hpp"
#include "render/ansi.hpp"
#include "render/buffer.hpp"
#include "render/buffer_diff.hpp"
#include "render/buffer_emit.hpp"
#include "render/camera.hpp"
#include "render/compositor.hpp"
#include "render/light.hpp"
#include "render/palette.hpp"

#include <iostream>

namespace ash {
namespace app {

namespace {
constexpr const char* kBannerLine2 = "A hand-authored ASCII RPG";

void write_banner_line1(std::ostream& os) {
    os << ansi::set_fg(255, 80, 200)
       << "ASH v" << core::ASH_VERSION_STRING
       << " (" << core::ASH_CODENAME << ")"
       << ansi::reset();
}
}  // namespace

App::App(int argc, char** argv)
    : args_(cli::parse(argc, argv)) {
    log::init();
    if (args_.log_level) {
        log::set_level(*args_.log_level);
    }
    platform::install_signal_handlers();
}

App::~App() {
    // Phase 0: no terminal state to restore yet. Phase 1 will restore
    // raw-mode, alt screen, and hidden cursor.
    std::cout << ansi::show_cursor() << std::flush;
}

int App::run() {
    if (args_.error) {
        return 1;
    }
    if (args_.show_version) {
        write_banner_line1(std::cout);
        std::cout << std::flush;
        return 0;
    }
    if (args_.show_help) {
        cli::usage(std::cout);
        return 0;
    }
    if (args_.render_test) {
        // Phase 0 stub: smoke-test the render primitives so the symbols
        // are exercised. Phase 1 will replace this with a real event loop.
        render::Terminal t = render::terminal_query();
        render::Buffer buf(t);
        buf.fill_rect(0, 0, t.width_cells, t.height_cells,
                      {0x20, 20, 30, 80, 20, 30, 80, 0});
        render::Camera cam;
        cam.width_cells = t.width_cells;
        cam.height_cells = t.height_cells;
        cam.map_width_cells = t.width_cells;
        cam.map_height_cells = t.height_cells;
        cam.world_pos = {render::Camera::Fixed{0}, render::Camera::Fixed{0}};
        render::CompositeInput ci{};
        ci.layers[0] = buf;
        ci.ambient = 0;
        render::LightGrid lg;
        lg.resize(t.width_cells, t.height_cells);
        ci.light = &lg;
        render::Buffer out = render::composite(ci);
        std::cout << ansi::alt_screen_on() << ansi::hide_cursor();
        std::string s;
        render::buffer_emit(out, s,
                            {render::DirtyRect{0, 0, t.width_cells, t.height_cells}});
        std::cout << s << std::flush;
        std::cout << ansi::alt_screen_off() << ansi::show_cursor() << std::flush;
        return 0;
    }

    // Default action: truecolor banner (spec lines 384–386).
    write_banner_line1(std::cout);
    std::cout << "\n"
              << ansi::set_fg(80, 200, 255)
              << kBannerLine2
              << ansi::reset() << std::flush;
    return 0;
}

}  // namespace app
}  // namespace ash