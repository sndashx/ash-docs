#include "combat/path.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ash {
namespace combat {

namespace {

struct Node {
    core::IVec2 cell{};
    int         f{0};
    int         g{0};
    bool operator>(Node const& o) const noexcept { return f > o.f; }
};

int chebyshev(core::IVec2 a, core::IVec2 b) noexcept {
    int dx = a.x - b.x; if (dx < 0) dx = -dx;
    int dy = a.y - b.y; if (dy < 0) dy = -dy;
    return dx + dy;
}

std::vector<core::IVec2> reconstruct(
    core::IVec2 start,
    core::IVec2 goal,
    std::unordered_map<int, core::IVec2> const& came_from) {
    std::vector<core::IVec2> path;
    core::IVec2 cur = goal;
    path.push_back(cur);
    while (!(cur == start)) {
        auto it = came_from.find(world::idx(cur.x, cur.y));
        if (it == came_from.end()) {
            return {};
        }
        cur = it->second;
        path.push_back(cur);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

}  // namespace

std::vector<core::IVec2> find_path(world::Map const& map,
                                   core::IVec2       from,
                                   core::IVec2       to,
                                   bool              can_open_doors) noexcept {
    if (from == to) {
        return std::vector<core::IVec2>{from};
    }
    if (!map.is_walkable(to.x, to.y, can_open_doors)) {
        return {};
    }
    static constexpr int dx[4] = {1, -1, 0, 0};
    static constexpr int dy[4] = {0, 0, 1, -1};

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    std::unordered_map<int, int> g_score;
    std::unordered_map<int, core::IVec2> came_from;
    std::unordered_set<int> closed;

    g_score[world::idx(from.x, from.y)] = 0;
    open.push(Node{from, chebyshev(from, to), 0});

    while (!open.empty()) {
        Node cur = open.top();
        open.pop();
        int ck = world::idx(cur.cell.x, cur.cell.y);
        if (closed.count(ck)) continue;
        closed.insert(ck);

        if (cur.cell == to) {
            return reconstruct(from, to, came_from);
        }

        for (int i = 0; i < 4; ++i) {
            core::IVec2 next{cur.cell.x + dx[i], cur.cell.y + dy[i]};
            if (!map.is_walkable(next.x, next.y, can_open_doors)) continue;
            int nk = world::idx(next.x, next.y);
            if (closed.count(nk)) continue;
            int tentative_g = cur.g + 1;
            auto it = g_score.find(nk);
            if (it == g_score.end() || tentative_g < it->second) {
                g_score[nk] = tentative_g;
                came_from[nk] = cur.cell;
                int f = tentative_g + chebyshev(next, to);
                open.push(Node{next, f, tentative_g});
            }
        }
    }
    return {};
}

std::vector<core::IVec2> find_path_cached(
    world::Map const& map,
    core::IVec2       from,
    core::IVec2       to,
    std::unordered_map<PathCacheKey, std::vector<core::IVec2>, PathCacheKeyHash>& cache,
    bool              can_open_doors) noexcept {
    PathCacheKey key{from, to, map.door_state_hash(), can_open_doors};
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
    auto result = find_path(map, from, to, can_open_doors);
    cache.emplace(key, result);
    return result;
}

}  // namespace combat
}  // namespace ash