/// Phase 10: save/load roundtrip + perf + atomicity tests.
#include "test_harness.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "character/attributes.hpp"
#include "character/inventory.hpp"
#include "character/skills.hpp"
#include "core/ids.hpp"
#include "core/json.hpp"
#include "core/path.hpp"
#include "core/time.hpp"
#include "quest/flag_store.hpp"
#include "quest/qst_engine.hpp"
#include "save/autosave.hpp"
#include "save/load.hpp"
#include "save/save.hpp"
#include "save/schema_v0.hpp"
#include "settings/accessibility.hpp"
#include "settings/settings.hpp"

namespace fs = std::filesystem;

namespace {

using namespace ash;

class TmpRoot {
public:
    TmpRoot() {
        // Point both data + config dirs at a private scratch dir.
        base_ = fs::temp_directory_path() / ("ash_test_" + std::to_string(::getpid()) +
                                              "_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        core::override_data_dir(base_.string());
        core::override_config_dir((base_ / "config").string());
        fs::create_directories(base_);
        mock_now_ = static_cast<std::int64_t>(1700000000);
        core::set_clock_for_tests([]() -> std::int64_t { return TmpRoot::mock_now_; });
    }
    ~TmpRoot() {
        std::error_code ec;
        fs::remove_all(base_, ec);
        core::override_data_dir("");
        core::override_config_dir("");
        core::set_clock_for_tests(nullptr);
    }
    static std::int64_t mock_now_;
    fs::path base_;
};
std::int64_t TmpRoot::mock_now_ = 1700000000;

save::SaveData make_sample_data(const std::string& name) {
    save::SaveData d;
    d.player.name = name;
    d.player.race = "human";
    d.player.background = "woke_in_crypt";
    d.player.level.level = 7;
    d.player.level.xp = 4200;
    d.player.attributes[character::Attribute::Str] = 50;
    d.player.attributes[character::Attribute::Int] = 60;
    d.player.skills[character::Skill::Blade] = 25;
    d.player.skills[character::Skill::Mysticism] = 35;
    d.player.spells_known = {"mend", "light", "firebolt"};
    d.player.spell_book = {{"mend", "memorized"}, {"firebolt", "memorized"}};
    d.player.known_topics = {"maren_meet", "tutorial_wake"};
    if (!d.world.flags) d.world.flags = std::make_unique<quest::FlagStore>();
    d.world.flags->set_bool("maren_met", true);
    d.world.flags->set_int("rumors_heard", 7);
    d.world.current_map = "silvercliff_inn_floor";
    d.world.current_x = 12;
    d.world.current_y = 8;
    d.world.rng_seed = 12345678;
    d.world.game_hour = 14;
    d.world.game_minute = 30;
    d.world.game_year = 220;
    d.world.game_month = 4;
    d.world.game_day = 12;
    d.world.play_time_seconds = 14231;
    d.world.thumbnail = "thumb-test";
    d.world.counters.kills_total = 17;
    d.world.counters.secrets_found = 2;
    d.world.faction_rep["coastal_watch"] = 35;
    save::WorldSave::QuestState qs;
    qs.id = "main_act1_wake";
    qs.phase = quest::QuestPhase::Active;
    qs.current_stage = "find_brother";
    qs.visited = {"wake", "maren_meet", "find_brother"};
    qs.counters["kills"] = 3;
    d.world.quests.push_back(qs);
    save::WorldSave::NpcState n;
    n.alive = true;
    n.current_map = "silvercliff_inn_floor";
    n.x = 12;
    n.y = 8;
    n.disposition = 65;
    n.inventory = {"iron_cleaver", "leather_apron"};
    d.world.npcs["maren_vell"] = n;
    return d;
}

}  // namespace

TEST_CASE("save: roundtrip preserves state", "[save][roundtrip]") {
    TmpRoot root;
    auto in = make_sample_data("Stranger");
    auto save_res = save::save("wanderer", in);
    REQUIRE(save_res.ok);
    REQUIRE(save_res.error.empty());
    auto load_res = save::load("wanderer");
    REQUIRE(load_res.ok);
    REQUIRE(load_res.data.player.name == "Stranger");
    REQUIRE(load_res.data.player.attributes[character::Attribute::Str] == 50);
    REQUIRE(load_res.data.player.skills[character::Skill::Mysticism] == 35);
    REQUIRE(load_res.data.player.spells_known.size() == 3);
    REQUIRE(load_res.data.player.spell_book.at("firebolt") == "memorized");
    REQUIRE(load_res.data.world.current_map == "silvercliff_inn_floor");
    REQUIRE(load_res.data.world.current_x == 12);
    REQUIRE(load_res.data.world.game_year == 220);
    REQUIRE(load_res.data.world.counters.kills_total == 17);
    REQUIRE(load_res.data.world.faction_rep.at("coastal_watch") == 35);
    REQUIRE(load_res.data.world.flags.get() != nullptr);
    auto mb = load_res.data.world.flags->get_bool("maren_met");
    REQUIRE(mb.has_value());
    REQUIRE(*mb == true);
    auto ri = load_res.data.world.flags->get_int("rumors_heard");
    REQUIRE(ri.has_value());
    REQUIRE(*ri == 7);
    REQUIRE(load_res.data.world.thumbnail == "thumb-test");
    REQUIRE(load_res.data.world.npcs.count("maren_vell") == 1);
    REQUIRE(load_res.data.world.npcs.at("maren_vell").disposition == 65);
    REQUIRE(load_res.data.world.quests.size() == 1);
    REQUIRE(load_res.data.world.quests[0].id == "main_act1_wake");
}

TEST_CASE("save: atomic - 3-generation backup rotation", "[save][backup]") {
    TmpRoot root;
    {
        auto in = make_sample_data("v1");
        REQUIRE(save::save("slot_a", in).ok);
    }
    REQUIRE(!fs::exists(core::slot_dir("slot_a").string() + ".bak1"));
    {
        auto in = make_sample_data("v2");
        in.player.level.level = 8;
        REQUIRE(save::save("slot_a", in).ok);
    }
    REQUIRE(fs::exists(core::slot_dir("slot_a").string() + ".bak1/meta.json"));
    {
        auto in = make_sample_data("v3");
        in.player.level.level = 9;
        REQUIRE(save::save("slot_a", in).ok);
    }
    REQUIRE(fs::exists(core::slot_dir("slot_a").string() + ".bak2/meta.json"));
    auto res = save::load("slot_a");
    REQUIRE(res.ok);
    REQUIRE(res.data.player.level.level == 9);
}

TEST_CASE("save: load_with_fallback recovers from missing current", "[save][fallback]") {
    TmpRoot root;
    auto in = make_sample_data("Stranger");
    REQUIRE(save::save("slot_b", in).ok);
    fs::rename(core::slot_dir("slot_b"),
               core::slot_dir("slot_b").string() + ".bak1");
    auto res = save::load_with_fallback("slot_b");
    REQUIRE(res.ok);
    REQUIRE(res.data.player.name == "Stranger");
}

TEST_CASE("save: rejects forward-compat schema", "[save][schema]") {
    TmpRoot root;
    fs::create_directories(core::slot_dir("newer"));
    std::ofstream m(core::slot_dir("newer") / "meta.json");
    m << R"({"schema_version":99,"save_name":"newer","player_location":{"map_id":"","x":0,"y":0},"game_time":{"year":1,"month":1,"day":1,"hour":0,"minute":0}})";
    m.close();
    auto res = save::load("newer");
    REQUIRE(!res.ok);
    REQUIRE(!res.error.empty());
}

TEST_CASE("autosave: 10-minute tick fires once per interval", "[save][autosave]") {
    save::AutosaveTicker t(600);
    // First non-zero tick anchors the baseline; doesn't fire.
    REQUIRE(!t.tick(1));
    REQUIRE(!t.tick(100));
    REQUIRE(!t.tick(500));
    // 600s after baseline = exactly the interval → fires.
    REQUIRE(t.tick(601));
    // Subsequent ticks under 600s after the last fire don't fire.
    REQUIRE(!t.tick(900));
    REQUIRE(!t.tick(1199));
    // 600s after the last fire (601) = 1201 → fires again.
    REQUIRE(t.tick(1201));
}

TEST_CASE("autosave: map trigger throttle", "[save][autosave]") {
    save::MapSaveTrigger trig(60);
    REQUIRE(trig.on_map_entry(1000));
    REQUIRE(!trig.on_map_entry(1010));
    REQUIRE(trig.on_map_entry(1100));
}

TEST_CASE("settings: roundtrip + hot-reload", "[settings]") {
    TmpRoot root;
    auto s1 = settings::Settings{};
    s1.colorblind_mode = true;
    s1.high_contrast   = true;
    s1.no_color        = true;
    s1.text_speed_ms   = 12;
    s1.audio_cue_volume = 0.42f;
    s1.autosave_minutes = 15;
    REQUIRE(settings::save(s1));

    auto loaded = settings::load();
    REQUIRE(loaded.colorblind_mode);
    REQUIRE(loaded.high_contrast);
    REQUIRE(loaded.no_color);
    REQUIRE(loaded.text_speed_ms == 12);
    REQUIRE(loaded.audio_cue_volume > 0.41f && loaded.audio_cue_volume < 0.43f);
    REQUIRE(loaded.autosave_minutes == 15);

    int reload_count = 0;
    settings::SettingsWatcher watcher(settings::load().source_path,
        [&](const settings::Settings&){ ++reload_count; });
    REQUIRE(watcher.update() == false);

    {
        json::Value cfg(json::Object{});
        cfg["dyslexic_font"] = json::Value(true);
        cfg["text_speed_ms"] = json::Value(static_cast<std::int64_t>(50));
        std::ofstream f(watcher.current().source_path);
        f << json::dump(cfg, true);
    }
    std::filesystem::file_time_type now = std::filesystem::file_time_type::clock::now();
    std::filesystem::last_write_time(watcher.current().source_path, now);
    REQUIRE(watcher.update() == true);
    REQUIRE(reload_count == 1);
    REQUIRE(watcher.current().dyslexic_font);
    REQUIRE(watcher.current().text_speed_ms == 50);
}

TEST_CASE("settings: derive accessibility snapshots", "[settings][accessibility]") {
    auto s = settings::Settings{};
    s.no_color = true;
    s.colorblind_mode = true;
    s.reduce_motion = true;
    s.dyslexic_font = true;
    s.audio_cue_volume = 0.3f;
    s.text_speed_ms = 60;
    auto rr = settings::derive_render(s);
    REQUIRE(!rr.use_color);
    REQUIRE(rr.palette_swap);
    REQUIRE(rr.glyphs_for_color);
    auto ur = settings::derive_ui(s);
    REQUIRE(ur.reduce_motion);
    REQUIRE(ur.dyslexic_font);
    REQUIRE(ur.audio_cue_volume > 0.2f && ur.audio_cue_volume < 0.4f);
    REQUIRE(ur.text_speed_ms == 60);
    REQUIRE(settings::glyph_for_color(255, 0, 0)[0] == 'R');
    REQUIRE(settings::glyph_for_color(0, 255, 0)[0] == 'G');
}

TEST_CASE("budgets: save <500ms, load <1500ms", "[save][budgets]") {
    TmpRoot root;
    auto in = make_sample_data("Stranger");
    auto t0 = std::chrono::steady_clock::now();
    auto r = save::save("budget_slot", in);
    auto t1 = std::chrono::steady_clock::now();
    REQUIRE(r.ok);
    auto save_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    REQUIRE(save_ms < 500);

    auto t2 = std::chrono::steady_clock::now();
    auto r2 = save::load("budget_slot");
    auto t3 = std::chrono::steady_clock::now();
    REQUIRE(r2.ok);
    auto load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    REQUIRE(load_ms < 1500);
}

TEST_CASE("path: XDG defaults + override", "[path]") {
    TmpRoot root;
    auto d = core::data_dir();
    REQUIRE(d.string().find("ash_test") != std::string::npos);
    auto cfg = core::config_dir();
    REQUIRE(cfg.string().find("ash_test") != std::string::npos);
    REQUIRE(core::ensure_dir(core::saves_dir()));
    REQUIRE(fs::is_directory(core::saves_dir()));
    REQUIRE(core::ensure_dir(core::logs_dir()));
}