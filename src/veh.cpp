
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

int __cdecl find_return_base(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq;
    debug("find_return_base %2d %2d %s\n", veh->x, veh->y, veh->name());
    TileSearch ts;
    ts.init(veh->x, veh->y, veh->triad() == TRIAD_SEA ? TS_SEA_AND_SHORE : veh->triad());
    while ((sq = ts.get_next()) != NULL) {
        if (sq->is_base() && sq->owner == veh->faction_id) {
            return base_at(ts.rx, ts.ry);
        }
    }
    int region = region_at(veh->x, veh->y);
    int min_dist = INT_MAX;
    int base_id = -1;
    bool sea = region > MaxRegionLandNum;
    for (int i = 0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == veh->faction_id) {
            int base_region = region_at(base->x, base->y);
            int dist = map_range(base->x, base->y, veh->x, veh->y)
                * ((base_region > MaxRegionLandNum) == sea ? 1 : 4)
                * (base_region == region ? 1 : 8);
            if (dist < min_dist) {
                base_id = i;
                min_dist = dist;
            }
        }
    }
    return base_id;
}

int __cdecl probe_return_base(int UNUSED(x), int UNUSED(y), int veh_id) {
    return find_return_base(veh_id);
}

VehArmor best_armor(int faction, bool cheap) {
    int ci = ARM_NO_ARMOR;
    int cv = 4;
    for (int i=ARM_SYNTHMETAL_ARMOR; i<=ARM_RESONANCE_8_ARMOR; i++) {
        if (has_tech(faction, Armor[i].preq_tech)) {
            int val = Armor[i].defense_value;
            int cost = Armor[i].cost;
            if (cheap && (cost > 5 || cost > val))
                continue;
            int iv = val * (i >= ARM_PULSE_3_ARMOR ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return (VehArmor)ci;
}

VehWeapon best_weapon(int faction) {
    int ci = WPN_HAND_WEAPONS;
    int cv = 4;
    for (int i=WPN_LASER; i<=WPN_STRING_DISRUPTOR; i++) {
        if (has_tech(faction, Weapon[i].preq_tech)) {
            int iv = Weapon[i].offense_value *
                (i == WPN_RESONANCE_LASER || i == WPN_RESONANCE_BOLT ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return (VehWeapon)ci;
}

VehReactor best_reactor(int faction) {
    for (VehReactor r : {REC_SINGULARITY, REC_QUANTUM, REC_FUSION}) {
        if (has_tech(faction, Reactor[r - 1].preq_tech)) {
            return r;
        }
    }
    return REC_FISSION;
}

int offense_value(UNIT* u) {
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_type);
    if (u->weapon_type == WPN_CONVENTIONAL_PAYLOAD) {
        int wpn = best_weapon(*active_faction);
        return max(1, Weapon[wpn].offense_value * w);
    }
    return Weapon[u->weapon_type].offense_value * w;
}

int defense_value(UNIT* u) {
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_type);
    return Armor[u->armor_type].defense_value * w;
}

int set_move_to(int veh_id, int x, int y) {
    VEH* veh = &Vehicles[veh_id];
    debug("set_move_to %2d %2d -> %2d %2d %s\n", veh->x, veh->y, x, y, veh->name());
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->order = ORDER_MOVE_TO;
    veh->status_icon = 'G';
    veh->terraforming_turns = 0;
    mapnodes.erase({x, y, NODE_PATROL});
    mapnodes.erase({x, y, NODE_COMBAT_PATROL});
    if (veh->x == x && veh->y == y) {
        return mod_veh_skip(veh_id);
    }
    pm_target[x][y]++;
    return SYNC;
}

int set_move_next(int veh_id, int offset) {
    VEH* veh = &Vehicles[veh_id];
    int x = wrap(veh->x + BaseOffsetX[offset]);
    int y = veh->y + BaseOffsetY[offset];
    return set_move_to(veh_id, x, y);
}

int set_road_to(int veh_id, int x, int y) {
    VEH* veh = &Vehicles[veh_id];
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->order = ORDER_ROAD_TO;
    veh->status_icon = 'R';
    return SYNC;
}

int set_action(int veh_id, int act, char icon) {
    VEH* veh = &Vehicles[veh_id];
    if (act == ORDER_THERMAL_BOREHOLE) {
        mapnodes.insert({veh->x, veh->y, NODE_BOREHOLE});
    } else if (act == ORDER_TERRAFORM_UP) {
        mapnodes.insert({veh->x, veh->y, NODE_RAISE_LAND});
    } else if (act == ORDER_SENSOR_ARRAY) {
        mapnodes.insert({veh->x, veh->y, NODE_SENSOR_ARRAY});
    }
    veh->order = act;
    veh->status_icon = icon;
    veh->state &= ~VSTATE_UNK_10000;
    return SYNC;
}

int set_convoy(int veh_id, ResType res) {
    VEH* veh = &Vehicles[veh_id];
    mapnodes.insert({veh->x, veh->y, NODE_CONVOY});
    veh->order_auto_type = res-1;
    veh->order = ORDER_CONVOY;
    veh->status_icon = 'C';
    return veh_skip(veh_id);
}

int set_board_to(int veh_id, int trans_veh_id) {
    VEH* veh = &Vehicles[veh_id];
    VEH* v2 = &Vehicles[trans_veh_id];
    assert(veh_id != trans_veh_id);
    assert(veh->x == v2->x && veh->y == v2->y);
    assert(veh_cargo(trans_veh_id) > 0);
    veh->order = ORDER_SENTRY_BOARD;
    veh->waypoint_1_x = trans_veh_id;
    veh->waypoint_1_y = 0;
    veh->status_icon = 'L';
    debug("set_board_to %2d %2d %s -> %s\n", veh->x, veh->y, veh->name(), v2->name());
    return SYNC;
}

int cargo_loaded(int veh_id) {
    int n=0;
    for (int i=0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->order == ORDER_SENTRY_BOARD && veh->waypoint_1_x == veh_id) {
            n++;
        }
    }
    return n;
}

