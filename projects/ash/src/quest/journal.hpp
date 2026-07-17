#pragma once
/// Phase 09: Quest journal (per Pillar 8).
///
/// The journal is auto-generated from quest templates. Each quest has
/// a meta + per-stage description and objective. The journal renders
/// three views: active / completed / all.
///
/// Entries can also be added on the fly by `journal add` actions. Those
/// get a category + text and live alongside quest-derived entries.

#include "quest/qst_engine.hpp"

#include <string>
#include <vector>

namespace ash {
namespace quest {

struct JournalEntry {
    std::string quest_id;
    std::string category;
    std::string title;
    std::string text;
    std::string current_objective;
    bool        unread = true;
    bool        completed = false;
    bool        failed = false;
};

enum class JournalView { Active, Completed, All };

class Journal {
public:
    Journal(const QuestEngine& eng, const QuestCatalog& cat);

    /// Rebuild entries from the current engine state. Call this whenever
    /// a quest changes phase or a stage advances.
    void rebuild();

    /// Add an ad-hoc entry (from `journal add` action).
    void add_entry(const std::string& category, const std::string& text);

    /// Mark all current entries as read.
    void mark_all_read();

    /// View filter.
    std::vector<JournalEntry> view(JournalView v) const;
    std::size_t               size(JournalView v) const;
    std::size_t               unread_count() const;

    /// Raw access for tests / debug.
    const std::vector<JournalEntry>& entries() const { return entries_; }

private:
    const QuestEngine&    eng_;
    const QuestCatalog&   cat_;
    std::vector<JournalEntry> entries_;
};

}  // namespace quest
}  // namespace ash
