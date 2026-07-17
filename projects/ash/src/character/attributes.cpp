#include "character/attributes.hpp"

#include <algorithm>

namespace ash {
namespace character {

void Attributes::clamp() noexcept {
    for (auto& v : values) {
        v = std::clamp(v, kAttributeMin, kAttributeMax);
    }
}

constexpr char const* attribute_id(Attribute a) noexcept {
    switch (a) {
        case Attribute::Str: return "str";
        case Attribute::End: return "end";
        case Attribute::Agi: return "agi";
        case Attribute::Wil: return "wil";
        case Attribute::Int: return "int";
        case Attribute::Wit: return "wit";
        case Attribute::Cha: return "cha";
        case Attribute::Luc: return "luc";
        case Attribute::Voi: return "voi";
        case Attribute::Count: break;
    }
    return "unknown";
}

char const* attribute_name(Attribute a) noexcept {
    switch (a) {
        case Attribute::Str: return "Strength";
        case Attribute::End: return "Endurance";
        case Attribute::Agi: return "Agility";
        case Attribute::Wil: return "Willpower";
        case Attribute::Int: return "Intellect";
        case Attribute::Wit: return "Wit";
        case Attribute::Cha: return "Charisma";
        case Attribute::Luc: return "Luck";
        case Attribute::Voi: return "Voice";
        case Attribute::Count: break;
    }
    return "Unknown";
}

}  // namespace character
}  // namespace ash
