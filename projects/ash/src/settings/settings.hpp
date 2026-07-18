#pragma once
/// Phase 10: Settings file loader (D64 + 10.06.002).
///
/// Loads `~/.config/ash/config.json` on startup. Watches the file
/// (stat mtime) for external changes and hot-reloads if needed.
/// Provides typed accessors for every field with sensible defaults.
#include <filesystem>
#include <functional>
#include <string>

namespace ash {
namespace settings {

/// All accessibility / UX knobs. Defaults match pillar 9 (no special
/// accommodations) plus 11-phase-06 polish.
struct Settings {
    /// Color accessibility
    bool    colorblind_mode{false};       /// Palette swap (deuteranopia / protanopia / tritanopia flavor).
    bool    high_contrast{false};          /// Boost foreground/background luminance contrast.
    bool    no_color{false};               /// Strip ANSI; pure glyph mode (terminal-friendly).
    std::string palette_variant{"default"}; /// "default" | "deuteranopia" | "protanopia" | "tritanopia"

    /// UI preferences
    int     text_speed_ms{30};             /// Characters per second for dialog text. Lower = faster.
    std::string font{"standard"};          /// "standard" | "dyslexic"
    bool    reduce_motion{false};
    bool    screen_shake{true};
    int     subtitle_size{1};              /// 0 = small, 1 = medium, 2 = large.
    bool    dyslexic_font{false};
    float   audio_cue_volume{1.0f};        /// 0..1

    /// Misc
    bool    permadeath{false};
    bool    show_damage_numbers{true};
    bool    autosave_enabled{true};
    int     autosave_minutes{10};          /// Per spec: 10. Saved for hot-reload.

    /// Engine tuning
    int     frame_budget_ms{16};
    int     save_budget_ms{500};           /// D69
    int     load_budget_ms{1500};          /// D70

    /// File path used to load these settings (empty if defaults).
    std::filesystem::path source_path;
};

/// Load settings from `~/.config/ash/config.json`. If the file does not
/// exist, returns a default-constructed `Settings` (no error).
Settings load();

/// Save the supplied settings to `~/.config/ash/config.json`. Returns
/// false on filesystem failure.
bool save(const Settings& s);

/// Watcher for hot-reload. `update()` checks the file's mtime and
/// re-reads the settings if it changed. Returns true on hot-reload.
class SettingsWatcher {
public:
    using OnChange = std::function<void(const Settings&)>;

    /// Construct a watcher for the given config path. `cb` fires on
    /// every successful reload.
    SettingsWatcher(std::filesystem::path p, OnChange cb);

    /// Re-read the file if it changed. Returns true if a reload happened.
    bool update();

    const Settings& current() const { return current_; }

private:
    std::filesystem::path path_;
    OnChange             on_change_;
    Settings             current_;
    std::filesystem::file_time_type last_mtime_{};
    bool                has_last_{false};
};

}  // namespace settings
}  // namespace ash