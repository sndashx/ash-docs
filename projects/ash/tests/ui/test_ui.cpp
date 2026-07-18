/// Phase 11: UI mode stack + screen navigation + accessibility (per 16-phase-11).
#include "test_harness.hpp"

#include <filesystem>
#include <string>
#include <unistd.h>

#include "render/buffer.hpp"
#include "render/buffer_emit.hpp"
#include "render/cell.hpp"
#include "settings/accessibility.hpp"
#include "settings/settings.hpp"
#include "ui/character_ui.hpp"
#include "ui/dialogue_ui.hpp"
#include "ui/hud.hpp"
#include "ui/input_event.hpp"
#include "ui/inventory_ui.hpp"
#include "ui/journal_ui.hpp"
#include "ui/map_ui.hpp"
#include "ui/menu.hpp"
#include "ui/mode_stack.hpp"
#include "ui/screen.hpp"
#include "ui/screen_context.hpp"
#include "ui/spell_ui.hpp"

namespace fs = std::filesystem;

namespace {

using ash::ui::ScreenContext;
using ash::ui::Mode;
using ash::ui::ModeStack;
using ash::ui::Screen;
using ash::ui::ScreenResult;
using ash::ui::KeyEvent;
using ash::ui::Key;
using ash::ui::Rect;
using ash::ui::MainMenu;
using ash::ui::MenuAction;
using ash::ui::InventoryScreen;
using ash::ui::InventoryItem;
using ash::ui::DialogueScreen;
using ash::ui::DialogueTopic;
using ash::ui::JournalScreen;
using ash::ui::JournalEntry;
using ash::ui::CharacterScreen;
using ash::ui::DerivedStats;
using ash::ui::MapScreen;
using ash::ui::Region;
using ash::ui::SpellScreen;
using ash::ui::SpellEntry;
using ash::ui::HudOverlay;
using ash::ui::word_wrap;
using ash::ui::draw_list;
using ash::ui::ListItem;
using ash::ui::Theme;
using ash::render::Buffer;

void make_ctx(ScreenContext& ctx) {
    ctx.viewport_w = 100;
    ctx.viewport_h = 32;
    ctx.world.hp_max = 100;
    ctx.world.vp_max = 50;
    ctx.world.sp_max = 30;
    ctx.world.hp = 80;
    ctx.world.vp = 40;
    ctx.world.sp = 25;
    ctx.refresh_accessibility();
}

}  // namespace

TEST_CASE("ui: mode stack push / pop cycles", "[ui][mode]") {
    ScreenContext ctx;
    make_ctx(ctx);
    ModeStack stack;

    struct DummyScreen : public Screen {
        void render(ash::render::Buffer&, Rect) override {}
        std::string title() const override { return "dummy"; }
    };

    REQUIRE(stack.empty());
    stack.push(std::make_unique<DummyScreen>(), Mode::Menu, ctx);
    REQUIRE(!stack.empty());
    REQUIRE(stack.top_mode() == Mode::Menu);
    REQUIRE(stack.top_title() == "dummy");

    stack.push(std::make_unique<DummyScreen>(), Mode::Game, ctx);
    REQUIRE(stack.size() == 2);
    REQUIRE(stack.top_mode() == Mode::Game);

    stack.pop(ctx);
    REQUIRE(stack.size() == 1);
    REQUIRE(stack.top_mode() == Mode::Menu);

    stack.pop(ctx);
    REQUIRE(stack.empty());
    REQUIRE(stack.pop(ctx) == false);
}

TEST_CASE("ui: pop_to drops back to a named mode", "[ui][mode]") {
    ScreenContext ctx;
    make_ctx(ctx);
    ModeStack stack;
    struct DummyScreen : public Screen {
        void render(ash::render::Buffer&, Rect) override {}
        std::string title() const override { return "x"; }
    };
    stack.push(std::make_unique<DummyScreen>(), Mode::Menu, ctx);
    stack.push(std::make_unique<DummyScreen>(), Mode::Inventory, ctx);
    stack.push(std::make_unique<DummyScreen>(), Mode::Dialogue, ctx);
    stack.pop_to(Mode::Inventory, ctx);
    REQUIRE(stack.size() == 2);
    REQUIRE(stack.top_mode() == Mode::Inventory);
}

TEST_CASE("ui: dispatch_key routes only to top screen", "[ui][mode]") {
    ScreenContext ctx;
    make_ctx(ctx);
    ModeStack stack;

    int bottom_called = 0;
    int top_called    = 0;
    struct Bottom : public Screen {
        int* n;
        explicit Bottom(int* p) : n(p) {}
        void render(ash::render::Buffer&, Rect) override {}
        ScreenResult handle_key(KeyEvent const&, ScreenContext&) override {
            ++(*n);
            return ScreenResult::Stay;
        }
        std::string title() const override { return "bot"; }
    };
    struct Top : public Screen {
        int* n;
        explicit Top(int* p) : n(p) {}
        void render(ash::render::Buffer&, Rect) override {}
        ScreenResult handle_key(KeyEvent const&, ScreenContext&) override {
            ++(*n);
            return ScreenResult::Stay;
        }
        std::string title() const override { return "top"; }
    };

    stack.push(std::make_unique<Bottom>(&bottom_called), Mode::Game, ctx);
    stack.push(std::make_unique<Top>(&top_called),    Mode::Menu, ctx);

    KeyEvent ev;
    ev.key = Key::Enter;
    stack.dispatch_key(ev, ctx);
    stack.dispatch_key(ev, ctx);
    stack.dispatch_key(ev, ctx);

    REQUIRE(top_called == 3);
    REQUIRE(bottom_called == 0);
}

TEST_CASE("ui: dispatch_key pop disconnects screen", "[ui][mode]") {
    ScreenContext ctx;
    make_ctx(ctx);
    ModeStack stack;

    int popped = 0;
    struct TopOnce : public Screen {
        int* n;
        explicit TopOnce(int* p) : n(p) {}
        void render(ash::render::Buffer&, Rect) override {}
        ScreenResult handle_key(KeyEvent const&, ScreenContext&) override {
            ++(*n);
            return ScreenResult::Pop;
        }
        std::string title() const override { return "once"; }
    };

    stack.push(std::make_unique<TopOnce>(&popped), Mode::Menu, ctx);
    REQUIRE(stack.size() == 1);
    KeyEvent ev;
    ev.key = Key::Enter;
    stack.dispatch_key(ev, ctx);
    REQUIRE(popped == 1);
    REQUIRE(stack.empty());
}

TEST_CASE("ui: main menu navigates and selects", "[ui][menu]") {
    ScreenContext ctx;
    make_ctx(ctx);
    MainMenu menu([](MenuAction) {});
    ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};

    menu.render(buf, vp);
    KeyEvent down; down.key = Key::Down;
    menu.handle_key(down, ctx);
    menu.handle_key(down, ctx);
    KeyEvent enter; enter.key = Key::Enter;
    auto r = menu.handle_key(enter, ctx);
    REQUIRE(r == ScreenResult::Pop);
}

TEST_CASE("ui: hotkey 'q' on main menu requests quit", "[ui][menu]") {
    ScreenContext ctx;
    make_ctx(ctx);
    MainMenu menu([](MenuAction) {});
    KeyEvent q; q.ch = 'q';
    auto r = menu.handle_key(q, ctx);
    REQUIRE(r == ScreenResult::Quit);
    REQUIRE(ctx.quit_requested == true);
}

TEST_CASE("ui: inventory handles up/down/left/right", "[ui][inventory]") {
    ScreenContext ctx;
    make_ctx(ctx);
    std::vector<InventoryItem> items;
    items.push_back({"Sword", 1, false});
    items.push_back({"Bow",   1, false});
    items.push_back({"Potion",3, false});
    items.push_back({"Steak", 1, false});
    InventoryScreen inv;
    inv.set_items(std::move(items));

    ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
    inv.render(buf, vp);

    KeyEvent r; r.key = Key::Right;
    inv.handle_key(r, ctx);
    KeyEvent d; d.key = Key::Down;
    inv.handle_key(d, ctx);
    KeyEvent esc; esc.key = Key::Esc;
    auto result = inv.handle_key(esc, ctx);
    REQUIRE(result == ScreenResult::Pop);
}

TEST_CASE("ui: dialogue renders topics and pops on enter", "[ui][dialogue]") {
    ScreenContext ctx;
    make_ctx(ctx);
    std::vector<DialogueTopic> topics;
    topics.push_back({"Hello",       nullptr});
    topics.push_back({"Goodbye",     nullptr});
    DialogueScreen dlg("NPC", "Hello, traveler.", topics);

    ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
    dlg.render(buf, vp);

    KeyEvent d; d.key = Key::Down;
    dlg.handle_key(d, ctx);
    KeyEvent enter; enter.key = Key::Enter;
    auto r = dlg.handle_key(enter, ctx);
    REQUIRE(r == ScreenResult::Pop);
}

TEST_CASE("ui: journal tab + selection", "[ui][journal]") {
    ScreenContext ctx;
    make_ctx(ctx);
    std::vector<JournalEntry> entries;
    entries.push_back({"q1", "Title one", "Body one", false});
    entries.push_back({"q2", "Title two", "Body two", true});
    JournalScreen j;
    j.set_entries(std::move(entries));

    ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
    j.render(buf, vp);

    KeyEvent k1; k1.ch = '1';   // active tab
    j.handle_key(k1, ctx);
    KeyEvent k2; k2.ch = '2';   // completed tab
    j.handle_key(k2, ctx);
    KeyEvent esc; esc.key = Key::Esc;
    auto r = j.handle_key(esc, ctx);
    REQUIRE(r == ScreenResult::Pop);
}

TEST_CASE("ui: character sheet renders attrs", "[ui][character]") {
    ScreenContext ctx;
    make_ctx(ctx);
    std::array<int, 9>  attrs{};
    std::array<int, 24> skills{};
    for (auto& a : attrs)  a = 40;
    for (auto& s : skills) s = 5;
    DerivedStats ds;
    CharacterScreen ch("Hero", "Wanderer", attrs, skills, ds);

    ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
    ch.render(buf, vp);

    KeyEvent esc; esc.key = Key::Esc;
    REQUIRE(ch.handle_key(esc, ctx) == ScreenResult::Pop);
}

TEST_CASE("ui: map screen renders without crashing", "[ui][map]") {
    ScreenContext ctx;
    make_ctx(ctx);
    std::vector<Region> regions;
    regions.push_back({"coast", "Scoured Coast", true});
    regions.push_back({"spire", "Riven Spire",   false});
    MapScreen ms;
    ms.set_regions(std::move(regions));

    ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
    ms.render(buf, vp);
    REQUIRE(ms.title() == "map");
}

TEST_CASE("ui: spell screen list + pop", "[ui][spell]") {
    ScreenContext ctx;
    make_ctx(ctx);
    std::vector<SpellEntry> spells;
    spells.push_back({"Light", "Mysticism",   4});
    spells.push_back({"Spark", "Destruction", 5});
    SpellScreen sp;
    sp.set_spells(std::move(spells));

    ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
    sp.render(buf, vp);
    KeyEvent esc; esc.key = Key::Esc;
    REQUIRE(sp.handle_key(esc, ctx) == ScreenResult::Pop);
}

TEST_CASE("ui: hud overlay renders bars and exits", "[ui][hud]") {
    ScreenContext ctx;
    make_ctx(ctx);
    ctx.world.location_name = "Test";
    ctx.world.time_of_day_minutes = 8 * 60;

    HudOverlay overlay;
    ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
    overlay.render(buf, vp, ctx);
}

TEST_CASE("ui: settings colorblind variant changes palette", "[ui][accessibility]") {
    ash::settings::Settings s;
    s.palette_variant = "deuteranopia";
    ScreenContext ctx;
    ctx.settings = s;
    ctx.refresh_accessibility();
    REQUIRE(ctx.render_acc.palette_swap == true);

    s.palette_variant = "default";
    ctx.settings = s;
    ctx.refresh_accessibility();
    REQUIRE(ctx.render_acc.palette_swap == false);

    s.no_color = true;
    ctx.settings = s;
    ctx.refresh_accessibility();
    REQUIRE(ctx.render_acc.use_color == false);

    s.high_contrast = true;
    ctx.settings = s;
    ctx.refresh_accessibility();
    REQUIRE(ctx.render_acc.high_contrast == true);
}

TEST_CASE("ui: word_wrap splits long text", "[ui][widgets]") {
    auto lines = word_wrap("the quick brown fox jumps over the lazy dog", 10);
    REQUIRE(lines.size() >= 4);
    for (auto const& l : lines) {
        REQUIRE(l.size() <= 10);
    }
}

TEST_CASE("ui: render output differs across palette variants", "[ui][accessibility]") {
    auto render_with = [](std::string variant, bool no_color, bool hc) {
        ScreenContext ctx;
        ctx.viewport_w = 60;
        ctx.viewport_h = 20;
        ctx.settings.palette_variant = variant;
        ctx.settings.no_color = no_color;
        ctx.settings.high_contrast = hc;
        ctx.refresh_accessibility();

        HudOverlay overlay;
        ash::render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                                  static_cast<std::uint16_t>(ctx.viewport_h));
        buf.clear();
        overlay.render(buf, Rect{0, 0, ctx.viewport_w, ctx.viewport_h}, ctx);
        std::string out;
        std::vector<ash::render::DirtyRect> dirty;
        ash::render::DirtyRect d; d.x = 0; d.y = 0;
        d.w = buf.width; d.h = buf.height;
        dirty.push_back(d);
        ash::render::emit(buf, out, dirty);
        return out;
    };

    auto baseline = render_with("default", false, false);
    auto deutan   = render_with("deuteranopia", false, false);
    auto protan   = render_with("protanopia", false, false);
    auto tritan   = render_with("tritanopia", false, false);
    auto no_color = render_with("default", true, false);
    auto highcon  = render_with("default", false, true);

    // Different palette variants should produce different ANSI escapes.
    REQUIRE(baseline != deutan);
    REQUIRE(baseline != protan);
    REQUIRE(baseline != tritan);
    REQUIRE(baseline != no_color);
    REQUIRE(baseline != highcon);
    REQUIRE(no_color != highcon);
}

TEST_CASE("ui: draw_list highlights selected row", "[ui][widgets]") {
    ash::render::Buffer buf(static_cast<std::uint16_t>(40),
                       static_cast<std::uint16_t>(10));
    buf.clear();
    std::vector<ListItem> items;
    items.push_back({"Apple",  "", true, nullptr});
    items.push_back({"Banana", "", true, nullptr});
    items.push_back({"Cherry", "", true, nullptr});
    Theme th;
    Rect r{0, 0, 20, 5};
    Rect sel = draw_list(buf, r, items, 1, 0, th);
    REQUIRE(sel.contains(0, 1));
}
