// Catch2 entry point. When Catch2 is not available (offline scaffold),
// we fall back to a minimal custom harness so assertions actually run.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

int main(int argc, char** argv)
{
    return Catch::Session().run(argc, argv);
}
#else
#include "test_harness.hpp"
#include <cstdio>

int        ash_test_failures = 0;
int        ash_test_checks   = 0;
char const* ash_test_current = "<unknown>";

int main(int /*argc*/, char** /*argv*/)
{
    int failures = 0;
    int total    = 0;
    for (auto& e : ash_test_runtime::cases()) {
        ++total;
        ash_test_failures = 0;
        ash_test_checks   = 0;
        ash_test_current  = e.name;
        std::fprintf(stderr, "RUN   %s\n", e.name);
        e.fn();
        if (ash_test_failures == 0) {
            std::fprintf(stderr, "OK    %s (%d checks)\n", e.name, ash_test_checks);
        } else {
            std::fprintf(stderr, "FAIL  %s (%d/%d checks failed)\n",
                         e.name, ash_test_failures, ash_test_checks);
            ++failures;
        }
    }
    std::fprintf(stderr, "\n%d/%d tests passed\n", total - failures, total);
    return failures == 0 ? 0 : 1;
}

TEST_CASE("smoke test", "[core]")
{
    REQUIRE(true);
}
#endif
