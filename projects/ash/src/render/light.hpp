#pragma once
/// Phase 01 step 0108: Per-cell light intensity grid with additive lights,
/// 4-pass diffusion propagation, ambient term, and cell-shading application.
#include <cstdint>
#include <vector>

#include "core/rng.hpp"
#include "render/cell.hpp"

namespace ash {
namespace render {

struct LightGrid {
    std::vector<std::uint8_t> intensity;
    std::uint16_t width{0};
    std::uint16_t height{0};
    std::uint8_t  ambient{0};

    /// Resize and fill zeros (pitch dark).
    void resize(std::uint16_t w, std::uint16_t h);

    /// Clear all cells to zero (keep size).
    void clear() noexcept;

    /// Set explicit intensity at a single cell (clamped to [0,255]).
    void set(int x, int y, std::uint8_t v);

    /// Read intensity at (x, y). Returns 0 for OOB.
    std::uint8_t get(int x, int y) const noexcept;

    /// Add a radial light at (x, y) with given radius and intensity.
    /// Cells within radius get max(existing, max(0, intensity - d)).
    void add_light(int x, int y, int radius, std::uint8_t base_intensity);

    /// Same as add_light but with per-tick random intensity jitter.
    void add_flicker(int x, int y, int radius, std::uint8_t base_intensity,
                     core::rng::Xoshiro256pp& rng);

    /// Diffuse intensities with a 4-pass radial blur: each pass, cell =
    /// max(self, max of 4-neighbors). Smooths sharp falloffs.
    void propagate();

    /// Multiply fg/bg of the cell at (x, y) by (intensity + ambient) /
    /// 30.0, capped at 1.0. Mutates `c` in place.
    void apply_to(int x, int y, Cell& c, std::uint8_t ambient_override) const noexcept;
};

}  // namespace render
}  // namespace ash