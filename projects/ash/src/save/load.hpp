#pragma once
/// Phase 10: Load manager.
#include <string>

#include "save/save_data.hpp"

namespace ash {
namespace save {

struct LoadResult {
    bool        ok{false};
    std::string error;
    SaveData    data;
    SaveMeta    meta;
    /// Wall-clock milliseconds spent reading + parsing.
    long long   elapsed_ms{0};
};

/// Load the save at <saves_dir>/<savename>/. On schema mismatch the
/// runner applies migrations; if the chain can't reach the engine
/// version the call fails.
LoadResult load(const std::string& savename);

/// Load, with a fallback to bak1 / bak2 if the primary is missing or
/// corrupt. The most recent valid generation wins.
LoadResult load_with_fallback(const std::string& savename);

}  // namespace save
}  // namespace ash