#include "render/buffer_emit.hpp"

#include <cstdint>
#include <string>

#include "render/ansi.hpp"

namespace ash {
namespace render {

namespace {

inline void append_uint(std::string& s, unsigned v) {
    s += std::to_string(v);
}

inline void append_cell_color(std::string& s, Cell const& c) {
    s += ansi::set_fg(c.fg_r, c.fg_g, c.fg_b);
    s += ansi::set_bg(c.bg_r, c.bg_g, c.bg_b);
}

}  // namespace

void encode_utf8(std::uint32_t cp, char* out, int& len) {
    // RFC 3629 — restrict to U+0000..U+10FFFF, surrogates (D800..DFFF)
    // disallowed. Encode replacement char U+FFFD for invalid inputs.
    if (cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu)) {
        cp = 0xFFFDu;
    }
    if (cp <= 0x7Fu) {
        out[0] = static_cast<char>(cp);
        len = 1;
    } else if (cp <= 0x7FFu) {
        out[0] = static_cast<char>(0xC0u | (cp >> 6));
        out[1] = static_cast<char>(0x80u | (cp & 0x3Fu));
        len = 2;
    } else if (cp <= 0xFFFFu) {
        out[0] = static_cast<char>(0xE0u |  (cp >> 12));
        out[1] = static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = static_cast<char>(0x80u |  (cp & 0x3Fu));
        len = 3;
    } else {
        out[0] = static_cast<char>(0xF0u |  (cp >> 18));
        out[1] = static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = static_cast<char>(0x80u | ((cp >> 6)  & 0x3Fu));
        out[3] = static_cast<char>(0x80u |  (cp & 0x3Fu));
        len = 4;
    }
}

void emit(Buffer const& buf, std::string& out,
          std::vector<DirtyRect> const& dirty) {
    if (buf.width == 0 || buf.height == 0) return;

    for (DirtyRect const& r : dirty) {
        if (r.w == 0 || r.h == 0) continue;

        int prev_fg_r = -1, prev_fg_g = -1, prev_fg_b = -1;
        int prev_bg_r = -1, prev_bg_g = -1, prev_bg_b = -1;

        for (std::uint16_t ry = 0; ry < r.h; ++ry) {
            int const y = r.y + ry;
            for (std::uint16_t rx = 0; rx < r.w; ++rx) {
                int const x = r.x + rx;
                Cell const& c = buf.get(x, y);
                if (c.glyph == 0) continue;  // invisible cell skip

                // Move cursor (1-based for ANSI).
                out += "\x1b[";
                append_uint(out, static_cast<unsigned>(y + 1));
                out += ';';
                append_uint(out, static_cast<unsigned>(x + 1));
                out += 'H';

                // Emit color escape only when fg/bg differ from previous.
                if (c.fg_r != prev_fg_r || c.fg_g != prev_fg_g || c.fg_b != prev_fg_b
                    || c.bg_r != prev_bg_r || c.bg_g != prev_bg_g || c.bg_b != prev_bg_b) {
                    out += ansi::set_fg(c.fg_r, c.fg_g, c.fg_b);
                    out += ansi::set_bg(c.bg_r, c.bg_g, c.bg_b);
                    prev_fg_r = c.fg_r; prev_fg_g = c.fg_g; prev_fg_b = c.fg_b;
                    prev_bg_r = c.bg_r; prev_bg_g = c.bg_g; prev_bg_b = c.bg_b;
                }

                // Emit glyph bytes (UTF-8).
                char bytes[4];
                int  n = 0;
                encode_utf8(c.glyph, bytes, n);
                out.append(bytes, static_cast<std::size_t>(n));
            }
            prev_fg_r = prev_fg_g = prev_fg_b = -1;
            prev_bg_r = prev_bg_g = prev_bg_b = -1;
        }
    }
}

}  // namespace render
}  // namespace ash