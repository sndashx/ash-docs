#include "quest/qst_engine.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace ash {
namespace quest {

namespace {

std::int64_t to_int(const std::string& s) {
    try { return static_cast<std::int64_t>(std::stoll(s)); }
    catch (...) { return 0; }
}

}  // namespace

// ---- QuestCatalog ----------------------------------------------------

void QuestCatalog::add(ParsedQuest q) {
    for (auto& existing : quests_) {
        if (existing.def.id == q.def.id) { existing = std::move(q); return; }
    }
    quests_.push_back(std::move(q));
}

const ParsedQuest* QuestCatalog::find(const std::string& id) const {
    for (auto& q : quests_) {
        if (q.def.id == id) return &q;
    }
    return nullptr;
}

std::vector<std::string> QuestCatalog::ids() const {
    std::vector<std::string> out;
    out.reserve(quests_.size());
    for (auto& q : quests_) out.push_back(q.def.id);
    return out;
}

// ---- QuestEngine -----------------------------------------------------

QuestEngine::QuestEngine(FlagStore& flags, const QuestCatalog& cat, WorldHooks hooks)
    : flags_(flags), cat_(cat), hooks_(std::move(hooks)) {}

bool QuestEngine::offer(const std::string& quest_id) {
    auto* def = cat_.find(quest_id);
    if (!def) return false;
    auto& rt = runs_[quest_id];
    if (rt.phase != QuestPhase::Hidden) return false;
    rt.phase = QuestPhase::Offered;
    fire({quest_id, "", Event::QuestOffered});
    return true;
}

bool QuestEngine::accept(const std::string& quest_id) {
    auto* def = cat_.find(quest_id);
    if (!def) return false;
    auto& rt = runs_[quest_id];
    if (rt.phase != QuestPhase::Offered && rt.phase != QuestPhase::Hidden) return false;
    rt.phase = QuestPhase::Active;
    rt.current_stage = !def->def.entry.empty()
                         ? def->def.entry
                         : (def->def.stages.empty()
                                ? std::string{}
                                : def->def.stages.front().id);
    rt.entered = false;
    fire({quest_id, rt.current_stage, Event::StageEnter});
    return true;
}

bool QuestEngine::start(const std::string& quest_id) {
    auto* def = cat_.find(quest_id);
    if (!def) return false;
    auto& rt = runs_[quest_id];
    if (rt.phase == QuestPhase::Active || rt.phase == QuestPhase::Completed) return false;
    rt.phase = QuestPhase::Active;
    rt.current_stage = !def->def.entry.empty()
                         ? def->def.entry
                         : (def->def.stages.empty()
                                ? std::string{}
                                : def->def.stages.front().id);
    rt.entered = false;
    fire({quest_id, rt.current_stage, Event::StageEnter});
    return true;
}

bool QuestEngine::complete(const std::string& quest_id) {
    auto it = runs_.find(quest_id);
    if (it == runs_.end()) return false;
    if (it->second.phase == QuestPhase::Completed) return false;
    it->second.phase = QuestPhase::Completed;
    auto sid = it->second.current_stage;
    fire({quest_id, sid, Event::QuestComplete});
    return true;
}

bool QuestEngine::fail(const std::string& quest_id) {
    auto it = runs_.find(quest_id);
    if (it == runs_.end()) return false;
    if (it->second.phase == QuestPhase::Failed) return false;
    it->second.phase = QuestPhase::Failed;
    auto sid = it->second.current_stage;
    fire({quest_id, sid, Event::QuestFailed});
    return true;
}

bool QuestEngine::abandon(const std::string& quest_id) {
    return fail(quest_id);
}

void QuestEngine::on_event(Listener l) { listeners_.push_back(std::move(l)); }

void QuestEngine::fire(const Event& ev) {
    for (auto& l : listeners_) l(ev);
}

const QuestRuntime* QuestEngine::runtime(const std::string& quest_id) const {
    auto it = runs_.find(quest_id);
    if (it == runs_.end()) return nullptr;
    return &it->second;
}

QuestRuntime* QuestEngine::mutable_runtime(const std::string& quest_id) {
    auto it = runs_.find(quest_id);
    if (it == runs_.end()) return nullptr;
    return &it->second;
}

const ParsedQuest* QuestEngine::definition(const std::string& quest_id) const {
    return cat_.find(quest_id);
}

bool QuestEngine::is_stage_active(const std::string& quest_id,
                                   const std::string& stage_id) const {
    auto* rt = runtime(quest_id);
    return rt && rt->phase == QuestPhase::Active && rt->current_stage == stage_id;
}

std::int64_t QuestEngine::bump_kill_counter(const std::string& quest_id,
                                             const std::string& entity_tag) {
    auto* rt = mutable_runtime(quest_id);
    if (!rt) return 0;
    auto& dedup = rt->killed[rt->current_stage];
    if (!dedup.insert(entity_tag).second) return rt->counters[rt->current_stage];
    return ++rt->counters[rt->current_stage];
}

void QuestEngine::mark_found(const std::string& quest_id,
                              const std::string& entity_id) {
    auto* rt = mutable_runtime(quest_id);
    if (!rt) return;
    rt->found[rt->current_stage].insert(entity_id);
}

void QuestEngine::mark_arrived(const std::string& quest_id) {
    auto* rt = mutable_runtime(quest_id);
    if (!rt) return;
    rt->found[rt->current_stage].insert("__arrived__");
}

// ---- Condition evaluation -------------------------------------------

bool QuestEngine::eval_condition_inner(const Condition& c) const {
    switch (c.op) {
        case CondOp::True:    return true;
        case CondOp::InCombat:return hooks_.in_combat && hooks_.in_combat();
        case CondOp::Weather: {
            auto w = hooks_.weather ? hooks_.weather() : std::string{};
            return !c.args.empty() && w == c.args[0];
        }
        case CondOp::TimeOfDay: {
            if (!hooks_.game_hour || c.args.empty()) return false;
            int h = hooks_.game_hour();
            bool daytime = (h >= 6 && h < 18);
            const auto& unit = c.args[0];
            if (unit == "hour")  return daytime;
            if (unit == "day")   return daytime;
            if (unit == "night") return !daytime;
            return false;
        }
        case CondOp::HasFlag: {
            if (c.args.empty()) return false;
            return flags_.has(c.args[0]);
        }
        case CondOp::NotFlag:
            return !c.args.empty() && !flags_.has(c.args[0]);
        case CondOp::FlagEquals: {
            if (c.args.size() < 2) return false;
            auto v = flags_.get(c.args[0]);
            if (!v) return false;
            auto& wanted = c.args[1];
            try {
                auto wn = std::stoll(wanted);
                if (auto* iv = std::get_if<std::int64_t>(&*v)) return *iv == wn;
            } catch (...) {}
            if (auto* b = std::get_if<bool>(&*v)) {
                return (wanted == "true" && *b) || (wanted == "false" && !*b);
            }
            if (auto* s = std::get_if<std::string>(&*v)) return *s == wanted;
            return false;
        }
        case CondOp::FlagAtLeast: {
            if (c.args.size() < 2) return false;
            auto iv = flags_.get_int(c.args[0]);
            if (!iv) return false;
            return *iv >= to_int(c.args[1]);
        }
        case CondOp::HasItem:
            return !c.args.empty() && hooks_.inventory_count &&
                   hooks_.inventory_count("player", c.args[0]) > 0;
        case CondOp::HasItemCount: {
            if (c.args.size() < 2 || !hooks_.inventory_count) return false;
            return hooks_.inventory_count("player", c.args[0]) >= to_int(c.args[1]);
        }
        case CondOp::RepAtLeast: {
            if (c.args.size() < 2 || !hooks_.reputation_get) return false;
            return hooks_.reputation_get(c.args[0]) >= to_int(c.args[1]);
        }
        case CondOp::DispositionAtLeast: {
            if (c.args.size() < 2 || !hooks_.disposition_get) return false;
            return hooks_.disposition_get(c.args[0]) >= to_int(c.args[1]);
        }
        case CondOp::SkillAtLeast: {
            if (c.args.size() < 2 || !hooks_.skill_get) return false;
            return hooks_.skill_get(c.args[0]) >= to_int(c.args[1]);
        }
        case CondOp::CompletedQuest:
            return !c.args.empty() && hooks_.is_quest_completed &&
                   hooks_.is_quest_completed(c.args[0]);
        case CondOp::ActiveQuest:
            return !c.args.empty() && hooks_.is_quest_active &&
                   hooks_.is_quest_active(c.args[0]);
        case CondOp::AtLocation: {
            if (c.args.size() < 3) return false;
            if (!hooks_.current_map || !hooks_.current_pos) return false;
            if (hooks_.current_map() != c.args[0]) return false;
            auto pos = hooks_.current_pos();
            return pos.first == to_int(c.args[1]) &&
                   pos.second == to_int(c.args[2]);
        }
        case CondOp::And:
            for (auto& ch : c.children) if (!eval_condition_inner(ch)) return false;
            return true;
        case CondOp::Or:
            for (auto& ch : c.children) if (eval_condition_inner(ch)) return true;
            return false;
        case CondOp::Not:
            return c.children.empty() || !eval_condition_inner(c.children.front());
    }
    return false;
}

bool QuestEngine::eval_condition(const Condition& c) const {
    return eval_condition_inner(c);
}

// ---- Actions --------------------------------------------------------

void QuestEngine::run_actions(const std::vector<Action>& acts) {
    for (auto& a : acts) {
        switch (a.kind) {
            case ActionKind::SetFlag: {
                if (a.args.size() >= 2) {
                    auto& key = a.args[0];
                    auto& val = a.args[1];
                    try {
                        auto n = std::stoll(val);
                        flags_.set_int(key, n);
                    } catch (...) {
                        if (val == "true")       flags_.set_bool(key, true);
                        else if (val == "false") flags_.set_bool(key, false);
                        else                     flags_.set_string(key, val);
                    }
                }
                break;
            }
            case ActionKind::IncFlag:
                if (!a.args.empty())
                    flags_.increment(a.args[0],
                                     a.args.size() > 1 ? to_int(a.args[1]) : 1);
                break;
            case ActionKind::ToggleFlag:
                if (!a.args.empty()) flags_.toggle(a.args[0]);
                break;
            case ActionKind::ClearFlag:
                if (!a.args.empty()) flags_.clear(a.args[0]);
                break;
            case ActionKind::GiveItem:
                if (a.args.size() >= 3 && hooks_.give_item)
                    hooks_.give_item(a.args[0], a.args[1], to_int(a.args[2]));
                break;
            case ActionKind::TakeItem:
                if (a.args.size() >= 3 && hooks_.take_item)
                    hooks_.take_item(a.args[0], a.args[1], to_int(a.args[2]));
                break;
            case ActionKind::GiveXp:
                if (a.args.size() >= 2 && hooks_.give_xp)
                    hooks_.give_xp(a.args[0], to_int(a.args[1]));
                break;
            case ActionKind::Disposition:
                if (a.args.size() >= 3 && hooks_.disposition_set)
                    hooks_.disposition_set(a.args[0], to_int(a.args[2]),
                                            a.args[1] == "+=");
                break;
            case ActionKind::Reputation:
                if (a.args.size() >= 3 && hooks_.reputation_set)
                    hooks_.reputation_set(a.args[0], to_int(a.args[2]),
                                           a.args[1] == "+=");
                break;
            case ActionKind::JournalAdd:
                if (a.args.size() >= 2) {
                    flags_.set_string("journal.category", a.args[0]);
                    flags_.set_string("journal.entry",    a.args[1]);
                }
                break;
            case ActionKind::SpawnNpc:
                if (a.args.size() >= 4 && hooks_.spawn_npc)
                    hooks_.spawn_npc(a.args[0], a.args[1],
                                     static_cast<int>(to_int(a.args[2])),
                                     static_cast<int>(to_int(a.args[3])));
                break;
            case ActionKind::DespawnNpc:
                if (!a.args.empty() && hooks_.despawn_npc)
                    hooks_.despawn_npc(a.args[0]);
                break;
            case ActionKind::SetStage: {
                if (a.args.size() >= 2) {
                    auto it = runs_.find(a.args[0]);
                    if (it != runs_.end()) {
                        it->second.current_stage = a.args[1];
                        it->second.entered = false;
                        fire({a.args[0], a.args[1], Event::StageEnter});
                    }
                }
                break;
            }
            case ActionKind::CompleteQuest:
                if (!a.args.empty()) complete(a.args[0]);
                break;
            case ActionKind::FailQuest:
                if (!a.args.empty()) fail(a.args[0]);
                break;
            case ActionKind::Teleport:
                if (a.args.size() >= 4 && hooks_.teleport)
                    hooks_.teleport(a.args[0], a.args[1],
                                    static_cast<int>(to_int(a.args[2])),
                                    static_cast<int>(to_int(a.args[3])));
                break;
        }
    }
}

// ---- Stage transitions ------------------------------------------------

void QuestEngine::enter_stage(QuestRuntime& rt, const Stage& st) {
    rt.entered = true;
    rt.visited.push_back(st.id);
    run_actions(st.on_enter);
    if (st.kind == StageKind::Timed && st.duration_secs > 0) {
        rt.timers[st.id] = static_cast<double>(st.duration_secs);
    }
}

void QuestEngine::exit_stage(QuestRuntime& /*rt*/, const Stage& st) {
    run_actions(st.on_exit);
}

bool QuestEngine::advance(const std::string& quest_id) {
    auto* def = cat_.find(quest_id);
    if (!def) return false;
    auto it = runs_.find(quest_id);
    if (it == runs_.end()) return false;
    auto& rt = it->second;
    if (rt.phase != QuestPhase::Active) return false;

    auto idx = def->find_stage(rt.current_stage);
    if (idx == static_cast<std::size_t>(-1)) return false;
    const Stage& st = def->def.stages[idx];

    if (!rt.entered) enter_stage(rt, st);

    bool done = false;
    switch (st.kind) {
        case StageKind::Objective:
        case StageKind::Fetch:
            done = eval_condition_inner(st.completion);
            break;

        case StageKind::Branch: {
            for (auto& b : st.branches) {
                if (eval_condition_inner(b.cond)) {
                    rt.current_stage = b.target_stage;
                    rt.entered = false;
                    fire({quest_id, rt.current_stage, Event::StageEnter});
                    return true;
                }
            }
            return false;
        }

        case StageKind::Simultaneous: {
            for (auto& sr : st.objective_all) {
                bool ok = std::visit([&](auto&& r) -> bool {
                    using T = std::decay_t<decltype(r)>;
                    if constexpr (std::is_same_v<T, ItemReq>) {
                        return hooks_.inventory_count &&
                               hooks_.inventory_count("player", r.item) >= r.count;
                    } else if constexpr (std::is_same_v<T, FlagReq>) {
                        return flags_.has(r.key);
                    } else if constexpr (std::is_same_v<T, ExamineReq>) {
                        return rt.found[st.id].count(r.entity_id) > 0;
                    } else if constexpr (std::is_same_v<T, TalkReq>) {
                        return rt.found[st.id].count(r.npc_id) > 0;
                    }
                    return false;
                }, sr);
                if (!ok) return false;
            }
            done = true;
            break;
        }

        case StageKind::Investigate: {
            for (auto& sr : st.objective_any) {
                bool ok = std::visit([&](auto&& r) -> bool {
                    using T = std::decay_t<decltype(r)>;
                    if constexpr (std::is_same_v<T, FlagReq>) {
                        return flags_.has(r.key);
                    } else if constexpr (std::is_same_v<T, ExamineReq>) {
                        return rt.found[st.id].count(r.entity_id) > 0;
                    } else if constexpr (std::is_same_v<T, TalkReq>) {
                        return rt.found[st.id].count(r.npc_id) > 0;
                    }
                    return false;
                }, sr);
                if (ok) { done = true; break; }
            }
            if (!done) return false;
            break;
        }

        case StageKind::Kill: {
            if (!st.objective_count) { done = true; break; }
            auto& req = *st.objective_count;
            auto cur = rt.counters[st.id];
            if (cur >= req.count) { done = true; }
            else return false;
            break;
        }

        case StageKind::Escort: {
            bool arrived = rt.found[st.id].count("__arrived__") > 0;
            if (arrived) { done = true; break; }
            if (st.completion.op != CondOp::True &&
                eval_condition_inner(st.completion)) { done = true; break; }
            return false;
        }

        case StageKind::Timed: {
            auto it2 = rt.timers.find(st.id);
            if (it2 != rt.timers.end() && it2->second <= 0.0) {
                run_actions(st.on_timeout);
                if (def->def.meta.auto_complete) {
                    done = true;
                } else {
                    exit_stage(rt, st);
                    fire({quest_id, st.id, Event::StageExit});
                    fail_with_message(quest_id, "timed out");
                    return true;
                }
            } else if (eval_condition_inner(st.completion)) {
                done = true;
            } else {
                return false;
            }
            break;
        }
    }

    if (!done) return false;

    exit_stage(rt, st);
    run_actions(st.on_complete);
    fire({quest_id, st.id, Event::StageComplete});

    if (st.next.empty()) {
        complete(quest_id);
        return true;
    }
    rt.current_stage = st.next;
    rt.entered = false;
    fire({quest_id, rt.current_stage, Event::StageEnter});
    return true;
}

void QuestEngine::fail_with_message(const std::string& quest_id,
                                     const std::string& msg) {
    auto it = runs_.find(quest_id);
    if (it == runs_.end()) return;
    auto* def = cat_.find(quest_id);
    if (def) {
        auto idx = def->find_stage(it->second.current_stage);
        if (idx != static_cast<std::size_t>(-1)) {
            run_actions(def->def.stages[idx].on_fail);
        }
    }
    fail(quest_id);
    (void)msg;
}

void QuestEngine::update(double dt) {
    for (auto& [qid, rt] : runs_) {
        if (rt.phase != QuestPhase::Active) continue;
        auto* def = cat_.find(qid);
        if (!def) continue;
        for (auto& [stage_id, remaining] : rt.timers) {
            if (remaining <= 0.0) continue;
            remaining -= dt;
            (void)stage_id;
        }
    }
}

}  // namespace quest
}  // namespace ash
