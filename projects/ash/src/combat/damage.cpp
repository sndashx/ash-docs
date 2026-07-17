#include "combat/damage.hpp"

namespace ash {
namespace combat {

DamageOutcome apply_damage(world::Entity&      target,
                           DamageEvent const& evt,
                           int                dr_by_type,
                           ResistanceFn       res_fn) noexcept {
    DamageOutcome o{};
    if (!target.alive) {
        return o;
    }
    int amount = evt.amount;
    if (res_fn != nullptr) {
        int res = res_fn(evt.target, evt.type);
        if (res >= 100) {
            return o;  /// immune
        }
        amount = (amount * (100 - res)) / 100;
    }
    if (dr_by_type < 0)   dr_by_type = 0;
    if (dr_by_type > 100) dr_by_type = 100;
    amount = (amount * (100 - dr_by_type)) / 100;
    if (amount < 1) amount = 1;
    target.stats.hp -= amount;
    o.amount_dealt = amount;
    if (target.stats.hp <= 0) {
        target.stats.hp = 0;
        target.alive = false;
        o.target_died = true;
    }
    return o;
}

world::Entity make_corpse_from(world::Entity const& dead) noexcept {
    world::Entity c{};
    c.has_position = true;
    c.position.cell = dead.position.cell;
    c.position.facing = dead.position.facing;
    c.has_inventory = true;
    c.inventory = dead.inventory;
    c.has_corpse = true;
    c.corpse.former_owner = dead.id;
    c.alive = false;
    return c;
}

}  // namespace combat
}  // namespace ash
