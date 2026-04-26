
#include "veh_action.h"


int __cdecl terrain_avail(FormerItem frm_id, int ocean, int faction_id) {
    assert(frm_id >= FORMER_FARM && frm_id <= FORMER_MONOLITH);
    int preq_tech = (ocean ? Terraform[frm_id].preq_tech_sea : Terraform[frm_id].preq_tech);
    if (preq_tech < TECH_None || (*GameRules & RULES_SCN_NO_TERRAFORMING
    && (frm_id == FORMER_RAISE_LAND || frm_id == FORMER_LOWER_LAND))) {
        return false;
    }
    if (frm_id >= FORMER_CONDENSER && frm_id <= FORMER_LEVEL_TERRAIN
    && has_project(FAC_WEATHER_PARADIGM, faction_id)) {
        return true;
    }
    return has_tech(preq_tech, faction_id);
}

int __cdecl contribution(int veh_id, int frm_id) {
    assert(frm_id >= FORMER_FARM && frm_id <= FORMER_MONOLITH);
    int value = has_abil(Vehs[veh_id].unit_id, ABL_SUPER_TERRAFORMER) ? 4 : 2;
    if (frm_id == FORMER_REMOVE_FUNGUS || frm_id == FORMER_PLANT_FUNGUS) {
        if (has_project(FAC_XENOEMPATHY_DOME, Vehs[veh_id].faction_id)) {
            value *= 2;
        }
    } else if (has_project(FAC_WEATHER_PARADIGM, Vehs[veh_id].faction_id)) {
        value = (value * 3) / 2;
    }
    return value;
}

int __cdecl terraform_cost(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        assert(0);
        return 0;
    }
    int alt = sq->alt_level();
    int alt_dist = abs(alt - ALT_SHORE_LINE);
    int cost = (alt_dist + 2) * (alt_dist + 2) * (sq->is_fungus() ? 6 : 2);
    if (alt < ALT_SHORE_LINE) {
        cost *= (!TechOwners[TECH_DocAir] ? 4 : 2);
    }
    int own_base_id = base_find_2(x, y, faction_id);
    int enemy_base_id = base_find_3(x, y, -1, -1, faction_id, -1);

    if (own_base_id >= 0) {
        BASE* own = &Bases[own_base_id];
        cost *= clamp(map_range(x, y, own->x, own->y), 1, 100);
        if (enemy_base_id >= 0) {
            BASE* enemy = &Bases[enemy_base_id];
            if (!(Factions[faction_id].diplo_status[enemy->faction_id] & DIPLO_PACT)) {
                int enemy_val = (enemy->pop_size + 2) * map_range(x, y, own->x, own->y) / 3;
                int own_val   = (own->pop_size   + 2) * map_range(x, y, enemy->x, enemy->y) / 3;
                if (own_val > 0 && enemy_val > 0 && own_val < enemy_val) {
                    cost = cost * enemy_val / own_val;
                }
            }
        }
    }
    if (MFactions[faction_id].rule_flags & RFLAG_TERRAFORM) {
        cost /= 2;
    }
    return clamp(cost / 2, 1, 30000);
}

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

    // Fix: display notification and exit if the base limit is reached
    if (*BaseCount >= MaxBaseNum) {
        if (faction_id == player_id) {
            NetMsg_pop(NetMsg, "BASELIMIT", 5000, 0, 0);
        }
        return -1;
    }
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
            } else if (base_find_2(x, y, player_id) >= 0 && *BaseFindDist < 6) {
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
                    int base_id = base_find_3(tx, ty, -1, -1, -1, player_id);
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
                int base_id = base_find_3(tx, ty, -1, -1, -1, player_id);
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

/*
Move the unit to the closest friendly base, allied airbase,
or allied carrier based on the triad and current state.
Return value 1 if valid move order issued, 0 if no valid destination found.

flag is bitmask/integer determining the strictness of the return command.
0 : Default. Finds the best route and sends the unit home.
1 : Fuel shortage check. It will only be ordered home if it is
    very low on fuel and must return this turn. Otherwise, returns 0.
2 : Ignore vulnerable bases. Functions like 0, but ignores bases flagged
    with BSTATE_UNK_200000 (was hit by artillery attack).
4 : Strict range. The function will abort (return 0) if the best
    destination cannot be reached in the current turn.
*/
int __cdecl action_home(int veh_id, int flag) {
    VEH* const veh = &Vehs[veh_id];
    const int faction_id = veh->faction_id;
    const int vx = veh->x;
    const int vy = veh->y;
    const int veh_region = region_at(veh->x, veh->y);
    const int veh_range = veh->range();
    const int moves_left = veh_speed(veh_id, 0) - veh->moves_spent;
    int total_moves = clamp(moves_left, 0, 999) + (veh_range - veh->movement_turns - 1) * veh_speed(veh_id, 0);
    if (veh_range == 1) {
        total_moves += (veh->cur_hitpoints() - 1) / veh->reactor_type() * veh_speed(veh_id, 0);
    }
    int tile_moves = total_moves / Rules->move_rate_roads;
    int best_dist = 9999;
    int best_x = -1;
    int best_y = -1;

    bool search_veh = true;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id != faction_id && !(Factions[faction_id].diplo_status[base->faction_id] & DIPLO_PACT)) {
            continue;
        }
        if (veh->triad() == TRIAD_LAND && region_at(base->x, base->y) != veh_region) {
            continue;
        }
        if (veh->triad() == TRIAD_SEA && (base->x != vx || base->y != vy) && !base_on_sea(i, veh_region)) {
            continue;
        }
        if (flag == 2 && base->state_flags & BSTATE_UNK_200000) {
            continue;
        }
        int dist = map_range(base->x, base->y, vx, vy);
        if ((veh->state & VSTATE_ON_ALERT) && veh->order_auto_type == ORDERA_ON_ALERT
        && base->x == veh->waypoint_x[1] && base->y == veh->waypoint_y[1]
        && dist <= tile_moves) {
            best_dist = dist;
            best_x = base->x;
            best_y = base->y;
            search_veh = false;
            break;
        }
        if (dist < best_dist) {
            best_dist = dist;
            best_x = base->x;
            best_y = base->y;
        }
    }
    int max_offset = 0;
    if (search_veh && veh->triad() == TRIAD_AIR) {
        for (int i = 0; i < *VehCount; i++) {
            VEH* cur = &Vehs[i];
            if (cur->faction_id == faction_id && i != veh_id) {
                if (has_abil(cur->unit_id, ABL_CARRIER) || (cur->plan() == PLAN_SUPPLY && cur->triad() == TRIAD_AIR)) {
                    int orig_dist = map_range(cur->x, cur->y, vx, vy);
                    int dist = orig_dist;
                    bool skip = !(flag & 1) && flag != 4;
                    if (!skip && !is_human(faction_id)) {
                        dist += veh_speed(i, 0) / Rules->move_rate_roads;
                    }
                    if (skip || cur->plan() != PLAN_SUPPLY
                    || orig_dist <= clamp(moves_left, 0, 999) / Rules->move_rate_roads) {
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_x = cur->x;
                            best_y = cur->y;
                        }
                    }
                }
            }
        }
        max_offset = TableRange[min(8, best_dist)];
        for (int n = 0; n < max_offset; n++) {
            int px = wrap(vx + TableOffsetX[n]);
            int py = vy + TableOffsetY[n];
            MAP* psq = mapsq(px, py);
            if (!psq || !(psq->items & BIT_AIRBASE)) {
                continue;
            }
            int owner = psq->veh_who();
            if (owner >= 0 && owner != faction_id) {
                if (!(Factions[faction_id].diplo_status[owner] & DIPLO_PACT)) {
                    continue;
                }
            }
            if (is_human(faction_id) || (whose_territory(faction_id, px, py, 0, 0) == faction_id
            && (owner < 0 || owner == faction_id || (Factions[faction_id].diplo_status[owner] & DIPLO_PACT)))) {
                int dist = map_range(vx, vy, px, py);
                if ((veh->state & VSTATE_ON_ALERT) && veh->order_auto_type == ORDERA_ON_ALERT
                && px == veh->waypoint_x[1] && py == veh->waypoint_y[1]
                && dist <= tile_moves && !veh->damage_taken) {
                    best_x = px;
                    best_y = py;
                    break;
                }
                if (dist < best_dist) {
                    best_dist = dist;
                    best_x = px;
                    best_y = py;
                }
            }
        }
    }
    if (!veh_range && ((flag & 1) || flag == 4)) {
        return 0;
    }
    if (!on_map(best_x, best_y)) {
        return 0;
    }
    if (flag & 1 && best_dist + 1 <= tile_moves - 1) {
        return 0;
    }
    if (flag >= 0) {
        if (flag == 4 && best_dist > tile_moves) {
            return 0;
        }
        if (best_x != vx || best_y != vy) {
            veh->order = ORDER_MOVE_TO;
            veh->waypoint_x[0] = best_x;
            veh->waypoint_y[0] = best_y;
            if (veh_range && is_human(faction_id) && best_dist > tile_moves) {
                veh->order = ORDER_NONE;
                veh->state &= ~(VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
            }
        } else {
            veh->order = ORDER_NONE;
            if (!is_human(faction_id)) {
                veh_skip(veh_id);
                return 1;
            }
        }
    }
    return 1;
}

int __cdecl action_airdrop(int veh_id, int tx, int ty, int flags) {
    const int airdef_range = TableRange[clamp(AerospaceDefenseRange, 0, MaxTableRange)];
    const int player_id = *CurrentPlayerFaction;
    VEH* const veh = &Vehs[veh_id];
    const int faction_id = veh->faction_id;
    const char* image_drop = MFactions[faction_id].is_alien() ? "al_drop_sm.pcx" : "airdp_sm.pcx";
    Faction* const plr = &Factions[faction_id];
    *VehAttackFlags = flags;

    if (map_range(veh->x, veh->y, tx, ty) > drop_range(faction_id)) {
        if ((*VehAttackFlags & 1) && faction_id == player_id) {
            parse_num(0, Rules->max_airdrop_rng_wo_orbital_insert);
            NetMsg_pop(NetMsg, "DROPRANGE", -5000, 0, image_drop);
        }
        return 0;
    }
    MAP* sq = mapsq(tx, ty);
    if (is_ocean(sq)) {
        if ((*VehAttackFlags & 1) && faction_id == player_id) {
            NetMsg_pop(NetMsg, "DROPSEA", -5000, 0, image_drop);
        }
        return 0;
    }
    int veh_owner = sq->veh_who();
    if (veh_owner >= 0 && veh_owner != faction_id && !(plr->diplo_status[veh_owner] & DIPLO_PACT)) {
        if ((*VehAttackFlags & 1) && faction_id == player_id) {
            NetMsg_pop(NetMsg, "DROPUNIT", -5000, 0, image_drop);
        }
        return 0;
    }
    if (!Units[veh->unit_id].is_combat_unit() && zoc_move(tx, ty, faction_id)) {
        if ((*VehAttackFlags & 1) && faction_id == player_id) {
            NetMsg_pop(NetMsg, "NONCOMDROP", -5000, 0, image_drop);
        }
        return 0;
    }
    int base_owner = sq->base_who();
    if (base_owner >= 0 && base_owner != faction_id && !(plr->diplo_status[base_owner] & DIPLO_PACT)) {
        if (!Units[veh->unit_id].is_combat_unit()) {
            if ((*VehAttackFlags & 1) && faction_id == player_id) {
                NetMsg_pop(NetMsg, "NONCOMDROP", -5000, 0, image_drop);
            }
            return 0;
        }
        if ((*VehAttackFlags & 1) && faction_id == player_id
        && break_treaty(faction_id, base_owner, DIPLO_PACT|DIPLO_TREATY|DIPLO_TRUCE)) {
            return 0;
        }
    }
    // Fix: previously this skipped owner=0 bases
    for (int i = 0; i < airdef_range; i++) {
        int px = wrap(tx + TableOffsetX[i]);
        int py = ty + TableOffsetY[i];
        MAP* psq = mapsq(px, py);
        if (psq) {
            int owner = psq->base_who();
            if (owner >= 0 && owner != faction_id
            && (Factions[faction_id].diplo_status[owner] & DIPLO_VENDETTA)) {
                int base_id = base_at(px, py);
                if (base_id >= 0) {
                    if (has_facility(FAC_AEROSPACE_COMPLEX, base_id)
                    || mod_stack_check(veh_at(px, py), 2, PLAN_AIR_SUPERIORITY, -1, -1)) {
                        if ((*VehAttackFlags & 1) && faction_id == player_id) {
                            parse_says(0, Bases[base_id].name, -1, -1);
                            NetMsg_pop(NetMsg, "AIRDEFENSES", -5000, 0,
                                MFactions[faction_id].is_alien() ? "alair_sm.pcx" : "air_sm.pcx");
                        }
                        return 0;
                    }
                }
            }
        }
    }
    if (!(*VehAttackFlags & 2)) {
        return 1;
    }
    int goody_val = goody_at(tx, ty);
    bool is_visible = false;
    if (faction_id == player_id || (*GameState & STATE_OMNISCIENT_VIEW)
    || sq->veh_who() == player_id || sq->base_who() == player_id) {
        is_visible = true;
    } else {
        for (int i = 0; i < 8; i++) {
            int px = wrap(tx + BaseOffsetX[i]);
            int py = ty + BaseOffsetY[i];
            MAP* psq = mapsq(px, py);
            if (psq && (psq->veh_who() == player_id || psq->base_who() == player_id)) {
                is_visible = true;
                break;
            }
        }
    }
    bool no_quick_move = false;
    if (faction_id == player_id
    || ((plr->diplo_status[player_id] & DIPLO_PACT) ?
    (*GamePreferences & PREF_BSC_DONT_QUICK_MOVE_ALLY_VEH) :
    (*GamePreferences & PREF_BSC_DONT_QUICK_MOVE_ENEMY_VEH))) {
        no_quick_move = true;
    }
    int best_score = 999999;
    int best_offset = -1;
    for (int i = 0; i < 8; i++) {
        int px = wrap(tx + BaseOffsetX[i]);
        int py = ty + BaseOffsetY[i];
        MAP* psq = mapsq(px, py);
        if (psq) {
            int score = game_rand() % 10 + 50;
            if (i >= 6 || i <= 0) { score -= 25; }
            if (i == 5 || i == 1) { score -= 10; }
            int owner = psq->veh_who();
            if (owner < 0) {
                owner = psq->base_who();
            }
            if (owner >= 0) {
                score += 100;
                if (owner != faction_id) {
                    score += 1000;
                    if (!(plr->diplo_status[owner] & DIPLO_PACT)) {
                        score += 10000;
                    }
                }
            }
            if (score < best_score) {
                best_score = score;
                best_offset = i;
            }
        }
    }
    if (best_offset < 0) {
        return 0;
    }
    if (is_visible && faction_id != player_id) {
        int base_id = base_find_3(tx, ty, -1, -1, -1, 1);
        if (base_id >= 0) {
            parse_says(0, Bases[base_id].name, -1, -1);
        }
        *GenderDefault = MFactions[faction_id].noun_gender;
        *PluralDefault = MFactions[faction_id].is_noun_plural;
        parse_says(1, MFactions[faction_id].noun_faction, -1, -1);
        if (drop_range(faction_id) >= Rules->max_airdrop_rng_wo_orbital_insert) {
            popp(ScriptFile, "MADEORBITAL", 0, image_drop, 0);
        } else {
            popp(ScriptFile, "MADEAIRDROP", 0, image_drop, 0);
        }
    }
    veh->state |= VSTATE_MADE_AIRDROP;
    if (!Units[veh->unit_id].is_combat_unit()) {
        veh_skip(veh_id);
    }
    int old_x = veh->x;
    int old_y = veh->y;
    int drop_x = wrap(tx + BaseOffsetX[best_offset]);
    int drop_y = ty + BaseOffsetY[best_offset];
    stack_veh(veh_id, 1);
    draw_tile(old_x, old_y, 2);
    if (is_visible && no_quick_move) {
        veh_scoot(veh_id, drop_x, drop_y, best_offset ^ 4, 0);
    }
    unspot_stack(veh_id);
    for (int v = veh_top(veh_id); v >= 0; v = Vehs[v].next_veh_id_stack) {
        if (v != veh_id) { veh_skip(v); }
    }
    if (veh_id == MapWin->iUnit && MapWin->fUnitNotViewMode) {
        Console_cursor_next(MapWin, tx, ty);
    }
    stack_put(veh_id, tx, ty);
    int base_id = base_at(tx, ty);
    if (base_id < 0) {
        veh->damage_taken = min(veh->max_hitpoints() - 1,
            veh->damage_taken + 2 * Units[veh->unit_id].reactor_id);
    } else if (faction_id != Bases[base_id].faction_id
    && !(plr->diplo_status[Bases[base_id].faction_id] & DIPLO_PACT)) {
        if (!is_human(faction_id)) {
            veh_skip(veh_id);
        }
        *CurrentVehID = veh_id;
        veh->damage_taken = 0;
        act_of_aggression(faction_id, Bases[base_id].faction_id);
        mod_capture_base(base_id, faction_id, 0);
        veh_id = *CurrentVehID;
    }
    spot_all(veh_id, 1);
    Console_update_data(MapWin, 0);
    if (goody_val) {
        mod_goody_box(veh_id);
    }
    if (faction_id != player_id && !(*GameState & STATE_OMNISCIENT_VIEW)
    && is_visible && !MapWin->field_23BF0 && !shift_key_down() && !*MultiplayerActive) {
        clock_wait(no_quick_move ? 200 : 50);
    }
    return 1;
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

int __cdecl valid_patrol(int veh_id, int tx, int ty) {
    VEH* const veh = &Vehs[veh_id];
    MAP* const veh_sq = mapsq(veh->x, veh->y);
    MAP* const tgt_sq = mapsq(tx, ty);
    if ((tx == veh->x && ty == veh->y) || !tgt_sq || !veh_sq) {
        return 0;
    }
    int triad = veh->triad();
    if (triad == TRIAD_LAND) {
        if (tgt_sq->region != veh_sq->region) {
            return 0;
        }
        return 1;
    }
    if (triad == TRIAD_SEA) {
        int home_base = base_at(veh->x, veh->y);
        int tgt_base_id = base_at(tx, ty);

        if (home_base < 0) {
            if (tgt_base_id < 0) {
                if (tgt_sq->region != veh_sq->region) {
                    return 0;
                }
                return 1;
            }
            if (!base_on_sea(tgt_base_id, veh_sq->region)) {
                return 0;
            }
        } else if (tgt_base_id < 0) {
            if (!base_on_sea(home_base, tgt_sq->region)) {
                return 0;
            }
        } else {
            if (!port_to_port(home_base, tgt_base_id)) {
                return 0;
            }
        }
    } else if (triad == TRIAD_AIR && veh->range()) {
        // Fix: replace multiple incorrect checks for base_at and bool flags
        bool has_home_airbase = (base_at(veh->x, veh->y) >= 0 || (veh_sq->items & BIT_AIRBASE));
        bool has_tgt_airbase = (base_at(tx, ty) >= 0 || (tgt_sq->items & BIT_AIRBASE));
        if (!has_home_airbase && !has_tgt_airbase) {
            return 0;
        }
        int moves_left = veh_speed(veh_id, 0) - veh->moves_spent;
        int total_range = (clamp(moves_left, 0, 999)
            + (veh->range() - veh->movement_turns - 1) * veh_speed(veh_id, 0)) / Rules->move_rate_roads;
        int dist = map_range(tx, ty, veh->x, veh->y);
        if (has_home_airbase && has_tgt_airbase) {
            if (dist > total_range) {
                return 0;
            }
        } else if (dist > total_range / 2) {
            return 0;
        }
    }
    return 1;
}

int __cdecl action_patrol(int veh_id, int tx, int ty) {
    // Fix 5. [BUG] Setting more than one patrol waypoint with spacebar causes the values to be stored incorrectly.
    int result = valid_patrol(veh_id, tx, ty);
    if (result) {
        VEH* veh = &Vehs[veh_id];
        veh->order = ORDER_MOVE_TO;
        veh->waypoint_count = clamp(veh->waypoint_count - 1, 0, 2);
        veh->waypoint_x[veh->waypoint_count] = tx;
        veh->waypoint_y[veh->waypoint_count] = ty;
        veh->waypoint_count++;
        veh->waypoint_x[veh->waypoint_count] = veh->x;
        veh->waypoint_y[veh->waypoint_count] = veh->y;
        veh->state |= (VSTATE_ON_ALERT | VSTATE_UNK_1000000);
        veh->patrol_current_point = 0;
    }
    return result;
}

int  __cdecl shoot_it(int faction_id_atk, int faction_id_def, int tx, int ty, int flag) {
    const int flec_range = TableRange[clamp(FlechetteDefenseRange, 0, MaxTableRange)];
    const int player_id = *CurrentPlayerFaction;
    int has_flechette = 0;
    Faction* plr_def = &Factions[faction_id_def];

    for (auto& m : iterate_tiles(tx, ty, 0, flec_range)) {
        int base_id = base_at(m.x, m.y);
        if (base_id >= 0) {
            if (Bases[base_id].faction_id != faction_id_atk) {
                if (has_fac_built(FAC_FLECHETTE_DEFENSE_SYS, base_id)) {
                    has_flechette = 1;
                }
            }
        }
    }
    if (!plr_def->satellites_ODP && !has_flechette) {
        return 0;
    }
    int available_odp = plr_def->satellites_ODP - plr_def->ODP_deployed;
    int intercepted_by_odp = 0;
    if (available_odp > 0) {
        int attempts = 0;
        while (true) {
            plr_def->ODP_deployed++;
            if (game_rand() % 100 < MissileDefendChance) {
                intercepted_by_odp = 1;
                break;
            }
            if (++attempts >= available_odp) {
                break;
            }
        }
    }
    int remaining_odp = clamp(plr_def->satellites_ODP - plr_def->ODP_deployed, 0, 9999);

    auto show_odp_dialog = [&](const char* label_we_shot, const char* label_they_shot) {
        int near_id = base_find_2(tx, ty, faction_id_def);
        if (near_id < 0) {
            near_id = base_find(tx, ty);
        }
        parse_says(0, near_id >= 0 ? Bases[near_id].name : "", -1, -1);
        parse_num(0, plr_def->satellites_ODP);
        parse_num(1, remaining_odp);
        if (faction_id_def == player_id) {
            parse_says(1, MFactions[faction_id_atk].adj_name_faction, -1, -1);
            popp(ScriptFile, label_we_shot, 0, "space_sm.pcx", 0);
        } else if (faction_id_atk == player_id) {
            parse_says(1, MFactions[faction_id_def].adj_name_faction, -1, -1);
            popp(ScriptFile, label_they_shot, 0, "space_sm.pcx", 0);
        }
    };

    if (intercepted_by_odp) {
        show_odp_dialog(flag ? "FWESHOTIT" : "TWESHOTIT", flag ? "FTHEYSHOTIT" : "TTHEYSHOTIT");
        return 1;
    }
    for (auto& m : iterate_tiles(tx, ty, 0, flec_range)) {
        int base_id = base_at(m.x, m.y);
        if (base_id >= 0 && Bases[base_id].faction_id != faction_id_atk) {
            if (has_fac_built(FAC_FLECHETTE_DEFENSE_SYS, base_id)) {
                if (game_rand() % 100 < MissileDefendChance) {
                    parse_says(0, Bases[base_id].name, -1, -1);
                    if (faction_id_def == player_id) {
                        parse_says(1, MFactions[faction_id_atk].adj_name_faction, -1, -1);
                        popp(ScriptFile, flag ? "FWESHOTIT2" : "TWESHOTIT2", 0, "space_sm.pcx", 0);
                    } else if (faction_id_atk == player_id) {
                        parse_says(1, MFactions[Bases[base_id].faction_id].adj_name_faction, -1, -1);
                        popp(ScriptFile, flag ? "FTHEYSHOTIT2" : "TTHEYSHOTIT2", 0, "space_sm.pcx", 0);
                    }
                    return 1;
                }
            }
        }
    }
    if (plr_def->satellites_ODP > 0) {
        if (*MultiplayerActive || !is_human(faction_id_def)) {
            if (faction_id_def == player_id) {
                popp(ScriptFile, flag ? "FWEMUSTSAC" : "TWEMUSTSAC", 0, "space_sm.pcx", 0);
            } else if (faction_id_atk == player_id) {
                *GenderDefault = MFactions[faction_id_def].noun_gender;
                *PluralDefault = MFactions[faction_id_def].is_noun_plural;
                parse_says(0, MFactions[faction_id_def].noun_faction, -1, -1);
                popp(ScriptFile, "THEYMUSTSAC", 0, "space_sm.pcx", 0);
            }
            plr_def->satellites_ODP--;
            if (plr_def->satellites_ODP < plr_def->ODP_deployed) {
                plr_def->ODP_deployed = plr_def->satellites_ODP;
            }
            remaining_odp = clamp(plr_def->satellites_ODP - plr_def->ODP_deployed, 0, 9999);
            show_odp_dialog(flag ? "FWESHOTIT" : "TWESHOTIT", flag ? "FTHEYSHOTIT" : "TTHEYSHOTIT");
            return 1;
        } else {
            if (faction_id_def == player_id) {
                parse_num(0, plr_def->satellites_ODP);
                int base_id = base_find(tx, ty);
                parse_says(0, base_id >= 0 ? Bases[base_id].name : "", -1, -1);
                parse_says(1, MFactions[faction_id_atk].adj_name_faction, -1, -1);
                int value = popp(ScriptFile, flag ? "FUNGALSAC" : "TECTONICSAC", 0, "space_sm.pcx", 0);
                if (!value) {
                    plr_def->satellites_ODP--;
                    if (plr_def->satellites_ODP < plr_def->ODP_deployed) {
                        plr_def->ODP_deployed = plr_def->satellites_ODP;
                    }
                    remaining_odp = clamp(plr_def->satellites_ODP - plr_def->ODP_deployed, 0, 9999);
                    show_odp_dialog(flag ? "FWESHOTIT" : "TWESHOTIT", flag ? "FTHEYSHOTIT" : "TTHEYSHOTIT");
                    return 1;
                } else {
                    if (has_flechette) {
                        if (faction_id_def == player_id) {
                            parse_says(1, MFactions[faction_id_atk].adj_name_faction, -1, -1);
                            popp(ScriptFile, "WEFAILEDFLECHETTE", 0, "space_sm.pcx", 0);
                        } else if (faction_id_atk == player_id) {
                            parse_says(1, MFactions[faction_id_def].adj_name_faction, -1, -1);
                            popp(ScriptFile, "THEYFAILEDFLECHETTE", 0, "space_sm.pcx", 0);
                        }
                    }
                    return 0;
                }
            }
        }
    }
    if (has_flechette) {
        if (faction_id_def == player_id) {
            parse_says(1, MFactions[faction_id_atk].adj_name_faction, -1, -1);
            popp(ScriptFile, "WEFAILEDFLECHETTE", 0, "space_sm.pcx", 0);
        } else if (faction_id_atk == player_id) {
            parse_says(1, MFactions[faction_id_def].adj_name_faction, -1, -1);
            popp(ScriptFile, "THEYFAILEDFLECHETTE", 0, "space_sm.pcx", 0);
        }
    }
    return 0;
}

void  __cdecl action_tectonic(int veh_id, int tx, int ty) {
    const int player_id = *CurrentPlayerFaction;
    const int faction_id_atk = Vehs[veh_id].faction_id;
    const int reactor_id = Units[Vehs[veh_id].unit_id].reactor_id;
    const int range = TableRange[reactor_id];
    const int tgt_base_id = base_find(tx, ty);
    int is_visible = 0;
    FX_play2(Sounds, 83, Vehs[veh_id].x, Vehs[veh_id].y);

    if (faction_id_atk != player_id) {
        if (!(Factions[player_id].player_flags & PFLAG_MAP_REVEALED)) {
            if (mapsq(tx, ty)->is_visible(player_id)) {
                if (tgt_base_id >= 0 && Bases[tgt_base_id].faction_id == player_id) {
                    Console_focus(MapWin, tx, ty, faction_id_atk);
                    is_visible = 1;
                }
            }
        } else {
            if (tgt_base_id >= 0 && Bases[tgt_base_id].faction_id == player_id) {
                Console_focus(MapWin, tx, ty, faction_id_atk);
                is_visible = 1;
            }
        }
    }
    kill(veh_id);
    if (faction_id_atk != player_id && tgt_base_id >= 0) {
        *GenderDefault = MFactions[faction_id_atk].is_leader_female;
        *PluralDefault = 0;
        parse_says(0, MFactions[faction_id_atk].title_leader, -1, -1);
        parse_says(1, MFactions[faction_id_atk].name_leader, -1, -1);
        *GenderDefault = MFactions[faction_id_atk].noun_gender;
        *PluralDefault = MFactions[faction_id_atk].is_noun_plural;
        parse_says(2, MFactions[faction_id_atk].noun_faction, -1, -1);
        parse_says(3, Bases[tgt_base_id].name, -1, -1);
        popp(ScriptFile, "TECTONICMISSILE", 0, "tectonic_sm.pcx", 0);
    }
    int hit_factions[MaxPlayerNum] = {};
    int shot_down = 0;
    for (int i = 0; i < range; i++) {
        int x = wrap(tx + TableOffsetX[i]);
        int y = ty + TableOffsetY[i];
        if (on_map(x, y)) {
            int stack_id = stack_fix(veh_at(x, y));
            if (stack_id >= 0) {
                int def_id = Vehs[stack_id].faction_id;
                if (faction_id_atk != def_id) {
                    if (!(Factions[faction_id_atk].diplo_status[def_id] & DIPLO_PACT)) {
                        if (!hit_factions[def_id]) {
                            shot_down = shoot_it(faction_id_atk, def_id, tx, ty, 0);
                            if (shot_down) { break; }
                            hit_factions[def_id] = 1;
                        }
                    }
                }
            }
        }
    }
    if (!shot_down) {
        int owner = whose_territory(faction_id_atk, tx, ty, 0, 0);
        if (owner >= 0 && owner != faction_id_atk && !hit_factions[owner]) {
            shot_down = shoot_it(faction_id_atk, owner, tx, ty, 0);
        }
    }
    if (!shot_down) {
        for (int i = 0; i < reactor_id; i++) {
            world_raise_alt(tx, ty);
        }
        world_climate();
        draw_map(1);
        if (is_visible) {
            parse_says(0, Bases[tgt_base_id].name, -1, -1);
            *GenderDefault = MFactions[faction_id_atk].is_leader_female;
            *PluralDefault = 0;
            parse_says(1, MFactions[faction_id_atk].title_leader, -1, -1);
            parse_says(2, MFactions[faction_id_atk].name_leader, -1, -1);
            *GenderDefault = MFactions[faction_id_atk].noun_gender;
            *PluralDefault = MFactions[faction_id_atk].is_noun_plural;
            parse_says(3, MFactions[faction_id_atk].noun_faction, -1, -1);
            popp(ScriptFile, "TECMOFIED", 0, "tectonic_sm.pcx", 0);
        }
        TectonicDetonationCount[faction_id_atk]++;
        *GameDrawState |= 4;
    }
}

void  __cdecl action_fungal(int veh_id, int tx, int ty) {
    const int player_id = *CurrentPlayerFaction;
    const int faction_id_atk = Vehs[veh_id].faction_id;
    const int unit_id = Vehs[veh_id].unit_id;
    const int tgt_base_id = base_find(tx, ty);
    int is_visible = 0;
    FX_play2(Sounds, 84, Vehs[veh_id].x, Vehs[veh_id].y);
    kill(veh_id);

    if (faction_id_atk != player_id && tgt_base_id >= 0) {
        *GenderDefault = MFactions[faction_id_atk].is_leader_female;
        *PluralDefault = 0;
        parse_says(0, MFactions[faction_id_atk].title_leader, -1, -1);
        parse_says(1, MFactions[faction_id_atk].name_leader, -1, -1);
        *GenderDefault = MFactions[faction_id_atk].noun_gender;
        *PluralDefault = MFactions[faction_id_atk].is_noun_plural;
        parse_says(2, MFactions[faction_id_atk].noun_faction, -1, -1);
        parse_says(3, Bases[tgt_base_id].name, -1, -1);
        popp(ScriptFile, "FUNGALMISSILE", 0, "fungpayld_sm.pcx", 0);
    }
    int range = TableRange[1];
    int hit_factions[MaxPlayerNum] = {};
    int shot_down = 0;

    for (int i = 0; i < range; i++) {
        int x = wrap(tx + TableOffsetX[i]);
        int y = ty + TableOffsetY[i];
        if (on_map(x, y)) {
            int stack_id = stack_fix(veh_at(x, y));
            if (stack_id >= 0) {
                int def_id = Vehs[stack_id].faction_id;
                if (faction_id_atk != def_id) {
                    if (!(Factions[faction_id_atk].diplo_status[def_id] & DIPLO_PACT)) {
                        if (!hit_factions[def_id]) {
                            shot_down = shoot_it(faction_id_atk, def_id, tx, ty, 1);
                            if (shot_down) { break; }
                            hit_factions[def_id] = 1;
                        }
                    }
                }
            }
        }
    }
    if (!shot_down) {
        int owner = whose_territory(faction_id_atk, tx, ty, 0, 0);
        if (owner >= 0 && owner != faction_id_atk && !hit_factions[owner]) {
            shot_down = shoot_it(faction_id_atk, owner, tx, ty, 1);
        }
    }
    if (!shot_down) {
        int spawn_count = (conf.ignore_reactor_power ? 5 : Units[unit_id].reactor_id + 1);
        int spawn = 0;
        for (int i = 0; i < 50 && spawn < spawn_count; i++) {
            int n = (i == 0) ? 0 : (game_rand() & 7) + 1;
            int x = wrap(tx + TableOffsetX[n]);
            int y = ty + TableOffsetY[n];
            MAP* sq = mapsq(x, y);
            if (!sq) {
                continue;
            }
            if (game_rand() & 1 && sq->base_who() < 0 && sq->veh_who() < 0) {
                int spawn_unit = (is_ocean(sq) ? BSC_ISLE_OF_THE_DEEP : BSC_MIND_WORMS);
                veh_init(spawn_unit, 0, x, y);
                spawn += 1;
            } else {
                spawn += !sq->is_fungus();
            }
            if (sq->is_fungus()) {
                continue;
            }
            int focus_faction = faction_id_atk;
            if (faction_id_atk != player_id && is_known(x, y, player_id)) {
                focus_faction = player_id;
            }
            if (focus_faction == player_id) {
                if (tgt_base_id >= 0 && Bases[tgt_base_id].faction_id == player_id) {
                    Console_focus(MapWin, x, y, focus_faction);
                    is_visible = 1;
                }
            }
            bit_set(x, y, BIT_FUNGUS, 1);
            bit_set(x, y, BIT_FOREST|BIT_SOIL_ENRICHER|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD, 0);
            bit_set(x, y, Terraform[FORMER_PLANT_FUNGUS].bit_incompatible, 0);
            if (sq->alt_level() < ALT_OCEAN_SHELF) {
                world_alt_set(x, y, ALT_OCEAN_SHELF, 0);
            }
            if (is_known(x, y, player_id) || (*GameState & STATE_OMNISCIENT_VIEW)) {
                synch_bit(x, y, player_id);
                draw_tile(x, y, 2);
            }
        }
        if (is_ocean(mapsq(tx, ty))) {
            veh_init(conf.spawn_sealurks && (game_rand() & 1) ? BSC_SEALURK : BSC_ISLE_OF_THE_DEEP, 0, tx, ty);
        } else {
            veh_init(BSC_FUNGAL_TOWER, 0, tx, ty);
        }
        *GameDrawState |= 4;

        if (is_visible) {
            parse_says(0, Bases[tgt_base_id].name, -1, -1);
            *PluralDefault = 0;
            *GenderDefault = MFactions[faction_id_atk].is_leader_female;
            parse_says(1, MFactions[faction_id_atk].title_leader, -1, -1);
            parse_says(2, MFactions[faction_id_atk].name_leader, -1, -1);
            *GenderDefault = MFactions[faction_id_atk].noun_gender;
            *PluralDefault = MFactions[faction_id_atk].is_noun_plural;
            parse_says(3, MFactions[faction_id_atk].noun_faction, -1, -1);
            popp(ScriptFile, "FUNGMOTIZED", 0, "fungpayld_sm.pcx", 0);
        }
    }
}

void __cdecl action_give(int veh_id, int faction_id_tgt) {
    const int player_id = *CurrentPlayerFaction;
    VEH* const veh = &Vehs[veh_id];
    const int faction_id = veh->faction_id;
    const int vx = veh->x;
    const int vy = veh->y;

    int cur_id = veh_top(veh_id);
    while (cur_id >= 0) {
        VEH* cur = &Vehs[cur_id];
        UNIT* src = &Units[cur->unit_id];
        if (cur_id != veh_id) {
            if (cur->order != ORDER_SENTRY_BOARD || cur->waypoint_x[0] != veh_id) {
                cur_id = cur->next_veh_id_stack;
                continue;
            }
        }
        int new_unit_id = (cur->unit_id < MaxProtoFactionNum ? cur->unit_id : -1);
        int start_id = faction_id_tgt * MaxProtoFactionNum;
        if (new_unit_id < 0) {
            for (int i = 0; i < MaxProtoFactionNum; i++) {
                UNIT* u = &Units[start_id + i];
                if (u->is_active()
                && u->chassis_id == src->chassis_id
                && u->weapon_id == src->weapon_id
                && u->armor_id == src->armor_id
                && u->ability_flags == src->ability_flags
                && u->reactor_id == src->reactor_id) {
                    new_unit_id = start_id + i;
                    break;
                }
            }
        }
        if (new_unit_id < 0) {
            new_unit_id = propose_proto(faction_id_tgt, src->chassis_id, src->weapon_id, src->armor_id,
                src->ability_flags, src->reactor_id, src->group_id, 0);
        }
        if (new_unit_id < 0) {
            for (int i = 0; i < MaxProtoFactionNum; i++) {
                UNIT* u = &Units[start_id + i];
                if (u->is_active()
                && u->chassis_id == src->chassis_id
                && u->weapon_id == src->weapon_id
                && u->armor_id == src->armor_id) {
                    new_unit_id = start_id + i;
                    break;
                }
            }
        }
        if (new_unit_id < 0) {
            for (int i = 0; i < MaxProtoFactionNum; i++) {
                UNIT* u = &Units[start_id + i];
                if (u->is_active() && u->plan == src->plan) {
                    new_unit_id = start_id + i;
                    break;
                }
            }
        }
        if (new_unit_id < 0) {
            for (int i = 0; i < MaxProtoFactionNum; i++) {
                UNIT* u = &Units[start_id + i];
                if (u->is_active() && u->chassis_id == src->chassis_id) {
                    new_unit_id = start_id + i;
                    break;
                }
            }
        }
        if (new_unit_id < 0) {
            new_unit_id = (cur->triad() == TRIAD_SEA ? BSC_TRANSPORT_FOIL : BSC_SCOUT_PATROL);
        }
        cur->faction_id = faction_id_tgt;
        cur->unit_id = new_unit_id;
        cur->order = ORDER_NONE;
        cur->state &= ~(VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
        int near_id = base_find_2(vx, vy, faction_id_tgt);
        cur->home_base_id = (near_id >= 0 ? near_id : -1);
        if (mapsq(vx, vy)->base_who() < 0) {
            owner_set(vx, vy, faction_id_tgt);
        }
        cur_id = cur->next_veh_id_stack;
    }

    draw_radius(vx, vy, 3, 2);
    if (faction_id_tgt == player_id) {
        parse_says(0, Units[veh->unit_id].name, -1, -1);
        *PluralDefault = 0;
        *GenderDefault = MFactions[faction_id].is_leader_female;
        parse_says(1, MFactions[faction_id].title_leader, -1, -1);
        parse_says(2, MFactions[faction_id].name_leader, -1, -1);
        *PluralDefault = MFactions[faction_id].is_noun_plural;
        *GenderDefault = MFactions[faction_id].noun_gender;
        parse_says(3, MFactions[faction_id].noun_faction, -1, -1);
        Console_focus(MapWin, vx, vy, faction_id);
        int near_id = base_find(vx, vy);
        if (near_id >= 0) {
            parse_says(4, Bases[near_id].name, -1, -1);
            if (*GameLanguage || is_human(faction_id)) {
                NetMsg_pop(NetMsg, "GOTUNIT", 5000, 0, 0);
            // simplify check to remove region_base_plan
            } else if (Units[veh->unit_id].plan == PLAN_DEFENSE) {
                X_pop("GOTUNITDEFEND", 0);
            } else {
                X_pop("GOTUNITOFFEND", 0);
            }
        }
    } else if (faction_id == player_id) {
        *PluralDefault = MFactions[faction_id_tgt].is_noun_plural;
        *GenderDefault = MFactions[faction_id_tgt].noun_gender;
        parse_says(3, MFactions[faction_id_tgt].noun_faction, -1, -1);
        if (faction_id_tgt) {
            NetMsg_pop(NetMsg, "GAVEUNIT", 5000, 0, 0);
        } else {
            NetMsg_pop(NetMsg, "GAVEWILD", 5000, 0, 0);
        }
    }
}

void __cdecl action_gate(int veh_id, int base_id) {
    VEH* veh = &Vehs[veh_id];
    BASE* tgt = &Bases[base_id];
    int src_base_id = base_at(veh->x, veh->y);
    if (src_base_id >= 0) {
        Bases[src_base_id].state_flags |= BSTATE_PSI_GATE_USED;
    }
    tgt->state_flags |= BSTATE_PSI_GATE_USED;
    veh_drop(veh_lift(veh_id), tgt->x, tgt->y);
    if (src_base_id >= 0) {
        draw_tile(Bases[src_base_id].x, Bases[src_base_id].y, 2);
    }
    draw_tile(tgt->x, tgt->y, 2);
    if (veh_id == MapWin->iUnit) {
        Console_focus(MapWin, veh->x, veh->y, veh->faction_id);
    }
}

/*
offset - Direction index [0-7] into BaseOffset representing the eight move directions.
A negative value is normalised to 8 at the start. Values >= 8 signal a no-move action (skip/hold),
which bypasses normal movement and can instead trigger the upkeep path or the aircraft home-return check.
flag - Written to VehAttackFlags. Uses two lowest bits that are always set by non-MP action functions.
*/
int __cdecl order_veh(int veh_id, int offset, int flag) {
    Ftext_get text_get = (Ftext_get)0x5FD570;
    int* const dword_8CC210 = (int*)0x8CC210;
    int* const dword_945AF4 = (int*)0x945AF4;
    int* const dword_945AF8 = (int*)0x945AF8;

    int goody_val;
    int prev_vehcount;
    int move_check_fuel;
    int move_fight;
    int move_no_base;
    int move_probe;
    int move_result;
    int veh_at_sea;
    int veh_fc_id;
    int veh_triad;
    int veh_x;
    int veh_y;
    int move_delay;
    int cost_val;
    int base_id;
    int visual_tgl;
    int stack_veh_id;
    int tgt_region;
    int tgt_owner;
    int tgt_no_airbase;
    int tgt_at_sea;
    int tgt_fc_id;
    int tgt_x;
    int tgt_y;
    int spawn_base_dist;
    int spawn_chance;
    MAP* veh_sq;
    MAP* tgt_sq;

    move_result = 0;
    move_no_base = 0;
    move_check_fuel = 0;
    move_fight = 0;
    move_probe = 0;
    visual_tgl = 0; // Fix: always initialize to avoid randomly triggering effects

    Popup cur_popup = {};
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });

    *VehAttackFlags = flag;
    if (flag & 1) {
        MapWin->field_23BE4 = 0;
    } else {
        MapWin->field_23BF0 = 0;
    }
    prev_vehcount = *VehCount;
    veh_x = Vehs[veh_id].x;
    veh_y = Vehs[veh_id].y;
    veh_fc_id = Vehs[veh_id].faction_id;
    if (offset < 0) {
        offset = 8;
    }
    tgt_x = wrap(veh_x + BaseOffsetX[offset]);
    tgt_y = veh_y + BaseOffsetY[offset];
    tgt_sq = mapsq(tgt_x, tgt_y);
    if (!tgt_sq) {
        goto MOV_END;
    }
    tgt_region = tgt_sq->region;
    stack_veh_id = stack_fix(veh_at(tgt_x, tgt_y));
    base_id = base_at(tgt_x, tgt_y);
    if (base_id < 0) {
        if (stack_veh_id < 0) {
            tgt_fc_id = -1;
        } else {
            tgt_fc_id = Vehs[stack_veh_id].faction_id;
        }
    } else {
        tgt_fc_id = Bases[base_id].faction_id;
    }
    veh_at_sea = is_ocean(mapsq(veh_x, veh_y));
    tgt_at_sea = is_ocean(tgt_sq);
    goody_val = goody_at(tgt_x, tgt_y);

    while (true) {
        move_delay = 0;
        if (Vehs[veh_id].state & (VSTATE_EXPLORE|VSTATE_ON_ALERT)) {
            move_delay = 1;
        }
        veh_triad = Vehs[veh_id].triad();
        if (!is_human(veh_fc_id) && Vehs[veh_id].order == ORDER_MOVE_TO) {
            tgt_owner = whose_territory(veh_fc_id, Vehs[veh_id].waypoint_x[0], Vehs[veh_id].waypoint_y[0], 0, 0);
            if (tgt_owner >= 0 && tgt_owner != veh_fc_id
            && !(Factions[veh_fc_id].diplo_status[tgt_owner] & (DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_VENDETTA|DIPLO_PACT))) {
                Vehs[veh_id].order = ORDER_NONE;
                goto MOV_END;
            }
        }
        if (Vehs[veh_id].plan() == PLAN_ARTIFACT && !(Vehs[veh_id].flags & VFLAG_IS_OBJECTIVE)) {
            if (*VehAttackFlags & 1) {
                Vehs[veh_id].movement_turns = 0;
            }
            if (*VehAttackFlags & 1 && base_id >= 0 && (Bases[base_id].faction_id == veh_fc_id)) {
                parse_says(0, Bases[base_id].name, -1, -1);
                Popup_start(&cur_popup, "SCRIPT", "ARTIFACT", -1, 0, 0, 0);
                int w = 0, h = 0;
                if (!Buffer_get_pcx_dimensions("art_dis_sm.pcx", &w, &h)
                && !Sprite_init(&cur_popup.sprite, "art_dis_sm.pcx", w, h)) {
                    cur_popup.field_2144 = &cur_popup.sprite;
                }
                parse_num(0, Bases[base_id].minerals_accumulated);
                int item_id = Bases[base_id].queue_items[0];
                int cost;
                if (item_id >= 0) {
                    cost = mod_veh_cost(item_id, base_id, 0) * mod_cost_factor(veh_fc_id, RSC_MINERAL, -1);
                } else {
                    cost = Facility[-item_id].cost * mod_cost_factor(veh_fc_id, RSC_MINERAL, -1);
                }
                parse_num(1, cost);
                if (!X_text_open("SCRIPT", "ARTIFACTMENU")) {
                    StrBuffer[0] = 0;
                    parse_string(text_get(), StrBuffer);
                    Dialogs_item(&cur_popup.dialogs, StrBuffer, 0);
                    int dialog_cnt = 1;
                    char* str = text_get();
                    if (has_fac_built(FAC_NETWORK_NODE, base_id)) {
                        if (project_base(FAC_UNIVERSAL_TRANSLATOR) != base_id) {
                            if (!(Bases[base_id].state_flags & BSTATE_ARTIFACT_ALREADY_LINKED)) {
                                StrBuffer[0] = 0;
                                parse_string(str, StrBuffer);
                                Dialogs_item(&cur_popup.dialogs, StrBuffer, 1);
                                dialog_cnt = 2;
                                if (!is_human(veh_fc_id)) {
                                    Vehs[veh_id].movement_turns = 1;
                                }
                            }
                        }
                    }
                    str = text_get();
                    if (project_base(FAC_UNIVERSAL_TRANSLATOR) == base_id) {
                        StrBuffer[0] = 0;
                        parse_string(str, StrBuffer);
                        Dialogs_item(&cur_popup.dialogs, StrBuffer, 1);
                        ++dialog_cnt;
                        if (!is_human(veh_fc_id)) {
                            Vehs[veh_id].movement_turns = 1;
                        }
                    }
                    str = text_get();
                    if (item_id <= -SP_ID_First) {
                        StrBuffer[0] = 0;
                        parse_says(1, Facility[abs(item_id)].name, -1, -1);
                        parse_string(str, StrBuffer);
                        Dialogs_item(&cur_popup.dialogs, StrBuffer, 2);
                        ++dialog_cnt;
                        if (!is_human(veh_fc_id)) {
                            Vehs[veh_id].movement_turns = 2;
                        }
                    }
                    str = text_get();
                    if (item_id >= 0 && item_id >= MaxProtoFactionNum) {
                        if (!(Units[item_id].unit_flags & UNIT_PROTOTYPED)) {
                            StrBuffer[0] = 0;
                            parse_says(2, Units[item_id].name, -1, -1);
                            parse_string(str, StrBuffer);
                            Dialogs_item(&cur_popup.dialogs, StrBuffer, 2);
                            ++dialog_cnt;
                        }
                    }
                    if (dialog_cnt > 1 && veh_fc_id == MapWin->cOwner && is_human(veh_fc_id)) {
                        Vehs[veh_id].movement_turns = BasePop_exec_3(&cur_popup, 0, 0);
                        if (*MultiplayerActive && !*ControlTurnC) { // Fix: added MP checks
                            synch_veh(veh_id);
                            NetDaemon_await_synch(NetState);
                        }
                    }
                }
            }
        }
        if (offset >= 8) {
            if (Vehs[veh_id].triad() == TRIAD_AIR && Vehs[veh_id].range()) {
                if ((*VehAttackFlags & 1) && veh_fc_id == MapWin->cOwner) {
                    tgt_sq = mapsq(tgt_x, tgt_y);
                    if (tgt_sq->base_who() < 0 && !(tgt_sq->items & BIT_AIRBASE)) {
                        bool check = false;
                        for (int cur_id = veh_top(veh_id); cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
                            if (has_abil(Vehs[cur_id].unit_id, ABL_CARRIER) // Fix: use correct veh_id in statement
                            || (Vehs[cur_id].plan() == PLAN_SUPPLY && Vehs[cur_id].triad() == TRIAD_AIR)) {
                                check = true;
                                break;
                            }
                        }
                        if (!check) {
                            if (veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent > 0
                            && (Vehs[veh_id].range() == 1 || Vehs[veh_id].movement_turns)) {
                                action_home(veh_id, 0);
                                if (Vehs[veh_id].order == ORDER_MOVE_TO) {
                                    if (*MultiplayerActive && !*ControlTurnC) {
                                        synch_veh(veh_id);
                                        NetDaemon_await_synch(NetState);
                                    }
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
            move_result = 1;
            if (!(*VehAttackFlags & 2)) {
                goto MOV_END;
            }
            veh_skip(veh_id);
            Vehs[veh_id].state &= ~VSTATE_UNK_40;
            goto MOV_UPKEEP;
        }
        Vehs[veh_id].state |= VSTATE_HAS_MOVED;
        if (Vehs[veh_id].plan() == PLAN_PROBE && tgt_fc_id >= 0 && veh_fc_id != tgt_fc_id) {
            bool check = true;
            if (*VehAttackFlags & 1) {
                Vehs[veh_id].flags &= ~VFLAG_PROBE_PACT_OPERATIONS;
                if (Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT) {
                    if (!move_delay && veh_fc_id == MapWin->cOwner) {
                        if (base_id < 0) {
                            if (mod_stack_check(stack_veh_id, 2, PLAN_PROBE, tgt_fc_id, -1)) {
                                parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                                if (X_pop("PROBEPACTTEAM", 0)) {
                                    Vehs[veh_id].flags |= VFLAG_PROBE_PACT_OPERATIONS;
                                } else {
                                    check = false;
                                }
                            }
                        } else {
                            parse_says(0, Bases[base_id].name, -1, -1);
                            parse_says(1, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                            if (X_pop("PROBEPACT", 0)) {
                                Vehs[veh_id].flags |= VFLAG_PROBE_PACT_OPERATIONS;
                            } else {
                                check = false;
                            }
                        }
                    }
                }
            }
            if (check) {
                if (base_id < 0) {
                    if (!(Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT)
                    && (stack_veh_id < 0 || !mod_stack_check(stack_veh_id, 2, PLAN_PROBE, -1, -1))) {
                        move_probe = 1;
                        if (probe(veh_id, -1, stack_veh_id, 1) && *MultiplayerActive
                        && !*ControlTurnC && *VehAttackFlags & 1 && !(*VehAttackFlags & 2)) {
                            synch_veh(veh_id);
                            NetDaemon_await_synch(NetState);
                            return 1;
                        }
                        if (*VehCount != prev_vehcount) {
                            return 0;
                        }
                        if (stack_veh_id >= 0) {
                            tgt_fc_id = Vehs[stack_veh_id].faction_id;
                        }
                    }
                } else if ((stack_veh_id < 0 || !mod_stack_check(stack_veh_id, 2, PLAN_PROBE, -1, -1))
                && (is_human(veh_fc_id) || !(Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT)
                || (Vehs[veh_id].waypoint_x[0] == Bases[base_id].x && Vehs[veh_id].waypoint_y[0] == Bases[base_id].y))) {
                    if (tgt_fc_id == MapWin->cOwner) {
                        Console_focus(MapWin, veh_x, veh_y, veh_fc_id);
                        if (!*MultiplayerActive) {
                            clock_wait(400);
                        }
                        stack_veh(veh_id, 1);
                        veh_scoot(veh_id, veh_x, veh_y, offset, 0);
                        stack_put(veh_id, veh_x, veh_y);
                        draw_tile(tgt_x, tgt_y, 2);
                    }
                    if (!probe(veh_id, base_id, -1, 1) || !*MultiplayerActive || *ControlTurnC
                    || !(*VehAttackFlags & 1) || *VehAttackFlags & 2) {
                        goto MOV_END;
                    }
                    synch_veh(veh_id);
                    NetDaemon_await_synch(NetState);
                    return 1;
                }
            }
        }
        if (veh_triad == TRIAD_LAND && tgt_at_sea) {
            if (veh_fc_id == tgt_fc_id || Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT) {
                if (mod_stack_check(stack_veh_id, 8, veh_fc_id, -1, -1) <= 0) {
                    if (base_id < 0
                    || (!has_abil(Vehs[veh_id].unit_id, ABL_AMPHIBIOUS)
                    && !mod_stack_check(stack_veh_id, 2, 7, -1, -1)
                    && !mod_stack_check(veh_id, 2, 7, -1, -1))) {
                        if (base_id >= 0
                        && !mod_stack_check(stack_veh_id, 2, 7, -1, -1)
                        && !mod_stack_check(veh_id, 2, 7, -1, -1)) {
                            if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                                parse_says(0, Ability[abil_index(8)].name, -1, -1);
                                NetMsg_pop(NetMsg, "AMPHIBBASE2", 5000, 0, 0);
                            }
                            goto MOV_END;
                        }
                        int stack_val = mod_stack_check(stack_veh_id, 2, 7, -1, -1);
                        if (stack_val && veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                            if (stack_val <= 1) {
                                NetMsg_pop(NetMsg, "FULLTRANSPORT", 5000, 0, 0);
                            } else {
                                NetMsg_pop(NetMsg, "FULLTRANSPORTS", 5000, 0, 0);
                            }
                        }
                        goto MOV_END;
                    }
                }
                if (base_id < 0) {
                    move_no_base = 1;
                }
            } else if (base_id >= 0) {
                if (!has_abil(Vehs[veh_id].unit_id, ABL_AMPHIBIOUS)) {
                    if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                        parse_says(0, Ability[abil_index(8)].name, -1, -1);
                        NetMsg_pop(NetMsg, "AMPHIBBASE", 5000, 0, 0);
                    }
                    goto MOV_END;
                }
            } else {
                if (stack_veh_id < 0
                || !has_abil(Vehs[veh_id].unit_id, ABL_AIR_SUPERIORITY)
                || !mod_stack_check(stack_veh_id, 18, -1, -1, -1)
                || mod_stack_check(stack_veh_id, 6, ABL_CARRIER, -1, -1)) {
                    goto MOV_END;
                }
                goto MOV_SPOT;
            }
        }
        if (!veh_at_sea && !tgt_at_sea) {
            if (Vehs[veh_id].triad() == TRIAD_LAND
            && Vehs[veh_id].plan() != PLAN_PROBE
            && !(Units[Vehs[veh_id].unit_id].ability_flags & ABL_CLOAKED)
            && stack_veh_id < 0
            && mod_zoc_move(veh_x, veh_y, veh_fc_id)
            && mod_zoc_move(tgt_x, tgt_y, veh_fc_id)) {
                if (veh_fc_id != MapWin->cOwner || move_delay || !(*VehAttackFlags & 1)) {
                    goto MOV_END;
                }
                FX_play(Sounds, 13);
                if (*MultiplayerActive) {
                    goto MOV_END;
                }
                if (veh_fc_id == MapWin->cOwner) {
                    if (*GamePreferences & PREF_BSC_TUTORIAL_MSGS
                    || (Factions[veh_fc_id].diff_level <= 1 && (++(*dword_945AF4), (*dword_945AF4 & 3) == 1))) {
                        int a = 0, b = 0, c = 0, d = 0;
                        parse_says(0, Ability[abil_index(4)].name, -1, -1);
                        quick_zoc(veh_x, veh_y, veh_fc_id, tgt_x, tgt_y, &a, &b);
                        quick_zoc(tgt_x, tgt_y, veh_fc_id, veh_x, veh_y, &c, &d);
                        TutWin_tut_map(TutWin, "ZOCMSG", a, b, veh_id, 0, 0, c, d);
                    }
                }
                move_probe = 0;
                for (int i = 0; i < 8; i++) {
                    int px = wrap(tgt_x + BaseOffsetX[i]);
                    int py = tgt_y + BaseOffsetY[i];
                    if (on_map(px, py)) {
                        int cur_id = stack_fix(veh_at(px, py));
                        if (cur_id >= 0 && Vehs[cur_id].faction_id != veh_fc_id) {
                            if (!(Factions[veh_fc_id].diplo_status[Vehs[cur_id].faction_id] & DIPLO_PACT)
                            && !(Vehs[cur_id].is_visible(veh_fc_id))) {
                                spot_loc(px, py, veh_fc_id);
                                move_probe = 1;
                            }
                        }
                    }
                }
                if (move_probe) {
                    draw_radius(tgt_x, tgt_y, 1, 2);
                }
                goto MOV_END;
            }
        }
        if (Vehs[veh_id].plan() == PLAN_SUPPLY && base_id >= 0 && !*MultiplayerActive) {
            int opt_val = supply_options(veh_id, base_id);
            if (*VehAttackFlags & 2) {
                if (opt_val) {
                    goto MOV_END;
                }
            } else {
                if (!opt_val) {
                    goto MOV_END;
                }
            }
        }
        if (!(*VehAttackFlags & 2)
        || Vehs[veh_id].triad() == TRIAD_AIR
        || stack_veh_id >= 0
        || !veh_fc_id
        || !(tgt_sq = mapsq(tgt_x, tgt_y))
        || !tgt_sq->is_fungus()
        || tgt_sq->items & BIT_MAGTUBE
        || veh_at_sea != tgt_at_sea
        || goody_at(tgt_x, tgt_y)
        || *GameRules & RULES_SCN_NO_NATIVE_LIFE) {
            goto MOV_SPOT;
        }
        if ((!Vehs[veh_id].offense_value() || Vehs[veh_id].order == ORDER_MOVE_TO)
        && has_project(FAC_XENOEMPATHY_DOME, veh_fc_id)) {
            goto MOV_SPOT;
        }
        base_find(tgt_x, tgt_y);
        if (*BaseFindDist <= 1) {
            goto MOV_SPOT;
        }
        spawn_base_dist = clamp(*BaseFindDist, 0, 32);
        spawn_chance = spawn_base_dist + 10 * *MapNativeLifeForms;
        if (tgt_sq->items & BIT_SUPPLY_REMOVE) {
            spawn_chance /= 2;
            if (!*MapNativeLifeForms) {
                spawn_chance /= 2;
            }
        }
        if (tgt_at_sea) {
            spawn_chance /= 2;
            if (!*MapNativeLifeForms) {
                spawn_chance /= 2;
            }
        }
        if (tgt_sq->items & BIT_ROAD) {
            spawn_chance /= 2;
        }
        if (Vehs[veh_id].is_native_unit()) { // remove redundant Spore Launcher reference
            spawn_chance /= 2;
        }
        for (int i = 0; i < 25; i++) {
            int px = wrap(tgt_x + TableOffsetX[i]);
            int py = tgt_y + TableOffsetY[i];
            MAP* psq = mapsq(px, py);
            if (psq) {
                if (psq->base_who() >= 0 || is_sensor(px, py)) {
                    if (i >= 9) {
                        spawn_chance /= 2;
                    } else {
                        spawn_chance /= 4;
                    }
                }
            }
        }
        if (game_rand() % 100 < spawn_chance) {
            VehBasicUnit spawn_unit_id;
            if (tgt_at_sea) {
                spawn_unit_id = BSC_ISLE_OF_THE_DEEP;
            } else if (!(game_rand() % 5) && *ExpansionEnabled && conf.spawn_spore_launchers) {
                spawn_unit_id = BSC_SPORE_LAUNCHER;
            } else {
                spawn_unit_id = BSC_MIND_WORMS;
            }
            stack_veh_id = veh_init(spawn_unit_id, 0, tgt_x, tgt_y);
            if (stack_veh_id >= 0) {
                Vehs[veh_id].order = ORDER_NONE;
                spot_stack(stack_veh_id, veh_fc_id);
                draw_tile(tgt_x, tgt_y, 2);
                bit_set(tgt_x, tgt_y, BIT_SUPPLY_REMOVE, 1);
                if (veh_fc_id == MapWin->cOwner) {
                    FX_play(Sounds, 13);
                    parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                    parse_says(1, Vehs[stack_veh_id].name(), -1, -1);
                    NetMsg_pop(NetMsg, tgt_at_sea ? "ISLESPOTTED" : "WORMSSPOTTED", 5000, 0, 0);
                }
                goto MOV_END;
            }
            goto MOV_NAVAL;
        }
MOV_SPOT:
        if (stack_veh_id < 0) {
            goto MOV_NAVAL;
        }
        if (!(Vehs[stack_veh_id].flags & VFLAG_INVISIBLE)
        && (veh_fc_id == tgt_fc_id || (Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT
        && (Vehs[veh_id].plan() != PLAN_PROBE || !(Vehs[veh_id].flags & VFLAG_PROBE_PACT_OPERATIONS))))) {
            goto MOV_NAVAL;
        }
        if (move_probe) {
            goto MOV_END;
        }
        if (Vehs[stack_veh_id].flags & VFLAG_INVISIBLE) {
            Vehs[veh_id].order = ORDER_NONE;
            spot_stack(stack_veh_id, veh_fc_id);
            draw_tile(tgt_x, tgt_y, 2);
            if (veh_fc_id == MapWin->cOwner) {
                FX_play(Sounds, 13);
                parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                parse_says(1, Vehs[stack_veh_id].name(), -1, -1);
                NetMsg_pop(NetMsg, tgt_fc_id ? "SPOTTED" : "SPOTTED0", 5000, 0, 0);
            }
            goto MOV_END;
        }
        if (base_id < 0
        && (Vehs[stack_veh_id].faction_id != veh_fc_id
        || (Vehs[stack_veh_id].is_invisible_lurker())) // Fix flags field access
        && !(Vehs[stack_veh_id].is_visible(veh_fc_id)) && is_human(veh_fc_id)) {
            if (*VehAttackFlags & 1 && veh_fc_id == MapWin->cOwner) {
                Vehs[veh_id].order = ORDER_NONE;
                spot_stack(stack_veh_id, veh_fc_id);
                draw_tile(tgt_x, tgt_y, 2);
                FX_play(Sounds, 13);
                parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                parse_says(1, Vehs[stack_veh_id].name(), -1, -1);
                NetMsg_pop(NetMsg, tgt_fc_id ? "SPOTTED" : "SPOTTED0", 5000, 0, 0);
                goto MOV_END;
            }
        }
        if (base_id < 0 && !(mapsq(tgt_x, tgt_y)->items & BIT_AIRBASE)
        && mod_stack_check(stack_veh_id, 18, -1, -1, -1)
        && !mod_stack_check(stack_veh_id, 6, 128, -1, -1)
        && !has_abil(Vehs[veh_id].unit_id, ABL_AIR_SUPERIORITY)) {
            if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                parse_says(0, Ability[abil_index(32)].name, -1, -1);
                NetMsg_pop(NetMsg, "FIGHTERRULE", 5000, 0, 0);
            }
            if (Vehs[veh_id].state & (VSTATE_EXPLORE|VSTATE_ON_ALERT)) {
                veh_skip(veh_id);
            }
            goto MOV_END;
        }
        if (Vehs[veh_id].unit_id == BSC_SEALURK || veh_at_sea) {
            if (Vehs[veh_id].unit_id != BSC_SEALURK) {
                if (!tgt_at_sea && veh_triad == TRIAD_SEA
                && !can_arty(Vehs[veh_id].unit_id, 1) && Vehs[veh_id].offense_value()) {
                    if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                        NetMsg_pop(NetMsg, "PSIBOMB", 5000, 0, 0);
                    }
                    goto MOV_END;
                }
            }
            if (veh_at_sea && veh_triad == TRIAD_LAND
            && !has_abil(Vehs[veh_id].unit_id, ABL_AMPHIBIOUS)
            && Vehs[veh_id].plan() != PLAN_PROBE) {
                if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                    parse_says(0, Ability[abil_index(8)].name, -1, -1);
                    veh_sq = mapsq(veh_x, veh_y);
                    if (veh_sq->base_who() >= 0) {
                        NetMsg_pop(NetMsg, "AMPHIBBASE", 5000, 0, 0);
                    } else {
                        NetMsg_pop(NetMsg, "NOTAMPHIB", 5000, 0, 0);
                    }
                }
                goto MOV_END;
            }
        }
        if (Vehs[veh_id].weapon_mode() != WMODE_PROBE || !mod_stack_check(stack_veh_id, 2, 11, -1, -1)) {
            if (!Vehs[veh_id].offense_value()) {
                if (*VehAttackFlags & 1 && veh_fc_id == MapWin->cOwner) {
                    FX_play(Sounds, 51);
                    NetMsg_pop(NetMsg, "NONCOMBATANT", 5000, 0, 0);
                }
                goto MOV_END;
            }
            if (Vehs[stack_veh_id].plan() == PLAN_PROBE
            && mod_stack_check(stack_veh_id, 1, -1, -1, -1) >= 1
            && !*MultiplayerActive
            && (Factions[veh_fc_id].diplo_status[tgt_fc_id] & (DIPLO_TRUCE|DIPLO_TREATY))
            && !(Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT)) {
                int near_base_id = base_find(tgt_x, tgt_y);
                if (near_base_id < 0 || Bases[near_base_id].faction_id != veh_fc_id) {
                    if (veh_fc_id == MapWin->cOwner) {
                        parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                        parse_says(1, Vehs[stack_veh_id].name(), -1, -1);
                        *PluralDefault = 0;
                        *GenderDefault = MFactions[veh_fc_id].is_leader_female;
                        parse_says(2, MFactions[veh_fc_id].title_leader, -1, -1);
                        int choice;
                        if (!MFactions[tgt_fc_id].is_alien()) {
                            choice = popp(ScriptFile, "ENEMYPROBE2", 0, "capture_sm.pcx", 0);
                        } else {
                            choice = popp(ScriptFile, "ENEMYPROBE2", 0, "al_cap_sm.pcx", 0);
                        }
                        if (!choice) {
                            goto MOV_END;
                        }
                    }
                } else {
                    parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                    parse_says(1, Vehs[stack_veh_id].name(), -1, -1);
                    *PluralDefault = 0;
                    *GenderDefault = MFactions[veh_fc_id].is_leader_female;
                    parse_says(2, MFactions[veh_fc_id].title_leader, -1, -1);
                    int choice;
                    if (!is_human(veh_fc_id)) {
                        choice = 1;
                    } else if (!MFactions[tgt_fc_id].is_alien()) {
                        choice = popp(ScriptFile, "ENEMYPROBE", 0, "capture_sm.pcx", 0);
                    } else {
                        choice = popp(ScriptFile, "ENEMYPROBE", 0, "al_cap_sm.pcx", 0);
                    }
                    if (!choice) {
                        goto MOV_END;
                    }
                    if (choice == 1) {
                        if (veh_fc_id == MapWin->cOwner || tgt_fc_id == MapWin->cOwner) {
                            Console_focus(MapWin, veh_x, veh_y, veh_fc_id);
                            stack_veh(veh_id, 1);
                            veh_scoot(veh_id, veh_x, veh_y, offset, 0);
                            stack_put(veh_id, veh_x, veh_y);
                            draw_tile(veh_x, veh_y, 2);
                        }
                        // Fix issue that caused sea-based probe teams to be returned to landlocked bases.
                        int ret_base_id = find_return_base(stack_veh_id);
                        if (ret_base_id < 0) {
                            if (!is_human(tgt_fc_id)) {
                                veh_kill(stack_veh_id);
                            }
                        } else {
                            veh_put(stack_veh_id, Bases[ret_base_id].x, Bases[ret_base_id].y);
                            Vehs[stack_veh_id].visibility = 0;
                            parse_says(2, Bases[ret_base_id].name, -1, -1);
                        }
                        draw_tile(tgt_x, tgt_y, 2);
                        if (tgt_fc_id == MapWin->cOwner) {
                            parse_says(0, MFactions[veh_fc_id].adj_name_faction, -1, -1);
                            *PluralDefault = 0;
                            *GenderDefault = MFactions[tgt_fc_id].is_leader_female;
                            parse_says(1, MFactions[tgt_fc_id].title_leader, -1, -1);
                            NetMsg_pop(NetMsg, "GOTMYPROBE", 5000, 0, 0);
                        } else if (veh_fc_id == MapWin->cOwner) {
                            parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                            *PluralDefault = 0;
                            *GenderDefault = MFactions[veh_fc_id].is_leader_female;
                            parse_says(1, MFactions[veh_fc_id].title_leader, -1, -1);
                            NetMsg_pop(NetMsg, "GOTYOURPROBE", 5000, 0, 0);
                        }
                        goto MOV_END;
                    }
                }
            }
        }
        int moves_avail;
        moves_avail = clamp(veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent, 0, 999);
        if (moves_avail < Rules->move_rate_roads
        && Vehs[veh_id].unit_id != BSC_SPORE_LAUNCHER
        && Vehs[veh_id].unit_id != BSC_MIND_WORMS) {
            if (veh_fc_id == MapWin->cOwner && !move_delay && *VehAttackFlags & 1) {
                parse_says(0, Vehs[veh_id].name(), -1, -1);
                parse_num(0, moves_avail);
                parse_num(1, Rules->move_rate_roads);
                if (popp(ScriptFile, "HASTY", 0, "bad_sm.pcx", 0)) {
                    goto MOV_END;
                }
            } else if (!is_human(veh_fc_id)) {
                // TODO this condition could be adjusted
                if (moves_avail <= Rules->move_rate_roads / 2) {
                    goto MOV_END;
                }
            }
        }
        *CurrentVehID = veh_id;
        if (!(*VehAttackFlags & 2)) {
            move_result = mod_battle_fight(veh_id, offset, 0, 1, 0) == 0;
            goto MOV_END;
        }
        if (!tgt_fc_id && Vehs[veh_id].triad() != TRIAD_AIR) {
            Faction* plr = &Factions[veh_fc_id];
            int tgt_unit_id = Vehs[stack_veh_id].unit_id;
            int unit_count = plr->units_active[tgt_unit_id];
            if (plr->SE_planet > 0 && tgt_unit_id != BSC_ALIEN_ARTIFACT && tgt_unit_id != BSC_FUNGAL_TOWER) {
                bool do_capture = plr->SE_planet_base > 0 && !unit_count;
                if (do_capture || clamp(veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent, 0, 999)
                >= Rules->move_rate_roads) {
                    if (*VehAttackFlags & 2) {
                        int fc_base_id = base_find_2(tgt_x, tgt_y, veh_fc_id);
                        if (Vehs[stack_veh_id].order_auto_type == veh_fc_id
                        || (fc_base_id >= 0 && Bases[fc_base_id].eco_damage)) {
                            // log_say_2("Agitated", 0, 0, 0);
                            if (veh_fc_id == MapWin->cOwner) {
                                NetMsg_pop(NetMsg, "WORMSAGITATED", 5000, 0, 0);
                            }
                        } else {
                            // log_say_2("Precapture who, enemy", veh_fc_id, stack_veh_id, 0);
                            // log_say_2("Precapture fac, proto", plr->SE_planet_base, plr->units_active[tgt_unit_id], 0);
                            // log_say_2("Precapture type, soc2", tgt_unit_id, plr->SE_planet, 0);
                            if (!(Vehs[stack_veh_id].flags & VFLAG_UNK_2000)) {
                                if (game_rand() % 4 < plr->SE_planet) {
                                    int rnd_val = game_randv(*CurrentTurn / 5 + 100);
                                    if (rnd_val >= 10 * unit_count || !(game_rand() % 10)) {
                                        do_capture = true;
                                    }
                                }
                                if (do_capture) {
                                    int cur_id = veh_top(stack_veh_id);
                                    for (; cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
                                        Vehs[cur_id].faction_id = veh_fc_id;
                                        Vehs[cur_id].morale = mod_morale_alien(cur_id, 0);
                                        int near_id = base_find(tgt_x, tgt_y);
                                        if (near_id < 0 || Bases[near_id].faction_id != veh_fc_id
                                        || Bases[near_id].pop_size < 3) {
                                            Vehs[cur_id].home_base_id = -1;
                                        } else {
                                            Vehs[cur_id].home_base_id = near_id;
                                        }
                                        veh_skip(cur_id);
                                    }
                                    bit_set(tgt_x, tgt_y, BIT_SUPPLY_REMOVE, 1);
                                    owner_set(tgt_x, tgt_y, veh_fc_id);
                                    if (veh_fc_id == MapWin->cOwner) {
                                        draw_tile_fixup(tgt_x, tgt_y, 1, 2);
                                    } else {
                                        draw_tile(tgt_x, tgt_y, 2);
                                    }
                                    if (veh_fc_id == MapWin->cOwner) {
                                        parse_says(0, Vehs[stack_veh_id].name(), -1, -1);
                                        if (tgt_unit_id == BSC_SPORE_LAUNCHER) {
                                            if (plr->units_active[BSC_SPORE_LAUNCHER]) {
                                                NetMsg_pop(NetMsg, "WORMCAPTURE", 5000, 0, 0);
                                            } else {
                                                popp(ScriptFile, "WORMCAPTURE", 0, "sporlnch_sm.pcx", 0);
                                            }
                                        } else if (tgt_unit_id == BSC_SEALURK) {
                                            if (plr->units_active[BSC_SEALURK]) {
                                                NetMsg_pop(NetMsg, "WORMCAPTURE", 5000, 0, 0);
                                            } else {
                                                popp(ScriptFile, "WORMCAPTURE", 0, "sealrk_sm.pcx", 0);
                                            }
                                        } else if (plr->units_active[tgt_unit_id]) {
                                            NetMsg_pop(NetMsg, "WORMCAPTURE", 5000, 0, 0);
                                        } else {
                                            bool sea = is_ocean(mapsq(tgt_x, tgt_y));
                                            popp(ScriptFile, "WORMCAPTURE", 0, sea ? "isle_sm.pcx" : "mindworm_sm.pcx", 0);
                                        }
                                    }
                                    // log_say_2("Captured", 0, 0, 0);
                                    goto MOV_NAVAL;
                                }
                            }
                            Vehs[stack_veh_id].flags |= VFLAG_UNK_2000;
                            if (veh_fc_id == MapWin->cOwner) {
                                NetMsg_pop(NetMsg, "WORMNOCAPTURE", 5000, 0, 0);
                            }
                            // log_say_2("No Capture", 0, 0, 0);
                        }
                    }
                }
            }
        }
        {
            move_fight = 1;
            int value = mod_battle_fight(veh_id, offset, 0, 1, 0);
            if (!value) {
                goto MOV_END;
            }
            if (value != 1) {
                move_result = 0;
                goto MOV_END;
            }
            veh_id = *CurrentVehID;
            assert(veh_id >= 0 && veh_id < *VehCount);
            assert(Vehs[veh_id].faction_id == veh_fc_id);
            assert(Vehs[veh_id].x == veh_x && Vehs[veh_id].y == veh_y);
            tgt_x = veh_x;
            tgt_y = veh_y;
            move_check_fuel = 1;
            goody_val = 0;
        }
        goto MOV_UPKEEP; // break while loop

MOV_NAVAL:
        if (veh_at_sea && !tgt_at_sea) {
            // Fix 24. [BUG] Units without the Amphibious Pods ability can no longer move to a land tile
            // from a sea base without there being a transport in land tile.
            if (veh_triad == TRIAD_LAND && !has_abil(Vehs[veh_id].unit_id, ABL_AMPHIBIOUS)) {
                veh_sq = mapsq(veh_x, veh_y);
                if (veh_sq->base_who() >= 0 && !mod_stack_check(veh_id, 2, PLAN_NAVAL_TRANSPORT, -1, -1)) {
                    if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                        parse_says(0, Ability[abil_index(8)].name, -1, -1);
                        NetMsg_pop(NetMsg, "AMPHIBBASE2", 5000, 0, 0);
                    }
                    goto MOV_END;
                }
            }
        }
        if (!tgt_at_sea && base_id < 0 && veh_triad == TRIAD_SEA) {
            if (*MultiplayerActive
            || veh_fc_id != MapWin->cOwner
            || !(*VehAttackFlags & 1)) {
                goto MOV_END;
            }
            // Fix 39. [BUG] #DISEMBARKFAIL wasn't being displayed due to incorrect logic check.
            if (!mod_stack_check(veh_id, 2, PLAN_NAVAL_TRANSPORT, -1, -1)) {
                goto MOV_END;
            }
            Vehs[veh_id].order = ORDER_NONE;
            parse_says(0, Vehs[veh_id].name(), -1, -1);
            Popup_start(&cur_popup, "SCRIPT", "DISEMBARK", -1, 0, 0, 0);
            if (X_text_open("SCRIPT", "DISEMBARKERS")) {
                goto MOV_END;
            }
            stack_veh(veh_id, 0);
            text_get();
            int iter_cnt = 0;
            int last_id = -1;
            int cur_id = veh_top(veh_id);
            if (cur_id < 0) {
                goto MOV_END;
            }
            for (; cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
                if (Vehs[cur_id].order == ORDER_SENTRY_BOARD && Vehs[cur_id].waypoint_x[0] == veh_id) {
                    ++iter_cnt;
                    if (veh_speed(cur_id, 0) - Vehs[cur_id].moves_spent > 0) {
                        last_id = cur_id;
                        StrBuffer[0] = 0;
                        parse_says(1, Vehs[cur_id].name(), -1, -1);
                        parse_string(text_buf_ptr(), StrBuffer);
                        Dialogs_item(&cur_popup.dialogs, StrBuffer, cur_id + 1);
                    }
                }
            }
            if (last_id < 0) {
                if (!iter_cnt) {
                    NetMsg_pop(NetMsg, "DISEMBARKFAIL", 5000, 0, 0);
                }
                goto MOV_END;
            }
            // note veh_id is restored from popup choices
            int pop_val = BasePop_exec_3(&cur_popup, 0, 0) - 1;
            if (pop_val < 0) {
                goto MOV_END;
            }
            veh_id = pop_val;
            Vehs[pop_val].order = ORDER_NONE;
            continue;
        } else {
            break;
        }
    }
    // while loop ends here
    bool check_move;
    check_move = base_id < 0 || tgt_fc_id == veh_fc_id
        || (Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT);
    if (!check_move) {
        if (veh_triad == TRIAD_AIR) {
            if (Vehs[veh_id].range()) {
                if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                    parse_says(0, Chassis[Units[Vehs[veh_id].unit_id].chassis_id].offsv1_name, -1, -1);
                    NetMsg_pop(NetMsg, "AIRCONQUER", 5000, 0, 0);
                }
                goto MOV_END;
            }
        } else if (veh_triad == TRIAD_SEA) {
            if (!tgt_at_sea) {
                if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                    NetMsg_pop(NetMsg, "SEACONQUER", 5000, 0, 0);
                }
                goto MOV_END;
            }
        } else if (veh_triad == TRIAD_LAND) {
            if (tgt_at_sea && !has_abil(Vehs[veh_id].unit_id, ABL_AMPHIBIOUS)) {
                if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                    NetMsg_pop(NetMsg, "LANDCONQUER", 5000, 0, 0);
                }
                goto MOV_END;
            }
        }
        if (!Vehs[veh_id].offense_value()) {
            if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                NetMsg_pop(NetMsg, "COMBATCONQUER", 5000, 0, 0);
            }
            goto MOV_END;
        }
        if (!(Factions[veh_fc_id].diplo_status[tgt_fc_id] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))) {
            check_move = true;
        } else {
            if (!is_human(veh_fc_id)) {
                Vehs[veh_id].order = ORDER_NONE;
                goto MOV_END;
            }
            if (veh_fc_id == MapWin->cOwner) {
                if (*VehAttackFlags & 1) {
                    if (break_treaty(veh_fc_id, tgt_fc_id, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
                        goto MOV_END;
                    }
                }
                check_move = true;
            }
        }
    }
    if (check_move) {
        if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id) && (*VehAttackFlags & 1)) {
            if (Vehs[veh_id].order == ORDER_MOVE_TO
            && (tgt_x != Vehs[veh_id].waypoint_x[0] || tgt_y != Vehs[veh_id].waypoint_y[0])
            && Vehs[veh_id].triad() != TRIAD_AIR) {
                if (!(Vehs[veh_id].state & (VSTATE_EXPLORE|VSTATE_ON_ALERT))
                && goody_at(tgt_x, tgt_y) && popp(ScriptFile, "GOTOGOODY", 0, "supply_sm.pcx", 0)) {
                    Vehs[veh_id].order = ORDER_NONE;
                    goto MOV_END;
                }
            }
        }
    }
    if (veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent <= 0) {
        goto MOV_END;
    }
    int move_val;
    cost_val = mod_hex_cost(Vehs[veh_id].unit_id, veh_fc_id, veh_x, veh_y, tgt_x, tgt_y, 0);
    move_val = clamp(veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent, 0, 999);
    if (cost_val > move_val && veh_triad == TRIAD_LAND) {
        if (move_val < Rules->move_rate_roads
        || (mapsq(tgt_x, tgt_y)->is_fungus()
        && !Vehs[veh_id].is_native_unit()
        && Vehs[veh_id].plan() != PLAN_TERRAFORM
        && Vehs[veh_id].plan() != PLAN_ARTIFACT
        && stack_veh_id < 0)) {
            if (*VehAttackFlags & 1 || !is_human(veh_fc_id)) {
                int rnd_val;
                if (*ControlTurnC) {
                    rnd_val = game_randv(cost_val);
                } else {
                    rnd_val = game_random(0, cost_val);
                }
                int addon_val = move_val + ((Vehs[veh_id].state & VSTATE_UNK_8000) ? Rules->move_rate_roads : 0);
                if (rnd_val >= addon_val) {
                    if (veh_fc_id == MapWin->cOwner) {
                        if (Vehs[veh_id].order == ORDER_NONE
                        && !shift_key_down()
                        && (*VehAttackFlags & 1)
                        && !(Vehs[veh_id].state & VSTATE_ON_ALERT)
                        && !(*GameMorePreferences & MPREF_ADV_QUICK_MOVE_ALL_VEH)) {
                            Console_focus(MapWin, veh_x, veh_y, MapWin->cOwner);
                            stack_veh(veh_id, 1);
                            veh_scoot(veh_id, veh_x, veh_y, offset + 8, 0);
                            stack_put(veh_id, veh_x, veh_y);
                            for (int i = 0; i < 8; i++) {
                                int px = wrap(tgt_x + BaseOffsetX[i]);
                                int py = tgt_y + BaseOffsetY[i];
                                if (on_map(px, py)) {
                                    spot_loc(px, py, veh_fc_id);
                                }
                            }
                            draw_tiles(tgt_x, tgt_y, 2);
                        }
                        if (veh_fc_id == MapWin->cOwner) {
                            tgt_sq = mapsq(tgt_x, tgt_y);
                            if (tgt_sq->is_fungus() && Vehs[veh_id].order == ORDER_NONE) {
                                int plan = Vehs[veh_id].plan();
                                if (plan != PLAN_TERRAFORM && plan != PLAN_ARTIFACT && *VehAttackFlags & 1) {
                                    if (tut_check(0x1000)) {
                                        ++(*dword_945AF8);
                                        FX_play(Sounds, 9);
                                        TutWin_tut_map(TutWin, "SLOWFUNGUS", tgt_x, tgt_y, -1, 0, 0, -1, -1);
                                        Vehs[veh_id].order = ORDER_NONE;
                                        goto MOV_END;
                                    }
                                    if (!move_delay) {
                                        ++(*dword_945AF8);
                                    }
                                }
                            }
                            if (veh_fc_id == MapWin->cOwner && !Vehs[veh_id].order && !(*dword_8CC210)) {
                                if (*VehAttackFlags & 1 && !tgt_sq->is_fungus()) {
                                    if (tut_check(0x4000)) {
                                        TutWin_tut_map(TutWin, "FRACTIONALMOVE", tgt_x, tgt_y, veh_id, 0, 0, veh_x, veh_y);
                                        Vehs[veh_id].order = ORDER_NONE;
                                        goto MOV_END;
                                    }
                                }
                            }
                        }
                    }
                    veh_skip(veh_id);
                    Vehs[veh_id].state |= VSTATE_UNK_8000;
                    if ((*VehAttackFlags & 1) && *MultiplayerActive && !*ControlTurnC) {
                        synch_veh(veh_id);
                        NetDaemon_await_synch(NetState);
                    }
                    move_result = 1;
                    goto MOV_END;
                }
            }
        }
    }
    move_result = 1;
    if (!(*VehAttackFlags & 2)) {
        goto MOV_END;
    }
    Vehs[veh_id].moves_spent += cost_val;
    Vehs[veh_id].state &= ~(VSTATE_UNK_8000|VSTATE_UNK_40);
    if (base_id >= 0) {
        if (tgt_fc_id != veh_fc_id && !(Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT)) {
            if (Factions[tgt_fc_id].diplo_status[veh_fc_id] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)
            && tgt_fc_id == MapWin->cOwner) {
                *PluralDefault = 0;
                *GenderDefault = MFactions[veh_fc_id].is_leader_female;
                parse_says(0, MFactions[veh_fc_id].title_leader, -1, -1);
                parse_says(1, MFactions[veh_fc_id].name_leader, -1, -1);
                *PluralDefault = MFactions[veh_fc_id].is_noun_plural;
                *GenderDefault = MFactions[veh_fc_id].noun_gender;
                parse_says(2, MFactions[veh_fc_id].noun_faction, -1, -1);
                NetMsg_pop(NetMsg, "SURPRISE", 5000, 0, 0);
            }
            if (veh_fc_id == MapWin->cOwner) {
                visual_tgl = 1;
            } else {
                if (tgt_fc_id != MapWin->cOwner) {
                    if (Bases[base_id].faction_id == MapWin->cOwner
                    || ((1 << MapWin->cOwner) & Bases[base_id].visibility)) {
                        if (veh_fc_id) {
                            spot_stack(veh_id, MapWin->cOwner);
                        }
                    }
                }
            }
        }
    }
    if (veh_fc_id == MapWin->cOwner
    || (Vehs[veh_id].faction_id == MapWin->cOwner
    && !Vehs[veh_id].is_invisible_lurker())) {
        visual_tgl = 1;
    } else {
        visual_tgl = (*GameState & STATE_OMNISCIENT_VIEW)
            || Vehs[veh_id].is_visible(MapWin->cOwner);
    }
    if (!visual_tgl && !(*GameState & STATE_OMNISCIENT_VIEW)
    && !is_known(tgt_x, tgt_y, MapWin->cOwner)) {
        move_delay = 0;
    } else {
        if (!visual_tgl) {
            tgt_sq = mapsq(tgt_x, tgt_y);
            for (int i = 0; i < 21; i++) {
                int px = wrap(tgt_x + TableOffsetX[i]);
                int py = tgt_y + TableOffsetY[i];
                MAP* psq = mapsq(px, py);
                if (psq && tgt_sq
                && (is_ocean(psq) != is_ocean(tgt_sq) || psq->region == tgt_sq->region)) {
                    if (is_sensor(px, py) && MapWin->cOwner == whose_territory(MapWin->cOwner, px, py, 0, 0)) {
                        visual_tgl = 1;
                        break;
                    }
                    if (MapWin->cOwner == (i >= 9 ? psq->base_who() : psq->anything_at())) {
                        visual_tgl = 1;
                        break;
                    }
                }
            }
        }
        if (!visual_tgl) {
            move_delay = 0;
        } else {
            move_delay = 1;
            if (veh_fc_id != MapWin->cOwner
            && (!(*GamePreferences & PREF_BSC_DONT_QUICK_MOVE_ENEMY_VEH)
            || Factions[MapWin->cOwner].diplo_status[veh_fc_id] & DIPLO_PACT)
            && (!(*GamePreferences & PREF_BSC_DONT_QUICK_MOVE_ALLY_VEH)
            || !(Factions[MapWin->cOwner].diplo_status[veh_fc_id] & DIPLO_PACT))) {
                if (base_id < 0 || (veh_fc_id == Bases[base_id].faction_id)
                || Factions[veh_fc_id].diplo_status[Bases[base_id].faction_id] & DIPLO_PACT) {
                    move_delay = 0;
                }
            }
            bool shift, toggle;
            shift = shift_key_down();
            toggle = true;
            if (shift) {
                move_delay = 0;
            }
            if (cost_val || veh_fc_id == MapWin->cOwner) {
                if (cost_val || (!(Vehs[veh_id].state & (VSTATE_EXPLORE|VSTATE_ON_ALERT))
                && Vehs[veh_id].order != ORDER_MOVE_TO)) {
                    if (shift || (*GameMorePreferences & MPREF_ADV_QUICK_MOVE_ALL_VEH)) {
                        toggle = true;
                    } else if (!(*GameMorePreferences & MPREF_ADV_QUICK_MOVE_VEH_ORDERS)) {
                        toggle = false;
                    } else if (veh_fc_id != MapWin->cOwner && veh_triad != TRIAD_AIR) {
                        toggle = false;
                    } else if (veh_fc_id == MapWin->cOwner
                    && !(Vehs[veh_id].state & (VSTATE_EXPLORE|VSTATE_ON_ALERT))
                    && Vehs[veh_id].order != ORDER_MOVE_TO) {
                        toggle = false;
                    }
                }
            }
            if (toggle) {
                move_delay = 0;
                MapWin->field_23BF0 = 1;
                if (veh_fc_id == MapWin->cOwner) {
                    MapWin->field_23BF0 = 2;
                    Console_cursor_next(MapWin, tgt_x, tgt_y);
                }
            }
        }
    }
    if (visual_tgl && !ctrl_key_down()
    && (move_delay || !(*GamePreferences & PREF_ADV_NO_CENTER_VEH_ORDERS))) {
        Console_focus(MapWin, veh_x, veh_y, veh_fc_id);
    }
    stack_veh(veh_id, 1);
    if (move_delay) {
        if (veh_fc_id == MapWin->cOwner) {
            int delay = mapsq(tgt_x, tgt_y)->alt_level()
                <= mapsq(veh_x, veh_y)->alt_level() ? 200 : 300;
            FX_engine_pitch(Sounds, delay);
        }
        veh_scoot(veh_id, veh_x, veh_y, offset, 0);
        if (veh_fc_id == MapWin->cOwner) {
            FX_engine_pitch(Sounds, 0);
        }
    } else if (visual_tgl) {
        veh_scoot(veh_id, veh_x, veh_y, offset, 0x4000);
    } else {
        Vehs[veh_id].rotate_angle = offset;
    }
    if (veh_fc_id) {
        unspot_stack(veh_id);
    } else {
        Vehs[veh_id].visibility &= mapsq(tgt_x, tgt_y)->visibility;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if ((1 << i) & Vehs[veh_id].visibility) {
                base_find_2(tgt_x, tgt_y, i);
                if (*BaseFindDist >= 8 && whose_territory(i, tgt_x, tgt_y, 0, 0) != i) {
                    Vehs[veh_id].visibility &= ~(1 << i);
                }
            }
        }
        tgt_owner = MaxPlayerNum;
    }
    tgt_sq = mapsq(tgt_x, tgt_y);
    if (tgt_sq->items & BIT_BUNKER && stack_veh_id < 0) {
        if (Vehs[veh_id].triad() == TRIAD_LAND && Vehs[veh_id].plan() != PLAN_PROBE) {
            tgt_owner = whose_territory(veh_fc_id, tgt_x, tgt_y, 0, 0);
            if (tgt_owner >= 0 && tgt_owner != veh_fc_id
            && !(Factions[veh_fc_id].diplo_status[tgt_owner] & DIPLO_PACT)) {
                veh_skip(veh_id);
                if (veh_fc_id == MapWin->cOwner) {
                    NetMsg_pop(NetMsg, "BUNKERCAPTURED", 5000, 0, 0);
                }
            }
        }
    }
    assert(base_id == base_at(tgt_x, tgt_y));
    if (base_id >= 0 && veh_fc_id != tgt_fc_id && !veh_fc_id) {
        for (int iter_val = 0; iter_val <= 100; iter_val++) {
            // Fix: remove code that checked the wrong faction definition
            // and always ignored the intention not to destroy free facilities.
            // First does 100 attempts at destroying random facility before production loss.
            if (iter_val < 100) {
                int rnd_id = game_rand() % 70;
                if (rnd_id <= 1 || rnd_id >= 65) {
                    continue;
                }
                if (rnd_id == FAC_PRESSURE_DOME && is_ocean(mapsq(tgt_x, tgt_y))) {
                    continue;
                }
                if (!has_fac_built((FacilityId)rnd_id, base_id)
                || has_free_facility((FacilityId)rnd_id, tgt_fc_id)) {
                    continue;
                }
                set_fac((FacilityId)rnd_id, base_id, 0);
                parse_says(1, Facility[rnd_id].name, -1, -1);
            }
            parse_says(2, Vehs[veh_id].name(), -1, -1);
            if (visual_tgl && tgt_fc_id == MapWin->cOwner) {
                parse_says(0, Bases[base_id].name, -1, -1);
                NetMsg_pop(NetMsg, iter_val < 100 ? "WORMSATTACK1" : "WORMSATTACK", 5000, 0, 0);
            }
            kill(veh_id);
            int num = 0;
            for (int i = 0; i < 25; i++) {
                int px = wrap(tgt_x + TableOffsetX[i]);
                int py = tgt_y + TableOffsetY[i];
                if (on_map(px, py)) {
                    veh_id = stack_fix(veh_at(px, py));
                    if (veh_id >= 0 && !Vehs[veh_id].faction_id) {
                        veh_id = veh_top(veh_id);
                        int next_id = veh_id;
                        while (next_id >= 0) {
                            next_id = Vehs[veh_id].next_veh_id_stack;
                            if (num >= Factions[tgt_fc_id].clean_minerals_modifier) {
                                veh_kill(veh_id);
                                if (next_id >= 0 && veh_id < next_id) {
                                    --next_id;
                                }
                            } else {
                                ++num;
                            }
                            veh_id = next_id;
                        }
                    }
                }
            }
            if (*CurrentTurn >= 10) {
                --Bases[base_id].pop_size;
                if (is_objective(base_id)) {
                    Bases[base_id].pop_size = max(1, (int)Bases[base_id].pop_size);
                }
            }
            if (*CurrentTurn < 10 || Bases[base_id].pop_size > 0) {
                if (iter_val >= 100) {
                    Bases[base_id].minerals_accumulated = 0;
                }
                draw_tile(Bases[base_id].x, Bases[base_id].y, 2);
                BaseWin_check_base(BaseWin, base_id);
            } else {
                mod_base_kill(base_id);
                draw_map(1);
                mod_eliminate_player(tgt_fc_id, 0);
                draw_map(1);
            }
            goto MOV_END;
        }
    }
    stack_put(veh_id, tgt_x, tgt_y);
    if (base_id < 0) {
        if (tgt_at_sea && Vehs[veh_id].triad() == TRIAD_SEA) {
            int find_count = 0;
            int find_fc_id = 0;
            for (int i = 0; i < 25; i++) {
                int px = wrap(tgt_x + TableOffsetX[i]);
                int py = tgt_y + TableOffsetY[i];
                if (on_map(px, py)) {
                    int iter_id = stack_fix(veh_at(px, py));
                    for (; iter_id >= 0; iter_id = Vehs[iter_id].next_veh_id_stack) {
                        if (can_arty(Vehs[iter_id].unit_id, 1) && !Vehs[iter_id].triad()) {
                            int iter_fc = Vehs[iter_id].faction_id;
                            if (iter_fc != veh_fc_id) {
                                if (Factions[veh_fc_id].diplo_status[iter_fc] & DIPLO_VENDETTA
                                && ((Vehs[veh_id].faction_id == Vehs[iter_id].faction_id
                                && (Vehs[veh_id].is_invisible_lurker()))
                                || Vehs[veh_id].visibility & (1 << iter_fc))) {
                                    ++find_count;
                                    find_fc_id = iter_fc;
                                    tgt_owner = iter_fc;
                                    spot_stack(iter_id, veh_fc_id);
                                } else {
                                    find_fc_id = tgt_owner;
                                }
                            }
                        }
                    }
                }
            }
            if (find_count) {
                Vehs[veh_id].moves_spent += find_count * Rules->move_rate_roads;
                if (veh_fc_id == MapWin->cOwner) {
                    parse_says(0, MFactions[find_fc_id].adj_name_faction, -1, -1);
                    NetMsg_pop(NetMsg, "GIBRALTAR", 5000, 0, 0);
                }
            }
        }
    } else if (veh_fc_id != tgt_fc_id && !(Factions[veh_fc_id].diplo_status[tgt_fc_id] & DIPLO_PACT)) {
        if (*ExpansionEnabled) {
            if (!is_battle_ogre(Vehs[veh_id].unit_id)) {
                Vehs[veh_id].damage_taken = 0;
            }
        }
        *CurrentVehID = veh_id;
        act_of_aggression(veh_fc_id, tgt_fc_id);
        mod_capture_base(base_id, veh_fc_id, 0);
        veh_id = *CurrentVehID;
    }
    if (base_id >= 0 && veh_cargo(veh_id)) {
        if (veh_fc_id == MapWin->cOwner || (*MultiplayerActive && is_human(veh_fc_id))) {
            if (Vehs[veh_id].order != ORDER_MOVE_TO
            || (Vehs[veh_id].waypoint_x[0] == tgt_x && Vehs[veh_id].waypoint_y[0] == tgt_y)) {
                for (int cur_id = veh_top(veh_id); cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
                    if (Vehs[cur_id].faction_id == veh_fc_id && Vehs[cur_id].order == ORDER_SENTRY_BOARD) {
                        if (Vehs[cur_id].waypoint_x[0] < 0 || Vehs[cur_id].waypoint_x[0] == veh_id) {
                            Vehs[cur_id].order = ORDER_NONE;
                        }
                    }
                }
            }
        }
    }
    if (veh_fc_id == MapWin->cOwner && *GamePreferences & PREF_AUTO_WAKE_VEH_TRANS_REACH_LAND) {
        if (Vehs[veh_id].triad() == TRIAD_SEA && veh_cargo(veh_id) && tgt_at_sea) {
            if ((veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent <= 0)
            && (Vehs[veh_id].order != ORDER_MOVE_TO
            || (Vehs[veh_id].waypoint_x[0] == tgt_x && Vehs[veh_id].waypoint_y[0] == tgt_y))) {
                for (int i = 0; i < 8; i++) {
                    int px = wrap(tgt_x + BaseOffsetX[i]);
                    int py = tgt_y + BaseOffsetY[i];
                    MAP* psq = mapsq(px, py);
                    if (psq && (!is_ocean(psq) || psq->base_who() >= 0)) {
                        int cur_id = veh_top(veh_id);
                        for (; cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
                            if (Vehs[cur_id].triad() == TRIAD_LAND
                            && Vehs[cur_id].order == ORDER_SENTRY_BOARD
                            && veh_speed(cur_id, 0) - Vehs[cur_id].moves_spent > 0) {
                                Vehs[cur_id].order = ORDER_NONE;
                                MapWin->iUnit = cur_id;
                            }
                        }
                    }
                }
            }
        }
    }
    if (veh_triad == TRIAD_LAND && veh_at_sea && !tgt_at_sea) {
        ++Factions[veh_fc_id].region_total_combat_units[tgt_region];
    }
MOV_UPKEEP:
    if (!Vehs[veh_id].offense_value()) {
        veh_demote(veh_id);
    }
    tgt_sq = mapsq(tgt_x, tgt_y);
    assert(tgt_sq);
    if (!veh_fc_id && tgt_sq->is_fungus()) {
        if (Vehs[veh_id].damage_taken > 0) {
            --Vehs[veh_id].damage_taken;
        }
    }
    spot_all(veh_id, 1);
    if (veh_fc_id != MapWin->cOwner
    && !is_human(veh_fc_id)
    && !(*GameState & STATE_OMNISCIENT_VIEW)
    && visual_tgl
    && !MapWin->field_23BF0
    && !shift_key_down()
    && !*MultiplayerActive) {
        clock_wait(!move_delay ? 5 : 100);
    }
    if (Vehs[veh_id].triad() == TRIAD_AIR) {
        tgt_no_airbase = tgt_sq->base_who() < 0 && !(tgt_sq->items & BIT_AIRBASE);
        for (int cur_id = veh_top(veh_id); tgt_no_airbase && cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
            if (has_abil(Vehs[cur_id].unit_id, ABL_CARRIER)) {
                tgt_no_airbase = 0;
            }
        }
        bool fuel_check = tgt_no_airbase;
        if (!fuel_check && is_human(veh_fc_id)) {
            int moves = veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent;
            if (Vehs[veh_id].range() == 1 && moves > 0 && move_check_fuel) {
                fuel_check = true;
            } else if (moves > 0) {
                fuel_check = (clamp(moves, 0, 999) > Rules->move_rate_roads)
                    || mod_zoc_veh(tgt_x, tgt_y, veh_fc_id);
            }
        }
        if (fuel_check) {
            int val_turns = Vehs[veh_id].movement_turns + 1;
            int val_range = Vehs[veh_id].range();
            if (val_range) {
                auto ret_home = [&]() -> bool {
                    return (veh_fc_id == MapWin->cOwner
                    && (*GamePreferences & PREF_AUTO_AIR_VEH_RET_HOME_FUEL_RNG
                    || (Vehs[veh_id].state & (VSTATE_EXPLORE|VSTATE_ON_ALERT)))
                    && (Vehs[veh_id].state & (VSTATE_UNK_1000000|VSTATE_ON_ALERT))
                    != (VSTATE_UNK_1000000|VSTATE_ON_ALERT)
                    && ((Vehs[veh_id].state & (VSTATE_UNK_2000000|VSTATE_ON_ALERT))
                    != (VSTATE_UNK_2000000|VSTATE_ON_ALERT)
                    || Vehs[veh_id].patrol_current_point >= Vehs[veh_id].waypoint_count));
                };
                if (!is_human(veh_fc_id) || ret_home()) {
                    action_home(veh_id, 1);
                }
                if (veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent <= 0) {
                    if (val_turns < val_range) {
                        if (val_turns >= val_range / 2) {
                            if (!is_human(veh_fc_id) || (ret_home() && Vehs[veh_id].order != ORDER_MOVE_TO)) {
                                action_home(veh_id, 0);
                            }
                        }
                        ++Vehs[veh_id].movement_turns;
                    } else {
                        Vehs[veh_id].state |= VSTATE_HAS_MOVED;
                        if (visual_tgl) {
                            boom(Vehs[veh_id].x, Vehs[veh_id].y, 128);
                        }
                        if (val_range != 1 || Vehs[veh_id].chassis_type() != CHS_COPTER) {
                            // Fix: adjust incorrect upper bounds checking
                            Vehs[veh_id].damage_taken = Vehs[veh_id].max_hitpoints();
                        } else {
                            Vehs[veh_id].damage_taken += 3 * Vehs[veh_id].reactor_type();
                        }
                        if (Vehs[veh_id].cur_hitpoints() <= 0) {
                            kill(veh_id);
                            if (visual_tgl && veh_fc_id == MapWin->cOwner) {
                                NetMsg_pop(NetMsg, "PLANECRASH", 5000, 0, 0);
                            }
                            goto MOV_END;
                        }
                        Vehs[veh_id].movement_turns = 0;
                        draw_tile(Vehs[veh_id].x, Vehs[veh_id].y, 2);
                        if (visual_tgl && veh_fc_id == MapWin->cOwner) {
                            NetMsg_pop(NetMsg, "CHOPPERFUEL", 5000, 0, 0);
                        }
                    }
                }
            }
        } else {
            Vehs[veh_id].movement_turns = 0;
            veh_skip(veh_id);
            veh_demote(veh_id);
            draw_tile(Vehs[veh_id].x, Vehs[veh_id].y, 2);
        }
    }
    if (tgt_sq->items & BIT_MONOLITH && veh_triad != TRIAD_AIR && !move_fight) {
        if (offset >= 0 || (Vehs[veh_id].offense_value()
        && (Vehs[veh_id].damage_taken || !(Vehs[veh_id].state & VSTATE_MONOLITH_UPGRADED)))) {
            mod_monolith(veh_id);
        }
    }
    if (goody_val && veh_triad != TRIAD_AIR && veh_fc_id && mod_goody_box(veh_id)) {
        goto MOV_END;
    }
    if (Vehs[veh_id].plan() == PLAN_ARTIFACT && offset < 8 && base_id < 0
    && mod_stack_check(veh_id, 1, -1, -1, -1) == 1) {
        for (int i = 0; i < 8; i++) {
            int px = wrap(tgt_x + BaseOffsetX[i]);
            int py = tgt_y + BaseOffsetY[i];
            if (!on_map(px, py) || is_ocean(mapsq(px, py))) {
                continue;
            }
            int iter_id = stack_fix(veh_at(px, py));
            if (iter_id >= 0) {
                int iter_fc_id = Vehs[iter_id].faction_id;
                if (iter_fc_id && iter_fc_id != veh_fc_id) {
                    if (!(Factions[veh_fc_id].diplo_status[iter_fc_id] & DIPLO_PACT)) {
                        if (mod_stack_check(iter_id, 1, -1, -1, -1) > mod_stack_check(iter_id, 2, 12, -1, -1)) {
                            Vehs[veh_id].faction_id = Vehs[iter_id].faction_id;
                            Vehs[veh_id].order = ORDER_NONE;
                            draw_tile(tgt_x, tgt_y, 2);
                            owner_set(tgt_x, tgt_y, Vehs[veh_id].faction_id);
                            if (veh_fc_id == MapWin->cOwner) {
                                MapWin->iUnit = -1;
                                *PluralDefault = MFactions[iter_fc_id].is_noun_plural;
                                *GenderDefault = MFactions[iter_fc_id].noun_gender;
                                parse_says(0, MFactions[iter_fc_id].noun_faction, -1, -1);
                                popp(ScriptFile, "TOOKARTIFACT", 0, "artstolen_sm.pcx", 0);
                            } else if (Vehs[iter_id].faction_id == MapWin->cOwner) {
                                parse_says(0, Vehs[iter_id].name(), -1, -1);
                                *PluralDefault = MFactions[veh_fc_id].is_noun_plural;
                                *GenderDefault = MFactions[veh_fc_id].noun_gender;
                                parse_says(1, MFactions[veh_fc_id].noun_faction, -1, -1);
                                popp(ScriptFile, "TAKENARTIFACT", 0, "artstolen_sm.pcx", 0);
                            }
                            veh_fc_id = Vehs[veh_id].faction_id;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (Vehs[veh_id].plan() == PLAN_ARTIFACT && base_id >= 0 && Vehs[veh_id].movement_turns) {
        if (Vehs[veh_id].movement_turns == 1) {
            if (mod_study_artifact(veh_id)) {
                veh_id = -1;
                goto MOV_END;
            }
        } else if (Vehs[veh_id].movement_turns == 2) {
            int mins = 10 * Units[Vehs[veh_id].unit_id].cost / 2 + Bases[base_id].minerals_accumulated;
            Bases[base_id].minerals_accumulated = mins;
            Bases[base_id].minerals_accumulated_2 = mins;
            Bases[base_id].production_id_last = Bases[base_id].queue_items[0];
            kill(veh_id);
            veh_id = -1;
            BaseWin_check_base(BaseWin, base_id);
            goto MOV_END;
        }
    }
    if (veh_at_sea && !tgt_at_sea && Vehs[veh_id].plan() == PLAN_COLONY) {
        Factions[veh_fc_id].region_flags[tgt_region] |= 2u;
    }
    if (move_no_base) {
        sleep(veh_id);
        veh_skip(veh_id);
        for (int cur_id = veh_top(veh_id); cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
            if (!veh_cargo(cur_id)) {
                if (Vehs[cur_id].moves_spent <= veh_speed(cur_id, 0)) { // might evaluate always true
                    stack_veh(cur_id, 0);
                    if (Vehs[veh_id].waypoint_x[0] == cur_id) {
                        MapWin->iUnit = cur_id;
                        Vehs[cur_id].order = ORDER_NONE;
                        break;
                    }
                } else {
                    Vehs[cur_id].order = ORDER_NONE;
                }
            }
        }
        draw_tile(tgt_x, tgt_y, 2);
    }
    if (!is_human(veh_fc_id)
    && Vehs[veh_id].plan() == PLAN_NAVAL_TRANSPORT
    && is_ocean(mapsq(tgt_x, tgt_y))
    && base_id < 0) {
        if (veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent <= 0) {
            if (mod_zoc_any(Vehs[veh_id].x, Vehs[veh_id].y, veh_fc_id)
            || (!thinker_move_upkeep(veh_fc_id)
            && (Vehs[veh_id].visibility & FactionStatus[0] || Vehs[veh_id].status_icon == 'u'))) {
                for (int cur_id = veh_top(veh_id); cur_id >= 0; cur_id = Vehs[cur_id].next_veh_id_stack) {
                    if (Vehs[cur_id].order == ORDER_SENTRY_BOARD && Vehs[cur_id].waypoint_x[0] == veh_id) {
                        Vehs[veh_id].order = ORDER_NONE; // stops transport order
                    }
                }
            }
            int px = tgt_x;
            int py = tgt_y;
            if (Vehs[veh_id].order == ORDER_MOVE_TO) {
                px = (tgt_x + Vehs[veh_id].waypoint_x[0]) / 2;
                py = (tgt_y + Vehs[veh_id].waypoint_y[0]) / 2;
                px = px - (px & 1) + (py & 1);
                if (!is_ocean(mapsq(px, py))) {
                    px = tgt_x;
                    py = tgt_y;
                }
            }
            if (is_ocean(mapsq(px, py))) {
                add_goal(veh_fc_id, AI_GOAL_UNK_1, 2, px, py, -1);
            }
        }
    }
    if (veh_fc_id == MapWin->cOwner) {
        if (Vehs[veh_id].order == ORDER_MOVE_TO || (Vehs[veh_id].state & (VSTATE_EXPLORE|VSTATE_ON_ALERT))) {
            ++Vehs[veh_id].iter_count;
            if (Vehs[veh_id].state & VSTATE_UNK_10000 && offset == (Vehs[veh_id].rotate_angle ^ 4)) {
                Vehs[veh_id].iter_count += 8;
            }
            if (Vehs[veh_id].iter_count >= 32) {
                parse_says(0, Vehs[veh_id].name(), -1, -1);
                if (*MultiplayerActive || X_pop("KEEPMOVING", 0)) {
                    Vehs[veh_id].order = ORDER_NONE;
                    Vehs[veh_id].state &= ~(VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
                }
                Vehs[veh_id].iter_count = 0;
            }
        }
    }
    if (offset < 0 || offset >= 8) {
        Vehs[veh_id].state &= ~VSTATE_UNK_10000;
    } else {
        Vehs[veh_id].rotate_angle = offset;
        Vehs[veh_id].state |= VSTATE_UNK_10000;
    }
MOV_END:
    if (veh_id >= 0 && *VehCount == prev_vehcount) {
        if (move_result) {
            if (*VehAttackFlags & 2) {
                if (offset >= 0 && offset < 8) {
                    if (Vehs[veh_id].state & VSTATE_UNK_8
                    && Vehs[veh_id].x == tgt_x
                    && Vehs[veh_id].y == tgt_y
                    && !mod_zoc_veh(tgt_x, tgt_y, veh_fc_id)) {
                        Vehs[veh_id].state &= ~VSTATE_UNK_8;
                    }
                }
                if (*MultiplayerActive && !*ControlTurnC
                && Vehs[veh_id].order == ORDER_MOVE_TO
                && Vehs[veh_id].x == Vehs[veh_id].waypoint_x[0]
                && Vehs[veh_id].y == Vehs[veh_id].waypoint_y[0]) {
                    Vehs[veh_id].order = ORDER_NONE;
                    if ((Vehs[veh_id].state & (VSTATE_UNK_2000000|VSTATE_ON_ALERT)) == (VSTATE_UNK_2000000|VSTATE_ON_ALERT)) {
                        int num = Vehs[veh_id].patrol_current_point;
                        if (num < Vehs[veh_id].waypoint_count && num >= 0 && num < 3) {
                            Vehs[veh_id].order = ORDER_MOVE_TO;
                            Vehs[veh_id].patrol_current_point = num + 1;
                            Vehs[veh_id].waypoint_x[0] = Vehs[veh_id].waypoint_x[num + 1];
                            Vehs[veh_id].waypoint_y[0] = Vehs[veh_id].waypoint_y[num + 1];
                            Vehs[veh_id].state &= ~(VSTATE_UNK_2000000|VSTATE_UNK_10000|VSTATE_ON_ALERT);
                        }
                    }
                }
            }
        } else {
            Vehs[veh_id].order = ORDER_NONE;
            if (!is_human(veh_fc_id) || (Vehs[veh_id].state & (VSTATE_EXPLORE|VSTATE_ON_ALERT))) {
                ++Vehs[veh_id].iter_count;
                if (Vehs[veh_id].iter_count >= 24 || is_human(veh_fc_id)) {
                    Vehs[veh_id].iter_count = 0;
                    veh_skip(veh_id);
                }
            }
            if (!(*VehAttackFlags & 2) && *MultiplayerActive && !*ControlTurnC) {
                synch_veh(veh_id);
                NetDaemon_await_synch(NetState);
            }
        }
        return move_result;
    } else {
        return 0;
    }
}


