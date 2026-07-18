#pragma once
/// Phase 11: trading / barter screen.
#include <string>
#include <vector>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

struct TradeItem {
    std::string name;
    int         value{0};
    bool        selected{false};
};

class BarterScreen : public Screen {
public:
    BarterScreen() = default;
    BarterScreen(std::vector<TradeItem> player_items,
                 std::vector<TradeItem> merchant_items);

    void set_player(std::vector<TradeItem> items);
    void set_merchant(std::vector<TradeItem> items);

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return "barter"; }

private:
    std::vector<TradeItem> player_;
    std::vector<TradeItem> merchant_;
    int  active_panel_{0};
    int  player_cursor_{0};
    int  merchant_cursor_{0};

    int totals(int& player_total, int& merchant_total) const;
};

}  // namespace ui
}  // namespace ash