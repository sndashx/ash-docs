#pragma once
/// Phase 09: Flag store (per Pillar 8 + 14-phase-09-quests).
///
/// The flag store is the central typed key/value store that powers quest
/// progression, dialogue gating, and consequence rules. Keys map to one of
/// three value types (int / bool / string). Setting a flag notifies any
/// registered observers (used by the consequence engine).
///
/// The store is header-only safe (no external deps), uses std::variant for
/// the value union, and round-trips cleanly through JSON for Phase 10 save
/// integration (see to_json / from_json).

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

namespace ash {
namespace quest {

using FlagValue = std::variant<std::int64_t, bool, std::string>;

struct FlagChange {
    std::string key;
    /// Previous value (nullopt if newly created).
    std::optional<FlagValue> previous;
    /// New value.
    FlagValue current;
};

using FlagObserver = std::function<void(const FlagChange&)>;

class FlagStore {
public:
    /// Set an int value at key, creating the slot if missing.
    void set_int(const std::string& key, std::int64_t v);
    /// Set a bool value at key.
    void set_bool(const std::string& key, bool v);
    /// Set a string value at key.
    void set_string(const std::string& key, std::string v);

    /// Generic setter accepting a variant.
    void set(const std::string& key, FlagValue v);

    /// Remove the key entirely. Fires a notification if it existed.
    void clear(const std::string& key);

    /// Increment an int flag. Creates at 0 then adds 1.
    /// Non-int values are converted to int (true -> 1, false -> 0,
    /// string -> stoi with fallback 0).
    void increment(const std::string& key, std::int64_t delta = 1);

    /// Decrement an int flag.
    void decrement(const std::string& key, std::int64_t delta = 1);

    /// Toggle a bool flag. Creates at false then flips.
    void toggle(const std::string& key);

    /// Type-checked getter. Returns nullopt if missing or wrong type.
    std::optional<std::int64_t> get_int(const std::string& key) const;
    std::optional<bool>         get_bool(const std::string& key) const;
    std::optional<std::string>  get_string(const std::string& key) const;
    std::optional<FlagValue>    get(const std::string& key) const;

    /// True if a value exists at key (regardless of type).
    bool has(const std::string& key) const;

    /// Erase all flags. Notifies no-one.
    void reset_all();

    /// Number of flags currently stored.
    std::size_t size() const { return flags_.size(); }

    /// Observer hook. Returns an opaque handle (index) the caller can pass
    /// to remove_observer later. Observers fire on any set/clear/inc/dec/
    /// toggle call (NOT on reset_all, which is a bulk operation).
    std::size_t add_observer(FlagObserver obs);
    void        remove_observer(std::size_t handle);

    /// JSON round-trip (used by save system in Phase 10).
    std::string to_json() const;
    void        from_json(const std::string& blob);

private:
    void notify(const std::string& key,
                const std::optional<FlagValue>& previous,
                const FlagValue& current);

    std::unordered_map<std::string, FlagValue> flags_;
    std::vector<std::pair<FlagObserver, bool>> observers_;
};

}  // namespace quest
}  // namespace ash