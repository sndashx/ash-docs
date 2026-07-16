#pragma once

/// Phase 00: ANSI escape helpers (truecolor, cursor, alt screen, etc.).
///
/// All functions return std::string containing a single escape sequence
/// (or a small composed sequence) suitable for writing directly to
/// stdout or to a captured buffer for the renderer.
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace ash
{
namespace ansi
{
/// "\x1b[{y};{x}H" — both 1-based.
std::string move_cursor(int x, int y);

/// "\x1b[38;2;{r};{g};{b}m" — 24-bit foreground.
std::string set_fg(std::uint8_t r, std::uint8_t g, std::uint8_t b);

/// "\x1b[48;2;{r};{g};{b}m" — 24-bit background.
std::string set_bg(std::uint8_t r, std::uint8_t g, std::uint8_t b);

/// "\x1b[0m" — reset all attributes.
std::string reset();

/// "\x1b[1m" or "\x1b[22m".
std::string bold(bool on);

/// "\x1b[4m" or "\x1b[24m".
std::string underline(bool on);

/// "\x1b[3m" or "\x1b[23m".
std::string italic(bool on);

/// "\x1b[7m" or "\x1b[27m".
std::string inverse(bool on);

/// "\x1b[?25l" — hide cursor.
std::string hide_cursor();

/// "\x1b[?25h" — show cursor.
std::string show_cursor();

/// "\x1b[?1049h" — switch to alternate screen buffer.
std::string alt_screen_on();

/// "\x1b[?1049l" — switch back to primary screen buffer.
std::string alt_screen_off();

/// "\x1b[2J\x1b[H" — erase screen and home the cursor.
std::string clear_screen();

/// "\x1b[2K" — erase the current row.
std::string clear_line();

/// "\x1b[s" — save current cursor position.
std::string save_cursor();

/// "\x1b[u" — restore the cursor saved by save_cursor().
std::string restore_cursor();

/// Stub for terminal size query. Returns nullopt in Phase 0; Phase 1
/// replaces this with a real ioctl-based implementation.
std::optional<std::pair<int, int>> query_terminal_size();
}  // namespace ansi
}  // namespace ash