#include "render/buffer_emit.hpp"

#include "render/ansi.hpp"

namespace ash {
namespace render {

namespace {

inline void append_num(std::string& s, int v) {
    s += std::to_string(v);
}

inline void append_move(std::string& s, int x, int y) {
    s += "\x1b[";
    append_num(s, y);
    s += ';';
    append_num(s, x);
    s += 'H';
}

inline void append_fg(std::string& s, uint8_t r, uint8_t g, uint8_t b) {
    s += "\x1b[38;2;";
    append_num(s, r);
    s += ';';
    append_num(s, g);
    s += ';';
    append_num(s, b);
    s += 'm';
}

inline void append_bg(std::string& s, uint8_t r, uint8_t g, uint8_t b) {
    s += "\x1b[48;2;";
    append_num(s, r);
    s += ';';
    append_num(s, g);
    s += ';';
    append_num(s, b);
    s += 'm';
}

}  // namespace

void encode_utf8(uint32_t cp, char* out, int& len) {
    if (cp <= 0x7F) {
        out[0] = static_cast<char>(cp);
        len = 1;
    } else if (cp <= 0x7FF) {
        out[0] = static_cast<char>(0xC0 | (cp >> 6));
        out[1] = static_cast<char>(0x80 | (cp & 0x3F));
        len = 2;
    } else if (cp <= 0xFFFF) {
        out[0] = static_cast<char>(0xE0 | (cp >> 12));
        out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (cp & 0x3F));
        len = 3;
    } else if (cp <= 0x10FFFF) {
        out[0] = static_cast<char>(0xF0 | (cp >> 18));
        out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (cp & 0x3F));
        len = 4;
    } else {
        out[0] = '?';
        len = 1;
    }
}

void buffer_emit(Buffer const& buf, std::string& out, std::vector<DirtyRect> const& dirty) {
    for (auto const& r : dirty) {
        if (r.w == 0 || r.h == 0) continue;
        int x0 = r.x;
        int y0 = r.y;
        int x1 = r.x + r.w;
        int y1 = r.y + r.h;
        if (x1 > buf.width)  x1 = buf.width;
        if (y1 > buf.height) y1 = buf.height;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 <= x0 || y1 <= y0) continue;

        int cur_x = 0, cur_y = 0;
        bool have_pos = false;

        for (int y = y0; y < y1; ++y) {
            int run_start = -1;
            uint8_t cur_fg_r = 0, cur_fg_g = 0, cur_fg_b = 0;
            uint8_t cur_bg_r = 0, cur_bg_g = 0, cur_bg_b = 0;
            uint8_t cur_flags = 0;
            bool style_emitted = false;

            auto flush_one = [&](int x) {
                Cell const& c = buf.cells[static_cast<std::size_t>(y) * buf.width + static_cast<std::size_t>(x)];
                if (!have_pos || cur_x != x || cur_y != y) {
                    append_move(out, x + 1, y + 1);
                    cur_x = x + 1;
                    cur_y = y;
                    have_pos = true;
                    style_emitted = false;
                }
                if (!style_emitted ||
                    c.fg_r != cur_fg_r || c.fg_g != cur_fg_g || c.fg_b != cur_fg_b) {
                    append_fg(out, c.fg_r, c.fg_g, c.fg_b);
                    cur_fg_r = c.fg_r; cur_fg_g = c.fg_g; cur_fg_b = c.fg_b;
                }
                if (!style_emitted ||
                    c.bg_r != cur_bg_r || c.bg_g != cur_bg_g || c.bg_b != cur_bg_b) {
                    append_bg(out, c.bg_r, c.bg_g, c.bg_b);
                    cur_bg_r = c.bg_r; cur_bg_g = c.bg_g; cur_bg_b = c.bg_b;
                }
                if (!style_emitted || c.flags != cur_flags) {
                    out += ansi::bold((c.flags & 0x01) != 0);
                    cur_flags = c.flags;
                }
                style_emitted = true;

                char enc[4];
                int  enc_len = 0;
                encode_utf8(c.glyph, enc, enc_len);
                out.append(enc, static_cast<std::size_t>(enc_len));
                cur_x += 1;
            };

            for (int x = x0; x < x1; ++x) {
                Cell const& c = buf.cells[static_cast<std::size_t>(y) * buf.width + static_cast<std::size_t>(x)];
                if (run_start < 0 ||
                    c.fg_r != cur_fg_r || c.fg_g != cur_fg_g || c.fg_b != cur_fg_b ||
                    c.bg_r != cur_bg_r || c.bg_g != cur_bg_g || c.bg_b != cur_bg_b) {
                    if (run_start >= 0) flush_one(run_start);
                    run_start = x;
                    cur_fg_r = c.fg_r; cur_fg_g = c.fg_g; cur_fg_b = c.fg_b;
                    cur_bg_r = c.bg_r; cur_bg_g = c.bg_g; cur_bg_b = c.bg_b;
                }
            }
            if (run_start >= 0) flush_one(run_start);
        }
        out += ansi::reset();
    }
}

}  // namespace render
}  // namespace ash