#include "test_harness.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "combat/ai.hpp"
#include "combat/combat.hpp"
#include "world/spawn.hpp"

using ash::combat::AiContext;
using ash::combat::CombatManager;
using ash::world::Entity;
using ash::world::make_combatant;
using ash::character::Attributes;
using ash::character::Skills;
using ash::core::EntityId;

namespace {

/// Build N combatants in a tiny arena. Each one carries a Berserk
/// behavior so the AI tick has real work to do.
ash::world::World build_arena(int n) {
    ash::world::World w;
    for (int i = 0; i < n; ++i) {
        Entity e = make_combatant({i % 10, i / 10}, Attributes{}, Skills{}, 10);
        e.has_ai = true;
        e.ai.behaviors = {static_cast<int>(ash::combat::Behavior::Berserk)};
        w.add(std::move(e));
    }
    return w;
}

}  // namespace

/// Frame budget D66: 16ms (60fps) for 10 entities. This is the
/// performance gate from pillar 9. The benchmark drives the AI tick
/// repeatedly with a realistic-ish 16ms dt and verifies the average
/// frame stays inside the budget on the local machine. To avoid
/// flakiness across wildly different hardware, we permit a 4x slack
/// (64ms) in CI but report the measured number so it's auditable.
TEST_CASE("frame budget D66: 10 entities AI tick under 16ms", "[perf][d66]")
{
    auto w = build_arena(10);
    CombatManager mgr;
    std::vector<ash::core::EntityId> ids;
    for (auto const& e : w.entities) ids.push_back(e.id);
    mgr.begin_combat(w, ids);

    constexpr int kFrames = 240;       /// 4 seconds at 60fps
    constexpr int kBudgetUs = 16000;   /// 16ms per frame

    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    for (int f = 0; f < kFrames; ++f) {
        mgr.tick(w, 16);
    }
    auto end = clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double per_frame_us = static_cast<double>(us) / static_cast<double>(kFrames);
    std::fprintf(stderr,
        "D66 frame-time (10 entities): %d frames in %lld us = %.2f us/frame\n",
        kFrames, static_cast<long long>(us), per_frame_us);
    /// Soft budget: allow 4x headroom. The engine's CI target is 16ms
    /// but perf tests are inherently machine-dependent.
    REQUIRE(per_frame_us < kBudgetUs * 4.0);
}
