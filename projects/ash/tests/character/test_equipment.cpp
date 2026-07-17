#include "test_harness.hpp"
#include "character/equipment.hpp"
#include "character/attributes.hpp"
#include "character/skills.hpp"
#include <string>

using namespace ash::character;
using ash::core::ItemId;

namespace {

EquipSlot slot_for(ItemId id) {
    auto v = id.value;
    if ((v % 10) == 0) { return EquipSlot::Weapon;    }
    if ((v % 10) == 1) { return EquipSlot::OffHand;   }
    if ((v % 10) == 2) { return EquipSlot::Chest;     }
    if ((v % 10) == 3) { return EquipSlot::Helm;      }
    if ((v % 10) == 4) { return EquipSlot::Boots;     }
    if ((v % 10) == 5) { return EquipSlot::Gloves;    }
    if ((v % 10) == 6) { return EquipSlot::RingLeft;  }
    if ((v % 10) == 7) { return EquipSlot::RingRight; }
    return EquipSlot::Amulet;
}

RequirementSet req_for(ItemId id) {
    RequirementSet r{};
    if (id.value == 100) {
        r.attr_min[static_cast<std::size_t>(Attribute::Str)] = 60;
    }
    if (id.value == 200) {
        r.skill_min[static_cast<std::size_t>(Skill::Spellcraft)] = 30;
    }
    return r;
}

}  // namespace

TEST_CASE("equipment: nine slots defined", "[character][equipment]")
{
    REQUIRE(static_cast<std::size_t>(EquipSlot::Count) == 9u);
    REQUIRE(equip_slot_name(EquipSlot::Weapon) != nullptr);
}

TEST_CASE("equipment: equip requires item in inventory", "[character][equipment]")
{
    Inventory inv;
    auto const r = equip(inv, EquipSlot::Weapon, ItemId{10}, &slot_for, &req_for);
    REQUIRE(r.success == false);
    REQUIRE(std::string(r.failure_reason ? r.failure_reason : "") == "not in inventory");
}

TEST_CASE("equipment: equip rejects wrong slot", "[character][equipment]")
{
    Inventory inv;
    inv.add_item(ItemId{11}, 1, nullptr);
    auto const r = equip(inv, EquipSlot::Weapon, ItemId{11}, &slot_for, &req_for);
    REQUIRE(r.success == false);
    REQUIRE(std::string(r.failure_reason ? r.failure_reason : "") == "wrong slot");
}

TEST_CASE("equipment: equip happy path records slot", "[character][equipment]")
{
    Inventory inv;
    inv.add_item(ItemId{10}, 1, nullptr);
    auto const r = equip(inv, EquipSlot::Weapon, ItemId{10}, &slot_for, &req_for);
    REQUIRE(r.success == true);
    REQUIRE(inv.equipped[EquipSlot::Weapon] == ItemId{10});
    REQUIRE(inv.count_of(ItemId{10}) == 0);
}

TEST_CASE("equipment: equip swaps previous occupant back to inventory", "[character][equipment]")
{
    Inventory inv;
    inv.max_slots = 10;
    inv.add_item(ItemId{10}, 1, nullptr);
    auto const r1 = equip(inv, EquipSlot::Weapon, ItemId{10}, &slot_for, &req_for);
    REQUIRE(r1.success == true);

    inv.add_item(ItemId{20}, 1, nullptr);
    auto const r2 = equip(inv, EquipSlot::Weapon, ItemId{20}, &slot_for, &req_for);
    REQUIRE(r2.success == true);
    REQUIRE(inv.equipped[EquipSlot::Weapon] == ItemId{20});
    REQUIRE(r2.displaced.has_value());
    REQUIRE(r2.displaced.value() == ItemId{10});
    REQUIRE(inv.has_item(ItemId{10}) == true);
    REQUIRE(inv.has_item(ItemId{20}) == false);
}

TEST_CASE("equipment: equip is a clean swap when there is room", "[character][equipment]")
{
    /// Regression: with max_slots=1 and bag holding the to-be-equipped
    /// item, removing it leaves room for the displaced item, so the
    /// equip succeeds. (The "inventory full" rollback path is a
    /// defensive branch that cannot be reached through the public API
    /// because add_item enforces max_slots on every push; we keep the
    /// branch in source as a guard for future changes.)
    Inventory inv;
    inv.max_slots = 1;
    inv.add_item(ItemId{10}, 1, nullptr);  /// weapon A
    equip(inv, EquipSlot::Weapon, ItemId{10}, &slot_for, &req_for);
    /// Drop A from equipped by removing via. unequip is the supported
    /// path, but to re-add a different weapon we use the swap path.
    inv.remove_item(ItemId{10}, 1, nullptr);  /// bag empty
    inv.add_item(ItemId{20}, 1, nullptr);     /// bag full again
    auto const r = equip(inv, EquipSlot::Weapon, ItemId{20}, &slot_for, &req_for);
    REQUIRE(r.success == true);
    REQUIRE(inv.equipped[EquipSlot::Weapon] == ItemId{20});
}

TEST_CASE("equipment: unequip returns item to inventory", "[character][equipment]")
{
    Inventory inv;
    inv.add_item(ItemId{10}, 1, nullptr);
    equip(inv, EquipSlot::Weapon, ItemId{10}, &slot_for, &req_for);
    ItemId moved{};
    bool const ok = unequip(inv, EquipSlot::Weapon, &moved);
    REQUIRE(ok == true);
    REQUIRE(moved == ItemId{10});
    REQUIRE(inv.equipped.find(EquipSlot::Weapon) == inv.equipped.end());
    REQUIRE(inv.has_item(ItemId{10}) == true);
}

TEST_CASE("equipment: meets_attribute_requirement / meets_skill_requirement", "[character][equipment]")
{
    Attributes a;
    a[Attribute::Str] = 50;
    RequirementSet r{};
    r.attr_min[static_cast<std::size_t>(Attribute::Str)] = 60;
    REQUIRE(meets_attribute_requirement(a, r) == false);
    a[Attribute::Str] = 70;
    REQUIRE(meets_attribute_requirement(a, r) == true);

    Skills s;
    s[Skill::Spellcraft] = 10;
    RequirementSet r2{};
    r2.skill_min[static_cast<std::size_t>(Skill::Spellcraft)] = 30;
    REQUIRE(meets_skill_requirement(s, r2) == false);
    s[Skill::Spellcraft] = 30;
    REQUIRE(meets_skill_requirement(s, r2) == true);
}
