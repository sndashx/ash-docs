#pragma once
/// Phase 01 step 0101: Packed 12-byte cell. 4 (glyph) + 3 (fg) + 3 (bg) + 1 (flags)
/// = 11 bytes of payload + 1 byte tail padding under typical Itanium ABI alignment.
#include <cstdint>

namespace ash {
namespace render {

struct Cell {
    uint32_t glyph;
    uint8_t  fg_r;
    uint8_t  fg_g;
    uint8_t  fg_b;
    uint8_t  bg_r;
    uint8_t  bg_g;
    uint8_t  bg_b;
    uint8_t  flags;

    constexpr Cell() noexcept
        : glyph(0x20), fg_r(0), fg_g(0), fg_b(0), bg_r(0), bg_g(0), bg_b(0), flags(0) {}

    constexpr Cell(uint32_t g,
                   uint8_t fr, uint8_t fg, uint8_t fb) noexcept
        : glyph(g), fg_r(fr), fg_g(fg), fg_b(fb),
          bg_r(0), bg_g(0), bg_b(0), flags(0) {}

    constexpr Cell(uint32_t g,
                   uint8_t fr, uint8_t fgcol, uint8_t fb,
                   uint8_t br, uint8_t bg, uint8_t bb,
                   uint8_t f) noexcept
        : glyph(g), fg_r(fr), fg_g(fgcol), fg_b(fb),
          bg_r(br), bg_g(bg), bg_b(bb), flags(f) {}
};

namespace cell_flag {
inline constexpr uint8_t NONE      = 0;
inline constexpr uint8_t BOLD      = 1;
inline constexpr uint8_t ITALIC    = 2;
inline constexpr uint8_t UNDERLINE = 4;
inline constexpr uint8_t BLINK     = 8;
inline constexpr uint8_t INVERSE   = 16;
inline constexpr uint8_t STRIKE    = 32;
inline constexpr uint8_t DIM       = 64;
}  // namespace cell_flag

inline constexpr Cell CELL_BLANK = { 0x20, 0, 0, 0, 0, 0, 0, 0 };

static_assert(sizeof(Cell) <= 12,
              "Cell must fit in 12 bytes; verify pack padding matches ABI expectation");

}  // namespace render
}  // namespace ash
