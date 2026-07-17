#include "world/map.hpp"

#include <cstdint>

namespace ash {
namespace world {

bool Map::is_walkable(int x, int y, bool can_open_doors) const noexcept {
    if (x < 0 || x >= kMapWidth || y < 0 || y >= kMapHeight) {
        return false;
    }
    std::uint16_t const f = tiles[static_cast<std::size_t>(idx(x, y))];
    if (has_flag(f, TF_WALKABLE) && !has_flag(f, TF_BLOCKING)) {
        return true;
    }
    if (has_flag(f, TF_DOOR)) {
        auto it = doors.find(idx(x, y));
        bool open = (it == doors.end()) ? false : it->second;
        if (open) {
            return true;
        }
        return can_open_doors;
    }
    return false;
}

bool Map::is_opaque(int x, int y) const noexcept {
    if (x < 0 || x >= kMapWidth || y < 0 || y >= kMapHeight) {
        return false;
    }
    std::uint16_t const f = tiles[static_cast<std::size_t>(idx(x, y))];
    if (has_flag(f, TF_DOOR)) {
        auto it = doors.find(idx(x, y));
        bool open = (it == doors.end()) ? false : it->second;
        return !open;
    }
    if (has_flag(f, TF_OPAQUE)) {
        return true;
    }
    return false;
}

bool Map::is_blocking(int x, int y, bool can_open_doors) const noexcept {
    if (x < 0 || x >= kMapWidth || y < 0 || y >= kMapHeight) {
        return true;
    }
    std::uint16_t const f = tiles[static_cast<std::size_t>(idx(x, y))];
    if (has_flag(f, TF_DOOR)) {
        auto it = doors.find(idx(x, y));
        bool open = (it == doors.end()) ? false : it->second;
        if (!open) {
            return !can_open_doors;
        }
    }
    if (has_flag(f, TF_BLOCKING)) {
        return true;
    }
    return false;
}

void Map::set_door_open(int x, int y, bool open) noexcept {
    if (x < 0 || x >= kMapWidth || y < 0 || y >= kMapHeight) {
        return;
    }
    int k = idx(x, y);
    if (!has_flag(tiles[static_cast<std::size_t>(k)], TF_DOOR)) {
        return;
    }
    doors[k] = open;
}

std::uint64_t Map::door_state_hash() const noexcept {
    if (doors.empty()) {
        return 0;
    }
    std::uint64_t h = static_cast<std::uint64_t>(0xCBF29CE484222325ULL);
    for (auto const& [k, open] : doors) {
        std::uint64_t const salt = static_cast<std::uint64_t>(0x9E3779B97F4A7C15ULL);
        std::uint64_t const kk = static_cast<std::uint64_t>(static_cast<std::size_t>(k)) + salt;
        h ^= kk + (h << 6) + (h >> 2);
        std::uint64_t const bo = static_cast<std::uint64_t>(open ? 1u : 0u) + salt;
        h ^= bo + (h << 6) + (h >> 2);
    }
    return h;
}

Map make_open_map(int w, int h) noexcept {
    Map m;
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            int k = idx(x, y);
            if (x < w && y < h) {
                m.tiles[static_cast<std::size_t>(k)] = TF_WALKABLE;
            }
        }
    }
    return m;
}

Map make_arena(int w, int h) noexcept {
    Map m;
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            int k = idx(x, y);
            bool inside = (x >= 0 && x < w && y >= 0 && y < h);
            bool border  = (x == 0 || y == 0 || x == w - 1 || y == h - 1);
            if (!inside || border) {
                m.tiles[static_cast<std::size_t>(k)] = TF_BLOCKING | TF_OPAQUE;
            } else {
                m.tiles[static_cast<std::size_t>(k)] = TF_WALKABLE;
            }
        }
    }
    return m;
}

}  // namespace world
}  // namespace ash