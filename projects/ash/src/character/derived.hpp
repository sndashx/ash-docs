#pragma once
/// Phase 06: derived stats (Pillar 5 + phase spec step 0603).
/// All formulas pinned to integers per spec. The recompute order is:
///   1. Pull base values from Attributes.
///   2. Apply skill contributions.
///   3. Apply condition modifiers (multiplicative on maxes, additive
///      on speed/crit).
///   4. Apply encumbrance (speed penalty if current_weight > 0.8 *
///      carry_capacity).
#include "character/attributes.hpp"
#include "character/inventory.hpp"
#include "character/skills.hpp"

namespace ash {
namespace character {

/// Multipliers and modifiers applied by active conditions. These are
/// computed by `condition.hpp` from the active condition list and
/// passed in here so `derived.hpp` doesn't depend on `condition.hpp`
/// directly (avoids a cycle).
struct ConditionMod {
    float hp_max_mult{1.0f};
    float vp_max_mult{1.0f};
    float sp_max_mult{1.0f};
    float speed_mult{1.0f};
    int   speed_add{0};      /// additive override (Hasted sets this).
    int   crit_add_pct{0};   /// additive override (Inspired).
};

/// The final, fully-resolved stat block used everywhere else in the
/// engine. Held by the `Stats` component in the world.
struct DerivedStats {
    int hp_max{0};
    int vp_max{0};
    int sp_max{0};
    int carry_capacity{0};
    int speed_cells_per_sec{0};
    int crit_chance_pct{0};
    /// Barter multiplier: 1.0 = full price, 0.5 = half price.
    float barter_discount{1.0f};
    int identify_skill{0};
    int pick_skill{0};
    int persuade_skill{0};
    bool encumbered{false};
};

/// Recompute derived stats from raw inputs.
DerivedStats recompute(Attributes const& a,
                       Skills const&     s,
                       Inventory const&  inv,
                       ConditionMod      mod = {}) noexcept;

}  // namespace character
}  // namespace ash