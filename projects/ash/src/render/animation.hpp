#pragma once
/// Phase 01 step 0111: Animation registry + per-entity playback state.
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "render/palette.hpp"

namespace ash {
namespace render {

struct AnimFrame {
    int           duration_ms{0};
    std::uint32_t glyph{0x20};
    std::optional<Color> fg;
    std::optional<Color> bg;
    int           offset_x{0};
    int           offset_y{0};
};

struct Animation {
    std::string              id;
    bool                     loop{true};
    int                      fps{8};
    std::vector<AnimFrame>   frames;
};

struct AnimationState {
    std::string anim_id;
    int         elapsed_ms{0};
    int         current_frame{0};
    bool        finished{false};
};

/// Advance an AnimationState by `dt_ms` against the matching Animation.
/// Wraps frame index on loop, sets finished=true on terminal non-loop.
void tick(AnimationState& s, Animation const& a, int dt_ms);

/// Convenience: build a state bound to `anim_id`. Frame 0, elapsed 0.
inline AnimationState make_state(std::string const& id) {
    AnimationState s;
    s.anim_id = id;
    return s;
}

class AnimationRegistry {
public:
    AnimationRegistry() = default;

    /// Load all animations from a directory of JSON files.
    /// Failures are ignored (logged at info by caller if desired).
    void load(std::filesystem::path const& dir);

    /// Look up an animation by id. Returns nullptr if missing.
    Animation const* get(std::string const& id) const noexcept;

    /// Re-scan the directory and refresh contents (dev mode hot-reload).
    void hot_reload();

    /// Manual insert/remove (used by tests and content tooling).
    void insert(Animation a);
    bool remove(std::string const& id);
    void clear();

    std::size_t size() const noexcept { return by_id_.size(); }

private:
    std::unordered_map<std::string, Animation> by_id_;
    std::filesystem::path                      watch_dir_;
};

}  // namespace render
}  // namespace ash