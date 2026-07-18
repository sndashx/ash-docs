#pragma once
/// Phase 10: Autosave tick (per 10.06.002 — every 10 minutes).
///
/// `AutosaveTicker` is a clock-driven helper. The game loop calls
/// `tick(now_seconds)` once per game-second and the ticker reports
/// whether an autosave is due. A typical call site:
///   if (autosave.tick(now_s)) save::save(save::kAutosaveSlot, data);
///
/// Tests pass a mocked clock via `set_now_fn` to assert behaviour
/// without real time.
#include <cstdint>
#include <functional>

#include "save/save_data.hpp"

namespace ash {
namespace save {

class AutosaveTicker {
public:
    /// Default cadence: 600 seconds (10 minutes). Tests override.
    explicit AutosaveTicker(int interval_seconds = 600);

    /// Advance the ticker. Returns true on the frame the autosave is
    /// due. After firing, the internal timer resets so the next fire is
    /// another full interval later.
    bool tick(std::int64_t now_seconds);

    /// Force-fire (e.g. on map transition). Returns the previous "due"
    /// state and resets the timer.
    bool force_fire();

    /// Inject a clock for tests; pass nullptr to clear.
    using NowFn = std::function<std::int64_t()>;
    void set_now_fn(NowFn fn);

    /// Seconds remaining until the next autosave. <=0 means due now.
    std::int64_t remaining(std::int64_t now_seconds) const;

    int interval() const noexcept { return interval_; }

private:
    int             interval_;
    std::int64_t    last_fire_{0};
    bool            has_last_{false};
    NowFn           now_fn_;
};

/// Map-transition trigger. The game code calls this on entering a new
/// map; the ticker fires once if the previous fire was at least
/// `min_interval` seconds ago (so we don't hammer on rapid back-and-
/// forth teleports).
class MapSaveTrigger {
public:
    explicit MapSaveTrigger(int min_interval_seconds = 60);

    /// Returns true when an autosave should fire on this map entry.
    bool on_map_entry(std::int64_t now_seconds);

private:
    int          min_interval_;
    std::int64_t last_fire_{0};
};

}  // namespace save
}  // namespace ash