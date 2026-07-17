#include "render/ansi.hpp"

namespace ash {
namespace ansi {

std::string move_cursor(int x, int y) {
    return "\x1b[" + std::to_string(y) + ";" + std::to_string(x) + "H";
}

std::string set_fg(uint8_t r, uint8_t g, uint8_t b) {
    return "\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g)
         + ";" + std::to_string(b) + "m";
}

std::string set_bg(uint8_t r, uint8_t g, uint8_t b) {
    return "\x1b[48;2;" + std::to_string(r) + ";" + std::to_string(g)
         + ";" + std::to_string(b) + "m";
}

std::string reset() {
    return "\x1b[0m";
}

std::string bold(bool on) {
    return on ? std::string("\x1b[1m") : std::string("\x1b[22m");
}

std::string underline(bool on) {
    return on ? std::string("\x1b[4m") : std::string("\x1b[24m");
}

std::string italic(bool on) {
    return on ? std::string("\x1b[3m") : std::string("\x1b[23m");
}

std::string inverse(bool on) {
    return on ? std::string("\x1b[7m") : std::string("\x1b[27m");
}

std::string hide_cursor() {
    return "\x1b[?25l";
}

std::string show_cursor() {
    return "\x1b[?25h";
}

std::string alt_screen_on() {
    return "\x1b[?1049h";
}

std::string alt_screen_off() {
    return "\x1b[?1049l";
}

std::string clear_screen() {
    return "\x1b[2J\x1b[H";
}

std::string clear_line() {
    return "\x1b[2K";
}

std::string save_cursor() {
    return "\x1b[s";
}

std::string restore_cursor() {
    return "\x1b[u";
}

std::optional<std::pair<int, int>> query_terminal_size() {
    return std::nullopt;
}

}  // namespace ansi
}  // namespace ash