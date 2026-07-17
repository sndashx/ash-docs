#include "world/components.hpp"

namespace ash {
namespace world {

core::IVec2 facing_vec(Facing f) noexcept {
    switch (f) {
        case Facing::East:  return core::IVec2{ 1,  0};
        case Facing::South: return core::IVec2{ 0,  1};
        case Facing::West:  return core::IVec2{-1,  0};
        case Facing::North: return core::IVec2{ 0, -1};
    }
    return core::IVec2{1, 0};
}

}  // namespace world
}  // namespace ash