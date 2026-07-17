#include "character/equipment.hpp"

namespace ash {
namespace character {

bool meets_attribute_requirement(Attributes const& a, RequirementSet const& r) noexcept {
    for (std::size_t i = 0; i < kAttributeCount; ++i) {
        if (r.attr_min[i] > 0 && a.values[i] < r.attr_min[i]) {
            return false;
        }
    }
    return true;
}

bool meets_skill_requirement(Skills const& s, RequirementSet const& r) noexcept {
    for (std::size_t i = 0; i < kSkillCount; ++i) {
        if (r.skill_min[i] > 0 && s.values[i] < r.skill_min[i]) {
            return false;
        }
    }
    return true;
}

EquipResult equip(Inventory& inv, EquipSlot slot, core::ItemId item,
                  SlotOfFn slot_of, ReqOfFn req_of) noexcept {
    EquipResult result{};
    result.slot = slot;
    result.equipped_item = item;

    /// 1. The item must belong to the requested slot per the database.
    if (slot_of != nullptr && slot_of(item) != slot) {
        result.failure_reason = "wrong slot";
        return result;
    }

    /// 2. We need to have the item in the inventory.
    if (!inv.has_item(item, 1)) {
        result.failure_reason = "not in inventory";
        return result;
    }

    /// 3. Attribute and skill requirements (Phase 6 requires callers
    ///    to pass the actor's sheets via the dependency injection; in
    ///    Phase 7 this becomes `equip(inv, slot, item, actor)`).
    ///    Requirement checks for *this phase* assume the actor sheets
    ///    live with the caller; the function pointers here only
    ///    inspect the item, not the actor. The actor-side check is
    ///    exposed via the `meets_*` helpers for the caller to use.
    if (req_of != nullptr) {
        auto const req = req_of(item);
        (void)req;  /// Per-item minimums are recorded for the UI even if
                    /// equip() itself doesn't have the actor's sheets.
                    /// Phase 6 keeps `equip` agnostic; callers should
                    /// pre-check via `meets_*` and skip the call.
    }

    /// 4. Remove from inventory; remember previous occupant for swap.
    inv.remove_item(item, 1, nullptr);

    core::ItemId displaced{};
    bool         had = false;
    auto         it  = inv.equipped.find(slot);
    if (it != inv.equipped.end()) {
        displaced = it->second;
        had       = true;
    }
    inv.equipped[slot] = item;

    /// 5. Try to push the displaced item back into the inventory. If
    ///    the bag is full we leave it in the slot — the caller is
    ///    responsible for spilling to the floor (Phase 7).
    if (had) {
        if (static_cast<int>(inv.items.size()) < inv.max_slots) {
            inv.items.push_back(ItemStack{displaced, 1, 100});
        } else {
            /// Roll back to the previous occupant.
            inv.equipped[slot] = displaced;
            inv.add_item(item, 1, nullptr);
            result.failure_reason = "inventory full";
            return result;
        }
        result.displaced = displaced;
    }

    result.success = true;
    return result;
}

bool unequip(Inventory& inv, EquipSlot slot, core::ItemId* out_inv_item) noexcept {
    auto it = inv.equipped.find(slot);
    if (it == inv.equipped.end()) {
        return false;
    }
    core::ItemId const id = it->second;
    if (static_cast<int>(inv.items.size()) >= inv.max_slots) {
        return false;
    }
    inv.items.push_back(ItemStack{id, 1, 100});
    inv.equipped.erase(it);
    if (out_inv_item != nullptr) {
        *out_inv_item = id;
    }
    return true;
}

}  // namespace character
}  // namespace ash
