#include "save/autosave.hpp"

#include "core/time.hpp"

namespace ash {
namespace save {

AutosaveTicker::AutosaveTicker(int interval_seconds)
    : interval_(interval_seconds > 0 ? interval_seconds : 600) {}

bool AutosaveTicker::tick(std::int64_t now_seconds) {
    if (now_seconds <= 0) return false;
    if (!has_last_) {
        last_fire_ = now_seconds;
        has_last_  = true;
        return false;
    }
    if (now_seconds - last_fire_ >= interval_) {
        last_fire_ = now_seconds;
        return true;
    }
    return false;
}

bool AutosaveTicker::force_fire() {
    auto now = now_fn_ ? now_fn_() : core::unix_seconds();
    if (now <= 0) now = 0;
    last_fire_ = now;
    has_last_  = true;
    return true;
}

void AutosaveTicker::set_now_fn(NowFn fn) {
    now_fn_ = std::move(fn);
}

std::int64_t AutosaveTicker::remaining(std::int64_t now_seconds) const {
    if (!has_last_ || now_seconds <= last_fire_) return interval_;
    auto elapsed = now_seconds - last_fire_;
    return elapsed >= interval_ ? 0 : interval_ - elapsed;
}

MapSaveTrigger::MapSaveTrigger(int min_interval_seconds)
    : min_interval_(min_interval_seconds > 0 ? min_interval_seconds : 60) {}

bool MapSaveTrigger::on_map_entry(std::int64_t now_seconds) {
    if (now_seconds - last_fire_ >= min_interval_) {
        last_fire_ = now_seconds;
        return true;
    }
    return false;
}

}  // namespace save
}  // namespace ash