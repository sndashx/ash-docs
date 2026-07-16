#include "render/ansi.hpp"

#include <fmt/format.h>

namespace ash
{
namespace ansi
{
namespace
{
constexpr const char* kEsc = "\x1b";
constexpr const char* kCsi = "\x1b[";
}  // namespace

std::string move_cursor(int x, int y)
{
    return fmt::format("\x1b[{};{}H", y, x);
}

std::string set_fg(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return fmt::format("\x1b[38;2;{};{};{}m",
        static_cast<unsigned>(r),
        static_cast<unsigned>(g),
        static_cast<unsigned>(b));
}

std::string set_bg(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    return fmt::format("\x1b[48;2;{};{};{}m",
        static_cast<unsigned>(r),
        static_cast<unsigned>(g),
        static_cast<unsigned>(b));
}

std::string reset()
{
    return std::string(kCsi) + "0m";
}

std::string bold(bool on)
{
    return std::string(kCsi) + (on ? "1m" : "22m");
}

std::string underline(bool on)
{
    return std::string(kCsi) + (on ? "4m" : "24m");
}

std::string italic(bool on)
{
    return std::string(kCsi) + (on ? "3m" : "23m");
}

std::string inverse(bool on)
{
    return std::string(kCsi) + (on ? "7m" : "27m");
}

std::string hide_cursor()
{
    return std::string(kEsc) + "[?25l";
}

std::string show_cursor()
{
    return std::string(kEsc) + "[?25h";
}

std::string alt_screen_on()
{
    return std::string(kEsc) + "[?1049h";
}

std::string alt_screen_off()
{
    return std::string(kEsc) + "[?1049l";
}

std::string clear_screen()
{
    return std::string(kEsc) + "[2J" + std::string(kEsc) + "[H";
}

std::string clear_line()
{
    return std::string(kCsi) + "2K";
}

std::string save_cursor()
{
    return std::string(kEsc) + "[s";
}

std::string restore_cursor()
{
    return std::string(kEsc) + "[u";
}

std::optional<std::pair<int, int>> query_terminal_size()
{
    // Phase 0 stub. Phase 1 (src/render/terminal.cpp) replaces this with
    // a real ioctl(TIOCGWINSZ) implementation.
    return std::nullopt;
}
}  // namespace ansi
}  // namespace ash