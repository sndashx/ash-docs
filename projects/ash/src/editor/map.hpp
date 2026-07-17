#pragma once
/// Phase 04: In-game editor — Map and entity types.
///
/// The editor needs a concrete map to operate on. The full Phase 2
/// `world::Map` (entt ECS, collision grids, triggers) is still
/// scaffold-stubbed; the editor depends only on a minimal Cell grid
/// plus optional entity specs. We define what the editor needs here
/// rather than waiting for Phase 2 to land; the types are deliberately
/// POD and dependency-free so Phase 2 can swap them later.
#include "render/cell.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ash {
namespace editor {

/// Inclusive integer rectangle used for both selection boxes and
/// absolute map regions. Always kept in normalized form via
/// `normalize()` — callers may rely on `x0 <= x1` and `y0 <= y1`.
struct IRect {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;

    constexpr int width()  const noexcept { return x1 - x0 + 1; }
    constexpr int height() const noexcept { return y1 - y0 + 1; }
    constexpr int area()   const noexcept { return width() * height(); }
    constexpr bool empty() const noexcept { return width() <= 0 || height() <= 0; }

    constexpr bool contains(int x, int y) const noexcept {
        return x >= x0 && x <= x1 && y >= y0 && y <= y1;
    }

    /// Returns a rectangle sorted so `x0<=x1, y0<=y1`. Empty rect
    /// becomes `{0,0,-1,-1}` so `width()==0`.
    IRect normalize() const noexcept {
        IRect r;
        r.x0 = std::min(x0, x1);
        r.x1 = std::max(x0, x1);
        r.y0 = std::min(y0, y1);
        r.y1 = std::max(y0, y1);
        return r;
    }

    bool operator==(IRect const& o) const noexcept {
        return x0==o.x0 && y0==o.y0 && x1==o.x1 && y1==o.y1;
    }
};

/// Ordered integer point used by selection anchors and warp targets.
struct IVec2 {
    int x = 0;
    int y = 0;
    bool operator==(IVec2 const& o) const noexcept { return x==o.x && y==o.y; }
    bool operator!=(IVec2 const& o) const noexcept { return !(*this==o); }
};

/// Entity kinds the editor can place. Concrete enum captures all
/// authoring-tool slots per Phase 4 design doc.
enum class EntityKind : std::uint8_t {
    NPC         = 0,
    Item        = 1,
    Container   = 2,
    Door        = 3,
    Trigger     = 4,
    Light       = 5,
    Sign        = 6,
    Count       = 7,
};

/// Lightweight entity record. Mirrors what gets serialized in
/// sidecar JSON; Phase 2 will own the actual fields. We keep it
/// POD-shaped here so test code does not need entt.
struct EntitySpec {
    std::uint64_t id   = 0;       /// Stable across editor sessions.
    EntityKind    kind = EntityKind::Trigger;
    std::string   type_id;        /// Content registry id (e.g. "npc_bandit_01").
    IVec2         pos;            /// Cell coordinates.
    std::int32_t  layer = 4;      /// Entity layers per layer_spec convention.
    std::string   label;          /// Optional editor label.
};

/// 9 layers, single layer index. Layer 0 is "overlay/runtime" per
/// the design doc. Layer 9 is "entities top".
inline constexpr int kLayerCount = 9;

/// A map is a fixed-grid 9-layer stack of `render::Cell`s plus a
/// flat list of `EntitySpec`s. Operations on the editor mutate this
/// in place; the save flow reads through it via the writer.
struct Map {
    int width  = 80;   /// Cells per row.
    int height = 25;   /// Cells per column.
    std::string map_id = "untitled";

    /// `cells[layer][y*width + x]`. Layer index is 1..9 inclusive;
    /// index 0 is unused (layer 0 is the runtime overlay).
    std::vector<std::vector<render::Cell>> cells;

    /// Entity overlay across all entity-bearing layers.
    std::vector<EntitySpec> entities;

    Map() = default;
    Map(int w, int h) { resize(w, h); }

    /// Resize the grid; wipes existing cell contents to blank.
    void resize(int w, int h);
    /// True if (x, y) lives inside the grid.
    constexpr bool in_bounds(int x, int y) const noexcept {
        return x >= 0 && x < width && y >= 0 && y < height;
    }
    /// True if (x, y) lives on the requested layer (1..9).
    constexpr bool valid_layer(int layer) const noexcept {
        return layer >= 1 && layer <= kLayerCount;
    }
    /// Read a cell; returns CELL_BLANK when out of bounds.
    render::Cell const& at(int layer, int x, int y) const noexcept;
    /// Reference to a cell; expects valid coordinates.
    render::Cell& cell(int layer, int x, int y) noexcept;
};

}  // namespace editor
}  // namespace ash
