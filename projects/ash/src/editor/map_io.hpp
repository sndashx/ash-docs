#pragma once
/// Phase 04: Map save / load (in-memory text format).
///
/// Phase 4 ships its own compact text format for the editor. The
/// `xp_writer` from Phase 3 is a real binary format; the editor's
/// own loader is intentionally tiny (line-based, 9 layers one per
/// line) so it can be implemented before Phase 2's world layer
/// lands. The two will be reconciled in Phase 11.
#include "editor/map.hpp"

#include <string>

namespace ash {
namespace editor {

/// Write the map to a path. Format:
///   line 1   : `ASH-MAP-V1 <w> <h> <map_id>`
///   lines 2..10 : 9 layer rows; each row is `width` cells. Each cell
///   is written as four space-separated hex tokens:
///     <glyph:8> <fg:6> <bg:6> <flags:2>
///   where `glyph` is an 8-char lowercase hex UTF-32 codepoint,
///   `fg`/`bg` are 6-char hex (rrggbb), and `flags` is 2-char hex.
///   Entities are written on a line `E id kind type x y layer label`.
bool save_map(Map const& m, std::string const& path);

/// Read a map from the format produced by `save_map`. Returns true on
/// success; on failure the input `m` is left unmodified.
bool load_map(Map& m, std::string const& path);

}  // namespace editor
}  // namespace ash