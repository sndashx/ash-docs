#pragma once
/// Phase 09: Quest AST (per appendix C-quest-format.txt).
///
/// The AST mirrors the structure of a .qst file: a top-level QuestDef
/// with metadata and a map of Stage nodes. Each Stage has entry / exit
/// actions, completion conditions, optional branch targets, and an
/// optional stage-specific kind (objective, branch, simultaneous,
/// escort, investigate, timed, kill, fetch).
///
/// The AST is intentionally framework-free: it has no dependency on the
/// engine, world, or renderer. The qst_engine layer is responsible for
/// interpreting it against the live world.

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ash {
namespace quest {

enum class StageKind {
    Objective,
    Branch,
    Simultaneous,
    Escort,
    Investigate,
    Timed,
    Kill,
    Fetch,
};

inline const char* stage_kind_name(StageKind k) {
    switch (k) {
        case StageKind::Objective:    return "objective";
        case StageKind::Branch:       return "branch";
        case StageKind::Simultaneous: return "simultaneous";
        case StageKind::Escort:       return "escort";
        case StageKind::Investigate:  return "investigate";
        case StageKind::Timed:        return "timed";
        case StageKind::Kill:         return "kill";
        case StageKind::Fetch:        return "fetch";
    }
    return "objective";
}

enum class CondOp {
    HasFlag,
    FlagEquals,
    FlagAtLeast,
    NotFlag,
    HasItem,
    HasItemCount,
    RepAtLeast,
    DispositionAtLeast,
    SkillAtLeast,
    CompletedQuest,
    ActiveQuest,
    AtLocation,
    InCombat,
    TimeOfDay,
    Weather,
    And,
    Or,
    Not,
    True,
};

struct Condition {
    CondOp op = CondOp::True;
    std::vector<std::string> args;
    std::vector<Condition> children;
};

enum class ActionKind {
    SetFlag,
    IncFlag,
    ToggleFlag,
    ClearFlag,
    GiveItem,
    TakeItem,
    GiveXp,
    Disposition,
    Reputation,
    JournalAdd,
    SpawnNpc,
    DespawnNpc,
    SetStage,
    CompleteQuest,
    FailQuest,
    Teleport,
};

struct Action {
    ActionKind kind = ActionKind::SetFlag;
    std::vector<std::string> args;
};

struct ItemReq    { std::string item; std::int64_t count = 1; };
struct ExamineReq { std::string entity_id; };
struct TalkReq    { std::string npc_id; };
struct FlagReq    { std::string key; };
struct KillReq    { std::string faction; std::int64_t count = 1; };

using SubReq = std::variant<ItemReq, ExamineReq, TalkReq, FlagReq, KillReq>;

struct Branch {
    Condition cond;
    std::string target_stage;
};

struct Stage {
    std::string id;
    StageKind   kind = StageKind::Objective;

    std::string description;
    std::string objective;

    std::vector<SubReq>    objective_all;
    std::optional<KillReq> objective_count;
    std::vector<SubReq>    objective_any;

    std::string            next;
    std::vector<Branch>    branches;

    std::string escort_npc;
    std::string destination;

    std::string    duration;
    std::int64_t   duration_secs = 0;

    std::vector<Action> on_enter;
    std::vector<Action> on_exit;
    std::vector<Action> on_complete;
    std::vector<Action> on_fail;
    std::vector<Action> on_arrive;
    std::vector<Action> on_timeout;

    Condition completion;
};

struct QuestMeta {
    std::string name;
    std::string category = "Side Quest";
    std::int64_t priority = 0;
    std::string giver;
    std::int64_t sort_index = 0;
    std::optional<Condition> hidden_until;
    bool auto_complete = false;
};

struct QuestDef {
    std::string id;
    QuestMeta   meta;
    std::string entry;
    std::vector<Stage> stages;
};

struct ParsedQuest {
    QuestDef def;
    std::vector<std::pair<std::string, std::size_t>> stage_index;
    std::size_t find_stage(const std::string& id) const;
};

}  // namespace quest
}  // namespace ash