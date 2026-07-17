#include "combat/weapon.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace ash {
namespace combat {

char const* weapon_type_name(WeaponType t) noexcept {
    switch (t) {
        case WeaponType::Melee1H:  return "melee_1h";
        case WeaponType::Melee2H:  return "melee_2h";
        case WeaponType::Polearm:  return "polearm";
        case WeaponType::Bow:      return "bow";
        case WeaponType::Crossbow: return "crossbow";
        case WeaponType::Firearm:  return "firearm";
        case WeaponType::Thrown:   return "thrown";
        case WeaponType::Count:    break;
    }
    return "unknown";
}

char const* damage_type_name(DamageType t) noexcept {
    switch (t) {
        case DamageType::Slash:   return "slash";
        case DamageType::Pierce:  return "pierce";
        case DamageType::Crush:   return "crush";
        case DamageType::Element: return "element";
        case DamageType::Voice:   return "voice";
        case DamageType::Count:   break;
    }
    return "unknown";
}

char const* ammo_type_name(AmmoType t) noexcept {
    switch (t) {
        case AmmoType::None:      return "none";
        case AmmoType::Arrow:     return "arrow";
        case AmmoType::Bolt:      return "bolt";
        case AmmoType::Bullet:    return "bullet";
        case AmmoType::Throwable: return "throwable";
        case AmmoType::Count:     break;
    }
    return "none";
}

DamageType damage_type_from_string(std::string const& s) noexcept {
    if (s == "slash")   return DamageType::Slash;
    if (s == "pierce")  return DamageType::Pierce;
    if (s == "crush")   return DamageType::Crush;
    if (s == "element") return DamageType::Element;
    if (s == "voice")   return DamageType::Voice;
    return DamageType::Slash;
}

namespace {

WeaponType weapon_type_from_string(std::string const& s) noexcept {
    if (s == "melee_1h")  return WeaponType::Melee1H;
    if (s == "melee_2h")  return WeaponType::Melee2H;
    if (s == "polearm")   return WeaponType::Polearm;
    if (s == "bow")       return WeaponType::Bow;
    if (s == "crossbow")  return WeaponType::Crossbow;
    if (s == "firearm")   return WeaponType::Firearm;
    if (s == "thrown")    return WeaponType::Thrown;
    return WeaponType::Melee1H;
}

AmmoType ammo_type_from_string(std::string const& s) noexcept {
    if (s == "arrow")     return AmmoType::Arrow;
    if (s == "bolt")      return AmmoType::Bolt;
    if (s == "bullet")    return AmmoType::Bullet;
    if (s == "throwable") return AmmoType::Throwable;
    return AmmoType::None;
}

int str_to_int(json::Value const* v, int fallback) noexcept {
    if (v == nullptr || !v->is_number()) return fallback;
    return v->as_int();
}

float str_to_float(json::Value const* v, float fallback) noexcept {
    if (v == nullptr || !v->is_number()) return fallback;
    return static_cast<float>(v->as_number());
}

std::string str_to_string(json::Value const* v, std::string const& fallback) noexcept {
    if (v == nullptr || !v->is_string()) return fallback;
    return v->as_string();
}

}  // namespace

void WeaponDatabase::load_from_file(char const* path) noexcept {
    if (path == nullptr) {
        return;
    }
    std::ifstream f(path);
    if (!f.is_open()) {
        return;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    try {
        json::Value root = json::parse(ss.str());
        load_from_json(root);
    } catch (std::exception const& e) {
        std::fprintf(stderr, "weapons.json parse error: %s\n", e.what());
    }
}

void WeaponDatabase::load_from_json(json::Value const& root) noexcept {
    if (!root.is_array()) {
        return;
    }
    for (auto const& v : root.as_array()) {
        if (!v.is_object()) continue;
        WeaponDef w{};
        json::Value const* id_v     = v.get("id");
        json::Value const* code_v   = v.get("code");
        json::Value const* name_v   = v.get("name");
        json::Value const* type_v   = v.get("type");
        json::Value const* dmg_v    = v.get("damage");
        json::Value const* min_v    = v.get("damage_min");
        json::Value const* max_v    = v.get("damage_max");
        json::Value const* arc_v    = v.get("swing_arc");
        json::Value const* reach_v  = v.get("reach");
        json::Value const* range_v  = v.get("range");
        json::Value const* ammo_v   = v.get("ammo");
        json::Value const* speed_v  = v.get("attack_speed_ms");
        json::Value const* weight_v = v.get("weight");
        json::Value const* cond_v   = v.get("condition");
        json::Value const* tags_v   = v.get("tags");

        w.code_name      = str_to_string(code_v, "");
        w.display_name   = str_to_string(name_v, w.code_name);
        w.type           = weapon_type_from_string(str_to_string(type_v, "melee_1h"));
        w.damage         = damage_type_from_string(str_to_string(dmg_v, "slash"));
        w.damage_min     = str_to_int(min_v, 1);
        w.damage_max     = str_to_int(max_v, 4);
        w.swing_arc_deg  = str_to_int(arc_v, 90);
        w.reach          = str_to_int(reach_v, 1);
        w.range          = str_to_int(range_v, 0);
        w.ammo           = ammo_type_from_string(str_to_string(ammo_v, "none"));
        w.attack_speed_ms = str_to_int(speed_v, 1500);
        w.weight         = str_to_float(weight_v, 1.0f);
        w.condition      = str_to_int(cond_v, 100);
        if (tags_v != nullptr && tags_v->is_array()) {
            for (auto const& t : tags_v->as_array()) {
                if (t.is_string()) {
                    w.tags.push_back(t.as_string());
                }
            }
        }
        if (id_v != nullptr && id_v->is_number()) {
            w.id = static_cast<std::uint64_t>(id_v->as_number());
        } else {
            /// Hash the code name to a stable id (FNV-1a 64-bit).
            std::uint64_t h = 0xCBF29CE484222325ULL;
            for (char c : w.code_name) {
                h ^= static_cast<std::uint64_t>(static_cast<std::uint8_t>(c));
                h *= 0x100000001B3ULL;
            }
            w.id = h;
        }
        add(std::move(w));
    }
}

void WeaponDatabase::add(WeaponDef def) noexcept {
    std::size_t const idx = order_.size();
    order_.push_back(std::move(def));
    auto const& d = order_.back();
    by_id_[d.id]      = idx;
    by_code_[d.code_name] = idx;
}

WeaponDef const* WeaponDatabase::get(std::uint64_t id) const noexcept {
    auto it = by_id_.find(id);
    if (it == by_id_.end()) return nullptr;
    return &order_[it->second];
}

WeaponDef const* WeaponDatabase::get_by_code(std::string const& code) const noexcept {
    auto it = by_code_.find(code);
    if (it == by_code_.end()) return nullptr;
    return &order_[it->second];
}

}  // namespace combat
}  // namespace ash