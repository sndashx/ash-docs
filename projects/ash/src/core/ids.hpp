#pragma once
/// Phase 01: Strongly-typed IDs (Entity, Map, Item, ...).
#include <compare>
#include <cstddef>

namespace ash {
namespace core {

template <typename Tag>
struct StrongId
{
    std::size_t value{0};

    constexpr StrongId() = default;
    constexpr explicit StrongId(std::size_t v) : value(v) {}

    constexpr bool operator==(const StrongId&) const = default;
    constexpr auto operator<=>(const StrongId&) const = default;
};

struct EntityTag {};
struct MapTag {};
struct ItemTag {};
struct NpcTag {};
struct DialogueTag {};
struct QuestTag {};
struct FactionTag {};

using EntityId    = StrongId<EntityTag>;
using MapId       = StrongId<MapTag>;
using ItemId      = StrongId<ItemTag>;
using NpcId       = StrongId<NpcTag>;
using DialogueId  = StrongId<DialogueTag>;
using QuestId     = StrongId<QuestTag>;
using FactionId   = StrongId<FactionTag>;

}  // namespace core
}  // namespace ash
