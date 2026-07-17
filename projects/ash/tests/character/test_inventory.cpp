#include "test_harness.hpp"
#include "character/inventory.hpp"

using ash::character::Inventory;
using ash::character::EquipSlot;
using ash::character::ItemStack;
using ash::core::ItemId;

TEST_CASE("inventory: empty by default", "[character][inventory]")
{
    Inventory inv;
    REQUIRE(inv.items.empty());
    REQUIRE(inv.count_of(ItemId{42}) == 0);
    REQUIRE(inv.has_item(ItemId{42}) == false);
    REQUIRE(inv.total_count() == 0);
    REQUIRE(inv.current_weight == 0.0f);
}

TEST_CASE("inventory: add_item stacks on matching id", "[character][inventory]")
{
    Inventory inv;
    ItemId const apple{1};
    REQUIRE(inv.add_item(apple, 3, nullptr) == true);
    REQUIRE(inv.total_count() == 3);
    REQUIRE(inv.count_of(apple) == 3);
    REQUIRE(inv.add_item(apple, 2, nullptr) == true);
    REQUIRE(inv.items.size() == 1u);
    REQUIRE(inv.count_of(apple) == 5);
}

TEST_CASE("inventory: add_item respects max_slots", "[character][inventory]")
{
    Inventory inv;
    inv.max_slots = 2;
    REQUIRE(inv.add_item(ItemId{1}, 1, nullptr) == true);
    REQUIRE(inv.add_item(ItemId{2}, 1, nullptr) == true);
    REQUIRE(inv.add_item(ItemId{3}, 1, nullptr) == false);
}

TEST_CASE("inventory: add_item tracks weight", "[character][inventory]")
{
    Inventory inv;
    float const w = 1.5f;
    REQUIRE(inv.add_item(ItemId{7}, 4, &w) == true);
    REQUIRE(inv.current_weight == 6.0f);
}

TEST_CASE("inventory: remove_item decrements and drops empty stacks", "[character][inventory]")
{
    Inventory inv;
    ItemId const potion{9};
    float const w = 0.5f;
    inv.add_item(potion, 5, &w);
    REQUIRE(inv.total_count() == 5);
    int const removed = inv.remove_item(potion, 3, &w);
    REQUIRE(removed == 3);
    REQUIRE(inv.count_of(potion) == 2);
    REQUIRE(inv.current_weight == 1.0f);

    int const removed2 = inv.remove_item(potion, 100, &w);
    REQUIRE(removed2 == 2);
    REQUIRE(inv.count_of(potion) == 0);
    REQUIRE(inv.items.empty());
    REQUIRE(inv.current_weight == 0.0f);
}

TEST_CASE("inventory: remove_item of unknown returns 0", "[character][inventory]")
{
    Inventory inv;
    REQUIRE(inv.remove_item(ItemId{999}, 1, nullptr) == 0);
}

TEST_CASE("inventory: has_item with custom count", "[character][inventory]")
{
    Inventory inv;
    inv.add_item(ItemId{4}, 2, nullptr);
    REQUIRE(inv.has_item(ItemId{4}, 2) == true);
    REQUIRE(inv.has_item(ItemId{4}, 3) == false);
    REQUIRE(inv.has_item(ItemId{99}) == false);
}