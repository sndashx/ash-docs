#include "platform/signal.hpp"

#include <atomic>
#include <csignal>

namespace ash {
namespace platform {

std::atomic<bool> g_resize_pending{false};
std::atomic<bool> g_shutdown_requested{false};

namespace {

extern "C" void handle_sigwinch(int /*sig*/) {
    g_resize_pending.store(true, std::memory_order_relaxed);
}

extern "C" void handle_shutdown(int /*sig*/) {
    g_shutdown_requested.store(true, std::memory_order_relaxed);
}

}  // namespace

void install_signal_handlers() {
    struct sigaction sa{};
    sa.sa_handler = &handle_sigwinch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    ::sigaction(SIGWINCH, &sa, nullptr);

    sa.sa_handler = &handle_shutdown;
    ::sigaction(SIGINT,  &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
}

}  // namespace platform
}  // namespace ash