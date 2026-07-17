#pragma once
/// Phase 04: Cut / copy / paste buffer.
///
/// `Clipboard` is a singleton-like POD with `copy`, `cut`, and
/// `paste` operations that snap a sub-rectangle of one map layer and
/// optionally the entities within it. Paste pushes a series of
/// `Command`s onto an `UndoStack` so the user can roll back the
/// paste the same way as any other edit.
#include "editor/map.hpp"
#include "editor/undo.hpp"

#include <cstdint>
#include <vector>

namespace ash {
namespace editor {

struct Clipboard {
    IRect                        region{};     // Normalized rect in source coords.
    int                          layer = 0;    // Source layer (1..9).
    std::vector<render::Cell>    cells;        // Row-major, row 0 = region.y0.
    std::vector<EntitySpec>      entities;     // Entities captured in the region.
    std::uint64_t                next_entity_id = 1;  // Counter used during paste.
    bool                         has_data = false;

    /// Snapshot the in-bounds portion of `region` on `layer` and any
    /// entities whose `pos` lives inside `region`. Returns true on
    /// success and clears any prior content.
    bool copy(Map const& m, IRect region, int layer);

    /// Copy + delete (delete pushes nothing on its own; the caller is
    /// expected to push the matching DeleteCell / DeleteEntity commands
    /// onto their UndoStack if they want the cut to be reversible).
    void cut(Map& m, IRect region, int layer);

    /// Paste the captured snapshot with top-left at (x, y). `x`/`y`
    /// may be negative or out of bounds — out-of-range cells are
    /// clipped. Each pasted cell becomes its own PaintCellCommand;
    /// each pasted entity becomes a PlaceEntityCommand. Returns the
    /// number of cell-level writes performed.
    std::size_t paste(Map& m, int x, int y, UndoStack& undo);

    /// Free the buffer.
    void clear() noexcept;

    /// True if the clipboard has any content to paste.
    bool empty() const noexcept { return !has_data; }
};

}  // namespace editor
}  // namespace ash