#pragma once
/// Phase 0 step 0008 / Phase 1 step 0116: Top-level application lifecycle.
#include "app/cli.hpp"

namespace ash {
namespace app {

/// Owns CLI args, logging init, and signal handler installation.
/// Phase 0: init → banner → cleanup. Phase 1: extends with input pump + render.
class App {
public:
    App(int argc, char** argv);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// Phase 0 main loop: prints banner, returns 0.
    /// Phase 1 will pump input + render until shutdown requested.
    int run();

    /// Read-only access to parsed CLI args (Phase 1 uses for --render-test etc.).
    const cli::CliArgs& args() const { return args_; }

private:
    cli::CliArgs args_;
};

}  // namespace app
}  // namespace ash