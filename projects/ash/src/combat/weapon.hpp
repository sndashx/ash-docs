#pragma once
/// Phase 07: weapon definitions + database (Step 0701).
/// Weapons are loaded from `content/items/weapons.json`. Each weapon has
/// damage range, swing arc (90 deg default), reach (cells), range (for
/// ranged), ammo type, attack speed in ms, and free-form tags.
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "combat/json_mini.hpp"

namespace ash {
namespace combat {

enum class WeaponType : std::uint8_t {
    Melee1H  = 0,
    Melee2H  = 1,
    Polearm  = 2,
    Bow      = 3,
    Crossbow = 4,
    Firearm  = 5,
    Thrown   = 6,
    Count,
};

enum class DamageType : std::uint8_t {
    Slash   = 0,
    Pierce  = 1,
    Crush   = 2,
    Element = 3,
    Voice   = 4,
    Count,
};

enum class AmmoType : std::uint8_t {
    None      = 0,
    Arrow     = 1,
    Bolt      = 2,
    Bullet    = 3,
    Throwable = 4,
    Count,
};

char const* weapon_type_name(WeaponType t) noexcept;
char const* damage_type_name(DamageType t) noexcept;
char const* ammo_type_name(AmmoType t) noexcept;
DamageType  damage_type_from_string(std::string const& s) noexcept;

struct WeaponDef {
    std::uint64_t    id{0};            /// ItemId.value
    std::string      code_name{};      /// "iron_cleaver"
    std::string      display_name{};
    WeaponType       type{WeaponType::Melee1H};
    DamageType       damage{DamageType::Slash};
    int              damage_min{1};
    int              damage_max{4};
    /// Swing arc in degrees. 360 = omnidirectional (whips), 90 = default.
    int              swing_arc_deg{90};
    /// Melee reach in cells (1 = adjacent).
    int              reach{1};
    /// Ranged range in cells.
    int              range{0};
    AmmoType         ammo{AmmoType::None};
    /// Time between attacks in ms. Drives the bonus-action economy.
    int              attack_speed_ms{1500};
    /// "weight" is lbs in the spec; kept as float for encumbrance math.
    float            weight{1.0f};
    /// Weapon condition 0..100 (100 = pristine).
    int              condition{100};
    /// Free-form tags: "two_handed", "reach", "throwable", "stamina_drain".
    std::vector<std::string> tags{};

    /// True if this weapon is a melee weapon (uses reach + swing arc).
    bool is_melee() const noexcept {
        return type == WeaponType::Melee1H || type == WeaponType::Melee2H ||
               type == WeaponType::Polearm;
    }
    /// True if this weapon is a ranged weapon (uses range + LoS).
    bool is_ranged() const noexcept {
        return type == WeaponType::Bow || type == WeaponType::Crossbow ||
               type == WeaponType::Firearm || type == WeaponType::Thrown;
    }
};

/// Loaded weapon database. Lookup by item id or by code name. Content
/// loading is optional; an empty DB returns "not found" gracefully so
/// tests can run without the JSON.
class WeaponDatabase {
public:
    /// Load weapons from `content/items/weapons.json`. Missing file is
    /// not an error (Phase 7 allows an empty weapon table for tests).
    void load_from_file(char const* path) noexcept;

    /// Load from a parsed JSON value (array of weapon objects).
    void load_from_json(json::Value const& root) noexcept;

    /// Insert a weapon definition manually (used by tests and the demo).
    void add(WeaponDef def) noexcept;

    /// Lookup by id. Returns nullptr if missing.
    WeaponDef const* get(std::uint64_t id) const noexcept;
    /// Lookup by code name. Returns nullptr if missing.
    WeaponDef const* get_by_code(std::string const& code) const noexcept;

    /// Total weapon count (after all loads).
    std::size_t size() const noexcept { return by_id_.size(); }

    /// Iterate over every weapon, in load order.
    std::vector<WeaponDef> const& all() const noexcept { return order_; }

private:
    std::unordered_map<std::uint64_t, std::size_t> by_id_{};
    std::unordered_map<std::string,  std::size_t> by_code_{};
    std::vector<WeaponDef>                        order_{};
};

}  // namespace combat
}  // namespace ash
