#pragma once
/// Phase 10: XDG path resolution.
///
/// Per pillar 9: data goes in ~/.local/share/ash, config in
/// ~/.config/ash, logs in ~/.local/share/ash/logs, and bundled
/// content lives alongside the executable (or in $ASH_CONTENT_DIR).
#include <filesystem>
#include <string>

namespace ash {
namespace core {

namespace fs = std::filesystem;

/// Root data dir (~/.local/share/ash or $XDG_DATA_HOME/ash).
/// Created on first call if missing.
fs::path data_dir();

/// Per-spec: ~/.local/share/ash/saves/
fs::path saves_dir();

/// Per-spec: ~/.config/ash/config.json (D64)
fs::path config_dir();

/// Per-spec: ~/.local/share/ash/logs/ash.log
fs::path logs_dir();

/// Bundled content directory. Searched in this order:
///   $ASH_CONTENT_DIR
///   <exe-dir>/../share/ash/content
///   <exe-dir>/content
///   ./content  (cwd fallback)
fs::path content_dir();

/// Slot directory: <saves_dir()>/<savename>
fs::path slot_dir(const std::string& savename);

/// Ensure a directory exists (creates parents). Returns true if the
/// directory exists after the call.
bool ensure_dir(const fs::path& p);

/// Test-only: override the data_dir() result. Pass empty to clear.
void override_data_dir(const std::string& path);
void override_config_dir(const std::string& path);

}  // namespace core
}  // namespace ash
