#include "test_harness.hpp"
#include "character/attributes.hpp"

using ash::character::Attributes;
using ash::character::Attribute;
using ash::character::kAttributeMax;
using ash::character::kAttributeMin;
using ash::character::kAttributeDefault;

TEST_CASE("attributes: default values clamp and carry_capacity", "[character][attributes]")
{
    Attributes a;
    for (auto i = 0u; i < a.values.size(); ++i) {
        REQUIRE(a.values[i] == kAttributeDefault);
    }
    REQUIRE(a.carry_capacity() == kAttributeDefault * 5);
    REQUIRE(a.crit_chance_pct() == kAttributeDefault / 4);
}

TEST_CASE("attributes: modifier formula pinned", "[character][attributes]")
{
    REQUIRE(Attributes::modifier(50) == 0);
    REQUIRE(Attributes::modifier(60) == 2);
    REQUIRE(Attributes::modifier(40) == -2);
    REQUIRE(Attributes::modifier(100) == 10);
    REQUIRE(Attributes::modifier(1) == -9);
}

TEST_CASE("attributes: per-attribute modifiers and clamping", "[character][attributes]")
{
    Attributes a;
    a[Attribute::Str] = 80;
    a[Attribute::Luc] = 60;
    REQUIRE(a.carry_capacity() == 400);
    REQUIRE(a.crit_chance_pct() == 15);
    REQUIRE(a.modifier(Attribute::Str) == 6);

    a[Attribute::Str] = 200;
    a.clamp();
    REQUIRE(a[Attribute::Str] == kAttributeMax);
    a[Attribute::Luc] = -5;
    a.clamp();
    REQUIRE(a[Attribute::Luc] == kAttributeMin);
}

TEST_CASE("attributes: all nine slots present", "[character][attributes]")
{
    Attributes a;
    REQUIRE(a.values.size() == static_cast<std::size_t>(Attribute::Count));
    REQUIRE(static_cast<int>(Attribute::Count) == 9);
}
