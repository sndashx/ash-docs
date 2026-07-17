#pragma once
/// Phase 06: XP curve, attribute points, skill pool (Pillar 5 + phase
/// spec 11-phase-06-character.txt step 0602).
/// xp_to_next(n) = 100 * n * n. Level 1 -> 2: 100 xp. Level 10 -> 11:
/// 10,000 xp. The curve is intentionally quadratic and tunable; the
/// tests pin it so future tunings are intentional.
#include <cstdint>

namespace ash {
namespace character {

inline constexpr int kLevelMin       = 1;
inline constexpr int kLevelMax       = 100;
inline constexpr int kLevelDefault   = 1;
inline constexpr int kAttributesPerLevel = 3;   /// Spec.
inline constexpr int kSkillPointsPerLevel  = 1;  /// Spec (skill points granted).

/// XP needed to advance FROM `level` to `level+1`.
///   xp_to_next(1) = 100, xp_to_next(10) = 10,000.
constexpr int xp_to_next(int level) noexcept {
    if (level < kLevelMin) {
        level = kLevelMin;
    }
    int const n = level;
    return 100 * n * n;
}

struct LevelState {
    int level{kLevelDefault};
    int xp{0};
    int xp_to_next{::ash::character::xp_to_next(1)};
    int attribute_points{kAttributesPerLevel};  /// Granted at level-up.
    int skill_pool{kSkillPointsPerLevel};
};

/// Apply `xp_gained` (>=0). Handles any number of level-ups in one call.
/// Returns the number of levels gained (>=0). The `out` LevelState is
/// updated in-place.
int gain_xp(LevelState& out, int xp_gained) noexcept;

}  // namespace character
}  // namespace ash