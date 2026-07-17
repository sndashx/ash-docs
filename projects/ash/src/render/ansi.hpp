#pragma once
/// Phase 00 step 0006: pure ANSI escape sequence helpers (D11 truecolor).
/// All functions return std::string and use no global state.
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace ash {
namespace ansi {

std::string move_cursor(int x, int y);
std::string set_fg(uint8_t r, uint8_t g, uint8_t b);
std::string set_bg(uint8_t r, uint8_t g, uint8_t b);
std::string reset();
std::string bold(bool on);
std::string underline(bool on);
std::string italic(bool on);
std::string inverse(bool on);

std::string hide_cursor();
std::string show_cursor();
std::string alt_screen_on();
std::string alt_screen_off();
std::string clear_screen();
std::string clear_line();

std::string save_cursor();
std::string restore_cursor();

/// Emits the DCS request `\x1b[18t` to ask the terminal for window size in cells.
/// Full response parsing belongs to a later input pump. For now returns nullopt;
/// callers should use Terminal::query() (Phase 01 step 0102) for window size.
std::optional<std::pair<int, int>> query_terminal_size();

}  // namespace ansi
}  // namespace ash