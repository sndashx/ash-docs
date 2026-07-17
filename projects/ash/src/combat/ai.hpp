#pragma once
/// Phase 07: AI behavior tree (Step 0709).
///
/// Each behavior is a node returning `Success`, `Failure`, or `Running`.
/// The AI tick walks the entity's composed behavior list and runs the
/// highest-priority one that returns `Success` or `Running`. Behaviors
/// can fall back: if the primary returns `Failure`, the next behavior
/// in the composition is tried.
///
/// The 13 behavior tags from the spec are exposed as `enum Behavior`.
/// Each one is implemented as a small free function that returns
/// `NodeResult` based on the world state + the entity's stats.
#include <cstdint>
#include <vector>

#include "core/ids.hpp"
#include "world/world.hpp"

namespace ash {
namespace combat {

enum class Behavior : std::uint8_t {
    Idle          = 0,
    Patrol        = 1,
    Guard         = 2,
    Flee          = 3,
    Berserk       = 4,
    Caster        = 5,
    Summoner      = 6,
    Support       = 7,
    Ambusher      = 8,
    Coward        = 9,
    PackHunter    = 10,
    Territorial   = 11,
    PackRetreat   = 12,
    Count,
};

enum class NodeResult : std::uint8_t {
    Failure = 0,
    Success = 1,
    Running = 2,
};

char const* behavior_name(Behavior b) noexcept;
Behavior    behavior_from_string(std::string const& s) noexcept;

struct AiContext {
    world::World&           world;
    core::EntityId          self_id{};
    /// Simulation time since the AI tick started, used by long-running
    /// behaviors that return `Running`.
    int                     dt_ms{0};
};

/// Look up an entity by id in the context (returns nullptr if missing).
world::Entity*       ctx_entity(AiContext& ctx, core::EntityId id) noexcept;
world::Entity const* ctx_entity(AiContext const& ctx, core::EntityId id) noexcept;

/// One step of the behavior tree. Each behavior is a free function
/// returning `NodeResult`. AI tick tries them in priority order.
NodeResult bhv_idle        (AiContext& ctx) noexcept;
NodeResult bhv_patrol      (AiContext& ctx) noexcept;
NodeResult bhv_guard       (AiContext& ctx) noexcept;
NodeResult bhv_flee        (AiContext& ctx) noexcept;
NodeResult bhv_berserk     (AiContext& ctx) noexcept;
NodeResult bhv_caster      (AiContext& ctx) noexcept;
NodeResult bhv_summoner    (AiContext& ctx) noexcept;
NodeResult bhv_support     (AiContext& ctx) noexcept;
NodeResult bhv_ambusher    (AiContext& ctx) noexcept;
NodeResult bhv_coward      (AiContext& ctx) noexcept;
NodeResult bhv_pack_hunter (AiContext& ctx) noexcept;
NodeResult bhv_territorial (AiContext& ctx) noexcept;
NodeResult bhv_pack_retreat(AiContext& ctx) noexcept;

/// Dispatch by enum (used by the AI tick).
NodeResult run_behavior(Behavior b, AiContext& ctx) noexcept;

/// One step of the AI for one entity. Walks the entity's behavior
/// composition in priority order and returns the first non-Failure.
NodeResult ai_tick(AiContext& ctx) noexcept;

}  // namespace combat
}  // namespace ash
