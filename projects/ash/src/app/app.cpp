#include "app/app.hpp"

#include "app/ui_loop.hpp"
#include "character/attributes.hpp"
#include "character/derived.hpp"
#include "character/inventory.hpp"
#include "character/skills.hpp"
#include "core/log.hpp"
#include "core/version.hpp"
#include "editor/editor.hpp"
#include "editor/map_io.hpp"
#include "platform/signal.hpp"
#include "render/ansi.hpp"
#include "settings/settings.hpp"
#include "ui/screen_context.hpp"

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
    if (args_.ui_demo) {
        /// Phase 11: drive the UI mode stack end-to-end.
        ui::ScreenContext ctx;
        ctx.settings = settings::load();
        ctx.refresh_accessibility();
        std::string script = args_.script_path ? *args_.script_path : std::string{};
        return run_ui_demo(ctx, script);
    }
    if (args_.perf_bench) {
        /// Phase 11: performance budgets D66/D67/D68.
        ui::ScreenContext ctx;
        ctx.settings = settings::load();
        ctx.refresh_accessibility();
        return run_perf_bench(ctx);
    }
    if (args_.editor_test) {
        /// Drive the in-game editor (Phase 4) non-interactively.
        ///
        /// The non-TTY path here is a smoke harness that exercises
        /// every tool from C++ code — exactly the same sequence a
        /// user would issue from a terminal:
        ///
        ///   1. Switch to Paint tool, stamp '#' on layer 1.
        ///   2. Switch to Fill, fill the rest with '.'.
        ///   3. Pick, sample, paint.
        ///   4. Select a 3x3, copy it, paste to a new location.
        ///   5. Place an NPC entity.
        ///   6. Warp player to (10, 5).
        ///   7. Ctrl+Z four times to unwind.
        ///   8. Save to /tmp/ash_editor_demo.xp and reload to verify.
        using namespace ash::editor;
        Editor ed;
        ed.init();
        std::cout << "ASH Phase 4 editor smoke harness\n";
        std::cout << ed.status_line() << "\n";

        // 1. Paint.
        ed.current_tool = Tool::Paint;
        ed.paint_cell = render::Cell{ uint32_t('#'), 220, 220, 220 };
        ed.active_layer = 1;
        EditorInput clk;
        clk.kind = EditorInput::Kind::MouseClick;
        clk.mouse_x = 5; clk.mouse_y = 5;
        ed.handle(clk);
        std::cout << "after paint:    " << ed.status_line() << "\n";
        if (ed.map.at(1, 5, 5).glyph != uint32_t('#')) {
            std::cerr << "FAIL: paint did not stamp glyph\n";
            return 1;
        }

        // 2. Fill.
        ed.current_tool = Tool::Fill;
        ed.fill_cell = render::Cell{ uint32_t('.'), 100, 100, 100 };
        clk.mouse_x = 0; clk.mouse_y = 0;
        ed.handle(clk);
        std::cout << "after fill:     " << ed.status_line() << "\n";

        // 3. Pick + paint again.
        ed.current_tool = Tool::Pick;
        clk.mouse_x = 0; clk.mouse_y = 0;
        ed.handle(clk);
        if (ed.paint_cell.glyph != uint32_t('.')) {
            std::cerr << "FAIL: pick did not sample glyph\n";
            return 1;
        }

        ed.current_tool = Tool::Paint;
        clk.mouse_x = 7; clk.mouse_y = 7;
        ed.handle(clk);
        if (ed.map.at(1, 7, 7).glyph != uint32_t('.')) {
            std::cerr << "FAIL: paint after pick did not stamp\n";
            return 1;
        }

        // 4. Select, copy, paste.
        ed.current_tool = Tool::Select;
        EditorInput mm;
        mm.kind = EditorInput::Kind::MouseMove;
        mm.mouse_x = 10; mm.mouse_y = 10;
        ed.handle(mm);
        EditorInput mc;
        mc.kind = EditorInput::Kind::MouseClick;
        mc.mouse_x = 10; mc.mouse_y = 10;
        ed.handle(mc);
        mm.mouse_x = 13; mm.mouse_y = 13;
        ed.handle(mm);
        ed.selection.x0 = 10; ed.selection.y0 = 10;
        ed.selection.x1 = 13; ed.selection.y1 = 13;
        ed.selection = ed.selection.normalize();
        EditorInput ctrl;
        ctrl.kind = EditorInput::Kind::Ctrl;
        ctrl.ctrl_letter = 'c';
        ed.handle(ctrl);
        if (!ed.clipboard.has_data) {
            std::cerr << "FAIL: copy did not populate clipboard\n";
            return 1;
        }
        ctrl.ctrl_letter = 'v';
        ed.cursor = { 20, 20 };
        ed.handle(ctrl);
        std::cout << "after copy/paste: " << ed.status_line() << "\n";

        // 5. Place entity.
        ed.current_tool = Tool::Entity;
        mc.mouse_x = 15; mc.mouse_y = 15;
        ed.handle(mc);
        if (ed.map.entities.size() != std::size_t{1}) {
            std::cerr << "FAIL: place entity did not add entity\n";
            return 1;
        }
        std::cout << "after place entity: " << ed.status_line() << "\n";

        // 6. Warp player.
        ed.current_tool = Tool::Warp;
        mc.mouse_x = 10; mc.mouse_y = 5;
        ed.handle(mc);
        bool found = false;
        for (auto const& e : ed.map.entities) {
            if (e.id == 0 && e.pos.x == 10 && e.pos.y == 5) found = true;
        }
        if (!found) {
            std::cerr << "FAIL: warp did not move player\n";
            return 1;
        }
        std::cout << "after warp: " << ed.status_line() << "\n";

        // 7. Undo four times.
        for (int i = 0; i < 4; ++i) ed.undo.undo(ed.map);
        std::cout << "after 4x undo:  " << ed.status_line() << "\n";

        // 8. Save + reload roundtrip.
        std::string path = "/tmp/ash_editor_demo.xp";
        if (!ed.save_to(path)) {
            std::cerr << "FAIL: save_to returned false\n";
            return 1;
        }
        Map saved;
        if (!load_map(saved, path)) {
            std::cerr << "FAIL: load_map returned false\n";
            return 1;
        }
        if (saved.width != ed.map.width || saved.height != ed.map.height
            || saved.map_id != ed.map.map_id) {
            std::cerr << "FAIL: reloaded map metadata mismatch\n";
            return 1;
        }
        // Verify a known painted cell survived the roundtrip.
        if (saved.at(1, 5, 5).glyph != uint32_t('#')) {
            std::cerr << "FAIL: reloaded cell glyph mismatch at (5,5) layer 1\n";
            return 1;
        }
        // Verify entity count survived too.
        if (saved.entities.size() != ed.map.entities.size()) {
            std::cerr << "FAIL: reloaded entity count mismatch\n";
            return 1;
        }
        std::cout << "save+reload ok  (" << saved.width << "x" << saved.height
                  << " id=" << saved.map_id << ")\n";
        std::cout << "OK: editor smoke harness passed\n";
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