#include "render/animation.hpp"

#include <fstream>
#include <sstream>
#include <utility>

#include "core/json.hpp"

namespace ash {
namespace render {

namespace {

Animation parse_animation_json(std::string const& text,
                               std::string const& fallback_id) {
    Animation a;
    a.id = fallback_id;
    a.loop = true;
    a.fps = 8;
    a.frames.push_back(AnimFrame{100, 0x20, std::nullopt, std::nullopt, 0, 0});
    try {
        auto j = json::parse(text);
        if (!j.is_object()) return a;
        a.id  = j["id"].str_or(fallback_id);
        a.loop = j["loop"].bool_or(true);
        a.fps  = static_cast<int>(j["fps"].int_or(8));
        if (j.contains("frames") && j["frames"].is_array()) {
            auto const& arr = j["frames"].as_array();
            a.frames.clear();
            for (auto const& fv : arr) {
                if (!fv.is_object()) continue;
                AnimFrame f;
                f.duration_ms = static_cast<int>(fv["duration_ms"].int_or(100));
                std::string glyph = fv["glyph"].str_or(" ");
                if (!glyph.empty()) {
                    f.glyph = static_cast<std::uint32_t>(static_cast<unsigned char>(glyph[0]));
                }
                if (fv.contains("fg") && fv["fg"].is_object()) {
                    auto const& fg = fv["fg"].as_object();
                    Color c;
                    c.r = static_cast<std::uint8_t>(fg.at("r").int_or(255));
                    c.g = static_cast<std::uint8_t>(fg.at("g").int_or(255));
                    c.b = static_cast<std::uint8_t>(fg.at("b").int_or(255));
                    f.fg = c;
                }
                if (fv.contains("bg") && fv["bg"].is_object()) {
                    auto const& bg = fv["bg"].as_object();
                    Color c;
                    c.r = static_cast<std::uint8_t>(bg.at("r").int_or(0));
                    c.g = static_cast<std::uint8_t>(bg.at("g").int_or(0));
                    c.b = static_cast<std::uint8_t>(bg.at("b").int_or(0));
                    f.bg = c;
                }
                f.offset_x = static_cast<int>(fv["offset_x"].int_or(0));
                f.offset_y = static_cast<int>(fv["offset_y"].int_or(0));
                a.frames.push_back(f);
            }
        }
        if (a.frames.empty()) {
            a.frames.push_back(AnimFrame{100, 0x20, std::nullopt, std::nullopt, 0, 0});
        }
    } catch (std::exception const&) {
        // Bad JSON: leave defaults.
    }
    return a;
}

}  // namespace

void tick(AnimationState& s, Animation const& a, int dt_ms) {
    if (s.finished || a.frames.empty()) {
        s.finished = true;
        return;
    }
    int remaining = dt_ms;
    while (remaining > 0 && !s.finished) {
        auto const& frame = a.frames[static_cast<std::size_t>(s.current_frame)];
        int const dur = frame.duration_ms;
        if (s.elapsed_ms + remaining < dur) {
            s.elapsed_ms += remaining;
            remaining = 0;
            break;
        }
        remaining -= (dur - s.elapsed_ms);
        s.elapsed_ms = 0;
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
    }
}

void AnimationRegistry::insert(Animation a) {
    by_id_[a.id] = std::move(a);
}

bool AnimationRegistry::remove(std::string const& id) {
    return by_id_.erase(id) > 0;
}

void AnimationRegistry::clear() {
    by_id_.clear();
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
        std::stringstream buf;
        buf << in.rdbuf();
        Animation a = parse_animation_json(buf.str(), entry.path().stem().string());
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