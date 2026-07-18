#pragma once
/// Phase 0: minimal logging stubs. spdlog v1.12.x will replace these in a
/// later phase; the API surface here is what the rest of the engine
/// already calls (init / set_level).
#include <string>

namespace ash {
namespace log {

/// Initialize the logging subsystem. Safe to call multiple times.
void init();

/// Set the active log level by name (trace|debug|info|warn|error|critical|off).
/// Unknown names are ignored.
void set_level(const std::string& level);

/// Emit an informational line. Phase 10 added this so save / migrate can
/// surface "saving", "loading", "migration applied" lines without pulling
/// in spdlog. Phase 12 will replace these with real log calls.
void info(const std::string& msg);

/// Emit a warning line.
void warn(const std::string& msg);

/// Emit an error line.
void error(const std::string& msg);

}  // namespace log
}  // namespace ash