/// Phase 10: migration runner tests.
#include "test_harness.hpp"

#include "core/json.hpp"
#include "save/migrate.hpp"

using namespace ash;

TEST_CASE("migrate: v0 → v1 renames legacy flag", "[save][migrate]") {
    json::Value doc(json::Object{});
    doc["schema_version"] = json::Value(static_cast<std::int64_t>(0));
    {
        json::Value flags(json::Object{});
        flags["npcs_killed_legacy"] = json::Value(static_cast<std::int64_t>(4));
        flags["maren_met"]          = json::Value(true);
        doc["flags"] = std::move(flags);
    }
    doc["global_counters"] = json::Value(json::Object{});

    auto out = save::run_migrations(doc, 0);
    REQUIRE(out.contains("flags"));
    auto& f = out["flags"].as_object();
    REQUIRE(f.find("npcs_killed_legacy") == f.end());
    REQUIRE(out.contains("global_counters"));
    REQUIRE(out["global_counters"]["kills_total"].int_or(0) == 4);
}

TEST_CASE("migrate: rejects versions below minimum", "[save][migrate]") {
    json::Value doc(json::Object{});
    doc["schema_version"] = json::Value(static_cast<std::int64_t>(0));
    bool threw = false;
    try { save::run_migrations(doc, -5); }
    catch (std::exception const&) { threw = true; }
    REQUIRE(threw);
}

TEST_CASE("migrate: rejects forward-compat schema", "[save][migrate]") {
    json::Value doc(json::Object{});
    doc["schema_version"] = json::Value(static_cast<std::int64_t>(0));
    bool threw = false;
    try { save::run_migrations(doc, 99); }
    catch (std::exception const&) { threw = true; }
    REQUIRE(threw);
}

TEST_CASE("migrate: registry lists at least one v0→v1 migration", "[save][migrate]") {
    save::MigrationRegistry reg;
    REQUIRE(reg.all().size() >= 1);
    bool found = false;
    for (auto const& m : reg.all()) {
        if (m.from == 0 && m.to == 1) { found = true; break; }
    }
    REQUIRE(found);
}