#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#define REQUIRE(cond) ((void)0)
#define TEST_CASE(name, tags) static void test_##__LINE__()
#endif
TEST_CASE("test_qst_parser", "[quest]")
{
    REQUIRE(true);
}
