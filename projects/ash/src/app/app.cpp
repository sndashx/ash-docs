#include "app/app.hpp"

#include "core/log.hpp"
#include "core/version.hpp"
#include "platform/signal.hpp"
#include "render/ansi.hpp"

#include <iostream>

namespace ash {
namespace app {

namespace {
constexpr const char* kBannerLine1 = "ASH v0.0.1 (First Spark)";
constexpr const char* kBannerLine2 = "A hand-authored ASCII RPG";
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
        std::cout << ansi::set_fg(255, 80, 200)
                  << "ASH v" << core::ASH_VERSION_STRING
                  << " (" << core::ASH_CODENAME << ")"
                  << ansi::reset() << std::flush;
        return 0;
    }
    if (args_.show_help) {
        cli::usage(std::cout);
        return 0;
    }

    // Default action: truecolor banner (spec lines 384–386).
    std::cout << ansi::set_fg(255, 80, 200)
              << kBannerLine1
              << ansi::reset() << "\n"
              << ansi::set_fg(80, 200, 255)
              << kBannerLine2
              << ansi::reset() << std::flush;
    return 0;
}

}  // namespace app
}  // namespace ash