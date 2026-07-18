#pragma once
/// Phase 11: UI loop driver. Stand-in for the real input pump + render
/// loop. Two entry points:
///
///   * `run_ui_demo` — boots the menu, then steps through every screen
///     by reading scripted key events from stdin (one character per
///     "tick"). Renders each frame as a single ANSI snapshot via the
///     render::emit pipeline. Used by `--ui-demo` and the navigation
///     smoke test.
///
///   * `run_perf_bench` — exercises the budgets (D66 frame, D67 memory,
///     D68 startup). Synthesizes 100 entities, drives 600 frames, and
///     asserts elapsed wall time stays under the configured budget.
#include "ui/screen_context.hpp"

namespace ash {
namespace app {

/// Run the UI demo loop. Returns 0 on success, 1 on error. `script_path`
/// may be empty to read keys from stdin (TTY mode).
int run_ui_demo(ui::ScreenContext& ctx, std::string const& script_path);

/// Run the perf benchmark suite. Returns 0 on success, 1 on budget
/// violation. Results are printed to stdout in a parseable summary
/// format.
int run_perf_bench(ui::ScreenContext& ctx);

}  // namespace app
}  // namespace ash