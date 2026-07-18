#pragma once
/// Phase 11: character sheet screen.
#include <array>
#include <string>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

using Attributes = std::array<int, 9>;
using Skills     = std::array<int, 24>;

struct DerivedStats {
    int hp_max{100};
    int vp_max{50};
    int sp_max{30};
    int crit_chance_pct{5};
    int carry_capacity{150};
    int speed_cells_per_sec{5};
    int barter_discount{0};
    int identify_skill{0};
    int pick_skill{0};
    int persuade_skill{0};
    bool encumbered{false};
};

class CharacterScreen : public Screen {
public:
    CharacterScreen() = default;
    CharacterScreen(std::string player_name,
                    std::string class_hint,
                    Attributes attrs,
                    Skills skills,
                    DerivedStats derived);

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return "character"; }

private:
    std::string  player_name_{"Player"};
    std::string  class_hint_{"Adventurer"};
    Attributes   attrs_{};
    Skills       skills_{};
    DerivedStats derived_{};
};

}  // namespace ui
}  // namespace ash