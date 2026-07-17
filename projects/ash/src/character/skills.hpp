#pragma once
/// Phase 06: 24-skill system under the 9 attributes (Pillar 5).
/// Each skill is a single integer in [0, 1000]. Skills affect derived
/// stats and dialogue options. `gain_skill` applies diminishing returns
/// so per-level growth flattens as the skill climbs.
#include <array>
#include <cstdint>

#include "character/attributes.hpp"

namespace ash {
namespace character {

inline constexpr int kSkillMin     = 0;
inline constexpr int kSkillMax     = 1000;
inline constexpr int kSkillDefault = 5;

enum class Skill : std::uint8_t {
    Armorer = 0,
    Blunt,
    Blade,
    Marksman,
    Dodge,
    Thrown,
    Mysticism,
    Restoration,
    Warding,
    Alchemy,
    Enchant,
    Spellcraft,
    Stealth,
    Pickpocket,
    Security,
    Speechcraft,
    Mercantile,
    Illusion,
    Deception,
    Intimidation,
    Seduction,
    History,
    Theology,
    Linguistics,
    Count,
};

inline constexpr std::size_t kSkillCount = static_cast<std::size_t>(Skill::Count);

/// Stable, ordered set of skill IDs under one governing attribute.
/// Used by derived stats, by the character UI skill tab, and by skill
/// voices during dialogue.
struct SkillGroup {
    Attribute            governing{};
    std::array<Skill, 3> skills{};
};

/// Returns the governing attribute for a skill (e.g. Spellcraft -> Int).
Attribute skill_attribute(Skill s) noexcept;

/// 24 skills in display order, grouped by governing attribute (matches
/// the column layout in the character UI).
std::array<SkillGroup, 8> const& skill_groups() noexcept;

/// Stable, lower-case identifier used in save data.
constexpr char const* skill_id(Skill s) noexcept;
char const*          skill_name(Skill s) noexcept;

struct Skills {
    std::array<int, kSkillCount> values{};

    constexpr Skills() {
        values.fill(kSkillDefault);
    }

    int&       operator[](Skill s)       noexcept { return values[static_cast<std::size_t>(s)]; }
    int const& operator[](Skill s) const noexcept { return values[static_cast<std::size_t>(s)]; }

    void clamp() noexcept;

    /// Apply `amount` (positive or negative) with diminishing returns.
    /// The fraction of `amount` actually applied shrinks as the skill
    /// climbs: at skill 0 we apply 100%; at skill 1000 we apply ~10%.
    /// Formula: scale = 1 / (1 + skill/100), then new = clamp(skill + amount*scale).
    /// Returns the actual delta applied (may be 0 at the cap).
    int gain_skill(Skill s, int amount) noexcept;
};

}  // namespace character
}  // namespace ash
