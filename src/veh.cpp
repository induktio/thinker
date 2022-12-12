
#include "veh.h"


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

int mod_veh_skip(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    veh->waypoint_1_x = veh->x;
    veh->waypoint_1_y = veh->y;
    veh->status_icon = '-';
    if (veh->need_monolith()) {
        MAP* sq = mapsq(veh->x, veh->y);
        if (sq && sq->items & BIT_MONOLITH) {
            monolith(veh_id);
        }
    }
    return veh_skip(veh_id);
}

