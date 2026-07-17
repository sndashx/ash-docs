#include "character/derived.hpp"

#include <algorithm>

namespace ash {
namespace character {

DerivedStats recompute(Attributes const& a,
                       Skills const&     s,
                       Inventory const&  inv,
                       ConditionMod      mod) noexcept {
    DerivedStats out{};

    int const end  = a[Attribute::End];
    int const voi  = a[Attribute::Voi];
    int const agi  = a[Attribute::Agi];
    int const cha  = a[Attribute::Cha];
    int const intl = a[Attribute::Int];
    int const luc  = a[Attribute::Luc];
    int const str  = a[Attribute::Str];

    /// Per spec: HP_max = End*10 + (skill_blunt + skill_blade + skill_armorer)/4.
    int const weapon_skills = s[Skill::Blunt] + s[Skill::Blade] + s[Skill::Armorer];
    int hp_base = end * 10 + weapon_skills / 4;
    out.hp_max  = static_cast<int>(static_cast<float>(hp_base) * mod.hp_max_mult);
    out.hp_max  = std::max(0, out.hp_max);

    /// VP_max = Voi*8 + skill_mysticism/2.
    int const vp_base = voi * 8 + s[Skill::Mysticism] / 2;
    out.vp_max  = static_cast<int>(static_cast<float>(vp_base) * mod.vp_max_mult);
    out.vp_max  = std::max(0, out.vp_max);

    /// SP_max = End*6 + skill_armorer/3.
    int const sp_base = end * 6 + s[Skill::Armorer] / 3;
    out.sp_max  = static_cast<int>(static_cast<float>(sp_base) * mod.sp_max_mult);
    out.sp_max  = std::max(0, out.sp_max);

    /// Carry = Str*5.
    out.carry_capacity = str * 5;

    /// Speed = Agi*10, modified by conditions.
    int speed = agi * 10;
    speed     = static_cast<int>(static_cast<float>(speed) * mod.speed_mult) + mod.speed_add;
    out.speed_cells_per_sec = std::max(0, speed);

    /// Crit% = Luc/4, additive condition bonus.
    out.crit_chance_pct = luc / 4 + mod.crit_add_pct;
    if (out.crit_chance_pct < 0)   { out.crit_chance_pct = 0; }
    if (out.crit_chance_pct > 95) { out.crit_chance_pct = 95; }

    /// Barter = 1 - (Cha + skill_mercantile)/200, clamped to [0.0, 1.0].
    int const barter_num = cha + s[Skill::Mercantile];
    float   b = 1.0f - static_cast<float>(barter_num) / 200.0f;
    if (b < 0.0f) { b = 0.0f; }
    if (b > 1.0f) { b = 1.0f; }
    out.barter_discount = b;

    out.identify_skill = intl + s[Skill::Spellcraft];
    out.pick_skill     = agi  + s[Skill::Security];
    out.persuade_skill = cha  + s[Skill::Speechcraft];

    /// Encumbrance: weight > 0.8 * carry_capacity → speed halved.
    out.encumbered = (inv.current_weight > 0.8f * static_cast<float>(out.carry_capacity));
    if (out.encumbered) {
        out.speed_cells_per_sec /= 2;
    }

    return out;
}

}  // namespace character
}  // namespace ash