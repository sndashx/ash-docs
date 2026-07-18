#include "character/inventory.hpp"

#include <algorithm>
#include <cstring>
#include <functional>

namespace ash {
namespace character {

char const* equip_slot_name(EquipSlot s) noexcept {
    switch (s) {
        case EquipSlot::Weapon:    return "Weapon";
        case EquipSlot::OffHand:   return "Off-Hand";
        case EquipSlot::Chest:     return "Chest";
        case EquipSlot::Helm:      return "Helm";
        case EquipSlot::Boots:     return "Boots";
        case EquipSlot::Gloves:    return "Gloves";
        case EquipSlot::RingLeft:  return "Ring (L)";
        case EquipSlot::RingRight: return "Ring (R)";
        case EquipSlot::Amulet:    return "Amulet";
        case EquipSlot::Count:     break;
    }
    return "Unknown";
}

char const* equip_slot_id(EquipSlot s) noexcept {
    switch (s) {
        case EquipSlot::Weapon:    return "weapon";
        case EquipSlot::OffHand:   return "off_hand";
        case EquipSlot::Chest:     return "chest";
        case EquipSlot::Helm:      return "helm";
        case EquipSlot::Boots:     return "boots";
        case EquipSlot::Gloves:    return "gloves";
        case EquipSlot::RingLeft:  return "ring_left";
        case EquipSlot::RingRight: return "ring_right";
        case EquipSlot::Amulet:    return "amulet";
        case EquipSlot::Count:     break;
    }
    return "unknown";
}

EquipSlot equip_slot_from_id(const char* id) noexcept {
    if (!id) return EquipSlot::Count;
    auto eq = [id](const char* s) { return std::strcmp(id, s) == 0; };
    if (eq("weapon"))    return EquipSlot::Weapon;
    if (eq("off_hand"))   return EquipSlot::OffHand;
    if (eq("chest"))      return EquipSlot::Chest;
    if (eq("helm"))       return EquipSlot::Helm;
    if (eq("boots"))      return EquipSlot::Boots;
    if (eq("gloves"))     return EquipSlot::Gloves;
    if (eq("ring_left"))  return EquipSlot::RingLeft;
    if (eq("ring_right")) return EquipSlot::RingRight;
    if (eq("amulet"))     return EquipSlot::Amulet;
    return EquipSlot::Count;
}

int Inventory::total_count() const noexcept {
    int total = 0;
    for (auto const& s : items) {
        total += s.count;
    }
    return total;
}

int Inventory::count_of(core::ItemId id) const noexcept {
    int total = 0;
    for (auto const& s : items) {
        if (s.id == id) {
            total += s.count;
        }
    }
    return total;
}

bool Inventory::has_item(core::ItemId id, int count) const noexcept {
    return count_of(id) >= count;
}

bool Inventory::add_item(core::ItemId id, int count, float const* weight_per_unit) noexcept {
    if (count <= 0) {
        return false;
    }
    /// Try to merge into an existing stack first.
    for (auto& s : items) {
        if (s.id == id) {
            s.count += count;
            if (weight_per_unit != nullptr) {
                current_weight += static_cast<float>(count) * (*weight_per_unit);
            }
            return true;
        }
    }
    /// Otherwise, allocate a new stack if there's room.
    if (static_cast<int>(items.size()) >= max_slots) {
        return false;
    }
    items.push_back(ItemStack{id, count, 100});
    if (weight_per_unit != nullptr) {
        current_weight += static_cast<float>(count) * (*weight_per_unit);
    }
    return true;
}

int Inventory::remove_item(core::ItemId id, int count, float const* weight_per_unit) noexcept {
    if (count <= 0) {
        return 0;
    }
    int remaining = count;
    for (auto it = items.begin(); it != items.end() && remaining > 0;) {
        if (it->id != id) {
            ++it;
            continue;
        }
        int const take = std::min(remaining, it->count);
        it->count -= take;
        remaining  -= take;
        if (weight_per_unit != nullptr) {
            current_weight -= static_cast<float>(take) * (*weight_per_unit);
        }
        if (it->count <= 0) {
            it = items.erase(it);
        } else {
            ++it;
        }
    }
    if (current_weight < 0.0f) {
        current_weight = 0.0f;
    }
    return count - remaining;
}

void Inventory::recompute_weight(float weight_of) noexcept {
    current_weight = 0.0f;
    for (auto const& s : items) {
        current_weight += static_cast<float>(s.count) * weight_of;
    }
}

}  // namespace character
}  // namespace ash