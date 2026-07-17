#include "character/leveling.hpp"

#include <algorithm>

namespace ash {
namespace character {

int gain_xp(LevelState& out, int xp_gained) noexcept {
    if (xp_gained <= 0) {
        return 0;
    }
    out.xp += xp_gained;
    int levels = 0;
    while (out.level < kLevelMax && out.xp >= out.xp_to_next) {
        out.xp -= out.xp_to_next;
        out.level += 1;
        out.attribute_points += kAttributesPerLevel;
        out.skill_pool += kSkillPointsPerLevel;
        out.xp_to_next = xp_to_next(out.level);
        ++levels;
    }
    if (out.level >= kLevelMax) {
        out.xp_to_next = 0;
        out.xp = std::min(out.xp, 0);
    }
    return levels;
}

}  // namespace character
}  // namespace ash