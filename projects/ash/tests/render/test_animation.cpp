/// Phase 11: animation registry + tick correctness (per 16-phase-11).
#include "test_harness.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

#include "render/animation.hpp"

namespace fs = std::filesystem;

namespace {

using namespace ash::render;

void write_file(fs::path const& p, std::string const& body) {
    std::ofstream f(p);
    f << body;
}

Animation mk_simple(int frame_ms = 100) {
    Animation a;
    a.id = "test";
    a.loop = true;
    a.fps = 8;
    for (int i = 0; i < 3; ++i) {
        AnimFrame f;
        f.duration_ms = frame_ms;
        f.glyph = static_cast<uint32_t>('a' + i);
        a.frames.push_back(f);
    }
    return a;
}

}  // namespace

TEST_CASE("animation: tick advances frames on a loop", "[render][animation]") {
    auto a = mk_simple(100);
    auto s = make_state("test");
    REQUIRE(s.current_frame == 0);
    tick(s, a, 50);
    REQUIRE(s.current_frame == 0);
    tick(s, a, 60);   // cumulative 110 -> advance to 1
    REQUIRE(s.current_frame == 1);
    tick(s, a, 200);  // cumulative 310 -> wrap to 0 (frame 1 -> 2 -> wrap to 0)
    REQUIRE(s.current_frame == 0);
}

TEST_CASE("animation: non-loop stops at last frame", "[render][animation]") {
    auto a = mk_simple(100);
    a.loop = false;
    auto s = make_state("test");
    tick(s, a, 1000);
    REQUIRE(s.current_frame == 2);
    REQUIRE(s.finished == true);
}

TEST_CASE("animation: zero-frame animation finishes immediately", "[render][animation]") {
    Animation a;
    a.id = "empty";
    a.loop = true;
    a.fps = 8;
    auto s = make_state("empty");
    tick(s, a, 16);
    REQUIRE(s.finished == true);
}

TEST_CASE("animation: registry load + JSON parse", "[render][animation]") {
    auto tmp = fs::temp_directory_path() /
        ("ash_anim_test_" + std::to_string(::getpid()));
    fs::create_directories(tmp);

    write_file(tmp / "walk.json", R"({
        "id": "player_walk",
        "loop": true,
        "fps": 8,
        "frames": [
            {"duration_ms": 100, "glyph": "@"},
            {"duration_ms": 100, "glyph": "#"}
        ]
    })");

    AnimationRegistry reg;
    reg.load(tmp);
    auto const* a = reg.get("player_walk");
    REQUIRE(a != nullptr);
    REQUIRE(a->loop == true);
    REQUIRE(a->fps == 8);
    REQUIRE(a->frames.size() == 2);
    REQUIRE(a->frames[0].glyph == uint32_t('@'));
    REQUIRE(a->frames[1].glyph == uint32_t('#'));

    fs::remove_all(tmp);
}

TEST_CASE("animation: registry ignores invalid JSON", "[render][animation]") {
    auto tmp = fs::temp_directory_path() /
        ("ash_anim_bad_" + std::to_string(::getpid()));
    fs::create_directories(tmp);
    write_file(tmp / "bad.json", "{not valid json");

    AnimationRegistry reg;
    reg.load(tmp);
    // Falls back to single-frame placeholder.
    auto const* a = reg.get("bad");
    REQUIRE(a != nullptr);
    REQUIRE(a->frames.size() == 1);

    fs::remove_all(tmp);
}

TEST_CASE("animation: hot reload clears and re-reads", "[render][animation]") {
    auto tmp = fs::temp_directory_path() /
        ("ash_anim_reload_" + std::to_string(::getpid()));
    fs::create_directories(tmp);
    write_file(tmp / "a.json", R"({"id":"a","frames":[{"duration_ms":50,"glyph":"x"}]})");
    write_file(tmp / "b.json", R"({"id":"b","frames":[{"duration_ms":60,"glyph":"y"}]})");

    AnimationRegistry reg;
    reg.load(tmp);
    REQUIRE(reg.get("a") != nullptr);
    REQUIRE(reg.get("b") != nullptr);

    fs::remove(tmp / "b.json");
    reg.hot_reload();
    REQUIRE(reg.get("a") != nullptr);
    REQUIRE(reg.get("b") == nullptr);

    fs::remove_all(tmp);
}
