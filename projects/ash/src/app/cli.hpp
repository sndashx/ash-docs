#pragma once

/// Phase 00: CLI argument parsing for the ASH engine.
///
/// Public API:
///   - ash::cli::CliArgs        — struct holding parsed flags
///   - ash::cli::parse(argc,argv) — populate CliArgs; sets ok=false on
///                                  unknown flag and writes error to stderr
///   - ash::cli::usage()         — multi-line usage text
#include <optional>
#include <string>

namespace ash
{
namespace cli
{
struct CliArgs
{
    bool show_version = false;
    bool show_help = false;
    /// When set, override ASH_LOG_LEVEL for this run.
    std::optional<std::string> log_level;
    /// Any positional / unparsed tokens left after flag parsing.
    std::string error_message;
    bool ok = true;
};

/// Parse command-line arguments. On unknown flags, sets ok=false and
/// writes "Error: unknown flag '...'\nTry 'ash --help' for usage.\n"
/// to stderr.
CliArgs parse(int argc, char** argv);

/// Multi-line usage text. Ends with a trailing newline.
std::string usage();

/// Entry point used by main(). Initializes logging, parses CLI, and
/// dispatches: --version prints the version line, --help prints usage,
/// default prints the truecolor banner. Returns the process exit code.
int cli_main(int argc, char** argv);
}  // namespace cli
}  // namespace ash