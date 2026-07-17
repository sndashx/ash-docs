#pragma once
/// Phase 01 step 0106: Buffer → ANSI string emitter for a dirty-rect list.
#include <cstdint>
#include <string>
#include <vector>

#include "render/buffer.hpp"
#include "render/buffer_diff.hpp"

namespace ash {
namespace render {

void buffer_emit(Buffer const& buf,
                 std::string& out,
                 std::vector<DirtyRect> const& dirty);

void encode_utf8(uint32_t codepoint, char* out, int& len);

}  // namespace render
}  // namespace ash