#include "test_harness.hpp"
#include "quest/flag_store.hpp"
#include "quest/qst_engine.hpp"
#include "quest/qst_parser.hpp"
#include "quest/consequence.hpp"
#include "quest/journal.hpp"

#include <map>
#include <string>

using namespace ash::quest;

namespace {

// Fake world state used across engine / consequence tests.
struct FakeWorld {
    FlagStore flags;
    std::map<std::string, std::map<std::string, std::int64_t>> inv;
    std::map<std::string, std::int64_t> rep;
    std::map<std::string, std::int64_t> disp;
    std::map<std::string, std::int64_t> skills;
    std::string cur_map = "silvercliff";
    std::pair<int, int> cur_pos = {0, 0};
    bool in_combat = false;
    int hour = 12;
    std::string weather_str = "clear";
    std::vector<std::string> spawned;
    std::vector<std::string> despawned;
    std::vector<std::tuple<std::string, std::string, int, int>> teleports;
    std::int64_t xp = 0;

    WorldHooks hooks() {
        WorldHooks h;
        h.inventory_count = [this](const std::string& who, const std::string& it) {
            auto wit = inv.find(who);
            if (wit == inv.end()) return (std::int64_t)0;
            auto iit = wit->second.find(it);
            if (iit == wit->second.end()) return (std::int64_t)0;
            return iit->second;
        };
        h.give_item = [this](const std::string& who, const std::string& it, std::int64_t n) {
            inv[who][it] += n;
        };
        h.take_item = [this](const std::string& who, const std::string& it, std::int64_t n) {
            if (inv[who][it] < n) return false;
            inv[who][it] -= n;
            return true;
        };
        h.give_xp = [this](const std::string& /*who*/, std::int64_t n) {
            xp += n;
        };
        h.reputation_get = [this](const std::string& f) -> std::int64_t {
            return rep.count(f) ? rep[f] : 0;
        };
        h.reputation_set = [this](const std::string& f, std::int64_t v, bool add) {
            if (add) rep[f] += v; else rep[f] = v;
        };
        h.disposition_get = [this](const std::string& n) -> std::int64_t {
            return disp.count(n) ? disp[n] : 0;
        };
        h.disposition_set = [this](const std::string& n, std::int64_t v, bool add) {
            if (add) disp[n] += v; else disp[n] = v;
        };
        h.skill_get = [this](const std::string& s) -> std::int64_t {
            return skills.count(s) ? skills[s] : 0;
        };
        h.current_map   = [this]{ return cur_map; };
        h.current_pos   = [this]{ return cur_pos; };
        h.in_combat     = [this]{ return in_combat; };
        h.game_hour     = [this]{ return hour; };
        h.weather       = [this]{ return weather_str; };
        h.is_quest_completed = [](const std::string&) { return false; };
        h.is_quest_active    = [](const std::string&) { return false; };
        h.spawn_npc = [this](const std::string& npc, const std::string& map, int x, int y) {
            spawned.push_back(npc);
            (void)map; (void)x; (void)y;
        };
        h.despawn_npc = [this](const std::string& npc) {
            despawned.push_back(npc);
        };
        h.teleport = [this](const std::string& who, const std::string& map, int x, int y) {
            teleports.push_back({who, map, x, y});
        };
        return h;
    }
};

QuestCatalog make_simple_catalog() {
    QuestCatalog cat;
    cat.add(parse_qst_string(R"(
        quest simple3 {
            meta { name: "Simple Three"; category: "Main Quest"; priority: 100; giver: maren_vell; }
            entry: stage_a;
            stage stage_a {
                description: "A";
                objective: "Do A.";
                on_enter { set_flag stage_a_entered = true; }
                condition { flag:flag_a; }
                on_exit { inc_flag counter, 1; }
                next: stage_b;
            }
            stage stage_b {
                description: "B";
                objective: "Do B.";
                condition { flag:flag_b; }
                next: stage_c;
            }
            stage stage_c {
                description: "C";
                objective: "Do C.";
            }
        }
    )"));
    return cat;
}

}  // namespace

TEST_CASE("qst_engine.offer_then_accept", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat = make_simple_catalog();
    QuestEngine eng(w.flags, cat, w.hooks());
    REQUIRE(eng.offer("simple3"));
    auto* rt = eng.runtime("simple3");
    REQUIRE(rt);
    REQUIRE(rt->phase == QuestPhase::Offered);
    REQUIRE(eng.accept("simple3"));
    REQUIRE(rt->phase == QuestPhase::Active);
    REQUIRE(rt->current_stage == "stage_a");
}

TEST_CASE("qst_engine.advance_through_stages_on_flag_set", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat = make_simple_catalog();
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("simple3");

    REQUIRE(eng.is_stage_active("simple3", "stage_a"));
    // First advance enters stage_a (runs on_enter) but does not satisfy it.
    eng.advance("simple3");
    REQUIRE(w.flags.get_bool("stage_a_entered").value_or(false) == true);
    REQUIRE(eng.is_stage_active("simple3", "stage_a"));

    eng.advance("simple3");   // stage_a not satisfied yet
    REQUIRE(eng.is_stage_active("simple3", "stage_a"));

    w.flags.set_bool("flag_a", true);
    eng.advance("simple3");   // stage_a satisfied -> stage_b
    REQUIRE(eng.is_stage_active("simple3", "stage_b"));
    REQUIRE(w.flags.get_int("counter").value() == 1);

    w.flags.set_bool("flag_b", true);
    eng.advance("simple3");   // stage_b satisfied -> stage_c
    REQUIRE(eng.is_stage_active("simple3", "stage_c"));

    // stage_c has no next and no completion condition (True defaults to false
    // because True op is handled separately; Objective kind falls through to
    // eval_condition_inner(True) which returns true).
    eng.advance("simple3");
    auto* rt = eng.runtime("simple3");
    REQUIRE(rt);
    REQUIRE(rt->phase == QuestPhase::Completed);
}

TEST_CASE("qst_engine.branch_selects_target", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat;
    cat.add(parse_qst_string(R"(
        quest br {
            meta { name: "BR"; category: "Misc"; priority: 1; }
            entry: pick;
            stage pick {
                description: "Pick";
                branch {
                    if (flag:chose_a) -> path_a;
                    else if (flag:chose_b) -> path_b;
                    else -> path_default;
                }
            }
            stage path_a { description: "a"; }
            stage path_b { description: "b"; }
            stage path_default { description: "d"; }
        }
    )"));
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("br");
    REQUIRE(eng.is_stage_active("br", "pick"));
    w.flags.set_bool("chose_b", true);
    eng.advance("br");
    REQUIRE(eng.is_stage_active("br", "path_b"));
}

TEST_CASE("qst_engine.branch_default_else", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat;
    cat.add(parse_qst_string(R"(
        quest br2 {
            meta { name: "BR2"; category: "Misc"; priority: 1; }
            entry: pick;
            stage pick {
                description: "Pick";
                branch {
                    if (flag:chose_a) -> path_a;
                    else -> path_default;
                }
            }
            stage path_a { description: "a"; }
            stage path_default { description: "d"; }
        }
    )"));
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("br2");
    eng.advance("br2");
    REQUIRE(eng.is_stage_active("br2", "path_default"));
}

TEST_CASE("qst_engine.simultaneous_fetch_uses_inventory", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat;
    cat.add(parse_qst_string(R"(
        quest fetch {
            meta { name: "Fetch"; category: "Misc"; priority: 1; }
            entry: gather;
            stage gather {
                description: "Gather";
                objective_all [
                    { item: "moonstone_dust", count: 3 },
                    { item: "ashwood_bark",   count: 5 }
                ];
                next: end;
            }
            stage end { description: "end"; }
        }
    )"));
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("fetch");
    w.inv["player"]["moonstone_dust"] = 3;
    w.inv["player"]["ashwood_bark"]   = 5;
    eng.advance("fetch");
    REQUIRE(eng.is_stage_active("fetch", "end"));
}

TEST_CASE("qst_engine.kill_counter_advances", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat;
    cat.add(parse_qst_string(R"(
        quest k {
            meta { name: "K"; category: "Misc"; priority: 1; }
            entry: c;
            stage c {
                description: "C";
                objective_count { faction: "creature_hostile", count: 3 };
                next: done;
            }
            stage done { description: "d"; }
        }
    )"));
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("k");
    eng.bump_kill_counter("k", "wolf_1");
    eng.bump_kill_counter("k", "wolf_2");
    eng.bump_kill_counter("k", "wolf_3");
    eng.advance("k");
    REQUIRE(eng.is_stage_active("k", "done"));
}

TEST_CASE("qst_engine.kill_counter_dedups", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat;
    cat.add(parse_qst_string(R"(
        quest k {
            meta { name: "K"; category: "Misc"; priority: 1; }
            entry: c;
            stage c {
                description: "C";
                objective_count { faction: "creature_hostile", count: 2 };
                next: done;
            }
            stage done { description: "d"; }
        }
    )"));
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("k");
    eng.bump_kill_counter("k", "wolf_1");
    eng.bump_kill_counter("k", "wolf_1");  // dedup
    eng.bump_kill_counter("k", "wolf_2");
    eng.advance("k");
    REQUIRE(eng.is_stage_active("k", "done"));
}

TEST_CASE("qst_engine.timed_stage_fails_on_timeout", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat;
    cat.add(parse_qst_string(R"(
        quest tt {
            meta { name: "TT"; category: "Misc"; priority: 1; auto_complete: false; }
            entry: r;
            stage r {
                description: "Race";
                duration: "10s";
                condition { flag:rescued; }
                next: done;
            }
            stage done { description: "d"; }
        }
    )"));
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("tt");
    eng.advance("tt");           // enter stage (timer populated by on_enter-equivalent)
    eng.update(11.0);             // drain timer
    eng.advance("tt");           // now stage should fail
    auto* rt = eng.runtime("tt");
    REQUIRE(rt);
    REQUIRE(rt->phase == QuestPhase::Failed);
}

TEST_CASE("qst_engine.events_fire", "[quest][engine]") {
    FakeWorld w;
    QuestCatalog cat = make_simple_catalog();
    QuestEngine eng(w.flags, cat, w.hooks());
    int stage_enter = 0;
    int stage_complete = 0;
    int quest_complete = 0;
    eng.on_event([&](const QuestEngine::Event& e) {
        if (e.kind == QuestEngine::Event::StageEnter) ++stage_enter;
        if (e.kind == QuestEngine::Event::StageComplete) ++stage_complete;
        if (e.kind == QuestEngine::Event::QuestComplete) ++quest_complete;
    });
    eng.start("simple3");
    w.flags.set_bool("flag_a", true);
    w.flags.set_bool("flag_b", true);
    eng.advance("simple3"); // a -> b
    eng.advance("simple3"); // b -> c
    eng.advance("simple3"); // c -> complete
    REQUIRE(stage_enter    >= 3);
    REQUIRE(stage_complete >= 2);
    REQUIRE(quest_complete == 1);
}

TEST_CASE("consequence.fires_on_flag_set", "[quest][consequence]") {
    FakeWorld w;
    WorldHooks h = w.hooks();
    ConsequenceRunner cr(w.flags, h);
    Consequence r;
    r.kind = ConsequenceKind::SpawnNpc;
    r.trigger_flag = "maren_died";
    r.want_bool = true;
    r.args = {"maren_ghost", "crypt", "5", "5"};
    cr.add(r);

    REQUIRE(w.spawned.empty());
    w.flags.set_bool("maren_died", true);
    REQUIRE(w.spawned.size() == 1u);
    REQUIRE(w.spawned[0] == "maren_ghost");
    REQUIRE(cr.fires().size() == 1u);
}

TEST_CASE("consequence.does_not_fire_on_wrong_value", "[quest][consequence]") {
    FakeWorld w;
    ConsequenceRunner cr(w.flags, w.hooks());
    Consequence r;
    r.kind = ConsequenceKind::DespawnNpc;
    r.trigger_flag = "doomed";
    r.want_bool = true;
    r.args = {"ghost"};
    cr.add(r);
    w.flags.set_bool("doomed", false);
    REQUIRE(w.despawned.empty());
}

TEST_CASE("journal.three_views_after_engine_run", "[quest][journal]") {
    FakeWorld w;
    QuestCatalog cat = make_simple_catalog();
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("simple3");
    w.flags.set_bool("flag_a", true);
    w.flags.set_bool("flag_b", true);
    eng.advance("simple3");
    eng.advance("simple3");
    eng.advance("simple3"); // completes

    Journal j(eng, cat);
    j.rebuild();
    REQUIRE(j.size(JournalView::Completed) == 1u);
    REQUIRE(j.size(JournalView::Active)    == 0u);
    REQUIRE(j.size(JournalView::All)       == 1u);
    j.mark_all_read();
    REQUIRE(j.unread_count() == 0u);
}

TEST_CASE("journal.active_objective_text", "[quest][journal]") {
    FakeWorld w;
    QuestCatalog cat = make_simple_catalog();
    QuestEngine eng(w.flags, cat, w.hooks());
    eng.start("simple3");
    Journal j(eng, cat);
    j.rebuild();
    auto active = j.view(JournalView::Active);
    REQUIRE(active.size() == 1u);
    REQUIRE(active[0].quest_id == "simple3");
    REQUIRE(active[0].current_objective == "Do A.");
}

TEST_CASE("journal.add_entry_from_action", "[quest][journal]") {
    FakeWorld w;
    QuestCatalog cat = make_simple_catalog();
    QuestEngine eng(w.flags, cat, w.hooks());
    Journal j(eng, cat);
    j.add_entry("Main Quest", "I met Maren today.");
    REQUIRE(j.size(JournalView::All) == 1u);
    REQUIRE(j.entries()[0].text == "I met Maren today.");
}

TEST_CASE("engine.act1_wake_plays_through", "[quest][engine][integration]") {
    FakeWorld w;
    QuestCatalog cat;
    cat.add(parse_qst_file("content/quests/main/main_act1_wake.qst"));
    QuestEngine eng(w.flags, cat, w.hooks());

    eng.start("main_act1_wake");
    REQUIRE(eng.is_stage_active("main_act1_wake", "find_maren"));

    w.flags.set_bool("met_maren", true);
    eng.advance("main_act1_wake");
    REQUIRE(eng.is_stage_active("main_act1_wake", "find_rellan"));
    REQUIRE(w.flags.get_int("maren_trust").value() == 5);

    w.flags.set_bool("rellan_clue_docks", true);
    eng.advance("main_act1_wake");
    REQUIRE(eng.is_stage_active("main_act1_wake", "report_to_maren"));

    eng.advance("main_act1_wake");   // enters report_to_maren (give_item + rep)
    REQUIRE(w.inv["player"]["maren_gratitude"] == 1);
    REQUIRE(w.rep["coastal_watch"] == 2);

    eng.advance("main_act1_wake");   // completes (next is empty)
    auto* rt = eng.runtime("main_act1_wake");
    REQUIRE(rt->phase == QuestPhase::Completed);
}