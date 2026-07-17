#include "combat/combat.hpp"

#include <algorithm>

#include "combat/status.hpp"

namespace ash {
namespace combat {

namespace {

int default_rng(int lo, int hi, void*) {
    if (lo >= hi) return lo;
    static std::uint64_t s = 0xC0FFEEBABE123456ULL;
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    int span = hi - lo + 1;
    return lo + static_cast<int>((s >> 33) % static_cast<std::uint64_t>(span));
}

int attr_mod(world::Entity const& e, character::Attribute a) noexcept {
    if (!e.has_combatant) return 0;
    return e.combatant.attrs.modifier(a);
}

}  // namespace

void CombatManager::begin_combat(world::World& w,
                                 std::vector<core::EntityId> participants) noexcept {
    state_ = CombatState::InCombat;
    round_ = CombatRound{};
    round_.round_number = 1;
    ms_into_round_ = 0;
    total_ticks_ms_ = 0;
    economies_.clear();

    RngFn rng = rng_ ? rng_ : &default_rng;
    void* rng_user = rng_user_;

    std::vector<Initiative> rolls;
    rolls.reserve(participants.size());
    for (auto id : participants) {
        auto* e = w.find(id);
        if (e == nullptr || !e->alive) continue;
        int d20 = rng(1, 20, rng_user);
        int init = d20 + attr_mod(*e, character::Attribute::Wit) +
                   (e->has_combatant ? (e->combatant.attrs[character::Attribute::Luc] / 4) : 0);
        rolls.push_back(Initiative{id, init});
    }
    std::sort(rolls.begin(), rolls.end(), [](Initiative const& a, Initiative const& b) {
        if (a.init_roll != b.init_roll) return a.init_roll > b.init_roll;
        return a.id.value < b.id.value;
    });
    for (auto& r : rolls) {
        round_.initiative_order.push_back(r.id);
        economies_.push_back(ActionEconomy{});
    }
    round_.current_actor_idx = 0;
}

void CombatManager::end_combat() noexcept {
    state_ = CombatState::OutOfCombat;
    round_ = CombatRound{};
    economies_.clear();
}

void CombatManager::pause() noexcept {
    if (state_ == CombatState::InCombat) {
        state_ = CombatState::Paused;
    }
}

void CombatManager::resume() noexcept {
    if (state_ == CombatState::Paused) {
        state_ = CombatState::InCombat;
    }
}

void CombatManager::tick(world::World& w, int dt_ms) noexcept {
    if (state_ == CombatState::OutOfCombat) return;
    total_ticks_ms_ += static_cast<std::uint64_t>(dt_ms);
    for (auto& e : w.entities) {
        if (!e.has_ai || !e.alive) continue;
        AiContext ctx{w, e.id, dt_ms};
        ai_tick(ctx);
    }
    if (state_ == CombatState::Paused) return;

    ms_into_round_ += dt_ms;
    if (ms_into_round_ >= kRoundMs) {
        ms_into_round_ -= kRoundMs;
        round_.round_number += 1;
        for (auto& eco : economies_) eco.reset();
        for (auto& e : w.entities) {
            if (e.alive) tick_conditions(e, dt_ms);
        }
    }
}

bool CombatManager::should_end_combat(world::World const& w) const noexcept {
    if (state_ == CombatState::OutOfCombat) return true;
    int alive_count = 0;
    for (auto const& id : round_.initiative_order) {
        auto const* e = w.find(id);
        if (e != nullptr && e->alive) ++alive_count;
    }
    return alive_count <= 1;
}

}  // namespace combat
}  // namespace ash