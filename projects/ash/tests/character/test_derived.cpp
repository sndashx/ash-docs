#include "test_harness.hpp"
#include "character/attributes.hpp"
#include "character/derived.hpp"
#include "character/condition.hpp"
#include "character/inventory.hpp"
#include "character/leveling.hpp"
#include "character/skills.hpp"

using namespace ash::character;

namespace {
Attributes starting_attrs() {
    Attributes a;
    for (auto& v : a.values) { v = 40; }
    return a;
}
Skills starting_skills() {
    Skills s;
    for (auto& v : s.values) { v = 5; }
    return s;
}
}  // namespace

TEST_CASE("derived: starting human fixture (9 attrs=40, 24 skills=5)", "[character][derived]")
{
    Attributes a = starting_attrs();
    Skills s     = starting_skills();
    Inventory inv;
    auto const d = recompute(a, s, inv);
    REQUIRE(d.hp_max              == 403);
    REQUIRE(d.vp_max              == 322);
    REQUIRE(d.sp_max              == 241);
    REQUIRE(d.carry_capacity      == 200);
    REQUIRE(d.speed_cells_per_sec == 400);
    REQUIRE(d.crit_chance_pct     == 10);
    REQUIRE(d.barter_discount     >  0.77f);
    REQUIRE(d.barter_discount     <  0.78f);
    REQUIRE(d.identify_skill      == 45);
    REQUIRE(d.pick_skill          == 45);
    REQUIRE(d.persuade_skill      == 45);
    REQUIRE(d.encumbered          == false);
}

TEST_CASE("derived: higher attributes produce higher maxima", "[character][derived]")
{
    Attributes a;
    for (auto& v : a.values) { v = 70; }
    Skills s; for (auto& v : s.values) { v = 50; }
    Inventory inv;
    auto const d = recompute(a, s, inv);
    REQUIRE(d.hp_max              == 700 + 37);
    REQUIRE(d.vp_max              == 560 + 25);
    REQUIRE(d.sp_max              == 420 + 16);
    REQUIRE(d.carry_capacity      == 350);
    REQUIRE(d.speed_cells_per_sec == 700);
    REQUIRE(d.crit_chance_pct     == 17);
}

TEST_CASE("derived: encumbrance halves speed", "[character][derived]")
{
    Attributes a = starting_attrs();
    Skills s = starting_skills();
    Inventory inv;
    inv.current_weight = 165.0f;
    auto const d = recompute(a, s, inv);
    REQUIRE(d.encumbered          == true);
    REQUIRE(d.speed_cells_per_sec == 200);
}

TEST_CASE("derived: condition modifiers compose", "[character][derived]")
{
    Attributes a = starting_attrs();
    Skills s = starting_skills();
    Inventory inv;

    ConditionMod mod{};
    mod.hp_max_mult = 0.5f;
    mod.speed_mult  = 0.5f;
    mod.speed_add   = 100;
    auto const d = recompute(a, s, inv, mod);
    REQUIRE(d.hp_max              == 201);
    REQUIRE(d.speed_cells_per_sec == 300);
}

TEST_CASE("derived: barter floor at 0 and ceiling at 1", "[character][derived]")
{
    Attributes a;
    a[Attribute::Cha] = 100;
    Skills s;
    s[Skill::Mercantile] = 200;
    Inventory inv;
    auto const d = recompute(a, s, inv);
    REQUIRE(d.barter_discount <= 0.0f + 1e-6f);

    /// Pure-base scenario: Barter = 1 - (1+0)/200 = 0.995.
    a[Attribute::Cha] = ash::character::kAttributeMin;
    s[Skill::Mercantile] = 0;
    auto const d2 = recompute(a, s, inv);
    REQUIRE(d2.barter_discount >  0.99f);
    REQUIRE(d2.barter_discount <= 1.0f);
}

TEST_CASE("derived: crit clamps to 95", "[character][derived]")
{
    Attributes a;
    a[Attribute::Luc] = 100;
    Skills s;
    Inventory inv;
    ConditionMod mod{};
    mod.crit_add_pct = 50;
    auto const d = recompute(a, s, inv, mod);
    REQUIRE(d.crit_chance_pct == 75);
    mod.crit_add_pct = 200;
    auto const d2 = recompute(a, s, inv, mod);
    REQUIRE(d2.crit_chance_pct == 95);
}

TEST_CASE("leveling: xp_to_next pinned", "[character][derived][leveling]")
{
    REQUIRE(xp_to_next(1) == 100);   /// Level 1 -> 2: 100
    REQUIRE(xp_to_next(2) == 400);   /// Level 2 -> 3: 400
    REQUIRE(xp_to_next(3) == 900);   /// Level 3 -> 4: 900
    REQUIRE(xp_to_next(10) == 10000); /// Level 10 -> 11: 10000
}

TEST_CASE("leveling: gain_xp levels up and grants points", "[character][derived][leveling]")
{
    LevelState st;
    REQUIRE(st.level == 1);
    int levels = gain_xp(st, 100);   /// 1 -> 2
    REQUIRE(levels == 1);
    REQUIRE(st.level == 2);
    REQUIRE(st.attribute_points == ash::character::kAttributesPerLevel * 2);
    REQUIRE(st.skill_pool == ash::character::kSkillPointsPerLevel * 2);

    /// Big chunk that crosses three levels (2->3 needs 400, 3->4 900, 4->5 1600).
    levels = gain_xp(st, 400 + 900 + 1600);
    REQUIRE(levels == 3);
    REQUIRE(st.level == 5);
}

TEST_CASE("leveling: zero xp is no-op", "[character][derived][leveling]")
{
    LevelState st;
    int levels = gain_xp(st, 0);
    REQUIRE(levels == 0);
    REQUIRE(st.level == 1);
}

TEST_CASE("condition: apply, find, clear", "[character][condition]")
{
    ConditionList cl;
    REQUIRE(cl.has(Condition::Bleeding) == false);
    cl.apply(Condition::Bleeding, 5000, 3, 0);
    REQUIRE(cl.has(Condition::Bleeding) == true);
    REQUIRE(cl.find(Condition::Bleeding)->magnitude == 3);

    cl.apply(Condition::Bleeding, 3000, 5, 0);  /// stronger replaces
    REQUIRE(cl.find(Condition::Bleeding)->magnitude == 5);
    REQUIRE(cl.find(Condition::Bleeding)->duration_ms == 5000);  /// longer wins

    REQUIRE(cl.clear(Condition::Bleeding) == 1);
    REQUIRE(cl.has(Condition::Bleeding) == false);
}

TEST_CASE("condition: tick removes expired", "[character][condition]")
{
    ConditionList cl;
    cl.apply(Condition::Slowed,  1000, 1);
    cl.apply(Condition::Blessed, 3000, 1);
    int const exp1 = cl.tick(1500);
    REQUIRE(exp1 == 1);
    REQUIRE(cl.has(Condition::Slowed)  == false);
    REQUIRE(cl.has(Condition::Blessed) == true);
    int const exp2 = cl.tick(2000);
    REQUIRE(exp2 == 1);
    REQUIRE(cl.has(Condition::Blessed) == false);
}

TEST_CASE("condition: mod() composes Slowed + Hasted", "[character][condition]")
{
    ConditionList cl;
    cl.apply(Condition::Slowed, 1000, 1);
    cl.apply(Condition::Hasted, 1000, 1);
    auto const m = cl.mod();
    /// 0.5 * 1.5 = 0.75
    REQUIRE(m.speed_mult > 0.74f);
    REQUIRE(m.speed_mult < 0.76f);
}

TEST_CASE("condition: mod() applies Inspired crit bonus", "[character][condition]")
{
    ConditionList cl;
    cl.apply(Condition::Inspired, 1000, 1);
    auto const m = cl.mod();
    REQUIRE(m.crit_add_pct == 10);
}

TEST_CASE("condition: enum has 15 entries", "[character][condition]")
{
    REQUIRE(static_cast<int>(Condition::Count) == 15);
    REQUIRE(condition_name(Condition::Bleeding) != nullptr);
}
