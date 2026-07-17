#include "test_harness.hpp"

#include "combat/line_of_sight.hpp"
#include "world/map.hpp"
#include "world/tile_flags.hpp"

using ash::combat::has_line_of_sight;
using ash::combat::bresenham_line;
using ash::world::Map;
using ash::world::make_arena;
using ash::world::TF_BLOCKING;
using ash::world::TF_OPAQUE;
using ash::world::TF_DOOR;

TEST_CASE("los: clear through open arena", "[combat][los]")
{
    Map m = make_arena(20, 20);
    REQUIRE(has_line_of_sight(m, {1, 1}, {18, 18}));
}

TEST_CASE("los: blocked by opaque wall", "[combat][los]")
{
    Map m = make_arena(20, 20);
    for (int y = 0; y < 20; ++y) {
        m.tiles[ash::world::idx(10, y)] = TF_BLOCKING | TF_OPAQUE;
    }
    REQUIRE(!has_line_of_sight(m, {1, 5}, {18, 5}));
}

TEST_CASE("los: open door does not block", "[combat][los]")
{
    Map m = make_arena(20, 20);
    m.tiles[ash::world::idx(10, 5)] = TF_DOOR | TF_OPAQUE;
    m.set_door_open(10, 5, true);
    REQUIRE(has_line_of_sight(m, {1, 5}, {18, 5}));
}

TEST_CASE("los: closed door blocks", "[combat][los]")
{
    Map m = make_arena(20, 20);
    m.tiles[ash::world::idx(10, 5)] = TF_DOOR | TF_OPAQUE;
    m.set_door_open(10, 5, false);
    REQUIRE(!has_line_of_sight(m, {1, 5}, {18, 5}));
}

TEST_CASE("los: bresenham line covers every cell", "[combat][los]")
{
    auto line = bresenham_line({0, 0}, {4, 3});
    REQUIRE(line.size() >= 5);
    REQUIRE(line.front().x == 0 && line.front().y == 0);
    REQUIRE(line.back().x == 4 && line.back().y == 3);
}