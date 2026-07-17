#pragma once
/// Phase 01 step 0111: Animation definitions, registry, per-entity state.
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "render/palette.hpp"

namespace ash {
namespace render {

struct AnimFrame {
    int duration_ms = 0;
    uint32_t glyph = 0x20;
    std::optional<Color> fg;
    std::optional<Color> bg;
    int offset_x = 0;
    int offset_y = 0;
};

struct Animation {
    std::string id;
    bool loop = true;
    int  fps = 0;
    std::vector<AnimFrame> frames;
};

struct AnimationState {
    std::string anim_id;
    int elapsed_ms = 0;
    int current_frame = 0;
    bool finished = false;
};

class AnimationRegistry {
public:
    void load(std::filesystem::path const& root);
    void hot_reload(std::filesystem::path const& root);
    std::optional<Animation> get(std::string const& id) const;
    std::size_t size() const noexcept { return anims_.size(); }

private:
    std::map<std::string, Animation> anims_;
};

void animation_tick(AnimationState& s, Animation const& a, int dt_ms);

}  // namespace render
}  // namespace ash