#include "test_harness.hpp"

#include "combat/armor.hpp"
#include "combat/damage.hpp"
#include "world/spawn.hpp"

using ash::combat::apply_damage;
using ash::combat::DamageEvent;
using ash::combat::DamageType;
using ash::combat::ArmorDef;
using ash::combat::ArmorDatabase;
using ash::combat::ArmorSlot;
using ash::combat::ArmorSummary;
using ash::combat::summarize;
using ash::character::Attributes;
using ash::character::Skills;
using ash::world::Entity;

TEST_CASE("damage: reduces HP and clamps at zero", "[combat][damage]")
{
    Attributes a;
    Skills s;
    auto e = ash::world::make_combatant(ash::core::IVec2{0, 0}, a, s, 10);
    DamageEvent evt{};
    evt.amount = 4;
    evt.type = DamageType::Slash;
    evt.target = e.id;
    auto out = apply_damage(e, evt, /*dr=*/0);
    REQUIRE(e.stats.hp == 6);
    REQUIRE(!out.target_died);
}

TEST_CASE("damage: kills at hp 0 and spawns corpse entity", "[combat][damage]")
{
    Attributes a;
    Skills s;
    auto e = ash::world::make_combatant(ash::core::IVec2{0, 0}, a, s, 5);
    DamageEvent evt{};
    evt.amount = 99;
    evt.type = DamageType::Crush;
    evt.target = e.id;
    auto out = apply_damage(e, evt, /*dr=*/0);
    REQUIRE(e.stats.hp == 0);
    REQUIRE(out.target_died);
    REQUIRE(!e.alive);
    Entity corpse = ash::combat::make_corpse_from(e);
    REQUIRE(corpse.has_corpse);
    REQUIRE(corpse.alive == false);
    REQUIRE(corpse.position.cell.x == 0);
    REQUIRE(corpse.position.cell.y == 0);
}

TEST_CASE("damage: armor DR reduces damage by exact percent", "[combat][damage]")
{
    Attributes a;
    Skills s;
    auto e = ash::world::make_combatant(ash::core::IVec2{0, 0}, a, s, 100);
    DamageEvent evt{};
    evt.amount = 20;
    evt.type = DamageType::Slash;
    auto out = apply_damage(e, evt, /*dr=*/25);
    REQUIRE(out.amount_dealt == 15);
    REQUIRE(e.stats.hp == 85);
}

TEST_CASE("damage: minimum 1 damage applied", "[combat][damage]")
{
    Attributes a;
    Skills s;
    auto e = ash::world::make_combatant(ash::core::IVec2{0, 0}, a, s, 100);
    DamageEvent evt{};
    evt.amount = 1;
    evt.type = DamageType::Slash;
    auto out = apply_damage(e, evt, /*dr=*/75);
    REQUIRE(out.amount_dealt == 1);
}

TEST_CASE("armor: summary caps each type at 75", "[combat][armor]")
{
    ArmorDef a1{};
    a1.dr_slash = 60;
    ArmorDef a2{};
    a2.dr_slash = 50;
    std::vector<ArmorDef const*> v{&a1, &a2};
    ArmorSummary s = summarize(v, 75);
    REQUIRE(s.slash == 75);
}

TEST_CASE("armor: database loads JSON content", "[combat][armor]")
{
    ArmorDatabase db;
    db.load_from_file("content/items/armor.json");
    REQUIRE(db.size() >= 6);
    auto const* iron = db.get_by_code("iron_hauberk");
    REQUIRE(iron != nullptr);
    REQUIRE(iron->dr_slash == 30);
    REQUIRE(iron->slot == ArmorSlot::Chest);
}