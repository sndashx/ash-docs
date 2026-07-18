#include "save/schema_v0.hpp"

#include <stdexcept>
#include <string>

#include "character/attributes.hpp"
#include "character/condition.hpp"
#include "character/inventory.hpp"
#include "character/leveling.hpp"
#include "character/skills.hpp"
#include "core/json.hpp"
#include "quest/flag_store.hpp"
#include "quest/qst_engine.hpp"

namespace ash {
namespace save {

namespace {

using json::Value;

void require(const Value& v, const char* want) {
    if (v.is_null()) throw std::runtime_error(std::string("schema_v0: required field missing: ") + want);
}

// Map an in-game attribute index back to its JSON id and vice versa.
character::Attribute attr_from_id(const std::string& s) {
    if (s == "str") return character::Attribute::Str;
    if (s == "end") return character::Attribute::End;
    if (s == "agi") return character::Attribute::Agi;
    if (s == "wil") return character::Attribute::Wil;
    if (s == "int") return character::Attribute::Int;
    if (s == "wit") return character::Attribute::Wit;
    if (s == "cha") return character::Attribute::Cha;
    if (s == "luc") return character::Attribute::Luc;
    if (s == "voi") return character::Attribute::Voi;
    throw std::runtime_error("schema_v0: unknown attribute id: " + s);
}

character::Skill skill_from_id(const std::string& s) {
    if (s == "armorer")      return character::Skill::Armorer;
    if (s == "blunt")        return character::Skill::Blunt;
    if (s == "blade")        return character::Skill::Blade;
    if (s == "marksman")     return character::Skill::Marksman;
    if (s == "dodge")        return character::Skill::Dodge;
    if (s == "thrown")       return character::Skill::Thrown;
    if (s == "mysticism")    return character::Skill::Mysticism;
    if (s == "restoration")  return character::Skill::Restoration;
    if (s == "warding")      return character::Skill::Warding;
    if (s == "alchemy")      return character::Skill::Alchemy;
    if (s == "enchant")      return character::Skill::Enchant;
    if (s == "spellcraft")   return character::Skill::Spellcraft;
    if (s == "stealth")      return character::Skill::Stealth;
    if (s == "pickpocket")   return character::Skill::Pickpocket;
    if (s == "security")     return character::Skill::Security;
    if (s == "speechcraft")  return character::Skill::Speechcraft;
    if (s == "mercantile")   return character::Skill::Mercantile;
    if (s == "illusion")     return character::Skill::Illusion;
    if (s == "deception")    return character::Skill::Deception;
    if (s == "intimidation") return character::Skill::Intimidation;
    if (s == "seduction")    return character::Skill::Seduction;
    if (s == "history")      return character::Skill::History;
    if (s == "theology")     return character::Skill::Theology;
    if (s == "linguistics")  return character::Skill::Linguistics;
    throw std::runtime_error("schema_v0: unknown skill id: " + s);
}

quest::QuestPhase phase_from_id(const std::string& s) {
    if (s == "hidden")    return quest::QuestPhase::Hidden;
    if (s == "offered")   return quest::QuestPhase::Offered;
    if (s == "active")    return quest::QuestPhase::Active;
    if (s == "completed") return quest::QuestPhase::Completed;
    if (s == "failed")    return quest::QuestPhase::Failed;
    throw std::runtime_error("schema_v0: unknown quest phase: " + s);
}

}  // namespace

json::Value player_to_json(const PlayerSave& p) {
    Value obj(json::Object{});
    obj["schema_version"] = static_cast<std::int64_t>(kSchemaVersion);
    obj["name"]       = p.name;
    obj["race"]       = p.race;
    obj["background"] = p.background;

    obj["level"]        = static_cast<std::int64_t>(p.level.level);
    obj["experience"]   = static_cast<std::int64_t>(p.level.xp);
    obj["xp_to_next"]   = static_cast<std::int64_t>(p.level.xp_to_next);
    obj["attribute_points_unspent"] = static_cast<std::int64_t>(p.level.attribute_points);
    obj["skill_pool"]   = static_cast<std::int64_t>(p.level.skill_pool);

    {
        Value attrs(json::Object{});
        for (std::size_t i = 0; i < character::kAttributeCount; ++i) {
            attrs[character::attribute_id(static_cast<character::Attribute>(i))]
                = static_cast<std::int64_t>(p.attributes.values[i]);
        }
        obj["attributes"] = std::move(attrs);
    }
    {
        Value skills(json::Object{});
        for (std::size_t i = 0; i < character::kSkillCount; ++i) {
            skills[character::skill_id(static_cast<character::Skill>(i))]
                = static_cast<std::int64_t>(p.skills.values[i]);
        }
        obj["skills"] = std::move(skills);
    }
    {
        Value stats(json::Object{});
        stats["hp"]     = static_cast<std::int64_t>(p.hp);
        stats["hp_max"] = static_cast<std::int64_t>(p.hp_max);
        stats["vp"]     = static_cast<std::int64_t>(p.vp);
        stats["vp_max"] = static_cast<std::int64_t>(p.vp_max);
        stats["sp"]     = static_cast<std::int64_t>(p.sp);
        stats["sp_max"] = static_cast<std::int64_t>(p.sp_max);
        obj["stats"] = std::move(stats);
    }
    {
        Value conds(json::Array{});
        for (auto const& ac : p.conditions.active) {
            Value c(json::Object{});
            c["type"]        = character::condition_id(ac.type);
            c["duration_ms"] = static_cast<std::int64_t>(ac.duration_ms);
            c["magnitude"]   = static_cast<std::int64_t>(ac.magnitude);
            c["source"]      = static_cast<std::int64_t>(ac.source);
            conds.as_array().push_back(std::move(c));
        }
        obj["conditions"] = std::move(conds);
    }
    {
        Value inv(json::Object{});
        Value items(json::Array{});
        for (auto const& s : p.inventory.items) {
            Value it(json::Object{});
            it["id"]        = static_cast<std::int64_t>(s.id.value);
            it["count"]     = static_cast<std::int64_t>(s.count);
            it["condition"] = static_cast<std::int64_t>(s.condition);
            items.as_array().push_back(std::move(it));
        }
        inv["items"] = std::move(items);
        Value eq(json::Object{});
        for (auto const& [slot, id] : p.inventory.equipped) {
            eq[character::equip_slot_id(slot)] = static_cast<std::int64_t>(id.value);
        }
        inv["equipped"] = std::move(eq);
        inv["max_slots"]  = static_cast<std::int64_t>(p.inventory.max_slots);
        inv["max_weight"] = p.inventory.max_weight;
        inv["current_weight"] = p.inventory.current_weight;
        obj["inventory"] = std::move(inv);
    }
    {
        Value sk(json::Array{});
        for (auto const& s : p.spells_known) sk.as_array().push_back(s);
        obj["spells_known"] = std::move(sk);
    }
    {
        Value book(json::Object{});
        for (auto const& [k, v] : p.spell_book) book[k] = v;
        obj["spell_book"] = std::move(book);
    }
    {
        Value kt(json::Array{});
        for (auto const& s : p.known_topics) kt.as_array().push_back(s);
        obj["known_topics"] = std::move(kt);
    }
    return obj;
}

json::Value world_to_json(const WorldSave& w) {
    Value obj(json::Object{});
    obj["schema_version"] = static_cast<std::int64_t>(kSchemaVersion);

    {
        Value pos(json::Object{});
        pos["map_id"] = w.current_map;
        pos["x"] = static_cast<std::int64_t>(w.current_x);
        pos["y"] = static_cast<std::int64_t>(w.current_y);
        obj["player_location"] = std::move(pos);
    }
    obj["rng_seed"] = static_cast<std::int64_t>(w.rng_seed);
    obj["play_time_seconds"] = static_cast<std::int64_t>(w.play_time_seconds);

    {
        Value t(json::Object{});
        t["year"]   = static_cast<std::int64_t>(w.game_year);
        t["month"]  = static_cast<std::int64_t>(w.game_month);
        t["day"]    = static_cast<std::int64_t>(w.game_day);
        t["hour"]   = static_cast<std::int64_t>(w.game_hour);
        t["minute"] = static_cast<std::int64_t>(w.game_minute);
        obj["game_time"] = std::move(t);
    }
    // flags: reuse FlagStore::to_json string format for parity.
    obj["flags"] = json::Value(json::Object{});
    obj["flags_raw"] = w.flags ? w.flags->to_json() : std::string("{}");

    {
        Value quests(json::Object{});
        for (auto const& qs : w.quests) {
            Value q(json::Object{});
            q["state"] = quest::quest_phase_name(qs.phase);
            q["current_stage"] = qs.current_stage;
            Value visited(json::Array{});
            for (auto const& v : qs.visited) visited.as_array().push_back(v);
            q["visited_stages"] = std::move(visited);
            Value cnts(json::Object{});
            for (auto const& [k, v] : qs.counters) cnts[k] = static_cast<std::int64_t>(v);
            q["counters"] = std::move(cnts);
            if (!qs.started_at.empty()) q["started_at"] = qs.started_at;
            quests[qs.id] = std::move(q);
        }
        obj["quests"] = std::move(quests);
    }
    {
        Value factions(json::Object{});
        for (auto const& [k, v] : w.faction_rep) factions[k] = static_cast<std::int64_t>(v);
        obj["factions"] = std::move(factions);
    }
    {
        Value npcs(json::Object{});
        for (auto const& [id, n] : w.npcs) {
            Value nv(json::Object{});
            nv["alive"] = n.alive;
            nv["current_map"] = n.current_map;
            nv["x"] = static_cast<std::int64_t>(n.x);
            nv["y"] = static_cast<std::int64_t>(n.y);
            nv["schedule_offset_min"] = static_cast<std::int64_t>(n.schedule_offset_min);
            nv["disposition"] = static_cast<std::int64_t>(n.disposition);
            Value inv(json::Array{});
            for (auto const& s : n.inventory) inv.as_array().push_back(s);
            nv["inventory"] = std::move(inv);
            Value fs(json::Array{});
            for (auto const& s : n.flags) fs.as_array().push_back(s);
            nv["flags"] = std::move(fs);
            npcs[id] = std::move(nv);
        }
        obj["npcs"] = std::move(npcs);
    }
    {
        Value creatures(json::Object{});
        for (auto const& [id, c] : w.creatures) {
            Value cv(json::Object{});
            cv["alive"] = c.alive;
            cv["killed_by_player"] = c.killed_by_player;
            if (!c.death_time.empty()) cv["death_time"] = c.death_time;
            cv["corpse_looted"] = c.corpse_looted;
            creatures[id] = std::move(cv);
        }
        obj["creatures"] = std::move(creatures);
    }
    {
        Value containers(json::Object{});
        for (auto const& [id, c] : w.containers) {
            Value cv(json::Object{});
            cv["locked"] = c.locked;
            cv["opened"] = c.opened;
            Value contents(json::Array{});
            for (auto const& s : c.contents) contents.as_array().push_back(s);
            cv["contents"] = std::move(contents);
            containers[id] = std::move(cv);
        }
        obj["containers"] = std::move(containers);
    }
    {
        Value doors(json::Object{});
        for (auto const& [id, d] : w.doors) {
            Value dv(json::Object{});
            dv["open"] = d.open;
            dv["locked"] = d.locked;
            dv["key_id"] = d.key_id;
            doors[id] = std::move(dv);
        }
        obj["doors"] = std::move(doors);
    }
    {
        Value triggers(json::Object{});
        for (auto const& [id, t] : w.triggers) {
            Value tv(json::Object{});
            tv["fired_count"] = static_cast<std::int64_t>(t.fired_count);
            if (!t.last_fired.empty()) tv["last_fired"] = t.last_fired;
            triggers[id] = std::move(tv);
        }
        obj["triggers"] = std::move(triggers);
    }
    {
        Value gc(json::Object{});
        gc["kills_total"] = static_cast<std::int64_t>(w.counters.kills_total);
        Value kbf(json::Object{});
        for (auto const& [k, v] : w.counters.kills_by_faction) kbf[k] = static_cast<std::int64_t>(v);
        gc["kills_by_faction"] = std::move(kbf);
        gc["books_read"] = static_cast<std::int64_t>(w.counters.books_read);
        gc["secrets_found"] = static_cast<std::int64_t>(w.counters.secrets_found);
        gc["npc_conversations"] = static_cast<std::int64_t>(w.counters.npc_conversations);
        obj["global_counters"] = std::move(gc);
    }
    {
        Value dm(json::Object{});
        for (auto const& [id, bytes] : w.dirty_maps) {
            // Encode the bytes as a hex string so the JSON stays valid.
            static constexpr char kHex[] = "0123456789abcdef";
            std::string hex;
            hex.reserve(bytes.size() * 2);
            for (auto b : bytes) {
                hex += kHex[(b >> 4) & 0xF];
                hex += kHex[b & 0xF];
            }
            dm[id] = std::move(hex);
        }
        obj["dirty_maps"] = std::move(dm);
    }
    return obj;
}

json::Value meta_to_json(const SaveMeta& m) {
    Value obj(json::Object{});
    obj["version"]         = static_cast<std::int64_t>(kSchemaVersion);
    obj["schema_version"]  = static_cast<std::int64_t>(kSchemaVersion);
    obj["save_name"]       = m.save_name;
    obj["created_at"]      = m.created_at;
    obj["last_played_at"]  = m.last_played_at;
    obj["play_time_seconds"] = static_cast<std::int64_t>(m.play_time_seconds);
    obj["player_level"]    = static_cast<std::int64_t>(m.player_level);
    {
        Value pos(json::Object{});
        pos["map_id"] = m.player_map;
        pos["x"] = static_cast<std::int64_t>(m.player_x);
        pos["y"] = static_cast<std::int64_t>(m.player_y);
        obj["player_location"] = std::move(pos);
    }
    obj["rng_seed"] = static_cast<std::int64_t>(m.rng_seed);
    {
        Value t(json::Object{});
        t["year"]   = static_cast<std::int64_t>(m.game_time.year);
        t["month"]  = static_cast<std::int64_t>(m.game_time.month);
        t["day"]    = static_cast<std::int64_t>(m.game_time.day);
        t["hour"]   = static_cast<std::int64_t>(m.game_time.hour);
        t["minute"] = static_cast<std::int64_t>(m.game_time.minute);
        obj["game_time"] = std::move(t);
    }
    obj["weather"]    = m.weather;
    obj["difficulty"] = m.difficulty;
    obj["permadeath"] = m.permadeath;
    obj["version_str"]= m.version;
    obj["checksum"]   = static_cast<std::int64_t>(m.checksum);
    return obj;
}

void player_from_json(const json::Value& v, PlayerSave& out) {
    require(v, "player");
    out.name = v["name"].str_or("Stranger");
    out.race = v["race"].str_or("human");
    out.background = v["background"].str_or("woke_in_crypt");

    out.level.level          = static_cast<int>(v["level"].int_or(1));
    out.level.xp             = static_cast<int>(v["experience"].int_or(0));
    out.level.xp_to_next     = static_cast<int>(v["xp_to_next"].int_or(100));
    out.level.attribute_points = static_cast<int>(v["attribute_points_unspent"].int_or(0));
    out.level.skill_pool     = static_cast<int>(v["skill_pool"].int_or(0));

    if (v.contains("attributes") && v["attributes"].is_object()) {
        for (auto const& [k, val] : v["attributes"].as_object()) {
            auto a = attr_from_id(k);
            out.attributes[a] = static_cast<int>(val.int_or(0));
        }
    }
    if (v.contains("skills") && v["skills"].is_object()) {
        for (auto const& [k, val] : v["skills"].as_object()) {
            auto s = skill_from_id(k);
            out.skills[s] = static_cast<int>(val.int_or(0));
        }
    }
    if (v.contains("stats") && v["stats"].is_object()) {
        auto const& s = v["stats"].as_object();
        out.hp     = static_cast<int>(s.at("hp").int_or(0));
        out.hp_max = static_cast<int>(s.at("hp_max").int_or(0));
        out.vp     = static_cast<int>(s.at("vp").int_or(0));
        out.vp_max = static_cast<int>(s.at("vp_max").int_or(0));
        out.sp     = static_cast<int>(s.at("sp").int_or(0));
        out.sp_max = static_cast<int>(s.at("sp_max").int_or(0));
    }
    if (v.contains("conditions") && v["conditions"].is_array()) {
        out.conditions.active.clear();
        for (auto const& c : v["conditions"].as_array()) {
            if (!c.is_object()) continue;
            auto type = character::condition_from_id(c["type"].str_or("unknown").c_str());
            if (type == character::Condition::Count) continue;
            out.conditions.active.push_back({
                type,
                static_cast<int>(c["duration_ms"].int_or(0)),
                static_cast<int>(c["magnitude"].int_or(0)),
                static_cast<std::uint64_t>(c["source"].int_or(0)),
            });
        }
    }
    if (v.contains("inventory") && v["inventory"].is_object()) {
        auto const& inv = v["inventory"].as_object();
        out.inventory.items.clear();
        if (inv.count("items") && inv.at("items").is_array()) {
            for (auto const& it : inv.at("items").as_array()) {
                if (!it.is_object()) continue;
                out.inventory.items.push_back({
                    core::ItemId(static_cast<std::size_t>(it["id"].int_or(0))),
                    static_cast<int>(it["count"].int_or(1)),
                    static_cast<int>(it["condition"].int_or(100)),
                });
            }
        }
        out.inventory.equipped.clear();
        if (inv.count("equipped") && inv.at("equipped").is_object()) {
            for (auto const& [k, val] : inv.at("equipped").as_object()) {
                auto slot = character::equip_slot_from_id(k.c_str());
                if (slot == character::EquipSlot::Count) continue;
                out.inventory.equipped[slot] = core::ItemId(static_cast<std::size_t>(val.int_or(0)));
            }
        }
        out.inventory.max_slots     = static_cast<int>(inv.at("max_slots").int_or(100));
        out.inventory.max_weight    = static_cast<float>(inv.at("max_weight").dbl_or(0.0));
        out.inventory.current_weight= static_cast<float>(inv.at("current_weight").dbl_or(0.0));
    }
    if (v.contains("spells_known") && v["spells_known"].is_array()) {
        out.spells_known.clear();
        for (auto const& s : v["spells_known"].as_array()) {
            if (s.is_string()) out.spells_known.push_back(s.as_string());
        }
    }
    if (v.contains("spell_book") && v["spell_book"].is_object()) {
        out.spell_book.clear();
        for (auto const& [k, val] : v["spell_book"].as_object()) {
            out.spell_book[k] = val.str_or("");
        }
    }
    if (v.contains("known_topics") && v["known_topics"].is_array()) {
        out.known_topics.clear();
        for (auto const& s : v["known_topics"].as_array()) {
            if (s.is_string()) out.known_topics.push_back(s.as_string());
        }
    }
}

void world_from_json(const json::Value& v, WorldSave& out) {
    require(v, "world");
    if (v.contains("player_location") && v["player_location"].is_object()) {
        auto const& pl = v["player_location"].as_object();
        out.current_map = pl.at("map_id").str_or("");
        out.current_x   = static_cast<int>(pl.at("x").int_or(0));
        out.current_y   = static_cast<int>(pl.at("y").int_or(0));
    }
    out.rng_seed          = static_cast<int>(v["rng_seed"].int_or(0));
    out.play_time_seconds = static_cast<int>(v["play_time_seconds"].int_or(0));
    if (v.contains("game_time") && v["game_time"].is_object()) {
        auto const& t = v["game_time"].as_object();
        out.game_year   = static_cast<int>(t.at("year").int_or(220));
        out.game_month  = static_cast<int>(t.at("month").int_or(1));
        out.game_day    = static_cast<int>(t.at("day").int_or(1));
        out.game_hour   = static_cast<int>(t.at("hour").int_or(6));
        out.game_minute = static_cast<int>(t.at("minute").int_or(0));
    }
    // flags_raw stores the FlagStore JSON blob verbatim.
    if (!out.flags) out.flags = std::make_unique<quest::FlagStore>();
    if (v.contains("flags_raw") && v["flags_raw"].is_string()) {
        out.flags->from_json(v["flags_raw"].as_string());
    } else {
        out.flags->reset_all();
    }
    if (v.contains("quests") && v["quests"].is_object()) {
        out.quests.clear();
        for (auto const& [id, qv] : v["quests"].as_object()) {
            if (!qv.is_object()) continue;
            WorldSave::QuestState qs;
            qs.id            = id;
            qs.phase         = phase_from_id(qv.at("state").str_or("hidden"));
            qs.current_stage = qv.at("current_stage").str_or("");
            if (qv.contains("visited_stages") && qv["visited_stages"].is_array()) {
                for (auto const& s : qv["visited_stages"].as_array()) {
                    if (s.is_string()) qs.visited.push_back(s.as_string());
                }
            }
            if (qv.contains("counters") && qv["counters"].is_object()) {
                for (auto const& [k, val] : qv["counters"].as_object()) {
                    qs.counters[k] = val.int_or(0);
                }
            }
            if (qv.contains("started_at")) qs.started_at = qv["started_at"].str_or("");
            out.quests.push_back(std::move(qs));
        }
    }
    if (v.contains("factions") && v["factions"].is_object()) {
        out.faction_rep.clear();
        for (auto const& [k, val] : v["factions"].as_object()) {
            out.faction_rep[k] = static_cast<int>(val.int_or(0));
        }
    }
    if (v.contains("npcs") && v["npcs"].is_object()) {
        out.npcs.clear();
        for (auto const& [id, nv] : v["npcs"].as_object()) {
            if (!nv.is_object()) continue;
            WorldSave::NpcState n;
            n.alive      = nv["alive"].bool_or(true);
            n.current_map= nv["current_map"].str_or("");
            n.x          = static_cast<int>(nv["x"].int_or(0));
            n.y          = static_cast<int>(nv["y"].int_or(0));
            n.schedule_offset_min = static_cast<int>(nv["schedule_offset_min"].int_or(0));
            n.disposition= static_cast<int>(nv["disposition"].int_or(0));
            if (nv.contains("inventory") && nv["inventory"].is_array()) {
                for (auto const& s : nv["inventory"].as_array()) {
                    if (s.is_string()) n.inventory.push_back(s.as_string());
                }
            }
            if (nv.contains("flags") && nv["flags"].is_array()) {
                for (auto const& s : nv["flags"].as_array()) {
                    if (s.is_string()) n.flags.insert(s.as_string());
                }
            }
            out.npcs[id] = std::move(n);
        }
    }
    if (v.contains("creatures") && v["creatures"].is_object()) {
        out.creatures.clear();
        for (auto const& [id, cv] : v["creatures"].as_object()) {
            if (!cv.is_object()) continue;
            WorldSave::CreatureState c;
            c.alive = cv["alive"].bool_or(true);
            c.killed_by_player = cv["killed_by_player"].bool_or(false);
            if (cv.contains("death_time")) c.death_time = cv["death_time"].str_or("");
            c.corpse_looted = cv["corpse_looted"].bool_or(false);
            out.creatures[id] = std::move(c);
        }
    }
    if (v.contains("containers") && v["containers"].is_object()) {
        out.containers.clear();
        for (auto const& [id, cv] : v["containers"].as_object()) {
            if (!cv.is_object()) continue;
            WorldSave::ContainerState c;
            c.locked = cv["locked"].bool_or(false);
            c.opened = cv["opened"].bool_or(false);
            if (cv.contains("contents") && cv["contents"].is_array()) {
                for (auto const& s : cv["contents"].as_array()) {
                    if (s.is_string()) c.contents.push_back(s.as_string());
                }
            }
            out.containers[id] = std::move(c);
        }
    }
    if (v.contains("doors") && v["doors"].is_object()) {
        out.doors.clear();
        for (auto const& [id, dv] : v["doors"].as_object()) {
            if (!dv.is_object()) continue;
            WorldSave::DoorState d;
            d.open   = dv["open"].bool_or(false);
            d.locked = dv["locked"].bool_or(false);
            if (dv.contains("key_id") && dv["key_id"].is_string()) d.key_id = dv["key_id"].as_string();
            out.doors[id] = std::move(d);
        }
    }
    if (v.contains("triggers") && v["triggers"].is_object()) {
        out.triggers.clear();
        for (auto const& [id, tv] : v["triggers"].as_object()) {
            if (!tv.is_object()) continue;
            WorldSave::TriggerState t;
            t.fired_count = static_cast<int>(tv["fired_count"].int_or(0));
            if (tv.contains("last_fired")) t.last_fired = tv["last_fired"].str_or("");
            out.triggers[id] = std::move(t);
        }
    }
    if (v.contains("global_counters") && v["global_counters"].is_object()) {
        auto const& gc = v["global_counters"].as_object();
        out.counters.kills_total        = static_cast<int>(gc.at("kills_total").int_or(0));
        out.counters.books_read         = static_cast<int>(gc.at("books_read").int_or(0));
        out.counters.secrets_found      = static_cast<int>(gc.at("secrets_found").int_or(0));
        out.counters.npc_conversations  = static_cast<int>(gc.at("npc_conversations").int_or(0));
        out.counters.kills_by_faction.clear();
        if (gc.count("kills_by_faction") && gc.at("kills_by_faction").is_object()) {
            for (auto const& [k, val] : gc.at("kills_by_faction").as_object()) {
                out.counters.kills_by_faction[k] = static_cast<int>(val.int_or(0));
            }
        }
    }
    if (v.contains("dirty_maps") && v["dirty_maps"].is_object()) {
        out.dirty_maps.clear();
        for (auto const& [id, hex] : v["dirty_maps"].as_object()) {
            if (!hex.is_string()) continue;
            auto const& s = hex.as_string();
            std::vector<std::uint8_t> bytes;
            bytes.reserve(s.size() / 2);
            auto nib = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
                if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
                return -1;
            };
            for (std::size_t i = 0; i + 1 < s.size(); i += 2) {
                int hi = nib(s[i]);
                int lo = nib(s[i + 1]);
                if (hi < 0 || lo < 0) break;
                bytes.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
            }
            out.dirty_maps[id] = std::move(bytes);
        }
    }
}

void meta_from_json(const json::Value& v, SaveMeta& out) {
    require(v, "meta");
    out.schema_version   = static_cast<int>(v["schema_version"].int_or(0));
    out.version          = v["version_str"].str_or("0.1.0");
    out.save_name        = v["save_name"].str_or("");
    out.created_at       = v["created_at"].str_or("");
    out.last_played_at   = v["last_played_at"].str_or("");
    out.play_time_seconds= static_cast<int>(v["play_time_seconds"].int_or(0));
    out.player_level     = static_cast<int>(v["player_level"].int_or(1));
    if (v.contains("player_location") && v["player_location"].is_object()) {
        auto const& pl = v["player_location"].as_object();
        out.player_map = pl.at("map_id").str_or("");
        out.player_x   = static_cast<int>(pl.at("x").int_or(0));
        out.player_y   = static_cast<int>(pl.at("y").int_or(0));
    }
    out.rng_seed = static_cast<int>(v["rng_seed"].int_or(0));
    if (v.contains("game_time") && v["game_time"].is_object()) {
        auto const& t = v["game_time"].as_object();
        out.game_time.year   = static_cast<int>(t.at("year").int_or(220));
        out.game_time.month  = static_cast<int>(t.at("month").int_or(1));
        out.game_time.day    = static_cast<int>(t.at("day").int_or(1));
        out.game_time.hour   = static_cast<int>(t.at("hour").int_or(6));
        out.game_time.minute = static_cast<int>(t.at("minute").int_or(0));
    }
    out.weather    = v["weather"].str_or("clear");
    out.difficulty = v["difficulty"].str_or("normal");
    out.permadeath = v["permadeath"].bool_or(false);
    out.checksum   = static_cast<std::uint32_t>(v["checksum"].int_or(0));
}

}  // namespace save
}  // namespace ash