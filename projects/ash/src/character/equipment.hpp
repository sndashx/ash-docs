#pragma once
/// Phase 06: equip / unequip with attribute and skill requirements.
/// `equip()` checks whether the wearer meets the requirements, then
/// moves the item from inventory into the slot (swapping any current
/// occupant back to inventory). Requirements and per-item effects are
/// provided by the item_db (added in 0607); for Phase 6 we expose a
/// `RequirementSet` struct so tests can synthesize gear without
/// touching JSON.
#include <array>
#include <optional>

#include "character/attributes.hpp"
#include "character/inventory.hpp"
#include "character/skills.hpp"
#include "core/ids.hpp"

namespace ash {
namespace character {

/// Per-item requirements (Phase 6 item_db synthesizes one of these).
/// Each field is a minimum; absent (==0) means "no requirement".
struct RequirementSet {
    std::array<int, kAttributeCount> attr_min{};
    std::array<int, kSkillCount>     skill_min{};
};

/// Result of an attempt to equip an item. Tells the caller whether the
/// equip succeeded, why it didn't, and what was displaced.
struct EquipResult {
    bool                    success{false};
    EquipSlot               slot{EquipSlot::Weapon};
    core::ItemId            equipped_item{};
    std::optional<core::ItemId> displaced{};  /// Previous occupant, if any.
    /// First unmet requirement, for tooltips / log messages.
    char const*             failure_reason{nullptr};
};

/// Equip `item` into `slot`. Reads the item's `slot` requirement via
/// `slot_of`, the per-slot attribute/skill requirements via `req_of`,
/// and writes the previous occupant back to inventory if there was
/// room (else it's dropped on the floor — Phase 7 handles the floor).
/// The two callbacks are dependency-injected so this header does not
/// pull in the item_db directly.
using SlotOfFn    = EquipSlot (*)(core::ItemId);
using ReqOfFn     = RequirementSet (*)(core::ItemId);

EquipResult equip(Inventory& inv, EquipSlot slot, core::ItemId item,
                  SlotOfFn slot_of, ReqOfFn req_of) noexcept;

/// Move the currently equipped item at `slot` back into the inventory.
/// Returns true on success; returns false if the inventory is full (the
/// item stays equipped). On success, `out_inv_item` (if non-null) is
/// set to the displaced item id.
bool unequip(Inventory& inv, EquipSlot slot,
             core::ItemId* out_inv_item = nullptr) noexcept;

/// Convenience: does `attr` meet the per-slot requirement?
bool meets_attribute_requirement(Attributes const& a, RequirementSet const& r) noexcept;

/// Convenience: does `s` meet the per-slot requirement?
bool meets_skill_requirement(Skills const& s, RequirementSet const& r) noexcept;

}  // namespace character
}  // namespace ash
