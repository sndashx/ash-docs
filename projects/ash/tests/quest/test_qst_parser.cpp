#include "test_harness.hpp"
#include "quest/qst_parser.hpp"

#include <string>

using namespace ash::quest;

TEST_CASE("qst_parser.empty_quest_minimal", "[quest][parser]") {
    auto pq = parse_qst_string(R"(
        quest minimal {
            meta { name: "Minimal"; category: "Misc"; priority: 1; }
            entry: ;
            stage none {
                description: "No-op stage.";
            }
        }
    )");
    REQUIRE(pq.def.id == "minimal");
    REQUIRE(pq.def.meta.name == "Minimal");
    REQUIRE(pq.def.meta.category == "Misc");
    REQUIRE(pq.def.meta.priority == 1);
    REQUIRE(pq.def.stages.size() == 1u);
    REQUIRE(pq.find_stage("none") == 0u);
}

TEST_CASE("qst_parser.single_objective_with_flag_condition", "[quest][parser]") {
    auto pq = parse_qst_string(R"(
        quest simple {
            meta { name: "Simple"; category: "Side Quest"; priority: 10; }
            entry: talk;
            stage talk {
                description: "Talk to someone.";
                objective: "Talk to someone.";
                condition { flag:talked; }
                on_enter { set_flag stage_entered = true; }
                on_exit { inc_flag progress, 1; }
                next: report;
            }
            stage report {
                description: "Report back.";
            }
        }
    )");
    REQUIRE(pq.def.stages.size() == 2u);
    REQUIRE(pq.def.stages[0].completion.op == CondOp::HasFlag);
    REQUIRE(pq.def.stages[0].completion.args[0] == "talked");
    REQUIRE(pq.def.stages[0].on_enter.size() == 1u);
    REQUIRE(pq.def.stages[0].on_enter[0].kind == ActionKind::SetFlag);
    REQUIRE(pq.def.stages[0].on_enter[0].args[0] == "stage_entered");
    REQUIRE(pq.def.stages[0].on_enter[0].args[1] == "true");
    REQUIRE(pq.def.stages[0].on_exit.size() == 1u);
    REQUIRE(pq.def.stages[0].on_exit[0].kind == ActionKind::IncFlag);
    REQUIRE(pq.def.stages[1].id == "report");
}

TEST_CASE("qst_parser.branch_stage", "[quest][parser]") {
    auto pq = parse_qst_string(R"(
        quest b {
            meta { name: "B"; category: "Misc"; priority: 1; }
            entry: pick;
            stage pick {
                description: "Pick a path.";
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
    )");
    REQUIRE(pq.def.stages.size() == 4u);
    REQUIRE(pq.def.stages[0].kind == StageKind::Branch);
    REQUIRE(pq.def.stages[0].branches.size() == 3u);
    REQUIRE(pq.def.stages[0].branches[0].cond.op == CondOp::HasFlag);
    REQUIRE(pq.def.stages[0].branches[0].cond.args[0] == "chose_a");
    REQUIRE(pq.def.stages[0].branches[0].target_stage == "path_a");
    REQUIRE(pq.def.stages[0].branches[1].target_stage == "path_b");
    REQUIRE(pq.def.stages[0].branches[2].cond.op == CondOp::True);
}

TEST_CASE("qst_parser.simultaneous_objectives", "[quest][parser]") {
    auto pq = parse_qst_string(R"(
        quest s {
            meta { name: "S"; category: "Misc"; priority: 1; }
            entry: gather;
            stage gather {
                description: "Gather things.";
                objective_all [
                    { item: "moonstone_dust", count: 3 },
                    { item: "ashwood_bark",   count: 5 },
                    { item: "saltwater_flask", count: 1 }
                ];
                next: done;
            }
            stage done { description: "Done"; }
        }
    )");
    REQUIRE(pq.def.stages[0].kind == StageKind::Simultaneous);
    REQUIRE(pq.def.stages[0].objective_all.size() == 3u);
    REQUIRE(pq.find_stage("done") == 1u);
}

TEST_CASE("qst_parser.timed_stage_with_duration_secs", "[quest][parser]") {
    auto pq = parse_qst_string(R"(
        quest t {
            meta { name: "T"; category: "Misc"; priority: 1; }
            entry: race;
            stage race {
                description: "Beat the clock.";
                duration: "300s";
                on_timeout {
                    spawn_npc prisoner_ghost, hollow_prison, 12, 8;
                    set_flag prisoner_died = true;
                }
                condition { flag:prisoner_rescued; }
                next: done;
            }
            stage done { description: "Done"; }
        }
    )");
    REQUIRE(pq.def.stages[0].kind == StageKind::Timed);
    REQUIRE(pq.def.stages[0].duration == "300s");
    REQUIRE(pq.def.stages[0].duration_secs == 300);
    REQUIRE(pq.def.stages[0].on_timeout.size() == 2u);
}

TEST_CASE("qst_parser.kill_counter", "[quest][parser]") {
    auto pq = parse_qst_string(R"(
        quest k {
            meta { name: "K"; category: "Misc"; priority: 1; }
            entry: clear;
            stage clear {
                description: "Clear the lair.";
                objective_count { faction: "creature_hostile", count: 8 };
                next: done;
            }
            stage done { description: "Done"; }
        }
    )");
    REQUIRE(pq.def.stages[0].kind == StageKind::Kill);
    REQUIRE(pq.def.stages[0].objective_count.has_value());
    REQUIRE(pq.def.stages[0].objective_count->faction == "creature_hostile");
    REQUIRE(pq.def.stages[0].objective_count->count == 8);
}

TEST_CASE("qst_parser.investigate_any", "[quest][parser]") {
    auto pq = parse_qst_string(R"(
        quest i {
            meta { name: "I"; category: "Misc"; priority: 1; }
            entry: look;
            stage look {
                description: "Find a clue.";
                objective_any [
                    { examine: "corpse_alley_north" },
                    { talk_to: "harric_dockworker" },
                    { flag: heard_from_witness }
                ];
                next: done;
            }
            stage done { description: "Done"; }
        }
    )");
    REQUIRE(pq.def.stages[0].kind == StageKind::Investigate);
    REQUIRE(pq.def.stages[0].objective_any.size() == 3u);
}

TEST_CASE("qst_parser.five_stage_sample_loads", "[quest][parser]") {
    // Loads the on-disk sample_five.qst (6 stages: 1, 2, 3a, 3b, 4, 5).
    auto pq = parse_qst_file("content/quests/side/sample_five.qst");
    REQUIRE(pq.def.id == "sample_five");
    REQUIRE(pq.def.stages.size() == 6u);
    REQUIRE(pq.find_stage("stage_1") == 0u);
    REQUIRE(pq.find_stage("stage_2") == 1u);
    REQUIRE(pq.find_stage("stage_3a") == 2u);
    REQUIRE(pq.find_stage("stage_3b") == 3u);
    REQUIRE(pq.find_stage("stage_4") == 4u);
    REQUIRE(pq.find_stage("stage_5") == 5u);
    REQUIRE(pq.def.stages[1].kind == StageKind::Branch);
    REQUIRE(pq.def.stages[2].kind == StageKind::Simultaneous);
    REQUIRE(pq.def.stages[4].on_complete.size() == 5u);
}

TEST_CASE("qst_parser.act1_wake_sample_loads", "[quest][parser]") {
    auto pq = parse_qst_file("content/quests/main/main_act1_wake.qst");
    REQUIRE(pq.def.id == "main_act1_wake");
    REQUIRE(pq.def.stages.size() == 3u);
    REQUIRE(pq.def.meta.giver == "maren_vell");
    REQUIRE(pq.def.entry == "find_maren");
}