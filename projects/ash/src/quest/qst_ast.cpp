#include "quest/qst_ast.hpp"

#include <algorithm>

namespace ash {
namespace quest {

std::size_t ParsedQuest::find_stage(const std::string& id) const {
    auto it = std::lower_bound(
        stage_index.begin(), stage_index.end(), id,
        [](auto& p, auto const& s) { return p.first < s; });
    if (it != stage_index.end() && it->first == id) return it->second;
    return static_cast<std::size_t>(-1);
}

}  // namespace quest
}  // namespace ash