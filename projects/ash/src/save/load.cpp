#include "save/load.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <set>
#include <stdexcept>
#include <system_error>

#include "core/json.hpp"
#include "core/log.hpp"
#include "core/path.hpp"
#include "save/migrate.hpp"
#include "save/save.hpp"
#include "save/schema_v0.hpp"

namespace ash {
namespace save {

namespace fs = std::filesystem;

namespace {

long long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::uint32_t crc32(const std::string& s) {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (char ch : s) {
        unsigned char c = static_cast<unsigned char>(ch);
        crc ^= c;
        for (int k = 0; k < 8; ++k) {
            crc = (crc >> 1) ^ (0xEDB88320u & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::string slurp(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("load: cannot open " + p.string());
    std::string out;
    f.seekg(0, std::ios::end);
    out.resize(static_cast<std::size_t>(f.tellg()));
    f.seekg(0, std::ios::beg);
    f.read(out.data(), static_cast<std::streamsize>(out.size()));
    return out;
}

std::vector<std::uint8_t> slurp_bytes(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    f.seekg(0, std::ios::end);
    auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> out(size);
    f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
    return out;
}

LoadResult load_from_dir(const fs::path& slot, std::string& out_error_chain) {
    LoadResult res;
    try {
        fs::path meta_p = slot / "meta.json";
        fs::path player_p = slot / "player.v0.json";
        fs::path world_p  = slot / "world.v0.json";
        if (!fs::exists(meta_p) || !fs::exists(player_p) || !fs::exists(world_p)) {
            res.error = "missing required file in " + slot.string();
            return res;
        }
        auto mj_raw = json::parse(slurp(meta_p));
        int from = static_cast<int>(mj_raw["schema_version"].int_or(kSchemaVersion));
        if (from > kCurrentSchema) {
            res.error = "save version " + std::to_string(from) + " is newer than engine "
                        + std::to_string(kCurrentSchema) + " (forward-compat error)";
            return res;
        }
        // Apply migrations to meta + player + world documents in lockstep.
        json::Value mj = (from == kCurrentSchema)
            ? mj_raw
            : run_migrations(mj_raw, from);
        auto pj_raw = json::parse(slurp(player_p));
        json::Value pj = (from == kCurrentSchema)
            ? pj_raw
            : run_migrations(pj_raw, from);
        auto wj_raw = json::parse(slurp(world_p));
        json::Value wj = (from == kCurrentSchema)
            ? wj_raw
            : run_migrations(wj_raw, from);

        meta_from_json(mj, res.meta);
        player_from_json(pj, res.data.player);
        world_from_json(wj, res.data.world);

        // Verify meta.checksum against the on-disk player/world blobs.
        // Only check unmigrated loads: migrated blobs were written under
        // an older schema and the checksum would have to be re-computed
        // after migration, which we deliberately avoid here to keep the
        // migration path side-effect free.
        if (from == kCurrentSchema) {
            std::string player_blob_v = slurp(player_p);
            std::string world_blob_v  = slurp(world_p);
            std::uint32_t want = crc32(player_blob_v) ^ crc32(world_blob_v);
            if (res.meta.checksum != want) {
                res.error = "save: checksum mismatch in " + slot.string() +
                            " (want=" + std::to_string(want) +
                            ", got=" + std::to_string(res.meta.checksum) +
                            "); data may be corrupted";
                out_error_chain += res.error + "; ";
                log::warn(res.error);
                return res;
            }
        }

        // Optional thumbnail.
        fs::path thumb_p = slot / "thumbnail.txt";
        if (fs::exists(thumb_p)) {
            res.data.world.thumbnail = slurp(thumb_p);
        }
        // Optional maps/.
        fs::path maps_p = slot / "maps";
        if (fs::exists(maps_p) && fs::is_directory(maps_p)) {
            for (auto const& entry : fs::directory_iterator(maps_p)) {
                if (!entry.is_regular_file()) continue;
                auto fname = entry.path().filename().string();
                if (fname.size() < 3 || fname.substr(fname.size() - 3) != ".xp") continue;
                std::string map_id = fname.substr(0, fname.size() - 3);
                res.data.world.dirty_maps[map_id] = slurp_bytes(entry.path());
            }
        }
        res.ok = true;
    } catch (std::exception const& e) {
        res.error = std::string("load: exception: ") + e.what();
        out_error_chain += res.error + "; ";
    }
    return res;
}

}  // namespace

LoadResult load(const std::string& savename) {
    LoadResult res;
    auto t0 = now_ms();
    std::string chain;
    fs::path slot = path_for(savename);
    if (!fs::exists(slot)) {
        res.error = "save: no such slot '" + savename + "'";
        res.elapsed_ms = now_ms() - t0;
        return res;
    }
    res = load_from_dir(slot, chain);
    res.elapsed_ms = now_ms() - t0;
    if (res.ok) log::info("load: loaded slot '" + savename + "' in " + std::to_string(res.elapsed_ms) + "ms");
    else       log::error("load: failed slot '" + savename + "': " + res.error);
    return res;
}

LoadResult load_with_fallback(const std::string& savename) {
    LoadResult res;
    auto t0 = now_ms();
    std::vector<fs::path> candidates;
    fs::path slot = path_for(savename);
    candidates.push_back(slot);
    candidates.push_back(slot.string() + ".bak1");
    candidates.push_back(slot.string() + ".bak2");

    for (auto const& c : candidates) {
        std::error_code ec;
        if (!fs::exists(c, ec)) continue;
        std::string chain;
        auto r = load_from_dir(c, chain);
        if (r.ok) {
            r.elapsed_ms = now_ms() - t0;
            log::warn("load: fell back to " + c.string() + " for slot '" + savename + "'");
            return r;
        }
    }
    res.error = "load: no valid generation for slot '" + savename + "'";
    res.elapsed_ms = now_ms() - t0;
    return res;
}

}  // namespace save
}  // namespace ash