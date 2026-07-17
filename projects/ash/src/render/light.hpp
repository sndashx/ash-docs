#pragma once
/// Phase 01 step 0108: Light grid with radial-falloff lights and propagation.
#include <cstdint>
#include <vector>

#include "core/rng.hpp"
#include "render/cell.hpp"

namespace ash {
namespace render {

struct LightGrid {
    std::vector<uint8_t> intensity;
    uint16_t width  = 0;
    uint16_t height = 0;
    uint8_t  ambient = 0;

    void resize(uint16_t w, uint16_t h);
    void set(int x, int y, uint8_t v) noexcept;
    uint8_t get(int x, int y) const noexcept;

    void add_light(int x, int y, uint8_t radius, uint8_t intensity_val) noexcept;
    void add_flicker(int x, int y, uint8_t radius, uint8_t base) noexcept;
    void propagate();

    void apply_to(Cell& c, uint8_t ambient_val) const noexcept;
};

}  // namespace render
}  // namespace ash