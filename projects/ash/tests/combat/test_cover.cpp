#include "test_harness.hpp"

#include <string>

#include "combat/cover.hpp"
#include "world/map.hpp"
#include "world/tile_flags.hpp"

using ash::combat::Cover;
using ash::combat::compute_cover;
using ash::combat::cover_to_hit_penalty;
using ash::combat::cover_name;
using ash::world::Map;
using ash::world::make_arena;
using ash::world::TF_BLOCKING;
using ash::world::TF_OPAQUE;

TEST_CASE("cover: none when both in open", "[combat][cover]")
{
    Map m = make_arena(20, 20);
    /// Both cells interior so arena borders don't trip Cover::ThreeQuarters.
    REQUIRE(compute_cover(m, {2, 2}, {17, 17}) == Cover::None);
    REQUIRE(cover_to_hit_penalty(Cover::None) == 0);
}

TEST_CASE("cover: full when target behind wall (no LoS)", "[combat][cover]")
{
    Map m = make_arena(20, 20);
    for (int y = 0; y < 20; ++y) {
        m.tiles[ash::world::idx(10, y)] = TF_BLOCKING | TF_OPAQUE;
    }
    REQUIRE(compute_cover(m, {1, 10}, {18, 10}) == Cover::Full);
    REQUIRE(cover_to_hit_penalty(Cover::Full) == 10);
}

TEST_CASE("cover: half from a single adjacent wall", "[combat][cover]")
{
    Map m = make_arena(20, 20);
    /// Place a wall next to the target but not blocking LoS.
    m.tiles[ash::world::idx(11, 10)] = TF_BLOCKING | TF_OPAQUE;
    /// Move target to (10, 10), attacker to (1, 10). LoS is clear; one blocking neighbor.
    REQUIRE(compute_cover(m, {1, 10}, {10, 10}) == Cover::Half);
    REQUIRE(cover_to_hit_penalty(Cover::Half) == 2);
}

TEST_CASE("cover: three-quarters when cornered", "[combat][cover]")
{
    Map m = make_arena(20, 20);
    /// Surround the target on 3+ sides with non-opaque walls (so LoS
    /// stays clear but blocking-neighbor count goes up).
    auto wall = TF_BLOCKING;  /// not opaque
    m.tiles[ash::world::idx(11, 10)] = wall;
    m.tiles[ash::world::idx(9,  10)] = wall;
    m.tiles[ash::world::idx(10, 11)] = wall;
    m.tiles[ash::world::idx(10, 9 )] = wall;
    REQUIRE(compute_cover(m, {1, 10}, {10, 10}) == Cover::ThreeQuarters);
    REQUIRE(cover_to_hit_penalty(Cover::ThreeQuarters) == 5);
}

TEST_CASE("cover: name covers all enum values", "[combat][cover]")
{
    REQUIRE(std::string(cover_name(Cover::None)) == "none");
    REQUIRE(std::string(cover_name(Cover::Half)) == "half");
    REQUIRE(std::string(cover_name(Cover::ThreeQuarters)) == "three_quarters");
    REQUIRE(std::string(cover_name(Cover::Full)) == "full");
}
