#include "app/cli.hpp"

#include "core/log.hpp"
#include "core/version.hpp"
#include "render/ansi.hpp"

#include <fmt/format.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace ash
{
namespace cli
{
namespace
{
constexpr const char* kUsageText = R"(Usage: ash [options]
Options:
  -v, --version     Show version
  -h, --help        Show this help
      --log-level LEVEL   Set log level (trace|debug|info|warn|error|critical|off)
)";

bool is_long_flag(std::string_view token)
{
    return token.size() >= 3 && token[0] == '-' && token[1] == '-';
}

bool is_short_flag(std::string_view token)
{
    return token.size() >= 2 && token[0] == '-' && token[1] != '-';
}

/// Apply a log-level override at runtime if the user passed --log-level.
void apply_log_level_override(const CliArgs& args)
{
    if (args.log_level)
    {
        // Reuse init's parser via a one-shot setenv so init() picks it up.
        // Simpler: call set_level with a parsed value here.
        std::string name = *args.log_level;
        std::string lower;
        lower.reserve(name.size());
        for (char c : name)
        {
            lower.push_back(static_cast<char>(
                (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c));
        }
        if (lower == "trace")
        {
            ash::log::set_level(spdlog::level::trace);
        }
        else if (lower == "debug")
        {
            ash::log::set_level(spdlog::level::debug);
        }
        else if (lower == "info")
        {
            ash::log::set_level(spdlog::level::info);
        }
        else if (lower == "warn" || lower == "warning")
        {
            ash::log::set_level(spdlog::level::warn);
        }
        else if (lower == "error" || lower == "err")
        {
            ash::log::set_level(spdlog::level::err);
        }
        else if (lower == "critical" || lower == "crit")
        {
            ash::log::set_level(spdlog::level::critical);
        }
        else if (lower == "off" || lower == "none")
        {
            ash::log::set_level(spdlog::level::off);
        }
        else
        {
            std::fprintf(stderr,
                "Error: unknown log level '%s'\n"
                "Try 'ash --help' for usage.\n",
                name.c_str());
        }
    }
}

void print_version_line()
{
    std::string line = ash::ansi::set_fg(255, 80, 200)
        + "ASH v" + ash::core::ash_version_string()
        + " (" + ash::core::ash_version_codename() + ")"
        + ash::ansi::reset() + "\n";
    std::fwrite(line.data(), 1, line.size(), stdout);
    std::fflush(stdout);
}

void print_banner()
{
    std::string out;
    out += ash::ansi::set_fg(255, 80, 200);
    out += "ASH v";
    out += ash::core::ash_version_string();
    out += " (";
    out += ash::core::ash_version_codename();
    out += ")";
    out += ash::ansi::reset();
    out += "\n";
    out += ash::ansi::set_fg(80, 200, 255);
    out += "A hand-authored ASCII RPG";
    out += ash::ansi::reset();
    out += "\n";
    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
}
}  // namespace

std::string usage()
{
    return std::string(kUsageText);
}

CliArgs parse(int argc, char** argv)
{
    CliArgs out;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view tok(argv[i]);
        if (tok == "--version" || tok == "-v")
        {
            out.show_version = true;
        }
        else if (tok == "--help" || tok == "-h")
        {
            out.show_help = true;
        }
        else if (tok == "--log-level")
        {
            if (i + 1 >= argc)
            {
                out.ok = false;
                out.error_message = "--log-level requires a value";
                std::fprintf(stderr,
                    "Error: --log-level requires a value\n"
                    "Try 'ash --help' for usage.\n");
                return out;
            }
            out.log_level = std::string(argv[++i]);
        }
        else if (is_long_flag(tok) || is_short_flag(tok))
        {
            std::string name(tok);
            out.ok = false;
            out.error_message = "unknown flag '" + name + "'";
            std::fprintf(stderr,
                "Error: unknown flag '%s'\n"
                "Try 'ash --help' for usage.\n",
                name.c_str());
            return out;
        }
        else
        {
            // Positional argument. Phase 0 has none; treat as unknown.
            std::string name(tok);
            out.ok = false;
            out.error_message = "unknown argument '" + name + "'";
            std::fprintf(stderr,
                "Error: unknown argument '%s'\n"
                "Try 'ash --help' for usage.\n",
                name.c_str());
            return out;
        }
    }
    return out;
}

int cli_main(int argc, char** argv)
{
    ash::log::init();
    CliArgs args = parse(argc, argv);
    if (!args.ok)
    {
        return 1;
    }
    apply_log_level_override(args);

    if (args.show_help)
    {
        std::string text = usage();
        std::fwrite(text.data(), 1, text.size(), stdout);
        std::fflush(stdout);
        return 0;
    }

    if (args.show_version)
    {
        print_version_line();
        return 0;
    }

    print_banner();
    return 0;
}
}  // namespace cli
}  // namespace ash