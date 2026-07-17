#include "character/skills.hpp"

#include <algorithm>

namespace ash {
namespace character {

namespace {

/// Display name tables. These are used by the character UI, save data,
/// debug dumps, and the `--char-test` smoke test.
struct SkillName {
    Skill        id;
    char const*  name;
    Attribute    governing;
};

constexpr SkillName kSkillNames[] = {
    {Skill::Armorer,      "Armorer",       Attribute::Str},
    {Skill::Blunt,        "Blunt",         Attribute::Str},
    {Skill::Blade,        "Blade",         Attribute::Str},
    {Skill::Marksman,     "Marksman",      Attribute::Agi},
    {Skill::Dodge,        "Dodge",         Attribute::Agi},
    {Skill::Thrown,       "Thrown",        Attribute::Agi},
    {Skill::Mysticism,    "Mysticism",     Attribute::Wil},
    {Skill::Restoration,  "Restoration",   Attribute::Wil},
    {Skill::Warding,      "Warding",       Attribute::Wil},
    {Skill::Alchemy,      "Alchemy",       Attribute::Int},
    {Skill::Enchant,      "Enchant",       Attribute::Int},
    {Skill::Spellcraft,   "Spellcraft",    Attribute::Int},
    {Skill::Stealth,      "Stealth",       Attribute::Agi},
    {Skill::Pickpocket,   "Pickpocket",    Attribute::Agi},
    {Skill::Security,     "Security",      Attribute::Agi},
    {Skill::Speechcraft,  "Speechcraft",   Attribute::Cha},
    {Skill::Mercantile,   "Mercantile",    Attribute::Cha},
    {Skill::Illusion,     "Illusion",      Attribute::Cha},
    {Skill::Deception,    "Deception",     Attribute::Wit},
    {Skill::Intimidation, "Intimidation",  Attribute::Wit},
    {Skill::Seduction,    "Seduction",     Attribute::Wit},
    {Skill::History,      "History",       Attribute::Int},
    {Skill::Theology,     "Theology",      Attribute::Int},
    {Skill::Linguistics,  "Linguistics",   Attribute::Int},
};

constexpr int kSkillNameCount = static_cast<int>(sizeof(kSkillNames) / sizeof(kSkillNames[0]));

SkillName const& lookup(Skill s) noexcept {
    auto idx = static_cast<int>(s);
    if (idx < 0 || idx >= kSkillNameCount) {
        idx = 0;
    }
    return kSkillNames[idx];
}

}  // namespace

Attribute skill_attribute(Skill s) noexcept {
    return lookup(s).governing;
}

std::array<SkillGroup, 8> const& skill_groups() noexcept {
    static std::array<SkillGroup, 8> const groups = {{
        {Attribute::Str, {{Skill::Armorer, Skill::Blunt,     Skill::Blade}}},
        {Attribute::Agi, {{Skill::Marksman, Skill::Dodge,     Skill::Thrown}}},
        {Attribute::Wil, {{Skill::Mysticism, Skill::Restoration, Skill::Warding}}},
        {Attribute::Int, {{Skill::Alchemy,  Skill::Enchant,   Skill::Spellcraft}}},
        {Attribute::Agi, {{Skill::Stealth,  Skill::Pickpocket, Skill::Security}}},
        {Attribute::Cha, {{Skill::Speechcraft, Skill::Mercantile, Skill::Illusion}}},
        {Attribute::Wit, {{Skill::Deception, Skill::Intimidation, Skill::Seduction}}},
        {Attribute::Int, {{Skill::History,  Skill::Theology,   Skill::Linguistics}}},
    }};
    return groups;
}

constexpr char const* skill_id(Skill s) noexcept {
    switch (s) {
        case Skill::Armorer:      return "armorer";
        case Skill::Blunt:        return "blunt";
        case Skill::Blade:        return "blade";
        case Skill::Marksman:     return "marksman";
        case Skill::Dodge:        return "dodge";
        case Skill::Thrown:       return "thrown";
        case Skill::Mysticism:    return "mysticism";
        case Skill::Restoration:  return "restoration";
        case Skill::Warding:      return "warding";
        case Skill::Alchemy:      return "alchemy";
        case Skill::Enchant:      return "enchant";
        case Skill::Spellcraft:   return "spellcraft";
        case Skill::Stealth:      return "stealth";
        case Skill::Pickpocket:   return "pickpocket";
        case Skill::Security:     return "security";
        case Skill::Speechcraft:  return "speechcraft";
        case Skill::Mercantile:   return "mercantile";
        case Skill::Illusion:     return "illusion";
        case Skill::Deception:    return "deception";
        case Skill::Intimidation: return "intimidation";
        case Skill::Seduction:    return "seduction";
        case Skill::History:      return "history";
        case Skill::Theology:     return "theology";
        case Skill::Linguistics:  return "linguistics";
        case Skill::Count:        break;
    }
    return "unknown";
}

char const* skill_name(Skill s) noexcept {
    return lookup(s).name;
}

void Skills::clamp() noexcept {
    for (auto& v : values) {
        v = std::clamp(v, kSkillMin, kSkillMax);
    }
}

int Skills::gain_skill(Skill s, int amount) noexcept {
    if (amount == 0) {
        return 0;
    }
    auto& v = values[static_cast<std::size_t>(s)];
    /// Diminishing returns: scale = 1 / (1 + skill/100).
    /// Skill 0: 1.00x, Skill 100: 0.50x, Skill 500: 0.17x, Skill 1000: 0.09x.
    int const current = v;
    int const scale_num = 100;
    int const scale_den = scale_num + std::max(0, current);
    int const scaled   = (amount * scale_num) / scale_den;
    int const target   = current + scaled;
    int const clamped  = std::clamp(target, kSkillMin, kSkillMax);
    int const delta    = clamped - current;
    v = clamped;
    return delta;
}

}  // namespace character
}  // namespace ash
