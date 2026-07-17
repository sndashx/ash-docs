#include "quest/consequence.hpp"

#include <cstdlib>

namespace ash {
namespace quest {

namespace {

std::int64_t to_int(const std::string& s) {
    try { return static_cast<std::int64_t>(std::stoll(s)); }
    catch (...) { return 0; }
}

}  // namespace

ConsequenceRunner::ConsequenceRunner(FlagStore& flags, WorldHooks hooks)
    : flags_(flags), hooks_(std::move(hooks)) {
    obs_ = flags_.add_observer([this](const FlagChange& ch) {
        on_flag_change(ch);
    });
}

void ConsequenceRunner::add(Consequence c) {
    rules_.push_back(std::move(c));
}

void ConsequenceRunner::set_logger(
    std::function<void(const Consequence&, const FlagChange&)> l) {
    logger_ = std::move(l);
}

void ConsequenceRunner::on_flag_change(const FlagChange& ch) {
    for (auto& rule : rules_) {
        if (rule.trigger_flag != ch.key) continue;
        bool match = true;
        if (rule.want_bool) {
            auto* b = std::get_if<bool>(&ch.current);
            match = b && (*b == *rule.want_bool);
        }
        if (match && rule.want_int) {
            auto* i = std::get_if<std::int64_t>(&ch.current);
            match = i && (*i == *rule.want_int);
        }
        if (match && rule.want_string) {
            auto* s = std::get_if<std::string>(&ch.current);
            match = s && (*s == *rule.want_string);
        }
        if (!match) continue;

        ++fires_;
        fires_log_.push_back({&rule, ch});
        if (logger_) logger_(rule, ch);

        switch (rule.kind) {
            case ConsequenceKind::SpawnNpc:
                if (rule.args.size() >= 4 && hooks_.spawn_npc)
                    hooks_.spawn_npc(rule.args[0], rule.args[1],
                                     static_cast<int>(to_int(rule.args[2])),
                                     static_cast<int>(to_int(rule.args[3])));
                break;
            case ConsequenceKind::DespawnNpc:
                if (!rule.args.empty() && hooks_.despawn_npc)
                    hooks_.despawn_npc(rule.args[0]);
                break;
            case ConsequenceKind::ModifyEntity:
                // Free-form: callers can register a custom hook via
                // hooks_.teleport or any side-channel.
                if (rule.args.size() >= 4 && hooks_.teleport)
                    hooks_.teleport(rule.args[0], rule.args[1],
                                    static_cast<int>(to_int(rule.args[2])),
                                    static_cast<int>(to_int(rule.args[3])));
                break;
            case ConsequenceKind::RunScript:
                // No script engine in v1; treated as a no-op. Games can
                // arrange side effects by chaining consequences with a
                // custom hook on FlagStore.
                break;
        }
    }
}

}  // namespace quest
}  // namespace ash