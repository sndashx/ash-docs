#include "app/app.hpp"

#include "character/attributes.hpp"
#include "character/derived.hpp"
#include "character/inventory.hpp"
#include "character/skills.hpp"
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
    if (args_.render_test) {
        /// Wired up by Phase 1. For now, same banner stub.
        write_banner_line1(std::cout);
        std::cout << "\n"
                  << ansi::set_fg(80, 200, 255)
                  << "(render-test stub)"
                  << ansi::reset() << std::flush;
        return 0;
    }
    if (args_.char_test) {
        /// Spawn a player with starting attrs (40 across the board) and
        /// starting skills (5 across the board), empty inventory, print
        /// the derived stat block per Pillar 5.
        using namespace ash::character;
        Attributes a;
        for (auto& v : a.values) { v = 40; }
        Skills s;
        for (auto& v : s.values) { v = 5; }
        Inventory inv;
        auto const d = recompute(a, s, inv);
        std::cout << "Character sheet (starting player, all attrs=40, all skills=5)\n"
                  << "  HP_max              = " << d.hp_max              << "\n"
                  << "  VP_max              = " << d.vp_max              << "\n"
                  << "  SP_max              = " << d.sp_max              << "\n"
                  << "  carry_capacity      = " << d.carry_capacity      << "\n"
                  << "  speed_cells_per_sec = " << d.speed_cells_per_sec << "\n"
                  << "  crit_chance_pct     = " << d.crit_chance_pct     << "\n"
                  << "  barter_discount     = " << d.barter_discount     << "\n"
                  << "  identify_skill      = " << d.identify_skill      << "\n"
                  << "  pick_skill          = " << d.pick_skill          << "\n"
                  << "  persuade_skill      = " << d.persuade_skill      << "\n"
                  << "  encumbered          = " << (d.encumbered ? "yes" : "no") << "\n"
                  << std::flush;
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