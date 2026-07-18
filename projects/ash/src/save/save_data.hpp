#pragma once
/// Phase 10: aggregate save-data snapshots.
///
/// Phase 10 needs a single, well-defined representation of the player and
/// world state that can be serialized to JSON and reloaded losslessly.
/// This header holds those snapshot types. Game systems fill them in via
/// const references; the save layer reads them out.
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "character/attributes.hpp"
#include "character/condition.hpp"
#include "character/inventory.hpp"
#include "character/leveling.hpp"
#include "character/skills.hpp"
#include "core/ids.hpp"
#include "quest/flag_store.hpp"
#include "quest/journal.hpp"
#include "quest/qst_engine.hpp"

namespace ash {
namespace save {

/// Player snapshot. Compose this from the in-game player object.
struct PlayerSave {
    std::string name;
    std::string race;
    std::string background;

    character::Attributes  attributes;
    character::Skills      skills;
    character::LevelState  level{};
    character::Inventory   inventory{};
    character::ConditionList conditions{};

    int hp{0}, hp_max{0};
    int vp{0}, vp_max{0};
    int sp{0}, sp_max{0};

    std::vector<std::string>  spells_known;
    std::map<std::string, std::string> spell_book;

    std::vector<std::string>  known_topics;

    /// Stable ad-hoc journal entries the engine has appended since the
    /// last save (quest-derived entries are reconstructed on load from
    /// `world.quests`).
    std::vector<quest::JournalEntry> ad_hoc_journal;
};

/// World snapshot. Holds shared quest state, NPCs, time, faction rep.
struct WorldSave {
    WorldSave() : flags(std::make_unique<quest::FlagStore>()) {}

    /// Flag store is heap-allocated because the underlying FlagStore has
    /// non-movable observers; rebuilding from JSON is cheap.
    std::unique_ptr<quest::FlagStore> flags;
    std::string               current_map;
    int                       current_x{0};
    int                       current_y{0};
    int                       play_time_seconds{0};
    int                       rng_seed{0};
    int                       game_hour{6};
    int                       game_minute{0};
    int                       game_day{0};
    int                       game_month{0};
    int                       game_year{220};

    /// NPC entries: id -> (alive, map_id, x, y, schedule_offset, disposition)
    struct NpcState {
        bool         alive{true};
        std::string  current_map;
        int          x{0}, y{0};
        int          schedule_offset_min{0};
        int          disposition{0};
        std::vector<std::string> inventory;
        std::set<std::string> flags;
    };
    std::map<std::string, NpcState> npcs;

    /// Quest runtime: mirrors quest::QuestRuntime.
    struct QuestState {
        std::string id;
        quest::QuestPhase phase{quest::QuestPhase::Hidden};
        std::string current_stage;
        std::vector<std::string> visited;
        std::unordered_map<std::string, std::int64_t> counters;
        std::string started_at;
    };
    std::vector<QuestState> quests;

    /// Faction reputation: name -> signed value.
    std::map<std::string, int> faction_rep;

    /// Creature kill timers + alive flags.
    struct CreatureState {
        bool alive{true};
        bool killed_by_player{false};
        std::string death_time;
        bool corpse_looted{false};
    };
    std::map<std::string, CreatureState> creatures;

    /// Container runtime state.
    struct ContainerState {
        bool locked{false};
        bool opened{false};
        std::vector<std::string> contents;
    };
    std::map<std::string, ContainerState> containers;

    /// Door state.
    struct DoorState {
        bool open{false};
        bool locked{false};
        std::string key_id;
    };
    std::map<std::string, DoorState> doors;

    /// Trigger record.
    struct TriggerState {
        int fired_count{0};
        std::string last_fired;
    };
    std::map<std::string, TriggerState> triggers;

    /// Global counters.
    struct Counters {
        int kills_total{0};
        std::map<std::string, int> kills_by_faction;
        int books_read{0};
        int secrets_found{0};
        int npc_conversations{0};
    };
    Counters counters;

    /// Maps dirtied at runtime: id -> raw .xp bytes (or a placeholder).
    std::map<std::string, std::vector<std::uint8_t>> dirty_maps;

    /// Last rendered frame ASCII (40x20 thumbnail).
    std::string thumbnail;
};

/// Bundle returned by `load` and consumed by `save`.
struct SaveData {
    PlayerSave player;
    WorldSave  world;
};

/// Meta block written alongside player/world per appendix D.
struct SaveMeta {
    int         schema_version{0};
    std::string version;       /// App version "0.1.0"
    std::string save_name;
    std::string created_at;
    std::string last_played_at;
    int         play_time_seconds{0};
    int         player_level{1};
    std::string player_map;
    int         player_x{0}, player_y{0};
    int         rng_seed{0};
    struct Time { int year{220}, month{1}, day{1}, hour{6}, minute{0}; };
    Time        game_time;
    std::string weather{"clear"};
    std::string difficulty{"normal"};
    bool        permadeath{false};
    std::uint32_t checksum{0};
};

}  // namespace save
}  // namespace ash
