#include "settings/settings.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include "core/json.hpp"
#include "core/path.hpp"

namespace ash {
namespace settings {

namespace fs = std::filesystem;

namespace {

fs::path default_config_path() {
    return core::config_dir() / "config.json";
}

Settings from_json(const json::Value& v) {
    Settings s;
    if (!v.is_object()) return s;
    s.colorblind_mode   = v["colorblind_mode"].bool_or(false);
    s.high_contrast     = v["high_contrast"].bool_or(false);
    s.no_color          = v["no_color"].bool_or(false);
    s.palette_variant   = v["palette_variant"].str_or("default");
    s.text_speed_ms     = static_cast<int>(v["text_speed_ms"].int_or(s.text_speed_ms));
    s.font              = v["font"].str_or("standard");
    s.reduce_motion     = v["reduce_motion"].bool_or(false);
    s.screen_shake      = v["screen_shake"].bool_or(true);
    s.subtitle_size     = static_cast<int>(v["subtitle_size"].int_or(s.subtitle_size));
    s.dyslexic_font     = v["dyslexic_font"].bool_or(false);
    s.audio_cue_volume  = static_cast<float>(v["audio_cue_volume"].dbl_or(s.audio_cue_volume));
    s.permadeath        = v["permadeath"].bool_or(false);
    s.show_damage_numbers = v["show_damage_numbers"].bool_or(true);
    s.autosave_enabled  = v["autosave_enabled"].bool_or(true);
    s.autosave_minutes  = static_cast<int>(v["autosave_minutes"].int_or(s.autosave_minutes));
    s.frame_budget_ms   = static_cast<int>(v["frame_budget_ms"].int_or(s.frame_budget_ms));
    s.save_budget_ms    = static_cast<int>(v["save_budget_ms"].int_or(s.save_budget_ms));
    s.load_budget_ms    = static_cast<int>(v["load_budget_ms"].int_or(s.load_budget_ms));
    return s;
}

json::Value to_json(const Settings& s) {
    json::Value v(json::Object{});
    v["colorblind_mode"]     = s.colorblind_mode;
    v["high_contrast"]       = s.high_contrast;
    v["no_color"]            = s.no_color;
    v["palette_variant"]     = s.palette_variant;
    v["text_speed_ms"]       = static_cast<std::int64_t>(s.text_speed_ms);
    v["font"]                = s.font;
    v["reduce_motion"]       = s.reduce_motion;
    v["screen_shake"]        = s.screen_shake;
    v["subtitle_size"]       = static_cast<std::int64_t>(s.subtitle_size);
    v["dyslexic_font"]       = s.dyslexic_font;
    v["audio_cue_volume"]    = static_cast<double>(s.audio_cue_volume);
    v["permadeath"]          = s.permadeath;
    v["show_damage_numbers"] = s.show_damage_numbers;
    v["autosave_enabled"]    = s.autosave_enabled;
    v["autosave_minutes"]    = static_cast<std::int64_t>(s.autosave_minutes);
    v["frame_budget_ms"]     = static_cast<std::int64_t>(s.frame_budget_ms);
    v["save_budget_ms"]      = static_cast<std::int64_t>(s.save_budget_ms);
    v["load_budget_ms"]      = static_cast<std::int64_t>(s.load_budget_ms);
    return v;
}

}  // namespace

Settings load() {
    Settings s;
    s.source_path = default_config_path();
    std::error_code ec;
    if (!fs::exists(s.source_path, ec)) {
        return s;
    }
    std::ifstream f(s.source_path);
    if (!f) return s;
    std::stringstream buf;
    buf << f.rdbuf();
    try {
        auto j = json::parse(buf.str());
        Settings loaded = from_json(j);
        loaded.source_path = s.source_path;
        return loaded;
    } catch (std::exception const& e) {
        // Bad config: return defaults but log the problem to stderr.
        std::fprintf(stderr, "[settings] failed to parse %s: %s\n",
                     s.source_path.string().c_str(), e.what());
        return s;
    }
}

bool save(const Settings& s) {
    fs::path p = s.source_path.empty() ? default_config_path() : s.source_path;
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << json::dump(to_json(s), true);
    return static_cast<bool>(f);
}

SettingsWatcher::SettingsWatcher(fs::path p, OnChange cb)
    : path_(std::move(p)), on_change_(std::move(cb)) {
    std::error_code ec;
    if (fs::exists(path_, ec)) {
        last_mtime_ = fs::last_write_time(path_, ec);
        has_last_ = true;
        try {
            std::ifstream f(path_);
            std::stringstream buf;
            buf << f.rdbuf();
            current_ = from_json(json::parse(buf.str()));
            current_.source_path = path_;
        } catch (...) {
            // leave defaults
        }
    }
}

bool SettingsWatcher::update() {
    std::error_code ec;
    if (!fs::exists(path_, ec)) return false;
    auto mtime = fs::last_write_time(path_, ec);
    if (has_last_ && mtime == last_mtime_) return false;
    last_mtime_ = mtime;
    has_last_ = true;
    try {
        std::ifstream f(path_);
        std::stringstream buf;
        buf << f.rdbuf();
        current_ = from_json(json::parse(buf.str()));
        current_.source_path = path_;
        if (on_change_) on_change_(current_);
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace settings
}  // namespace ash