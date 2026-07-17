#include "character/condition.hpp"

#include <algorithm>

namespace ash {
namespace character {

char const* condition_name(Condition c) noexcept {
    switch (c) {
        case Condition::Bleeding:  return "Bleeding";
        case Condition::Stunned:   return "Stunned";
        case Condition::Poisoned:  return "Poisoned";
        case Condition::Terrified: return "Terrified";
        case Condition::Blessed:   return "Blessed";
        case Condition::Cursed:    return "Cursed";
        case Condition::Warded:    return "Warded";
        case Condition::Slowed:    return "Slowed";
        case Condition::Hasted:    return "Hasted";
        case Condition::Silenced:  return "Silenced";
        case Condition::Rooted:    return "Rooted";
        case Condition::Burning:   return "Burning";
        case Condition::Frozen:    return "Frozen";
        case Condition::Dazed:     return "Dazed";
        case Condition::Inspired:  return "Inspired";
        case Condition::Count:     break;
    }
    return "Unknown";
}

bool ConditionList::has(Condition c) const noexcept {
    return find(c) != nullptr;
}

ActiveCondition const* ConditionList::find(Condition c) const noexcept {
    ActiveCondition const* best = nullptr;
    for (auto const& ac : active) {
        if (ac.type == c && (best == nullptr || ac.magnitude > best->magnitude)) {
            best = &ac;
        }
    }
    return best;
}

void ConditionList::apply(Condition c, int duration_ms, int magnitude, std::uint64_t source) noexcept {
    /// Refresh or add. We always take the longer duration and the larger
    /// magnitude — common behavior across status systems. This keeps
    /// "re-applying" idempotent without dropping the strongest version.
    for (auto& ac : active) {
        if (ac.type != c) {
            continue;
        }
        if (duration_ms > ac.duration_ms) {
            ac.duration_ms = duration_ms;
        }
        if (magnitude > ac.magnitude) {
            ac.magnitude = magnitude;
        }
        if (source != 0) {
            ac.source = source;
        }
        return;
    }
    active.push_back(ActiveCondition{c, duration_ms, magnitude, source});
}

int ConditionList::clear(Condition c) noexcept {
    int removed = 0;
    auto it = active.begin();
    while (it != active.end()) {
        if (it->type == c) {
            it = active.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

int ConditionList::tick(int dt_ms) noexcept {
    if (dt_ms <= 0) {
        return 0;
    }
    int expired = 0;
    auto it = active.begin();
    while (it != active.end()) {
        it->duration_ms -= dt_ms;
        if (it->duration_ms <= 0) {
            it = active.erase(it);
            ++expired;
        } else {
            ++it;
        }
    }
    return expired;
}

ConditionMod ConditionList::mod() const noexcept {
    ConditionMod m{};
    /// Walk every active condition once. Multiplicative effects are
    /// composed by repeated multiplication (so multiple slows stack).
    /// Additive effects use the largest magnitude seen so the strongest
    /// source wins.
    for (auto const& ac : active) {
        switch (ac.type) {
            case Condition::Bleeding:  m.hp_max_mult *= 0.75f; break;
            case Condition::Poisoned:  m.hp_max_mult *= 0.90f; break;
            case Condition::Cursed:    m.hp_max_mult *= 0.85f; break;
            case Condition::Blessed:   m.hp_max_mult *= 1.10f; break;
            case Condition::Warded:    m.vp_max_mult *= 1.25f; break;
            case Condition::Silenced:  m.vp_max_mult *= 0.50f; break;
            case Condition::Terrified: m.sp_max_mult *= 0.75f; break;
            case Condition::Dazed:     m.sp_max_mult *= 0.50f; break;
            case Condition::Slowed:    m.speed_mult *= 0.50f; break;
            case Condition::Hasted:    m.speed_mult *= 1.50f; break;
            case Condition::Stunned:   m.speed_add -= 1000;   break;
            case Condition::Rooted:    m.speed_add -= 1000;   break;
            case Condition::Burning:   /* DoT handled in tick(); no stat mod here. */ break;
            case Condition::Frozen:    m.speed_mult *= 0.25f; break;
            case Condition::Inspired:  m.crit_add_pct = std::max(m.crit_add_pct, 10); break;
            case Condition::Count:     break;
        }
    }
    return m;
}

}  // namespace character
}  // namespace ash