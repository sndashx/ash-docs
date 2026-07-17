#include "render/animation.hpp"

#include <fstream>
#include <sstream>

namespace ash {
namespace render {

namespace {

// Minimal hand-rolled parser for the simplified animation format:
//   id: <name>
//   loop: true|false
//   fps: 8
//   frames:
//     - duration_ms: 100
//       glyph: 'V'        (uint32 codepoint)
//       fg: 255,220,80
//       offset: 0,0
Animation parse_animation_file(std::filesystem::path const& file) {
    Animation anim;
    std::ifstream in(file);
    std::string line;
    AnimFrame cur;
    bool in_frames = false;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string key;
        if (!std::getline(iss, key, ':')) continue;
        std::string val;
        std::getline(iss, val);
        if (!val.empty() && val[0] == ' ') val.erase(0, 1);
        if (key == "id")       anim.id = val;
        else if (key == "loop") anim.loop = (val == "true" || val == "1");
        else if (key == "fps")  anim.fps = std::atoi(val.c_str());
        else if (key == "frames") {
            in_frames = true;
            cur = AnimFrame{};
        } else if (in_frames) {
            if (key == "  - duration_ms") cur.duration_ms = std::atoi(val.c_str());
            else if (key == "    glyph")    cur.glyph = static_cast<uint32_t>(val.size() ? val[0] : 0x20);
            else if (key == "    offset") {
                int ox = 0, oy = 0;
                if (std::sscanf(val.c_str(), "%d,%d", &ox, &oy) == 2) {
                    cur.offset_x = ox;
                    cur.offset_y = oy;
                }
            } else if (key == "  - end") {
                anim.frames.push_back(cur);
                cur = AnimFrame{};
            }
        }
    }
    if (in_frames && cur.duration_ms >= 0 && !cur.glyph && anim.frames.empty()) {
        anim.frames.push_back(cur);
    }
    return anim;
}

}  // namespace

void AnimationRegistry::load(std::filesystem::path const& root) {
    anims_.clear();
    if (!std::filesystem::exists(root)) return;
    for (auto const& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        Animation anim = parse_animation_file(entry.path());
        if (!anim.id.empty()) anims_[anim.id] = std::move(anim);
    }
}

void AnimationRegistry::hot_reload(std::filesystem::path const& root) {
    load(root);
}

std::optional<Animation> AnimationRegistry::get(std::string const& id) const {
    auto it = anims_.find(id);
    if (it == anims_.end()) return std::nullopt;
    return it->second;
}

void animation_tick(AnimationState& s, Animation const& a, int dt_ms) {
    if (a.frames.empty() || s.finished) return;
    std::size_t frame_idx = static_cast<std::size_t>(s.current_frame);
    s.elapsed_ms += dt_ms;
    while (frame_idx < a.frames.size() &&
           s.elapsed_ms >= a.frames[frame_idx].duration_ms) {
        s.elapsed_ms -= a.frames[frame_idx].duration_ms;
        if (frame_idx + 1 >= a.frames.size()) {
            if (!a.loop) {
                s.current_frame = static_cast<int>(a.frames.size()) - 1;
                s.finished = true;
                s.elapsed_ms = 0;
                return;
            }
            frame_idx = 0;
        } else {
            ++frame_idx;
        }
    }
    s.current_frame = static_cast<int>(frame_idx);
}

}  // namespace render
}  // namespace ash