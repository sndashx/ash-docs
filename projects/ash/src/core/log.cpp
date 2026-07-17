#include "core/log.hpp"

#include <atomic>
#include <cctype>
#include <string>

namespace ash {
namespace log {

namespace {

std::atomic<int> g_level{2};  // 0=trace .. 6=off; default info.

int level_for_name(const std::string& name) {
    std::string lc;
    lc.reserve(name.size());
    for (char c : name) {
        lc.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (lc == "trace") return 0;
    if (lc == "debug") return 1;
    if (lc == "info") return 2;
    if (lc == "warn" || lc == "warning") return 3;
    if (lc == "error" || lc == "err") return 4;
    if (lc == "critical" || lc == "fatal") return 5;
    if (lc == "off") return 6;
    return -1;
}

}  // namespace

void init() {
    // spdlog default-constructs its registry on first use. Nothing to do yet.
}

void set_level(const std::string& level) {
    int v = level_for_name(level);
    if (v >= 0) {
        g_level.store(v, std::memory_order_relaxed);
    }
}

}  // namespace log
}  // namespace ash