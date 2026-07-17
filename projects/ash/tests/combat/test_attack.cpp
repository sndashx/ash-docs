#include "test_harness.hpp"

#include <string>

#include "combat/attack.hpp"
#include "combat/weapon.hpp"
#include "world/components.hpp"
#include "world/spawn.hpp"

using ash::combat::AttackResult;
using ash::combat::attack_resolve;
using ash::combat::Cover;
using ash::combat::WeaponDef;
using ash::combat::WeaponType;
using ash::combat::DamageType;
using ash::character::Attributes;
using ash::character::Skills;
using namespace ash::core;
using ash::world::Entity;
using ash::world::Facing;

namespace {

/// Deterministic test RNG: returns a sequence of three pre-loaded ints
/// in order: d20, crit_roll (1..100), damage. Tests call reset_rng
/// before each scenario to lay down the exact rolls they want.
struct TestRngState {
    int  rolls[3]{20, 100, 5};
    int  idx{0};
};
TestRngState g_rng_state{};

int test_rng(int lo, int hi, void* /*user*/) {
    int v = g_rng_state.rolls[g_rng_state.idx];
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    g_rng_state.idx = (g_rng_state.idx + 1) % 3;
    return v;
}

void reset_rng(int d20_roll, int crit_roll, int damage_roll) {
    g_rng_state = TestRngState{{d20_roll, crit_roll, damage_roll}, 0};
}

WeaponDef make_test_weapon() {
    WeaponDef w{};
    w.code_name = "test_sword";
    w.type = WeaponType::Melee1H;
    w.damage = DamageType::Slash;
    w.damage_min = 5;
    w.damage_max = 5;
    w.swing_arc_deg = 90;
    w.reach = 1;
    w.attack_speed_ms = 1000;
    return w;
}

Entity make_test_entity(IVec2 c, int hp, int str_lvl, int blade_lvl,
                          int agi_lvl = 40, int dodge_skill = 5) {
    Attributes a;
    a[ash::character::Attribute::Str] = str_lvl;
    a[ash::character::Attribute::Agi] = agi_lvl;
    a[ash::character::Attribute::Wit] = 40;
    a[ash::character::Attribute::Luc] = 40;
    Skills s;
    s[ash::character::Skill::Blade] = blade_lvl;
    s[ash::character::Skill::Dodge] = dodge_skill;
    auto e = ash::world::make_combatant(c, a, s, hp);
    e.id = ash::core::EntityId{1};
    return e;
}

}  // namespace

TEST_CASE("attack: deterministic crit doubles damage", "[combat][attack]")
{
    reset_rng(20, 100, 5);   /// nat 20 = crit, base damage 5 doubled = 10
    WeaponDef w = make_test_weapon();

    Entity att = make_test_entity(IVec2{0, 0}, 10, 40, 30);
    Entity def = make_test_entity(IVec2{1, 0}, 10, 40, 5);
    att.position.facing = Facing::East;

    AttackResult r = attack_resolve(att, def, &w, Cover::None, test_rng);
    REQUIRE(r.hit);
    REQUIRE(r.crit);
    REQUIRE(r.damage >= 1);
    REQUIRE(r.damage == 10);
}

TEST_CASE("attack: miss when low roll vs AC", "[combat][attack]")
{
    reset_rng(1, 100, 5);
    WeaponDef w = make_test_weapon();
    Entity att = make_test_entity(IVec2{0, 0}, 10, 40, 0);
    Entity def = make_test_entity(IVec2{1, 0}, 10, 40, 5, /*agi=*/90);
    att.position.facing = Facing::East;
    AttackResult r = attack_resolve(att, def, &w, Cover::None, test_rng);
    REQUIRE(!r.hit);
}

TEST_CASE("attack: out of swing arc is rejected", "[combat][attack]")
{
    reset_rng(20, 100, 5);
    WeaponDef w = make_test_weapon();
    w.swing_arc_deg = 90;
    Entity att = make_test_entity(IVec2{0, 0}, 10, 40, 30);
    Entity def = make_test_entity(IVec2{1, 0}, 10, 40, 5);
    att.position.facing = Facing::West;
    AttackResult r = attack_resolve(att, def, &w, Cover::None, test_rng);
    REQUIRE(!r.hit);
    REQUIRE(std::string(r.message) == "out_of_arc");
}

TEST_CASE("attack: full cover blocks hit even on nat 20", "[combat][attack]")
{
    reset_rng(20, 100, 5);
    WeaponDef w = make_test_weapon();
    Entity att = make_test_entity(IVec2{0, 0}, 10, 40, 30);
    Entity def = make_test_entity(IVec2{1, 0}, 10, 40, 5);
    att.position.facing = Facing::East;
    AttackResult r = attack_resolve(att, def, &w, Cover::Full, test_rng);
    REQUIRE(!r.hit);
}
