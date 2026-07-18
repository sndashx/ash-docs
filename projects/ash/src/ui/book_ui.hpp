#pragma once
/// Phase 11: multi-page book viewer.
#include <string>
#include <vector>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

class BookScreen : public Screen {
public:
    BookScreen() = default;
    explicit BookScreen(std::vector<std::string> pages, std::string title_text = "Book");

    void set_pages(std::vector<std::string> pages);
    void set_title(std::string title_text);

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return title_; }

private:
    std::vector<std::string> pages_;
    std::string              title_{"book"};
    int                      page_{0};
};

}  // namespace ui
}  // namespace ash