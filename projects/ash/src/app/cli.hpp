#pragma once
/// Phase 0 step 0007: CLI argument parsing.
#include <optional>
#include <ostream>
#include <string>

namespace ash {
namespace cli {

struct CliArgs {
    bool show_version = false;
    bool show_help = false;
    bool render_test = false;
    bool char_test = false;
    bool editor_test = false;
    std::optional<std::string> log_level;
    std::optional<std::string> map_id;
    bool error = false;
};

/// Parse argv into CliArgs. On unknown flag, prints to stderr and sets
/// args.error = true; caller should exit non-zero.
CliArgs parse(int argc, char** argv);

/// Print the usage block to the given stream. Returns number of bytes written.
int usage(std::ostream& os);

}  // namespace cli
}  // namespace ash