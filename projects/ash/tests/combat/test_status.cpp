#include "test_harness.hpp"

#include "combat/status.hpp"
#include "world/spawn.hpp"

using ash::combat::apply_bleeding;
using ash::combat::apply_poisoned;
using ash::combat::apply_slowed;
using ash::combat::apply_stunned;
using ash::combat::tick_conditions;
using ash::character::Condition;
using ash::character::Attributes;
using ash::character::Skills;
using ash::world::Entity;

TEST_CASE("status: apply bleeding adds to condition list", "[combat][status]")
{
    Attributes a; Skills s;
    Entity e = ash::world::make_combatant(ash::core::IVec2{0,0}, a, s, 20);
    apply_bleeding(e, /*magnitude=*/2, /*duration_ms=*/3000, /*source=*/1);
    REQUIRE(e.combatant.conditions.has(Condition::Bleeding));
}

TEST_CASE("status: ticking drains HP from Bleeding", "[combat][status]")
{
    Attributes a; Skills s;
    Entity e = ash::world::make_combatant(ash::core::IVec2{0,0}, a, s, 20);
    apply_bleeding(e, 3, 3000, 1);
    tick_conditions(e, 1000);
    REQUIRE(e.stats.hp == 17);
}

TEST_CASE("status: Slowed halves speed", "[combat][status]")
{
    Attributes a; Skills s;
    Entity e = ash::world::make_combatant(ash::core::IVec2{0,0}, a, s, 20);
    int base_speed = e.combatant.derived.speed_cells_per_sec;
    apply_slowed(e, 2000, 1);
    int slowed = static_cast<int>(e.combatant.conditions.mod().speed_mult * base_speed);
    REQUIRE(slowed == base_speed / 2);
}

TEST_CASE("status: Stunned expires after duration", "[combat][status]")
{
    Attributes a; Skills s;
    Entity e = ash::world::make_combatant(ash::core::IVec2{0,0}, a, s, 20);
    apply_stunned(e, 500, 1);
    REQUIRE(e.combatant.conditions.has(Condition::Stunned));
    tick_conditions(e, 600);
    REQUIRE(!e.combatant.conditions.has(Condition::Stunned));
}

TEST_CASE("status: Poisoned stacks on reapply", "[combat][status]")
{
    Attributes a; Skills s;
    Entity e = ash::world::make_combatant(ash::core::IVec2{0,0}, a, s, 50);
    apply_poisoned(e, 4, 4000, 1);
    apply_poisoned(e, 8, 8000, 2);
    auto* c = e.combatant.conditions.find(Condition::Poisoned);
    REQUIRE(c != nullptr);
    REQUIRE(c->magnitude == 8);
    REQUIRE(c->duration_ms == 8000);
}