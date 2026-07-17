#pragma once
/// Phase 07: per-entity components attached to world occupants.
///
/// Phase 7 is built without entt to keep the engine easy to test and
/// build offline. Each entity has an `EntityId` and a tuple of components
/// held in the World. Components are value types so the move/copy
/// semantics stay simple and unit tests can construct entities without a
/// registry.
///
/// Components used by combat (Phase 7):
///   * Position       grid coordinates + facing
///   * Stats          fully-resolved derived stats + current HP/VP/SP
///   * Combatant      the combat-relevant attributes/skills/conditions
///                    plus weapon + armor snapshot
///   * AI             behavior composition + leash + disposition
///   * Corpse         marks an entity as a lootable dead body
///   * InventoryRef   points at the character's inventory snapshot for
///                    loot transfer and corpse drop
///   * FactionId      identifies the entity's faction for disposition
#include <cstdint>
#include <vector>

#include "character/attributes.hpp"
#include "character/condition.hpp"
#include "character/derived.hpp"
#include "character/equipment.hpp"
#include "character/inventory.hpp"
#include "character/skills.hpp"
#include "core/ids.hpp"
#include "core/math.hpp"

namespace ash {
namespace world {

/// Cardinal facing for swing-arc tests.
enum class Facing : std::uint8_t {
    East  = 0,
    South = 1,
    West  = 2,
    North = 3,
};

struct Position {
    core::IVec2 cell{};
    Facing       facing{Facing::East};
};

/// Resolve the unit vector for a facing.
core::IVec2 facing_vec(Facing f) noexcept;

/// Current state of one entity's combat-relevant resources. Mirrors the
/// `Stats` component in the spec; Phase 7 only needs HP since death
/// checks on `hp <= 0`. VP/SP are kept for future spellcasting hooks.
struct Stats {
    int hp{0};
    int hp_max{0};
    int vp{0};
    int vp_max{0};
    int sp{0};
    int sp_max{0};
};

/// Snapshot of one entity's combat identity (weapon + armor + condition
/// list). The `weapon_id` and `armor_id` indexes into the WeaponDatabase
/// and ArmorDatabase respectively (0 = empty). `armor_total_dr` is a
/// precomputed sum so the attack loop doesn't re-walk armor every hit.
struct Combatant {
    character::Attributes    attrs{};
    character::Skills        skills{};
    character::ConditionList conditions{};
    std::uint64_t            weapon_id{0};
    std::uint64_t            armor_id{0};
    /// Cached aggregate DR by damage type; recomputed when armor changes.
    int                      armor_dr_slash{0};
    int                      armor_dr_pierce{0};
    int                      armor_dr_crush{0};
    int                      armor_dr_element{0};
    int                      armor_dr_voice{0};
    int                      dodge{0};        /// includes armor agility mods
    /// Cached derived stats. Recompute via `recompute` whenever inputs
    /// change (armor swap, level-up, condition update).
    character::DerivedStats  derived{};
};

/// The AI behavior composition for one entity. Multiple behaviors are
/// tried in priority order; see `combat::ai` for the behavior tree.
struct AI {
    /// Behavior composition. The first entry is the primary behavior;
    /// subsequent entries are fallbacks if the primary returns Failure.
    std::vector<int> behaviors{};
    /// Optional leash: the entity returns to this position when idle
    /// or fleeing. (-1,-1) means no leash.
    core::IVec2      leash_cell{-1, -1};
    float            leash_radius{0.0f};
    float            aggression{0.5f};      /// 0 = passive, 1 = always attack
    float            self_preservation{0.5f};  /// willingness to flee
    /// Time spent in current behavior, used to avoid behavior thrash.
    int              behavior_time_ms{0};
};

struct InventoryRef {
    character::Inventory inventory{};
};

/// Drop an entity's inventory snapshot into the world when it dies.
struct Corpse {
    /// Loot the body via standard inventory operations; this struct just
    /// marks the entity as a corpse and remembers who used to own it.
    core::EntityId      former_owner{};
    int                 decay_ms{0};  /// 0 = no decay (until looted).
};

/// One occupant of the World. Phase 7 represents entities as values so
/// the test harness can copy/swap them without an ECS registry. The
/// `id` is stable across the entity's lifetime.
struct Entity {
    core::EntityId       id{};
    bool                 has_position{false};
    Position             position{};
    Stats                stats{};
    bool                 has_combatant{false};
    Combatant            combatant{};
    bool                 has_inventory{false};
    InventoryRef         inventory{};
    bool                 has_ai{false};
    AI                   ai{};
    bool                 has_corpse{false};
    Corpse               corpse{};
    /// Living flag — false from the moment death triggers until the
    /// corpse is removed from the world.
    bool                 alive{true};
    /// Set when the entity is the player character (single per World).
    bool                 is_player{false};
};

}  // namespace world
}  // namespace ash
