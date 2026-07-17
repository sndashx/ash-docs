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
///   lines 2..10 : 9 layer rows; each row is `width` UTF-32 codepoints
///   followed by `width*7` RGB bytes (fg_r fg_g fg_b bg_r bg_g bg_b flags).
///   The encoder emits them as space-separated hex bytes for
///   easy diffing. Entities are written on a line `E id kind type x y
///   layer label`.
bool save_map(Map const& m, std::string const& path);

/// Read a map from the format produced by `save_map`. Returns true on
/// success; on failure the input `m` is left unmodified.
bool load_map(Map& m, std::string const& path);

}  // namespace editor
}  // namespace ash