#include "app/app.hpp"

#include "character/attributes.hpp"
#include "character/derived.hpp"
#include "character/inventory.hpp"
#include "character/skills.hpp"
#include "combat/attack.hpp"
#include "combat/combat.hpp"
#include "combat/cover.hpp"
#include "combat/damage.hpp"
#include "combat/weapon.hpp"
#include "core/log.hpp"
#include "core/version.hpp"
#include "platform/signal.hpp"
#include "render/ansi.hpp"
#include "world/components.hpp"
#include "world/map.hpp"
#include "world/query.hpp"
#include "world/spawn.hpp"
#include "world/world.hpp"

#include <iostream>

namespace ash {
namespace app {

namespace {
constexpr const char* kBannerLine2 = "A hand-authored ASCII RPG";

void write_banner_line1(std::ostream& os) {
    os << ansi::set_fg(255, 80, 200)
       << "ASH v" << core::ASH_VERSION_STRING
       << " (" << core::ASH_CODENAME << ")"
       << ansi::reset();
}
}  // namespace

App::App(int argc, char** argv)
    : args_(cli::parse(argc, argv)) {
    log::init();
    if (args_.log_level) {
        log::set_level(*args_.log_level);
    }
    platform::install_signal_handlers();
}

App::~App() {
    // Phase 0: no terminal state to restore yet. Phase 1 will restore
    // raw-mode, alt screen, and hidden cursor.
    std::cout << ansi::show_cursor() << std::flush;
}

int App::run() {
    if (args_.error) {
        return 1;
    }
    if (args_.show_version) {
        write_banner_line1(std::cout);
        std::cout << std::flush;
        return 0;
    }
    if (args_.show_help) {
        cli::usage(std::cout);
        return 0;
    }
    if (args_.render_test) {
        /// Wired up by Phase 1. For now, same banner stub.
        write_banner_line1(std::cout);
        std::cout << "\n"
                  << ansi::set_fg(80, 200, 255)
                  << "(render-test stub)"
                  << ansi::reset() << std::flush;
        return 0;
    }
    if (args_.char_test) {
        /// Spawn a player with starting attrs (40 across the board) and
        /// starting skills (5 across the board), empty inventory, print
        /// the derived stat block per Pillar 5.
        using namespace ash::character;
        Attributes a;
        for (auto& v : a.values) { v = 40; }
        Skills s;
        for (auto& v : s.values) { v = 5; }
        Inventory inv;
        auto const d = recompute(a, s, inv);
        std::cout << "Character sheet (starting player, all attrs=40, all skills=5)\n"
                  << "  HP_max              = " << d.hp_max              << "\n"
                  << "  VP_max              = " << d.vp_max              << "\n"
                  << "  SP_max              = " << d.sp_max              << "\n"
                  << "  carry_capacity      = " << d.carry_capacity      << "\n"
                  << "  speed_cells_per_sec = " << d.speed_cells_per_sec << "\n"
                  << "  crit_chance_pct     = " << d.crit_chance_pct     << "\n"
                  << "  barter_discount     = " << d.barter_discount     << "\n"
                  << "  identify_skill      = " << d.identify_skill      << "\n"
                  << "  pick_skill          = " << d.pick_skill          << "\n"
                  << "  persuade_skill      = " << d.persuade_skill      << "\n"
                  << "  encumbered          = " << (d.encumbered ? "yes" : "no") << "\n"
                  << std::flush;
        return 0;
    }
    if (args_.combat_test) {
        /// Phase 7 acceptance-criteria demo: 5x5 arena, player + 1 enemy,
        /// spacebar pauses; on unpause, initiative rolls, both act.
        /// Player attacks, applies damage, enemy dies -> corpse.
        using namespace ash;
        std::cout << "ASH Phase 7 combat demo: 5x5 arena\n"
                  << "----------------------------------------\n";
        world::World w;
        w.active_map = world::make_arena(5, 5);

        character::Attributes player_attrs;
        for (auto& v : player_attrs.values) v = 50;
        character::Skills player_skills;
        for (auto& v : player_skills.values) v = 50;
        auto player = world::make_combatant({1, 2}, player_attrs, player_skills, 50);
        player.is_player = true;
        player.position.facing = world::Facing::East;

        character::Attributes enemy_attrs;
        for (auto& v : enemy_attrs.values) v = 30;
        character::Skills enemy_skills;
        for (auto& v : enemy_skills.values) v = 15;
        auto enemy = world::make_combatant({3, 2}, enemy_attrs, enemy_skills, 30);
        enemy.position.facing = world::Facing::West;
        enemy.has_ai = true;
        enemy.ai.leash_cell = {3, 2};
        enemy.ai.leash_radius = 5;
        enemy.ai.aggression = 1.0f;
        enemy.ai.behaviors = {static_cast<int>(combat::Behavior::Berserk)};

        combat::WeaponDatabase wdb;
        wdb.load_from_file("content/items/weapons.json");
        combat::WeaponDef sword{};
        sword.code_name = "demo_sword";
        sword.display_name = "Demo Sword";
        sword.type = combat::WeaponType::Melee1H;
        sword.damage = combat::DamageType::Slash;
        sword.damage_min = 8;
        sword.damage_max = 12;
        sword.swing_arc_deg = 120;
        sword.reach = 1;
        wdb.add(sword);
        combat::WeaponDef const* wpn = wdb.get_by_code("demo_sword");
        player.combatant.weapon_id = wpn ? wpn->id : 0;

        auto pid = w.add(std::move(player));
        auto eid = w.add(std::move(enemy));

        combat::CombatManager mgr;
        mgr.begin_combat(w, {pid, eid});
        std::cout << "Combat started. Initiative order:\n";
        for (auto id : mgr.order()) {
            auto const* e = w.find(id);
            std::cout << "  entity " << id.value
                      << " (HP=" << e->stats.hp << ")\n";
        }
        std::cout << "Paused state demo: calling mgr.pause() then mgr.tick(); round "
                  << "should stay at 1.\n";
        mgr.pause();
        mgr.tick(w, 1000);
        std::cout << "After paused tick: round = " << mgr.round_number() << "\n";
        std::cout << "Resuming combat and letting it tick for 6 seconds "
                  << "(two combat rounds at 3s each).\n";
        mgr.resume();
        for (int i = 0; i < 60; ++i) {
            mgr.tick(w, 100);
        }
        std::cout << "Round after 6s of combat: " << mgr.round_number() << "\n";

        /// Player attacks enemy each frame until one dies. The player
        /// uses a simple pathfinding-driven approach so the demo
        /// exercises both movement and attack subsystems end-to-end.
        int turn = 0;
        while (turn < 400) {
            auto* att = w.find(pid);
            auto* def = w.find(eid);
            if (!att->alive) {
                std::cout << "Player died. Combat ends.\n";
                break;
            }
            if (!def->alive) {
                std::cout << "Enemy died. Spawning corpse.\n";
                world::Entity corpse = combat::make_corpse_from(*def);
                auto cid = w.add(std::move(corpse));
                std::cout << "  corpse entity " << cid.value << " at ("
                          << w.find(cid)->position.cell.x << ","
                          << w.find(cid)->position.cell.y << ")\n";
                break;
            }
            /// If not adjacent, take one step toward the enemy.
            int cheb = world::chebyshev(att->position.cell, def->position.cell);
            if (cheb > 1) {
                int dx = def->position.cell.x - att->position.cell.x;
                int dy = def->position.cell.y - att->position.cell.y;
                int sx = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
                int sy = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;
                int nx = att->position.cell.x + sx;
                int ny = att->position.cell.y + sy;
                if ((sx == 0 && sy == 0) ||
                    !w.active_map.is_walkable(nx, ny, false) ||
                    (def->position.cell.x == nx && def->position.cell.y == ny)) {
                    /// Fallback: try x-only.
                    if (sx != 0 && w.active_map.is_walkable(att->position.cell.x + sx,
                                                            att->position.cell.y, false) &&
                        (def->position.cell.x != att->position.cell.x + sx ||
                         def->position.cell.y != att->position.cell.y)) {
                        att->position.cell.x += sx;
                    } else if (sy != 0 && w.active_map.is_walkable(att->position.cell.x,
                                                                   att->position.cell.y + sy, false) &&
                               (def->position.cell.x != att->position.cell.x ||
                                def->position.cell.y != att->position.cell.y + sy)) {
                        att->position.cell.y += sy;
                    }
                } else {
                    att->position.cell.x = nx;
                    att->position.cell.y = ny;
                }
                if (sx != 0 || sy != 0) {
                    att->position.facing = (sx > 0) ? world::Facing::East :
                                              (sx < 0) ? world::Facing::West :
                                              (sy > 0) ? world::Facing::South :
                                                          world::Facing::North;
                }
                std::cout << "Turn " << turn << ": moved to ("
                          << att->position.cell.x << ","
                          << att->position.cell.y << ")\n";
            } else {
                /// In range: attack.
                combat::Cover cover = combat::compute_cover(
                    w.active_map, att->position.cell, def->position.cell);
                auto r = combat::attack_resolve(*att, *def, wpn, cover);
                std::cout << "Turn " << turn << ": ";
                if (r.hit) {
                    int hp_before = def->stats.hp;
                    combat::DamageEvent de{};
                    de.amount = r.damage;
                    de.type = r.type;
                    de.source = pid;
                    de.target = eid;
                    de.crit = r.crit;
                    auto out = combat::apply_damage(*def, de, 0);
                    std::cout << (r.crit ? "CRIT " : "hit ")
                              << "for " << out.amount_dealt
                              << " damage (HP " << hp_before << " -> "
                              << def->stats.hp << ")";
                    if (out.target_died) {
                        std::cout << " [DEAD]";
                    }
                    std::cout << "\n";
                } else {
                    std::cout << "miss (" << r.message << ")\n";
                }
            }
            mgr.tick(w, 16);
            ++turn;
        }
        std::cout << "Combat demo complete.\n";
        return 0;
    }

    // Default action: truecolor banner (spec lines 384–386).
    write_banner_line1(std::cout);
    std::cout << "\n"
              << ansi::set_fg(80, 200, 255)
              << kBannerLine2
              << ansi::reset() << std::flush;
    return 0;
}

}  // namespace app
}  // namespace ash