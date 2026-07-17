#pragma once
/// Minimal test harness shared by unit-test files when Catch2 is not
/// available. Provides `TEST_CASE(name, tags)`, `REQUIRE(cond)`, and a
/// `RUN_ALL_TESTS()` entry point invoked from test_main.cpp.
#if __has_include(<catch2/catch_test_macros.hpp>)
// Catch2 available: just use its macros; nothing to do here.
#else
#include <cstdio>
#include <vector>

namespace ash_test_runtime {
struct Entry { char const* name; void (*fn)(); };
inline std::vector<Entry>& cases() {
    static std::vector<Entry> v;
    return v;
}
struct AutoRegister {
    AutoRegister(char const* n, void (*f)()) { cases().push_back({n, f}); }
};
}  // namespace ash_test_runtime

extern int  ash_test_failures;
extern int  ash_test_checks;
extern char const* ash_test_current;

struct AshCheck {
    bool ok;
    AshCheck(bool c)
        : ok(c) {
        ++ash_test_checks;
        if (!ok) {
            ++ash_test_failures;
            std::fprintf(stderr, "    FAIL [%s]: %s:%d: assertion failed\n",
                         ash_test_current, __FILE__, __LINE__);
        }
    }
};

#define ASH_PASTE2(a, b) a##b
#define ASH_PASTE(a, b) ASH_PASTE2(a, b)
#define ASH_CAT_INNER(a, b) a##b
#define ASH_CAT(a, b) ASH_CAT_INNER(a, b)
/// Captures __COUNTER__ once via a two-step expansion so each TEST_CASE
/// invocation produces exactly one token. The `__COUNTER__` here is
/// the global counter; using it through indirection ensures each use
/// of `TEST_CASE` increments by exactly 1 regardless of how many times
/// the macro references the inner identifier.
#define ASH_ONCE(name) ASH_CAT(name, __COUNTER__)
#define REQUIRE(cond) do { AshCheck _c((cond)); if (!_c.ok) return; } while (0)
/// Standard technique: use a helper macro that defers counter expansion.
#define TEST_CASE_INNER(name, suffix)                                       \
    static void ASH_CAT(ash_t_, suffix)();                                  \
    static ::ash_test_runtime::AutoRegister                                 \
        ASH_CAT(ash_r_, suffix)(name, &ASH_CAT(ash_t_, suffix));            \
    static void ASH_CAT(ash_t_, suffix)()
#define TEST_CASE(name, tags) TEST_CASE_INNER(name, __COUNTER__)

#endif
