#pragma once
/// Phase 07: factory helpers for common entity types used in combat tests
/// and the demo. These keep the test fixtures tiny and let the combat
/// code stay decoupled from the heavy NPC config schema (Phase 8).
#include "world/components.hpp"
#include "world/world.hpp"

namespace ash {
namespace world {

/// Build a basic humanoid combatant (player or NPC). The resulting
/// entity has Position + Stats + Combatant + InventoryRef. The caller
/// fills in weapon_id/armor_id via Combatant afterwards.
Entity make_combatant(core::IVec2 cell,
                      character::Attributes attrs,
                      character::Skills skills,
                      int hp_max) noexcept;

/// Build a corpse entity at `cell`. The inventory snapshot is copied
/// over so the body can be looted via standard inventory ops.
Entity make_corpse(core::IVec2 cell, InventoryRef const& loot) noexcept;

}  // namespace world
}  // namespace ash