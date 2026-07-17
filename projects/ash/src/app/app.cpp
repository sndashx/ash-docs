#include "app/app.hpp"

#include "core/log.hpp"
#include "core/version.hpp"
#include "platform/signal.hpp"
#include "render/ansi.hpp"

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