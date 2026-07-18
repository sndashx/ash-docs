/// Phase 10: load manager tests.
#include "test_harness.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "character/attributes.hpp"
#include "character/skills.hpp"
#include "core/path.hpp"
#include "quest/flag_store.hpp"
#include "save/load.hpp"
#include "save/save.hpp"

namespace fs = std::filesystem;
using namespace ash;

namespace {

class TmpRoot {
public:
    TmpRoot() {
        base_ = fs::temp_directory_path() / ("ash_load_test_" + std::to_string(::getpid()) +
                                              "_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        core::override_data_dir(base_.string());
        fs::create_directories(base_);
    }
    ~TmpRoot() {
        std::error_code ec;
        fs::remove_all(base_, ec);
        core::override_data_dir("");
    }
    fs::path base_;
};

save::SaveData minimal() {
    save::SaveData d;
    d.player.name = "Tester";
    d.player.attributes[character::Attribute::Str] = 33;
    d.player.skills[character::Skill::Blade] = 17;
    if (!d.world.flags) d.world.flags = std::make_unique<quest::FlagStore>();
    d.world.flags->set_int("test_flag", 42);
    d.world.current_map = "test_map";
    return d;
}

}  // namespace

TEST_CASE("load: returns missing-slot error", "[save][load]") {
    TmpRoot root;
    auto res = save::load("__nonexistent__");
    REQUIRE(!res.ok);
    REQUIRE(!res.error.empty());
}

TEST_CASE("load: corrupt JSON produces error, not crash", "[save][load]") {
    TmpRoot root;
    fs::create_directories(core::slot_dir("broken"));
    {
        std::ofstream m(core::slot_dir("broken") / "meta.json");
        m << R"({"schema_version": 0)";
    }
    auto res = save::load("broken");
    REQUIRE(!res.ok);
}

TEST_CASE("load: roundtrip preserves stats + flags", "[save][load]") {
    TmpRoot root;
    auto in = minimal();
    REQUIRE(save::save("stat_slot", in).ok);
    auto r = save::load("stat_slot");
    REQUIRE(r.ok);
    REQUIRE(r.data.player.attributes[character::Attribute::Str] == 33);
    REQUIRE(r.data.player.skills[character::Skill::Blade] == 17);
    REQUIRE(r.data.world.flags.get() != nullptr);
    auto f = r.data.world.flags->get_int("test_flag");
    REQUIRE(f.has_value());
    REQUIRE(*f == 42);
}

TEST_CASE("load: load_with_fallback to bak1 succeeds", "[save][load]") {
    TmpRoot root;
    auto a = minimal();
    REQUIRE(save::save("fb_slot", a).ok);
    // Move current -> bak1.
    fs::rename(core::slot_dir("fb_slot"),
               core::slot_dir("fb_slot").string() + ".bak1");
    auto r = save::load_with_fallback("fb_slot");
    REQUIRE(r.ok);
    REQUIRE(r.data.player.name == "Tester");
}

TEST_CASE("load: budget reported in elapsed_ms", "[save][load]") {
    TmpRoot root;
    auto in = minimal();
    REQUIRE(save::save("budget_load", in).ok);
    auto r = save::load("budget_load");
    REQUIRE(r.ok);
    REQUIRE(r.elapsed_ms < 1500);
}