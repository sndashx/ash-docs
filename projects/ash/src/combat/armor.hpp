#pragma once
/// Phase 07: armor definitions + database (Step 0702).
/// Damage reduction is summed per damage type across all equipped armor,
/// then capped at 75% (per spec). Encumbrance contributes to the
/// derived-stat speed penalty in `character::derived`.
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "combat/json_mini.hpp"
#include "combat/weapon.hpp"  // for DamageType

namespace ash {
namespace combat {

enum class ArmorSlot : std::uint8_t {
    Chest = 0,
    Helm  = 1,
    Boots = 2,
    Gloves = 3,
    Shield = 4,
    Count,
};

char const* armor_slot_name(ArmorSlot s) noexcept;

struct ArmorDef {
    std::uint64_t id{0};
    std::string   code_name{};
    std::string   display_name{};
    ArmorSlot     slot{ArmorSlot::Chest};
    /// Damage reduction by type. Negative values = weakness (vulnerable).
    int           dr_slash{0};
    int           dr_pierce{0};
    int           dr_crush{0};
    int           dr_element{0};
    int           dr_voice{0};
    /// Encumbrance: lbs added to carried weight. Heavy armor slows you.
    float         encumbrance{0.0f};
    int           condition_max{100};
    int           condition{100};
    std::vector<std::string> tags{};
};

/// Sum armor DR by damage type. Caps each type at `cap_pct` (default 75).
struct ArmorSummary {
    int slash{0};
    int pierce{0};
    int crush{0};
    int element{0};
    int voice{0};
};

/// Sum every piece in `pieces` into an `ArmorSummary`, capping each
/// value at 75 (Step 0702.00.03).
ArmorSummary summarize(std::vector<ArmorDef const*> const& pieces,
                       int cap_pct = 75) noexcept;

/// Lookup helper. `type` selects the field; out-of-range returns 0.
inline int const& summary_for(ArmorSummary const& s, DamageType t) noexcept {
    switch (t) {
        case DamageType::Slash:   return s.slash;
        case DamageType::Pierce:  return s.pierce;
        case DamageType::Crush:   return s.crush;
        case DamageType::Element: return s.element;
        case DamageType::Voice:   return s.voice;
        case DamageType::Count:   break;
    }
    return s.slash;
}

class ArmorDatabase {
public:
    void load_from_file(char const* path) noexcept;
    void load_from_json(json::Value const& root) noexcept;
    void add(ArmorDef def) noexcept;
    ArmorDef const* get(std::uint64_t id) const noexcept;
    ArmorDef const* get_by_code(std::string const& code) const noexcept;
    std::size_t size() const noexcept { return by_id_.size(); }
    std::vector<ArmorDef> const& all() const noexcept { return order_; }
private:
    std::unordered_map<std::uint64_t, std::size_t> by_id_{};
    std::unordered_map<std::string,  std::size_t> by_code_{};
    std::vector<ArmorDef>                        order_{};
};

}  // namespace combat
}  // namespace ash