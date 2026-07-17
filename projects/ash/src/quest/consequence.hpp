#pragma once
/// Phase 09: Consequence rules (per Pillar 8).
///
/// A consequence rule is fired when a flag transitions. The rules live
/// alongside the catalog and are typically loaded from world/consequences/
/// or constructed programmatically from .qst `on_*` action blocks. The
/// rule set is small and explicit: spawn / despawn / modify / run_script.
///
/// Engine integration: register a ConsequenceRunner on the FlagStore once
/// at boot. Any flag change then runs the relevant rules. The runner
/// shares QuestEngine's WorldHooks for spawn/despawn/etc.

#include "quest/flag_store.hpp"
#include "quest/qst_engine.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ash {
namespace quest {

enum class ConsequenceKind {
    SpawnNpc,
    DespawnNpc,
    ModifyEntity,
    RunScript,
};

struct Consequence {
    ConsequenceKind kind = ConsequenceKind::RunScript;
    /// Flag key to react to.
    std::string trigger_flag;
    /// Only fire when transitioning into this value (nullopt = any change).
    std::optional<bool>         want_bool;
    std::optional<std::int64_t> want_int;
    std::optional<std::string>  want_string;
    /// Action arguments (see Stage::on_*).
    std::vector<std::string> args;
};

class ConsequenceRunner {
public:
    ConsequenceRunner(FlagStore& flags, WorldHooks hooks);

    /// Add a rule.
    void add(Consequence c);

    /// Toggle logging of every fire (useful for tests).
    void set_logger(std::function<void(const Consequence&, const FlagChange&)> l);

    /// Number of fires so far (per-rule counter).
    std::size_t fires_total() const { return fires_; }

    /// Vector of fires (rule pointer + the change that triggered it).
    struct Fire { const Consequence* rule; FlagChange change; };
    std::vector<Fire> fires() const { return fires_log_; }

private:
    void on_flag_change(const FlagChange& ch);

    FlagStore&               flags_;
    WorldHooks               hooks_;
    std::vector<Consequence> rules_;
    std::size_t              obs_      = 0;
    std::size_t              fires_    = 0;
    std::vector<Fire>        fires_log_;
    std::function<void(const Consequence&, const FlagChange&)> logger_;
};

}  // namespace quest
}  // namespace ash