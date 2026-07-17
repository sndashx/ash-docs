#include "test_harness.hpp"

#include "combat/ai.hpp"
#include "combat/combat.hpp"
#include "world/map.hpp"
#include "world/spawn.hpp"

using ash::combat::ai_tick;
using ash::combat::Behavior;
using ash::combat::Behavior::Guard;
using ash::combat::Behavior::Idle;
using ash::combat::Behavior::Berserk;
using ash::combat::Behavior::Flee;
using ash::combat::Behavior::Caster;
using ash::combat::Behavior::Ambusher;
using ash::combat::Behavior::Coward;
using ash::combat::Behavior::PackHunter;
using ash::combat::Behavior::Territorial;
using ash::combat::AiContext;
using ash::combat::CombatManager;
using ash::combat::CombatState;
using ash::world::Entity;
using ash::world::make_combatant;
using ash::character::Attributes;
using ash::character::Skills;

TEST_CASE("ai: Idle always succeeds", "[combat][ai]")
{
    ash::world::World w;
    Entity e = make_combatant({5, 5}, Attributes{}, Skills{}, 10);
    e.has_ai = true;
    e.ai.behaviors = {static_cast<int>(Idle)};
    auto id = w.add(std::move(e));
    AiContext ctx{w, id, 16};
    REQUIRE(ai_tick(ctx) == ash::combat::NodeResult::Success);
}

TEST_CASE("ai: Guard fails with no target", "[combat][ai]")
{
    ash::world::World w;
    Entity e = make_combatant({5, 5}, Attributes{}, Skills{}, 10);
    e.has_ai = true;
    e.ai.behaviors = {static_cast<int>(Guard)};
    auto id = w.add(std::move(e));
    AiContext ctx{w, id, 16};
    REQUIRE(ai_tick(ctx) == ash::combat::NodeResult::Failure);
}

TEST_CASE("ai: Berserk succeeds when target exists", "[combat][ai]")
{
    ash::world::World w;
    Entity a = make_combatant({5, 5}, Attributes{}, Skills{}, 10);
    a.has_ai = true;
    a.ai.behaviors = {static_cast<int>(Berserk)};
    auto aid = w.add(std::move(a));
    Entity b = make_combatant({7, 5}, Attributes{}, Skills{}, 10);
    auto bid = w.add(std::move(b));
    AiContext ctx{w, aid, 16};
    REQUIRE(ai_tick(ctx) == ash::combat::NodeResult::Success);
}

TEST_CASE("ai: composition falls through on Failure", "[combat][ai]")
{
    ash::world::World w;
    Entity e = make_combatant({5, 5}, Attributes{}, Skills{}, 10);
    e.has_ai = true;
    /// First: Guard fails (no enemy). Second: Idle succeeds.
    e.ai.behaviors = {static_cast<int>(Guard), static_cast<int>(Idle)};
    auto id = w.add(std::move(e));
    AiContext ctx{w, id, 16};
    REQUIRE(ai_tick(ctx) == ash::combat::NodeResult::Success);
}

TEST_CASE("ai: Flee fails when threat is distant", "[combat][ai]")
{
    ash::world::World w;
    Entity me = make_combatant({5, 5}, Attributes{}, Skills{}, 10);
    me.has_ai = true;
    me.ai.behaviors = {static_cast<int>(Flee)};
    auto mid = w.add(std::move(me));
    Entity foe = make_combatant({20, 20}, Attributes{}, Skills{}, 10);
    w.add(std::move(foe));
    AiContext ctx{w, mid, 16};
    REQUIRE(ai_tick(ctx) == ash::combat::NodeResult::Failure);
}

TEST_CASE("ai: Caster requires mid range", "[combat][ai]")
{
    ash::world::World w;
    Entity me = make_combatant({5, 5}, Attributes{}, Skills{}, 10);
    me.has_ai = true;
    me.ai.behaviors = {static_cast<int>(Caster)};
    auto mid = w.add(std::move(me));
    Entity foe = make_combatant({10, 5}, Attributes{}, Skills{}, 10);
    w.add(std::move(foe));
    AiContext ctx{w, mid, 16};
    REQUIRE(ai_tick(ctx) == ash::combat::NodeResult::Success);
}

TEST_CASE("combat: manager begin + tick + end", "[combat][manager]")
{
    ash::world::World w;
    Entity p = make_combatant({1, 1}, Attributes{}, Skills{}, 30);
    Entity e = make_combatant({3, 1}, Attributes{}, Skills{}, 30);
    auto pid = w.add(std::move(p));
    auto eid = w.add(std::move(e));
    CombatManager mgr;
    mgr.begin_combat(w, {pid, eid});
    REQUIRE(mgr.state() == CombatState::InCombat);
    REQUIRE(mgr.order().size() == 2);
    mgr.tick(w, 16);
    REQUIRE(mgr.round_number() == 1);
    mgr.pause();
    REQUIRE(mgr.is_paused());
    mgr.tick(w, 16);  /// paused: should not advance round
    REQUIRE(mgr.round_number() == 1);
    mgr.resume();
    REQUIRE(!mgr.is_paused());
    mgr.end_combat();
    REQUIRE(mgr.state() == CombatState::OutOfCombat);
}