#include "ui/mode_stack.hpp"

namespace ash {
namespace ui {

std::string const& mode_name(Mode m) noexcept {
    static std::string const names[] = {
        "none", "menu", "game", "inventory", "map", "journal",
        "character", "dialogue", "spells", "book", "barter",
        "settings", "save", "load", "death",
    };
    auto idx = static_cast<std::size_t>(m);
    if (idx >= sizeof(names) / sizeof(names[0])) {
        static std::string const unknown = "unknown";
        return unknown;
    }
    return names[idx];
}

void ModeStack::push(std::unique_ptr<Screen> s, Mode mode, ScreenContext& ctx) {
    if (!s) return;
    s->on_push(ctx);
    stack_.push_back(Entry{mode, std::move(s)});
}

bool ModeStack::pop(ScreenContext& ctx) {
    if (stack_.empty()) return false;
    apply_pop(ctx);
    return true;
}

void ModeStack::apply_pop(ScreenContext& ctx) {
    (void)ctx;
    auto& top = stack_.back();
    if (top.screen) top.screen->on_pop();
    stack_.pop_back();
}

void ModeStack::pop_to(Mode mode, ScreenContext& ctx) {
    while (!stack_.empty() && stack_.back().mode != mode) {
        apply_pop(ctx);
    }
}

void ModeStack::reset(std::unique_ptr<Screen> s, Mode mode, ScreenContext& ctx) {
    while (!stack_.empty()) apply_pop(ctx);
    push(std::move(s), mode, ctx);
}

Mode ModeStack::top_mode() const noexcept {
    if (stack_.empty()) return Mode::None;
    return stack_.back().mode;
}

std::string ModeStack::top_title() const noexcept {
    if (stack_.empty()) return {};
    return stack_.back().screen ? stack_.back().screen->title() : std::string{};
}

void ModeStack::update(int dt_ms) {
    // Top-down: child most recent first.
    for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
        if (it->screen) it->screen->update(dt_ms);
    }
}

void ModeStack::render(render::Buffer& buf, Rect viewport) {
    // Only the topmost screen draws in full. Underlying screens draw a
    // dimmed background first so popping the top is visually clean.
    if (stack_.empty()) return;
    for (std::size_t i = 0; i + 1 < stack_.size(); ++i) {
        if (stack_[i].screen) stack_[i].screen->render(buf, viewport);
    }
    if (stack_.back().screen) stack_.back().screen->render(buf, viewport);
}

ScreenResult ModeStack::dispatch_key(KeyEvent const& ev, ScreenContext& ctx) {
    if (stack_.empty()) return ScreenResult::Stay;
    auto& top = stack_.back();
    if (!top.screen) return ScreenResult::Stay;
    ScreenResult r = top.screen->handle_key(ev, ctx);
    if (r == ScreenResult::Pop) {
        apply_pop(ctx);
        return ScreenResult::Stay;
    }
    if (r == ScreenResult::Quit) {
        ctx.quit_requested = true;
        return ScreenResult::Quit;
    }
    return r;
}

ScreenResult ModeStack::dispatch_mouse(MouseEvent const& ev, ScreenContext& ctx) {
    if (stack_.empty()) return ScreenResult::Stay;
    auto& top = stack_.back();
    if (!top.screen) return ScreenResult::Stay;
    return top.screen->handle_mouse(ev, ctx);
}

std::vector<std::pair<Mode, std::string>> ModeStack::entries() const {
    std::vector<std::pair<Mode, std::string>> out;
    out.reserve(stack_.size());
    for (auto const& e : stack_) {
        out.emplace_back(e.mode, e.screen ? e.screen->title() : std::string{});
    }
    return out;
}

}  // namespace ui
}  // namespace ash