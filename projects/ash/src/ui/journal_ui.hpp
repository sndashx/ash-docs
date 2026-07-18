#pragma once
/// Phase 11: quest journal (active / completed / all tabs).
#include <string>
#include <vector>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

struct JournalEntry {
    std::string quest_id;
    std::string title;
    std::string text;
    bool        completed{false};
};

class JournalScreen : public Screen {
public:
    JournalScreen() = default;
    explicit JournalScreen(std::vector<JournalEntry> entries);

    void set_entries(std::vector<JournalEntry> entries);

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return "journal"; }

private:
    std::vector<JournalEntry> entries_;
    int  tab_{0};        /// 0 active, 1 completed, 2 all
    int  selected_{0};
    int  scroll_{0};
    int  anim_t_ms_{0};

    void update(int dt_ms) override { anim_t_ms_ += dt_ms; }

    std::vector<JournalEntry const*> filtered() const;
};

}  // namespace ui
}  // namespace ash