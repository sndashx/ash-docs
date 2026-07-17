#pragma once
/// Phase 06: status conditions (Pillar 5 + phase spec step 0606).
/// Each active condition carries a duration in milliseconds (the engine
/// ticks in ms) and a magnitude (used by damage-over-time conditions
/// like Bleeding/Burning/Frozen and by Inspired's XP bonus). Sources
/// are tracked so a dispel can tell who/what applied it.
#include <array>
#include <cstdint>
#include <vector>

#include "character/derived.hpp"
#include "core/ids.hpp"

namespace ash {
namespace character {

enum class Condition : std::uint8_t {
    Bleeding = 0,
    Stunned,
    Poisoned,
    Terrified,
    Blessed,
    Cursed,
    Warded,
    Slowed,
    Hasted,
    Silenced,
    Rooted,
    Burning,
    Frozen,
    Dazed,
    Inspired,
    Count,
};

inline constexpr std::size_t kConditionCount = static_cast<std::size_t>(Condition::Count);

char const* condition_name(Condition c) noexcept;

struct ActiveCondition {
    Condition  type{Condition::Count};
    int        duration_ms{0};
    int        magnitude{0};
    /// Item or NPC id that applied the condition. 0 = unknown.
    std::uint64_t source{0};
};

struct ConditionList {
    std::vector<ActiveCondition> active{};

    /// Returns true if the condition is currently active.
    bool has(Condition c) const noexcept;

    /// Returns the strongest active instance of `c` (largest magnitude),
    /// or nullptr if none.
    ActiveCondition const* find(Condition c) const noexcept;

    /// Apply (or refresh) `c` with `duration_ms` and `magnitude`.
    /// If a stronger instance already exists, this is a no-op.
    void apply(Condition c, int duration_ms, int magnitude,
               std::uint64_t source = 0) noexcept;

    /// Remove every active instance of `c`. Returns the count removed.
    int clear(Condition c) noexcept;

    /// Advance all timers by `dt_ms`. Removes expired entries. Returns
    /// the number of conditions that expired this tick.
    int tick(int dt_ms) noexcept;

    /// Compose a `ConditionMod` snapshot for the derived-stat recompute.
    ConditionMod mod() const noexcept;
};

}  // namespace character
}  // namespace ash
