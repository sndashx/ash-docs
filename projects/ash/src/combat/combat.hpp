#pragma once
/// Phase 07: combat round manager (Step 0700).
///
/// Owns the combat state machine:
///   OutOfCombat -> InCombat -> (Paused) -> InCombat -> OutOfCombat
///
/// On `begin_combat()`:
///   * Roll initiative for every living combatant:
///         init = Wit_mod + Luc/4 + d20
///     Tie-break by Luc, then by EntityId.
///   * Build `initiative_order` and reset the round counter.
///
/// On each `tick(dt_ms)`:
///   * If Paused: time stops. AI still ticks (so the user can preview
///     intent), but no attacks fire.
///   * Otherwise advance the round timer; when a round elapses, refill
///     every actor's Move/Action/Bonus pool and decrement condition
///     durations. Drive the active actor's AI.
///
/// `pause`/`resume` toggle the pause state. The intended UI binding is
/// the spacebar.
#include <cstdint>
#include <vector>

#include "combat/ai.hpp"
#include "combat/path.hpp"
#include "core/ids.hpp"
#include "world/world.hpp"

namespace ash {
namespace combat {

enum class CombatState : std::uint8_t {
    OutOfCombat = 0,
    InCombat    = 1,
    Paused      = 2,
};

/// One initiative entry.
struct Initiative {
    core::EntityId id{};
    int            init_roll{0};
};

/// One combat round.
struct CombatRound {
    int                       round_number{0};
    std::vector<core::EntityId> initiative_order{};
    int                       current_actor_idx{0};
};

/// One actor's per-round action economy.
struct ActionEconomy {
    bool move_used{false};
    bool action_used{false};
    bool bonus_used{false};
    void reset() noexcept { move_used = action_used = bonus_used = false; }
};

class CombatManager {
public:
    /// Begin combat. `participants` is the set of EntityIds that should
    /// join the fight. The manager rolls initiative, sorts the order,
    /// and resets the round counter to 1.
    void begin_combat(world::World& w, std::vector<core::EntityId> participants) noexcept;

    /// End combat (reset state). Called when no enemies remain.
    void end_combat() noexcept;

    /// Toggle pause.
    void pause() noexcept;
    void resume() noexcept;
    bool is_paused() const noexcept { return state_ == CombatState::Paused; }
    CombatState state() const noexcept { return state_; }

    /// Run one simulation tick. `dt_ms` is wall-clock milliseconds.
    /// When paused, AI ticks only (the actor does not actually act).
    void tick(world::World& w, int dt_ms) noexcept;

    /// Returns true if combat should end this tick (e.g. all enemies
    /// dead or fled).
    bool should_end_combat(world::World const& w) const noexcept;

    /// Read-only access to current state.
    CombatRound const& round() const noexcept { return round_; }
    std::vector<core::EntityId> const& order() const noexcept { return round_.initiative_order; }
    int round_number() const noexcept { return round_.round_number; }
    std::uint64_t total_ticks_ms() const noexcept { return total_ticks_ms_; }

    /// The path cache used by AI movement. Public so tests can introspect
    /// cache hits.
    std::unordered_map<PathCacheKey, std::vector<core::IVec2>, PathCacheKeyHash>&
    path_cache() noexcept { return path_cache_; }

    /// RNG for initiative rolls. Defaults to deterministic until set.
    using RngFn = int (*)(int lo, int hi, void* user);
    void set_rng(RngFn rng, void* user) noexcept {
        rng_ = rng; rng_user_ = user;
    }

private:
    CombatState state_{CombatState::OutOfCombat};
    CombatRound round_{};
    std::vector<ActionEconomy> economies_{};
    int               ms_into_round_{0};
    std::uint64_t     total_ticks_ms_{0};
    RngFn             rng_{nullptr};
    void*             rng_user_{nullptr};
    std::unordered_map<PathCacheKey, std::vector<core::IVec2>, PathCacheKeyHash>
        path_cache_{};
};

/// Length of one combat round in real-world milliseconds.
inline constexpr int kRoundMs = 3000;

}  // namespace combat
}  // namespace ash
