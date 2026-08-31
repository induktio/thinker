
#include "gamewin.h"

void __thiscall Console_automate(Console*, int veh_id, VehOrderAutoType mode) {
    debug("Console_automate %d %d\n", veh_id, mode);
    if (veh_id < 0 || veh_id >= *VehCount) {
        return;
    }
    VEH& veh = Vehs[veh_id];
    UNIT& unit = Units[veh.unit_id];
    bool check = mode != ORDERA_BOMBING_RUN;
    if (unit.plan == PLAN_TERRAFORM) {
        if (mode == ORDERA_TERRA_AUTO_MAGTUBE || mode == ORDERA_TERRA_AUTO_ROAD) {
            if (unit.triad() != TRIAD_LAND || is_ocean(veh.x, veh.y)) {
                return;
            }
        }
        if (mode == ORDERA_TERRA_AUTO_MAGTUBE) {
            if (!has_tech(Terraform[FORMER_MAGTUBE].preq_tech, veh.faction_id)) {
                return;
            }
            check = true;
        } else if (mode == ORDERA_TERRA_FARM_MINE_ROAD || mode == ORDERA_TERRA_FARM_SOLAR_ROAD) {
            MAP* sq = mapsq(veh.x, veh.y);
            if (!sq || sq->base_who() >= 0) {
                return;
            }
        }
    }
    if (check) {
        if (!NetDaemon_lock_veh(NetState, &veh_id, 0, -1, -1, 0)) {
            if (mode == ORDERA_ON_ALERT) {
                veh.waypoint_x[1] = veh.x;
                veh.waypoint_y[1] = veh.y;
                if (unit.range() != 0) {
                    action_home(veh_id, 0);
                    if (veh.order == ORDER_MOVE_TO) {
                        veh.waypoint_x[1] = veh.waypoint_x[0];
                        veh.waypoint_y[1] = veh.waypoint_y[0];
                    }
                    veh.order = ORDER_NONE;
                }
            }
            veh.order_auto_type = mode;
            veh.order = ORDER_NONE;
            veh.state = (veh.state & ~(VSTATE_UNK_2000000|VSTATE_UNK_1000000)) | VSTATE_ON_ALERT;
            veh.waypoint_count = 0;
            synch_veh(veh_id);
            NetDaemon_await_synch(NetState);
            NetDaemon_unlock_veh(NetState);
        }
    } else {
        if (unit.triad() == TRIAD_AIR && unit.offense_value() != 0) {
            veh.order = ORDER_NONE;
            Console_cursor_on_2(MapWin, 11, -1);
        }
    }
}

/*
Fix potential crash when a game is loaded after using Edit Map > Generate/Remove Fungus > No Fungus.
Original version changed MapWin->cOwner variable for unknown reason which is skipped.
*/
void __thiscall Console_editor_fungus(Console*) {
    auto_undo();
    int v1 = X_pop_6("FUNGOSITY", PopDialogBtnCancel, 0);
    if (v1 >= 0) {
        int v2 = 0;
        if (!v1 || (v2 = X_pop_6("FUNGMOTIZE", PopDialogBtnCancel, 0)) > 0) {
            MAP* sq = *MapTiles;
            for (int i = 0; i < *MapAreaTiles; ++i, ++sq) {
                sq->items &= ~BIT_FUNGUS;
                for (int j = 1; j < 8; ++j) {
                    sq->visible_items[j - 1] = sq->items;
                }
            }
        }
        if (v2 < 0) {
            return;
        }
        if (v1 > 0) {
            int v3 = *MapNativeLifeForms;
            *MapNativeLifeForms = v1 - 1;
            world_fungus();
            *MapNativeLifeForms = v3;
        }
        draw_map(1);
    }
}

