#include <catch2/catch_test_macros.hpp>

#include "render/ansi.hpp"

TEST_CASE("ansi::move_cursor", "[render][ansi]")
{
    REQUIRE(ash::ansi::move_cursor(1, 1) == "\x1b[1;1H");
    REQUIRE(ash::ansi::move_cursor(10, 5) == "\x1b[5;10H");
    REQUIRE(ash::ansi::move_cursor(80, 24) == "\x1b[24;80H");
}

TEST_CASE("ansi::set_fg", "[render][ansi]")
{
    REQUIRE(ash::ansi::set_fg(255, 80, 200) == "\x1b[38;2;255;80;200m");
    REQUIRE(ash::ansi::set_fg(0, 0, 0) == "\x1b[38;2;0;0;0m");
    REQUIRE(ash::ansi::set_fg(255, 255, 255) == "\x1b[38;2;255;255;255m");
}

TEST_CASE("ansi::set_bg", "[render][ansi]")
{
    REQUIRE(ash::ansi::set_bg(80, 200, 255) == "\x1b[48;2;80;200;255m");
    REQUIRE(ash::ansi::set_bg(20, 30, 80) == "\x1b[48;2;20;30;80m");
}

TEST_CASE("ansi::reset", "[render][ansi]")
{
    REQUIRE(ash::ansi::reset() == "\x1b[0m");
}