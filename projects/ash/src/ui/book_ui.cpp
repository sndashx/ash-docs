#include "ui/book_ui.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>

#include "ui/widgets.hpp"

namespace ash {
namespace ui {

namespace {

void put(render::Buffer& buf, int x, int y, render::Cell c) {
    if (x >= 0 && y >= 0 && x < static_cast<int>(buf.width) && y < static_cast<int>(buf.height)) {
        buf.set(x, y, c);
    }
}

render::Cell mk(uint32_t g, render::Color fg, render::Color bg) {
    render::Cell c;
    c.glyph = g;
    c.fg_r = fg.r; c.fg_g = fg.g; c.fg_b = fg.b;
    c.bg_r = bg.r; c.bg_g = bg.g; c.bg_b = bg.b;
    return c;
}

void draw_string(render::Buffer& buf, int x, int y, std::string const& s,
                 render::Color fg, render::Color bg) {
    for (std::size_t i = 0; i < s.size(); ++i) {
        put(buf, x + static_cast<int>(i), y,
            mk(static_cast<uint32_t>(static_cast<unsigned char>(s[i])), fg, bg));
    }
}

void fill_rect(render::Buffer& buf, Rect r, render::Color bg) {
    for (int y = r.y; y < r.y + r.h; ++y) {
        for (int x = r.x; x < r.x + r.w; ++x) {
            put(buf, x, y, mk(uint32_t(' '), bg, bg));
        }
    }
}

}  // namespace

BookScreen::BookScreen(std::vector<std::string> pages, std::string title_text)
    : pages_(std::move(pages)), title_(std::move(title_text)) {
    if (pages_.empty()) pages_.push_back("");
}

void BookScreen::set_pages(std::vector<std::string> pages) {
    pages_ = std::move(pages);
    if (pages_.empty()) pages_.push_back("");
    if (page_ >= static_cast<int>(pages_.size())) page_ = static_cast<int>(pages_.size()) - 1;
    if (page_ < 0) page_ = 0;
}

void BookScreen::set_title(std::string title_text) {
    title_ = std::move(title_text);
}

void BookScreen::render(render::Buffer& buf, Rect viewport) {
    Theme t = Theme::derive(/*high_contrast=*/false,
                            /*no_color=*/false,
                            /*palette_variant=*/"");

    int panel_w = std::min<int>(viewport.w - 2, 70);
    int panel_h = std::min<int>(viewport.h - 2, 28);
    int px = viewport.x + (viewport.w - panel_w) / 2;
    int py = viewport.y + (viewport.h - panel_h) / 2;
    Rect panel{px, py, panel_w, panel_h};
    fill_rect(buf, panel, t.panel_bg);
    draw_border(buf, panel, t);
    Rect title{panel.x, panel.y, panel.w, 1};
    draw_title(buf, title, title_, t);

    Rect body{panel.x + 2, panel.y + 2, panel.w - 4, panel.h - 4};
    if (body.h > 0 && body.w > 0) {
        if (page_ >= 0 && page_ < static_cast<int>(pages_.size())) {
            draw_text(buf, body, pages_[static_cast<std::size_t>(page_)], t);
        }
    }

    char foot[96];
    int total = static_cast<int>(pages_.size());
    std::snprintf(foot, sizeof(foot),
                  "Page %d of %d    [<- ->] flip   [Esc] close",
                  page_ + 1, total);
    int fx = panel.x + (panel.w - static_cast<int>(std::strlen(foot))) / 2;
    draw_string(buf, fx, panel.y + panel.h - 1, foot, t.disabled_fg, t.panel_bg);
}

ScreenResult BookScreen::handle_key(KeyEvent const& ev, ScreenContext& /*ctx*/) {
    if (ev.key == Key::Esc) return ScreenResult::Pop;
    int total = static_cast<int>(pages_.size());
    if (ev.key == Key::Right || ev.key == Key::PageDown || ev.key == Key::Space) {
        if (page_ + 1 < total) ++page_;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Left || ev.key == Key::PageUp) {
        if (page_ > 0) --page_;
        return ScreenResult::Stay;
    }
    if (ev.key == Key::Home) { page_ = 0; return ScreenResult::Stay; }
    if (ev.key == Key::End)  { page_ = total - 1; return ScreenResult::Stay; }
    if (ev.key == Key::Enter) return ScreenResult::Pop;
    return ScreenResult::Stay;
}

}  // namespace ui
}  // namespace ash