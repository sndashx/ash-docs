#pragma once
/// Phase 10: Schema v0 readers/writers for the player + world JSON shape.
///
/// Each function takes/returns a `json::Value` whose shape matches the
/// fields in `appendices/D-save-schema.txt`. The conversion routines are
/// pure functions used by both `save()` (write direction) and `load()`
/// (read direction).
#include "core/json.hpp"
#include "save/save_data.hpp"

namespace ash {
namespace save {

constexpr int kSchemaVersion = 0;   /// v0 — the schema we WRITE for the slot payload.
constexpr int kEngineSchema  = 1;   /// Current engine schema after migrations.

/// Build the JSON object that represents `player` per appendix D
/// (`player.v0.json`). Pretty-print is the caller's choice.
json::Value player_to_json(const PlayerSave& player);

/// Build the JSON object for `world` per appendix D (`world.v0.json`).
json::Value world_to_json(const WorldSave& world);

/// Build the JSON object for `meta` per appendix D (`meta.json`).
json::Value meta_to_json(const SaveMeta& meta);

/// Apply `player_obj` to the supplied PlayerSave in-place. Throws
/// `std::runtime_error` on missing or wrong-type required fields.
void player_from_json(const json::Value& player_obj, PlayerSave& out);

/// Apply `world_obj` to the supplied WorldSave in-place.
void world_from_json(const json::Value& world_obj, WorldSave& out);

/// Apply `meta_obj` to the supplied SaveMeta in-place.
void meta_from_json(const json::Value& meta_obj, SaveMeta& out);

}  // namespace save
}  // namespace ash
