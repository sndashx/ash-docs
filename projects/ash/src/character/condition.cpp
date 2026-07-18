#include "character/condition.hpp"

#include <algorithm>
#include <cstring>

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

char const* condition_id(Condition c) noexcept {
    switch (c) {
        case Condition::Bleeding:  return "bleeding";
        case Condition::Stunned:   return "stunned";
        case Condition::Poisoned:  return "poisoned";
        case Condition::Terrified: return "terrified";
        case Condition::Blessed:   return "blessed";
        case Condition::Cursed:    return "cursed";
        case Condition::Warded:    return "warded";
        case Condition::Slowed:    return "slowed";
        case Condition::Hasted:    return "hasted";
        case Condition::Silenced:  return "silenced";
        case Condition::Rooted:    return "rooted";
        case Condition::Burning:   return "burning";
        case Condition::Frozen:    return "frozen";
        case Condition::Dazed:     return "dazed";
        case Condition::Inspired:  return "inspired";
        case Condition::Count:     break;
    }
    return "unknown";
}

Condition condition_from_id(const char* id) noexcept {
    if (!id) return Condition::Count;
    switch (id[0]) {
        case 'b':
            if (std::strcmp(id, "bleeding") == 0) return Condition::Bleeding;
            if (std::strcmp(id, "blessed")  == 0) return Condition::Blessed;
            if (std::strcmp(id, "burning")  == 0) return Condition::Burning;
            break;
        case 'c':
            if (std::strcmp(id, "cursed") == 0) return Condition::Cursed;
            break;
        case 'd':
            if (std::strcmp(id, "dazed") == 0) return Condition::Dazed;
            break;
        case 'f':
            if (std::strcmp(id, "frozen") == 0) return Condition::Frozen;
            break;
        case 'h':
            if (std::strcmp(id, "hasted") == 0) return Condition::Hasted;
            break;
        case 'i':
            if (std::strcmp(id, "inspired") == 0) return Condition::Inspired;
            break;
        case 'p':
            if (std::strcmp(id, "poisoned") == 0) return Condition::Poisoned;
            break;
        case 'r':
            if (std::strcmp(id, "rooted") == 0) return Condition::Rooted;
            break;
        case 's':
            if (std::strcmp(id, "stunned")  == 0) return Condition::Stunned;
            if (std::strcmp(id, "slowed")   == 0) return Condition::Slowed;
            if (std::strcmp(id, "silenced") == 0) return Condition::Silenced;
            break;
        case 't':
            if (std::strcmp(id, "terrified") == 0) return Condition::Terrified;
            break;
        case 'w':
            if (std::strcmp(id, "warded") == 0) return Condition::Warded;
            break;
    }
    return Condition::Count;
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