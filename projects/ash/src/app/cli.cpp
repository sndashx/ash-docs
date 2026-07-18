#include "app/cli.hpp"

#include <cstring>
#include <iostream>
#include <ostream>
#include <string>

namespace ash {
namespace cli {

namespace {

bool starts_with(const char* s, const char* prefix) {
    return std::strncmp(s, prefix, std::strlen(prefix)) == 0;
}

}  // namespace

CliArgs parse(int argc, char** argv) {
    CliArgs args;

    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];

        if (std::strcmp(a, "--version") == 0 || std::strcmp(a, "-v") == 0) {
            args.show_version = true;
        } else if (std::strcmp(a, "--help") == 0 || std::strcmp(a, "-h") == 0) {
            args.show_help = true;
        } else if (std::strcmp(a, "--render-test") == 0) {
            args.render_test = true;
        } else if (std::strcmp(a, "--char-test") == 0) {
            args.char_test = true;
        } else if (std::strcmp(a, "--editor-test") == 0) {
            args.editor_test = true;
        } else if (std::strcmp(a, "--ui-demo") == 0) {
            args.ui_demo = true;
        } else if (std::strcmp(a, "--perf-bench") == 0) {
            args.perf_bench = true;
        } else if (starts_with(a, "--log-level=")) {
            const char* value = a + std::strlen("--log-level=");
            if (*value == '\0') {
                std::cerr << "Error: --log-level requires a value\n"
                             "Try 'ash --help' for usage.\n";
                args.error = true;
                return args;
            }
            args.log_level = std::string(value);
        } else if (std::strcmp(a, "--log-level") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: --log-level requires a value\n"
                             "Try 'ash --help' for usage.\n";
                args.error = true;
                return args;
            }
            args.log_level = std::string(argv[++i]);
        } else if (starts_with(a, "--map=")) {
            const char* value = a + std::strlen("--map=");
            if (*value == '\0') {
                std::cerr << "Error: --map requires a value\n"
                             "Try 'ash --help' for usage.\n";
                args.error = true;
                return args;
            }
            args.map_id = std::string(value);
} else if (std::strcmp(a, "--map") == 0) {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Error: --map requires a value\n"
                             "Try 'ash --help' for usage.\n");
                args.error = true;
                return args;
            }
            args.map_id = std::string(argv[++i]);
        } else if (starts_with(a, "--script=")) {
            const char* value = a + std::strlen("--script=");
            if (*value == '\0') {
                std::fprintf(stderr, "Error: --script requires a value\n"
                             "Try 'ash --help' for usage.\n");
                args.error = true;
                return args;
            }
            args.script_path = std::string(value);
        } else if (std::strcmp(a, "--script") == 0) {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Error: --script requires a value\n"
                             "Try 'ash --help' for usage.\n");
                args.error = true;
                return args;
            }
            args.script_path = std::string(argv[++i]);
        } else {
            std::cerr << "Error: unknown flag '" << a << "'\n"
                         "Try 'ash --help' for usage.\n";
            args.error = true;
            return args;
        }
    }

    return args;
}

int usage(std::ostream& os) {
    const char* text =
        "Usage: ash [options]\n"
        "Options:\n"
        "  -v, --version           Show version\n"
        "  -h, --help              Show this help\n"
        "  --log-level LEVEL       Set log level (trace|debug|info|warn|error|critical|off)\n"
        "  --render-test           Run the render-test stub (Phase 1)\n"
        "  --char-test             Print derived stat table for starting player (Phase 6)\n"
        "  --editor-test           Drive the in-game editor stub (Phase 4)\n"
        "  --ui-demo               Drive the UI mode-stack demo (Phase 11)\n"
        "  --perf-bench            Run performance budgets D66/D67/D68 (Phase 11)\n"
        "  --map=ID                Select map for --render-test\n"
        "  --script=FILE           Read key events from FILE (for --ui-demo)\n";
    os << text;
    return static_cast<int>(std::strlen(text));
}

}  // namespace cli
}  // namespace ash