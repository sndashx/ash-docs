#include "save/migrate.hpp"

#include <algorithm>
#include <stdexcept>

#include "core/log.hpp"

namespace ash {
namespace save {

namespace {

// Example v0 -> v1 migration: rename the legacy "npcs_killed" list flag
// into a counter. Kept as a documentation template even when v1 doesn't
// exist yet — exercises the chain and the registry plumbing (D27).
json::Value rename_flag_to_counter(const json::Value& in) {
    if (!in.is_object()) return in;
    json::Value out = in;
    if (out.contains("flags") && out["flags"].is_object()) {
        auto& f = out["flags"].as_object();
        auto it = f.find("npcs_killed_legacy");
        if (it != f.end() && it->second.is_int()) {
            // Promote the count into the global counter bucket and drop.
            std::int64_t killed = it->second.as_int();
            if (!out.contains("global_counters") || !out["global_counters"].is_object()) {
                out["global_counters"] = json::Value(json::Object{});
            }
            auto& gc = out["global_counters"].as_object();
            std::int64_t cur = 0;
            auto gcit = gc.find("kills_total");
            if (gcit != gc.end() && gcit->second.is_int()) cur = gcit->second.as_int();
            gc["kills_total"] = cur + killed;
            f.erase(it);
        }
    }
    return out;
}

}  // namespace

MigrationRegistry::MigrationRegistry() {
    // Register every (from, to) migration we know about. New entries go
    // at the back; the runner applies them in order from the document's
    // current schema version up to kCurrentSchema.
    migrations_.push_back(Migration{
        0, 1,
        "rename npcs_killed_legacy int into global_counters.kills_total",
        &rename_flag_to_counter,
    });
}

json::Value MigrationRegistry::migrate(const json::Value& doc, int from_version) const {
    if (from_version < kMinSupportedSchema) {
        throw std::runtime_error("save/migrate: schema version "
            + std::to_string(from_version) + " is below minimum supported "
            + std::to_string(kMinSupportedSchema));
    }
    if (from_version > kCurrentSchema) {
        throw std::runtime_error("save/migrate: schema version "
            + std::to_string(from_version) + " is newer than engine "
            + std::to_string(kCurrentSchema) + " (forward-compat error)");
    }
    if (from_version == kCurrentSchema) {
        return doc;
    }
    json::Value cur = doc;
    int version = from_version;
    while (version < kCurrentSchema) {
        auto it = std::find_if(migrations_.begin(), migrations_.end(),
            [version](Migration const& m) { return m.from == version; });
        if (it == migrations_.end()) {
            throw std::runtime_error("save/migrate: no path from version "
                + std::to_string(version) + " to "
                + std::to_string(kCurrentSchema));
        }
        log::info("save: applying migration v" + std::to_string(it->from)
                  + " -> v" + std::to_string(it->to)
                  + " (" + it->description + ")");
        cur = it->transform(cur);
        // Stamp the new schema_version if the doc carries one.
        if (cur.is_object() && cur.contains("schema_version")) {
            cur["schema_version"] = static_cast<std::int64_t>(it->to);
        }
        version = it->to;
    }
    return cur;
}

json::Value run_migrations(const json::Value& doc, int from_version) {
    return MigrationRegistry{}.migrate(doc, from_version);
}

}  // namespace save
}  // namespace ash