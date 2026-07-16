#pragma once
/// Phase 01: CLI argument parsing and entry dispatch.
namespace ash {
namespace app {
/// Entry point used by main(). Parses CLI flags, initializes logging,
/// prints the ASH banner / version / usage, and dispatches to the game
/// loop. Returns the process exit code.
int cli_main(int argc, char** argv);
}  // namespace app
}  // namespace ash
