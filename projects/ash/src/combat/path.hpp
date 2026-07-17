#pragma once
/// Phase 07: A* pathfinder on the world grid (Step 0706).
///
/// * 4-directional movement (cardinal only — diagonal is reserved for
///   Phase 11 polish).
/// * Walks the `Map::is_walkable()` predicate so closed doors block
///   unless the pathfinder was told the agent can open them.
/// * Caches results by `(from, to, door_state_hash)` per Pillar 9
///   performance budget. The cache is invalidated by `clear_cache()`.
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "core/math.hpp"
#include "world/map.hpp"

namespace ash {
namespace combat {

struct PathCacheKey {
    core::IVec2   from{};
    core::IVec2   to{};
    std::uint64_t door_hash{0};
    bool          can_open_doors{false};

    bool operator==(PathCacheKey const& o) const noexcept {
        return from == o.from && to == o.to && door_hash == o.door_hash &&
               can_open_doors == o.can_open_doors;
    }
};

struct PathCacheKeyHash {
    std::size_t operator()(PathCacheKey const& k) const noexcept {
        std::uint64_t h = 0xCBF29CE484222325ULL;
        auto mix = [&](std::uint64_t v) {
            h ^= v + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
        };
        mix(static_cast<std::uint64_t>(k.from.x));
        mix(static_cast<std::uint64_t>(k.from.y));
        mix(static_cast<std::uint64_t>(k.to.x));
        mix(static_cast<std::uint64_t>(k.to.y));
        mix(k.door_hash);
        mix(k.can_open_doors ? 1 : 0);
        return static_cast<std::size_t>(h);
    }
};

/// Compute the shortest walkable path from `from` to `to`. Returns an
/// empty vector if no path exists. The returned vector starts at `from`
/// and ends at `to` (inclusive). If `from == to`, the result contains
/// exactly one cell.
std::vector<core::IVec2> find_path(world::Map const& map,
                                   core::IVec2       from,
                                   core::IVec2       to,
                                   bool              can_open_doors = false) noexcept;

/// Cached version. Hits `cache` first; on miss, runs `find_path` and
/// stores the result. Cache lifetime is the caller's responsibility;
/// paths become stale when `door_state_hash` changes — `clear_cache`
/// drops everything.
std::vector<core::IVec2> find_path_cached(
    world::Map const& map,
    core::IVec2       from,
    core::IVec2       to,
    std::unordered_map<PathCacheKey, std::vector<core::IVec2>, PathCacheKeyHash>& cache,
    bool              can_open_doors = false) noexcept;

/// Number of A* cache hits (cumulative). Useful for benchmark tests.
struct PathStats {
    std::uint64_t hits{0};
    std::uint64_t misses{0};
    std::uint64_t computed{0};  /// = misses
};

}  // namespace combat
}  // namespace ash
