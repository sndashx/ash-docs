#pragma once
/// Phase 06: inventory (Pillar 5 + phase spec step 0604).
/// Items are referenced by ID; an item database (Phase 6 item_db) holds
/// the canonical weight, value, slot, and requirement metadata. The
/// inventory itself only knows about stacks and slots.
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include "core/ids.hpp"

namespace ash {
namespace character {

inline constexpr int kInventoryMaxSlots = 100;
inline constexpr int kStackDefaultCount = 1;

enum class EquipSlot : std::uint8_t {
    Weapon = 0,
    OffHand,
    Chest,
    Helm,
    Boots,
    Gloves,
    RingLeft,
    RingRight,
    Amulet,
    Count,
};

inline constexpr std::size_t kEquipSlotCount = static_cast<std::size_t>(EquipSlot::Count);

char const* equip_slot_name(EquipSlot s) noexcept;

/// One slot in the inventory. Multiple stacks of the same ItemId are
/// collapsed into a single entry with `count > 1`. `condition` is in
/// [0, 100]; 100 = pristine, 0 = broken (Phase 7 uses this for armor).
struct ItemStack {
    core::ItemId id{};
    int          count{kStackDefaultCount};
    int          condition{100};
};

struct Inventory {
    std::vector<ItemStack>              items{};
    std::map<EquipSlot, core::ItemId>   equipped{};
    int                                 max_slots{kInventoryMaxSlots};
    float                               current_weight{0.0f};
    float                               max_weight{0.0f};

    /// Returns the number of items stored across all stacks.
    int total_count() const noexcept;

    /// How many of `id` are currently in the inventory (across stacks).
    int count_of(core::ItemId id) const noexcept;

    /// Returns true if at least `count` of `id` are present.
    bool has_item(core::ItemId id, int count = 1) const noexcept;

    /// Push a new stack or merge into an existing one with the same id
    /// when one is found. `weight_per_unit` is added to `current_weight`
    /// if non-null. Returns true on success, false if the inventory is
    /// full and no merge was possible.
    bool add_item(core::ItemId id, int count = 1, float const* weight_per_unit = nullptr) noexcept;

    /// Remove up to `count` of `id`. Returns the actual number removed.
    int remove_item(core::ItemId id, int count = 1, float const* weight_per_unit = nullptr) noexcept;

    /// Reset weight from external item-db lookup. Used by `derived.cpp`
    /// when it recomputes carry capacity.
    void recompute_weight(float weight_of) noexcept;
};

}  // namespace character
}  // namespace ash
