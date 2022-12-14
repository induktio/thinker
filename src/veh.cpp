
#include "veh.h"


bool __cdecl can_arty(int unit_id, bool allow_sea_arty) {
    UNIT& u = Units[unit_id];
    if ((Weapon[u.weapon_type].offense_value <= 0 // PSI + non-combat
    || Armor[u.armor_type].defense_value < 0) // PSI
    && unit_id != BSC_SPORE_LAUNCHER) { // Spore Launcher exception
        return false;
    }
    if (u.triad() == TRIAD_SEA) {
        return allow_sea_arty;
    }
    if (u.triad() == TRIAD_AIR) {
        return false;
    }
    return has_abil(unit_id, ABL_ARTILLERY); // TRIAD_LAND
}

/*
Original version assigned cargo capacity on Spore Launchers but it seems this feature is not used.
*/
int __cdecl veh_cargo(int veh_id) {
    VEH* v = &Vehicles[veh_id];
    UNIT* u = &Units[v->unit_id];
    if (u->carry_capacity > 0 && veh_id < MaxProtoFactionNum
    && Weapon[u->weapon_type].offense_value < 0) {
        return v->morale + 1;
    }
    return u->carry_capacity;
}

int mod_veh_speed(int veh_id) {
    return veh_speed(veh_id, 0);
}

int __cdecl mod_veh_init(int unit_id, int faction, int x, int y) {
    int id = veh_init(unit_id, faction, x, y);
    if (id >= 0) {
        Vehicles[id].home_base_id = -1;
        // Set these flags to disable any non-Thinker unit automation.
        Vehicles[id].state |= VSTATE_UNK_40000;
        Vehicles[id].state &= ~VSTATE_UNK_2000;
    }
    return id;
}

int __cdecl mod_veh_kill(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    debug("disband %2d %2d %s\n", veh->x, veh->y, veh->name());
    return veh_kill(veh_id);
}

int __cdecl mod_veh_skip(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    int moves = veh_speed(veh_id, 0);

    if (is_human(veh->faction_id)) {
        if (conf.activate_skipped_units) {
            if (!veh->road_moves_spent && veh->order < ORDER_FARM && !(veh->state & VSTATE_HAS_MOVED)) {
                veh->flags |= VFLAG_FULL_MOVE_SKIPPED;
            } else {
                veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
            }
        }
    } else if (veh->faction_id > 0) {
        veh->waypoint_1_x = veh->x;
        veh->waypoint_1_y = veh->y;
        veh->status_icon = '-';
        if (veh->need_monolith()) {
            MAP* sq = mapsq(veh->x, veh->y);
            if (sq && sq->items & BIT_MONOLITH) {
                monolith(veh_id);
            }
        }
    }
    veh->road_moves_spent = moves;
    return moves;
}

int __cdecl mod_veh_wake(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    if (veh->order >= ORDER_FARM && veh->order < ORDER_MOVE_TO && !(veh->state & VSTATE_CRAWLING)) {
        veh->road_moves_spent = veh_speed(veh_id, 0) - Rules->move_rate_roads;
        if (veh->terraforming_turns) {
            int turns = veh->terraforming_turns - contribution(veh_id, veh->order - 4);
            veh->terraforming_turns = max(0, turns);
        }
    }
    if (veh->state & VSTATE_ON_ALERT && !(veh->state & VSTATE_HAS_MOVED) && veh->order_auto_type == ORDERA_ON_ALERT) {
        veh->road_moves_spent = 0;
    }
    /*
    Formers are a special case since they might not move but can build items immediately.
    When formers build something, VSTATE_HAS_MOVED should be set to prevent them from moving twice per turn.
    */
    if (conf.activate_skipped_units) {
        if (veh->flags & VFLAG_FULL_MOVE_SKIPPED && !(veh->state & VSTATE_HAS_MOVED)) {
            veh->road_moves_spent = 0;
        }
        veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
    }
    veh->order = ORDER_NONE;
    veh->state &= ~(VSTATE_UNK_8000000|VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
    return veh_id;
}

