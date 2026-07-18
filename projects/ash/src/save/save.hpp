#pragma once
/// Phase 10: Save manager (per pillar 9 + appendix D + 10.05).
///
/// Writes the slot directory atomically:
///   1. Build all files into <saves_dir>/<name>.tmp/
///   2. fsync each file + the directory.
///   3. Rename .tmp -> <name> (atomic on POSIX).
///   4. Rotate backups: <name>.bak2 deleted, <name>.bak1 -> bak2,
///      <name> -> bak1. The new directory is the new current.
///
/// Trigger sources per 10.06:
///   - Manual save (player-initiated).
///   - F5 quicksave (slot "__quick__").
///   - Autosave every 10 minutes (slot "__autosave__").
///   - On map transition.
#include <filesystem>
#include <string>

#include "save/save_data.hpp"

namespace ash {
namespace save {

struct SaveResult {
    bool         ok{false};
    std::string  error;        /// empty on success
    std::filesystem::path path;
    /// Wall-clock milliseconds spent serializing and writing.
    long long    elapsed_ms{0};
};

/// Pre-defined slot names per 10.06.
inline constexpr const char* kAutosaveSlot = "__autosave__";
inline constexpr const char* kQuicksaveSlot = "__quick__";

/// Save the game state to <saves_dir>/<savename>/.
/// On success returns `SaveResult{ok=true, ...}`; on failure rolls back
/// the temporary directory and returns the error message.
SaveResult save(const std::string& savename, const SaveData& data);

/// Save + serialize-then-deserialize to validate (used by tests). Returns
/// the roundtripped data on success.
SaveResult save_with_meta(const std::string& savename, const SaveData& data,
                          SaveMeta& out_meta);

/// True if a save directory exists for `savename`.
bool exists(const std::string& savename);

/// Delete a save slot. Removes the current + both backups. Returns false
/// if the slot did not exist.
bool remove(const std::string& savename);

/// Return the full filesystem path for a slot directory.
std::filesystem::path path_for(const std::string& savename);

/// Lightweight metadata about a slot (read from `meta.json` without
/// loading the player/world payloads).
struct SlotInfo {
    std::string savename;
    std::string save_name;
    int         player_level{0};
    std::string player_map;
    std::int64_t last_played_at{0};
    int         play_time_seconds{0};
};

/// Enumerate known slots. Scans the saves directory for entries that
/// contain a `meta.json`.
std::vector<SlotInfo> list_slots();

}  // namespace save
}  // namespace ash