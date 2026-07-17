#include "test_harness.hpp"
#include "character/skills.hpp"

using ash::character::Skill;
using ash::character::Skills;
using ash::character::skill_attribute;
using ash::character::skill_groups;

TEST_CASE("skills: exactly 24 unique skill ids", "[character][skills]")
{
    REQUIRE(static_cast<int>(Skill::Count) == 24);
    Skills s;
    REQUIRE(s.values.size() == 24u);
}

TEST_CASE("skills: governing attributes pinned", "[character][skills]")
{
    REQUIRE(skill_attribute(Skill::Blunt)        == ash::character::Attribute::Str);
    REQUIRE(skill_attribute(Skill::Blade)        == ash::character::Attribute::Str);
    REQUIRE(skill_attribute(Skill::Armorer)      == ash::character::Attribute::Str);
    REQUIRE(skill_attribute(Skill::Marksman)     == ash::character::Attribute::Agi);
    REQUIRE(skill_attribute(Skill::Dodge)        == ash::character::Attribute::Agi);
    REQUIRE(skill_attribute(Skill::Thrown)       == ash::character::Attribute::Agi);
    REQUIRE(skill_attribute(Skill::Stealth)      == ash::character::Attribute::Agi);
    REQUIRE(skill_attribute(Skill::Pickpocket)   == ash::character::Attribute::Agi);
    REQUIRE(skill_attribute(Skill::Security)     == ash::character::Attribute::Agi);
    REQUIRE(skill_attribute(Skill::Mysticism)    == ash::character::Attribute::Wil);
    REQUIRE(skill_attribute(Skill::Restoration)  == ash::character::Attribute::Wil);
    REQUIRE(skill_attribute(Skill::Warding)      == ash::character::Attribute::Wil);
    REQUIRE(skill_attribute(Skill::Alchemy)      == ash::character::Attribute::Int);
    REQUIRE(skill_attribute(Skill::Enchant)      == ash::character::Attribute::Int);
    REQUIRE(skill_attribute(Skill::Spellcraft)   == ash::character::Attribute::Int);
    REQUIRE(skill_attribute(Skill::History)      == ash::character::Attribute::Int);
    REQUIRE(skill_attribute(Skill::Theology)     == ash::character::Attribute::Int);
    REQUIRE(skill_attribute(Skill::Linguistics)  == ash::character::Attribute::Int);
    REQUIRE(skill_attribute(Skill::Speechcraft)  == ash::character::Attribute::Cha);
    REQUIRE(skill_attribute(Skill::Mercantile)   == ash::character::Attribute::Cha);
    REQUIRE(skill_attribute(Skill::Illusion)     == ash::character::Attribute::Cha);
    REQUIRE(skill_attribute(Skill::Deception)    == ash::character::Attribute::Wit);
    REQUIRE(skill_attribute(Skill::Intimidation) == ash::character::Attribute::Wit);
    REQUIRE(skill_attribute(Skill::Seduction)    == ash::character::Attribute::Wit);
}

TEST_CASE("skills: groups cover 24 skills (3 per group, 8 groups)", "[character][skills]")
{
    auto const& g = skill_groups();
    REQUIRE(g.size() == 8u);
    for (auto const& grp : g) {
        for (auto sk : grp.skills) {
            REQUIRE(skill_attribute(sk) == grp.governing);
        }
    }
}

TEST_CASE("skills: gain_skill returns integer delta with diminishing returns", "[character][skills]")
{
    Skills s;
    s[Skill::Blade] = 0;
    int const d0 = s.gain_skill(Skill::Blade, 100);
    REQUIRE(d0 == 100);
    REQUIRE(s[Skill::Blade] == 100);

    int const d1 = s.gain_skill(Skill::Blade, 100);
    REQUIRE(d1 == 50);
    REQUIRE(s[Skill::Blade] == 150);

    s[Skill::Blade] = 900;
    int const d2 = s.gain_skill(Skill::Blade, 100);
    REQUIRE(d2 == 10);
}

TEST_CASE("skills: gain_skill clamps at cap and floors at zero", "[character][skills]")
{
    Skills s;
    s[Skill::Mysticism] = ash::character::kSkillMax - 5;
    int const d = s.gain_skill(Skill::Mysticism, 1000);
    REQUIRE(d == 5);
    REQUIRE(s[Skill::Mysticism] == ash::character::kSkillMax);

    s[Skill::Mysticism] = 3;
    int const d2 = s.gain_skill(Skill::Mysticism, -100);
    REQUIRE(d2 == -3);
    REQUIRE(s[Skill::Mysticism] == ash::character::kSkillMin);
}

TEST_CASE("skills: zero amount is no-op", "[character][skills]")
{
    Skills s;
    s[Skill::Blade] = 42;
    int const d = s.gain_skill(Skill::Blade, 0);
    REQUIRE(d == 0);
    REQUIRE(s[Skill::Blade] == 42);
}
