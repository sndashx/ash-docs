#pragma once
/// Phase 11: region overview / world map screen.
#include <string>
#include <vector>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

struct Region {
    std::string id;
    std::string name;
    bool        discovered{false};
};

class MapScreen : public Screen {
public:
    MapScreen() = default;
    explicit MapScreen(std::vector<Region> regions);

    void set_regions(std::vector<Region> regions);

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return "map"; }

private:
    std::vector<Region> regions_;
    int                 selected_{0};
    int                 scroll_{0};
};

}  // namespace ui
}  // namespace ash