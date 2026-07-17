#pragma once
/// Phase 01 step 0106: Encode a buffer's dirty regions as ANSI escape
/// sequences ready for terminal output. Adjacent cells with identical
/// fg/bg are coalesced so redundant colour changes are elided.
#include <cstdint>
#include <string>
#include <vector>

#include "render/buffer.hpp"
#include "render/buffer_diff.hpp"

namespace ash {
namespace render {

/// Append ANSI-encoded glyphs for every dirty rect in `buf` to `out`.
void emit(Buffer const& buf, std::string& out, std::vector<DirtyRect> const& dirty);

/// Encode a Unicode code point as UTF-8 bytes (RFC 3629). Caller passes a
/// buffer of at least 4 bytes. `len` receives the number of bytes written.
void encode_utf8(std::uint32_t codepoint, char* out, int& len);

}  // namespace render
}  // namespace ash