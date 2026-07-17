#pragma once
/// Phase 07: status effect application (Step 0705).
/// Status effects bridge the combat layer to the existing
/// `character::ConditionList` (Phase 6). Phase 7 adds the combat-time
/// helpers: applying Bleeding from a crit, applying Stunned from a
/// blunt weapon, applying Slowed from an ice spell, etc.
#include <cstdint>

#include "character/condition.hpp"
#include "combat/weapon.hpp"
#include "world/components.hpp"

namespace ash {
namespace combat {

/// Apply a `Bleeding` condition to `target` from the given source.
/// Standard "on hit" status that any slash/pierce weapon can apply with
/// the `bleeds` tag. Duration scales with `magnitude`.
void apply_bleeding(world::Entity& target, int magnitude,
                    int duration_ms, std::uint64_t source) noexcept;

/// Apply a `Stunned` condition (loses next turn + speed penalty).
void apply_stunned(world::Entity& target, int duration_ms,
                   std::uint64_t source) noexcept;

/// Apply a `Poisoned` condition.
void apply_poisoned(world::Entity& target, int magnitude,
                    int duration_ms, std::uint64_t source) noexcept;

/// Apply a `Slowed` condition (half speed).
void apply_slowed(world::Entity& target, int duration_ms,
                  std::uint64_t source) noexcept;

/// Apply a generic condition by enum (used by spells, items, etc.).
inline void apply_condition(world::Entity& target,
                            character::Condition c,
                            int magnitude, int duration_ms,
                            std::uint64_t source) noexcept {
    if (target.has_combatant) {
        target.combatant.conditions.apply(c, duration_ms, magnitude, source);
    }
}

/// Drive every active condition's timer by `dt_ms`. Returns the number
/// of conditions that expired this tick (so callers can show a "the
/// bleeding wore off" log line).
int tick_conditions(world::Entity& target, int dt_ms) noexcept;

}  // namespace combat
}  // namespace ash
