#pragma once
/// Phase 01 step 0103: SIGWINCH, SIGINT, SIGTERM handlers.
/// Handlers are async-signal-safe: atomic store only.
#include <atomic>

namespace ash {
namespace platform {

extern std::atomic<bool> g_resize_pending;
extern std::atomic<bool> g_shutdown_requested;

/// Install SIGWINCH (resize), SIGINT (Ctrl-C), SIGTERM handlers.
/// Idempotent — safe to call from main().
void install_signal_handlers();

}  // namespace platform
}  // namespace ash