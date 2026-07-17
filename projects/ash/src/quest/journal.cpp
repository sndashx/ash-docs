#include "quest/journal.hpp"

#include <algorithm>

namespace ash {
namespace quest {

Journal::Journal(const QuestEngine& eng, const QuestCatalog& cat)
    : eng_(eng), cat_(cat) {}

void Journal::rebuild() {
    entries_.clear();
    auto ids = cat_.ids();
    for (auto& qid : ids) {
        const auto* def = cat_.find(qid);
        if (!def) continue;
        const auto* rt = eng_.runtime(qid);
        JournalEntry e;
        e.quest_id  = qid;
        e.category  = def->def.meta.category;
        e.title     = def->def.meta.name.empty() ? qid : def->def.meta.name;

        bool active = rt && rt->phase == QuestPhase::Active;
        bool done   = rt && rt->phase == QuestPhase::Completed;
        bool failed = rt && rt->phase == QuestPhase::Failed;

        e.completed = done;
        e.failed    = failed;

        if (active && !rt->current_stage.empty()) {
            auto idx = def->find_stage(rt->current_stage);
            if (idx != static_cast<std::size_t>(-1)) {
                auto& st = def->def.stages[idx];
                e.text = st.description;
                e.current_objective = st.objective;
            }
        } else if (done) {
            e.text = "(Completed)";
        } else if (failed) {
            e.text = "(Failed)";
        } else {
            e.text = "(Not started)";
        }

        // active quests are always unread until viewed; rebuild does not
        // touch read state for entries that already existed.
        if (active) e.unread = true;

        entries_.push_back(std::move(e));
    }
    // Sort: active first (by priority desc, then sort_index asc), then
    // completed, then failed.
    std::sort(entries_.begin(), entries_.end(),
              [&](auto& a, auto& b) {
                  auto* da = cat_.find(a.quest_id);
                  auto* db = cat_.find(b.quest_id);
                  std::int64_t pa = da ? da->def.meta.priority  : 0;
                  std::int64_t pb = db ? db->def.meta.priority  : 0;
                  std::int64_t sa = da ? da->def.meta.sort_index : 0;
                  std::int64_t sb = db ? db->def.meta.sort_index : 0;
                  if (a.completed != b.completed) return !a.completed;
                  if (a.failed    != b.failed)    return !a.failed;
                  if (pa != pb) return pa > pb;
                  return sa < sb;
              });
}

void Journal::add_entry(const std::string& category, const std::string& text) {
    JournalEntry e;
    e.category = category;
    e.text     = text;
    e.title    = category;
    entries_.push_back(std::move(e));
}

void Journal::mark_all_read() {
    for (auto& e : entries_) e.unread = false;
}

std::vector<JournalEntry> Journal::view(JournalView v) const {
    std::vector<JournalEntry> out;
    out.reserve(entries_.size());
    for (auto& e : entries_) {
        switch (v) {
            case JournalView::Active:
                if (!e.completed && !e.failed) out.push_back(e);
                break;
            case JournalView::Completed:
                if (e.completed) out.push_back(e);
                break;
            case JournalView::All:
                out.push_back(e);
                break;
        }
    }
    return out;
}

std::size_t Journal::size(JournalView v) const {
    return view(v).size();
}

std::size_t Journal::unread_count() const {
    std::size_t n = 0;
    for (auto& e : entries_) if (e.unread) ++n;
    return n;
}

}  // namespace quest
}  // namespace ash