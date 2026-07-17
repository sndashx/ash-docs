#include "test_harness.hpp"

#include "combat/path.hpp"
#include "world/map.hpp"
#include "world/tile_flags.hpp"

using ash::combat::find_path;
using ash::combat::find_path_cached;
using ash::combat::PathCacheKey;
using ash::combat::PathCacheKeyHash;
using ash::world::Map;
using ash::world::make_arena;
using ash::world::TF_BLOCKING;
using ash::world::TF_OPAQUE;

TEST_CASE("path: straight line on open arena", "[combat][path]")
{
    Map m = make_arena(20, 20);
    auto p = find_path(m, {1, 1}, {18, 1});
    REQUIRE(!p.empty());
    REQUIRE(p.front().x == 1 && p.front().y == 1);
    REQUIRE(p.back().x == 18 && p.back().y == 1);
}

TEST_CASE("path: around obstacle", "[combat][path]")
{
    Map m = make_arena(20, 20);
    /// Block the direct horizontal corridor.
    for (int x = 5; x < 15; ++x) {
        m.tiles[ash::world::idx(x, 5)] = TF_BLOCKING | TF_OPAQUE;
    }
    auto p = find_path(m, {1, 5}, {18, 5});
    REQUIRE(!p.empty());
    /// Path must detour around the obstacle (leave row 5).
    bool leaves_row = false;
    for (auto const& c : p) {
        if (c.y != 5) { leaves_row = true; break; }
    }
    REQUIRE(leaves_row);
}

TEST_CASE("path: no path returns empty", "[combat][path]")
{
    Map m = make_arena(20, 20);
    /// Enclose the target with walls.
    m.tiles[ash::world::idx(10, 5)] = TF_BLOCKING | TF_OPAQUE;
    m.tiles[ash::world::idx(9, 5)] = TF_BLOCKING | TF_OPAQUE;
    m.tiles[ash::world::idx(11, 5)] = TF_BLOCKING | TF_OPAQUE;
    m.tiles[ash::world::idx(10, 4)] = TF_BLOCKING | TF_OPAQUE;
    m.tiles[ash::world::idx(10, 6)] = TF_BLOCKING | TF_OPAQUE;
    auto p = find_path(m, {1, 5}, {10, 5});
    REQUIRE(p.empty());
}

TEST_CASE("path: cache hits on second identical call", "[combat][path]")
{
    Map m = make_arena(20, 20);
    std::unordered_map<PathCacheKey, std::vector<ash::core::IVec2>, PathCacheKeyHash> cache;
    auto p1 = find_path_cached(m, {1, 1}, {18, 1}, cache);
    auto p2 = find_path_cached(m, {1, 1}, {18, 1}, cache);
    REQUIRE(p1 == p2);
    REQUIRE(cache.size() == 1);
}
