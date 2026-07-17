#include "combat/attack.hpp"

#include <algorithm>
#include <cmath>

#include "world/query.hpp"

namespace ash {
namespace combat {

namespace {

int default_rng(int lo, int hi, void* /*user*/) {
    if (lo >= hi) return lo;
    static std::uint64_t s = 0xDEADBEEF12345678ULL;
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    int span = hi - lo + 1;
    return lo + static_cast<int>((s >> 33) % static_cast<std::uint64_t>(span));
}

int attr_mod(world::Entity const& e, character::Attribute a) noexcept {
    if (!e.has_combatant) return 0;
    return e.combatant.attrs.modifier(a);
}

int skill_level(world::Entity const& e, character::Skill s) noexcept {
    if (!e.has_combatant) return 0;
    return e.combatant.skills[s];
}

character::Attribute governing_attr_for(WeaponDef const& w) noexcept {
    if (w.is_ranged()) return character::Attribute::Agi;
    if (w.damage == DamageType::Voice) return character::Attribute::Luc;
    return character::Attribute::Str;
}

character::Skill governing_skill_for(WeaponDef const& w) noexcept {
    if (w.is_ranged()) {
        if (w.type == WeaponType::Thrown) return character::Skill::Thrown;
        return character::Skill::Marksman;
    }
    if (w.damage == DamageType::Pierce) return character::Skill::Blade;
    if (w.damage == DamageType::Crush)  return character::Skill::Blunt;
    return character::Skill::Blade;
}

int armor_dr_for(world::Entity const& defender, DamageType type) noexcept {
    if (!defender.has_combatant) return 0;
    switch (type) {
        case DamageType::Slash:   return defender.combatant.armor_dr_slash;
        case DamageType::Pierce:  return defender.combatant.armor_dr_pierce;
        case DamageType::Crush:   return defender.combatant.armor_dr_crush;
        case DamageType::Element: return defender.combatant.armor_dr_element;
        case DamageType::Voice:   return defender.combatant.armor_dr_voice;
        case DamageType::Count:   break;
    }
    return 0;
}

}  // namespace

bool in_swing_arc(core::IVec2 attacker_cell,
                  core::IVec2 target_cell,
                  world::Facing attacker_facing,
                  int swing_arc_deg) noexcept {
    if (swing_arc_deg >= 360) return true;
    int dx = target_cell.x - attacker_cell.x;
    int dy = target_cell.y - attacker_cell.y;
    float mag = std::sqrt(static_cast<float>(dx * dx + dy * dy));
    if (mag < 0.0001f) return true;
    auto fv = world::facing_vec(attacker_facing);
    float fx = static_cast<float>(fv.x);
    float fy = static_cast<float>(fv.y);
    float tx = static_cast<float>(dx) / mag;
    float ty = static_cast<float>(dy) / mag;
    float dot = fx * tx + fy * ty;
    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;
    float angle = std::acos(dot);
    float deg = angle * (180.0f / 3.14159265f);
    return deg <= static_cast<float>(swing_arc_deg) * 0.5f;
}

AttackResult attack_resolve(world::Entity const& attacker,
                            world::Entity&      defender,
                            WeaponDef const*    weapon,
                            Cover               cover,
                            RngFn               rng,
                            void*               rng_user) noexcept {
    AttackResult r{};
    if (!attacker.alive || !defender.alive) {
        r.message = "dead";
        return r;
    }
    if (rng == nullptr) {
        rng = g_attack_rng ? g_attack_rng : &default_rng;
    }
    if (rng_user == nullptr) {
        rng_user = g_attack_rng_user;
    }

    /// Full cover means the defender is behind a solid wall — there is
    /// no line of sight. No ranged or melee attack can hit through a
    /// full-cover barrier, even on a natural 20.
    if (cover == Cover::Full) {
        r.message = "full_cover";
        return r;
    }

    int swing_arc = 90;
    int weapon_reach = 1;
    int weapon_range = 0;
    DamageType dmg_type = DamageType::Crush;
    int dmg_min = 1;
    int dmg_max = 3;

    if (weapon != nullptr) {
        swing_arc = weapon->swing_arc_deg;
        weapon_reach = weapon->reach;
        weapon_range = weapon->range;
        dmg_type = weapon->damage;
        dmg_min = weapon->damage_min;
        dmg_max = weapon->damage_max;
    }

    int cheb = world::chebyshev(
        attacker.has_position ? attacker.position.cell : core::IVec2{},
        defender.has_position ? defender.position.cell : core::IVec2{});
    if (weapon == nullptr || weapon->is_melee()) {
        if (cheb > weapon_reach) {
            r.message = "out_of_range";
            return r;
        }
        if (!in_swing_arc(attacker.position.cell, defender.position.cell,
                          attacker.position.facing, swing_arc)) {
            r.message = "out_of_arc";
            return r;
        }
    } else {
        if (weapon_range > 0 && cheb > weapon_range) {
            r.message = "out_of_range";
            return r;
        }
    }

    int d20 = rng(1, 20, rng_user);
    int attack_bonus = 0;
    if (weapon != nullptr) {
        auto sk = governing_skill_for(*weapon);
        int sk_lvl = skill_level(attacker, sk);
        attack_bonus += sk_lvl / 10;
        attack_bonus += attr_mod(attacker, governing_attr_for(*weapon));
    } else {
        attack_bonus += attr_mod(attacker, character::Attribute::Str);
    }

    int cover_pen = cover_to_hit_penalty(cover);
    int defense = 0;
    if (defender.has_combatant) {
        defense += defender.combatant.dodge;
        defense += attr_mod(defender, character::Attribute::Agi);
    }

    int roll = d20 + attack_bonus;
    bool crit = false;
    if (d20 == 20) {
        crit = true;
    } else {
        int crit_threshold = 0;
        if (attacker.has_combatant) {
            crit_threshold = attacker.combatant.attrs.crit_chance_pct();
        }
        if (crit_threshold > 0 && rng(1, 100, rng_user) <= crit_threshold) {
            crit = true;
        }
    }
    if (!crit && roll - cover_pen <= defense) {
        r.message = "miss";
        return r;
    }
    r.hit = true;
    r.crit = crit;
    r.type = dmg_type;
    r.cover = cover;

    int base_damage = rng(dmg_min, dmg_max, rng_user);
    if (crit) base_damage *= 2;
    int dr = armor_dr_for(defender, dmg_type);
    if (dr < 0)   dr = 0;
    if (dr > 75)  dr = 75;
    int final_damage = (base_damage * (100 - dr)) / 100;
    if (final_damage < 1) final_damage = 1;
    r.damage = final_damage;
    r.message = crit ? "crit" : "hit";
    return r;
}

}  // namespace combat
}  // namespace ash
