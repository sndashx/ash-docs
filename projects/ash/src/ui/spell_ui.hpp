#pragma once
/// Phase 11: spell list screen.
#include <string>
#include <vector>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

struct SpellEntry {
    std::string school;
    std::string name;
    int         cost_sp{0};
};

class SpellScreen : public Screen {
public:
    SpellScreen() = default;
    explicit SpellScreen(std::vector<SpellEntry> spells);

    void set_spells(std::vector<SpellEntry> spells);

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return "spells"; }

private:
    std::vector<SpellEntry> spells_;
    int                     selected_{0};
    int                     scroll_{0};
};

}  // namespace ui
}  // namespace ash