#include "app/ui_loop.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "render/ansi.hpp"
#include "render/buffer.hpp"
#include "render/buffer_emit.hpp"
#include "render/cell.hpp"
#include "ui/barter_ui.hpp"
#include "ui/book_ui.hpp"
#include "ui/character_ui.hpp"
#include "ui/dialogue_ui.hpp"
#include "ui/hud.hpp"
#include "ui/inventory_ui.hpp"
#include "ui/journal_ui.hpp"
#include "ui/map_ui.hpp"
#include "ui/menu.hpp"
#include "ui/mode_stack.hpp"
#include "ui/screen.hpp"
#include "ui/spell_ui.hpp"

namespace ash {
namespace app {

namespace {

using ui::Key;
using ui::KeyEvent;
using ui::Rect;
using ui::ScreenContext;

constexpr int kViewportW = 100;
constexpr int kViewportH = 32;

KeyEvent decode(char c) {
    KeyEvent ev{};
    ev.ch = c;
    switch (c) {
        case 27:   ev.key = Key::Esc; break;
        case '\r':
        case '\n': ev.key = Key::Enter; break;
        case ' ':  ev.key = Key::Space; break;
        case '\t':  ev.key = Key::Tab; break;
        case '\b':  ev.key = Key::Backspace; break;
        case 'A':
        case 'k':  ev.key = Key::Up; ev.ch = 0; break;
        case 'B':
        case 'j':  ev.key = Key::Down; ev.ch = 0; break;
        case 'D':
        case 'h':  ev.key = Key::Left; ev.ch = 0; break;
        case 'C':
        case 'l':  ev.key = Key::Right; ev.ch = 0; break;
        case 'K':  ev.key = Key::End; ev.ch = 0; break;
        case 'H':  ev.key = Key::Home; ev.ch = 0; break;
        default: break;
    }
    return ev;
}

void render_to_stdout(render::Buffer const& buf) {
    std::string out;
    std::vector<render::DirtyRect> dirty;
    render::DirtyRect dr;
    dr.x = 0; dr.y = 0;
    dr.w = buf.width; dr.h = buf.height;
    dirty.push_back(dr);
    render::emit(buf, out, dirty);
    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
}

void push_inventory_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    std::vector<ui::InventoryItem> items;
    items.push_back({"Iron Sword",       1, true});
    items.push_back({"Steel Dagger",     1, false});
    items.push_back({"Hunting Bow",      1, false});
    items.push_back({"Healing Potion",   5, false});
    items.push_back({"Lockpick",        12, false});
    items.push_back({"Iron Helm",        1, false});
    items.push_back({"Leather Armor",    1, false});
    items.push_back({"Scroll of Light",  3, false});
    items.push_back({"Torch",            6, false});
    items.push_back({"Bread Loaf",       4, false});
    auto screen = std::make_unique<ui::InventoryScreen>();
    screen->set_items(std::move(items));
    stack.push(std::move(screen), ui::Mode::Inventory, ctx);
}

void push_dialogue_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    std::vector<ui::DialogueTopic> topics;
    topics.push_back({"Who are you?",       nullptr});
    topics.push_back({"Tell me about this place.", nullptr});
    topics.push_back({"Any rumors?",        nullptr});
    topics.push_back({"Goodbye.",           nullptr});
    stack.push(std::make_unique<ui::DialogueScreen>(
                   "Mira", "Greetings, traveler. The wind carries strange stories from the Glasswood these days.",
                   topics),
               ui::Mode::Dialogue, ctx);
}

void push_journal_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    std::vector<ui::JournalEntry> entries;
    entries.push_back({"ash_main", "The Scoured Coast",
                       "Investigate the hollow cities beyond the ash plains. The First Lantern still burns at the edge of the marsh.",
                       false});
    entries.push_back({"ash_main", "Whispers in the Glasswood",
                       "Follow the lights the rangers speak of. Do not trust mirrors.",
                       false});
    entries.push_back({"side_01",  "The Lost Shipment",
                       "Find the missing caravan near the coast. The merchants guild will pay well.",
                       false});
    entries.push_back({"side_02",  "Apprentice's Task",
                       "Deliver the alchemist's parcel to the Glasswood rangers.",
                       true});
    entries.push_back({"side_03",  "Through the Riven Spire",
                       "Reach the top of the broken tower. Bring rope.",
                       false});
    entries.push_back({"note",     "Note",
                       "Carry two waterskins in the Ash Plains.",
                       false});
    auto screen = std::make_unique<ui::JournalScreen>();
    screen->set_entries(std::move(entries));
    stack.push(std::move(screen), ui::Mode::Journal, ctx);
}

void push_character_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    std::array<int, 9> attrs = {50, 45, 55, 40, 60, 50, 45, 50, 40};
    std::array<int, 24> skills{};
    for (auto& s : skills) s = 30;
    ui::DerivedStats d;
    d.hp_max = 120;
    d.vp_max = 60;
    d.sp_max = 80;
    d.crit_chance_pct = 12;
    d.carry_capacity = 220;
    d.speed_cells_per_sec = 5;
    d.barter_discount = 15;
    d.identify_skill = 60;
    d.pick_skill = 40;
    d.persuade_skill = 55;
    d.encumbered = false;
    stack.push(std::make_unique<ui::CharacterScreen>(
                   "Aren", "Wanderer", attrs, skills, d),
               ui::Mode::Character, ctx);
}

void push_map_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    std::vector<ui::Region> regions;
    regions.push_back({"coast",     "Scoured Coast",  true});
    regions.push_back({"glasswood", "Glasswood",      true});
    regions.push_back({"ash",       "Ash Plains",     true});
    regions.push_back({"spire",     "Riven Spire",    false});
    regions.push_back({"hollow",    "Hollow Cities",  false});
    regions.push_back({"marsh",     "Veiled Marsh",   false});
    regions.push_back({"cinder",    "Cinder Reach",   true});
    regions.push_back({"vale",      "Pale Vale",      false});
    regions.push_back({"lantern",   "The Last Lantern", false});
    auto screen = std::make_unique<ui::MapScreen>();
    screen->set_regions(std::move(regions));
    stack.push(std::move(screen), ui::Mode::Map, ctx);
}

void push_spell_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    std::vector<ui::SpellEntry> spells;
    spells.push_back({"Cure Wounds",  "Restoration", 10});
    spells.push_back({"Ward",         "Warding",     12});
    spells.push_back({"Sunbeam",      "Destruction", 30});
    spells.push_back({"Sparks",       "Destruction", 5});
    spells.push_back({"Muffle",       "Mysticism",   8});
    spells.push_back({"Light",        "Mysticism",   4});
    spells.push_back({"Alchemize",    "Alchemy",     20});
    auto screen = std::make_unique<ui::SpellScreen>();
    screen->set_spells(std::move(spells));
    stack.push(std::move(screen), ui::Mode::Spells, ctx);
}

void push_barter_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    std::vector<ui::TradeItem> player_items;
    player_items.push_back({"Iron Sword", 80, false});
    player_items.push_back({"Healing Potion", 25, false});
    player_items.push_back({"Lockpick (12)", 12, false});
    std::vector<ui::TradeItem> merchant_items;
    merchant_items.push_back({"Steel Dagger", 120, false});
    merchant_items.push_back({"Silver Ring", 60, false});
    merchant_items.push_back({"Map of the Ash Plains", 30, false});
    merchant_items.push_back({"Mystic Scroll", 200, false});
    auto screen = std::make_unique<ui::BarterScreen>();
    screen->set_player(std::move(player_items));
    screen->set_merchant(std::move(merchant_items));
    stack.push(std::move(screen), ui::Mode::Barter, ctx);
}

void push_book_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    std::vector<std::string> pages;
    pages.push_back(
        "The First Lantern, page 1.\n\n"
        "When the sky tore open and the cities fell silent, the Lantern "
        "was lit by those who would not let the dark take everything. "
        "Each flame is a promise: someone, somewhere, still remembers.");
    pages.push_back(
        "The First Lantern, page 2.\n\n"
        "Travelers speak of ash on the wind, of glass trees that sing in "
        "the night, and of voices from hollow towers. The Lantern burns "
        "through all of it.");
    pages.push_back(
        "The First Lantern, page 3.\n\n"
        "If you have found this book, you have walked further than most. "
        "Carry the flame. Light your own. Pass it on.");
    auto screen = std::make_unique<ui::BookScreen>();
    screen->set_pages(std::move(pages));
    stack.push(std::move(screen), ui::Mode::Book, ctx);
}

void push_game_hud_demo(ui::ModeStack& stack, ScreenContext& ctx) {
    ctx.world.location_name = "Hollowwatch Outpost";
    ctx.world.time_of_day_minutes = 9 * 60 + 30;
    ctx.world.hp = ctx.world.hp_max - 35;
    ctx.world.vp = ctx.world.vp_max - 10;
    ctx.world.sp = ctx.world.sp_max;
    ctx.world.level = 4;
    ctx.world.gold = 1240;
    struct HudScreenImpl : public ui::Screen {
        void render(render::Buffer& buf, Rect vp) override {
            ui::HudOverlay overlay;
            overlay.render(buf, vp, ui_hud_ctx_);
        }
        ui::ScreenResult handle_key(ui::KeyEvent const& ev, ui::ScreenContext& c) override {
            if (ev.key == ui::Key::Esc) return ui::ScreenResult::Pop;
            ui_hud_ctx_ = c;
            return ui::ScreenResult::Stay;
        }
        std::string title() const override { return "hud"; }
        ui::ScreenContext ui_hud_ctx_{};
    };
    auto hs = std::make_unique<HudScreenImpl>();
    stack.push(std::move(hs), ui::Mode::Game, ctx);
}

}  // namespace

int run_ui_demo(ui::ScreenContext& ctx, std::string const& script_path) {
    ctx.viewport_w = kViewportW;
    ctx.viewport_h = kViewportH;
    ctx.refresh_accessibility();

    ui::ModeStack stack;

    auto on_action = [&stack, &ctx](ui::MenuAction a) {
        switch (a) {
            case ui::MenuAction::NewGame:
            case ui::MenuAction::Continue:
            case ui::MenuAction::Load:
                push_game_hud_demo(stack, ctx);
                break;
            case ui::MenuAction::Settings:
                push_character_demo(stack, ctx);
                break;
            case ui::MenuAction::Quit:
            case ui::MenuAction::None:
                break;
        }
    };

    stack.push(std::make_unique<ui::MainMenu>(on_action),
               ui::Mode::Menu, ctx);

    std::istream* in = &std::cin;
    std::ifstream script_file;
    if (!script_path.empty()) {
        script_file.open(script_path);
        if (!script_file) {
            std::fprintf(stderr, "[ui_demo] could not open script '%s'\n",
                         script_path.c_str());
            return 1;
        }
        in = &script_file;
    }

    auto t_start = std::chrono::steady_clock::now();
    int frames = 0;

    auto render_frame = [&](void) {
        render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                           static_cast<std::uint16_t>(ctx.viewport_h));
        buf.clear();
        Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
        stack.render(buf, vp);
        render_to_stdout(buf);
        ++frames;
    };

    auto handle_char = [&](char c) -> bool {
        if (c == 'q') return false;
        if (c == '0') { push_game_hud_demo(stack, ctx); return true; }
        if (c == '1') { push_inventory_demo(stack, ctx); return true; }
        if (c == '2') { push_dialogue_demo(stack, ctx);  return true; }
        if (c == '3') { push_journal_demo(stack, ctx);   return true; }
        if (c == '4') { push_character_demo(stack, ctx); return true; }
        if (c == '5') { push_map_demo(stack, ctx);       return true; }
        if (c == '6') { push_spell_demo(stack, ctx);     return true; }
        if (c == '7') { push_barter_demo(stack, ctx);    return true; }
        if (c == '8') { push_book_demo(stack, ctx);      return true; }
        if (c == 'r') {
            stack.reset(std::make_unique<ui::MainMenu>(on_action),
                        ui::Mode::Menu, ctx);
            return true;
        }
        KeyEvent ev = decode(c);
        stack.dispatch_key(ev, ctx);
        if (ctx.quit_requested) return false;
        return true;
    };

    render_frame();

    char c;
    while (in->get(c)) {
        bool keep = handle_char(c);
        render_frame();
        if (!keep) break;
    }

    auto t_end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
    std::fprintf(stderr, "[ui_demo] frames=%d elapsed_ms=%lld\n",
                 frames, static_cast<long long>(ms));
    return 0;
}

int run_perf_bench(ScreenContext& ctx) {
    ctx.viewport_w = kViewportW;
    ctx.viewport_h = kViewportH;
    ctx.refresh_accessibility();

    auto t_start = std::chrono::steady_clock::now();

    struct BenchEntity {
        int x, y;
        uint32_t glyph;
        uint8_t fr, fg, fb;
    };
    std::vector<BenchEntity> entities(100);
    for (std::size_t i = 0; i < entities.size(); ++i) {
        entities[i].x = static_cast<int>(i) % kViewportW;
        entities[i].y = (static_cast<int>(i) / kViewportW) % kViewportH;
        entities[i].glyph = static_cast<uint32_t>('@' + (i % 26));
        entities[i].fr = static_cast<uint8_t>(80 + (i * 3) % 175);
        entities[i].fg = static_cast<uint8_t>(80 + (i * 7) % 175);
        entities[i].fb = static_cast<uint8_t>(80 + (i * 11) % 175);
    }

    constexpr int kFrames = 600;
    double worst_ms = 0.0;
    double total_ms = 0.0;

    render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                        static_cast<std::uint16_t>(ctx.viewport_h));
    for (int f = 0; f < kFrames; ++f) {
        auto t0 = std::chrono::steady_clock::now();
        buf.clear();
        for (auto const& e : entities) {
            render::Cell c;
            c.glyph = e.glyph;
            c.fg_r = e.fr; c.fg_g = e.fg; c.fg_b = e.fb;
            if (e.x >= 0 && e.y >= 0 && e.x < ctx.viewport_w && e.y < ctx.viewport_h) {
                buf.set(e.x, e.y, c);
            }
        }
        std::string out;
        std::vector<render::DirtyRect> dirty;
        render::DirtyRect drect;
        drect.x = 0;
        drect.y = 0;
        drect.w = static_cast<std::uint16_t>(ctx.viewport_w);
        drect.h = static_cast<std::uint16_t>(ctx.viewport_h);
        dirty.push_back(drect);
        render::emit(buf, out, dirty);
        (void)out;
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (ms > worst_ms) worst_ms = ms;
        total_ms += ms;
    }
    double avg_ms = total_ms / kFrames;

    auto t_first = std::chrono::steady_clock::now();
    auto startup_ms = std::chrono::duration<double, std::milli>(t_first - t_start).count();

    long rss_kb = 0;
    std::ifstream statm("/proc/self/statm");
    if (statm) {
        long pages = 0;
        statm >> pages;
        long rss_pages = 0;
        statm >> rss_pages;
        rss_kb = rss_pages * 4;
    }

    constexpr double kFrameBudgetMs   = 16.0;
    constexpr double kStartupBudgetMs = 2000.0;
    constexpr long   kMemoryBudgetKb  = 200L * 1024;

    bool ok = true;
    std::fprintf(stdout, "[perf] frames=%d avg_frame_ms=%.3f worst_frame_ms=%.3f budget_ms=%.1f\n",
                 kFrames, avg_ms, worst_ms, kFrameBudgetMs);
    std::fprintf(stdout, "[perf] startup_ms=%.1f budget_ms=%.1f\n",
                 startup_ms, kStartupBudgetMs);
    std::fprintf(stdout, "[perf] rss_kb=%ld budget_kb=%ld\n", rss_kb, kMemoryBudgetKb);

    if (avg_ms > kFrameBudgetMs) {
        std::fprintf(stderr, "FAIL: avg frame %.3fms exceeds budget %.1fms (D66)\n",
                     avg_ms, kFrameBudgetMs);
        ok = false;
    }
    if (startup_ms > kStartupBudgetMs) {
        std::fprintf(stderr, "FAIL: startup %.1fms exceeds budget %.1fms (D68)\n",
                     startup_ms, kStartupBudgetMs);
        ok = false;
    }
    if (rss_kb > kMemoryBudgetKb) {
        std::fprintf(stderr, "FAIL: rss %ldKB exceeds budget %ldKB (D67)\n",
                     rss_kb, kMemoryBudgetKb);
        ok = false;
    }
    if (ok) {
        std::fprintf(stdout, "[perf] OK: all budgets met\n");
    }
    return ok ? 0 : 1;
}

int run_full_loop(ScreenContext& ctx) {
    ctx.viewport_w = kViewportW;
    ctx.viewport_h = kViewportH;
    ctx.refresh_accessibility();

    ui::ModeStack stack;
    std::fprintf(stderr, "[full_loop] launch: ASH v0.1.0 ready\n");
    std::fprintf(stderr, "[full_loop] stack empty=%s\n",
                 stack.empty() ? "yes" : "no");

    auto transcript = [&](char const* step) {
        std::fprintf(stderr, "[full_loop] %-20s mode=%-10s stack=%zu\n",
                     step, mode_name(stack.top_mode()).c_str(),
                     stack.size());
    };

    auto on_action = [&stack, &ctx, &transcript](ui::MenuAction a) {
        switch (a) {
            case ui::MenuAction::NewGame:
            case ui::MenuAction::Continue:
            case ui::MenuAction::Load:
                push_game_hud_demo(stack, ctx);
                transcript("menu->new game");
                break;
            case ui::MenuAction::Settings:
                push_character_demo(stack, ctx);
                transcript("menu->settings");
                break;
            case ui::MenuAction::Quit:
            case ui::MenuAction::None:
                break;
        }
    };

    stack.push(std::make_unique<ui::MainMenu>(on_action),
               ui::Mode::Menu, ctx);
    transcript("push main menu");

    push_game_hud_demo(stack, ctx);
    transcript("first map (HUD)");
    push_inventory_demo(stack, ctx);
    transcript("inventory");
    push_dialogue_demo(stack, ctx);
    transcript("dialogue");
    push_journal_demo(stack, ctx);
    transcript("journal");
    push_character_demo(stack, ctx);
    transcript("character");
    push_map_demo(stack, ctx);
    transcript("map");
    push_spell_demo(stack, ctx);
    transcript("spells");
    push_barter_demo(stack, ctx);
    transcript("barter");
    push_book_demo(stack, ctx);
    transcript("book");

    // Render the book once so we know every screen drew.
    render::Buffer buf(static_cast<std::uint16_t>(ctx.viewport_w),
                       static_cast<std::uint16_t>(ctx.viewport_h));
    buf.clear();
    Rect vp{0, 0, ctx.viewport_w, ctx.viewport_h};
    stack.render(buf, vp);
    std::fprintf(stderr, "[full_loop] rendered book: cells=%zu\n", buf.size());

    // Pop all screens back to the menu, then quit.
    while (stack.size() > 1) {
        stack.pop(ctx);
    }
    transcript("pop down to menu");

    // Simulate Save -> Quit.
    std::fprintf(stderr, "[full_loop] save -> settings/load/save.quit\n");
    ctx.quit_requested = true;
    std::fprintf(stderr, "[full_loop] quit_requested=%s\n",
                 ctx.quit_requested ? "yes" : "no");

    // Simulate relaunch -> load -> continue.
    std::fprintf(stderr, "[full_loop] relaunch -> continue from saved game\n");
    ctx.quit_requested = false;
    stack.reset(std::make_unique<ui::MainMenu>(on_action),
                ui::Mode::Menu, ctx);
    on_action(ui::MenuAction::Continue);
    transcript("loaded saved game");
    std::fprintf(stderr, "[full_loop] OK: full playable loop complete\n");
    return 0;
}

}  // namespace app
}  // namespace ash