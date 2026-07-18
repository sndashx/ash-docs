#pragma once
/// Phase 06: 9-attribute character sheet (Pillar 5).
/// Each attribute is an integer in [1, 100]. The starting value is 40
/// (per spec; tuned at character creation). Derives `carry_capacity`,
/// `crit_chance_pct`, and the Morrowind-style `modifier` used everywhere
/// downstream (combat, dialogue, etc.).
#include <array>
#include <cstdint>

namespace ash {
namespace character {

inline constexpr int kAttributeMin      = 1;
inline constexpr int kAttributeMax      = 100;
inline constexpr int kAttributeDefault  = 40;
inline constexpr int kAttributeBaseLine = 50;  /// Modifier zero-point.

enum class Attribute : std::uint8_t {
    Str = 0,
    End,
    Agi,
    Wil,
    Int,
    Wit,
    Cha,
    Luc,
    Voi,
    Count,
};

inline constexpr std::size_t kAttributeCount = static_cast<std::size_t>(Attribute::Count);

struct Attributes {
    std::array<int, kAttributeCount> values{};

    constexpr Attributes() {
        values.fill(kAttributeDefault);
    }

    int&       operator[](Attribute a)       noexcept { return values[static_cast<std::size_t>(a)]; }
    int const& operator[](Attribute a) const noexcept { return values[static_cast<std::size_t>(a)]; }

    /// Morrowind-style modifier: roughly -10..+10 over the full range.
    /// `modifier(50) == 0`, `modifier(60) == +2`, `modifier(30) == -4`.
    static constexpr int modifier(int value) noexcept {
        return (value - kAttributeBaseLine) / 5;
    }

    int modifier(Attribute a) const noexcept { return modifier(values[static_cast<std::size_t>(a)]); }

    /// Derived from Strength per Pillar 5.
    constexpr int carry_capacity() const noexcept {
        return values[static_cast<std::size_t>(Attribute::Str)] * 5;
    }

    /// Derived from Luck per Pillar 5.
    constexpr int crit_chance_pct() const noexcept {
        return values[static_cast<std::size_t>(Attribute::Luc)] / 4;
    }

    /// Clamp every field to [kAttributeMin, kAttributeMax].
    void clamp() noexcept;
};

/// Long-form localized-style names; lower-case in headers, capitalized
/// for display. These are stable identifiers used by save data.
char const* attribute_id(Attribute a) noexcept;
char const* attribute_name(Attribute a) noexcept;

}  // namespace character
}  // namespace ash
