#include "editor/map.hpp"

namespace ash {
namespace editor {

void Map::resize(int w, int h) {
    width  = w;
    height = h;
    std::size_t const N = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
    cells.assign(static_cast<std::size_t>(kLayerCount + 1),
                 std::vector<render::Cell>(N, render::CELL_BLANK));
}

render::Cell const& Map::at(int layer, int x, int y) const noexcept {
    static render::Cell const blank = render::CELL_BLANK;
    if (!valid_layer(layer) || !in_bounds(x, y)) return blank;
    return cells[static_cast<std::size_t>(layer)]
               [static_cast<std::size_t>(y) * static_cast<std::size_t>(width)
                + static_cast<std::size_t>(x)];
}

render::Cell& Map::cell(int layer, int x, int y) noexcept {
    return cells[static_cast<std::size_t>(layer)]
              [static_cast<std::size_t>(y) * static_cast<std::size_t>(width)
               + static_cast<std::size_t>(x)];
}

}  // namespace editor
}  // namespace ash
