#include "combat/status.hpp"

#include <algorithm>

namespace ash {
namespace combat {

void apply_bleeding(world::Entity& target, int magnitude,
                    int duration_ms, std::uint64_t source) noexcept {
    if (!target.has_combatant) return;
    target.combatant.conditions.apply(
        character::Condition::Bleeding, duration_ms, magnitude, source);
}

void apply_stunned(world::Entity& target, int duration_ms,
                   std::uint64_t source) noexcept {
    if (!target.has_combatant) return;
    target.combatant.conditions.apply(
        character::Condition::Stunned, duration_ms, 1, source);
}

void apply_poisoned(world::Entity& target, int magnitude,
                    int duration_ms, std::uint64_t source) noexcept {
    if (!target.has_combatant) return;
    target.combatant.conditions.apply(
        character::Condition::Poisoned, duration_ms, magnitude, source);
}

void apply_slowed(world::Entity& target, int duration_ms,
                  std::uint64_t source) noexcept {
    if (!target.has_combatant) return;
    target.combatant.conditions.apply(
        character::Condition::Slowed, duration_ms, 1, source);
}

int tick_conditions(world::Entity& target, int dt_ms) noexcept {
    if (!target.has_combatant) return 0;
    int expired = target.combatant.conditions.tick(dt_ms);
    if (auto* bleed = target.combatant.conditions.find(character::Condition::Bleeding);
        bleed != nullptr) {
        target.stats.hp -= bleed->magnitude;
        if (target.stats.hp <= 0) {
            target.stats.hp = 0;
            target.alive = false;
        }
    }
    if (auto* psn = target.combatant.conditions.find(character::Condition::Poisoned);
        psn != nullptr) {
        int dmg = std::max(1, psn->magnitude / 4);
        target.stats.hp -= dmg;
        if (target.stats.hp <= 0) {
            target.stats.hp = 0;
            target.alive = false;
        }
    }
    if (auto* burn = target.combatant.conditions.find(character::Condition::Burning);
        burn != nullptr) {
        target.stats.hp -= burn->magnitude;
        if (target.stats.hp <= 0) {
            target.stats.hp = 0;
            target.alive = false;
        }
    }
    return expired;
}

}  // namespace combat
}  // namespace ash