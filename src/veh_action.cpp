
#include "veh_action.h"


int __cdecl action(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    uint8_t order = veh->order;

    if (order >= VehOrderFormerFirst && order <= VehOrderFormerLast) {
        action_terraform(veh_id, order - VehOrderFormerFirst, 1);
    } else if (order == ORDER_ROAD_TO || order == ORDER_MAGTUBE_TO) {
        action_road_to(veh_id);
    } else if (order == ORDER_MOVE_TO || order == ORDER_AI_GO_TO || order == ORDER_MOVE) {
        action_go_to(veh_id);
    } else {
        return 0;
    }
    if (veh->order & VehOrderResetFlag) { // ORDER_AI_GO_TO
        veh->order = ORDER_NONE;
    }
    return 1;
}

int __cdecl action_build(int veh_id, const char* name) {
    const int player_id = *CurrentPlayerFaction;
    VEH* veh = &Vehs[veh_id];
    int veh_x = veh->x;
    int veh_y = veh->y;
    int faction_id = veh->faction_id;
    MAP* sq = mapsq(veh_x, veh_y);
    debug("action_build %d %d %2d %2d\n", *CurrentTurn, faction_id, veh_x, veh_y);

    if (sq && sq->val3 & TILE_ROCKY) {
        rocky_set(veh_x, veh_y, LEVEL_ROLLING);
    }
    if (faction_id == player_id
    || veh->is_visible(player_id)
    || (*GameState & STATE_OMNISCIENT_VIEW)) {
        Console_focus(MapWin, veh_x, veh_y, faction_id);
    }
    int owner = whose_territory(faction_id, veh_x, veh_y, 0, 0);
    if (Factions[faction_id].base_count > 0 && owner >= 0 && owner != faction_id
    && !(Factions[faction_id].diplo_status[owner] & (DIPLO_VENDETTA | DIPLO_PACT))
    && (is_human(faction_id) || is_human(owner))) {
        if (Factions[faction_id].diplo_status[owner] & DIPLO_TREATY) {
            double_cross(faction_id, owner, -1);
        } else {
            treaty_on(faction_id, owner, DIPLO_VENDETTA);
        }
    }
    int base_id = mod_base_init(faction_id, veh_x, veh_y);
    if (base_id >= 0) {
        if (is_human(faction_id)) {
            int home_base_id = veh->home_base_id;
            if (home_base_id >= 0) {
                Bases[base_id].governor_flags = Bases[home_base_id].governor_flags;
            }
            if (veh->state & VSTATE_ON_ALERT) {
                Bases[base_id].governor_flags |= GOV_ACTIVE;
            }
        }
        Factions[faction_id].last_base_turn = *CurrentTurn;
        if (name && strlen(name) > 0) {
            strcpy_n(Bases[base_id].name, MaxBaseNameLen, name);
            make_base_unique(base_id);
        }
        veh_kill(veh_id);
        if (faction_id == player_id
        || Bases[base_id].faction_id == player_id
        || (Bases[base_id].visibility & (1 << player_id))
        || (*GameState & STATE_OMNISCIENT_VIEW)) {
            draw_map(1);
        }
        for (int i = 1; i < MaxPlayerNum; i++) {
            del_site(i, 8, veh_x, veh_y, 2);
        }
        mon_colony_founded(faction_id, Bases[base_id].name);
        return base_id;
    } else {
        veh_skip(veh_id);
        return base_id;
    }
}

int __cdecl action_terraform(int veh_id, int item_id, int toggle) {
    const int player_id = *CurrentPlayerFaction;
    VEH* const veh = &Vehs[veh_id];
    const int x = veh->x;
    const int y = veh->y;
    const int faction_id = veh->faction_id;
    MAP* const sq = mapsq(x, y);
    const int ocean = is_ocean(sq);
    const int initial_veh_count = *VehCount;

    if (item_id == FORMER_ROAD) {
        if (sq->items & BIT_ROAD) {
            int tech_id = ocean ?
                Terraform[FORMER_MAGTUBE].preq_tech_sea : Terraform[FORMER_MAGTUBE].preq_tech;
            if (has_tech(tech_id, faction_id)) {
                item_id = FORMER_MAGTUBE;
            }
        }
    } else if (item_id == FORMER_FARM) {
        if ((sq->items & BIT_FARM) && !ocean) {
            if (has_tech(Terraform[FORMER_SOIL_ENR].preq_tech, faction_id)) {
                item_id = FORMER_SOIL_ENR;
            }
        }
    }
    if (toggle) {
        veh->order = item_id + VehOrderFormerFirst;
    }
    int turns = Terraform[item_id].rate;

    switch (item_id) {
    case FORMER_SOLAR:
        turns += (turns * (sq->rocky_level())) / 2;
        break;
    case FORMER_ROAD:
    case FORMER_MAGTUBE: {
        int items = sq->items;
        turns = Terraform[(items & BIT_ROAD ? FORMER_MAGTUBE : FORMER_ROAD)].rate;
        if (items & BIT_RIVER) { ++turns; }
        if (sq->is_fungus()) { turns += 2; }
        if (items & BIT_FOREST) {
            turns += 2;
        } else {
            turns += sq->rocky_level();
        }
        break;
    }
    case FORMER_REMOVE_FUNGUS:
        if (has_abil(veh->unit_id, ABL_FUNGICIDAL)) {
            turns /= 2;
        }
        break;
    default:
        break;
    }

    if (!is_human(faction_id) && *DiffLevel > 3 && turns > 3) {
        --turns; // original AI bonus
    }
    int total_work = 2 * turns;
    int helper_rate = 0;
    int helper_done = 0;

    if (veh_id >= 0) {
        for (int cur_id = veh_top(veh_id); cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
            if (cur_id == veh_id) { continue; }
            VEH* other = &Vehs[cur_id];
            if (other->order != item_id + 4) { continue; }
            if (!toggle) {
                helper_rate += contribution(cur_id, item_id);
                helper_done += other->movement_turns;
            } else {
                veh->movement_turns += other->movement_turns;
                other->movement_turns = 0;
            }
        }
    }

    int work_rate = contribution(veh_id, item_id) + helper_rate;
    int work_done = veh->movement_turns + helper_done;

    if (!toggle) {
        int remaining_moves = veh_speed(veh_id, 0) - veh->moves_spent;
        if (remaining_moves > 0) {
            work_done += work_rate;
        }
        int remaining_work = total_work - work_done;
        int effective_rate = work_rate;
        if (effective_rate < 1) {
            effective_rate = 1;
        } else if (effective_rate > 255) {
            return (remaining_work + 254) / 255;
        }
        return (remaining_work + effective_rate - 1) / effective_rate;
    }

    veh_skip(veh_id);
    veh->movement_turns += work_rate;
    if (sq->base_who() >= 0) {
        veh->order = 0;
        return 0;
    }
    if (veh->movement_turns < total_work) {
        draw_tile(x, y, 2);
        BaseWin_check_veh(BaseWin, veh_id);
        return 0;
    }
    veh->movement_turns = 0;
    if (veh_id >= 0) {
        for (int cur_id = veh_top(veh_id); cur_id >= 0;) {
            VEH* cur = &Vehs[cur_id];
            if (cur->order == item_id + VehOrderFormerFirst) {
                cur->order = 0;
            }
            cur_id = cur->next_veh_id_stack;
        }
    }
    bool had_forest = sq->items & BIT_FOREST;
    bit_set(x, y, Terraform[item_id].bit_incompatible, 0);
    bit_set(x, y, Terraform[item_id].bit, 1);
    synch_bit(x, y, faction_id);

    switch (item_id) {
    case FORMER_REMOVE_FUNGUS:
    case FORMER_PLANT_FUNGUS:
        site_radius(x, y);
        break;
    case FORMER_CONDENSER:
    case FORMER_THERMAL_BORE:
        site_radius(x, y);
        world_climate();
        draw_map(1);
        break;
    case FORMER_AQUIFER:
        bit_set(x, y, BIT_RIVER_SRC, 1);
        world_climate();
        draw_map(1);
        break;
    case FORMER_RAISE_LAND:
        world_raise_alt(x, y);
        world_climate();
        draw_map(1);
        break;
    case FORMER_LOWER_LAND:
        world_lower_alt(x, y);
        world_climate();
        draw_map(1);
        break;
    case FORMER_LEVEL_TERRAIN:
        if (sq->rocky_level()) {
            rocky_set(x, y, sq->rocky_level() - 1);
            site_radius(x, y);
        }
        break;
    default:
        break;
    }

    if (Terraform[item_id].bit & (BIT_THERMAL_BORE|BIT_FOREST|BIT_FARM|BIT_SOLAR|BIT_MINE)) {
        int near_base_id = base_find_2(x, y, faction_id);
        if (near_base_id >= 0 && *BaseFindDist <= 2) {
            Bases[near_base_id].state_flags |= BSTATE_UNK_100;
        }
        if (near_base_id >= 0 && item_id == FORMER_THERMAL_BORE && faction_id == player_id) {
            parse_says(0, Bases[near_base_id].name, -1, -1);
            NetMsg_pop(NetMsg, "BOREHOLEMINE", 5000, 0, 0);
        }
    }
    bool lost_forest = had_forest && !(sq->items & BIT_FOREST) && !is_ocean(sq);
    if (lost_forest && item_id != FORMER_RAISE_LAND) {
        int base_id = veh->home_base_id;
        if (base_id < 0) {
            base_id = base_find_2(x, y, faction_id);
        }
        if (base_id >= 0) {
            BASE* b = &Bases[base_id];
            int new_minerals = Rules->minerals_harvesting_forest + b->minerals_accumulated;
            b->minerals_accumulated = new_minerals;
            b->minerals_accumulated_2 = new_minerals;
            b->production_id_last = b->queue_items[0];
            if (faction_id == player_id && is_human(faction_id) && Rules->minerals_harvesting_forest) {
                parse_says(0, b->name, -1, -1);
                parse_num(0, Rules->minerals_harvesting_forest);
                NetMsg_pop(NetMsg, "HARVESTFOREST", 5000, 0, 0);
            }
            BaseWin_check_base(BaseWin, base_id);
        }
    }
    /*
    Modify TERRAYOURS messages to only be displayed when they occur at most five tile distance
    from any friendly base. This also removes unintuitive excessive friction and treaty penalties
    when another faction alters rainfall patterns during terraforming.
    */
    if (item_id == FORMER_RAISE_LAND || item_id == FORMER_LOWER_LAND || item_id == FORMER_CONDENSER) {
        if (*dword_9B22E0 >= 0) {
            parse_says(0, Bases[*dword_9B22E0].name, -1, -1);
            parse_says(1, MFactions[faction_id].adj_name_faction, -1, -1);
            if (faction_id == player_id) {
                NetMsg_pop(NetMsg, "TERRAMINE", 5000, 0, 0);
            } else if (base_find_2(x, y, faction_id) >= 0 && *BaseFindDist < 6) {
                NetMsg_pop(NetMsg, "TERRAYOURS", 5000, 0, 0);
            }
        }
        if (MapBaseIdClosestSubmergedVeh[player_id] >= 0) {
            parse_says(0, Bases[MapBaseIdClosestSubmergedVeh[faction_id]].name, -1, -1);
            parse_says(1, MFactions[faction_id].adj_name_faction, -1, -1);
            if (faction_id == player_id) {
                NetMsg_pop(NetMsg, "DROWNMINE", 5000, 0, 0);
            } else {
                NetMsg_pop(NetMsg, "DROWNYOURS", 5000, 0, 0);
            }
        }
    }
    synch_bit(x, y, faction_id);
    draw_tiles(x, y, 2);
    if (initial_veh_count == *VehCount) {
        BaseWin_check_veh(BaseWin, veh_id);
        BaseWin_check_loc(BaseWin, x, y, 2);
    } else {
        GraphicWin_redraw(BaseWin);
    }
    if (faction_id == player_id && veh->state & VSTATE_ON_ALERT) {
        if (*GamePreferences & PREF_BSC_TUTORIAL_MSGS && !*MultiplayerActive) {
            int item_bit = 1 << item_id;
            if (!(TutWinMapState[ocean] & item_bit)) {
                TutWinMapState[ocean] |= item_bit;
                if (ocean) {
                    switch (item_id) {
                    case 0:
                        TutWin_tut_map(TutWin, "MADESEAFARM", x, y, -1, ImgMapFarm, 1, -1, -1);
                        break;
                    case 2:
                        TutWin_tut_map(TutWin, "MADESEAMINE", x, y, -1, ImgMapMine, 1, -1, -1);
                        break;
                    case 3:
                        TutWin_tut_map(TutWin, "MADESEASOLAR", x, y, -1, ImgMapSolar, 1, -1, -1);
                        break;
                    default:
                        TutWinMapState[ocean] &= ~item_bit;
                        break;
                    }
                } else {
                    switch (item_id) {
                    case 0:
                        TutWin_tut_map(TutWin, "MADEFARM", x, y, -1, ImgMapFarm, 1, -1, -1);
                        break;
                    case 1:
                        TutWin_tut_map(TutWin, "MADESOIL", x, y, -1, ImgMapSoilEnr, 1, -1, -1);
                        break;
                    case 2:
                        TutWin_tut_map(TutWin, "MADEMINE", x, y, -1, ImgMapMine, 1, -1, -1);
                        break;
                    case 3:
                        TutWin_tut_map(TutWin, "MADESOLAR", x, y, -1, ImgMapSolar, 1, -1, -1);
                        break;
                    case 4:
                        TutWin_tut_map(TutWin, "MADEFOREST", x, y, veh_id, nullptr, 1, -1, -1);
                        break;
                    case 5:
                        TutWin_tut_map(TutWin, "MADEROAD", x, y, veh_id, nullptr, 1, -1, -1);
                        break;
                    case 6:
                        TutWin_tut_map(TutWin, "MADETUBE", x, y, veh_id, nullptr, 1, -1, -1);
                        break;
                    case 9:
                        TutWin_tut_map(TutWin, "MADESENSOR", x, y, veh_id, nullptr, 1, -1, -1);
                        break;
                    case 12:
                        TutWin_tut_map(TutWin, "MADECONDENSER", x, y, veh_id, nullptr, 1, -1, -1);
                        break;
                    case 14:
                        TutWin_tut_map(TutWin, "MADEBOREHOLE", x, y, veh_id, nullptr, 1, -1, -1);
                        break;
                    case 15:
                        TutWin_tut_map(TutWin, "MADEAQUIFER", x, y, veh_id, nullptr, 1, -1, -1);
                        break;
                    default:
                        TutWinMapState[ocean] &= ~item_bit;
                        break;
                    }
                }
            }
        }
        if (veh->order == ORDER_NONE && !(veh->state & VSTATE_ON_ALERT) && !*dword_90DB84) {
            *dword_90DB84 = 1;
            wave_it(20);
            ambience(207);
        }
    }
    return 1;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-pointer="

void __cdecl action_go_to(net_int_t veh_id) {
    if (*MultiplayerActive && !*ControlTurnC) {
        *dword_93E908 = &veh_id;
    }
    int move_dir;
    if (Vehs[veh_id].order == ORDER_MOVE) {
        move_dir = Vehs[veh_id].waypoint_x[0];
    } else {
        int target = (Vehs[veh_id].order & VehOrderResetFlag) ? -1 : Vehs[veh_id].faction_id;
        move_dir = Path_move(Paths, veh_id, target);
    }
    if (move_dir < 0) {
        Vehs[veh_id].order = ORDER_NONE;
        if (*MultiplayerActive && !*ControlTurnC) {
            synch_veh(veh_id);
            NetDaemon_await_synch(NetState);
            NetDaemon_unlock_veh(NetState);
        }
    } else if (!*MultiplayerActive || *ControlTurnC) {
        if (order_veh(veh_id, move_dir, 3)) {
            uint8_t order = Vehs[veh_id].order;
            if (order == ORDER_MOVE) {
                if (Vehs[veh_id].x == Vehs[veh_id].waypoint_x[1] && Vehs[veh_id].y == Vehs[veh_id].waypoint_y[1]) {
                    Vehs[veh_id].order = ORDER_NONE;
                }
            } else if (order == ORDER_MOVE_TO) {
                if (Vehs[veh_id].x == Vehs[veh_id].waypoint_x[0] && Vehs[veh_id].y == Vehs[veh_id].waypoint_y[0]) {
                    Vehs[veh_id].order = ORDER_NONE;
                    if ((Vehs[veh_id].state & (VSTATE_UNK_2000000 | VSTATE_ON_ALERT)) == (VSTATE_UNK_2000000 | VSTATE_ON_ALERT)) {
                        uint8_t num = Vehs[veh_id].waypoint_count;
                        if (Vehs[veh_id].patrol_current_point < num) {
                            for (int i = 0; i < num; i++) {
                                Vehs[veh_id].waypoint_x[i] = Vehs[veh_id].waypoint_x[i + 1];
                                Vehs[veh_id].waypoint_y[i] = Vehs[veh_id].waypoint_y[i + 1];
                            }
                            Vehs[veh_id].order = ORDER_MOVE_TO;
                            if (!--Vehs[veh_id].waypoint_count) {
                                Vehs[veh_id].state &= ~(VSTATE_UNK_2000000 | VSTATE_UNK_10000 | VSTATE_ON_ALERT);
                            }
                        }
                    }
                    if (!is_human(Vehs[veh_id].faction_id)) {
                        if (Chassis[Units[Vehs[veh_id].unit_id].chassis_id].cargo) {
                            MAP* sq = mapsq(Vehs[veh_id].x, Vehs[veh_id].y);
                            if (sq && sq->base_who() >= 0) {
                                veh_skip(veh_id);
                            }
                        }
                    }
                }
            }
        }
    } else if (NetDaemon_order_veh(NetState, veh_id, move_dir, 1)) {
        NetDaemon_await_exec(NetState, 1);
    }
}

void __cdecl action_road_to(net_int_t veh_id) {
    if (*MultiplayerActive && !*ControlTurnC) {
        *dword_93E908 = &veh_id;
    }
    int faction_id = Vehs[veh_id].faction_id;
    int dest_x = Vehs[veh_id].waypoint_x[0];
    int dest_y = Vehs[veh_id].waypoint_y[0];
    int x = Vehs[veh_id].x;
    int y = Vehs[veh_id].y;
    MAP* sq = mapsq(x, y);
    auto net_synch_wait = [](int sync_veh_id) {
        if (*MultiplayerActive && !*ControlTurnC) {
            synch_veh(sync_veh_id);
            NetDaemon_await_synch(NetState);
            NetDaemon_unlock_veh(NetState);
        }
    };

    if (is_ocean(sq) || !on_map(dest_x, dest_y)) {
        Vehs[veh_id].order = ORDER_NONE;
        net_synch_wait(veh_id);
        return;
    }
    if (sq->is_fungus()
    && !has_tech(Rules->tech_preq_build_road_fungus, faction_id)
    && has_tech(Terraform[FORMER_REMOVE_FUNGUS].preq_tech, faction_id)) {
        if (sq->base_who() < 0) {
            if (*MultiplayerActive && !*ControlTurnC) {
                message_data(0x2405, 0, veh_id, 14, Vehs[veh_id].order, 0);
                NetDaemon_await_exec(NetState, 1);
                return;
            }
            uint8_t saved_order = Vehs[veh_id].order;
            action_terraform(veh_id, FORMER_REMOVE_FUNGUS, 1);
            Vehs[veh_id].order = saved_order;
            return;
        }
    }
    if (sq->base_who() < 0 && (!sq->is_fungus()
    || has_tech(Rules->tech_preq_build_road_fungus, faction_id))) {
        if (!(sq->items & BIT_ROAD)) {
            if (*MultiplayerActive && !*ControlTurnC) {
                message_data(0x2405, 0, veh_id, 9, Vehs[veh_id].order, 0);
                NetDaemon_await_exec(NetState, 1);
                return;
            }
            uint8_t saved_order = Vehs[veh_id].order;
            action_terraform(veh_id, FORMER_ROAD, 1);
            Vehs[veh_id].order = saved_order;
            return;
        }
        if (Vehs[veh_id].order == ORDER_MAGTUBE_TO && !(sq->items & BIT_MAGTUBE)) {
            if (has_tech(Terraform[FORMER_MAGTUBE].preq_tech, faction_id)) {
                if (*MultiplayerActive && !*ControlTurnC) {
                    message_data(0x2405, 0, veh_id, 9, ORDER_MAGTUBE_TO, 0);
                    NetDaemon_await_exec(NetState, 1);
                    return;
                }
                action_terraform(veh_id, FORMER_ROAD, 1);
                Vehs[veh_id].order = ORDER_MAGTUBE_TO;
                return;
            }
        }
    }
    if (x == dest_x && y == dest_y) {
        uint8_t completed_order = Vehs[veh_id].order;
        Vehs[veh_id].order = ORDER_NONE;
        net_synch_wait(veh_id);
        if (!(Vehs[veh_id].state & VSTATE_EXPLORE) && faction_id == *CurrentPlayerFaction) {
            StrBuffer[0] = 0;
            say_loc(StrBuffer, dest_x, dest_y, 0, 2, 0);
            parse_says(0, StrBuffer, -1, -1);
            if (completed_order == ORDER_ROAD_TO) {
                NetMsg_pop(NetMsg, "ROADTODONE", 5000, 0, 0);
            } else {
                NetMsg_pop(NetMsg, "TUBETODONE", 5000, 0, 0);
            }
        }
        return;
    }
    int path_flags = has_tech(Rules->tech_preq_build_road_fungus, faction_id) ? 0 : 16;
    int dir = Path_find(Paths, x, y, dest_x, dest_y, 0, -1, path_flags, -1);
    if (dir >= 0 && dir < 8) {
        if (!*MultiplayerActive || *ControlTurnC) {
            order_veh(veh_id, dir, 3);
        } else if (NetDaemon_order_veh(NetState, veh_id, dir, 1)) {
            NetDaemon_await_exec(NetState, 1);
        }
        return;
    }
    Vehs[veh_id].order = ORDER_NONE;
    net_synch_wait(veh_id);
}

#pragma GCC diagnostic pop

void __cdecl action_destroy(int veh_id, int tgt_item, int tgt_x, int tgt_y) {
    const int player_id = *CurrentPlayerFaction;
    VEH* veh = &Vehs[veh_id];
    int faction_id = veh->faction_id;
    int tx = tgt_x;
    int ty = tgt_y;
    // Fix bug that occurs after an artillery attack on unoccupied tile and then selecting
    // another unit to view combat odds and cancel the attack. After this VehAttackFlags will not
    // get cleared and bombarding next unoccupied tile always destroys the bombarding unit.
    veh_skip(veh_id);
    *VehAttackFlags = 0;
    veh->state |= VSTATE_UNK_40;
    bool is_forest_destroy = false;
    bool is_bombard = false;
    bool do_combat = false;
    bool do_arty = false;

    if (!on_map(tx, ty)) {
        tx = veh->x;
        ty = veh->y;
        do_arty = true; // unusual case, should not happen
    } else {
        if (mapsq(tx, ty)->base_who() >= 0) {
            if (faction_id == player_id) {
                boom(tx, ty, 128);
            }
            return;
        }
        if ((game_rand() % 2) == 0) {
            do_combat = true;
            do_arty = true;
        }
    }
    if (do_combat) {
        is_bombard = true;
        bool skip_boom = false;
        if (faction_id != player_id) {
            int terr = whose_territory(player_id, tx, ty, 0, 0);
            if (terr != player_id) {
                skip_boom = true;
            }
        }
        if (!skip_boom) {
            if (faction_id && faction_id != player_id) {
                boom(tx, ty, 128);
                boom(veh->x, veh->y, 128);
            }
            boom(tx, ty, 128);
        }
        int owner = whose_territory(faction_id, tx, ty, 0, 0);
        if (owner != faction_id) {
            for (int i = 1; i < 25; i++) {
                int px = wrap(veh->x + TableOffsetX[i]);
                int py = veh->y + TableOffsetY[i];
                if (on_map(px, py) && map_range(veh->x, veh->y, px, py) <= 2) {
                    int stack_id = stack_fix(veh_at(px, py));
                    if (stack_id >= 0) {
                        if (mod_stack_check(stack_id, 15, owner, -1, -1)) {
                            mod_battle_fight(veh_id, i, 1, 1, 0);
                            return;
                        }
                    }
                }
            }
        }
    }
    if (do_arty) {
        int unit_id = veh->unit_id;
        if (Units[unit_id].triad() == TRIAD_AIR && !Units[unit_id].is_missile()) {
            if ((game_rand() % 4) == 0) {
                if (faction_id == player_id) {
                    NetMsg_pop(NetMsg, "BOMBFAILED", 5000, 0, 0);
                }
                order_veh(veh_id, -1, 3);
                return;
            }
        }
        int item_flags;
        if (tgt_item <= 0) {
            item_flags = bit_at(tx, ty);
        } else {
            item_flags = bit_at(tx, ty) & tgt_item;
            tgt_item = item_flags;
            if (item_flags == 0) {
                order_veh(veh_id, -1, 3);
                return;
            }
            bit_set(tx, ty, item_flags, 0);
        }
        if (!has_abil(unit_id, ABL_FUNGICIDAL)) {
            item_flags &= ~BIT_FUNGUS;
        }
        int tf_idx = -1;
        int tf_found;
        while (true) {
            tf_found = FORMER_SENSOR;
            if (tf_idx >= 0) {
                tf_found = tf_idx;
            }
            if (item_flags & Terraform[tf_found].bit) {
                break;
            }
            if (++tf_idx >= 19) {
                debug("action_destroy_fail %08X %08X\n", tgt_item, item_flags);
                if (is_bombard) {
                    if (faction_id == player_id) {
                        NetMsg_pop(NetMsg, "ARTYBOMBFAILED", 5000, 0, 0);
                    }
                } else if (Units[unit_id].triad() == TRIAD_AIR && faction_id == player_id) {
                    NetMsg_pop(NetMsg, "BOMBFAILED", 5000, 0, 0);
                }
                order_veh(veh_id, -1, 3);
                return;
            }
        }
        int tf_bit = Terraform[tf_found].bit | tgt_item;
        bit_set(tx, ty, Terraform[tf_found].bit, 0);
        bool is_sea = is_ocean(mapsq(tx, ty));
        char* tf_name = is_sea ? Terraform[tf_found].name_sea : Terraform[tf_found].name;
        parse_says(2, tf_name, -1, -1);
        if (tf_found == FORMER_FOREST) {
            is_forest_destroy = 1;
        }
        if (Units[unit_id].triad() == TRIAD_AIR && faction_id == player_id) {
            NetMsg_pop(NetMsg, "BOMBSUCCEEDED", 5000, 0, 0);
        }
        synch_bit(tx, ty, faction_id);
        int owner = whose_territory(faction_id ? faction_id : player_id, tx, ty, 0, 0);
        synch_bit(tx, ty, owner);

        if (tf_bit & (BIT_THERMAL_BORE | BIT_ECH_MIRROR | BIT_CONDENSER)) {
            world_climate();
            draw_map(1);
        } else if (!(tf_bit & BIT_SENSOR) || owner != player_id) {
            draw_tiles(tx, ty, 2);
        } else {
            draw_radius(tx, ty, 2, 2);
        }
        if (faction_id) {
            if (is_bombard && faction_id == player_id) {
                NetMsg_pop(NetMsg, "BOMBARDSUCCEEDED", 5000, 0, 0);
            }
            if (owner >= 0 && owner != faction_id
            && Factions[faction_id].diplo_status[owner] & DIPLO_COMMLINK && is_known(tx, ty, owner)) {
                synch_bit(tx, ty, owner);
                add_goal(owner, AI_GOAL_ATTACK, 5, veh->x, veh->y, -1);
                if (owner == player_id) {
                    int base_id = mod_base_find3(tx, ty, -1, -1, -1, player_id);
                    *GenderDefault = MFactions[faction_id].noun_gender;
                    *PluralDefault = MFactions[faction_id].is_noun_plural;
                    parse_says(0, MFactions[faction_id].noun_faction, -1, -1);
                    parse_says(1, base_id >= 0 ? Bases[base_id].name : "", -1, -1);
                    parse_says(3, MFactions[faction_id].adj_name_faction, -1, -1);
                    const char* msg = is_bombard ? "HAVEDESTROYED1" : "HAVEDESTROYED";
                    NetMsg_pop(NetMsg, msg, 5000, 0, 0);
                }
            }
        } else {
            if (is_known(tx, ty, player_id) && owner == player_id) {
                int base_id = mod_base_find3(tx, ty, -1, -1, -1, player_id);
                if (base_id >= 0 && Bases[base_id].faction_id == player_id) {
                    parse_says(0, Bases[base_id].name, -1, -1);
                    parse_says(1, Units[unit_id].name, -1, -1);
                    Console_focus(MapWin, tx, ty, 0);
                    if (is_forest_destroy) {
                        if (unit_id == BSC_SPORE_LAUNCHER) {
                            popp(ScriptFile, "SPOREFOREST", 0, "sporlnch_sm.pcx", 0);
                        } else {
                            NetMsg_pop(NetMsg, "ATEFOREST", 5000, 0, 0);
                        }
                    } else if (unit_id == BSC_SPORE_LAUNCHER) {
                        popp(ScriptFile, "SPORESLAUNCHED", 0, "sporlnch_sm.pcx", 0);
                    } else {
                        NetMsg_pop(NetMsg, "ATESTUFF", 5000, 0, 0);
                    }
                }
            }
        }
        order_veh(veh_id, -1, 3);
        return;
    }
    if (faction_id == player_id) {
        NetMsg_pop(NetMsg, "ARTYBOMBFAILED", 5000, 0, 0);
    }
}

void __cdecl action_arty(net_int_t veh_id, int tx, int ty) {
    VEH* veh = &Vehs[veh_id];
    if (veh->faction_id != *CurrentPlayerFaction || veh_ready(veh_id)) {
        const int range = arty_range(veh->unit_id);
        if (map_range(veh->x, veh->y, tx, ty) <= range) {
            int tgt_veh_id = stack_fix(veh_at(tx, ty));
            if (tgt_veh_id >= 0) {
                VEH* tgt = &Vehs[tgt_veh_id];
                if (veh->faction_id != tgt->faction_id
                && !(Factions[veh->faction_id].diplo_status[tgt->faction_id] & DIPLO_PACT)) {
                    int offset = radius_move_2(veh->x, veh->y, tx, ty, TableRange[range]);
                    if (offset >= 0) {
                        if (*MultiplayerActive) {
                            if (!NetDaemon_maybe_lock(NetState, &veh_id, 0, 0, tx, ty, 4)) {
                                *VehAttackFlags = 1;
                                if (mod_battle_fight(veh_id, offset, 1, 1, 0)) {
                                    NetDaemon_unlock_veh(NetState);
                                } else {
                                    message_veh(0x2402, veh_id, offset, 0);
                                    NetDaemon_await_exec(NetState, 1);
                                }
                            }
                        } else {
                            *VehAttackFlags = 3;
                            mod_battle_fight(veh_id, offset, 1, 1, 0);
                        }
                    }
                }
            } else if (*MultiplayerActive) {
                if (!NetDaemon_lock_veh(NetState, &veh_id, 4, tx, ty, 0)) {
                    message_data(0x2413, 0, veh_id, tx, ty, 0);
                    NetDaemon_await_exec(NetState, 1);
                }
            } else {
                action_destroy(veh_id, 0, tx, ty);
            }
        } else {
            NetMsg_pop(NetMsg, "OUTOFRANGE", 5000, 0, 0);
        }
    } else {
        NetMsg_pop(NetMsg, "UNITMOVED", 5000, 0, 0);
    }
}

void __cdecl action_destruct(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    int vx = veh->x;
    int vy = veh->y;
    boom(vx, vy, 0);
    int weapon_val = weap_val(veh->unit_id, veh->faction_id);
    int blast_damage = clamp(weapon_val, 1, 20) * Units[veh->unit_id].reactor_id / 2;
    kill(veh_id);

    for (int i = 0; i < 9; i++) {
        int tx = wrap(vx + TableOffsetX[i]);
        int ty = vy + TableOffsetY[i];
        if (on_map(tx, ty) && base_at(tx, ty) < 0) {
            for (int n = veh_at(tx, ty); n >= 0; n = Vehs[n].next_veh_id_stack) {
                Vehs[n].damage_taken += blast_damage;
            }
            int stack_top;
            while ((stack_top = stack_fix(veh_at(tx, ty))) >= 0) {
                int dead_veh = -1;
                for (int n = stack_top; n >= 0; n = Vehs[n].next_veh_id_stack) {
                    if (Vehs[n].damage_taken >= Vehs[n].max_hitpoints()) {
                        dead_veh = n;
                        break;
                    }
                }
                if (dead_veh >= 0) {
                    stack_veh(dead_veh, 1);
                    stack_kill(dead_veh);
                } else {
                    break;
                }
            }
        }
    }
}

void __cdecl action_oblit(int veh_id, int base_id) {
    const int player_id = *CurrentPlayerFaction;
    VEH* veh = &Vehs[veh_id];
    BASE* base = &Bases[base_id];
    int faction_id = veh->faction_id;
    int faction_id_fmr = base->faction_id_former;
    parse_num(0, 10 * base->pop_size);
    parse_says(0, base->name, -1, -1);
    *PluralDefault = 0;
    *GenderDefault = MFactions[player_id].is_leader_female;
    parse_says(1, MFactions[player_id].title_leader, -1, -1);
    parse_says(2, MFactions[faction_id_fmr].adj_name_faction, -1, -1);
    *PluralDefault = 0;
    *GenderDefault = MFactions[faction_id].is_leader_female;
    parse_says(3, MFactions[faction_id].title_leader, -1, -1);
    parse_says(4, MFactions[faction_id].name_leader, -1, -1);

    bool is_visible = (base->faction_id == player_id) || (base->visibility & (1 << player_id));
    mod_base_kill(base_id);
    draw_map(1);

    if (faction_id == player_id) {
        popp(ScriptFile, "OBLITTED", 0, "baseobl_sm.pcx", 0);
    } else if (faction_id_fmr == player_id || is_visible) {
        popp(ScriptFile, "OBLITTED2", 0, "baseobl_sm.pcx", 0);
    }
    if (Rules->tgl_oblit_base_atrocity) {
        atrocity(faction_id, faction_id_fmr, 2, 0);
    }
}


