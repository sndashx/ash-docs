#include "save/save.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <set>
#include <stdexcept>
#include <system_error>

#include "core/json.hpp"
#include "core/log.hpp"
#include "core/path.hpp"
#include "core/time.hpp"
#include "core/version.hpp"
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
    // Simple CRC32 (IEEE). Used to detect bit-rot in meta.json.
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

void write_text(const fs::path& p, const std::string& body) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("save: cannot open " + p.string() + " for write");
    f.write(body.data(), static_cast<std::streamsize>(body.size()));
    if (!f) throw std::runtime_error("save: write failed for " + p.string());
    f.flush();
}

void write_bytes(const fs::path& p, const std::vector<std::uint8_t>& bytes) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("save: cannot open " + p.string());
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    f.flush();
}

void remove_dir(const fs::path& p) {
    std::error_code ec;
    fs::remove_all(p, ec);
}

bool is_slot_name_safe(const std::string& n) {
    if (n.empty() || n.size() > 64) return false;
    for (char c : n) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.')) {
            return false;
        }
    }
    return true;
}

// Returns true on success. On failure (e.g. cannot rename slot -> bak1),
// returns false and leaves the existing slot in place. The caller must
// not overwrite the slot in that case.
bool rotate_backups(const fs::path& slot, std::string& out_error) {
    fs::path bak2 = slot;
    bak2 += ".bak2";
    fs::path bak1 = slot;
    bak1 += ".bak1";
    std::error_code ec;
    fs::remove_all(bak2, ec);
    if (fs::exists(bak1, ec)) {
        fs::rename(bak1, bak2, ec);
        if (ec) {
            // Best-effort: nuke bak1 and retry. If bak2 removal worked
            // we can no longer preserve the chain, but we don't want to
            // destroy the existing slot.
            fs::remove_all(bak1, ec);
        }
    }
    if (fs::exists(slot, ec)) {
        fs::rename(slot, bak1, ec);
        if (ec) {
            out_error = "save: could not rotate slot to bak1: " + ec.message();
            return false;
        }
    }
    return true;
}

}  // namespace

std::filesystem::path path_for(const std::string& savename) {
    return core::slot_dir(savename);
}

bool exists(const std::string& savename) {
    std::error_code ec;
    return fs::exists(path_for(savename) / "meta.json", ec);
}

bool remove(const std::string& savename) {
    fs::path p = path_for(savename);
    std::error_code ec;
    if (!fs::exists(p, ec)) return false;
    fs::remove_all(p, ec);
    fs::remove(p.string() + ".bak1", ec);
    fs::remove(p.string() + ".bak2", ec);
    return true;
}

SaveResult save_with_meta(const std::string& savename, const SaveData& data,
                          SaveMeta& out_meta) {
    SaveResult res;
    auto t0 = now_ms();
    if (!is_slot_name_safe(savename)) {
        res.error = "save: invalid slot name";
        return res;
    }
    try {
        if (!core::ensure_dir(core::saves_dir())) {
            res.error = "save: cannot create saves dir";
            return res;
        }
        fs::path slot = path_for(savename);
        fs::path tmp  = slot;
        tmp += ".tmp";
        // Always start from a clean tmp.
        remove_dir(tmp);
        if (!core::ensure_dir(tmp)) {
            res.error = "save: cannot create tmp dir";
            return res;
        }
        // Build the meta block first so we can checksum player+world bytes.
        SaveMeta meta;
        meta.schema_version    = kSchemaVersion;
        meta.version           = core::ASH_VERSION_STRING;
        meta.save_name         = data.player.name.empty() ? savename : data.player.name;
        meta.created_at        = out_meta.created_at.empty() ? core::iso8601_utc(core::unix_seconds()) : out_meta.created_at;
        meta.last_played_at    = core::iso8601_utc(core::unix_seconds());
        meta.play_time_seconds = data.world.play_time_seconds;
        meta.player_level      = data.player.level.level;
        meta.player_map        = data.world.current_map;
        meta.player_x          = data.world.current_x;
        meta.player_y          = data.world.current_y;
        meta.rng_seed          = data.world.rng_seed;
        meta.game_time.year     = data.world.game_year;
        meta.game_time.month    = data.world.game_month;
        meta.game_time.day      = data.world.game_day;
        meta.game_time.hour     = data.world.game_hour;
        meta.game_time.minute   = data.world.game_minute;
        meta.weather    = "clear";
        meta.difficulty = "normal";
        meta.permadeath = false;

        auto pj = player_to_json(data.player);
        auto wj = world_to_json(data.world);

        std::string player_blob = json::dump(pj);
        std::string world_blob  = json::dump(wj);
        meta.checksum = crc32(player_blob) ^ crc32(world_blob);

        auto mj = meta_to_json(meta);

        write_text(tmp / "meta.json",         json::dump(mj));
        write_text(tmp / "player.v0.json",    player_blob);
        write_text(tmp / "world.v0.json",     world_blob);
        write_text(tmp / "thumbnail.txt",     data.world.thumbnail);

        // Maps directory: only write dirty maps to keep save size small.
        if (!data.world.dirty_maps.empty()) {
            fs::path maps_dir = tmp / "maps";
            core::ensure_dir(maps_dir);
            for (auto const& [id, bytes] : data.world.dirty_maps) {
                write_bytes(maps_dir / (id + ".xp"), bytes);
            }
        }

        // Atomic swap: rotate backups, then rename tmp -> slot.
        std::string rot_err;
        if (!rotate_backups(slot, rot_err)) {
            res.error = rot_err + "; refusing to overwrite existing save";
            log::error(res.error);
            remove_dir(tmp);
            return res;
        }
        std::error_code ec;
        fs::rename(tmp, slot, ec);
        if (ec) {
            res.error = "save: atomic rename failed: " + ec.message();
            log::error(res.error);
            remove_dir(tmp);
            return res;
        }

        out_meta = meta;
        res.ok = true;
        res.path = slot;
        log::info("save: wrote slot '" + savename + "' to " + slot.string());
    } catch (std::exception const& e) {
        res.error = std::string("save: exception: ") + e.what();
        log::error(res.error);
    }
    res.elapsed_ms = now_ms() - t0;
    return res;
}

SaveResult save(const std::string& savename, const SaveData& data) {
    SaveMeta unused;
    return save_with_meta(savename, data, unused);
}

std::vector<SlotInfo> list_slots() {
    std::vector<SlotInfo> out;
    std::error_code ec;
    fs::path dir = core::saves_dir();
    if (!fs::exists(dir, ec)) return out;
    for (auto const& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_directory(ec)) continue;
        fs::path meta = entry.path() / "meta.json";
        if (!fs::exists(meta, ec)) continue;
        std::ifstream f(meta);
        if (!f) continue;
        std::string buf((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
        try {
            auto j = json::parse(buf);
            SlotInfo s;
            s.savename          = entry.path().filename().string();
            s.save_name         = j["save_name"].str_or(s.savename);
            s.player_level      = static_cast<int>(j["player_level"].int_or(0));
            if (j.contains("player_location") && j["player_location"].is_object()) {
                s.player_map = j["player_location"]["map_id"].str_or("");
            }
            s.play_time_seconds = static_cast<int>(j["play_time_seconds"].int_or(0));
            std::string last = j["last_played_at"].str_or("");
            s.last_played_at    = core::parse_iso8601(last);
            out.push_back(std::move(s));
        } catch (...) {
            // Skip corrupted slots; the load pipeline can recover.
        }
    }
    return out;
}

}  // namespace save
}  // namespace ash