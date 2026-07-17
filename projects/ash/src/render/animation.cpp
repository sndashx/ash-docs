#include "render/animation.hpp"

#include <fstream>
#include <utility>

namespace ash {
namespace render {

void tick(AnimationState& s, Animation const& a, int dt_ms) {
    if (s.finished || a.frames.empty()) {
        s.finished = true;
        return;
    }
    s.elapsed_ms += dt_ms;
    auto const& frame = a.frames[static_cast<std::size_t>(s.current_frame)];
    if (s.elapsed_ms < frame.duration_ms) return;

    // Advance at least one frame.
    int remaining = s.elapsed_ms;
    while (remaining >= 0 && !s.finished) {
        int const cur_idx = s.current_frame;
        int const cur_dur = a.frames[static_cast<std::size_t>(cur_idx)].duration_ms;
        remaining -= cur_dur;
        ++s.current_frame;
        if (s.current_frame >= static_cast<int>(a.frames.size())) {
            if (a.loop) {
                s.current_frame = 0;
            } else {
                s.current_frame = static_cast<int>(a.frames.size()) - 1;
                s.finished = true;
                break;
            }
        }
        if (remaining <= 0) break;
    }
    s.elapsed_ms = std::max(0, remaining);
}

void AnimationRegistry::insert(Animation a) {
    by_id_[a.id] = std::move(a);
}

bool AnimationRegistry::remove(std::string const& id) {
    return by_id_.erase(id) > 0;
}

void AnimationRegistry::clear() {
    by_id_.clear();
    watch_dir_.clear();
}

Animation const* AnimationRegistry::get(std::string const& id) const noexcept {
    auto it = by_id_.find(id);
    return it == by_id_.end() ? nullptr : &it->second;
}

void AnimationRegistry::load(std::filesystem::path const& dir) {
    watch_dir_ = dir;
    if (!std::filesystem::exists(dir)) return;
    for (auto const& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;
        std::ifstream in(entry.path());
        if (!in) continue;
        // Phase-0 parser stub: full JSON schema lands with content tooling
        // (Phase 12). For now we just register an empty placeholder keyed by
        // filename stem, so smoke / wiring tests succeed.
        Animation a;
        a.id = entry.path().stem().string();
        a.loop = true;
        a.fps = 8;
        a.frames.push_back(AnimFrame{100, 0x20, std::nullopt, std::nullopt, 0, 0});
        by_id_[a.id] = std::move(a);
    }
}

void AnimationRegistry::hot_reload() {
    if (watch_dir_.empty()) return;
    clear();
    load(watch_dir_);
}

}  // namespace render
}  // namespace ash