#pragma once
/// Phase 10: schema migration runner (per appendix D + 10.07).
///
/// Each migration is a pure function on JSON. The registry maps `from`
/// to a list of step migrations, applied in order until the document is
/// at the engine's current schema version. We log every migration that
/// runs; the save manager refuses to load any save older than the
/// minimum supported version (kMinSupportedSchema).
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "core/json.hpp"
#include "save/schema_v0.hpp"

namespace ash {
namespace save {

constexpr int kMinSupportedSchema = 0;       /// Per appendix D: no older saves.
constexpr int kCurrentSchema      = kEngineSchema;

/// One migration step: from `from` → `to`. The function mutates and
/// returns the JSON document.
struct Migration {
    int from{0};
    int to{0};
    std::string description;
    std::function<json::Value(const json::Value&)> transform;
};

class MigrationRegistry {
public:
    MigrationRegistry();
    /// Apply all migrations until the document's `schema_version` equals
    /// `kCurrentSchema`. Throws if `from` is below `kMinSupportedSchema`
    /// or if the chain has gaps. Returns the migrated document.
    json::Value migrate(const json::Value& doc, int from_version) const;
    /// All registered migrations sorted by (from, to). For diagnostics.
    const std::vector<Migration>& all() const { return migrations_; }
private:
    std::vector<Migration> migrations_;
};

/// Convenience free function equivalent to
/// `MigrationRegistry{}.migrate(doc, from)`. Used by load().
json::Value run_migrations(const json::Value& doc, int from_version);

}  // namespace save
}  // namespace ash