#include "test_harness.hpp"
#include "quest/flag_store.hpp"

#include <string>
#include <vector>

using ash::quest::FlagStore;
using ash::quest::FlagChange;

TEST_CASE("flag_store.set_int", "[quest][flag]") {
    FlagStore fs;
    fs.set_int("count", 7);
    REQUIRE(fs.has("count"));
    REQUIRE(fs.get_int("count").value() == 7);
    REQUIRE(fs.size() == 1u);
}

TEST_CASE("flag_store.set_bool", "[quest][flag]") {
    FlagStore fs;
    fs.set_bool("met_npc", true);
    REQUIRE(fs.get_bool("met_npc").value() == true);
    REQUIRE(!fs.get_int("met_npc").has_value());
}

TEST_CASE("flag_store.set_string", "[quest][flag]") {
    FlagStore fs;
    fs.set_string("name", "Maren");
    REQUIRE(fs.get_string("name").value() == "Maren");
}

TEST_CASE("flag_store.increment", "[quest][flag]") {
    FlagStore fs;
    fs.increment("score");          // 0 -> 1
    fs.increment("score", 4);       // 1 -> 5
    REQUIRE(fs.get_int("score").value() == 5);
    fs.decrement("score", 2);       // 5 -> 3
    REQUIRE(fs.get_int("score").value() == 3);
}

TEST_CASE("flag_store.toggle", "[quest][flag]") {
    FlagStore fs;
    fs.toggle("a");
    REQUIRE(fs.get_bool("a").value() == true);
    fs.toggle("a");
    REQUIRE(fs.get_bool("a").value() == false);
}

TEST_CASE("flag_store.clear", "[quest][flag]") {
    FlagStore fs;
    fs.set_int("k", 9);
    REQUIRE(fs.has("k"));
    fs.clear("k");
    REQUIRE(!fs.has("k"));
}

TEST_CASE("flag_store.observers_fire_on_set", "[quest][flag]") {
    FlagStore fs;
    std::vector<std::string> events;
    auto h = fs.add_observer([&](const FlagChange& ch) {
        events.push_back(ch.key);
    });
    fs.set_int("a", 1);
    fs.set_bool("b", true);
    fs.increment("c");
    REQUIRE(events.size() == 3u);
    REQUIRE(events[0] == "a");
    REQUIRE(events[1] == "b");
    REQUIRE(events[2] == "c");
    fs.remove_observer(h);
    fs.set_int("d", 1);
    REQUIRE(events.size() == 3u);
}

TEST_CASE("flag_store.persistence_roundtrip", "[quest][flag]") {
    FlagStore fs;
    fs.set_int("score", 42);
    fs.set_bool("met_maren", true);
    fs.set_string("location", "silvercliff");

    auto blob = fs.to_json();
    REQUIRE(!blob.empty());

    FlagStore fs2;
    fs2.from_json(blob);
    REQUIRE(fs2.get_int("score").value() == 42);
    REQUIRE(fs2.get_bool("met_maren").value() == true);
    REQUIRE(fs2.get_string("location").value() == "silvercliff");
}

TEST_CASE("flag_store.previous_value_in_change", "[quest][flag]") {
    FlagStore fs;
    bool saw_prev = false;
    fs.add_observer([&](const FlagChange& ch) {
        if (ch.key == "k" && ch.previous.has_value()) {
            saw_prev = std::get<std::int64_t>(*ch.previous) == 5;
        }
    });
    fs.set_int("k", 5);
    fs.set_int("k", 9);
    REQUIRE(saw_prev);
}
