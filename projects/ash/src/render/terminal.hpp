#pragma once
/// Phase 01 step 0102: Terminal size + capability query.
#include <cstdint>
#include <cstdlib>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace ash {
namespace render {

struct Terminal {
    uint16_t width_cells   = 80;
    uint16_t height_cells  = 24;
    uint16_t cell_pixel_w  = 8;
    uint16_t cell_pixel_h  = 16;
    bool     truecolor     = false;

    Terminal() = default;
    Terminal(uint16_t w, uint16_t h) : width_cells(w), height_cells(h) {}

    std::size_t cell_count() const noexcept {
        return static_cast<std::size_t>(width_cells) *
               static_cast<std::size_t>(height_cells);
    }
};

namespace terminal_internal {
inline bool env_truecolor() {
    if (const char* c = std::getenv("COLORTERM")) {
        std::string v(c);
        return v == "truecolor" || v == "24bit";
    }
    return false;
}

inline bool read_env_int(const char* name, uint16_t& out) {
    if (const char* v = std::getenv(name)) {
        if (*v == '\0') return false;
        int parsed = std::atoi(v);
        if (parsed > 0 && parsed < 100000) {
            out = static_cast<uint16_t>(parsed);
            return true;
        }
    }
    return false;
}
}  // namespace terminal_internal

inline Terminal terminal_query() {
    Terminal t;

#if defined(__unix__) || defined(__APPLE__)
    {
        struct winsize ws{};
        if (ioctl(0, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
            t.width_cells  = static_cast<uint16_t>(ws.ws_col);
            t.height_cells = static_cast<uint16_t>(ws.ws_row);
            if (ws.ws_xpixel > 0) {
                t.cell_pixel_w = static_cast<uint16_t>(ws.ws_xpixel / ws.ws_col);
            }
            if (ws.ws_ypixel > 0) {
                t.cell_pixel_h = static_cast<uint16_t>(ws.ws_ypixel / ws.ws_row);
            }
        }
    }
#endif

    if (t.width_cells == 0 || t.width_cells == 80) {
        if (!terminal_internal::read_env_int("COLUMNS", t.width_cells)) {
            t.width_cells = 80;
        }
    }
    if (t.height_cells == 0 || t.height_cells == 24) {
        if (!terminal_internal::read_env_int("LINES", t.height_cells)) {
            t.height_cells = 24;
        }
    }

    t.truecolor = terminal_internal::env_truecolor();
    return t;
}

}  // namespace render
}  // namespace ash