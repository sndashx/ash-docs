#include "world/spawn.hpp"

namespace ash {
namespace world {

Entity make_combatant(core::IVec2 cell,
                      character::Attributes attrs,
                      character::Skills skills,
                      int hp_max) noexcept {
    Entity e{};
    e.has_position = true;
    e.position.cell = cell;
    e.has_combatant = true;
    e.combatant.attrs = attrs;
    e.combatant.skills = skills;
    e.combatant.derived = character::recompute(attrs, skills, character::Inventory{});
    /// Populate the combat-relevant snapshot fields from attributes and
    /// skills. The Phase 6 derived stats cover most values; combat
    /// tracks dodge directly (it's a defensive reaction roll).
    e.combatant.dodge = skills[character::Skill::Dodge] / 50;
    e.has_inventory = true;
    e.stats.hp     = hp_max;
    e.stats.hp_max = hp_max;
    e.stats.vp     = e.combatant.derived.vp_max;
    e.stats.vp_max = e.combatant.derived.vp_max;
    e.stats.sp     = e.combatant.derived.sp_max;
    e.stats.sp_max = e.combatant.derived.sp_max;
    e.alive = true;
    return e;
}

Entity make_corpse(core::IVec2 cell, InventoryRef const& loot) noexcept {
    Entity e{};
    e.has_position = true;
    e.position.cell = cell;
    e.has_corpse = true;
    e.corpse.former_owner = core::EntityId{};
    e.corpse.decay_ms = 0;
    e.has_inventory = true;
    e.inventory = loot;
    e.alive = false;
    return e;
}

}  // namespace world
}  // namespace ash