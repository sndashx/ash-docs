#include "combat/ai.hpp"

#include <algorithm>
#include <cmath>

#include "world/query.hpp"

namespace ash {
namespace combat {

char const* behavior_name(Behavior b) noexcept {
    switch (b) {
        case Behavior::Idle:        return "Idle";
        case Behavior::Patrol:      return "Patrol";
        case Behavior::Guard:       return "Guard";
        case Behavior::Flee:        return "Flee";
        case Behavior::Berserk:     return "Berserk";
        case Behavior::Caster:      return "Caster";
        case Behavior::Summoner:    return "Summoner";
        case Behavior::Support:     return "Support";
        case Behavior::Ambusher:    return "Ambusher";
        case Behavior::Coward:      return "Coward";
        case Behavior::PackHunter:  return "PackHunter";
        case Behavior::Territorial: return "Territorial";
        case Behavior::PackRetreat: return "PackRetreat";
        case Behavior::Count:       break;
    }
    return "Idle";
}

Behavior behavior_from_string(std::string const& s) noexcept {
    if (s == "Idle")        return Behavior::Idle;
    if (s == "Patrol")      return Behavior::Patrol;
    if (s == "Guard")       return Behavior::Guard;
    if (s == "Flee")        return Behavior::Flee;
    if (s == "Berserk")     return Behavior::Berserk;
    if (s == "Caster")      return Behavior::Caster;
    if (s == "Summoner")    return Behavior::Summoner;
    if (s == "Support")     return Behavior::Support;
    if (s == "Ambusher")    return Behavior::Ambusher;
    if (s == "Coward")      return Behavior::Coward;
    if (s == "PackHunter")  return Behavior::PackHunter;
    if (s == "Territorial") return Behavior::Territorial;
    if (s == "PackRetreat") return Behavior::PackRetreat;
    return Behavior::Idle;
}

world::Entity* ctx_entity(AiContext& ctx, core::EntityId id) noexcept {
    return ctx.world.find(id);
}
world::Entity const* ctx_entity(AiContext const& ctx, core::EntityId id) noexcept {
    return ctx.world.find(id);
}

namespace {

world::Entity const* find_target(world::World const& w, core::EntityId self) noexcept {
    world::Entity const* me = w.find(self);
    if (me == nullptr || !me->has_position) return nullptr;
    int best_d = 999999;
    world::Entity const* best = nullptr;
    for (auto const& e : w.entities) {
        if (!e.alive || !e.has_position || e.id == self) continue;
        int d = world::chebyshev(me->position.cell, e.position.cell);
        if (d < best_d) {
            best_d = d;
            best = &e;
        }
    }
    return best;
}

}  // namespace

NodeResult bhv_idle(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    if (me == nullptr) return NodeResult::Failure;
    me->ai.behavior_time_ms += ctx.dt_ms;
    return NodeResult::Success;
}

NodeResult bhv_patrol(AiContext& /*ctx*/) noexcept {
    return NodeResult::Success;
}

NodeResult bhv_guard(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    if (me == nullptr) return NodeResult::Failure;
    auto* t = find_target(ctx.world, ctx.self_id);
    if (t == nullptr) return NodeResult::Failure;
    int d = world::chebyshev(me->position.cell, t->position.cell);
    if (d > 8) return NodeResult::Failure;
    return NodeResult::Success;
}

NodeResult bhv_flee(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    auto* t = find_target(ctx.world, ctx.self_id);
    if (me == nullptr || t == nullptr) return NodeResult::Failure;
    int d = world::chebyshev(me->position.cell, t->position.cell);
    if (d < 4) return NodeResult::Success;
    return NodeResult::Failure;
}

NodeResult bhv_berserk(AiContext& ctx) noexcept {
    auto* t = find_target(ctx.world, ctx.self_id);
    return t ? NodeResult::Success : NodeResult::Failure;
}

NodeResult bhv_caster(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    auto* t = find_target(ctx.world, ctx.self_id);
    if (me == nullptr || t == nullptr) return NodeResult::Failure;
    int d = world::chebyshev(me->position.cell, t->position.cell);
    if (d >= 3 && d <= 10) return NodeResult::Success;
    return NodeResult::Failure;
}

NodeResult bhv_summoner(AiContext& /*ctx*/) noexcept {
    return NodeResult::Success;
}

NodeResult bhv_support(AiContext& /*ctx*/) noexcept {
    return NodeResult::Success;
}

NodeResult bhv_ambusher(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    auto* t = find_target(ctx.world, ctx.self_id);
    if (me == nullptr || t == nullptr) return NodeResult::Failure;
    int d = world::chebyshev(me->position.cell, t->position.cell);
    return d <= 3 ? NodeResult::Success : NodeResult::Failure;
}

NodeResult bhv_coward(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    auto* t = find_target(ctx.world, ctx.self_id);
    if (me == nullptr || t == nullptr) return NodeResult::Failure;
    int d = world::chebyshev(me->position.cell, t->position.cell);
    return d <= 5 ? NodeResult::Success : NodeResult::Failure;
}

NodeResult bhv_pack_hunter(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    auto* t = find_target(ctx.world, ctx.self_id);
    if (me == nullptr || t == nullptr) return NodeResult::Failure;
    int d = world::chebyshev(me->position.cell, t->position.cell);
    return d <= 12 ? NodeResult::Success : NodeResult::Failure;
}

NodeResult bhv_territorial(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    if (me == nullptr || !me->has_position) return NodeResult::Failure;
    if (me->ai.leash_cell.x < 0) return NodeResult::Success;
    int d = world::chebyshev(me->position.cell, me->ai.leash_cell);
    int rad = static_cast<int>(me->ai.leash_radius);
    return d <= rad ? NodeResult::Success : NodeResult::Failure;
}

NodeResult bhv_pack_retreat(AiContext& ctx) noexcept {
    auto* t = find_target(ctx.world, ctx.self_id);
    if (t == nullptr) return NodeResult::Success;
    return NodeResult::Failure;
}

NodeResult run_behavior(Behavior b, AiContext& ctx) noexcept {
    switch (b) {
        case Behavior::Idle:        return bhv_idle(ctx);
        case Behavior::Patrol:      return bhv_patrol(ctx);
        case Behavior::Guard:       return bhv_guard(ctx);
        case Behavior::Flee:        return bhv_flee(ctx);
        case Behavior::Berserk:     return bhv_berserk(ctx);
        case Behavior::Caster:      return bhv_caster(ctx);
        case Behavior::Summoner:    return bhv_summoner(ctx);
        case Behavior::Support:     return bhv_support(ctx);
        case Behavior::Ambusher:    return bhv_ambusher(ctx);
        case Behavior::Coward:      return bhv_coward(ctx);
        case Behavior::PackHunter:  return bhv_pack_hunter(ctx);
        case Behavior::Territorial: return bhv_territorial(ctx);
        case Behavior::PackRetreat: return bhv_pack_retreat(ctx);
        case Behavior::Count:       break;
    }
    return NodeResult::Failure;
}

NodeResult ai_tick(AiContext& ctx) noexcept {
    auto* me = ctx_entity(ctx, ctx.self_id);
    if (me == nullptr || !me->alive) return NodeResult::Failure;
    if (!me->has_ai || me->ai.behaviors.empty()) {
        return NodeResult::Failure;
    }
    for (int raw : me->ai.behaviors) {
        Behavior b = static_cast<Behavior>(raw);
        NodeResult r = run_behavior(b, ctx);
        if (r != NodeResult::Failure) {
            return r;
        }
    }
    return NodeResult::Failure;
}

}  // namespace combat
}  // namespace ash