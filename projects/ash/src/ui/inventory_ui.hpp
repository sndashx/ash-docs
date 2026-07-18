#pragma once
/// Phase 11: inventory screen with 3 categorized columns.
#include <cstdint>
#include <string>
#include <vector>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

enum class InventoryCategory : std::uint8_t {
    Weapon   = 0,
    Apparel  = 1,
    Misc     = 2,
    Count    = 3,
};

struct InventoryItem {
    std::string label;
    int         count{1};
    bool        equipped{false};
    InventoryCategory category{InventoryCategory::Misc};
};

class InventoryScreen : public Screen {
public:
    InventoryScreen() = default;
    explicit InventoryScreen(std::vector<InventoryItem> items);

    void set_items(std::vector<InventoryItem> items);

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return "inventory"; }

private:
    std::vector<InventoryItem> items_;
    int                        active_col_{0};
    int                        col_cursor_[3]{0, 0, 0};

    int col_size(int col) const;
    int col_offset(int col) const;
    void normalize_cursors();
};

}  // namespace ui
}  // namespace ash