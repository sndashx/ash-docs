#pragma once
/// Phase 07: attack resolution (Step 0703).
/// `attack_resolve` rolls d20 + skill/attribute bonus vs. dodge+armor,
/// optionally crits, rolls damage, applies it through armor DR, and
/// returns the result. The caller (CombatManager) turns the result into
/// HP changes, status effects, log lines, and animations.
///
/// All randomness flows through the injected `RngFn`. Tests pin a
/// deterministic seed; gameplay uses the engine's xoshiro PRNG.
#include <cstdint>

#include "character/skills.hpp"
#include "combat/armor.hpp"
#include "combat/cover.hpp"
#include "combat/weapon.hpp"
#include "core/ids.hpp"
#include "world/components.hpp"

namespace ash {
namespace combat {

struct AttackResult {
    bool       hit{false};
    bool       crit{false};
    int        damage{0};      /// Final HP to subtract (post-DR, post-crit).
    DamageType type{DamageType::Slash};
    Cover      cover{Cover::None};
    char const* message{"miss"}; /// Human-readable log line.
};

/// RNG callback. Returns an int in [lo, hi].
using RngFn = int (*)(int lo, int hi, void* user);
inline RngFn g_attack_rng = nullptr;
inline void* g_attack_rng_user = nullptr;

/// Resolve an attack. `attacker` and `defender` must be alive combat
/// entities. `weapon` may be null (unarmed). `cover` is computed by the
/// caller (typically via `compute_cover`). `rng` defaults to the global
/// xoshiro callback set up at app startup.
AttackResult attack_resolve(world::Entity const& attacker,
                            world::Entity&      defender,
                            WeaponDef const*    weapon,
                            Cover               cover,
                            RngFn               rng = nullptr,
                            void*               rng_user = nullptr) noexcept;

/// Convenience: convert a swing arc (degrees) + facing + attacker /
/// defender positions into a yes/no answer (Step 0703.00.04).
bool in_swing_arc(core::IVec2 attacker_cell,
                  core::IVec2 target_cell,
                  world::Facing attacker_facing,
                  int swing_arc_deg) noexcept;

}  // namespace combat
}  // namespace ash
