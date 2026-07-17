#pragma once
/// Phase 09: Quest stage machine (per Pillar 8).
///
/// The engine drives a quest through its stages. For each quest the
/// engine tracks:
///   - the current stage id,
///   - a small per-quest scratchpad (counters, visited stages, timers),
///   - completion state (active / completed / failed).
///
/// Stage evaluation consults the FlagStore for condition checks and the
/// WorldHooks interface for inventory / faction / time queries. Actions
/// emitted by stage callbacks mutate the FlagStore and call back into
/// WorldHooks for items / spawns / teleports.
///
/// The engine is decoupled from entt by design: it does not own a
/// registry. Game code wires the engine up to the actual world via
/// WorldHooks at construction time.

#include "quest/flag_store.hpp"
#include "quest/qst_ast.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ash {
namespace quest {

enum class QuestPhase {
    Hidden,
    Offered,
    Active,
    Completed,
    Failed,
};

inline const char* quest_phase_name(QuestPhase p) {
    switch (p) {
        case QuestPhase::Hidden:    return "hidden";
        case QuestPhase::Offered:   return "offered";
        case QuestPhase::Active:    return "active";
        case QuestPhase::Completed: return "completed";
        case QuestPhase::Failed:    return "failed";
    }
    return "hidden";
}

/// Per-quest runtime state.
struct QuestRuntime {
    std::string current_stage;
    QuestPhase  phase = QuestPhase::Hidden;
    std::vector<std::string> visited;
    /// Counter keyed by stage_id for KILL/FETCH-style accumulation.
    std::unordered_map<std::string, std::int64_t> counters;
    /// Dedup set for KILL kills by faction / entity tag.
    std::unordered_map<std::string, std::unordered_set<std::string>> killed;
    /// Per-stage "objective_found" set (for INVESTIGATE / SIMULTANEOUS).
    std::unordered_map<std::string, std::unordered_set<std::string>> found;
    /// Time-remaining for TIMED stages (seconds).
    std::unordered_map<std::string, double> timers;
    /// Whether the on_enter block has run for the current stage.
    bool entered = false;
};

/// Hooks into the surrounding world. Game code provides concrete
/// implementations; tests provide in-memory fakes.
struct WorldHooks {
    /// Inventory count for an item (returns 0 if absent).
    std::function<std::int64_t(const std::string& who,
                               const std::string& item)> inventory_count;
    /// Add an item to an entity's inventory.
    std::function<void(const std::string& who,
                       const std::string& item,
                       std::int64_t count)> give_item;
    /// Remove items from an entity's inventory. Returns false if not enough.
    std::function<bool(const std::string& who,
                       const std::string& item,
                       std::int64_t count)> take_item;
    /// Give XP to a player.
    std::function<void(const std::string& who, std::int64_t xp)> give_xp;
    /// Reputation lookup / mutate (mutator signature: += N or = N).
    std::function<std::int64_t(const std::string& faction)> reputation_get;
    std::function<void(const std::string& faction,
                        std::int64_t value, bool add)> reputation_set;
    /// Disposition lookup / mutate.
    std::function<std::int64_t(const std::string& npc)> disposition_get;
    std::function<void(const std::string& npc,
                        std::int64_t value, bool add)> disposition_set;
    /// Skill lookup.
    std::function<std::int64_t(const std::string& skill)> skill_get;
    /// Current map / position (for at(...)).
    std::function<std::string()> current_map;
    std::function<std::pair<int, int>()> current_pos;
    /// True if currently in combat.
    std::function<bool()> in_combat;
    /// Current in-game hour (0..23).
    std::function<int()> game_hour;
    /// Current weather string.
    std::function<std::string()> weather;
    /// Whether a quest is currently active / completed (for completed_quest /
    /// active_quest conditions).
    std::function<bool(const std::string&)> is_quest_completed;
    std::function<bool(const std::string&)> is_quest_active;
    /// Spawn / despawn NPCs.
    std::function<void(const std::string& npc_id,
                       const std::string& map_id,
                       int x, int y)> spawn_npc;
    std::function<void(const std::string& npc_id)> despawn_npc;
    /// Teleport an entity.
    std::function<void(const std::string& who,
                       const std::string& map_id,
                       int x, int y)> teleport;
};

/// Catalog of loaded quests.
class QuestCatalog {
public:
    void add(ParsedQuest q);
    const ParsedQuest* find(const std::string& id) const;
    std::size_t size() const { return quests_.size(); }
    std::vector<std::string> ids() const;
private:
    std::vector<ParsedQuest> quests_;
};

/// Per-world quest engine.
class QuestEngine {
public:
    QuestEngine(FlagStore& flags, const QuestCatalog& cat, WorldHooks hooks);

    /// Offer a quest to the player (Hidden -> Offered).
    bool offer(const std::string& quest_id);
    /// Accept an offered quest (Offered -> Active), entering entry stage.
    bool accept(const std::string& quest_id);
    /// Force-activate a quest skipping the offered phase.
    bool start(const std::string& quest_id);
    /// Mark a quest complete.
    bool complete(const std::string& quest_id);
    /// Mark a quest failed.
    bool fail(const std::string& quest_id);
    /// Abandon an active quest (fail it).
    bool abandon(const std::string& quest_id);

    /// Advance the quest by evaluating its current stage. Returns true if
    /// the stage advanced (i.e. it completed and moved on).
    bool advance(const std::string& quest_id);

    /// Notification hooks: surface engine events to the game / UI.
    struct Event {
        std::string quest_id;
        std::string stage_id;
        enum Kind { StageEnter, StageExit, StageComplete,
                    QuestComplete, QuestFailed, QuestOffered } kind;
    };
    using Listener = std::function<void(const Event&)>;
    void on_event(Listener l);

    /// Access runtime for read-only queries (journal / UI).
    const QuestRuntime* runtime(const std::string& quest_id) const;
    /// Mutable runtime accessor (used by game systems that need to bump
    /// counters directly: KILL stages count kills by entity, INVESTIGATE
    /// stages track clue lookups, etc.).
    QuestRuntime* mutable_runtime(const std::string& quest_id);
    const ParsedQuest* definition(const std::string& quest_id) const;

    /// True if `stage_id` is currently active for `quest_id`.
    bool is_stage_active(const std::string& quest_id,
                         const std::string& stage_id) const;

    /// Increment the kill counter for the current KILL stage. Returns the
    /// post-increment value.
    std::int64_t bump_kill_counter(const std::string& quest_id,
                                    const std::string& entity_tag);
    /// Mark a clue found for the current INVESTIGATE / SIMULTANEOUS stage.
    void mark_found(const std::string& quest_id,
                    const std::string& entity_id);
    /// Mark "arrived" for an ESCORT stage.
    void mark_arrived(const std::string& quest_id);

    /// Evaluate a Condition against current world state.
    bool eval_condition(const Condition& c) const;

    /// Per-tick update (decrements TIMED stages, fires on_timeout).
    /// dt is in real seconds.
    void update(double dt);

private:
    bool eval_condition_inner(const Condition& c) const;
    void run_actions(const std::vector<Action>& acts);
    void enter_stage(QuestRuntime& rt, const Stage& st);
    void exit_stage(QuestRuntime& rt, const Stage& st);
    void fail_with_message(const std::string& quest_id, const std::string& msg);
    void fire(const Event& ev);

    FlagStore&        flags_;
    const QuestCatalog& cat_;
    WorldHooks        hooks_;
    std::unordered_map<std::string, QuestRuntime> runs_;
    std::vector<Listener> listeners_;
};

}  // namespace quest
}  // namespace ash
