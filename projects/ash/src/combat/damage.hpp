#pragma once
/// Phase 07: damage application + resistance (Step 0704).
/// A `DamageEvent` is the result of a successful attack. The `apply`
/// function performs resistance lookup, armor DR subtraction, deducts
/// HP from the target's `Stats`, and emits a `DamageOutcome` the caller
/// (UI, log, animations) can act on.
#include <cstdint>

#include "character/condition.hpp"
#include "combat/weapon.hpp"
#include "core/ids.hpp"
#include "world/components.hpp"

namespace ash {
namespace combat {

/// A damage instance applied to a target.
struct DamageEvent {
    int          amount{0};
    DamageType   type{DamageType::Slash};
    core::EntityId source{};
    core::EntityId target{};
    /// "Bleeding on hit" probability etc. Phase 7 applies condition by
    /// separate explicit call; this struct just carries the event.
    bool         crit{false};
};

/// The result of `apply_damage`. The caller decides whether to spawn a
/// corpse, emit a log line, etc.
struct DamageOutcome {
    int  amount_dealt{0};   /// HP actually removed (post-DR).
    bool target_died{false};
};

/// Damage resistance lookup. By default an entity has 0 resistance. The
/// caller can subclass via the `ResistanceFn` callback to layer in
/// condition-derived or item-derived resistance.
using ResistanceFn = int (*)(core::EntityId target, DamageType type);

inline ResistanceFn default_resistance = nullptr;

/// Apply `evt` to `target` (which must point at the entity with id
/// `evt.target`). `dr_by_type` is the precomputed armor DR for the
/// damage type. Returns the outcome. `res_fn` may be nullptr.
DamageOutcome apply_damage(world::Entity&                target,
                           DamageEvent const&           evt,
                           int                          dr_by_type,
                           ResistanceFn                 res_fn = nullptr) noexcept;

/// Spawn a corpse entity carrying the target's inventory snapshot.
/// Returns the new entity id; the caller is responsible for inserting it
/// into the world.
world::Entity make_corpse_from(world::Entity const& dead) noexcept;

}  // namespace combat
}  // namespace ash