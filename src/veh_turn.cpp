
#include "veh_turn.h"

void __cdecl mod_enemy_turn(int faction_id) {
    debug("enemy_turn %d %d\n", *CurrentTurn, faction_id);
    for (int iter_type = 0; iter_type < 10; ++iter_type) {
        for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
            VEH* veh = &Vehs[veh_id];
            if (veh_id >= *VehCount || veh->faction_id != faction_id) {
                continue;
            }
            switch (iter_type) {
            case 0:
                if (!can_arty(veh->unit_id, 1)) {
                    continue;
                }
                break;
            case 1:
                if (veh->plan() != PLAN_DEFENSE
                && (veh->plan() != PLAN_NAVAL_TRANSPORT || !mod_stack_check(veh_id, 3, 0, -1, -1))) {
                    continue;
                }
                break;
            case 2:
                if (veh->triad() != TRIAD_AIR || veh->plan() == PLAN_AIR_SUPERIORITY) {
                    continue;
                }
                break;
            case 3:
                if (veh->plan() != PLAN_COMBAT) {
                    continue;
                }
                break;
            case 4:
                if (veh->damage_taken) {
                    continue;
                }
                break;
            default:
                break;
            }
            if (iter_type >= 7 || (!has_abil(veh->unit_id, ABL_DROP_POD)
            && veh->plan() != PLAN_AIR_SUPERIORITY && Units[veh->unit_id].group_id != 21)) {
                if (iter_type != 8 || (veh->plan() == PLAN_NAVAL_TRANSPORT && faction_id)) {
                    int iter = 0;
                    while (veh_id >= 0 && veh->order != ORDER_SENTRY_BOARD) {
                        if (iter_type != 8 || iter != 0) {
                            if (veh_speed(veh_id, 0) - veh->moves_spent <= 0) {
                                break;
                            }
                        }
                        int num = *VehCount;
                        if (mod_enemy_veh(veh_id) != VEH_SYNC || num != *VehCount) {
                            break;
                        }
                        if (veh->order != ORDER_MOVE_TO && veh->triad() != TRIAD_AIR) {
                            ++iter; // increase twice as fast
                        }
                        if (++iter > 32) {
                            veh_skip(veh_id);
                            break;
                        }
                    }
                }
            }
        }
    }
    Factions[faction_id].player_flags &= ~PFLAG_UNK_10000;
    // Additional goal planning after movement upkeep
    if (thinker_move_upkeep(faction_id)) {
        point_max_queue_t scores;
        for (const auto& m : mapdata) {
            MAP* sq;
            int x = m.first.x;
            int y = m.first.y;
            if (m.second.unit_path > 0 && (sq = mapsq(x, y))
            && !sq->is_base() && (sq->owner == faction_id || at_war(faction_id, sq->owner))) {
                if (near_sea_coast(x, y)) {
                    int score = 4*m.second.unit_path - m.second.get_enemy_dist()
                        + 8*mapnodes.count({x, y, NODE_GOAL_RAISE_LAND});
                    scores.push({x, y, score});
                }
            }
        }
        int score_limit = clamp(7 + plans[faction_id].land_combat_units/16, 15, 25);
        int num = 0;
        while (scores.size() > 0) {
            auto p = scores.top();
            if (p.score >= score_limit && ++num <= 8) {
                debug("raise_land %d %d %2d %2d score: %d\n",
                    *CurrentTurn, faction_id, p.x, p.y, p.score);
                add_goal(faction_id, AI_GOAL_RAISE_LAND, 5, p.x, p.y, -1);
            }
            scores.pop();
        }
    }
}

int __cdecl mod_enemy_veh(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    debug_ver("enemy_veh %d %2d %2d %s\n", veh_id, veh->x, veh->y, veh->name());
    bool net_sync = *MultiplayerActive && veh->faction_id == *CurrentPlayerFaction;
    if (*MultiplayerActive) {
        *VehAttackFlags = 3;
    }
    do_all_non_input();
    bool skip = veh->moves_spent && veh->order == ORDER_MOVE_TO
        && veh->plan() != PLAN_NAVAL_TRANSPORT && !(veh->state & VSTATE_EXPLORE)
        && !mod_zoc_any(veh->x, veh->y, veh->faction_id);

    if (!skip && mod_enemy_move(veh_id) != VEH_SYNC) {
        return VEH_SKIP;
    }
    if (skip || veh_speed(veh_id, 0) - veh->moves_spent > 0) {
        if (net_sync) {
            synch_veh(veh_id);
            NetDaemon_await_synch(NetState);
            if (NetDaemon_action(NetState, veh_id, 1)) {
                return VEH_SKIP;
            }
        } else {
            if (!action(veh_id)) {
                veh_skip(veh_id);
            }
        }
        return VEH_SYNC;
    } else {
        if (net_sync) {
            synch_veh(veh_id);
            NetDaemon_await_synch(NetState);
            NetDaemon_unlock_veh(NetState);
        }
        return VEH_SKIP;
    }
}

int __cdecl mod_enemy_move(int veh_id) {
    assert(veh_id >= 0 && veh_id < *VehCount);
    if (veh_id < 0) {
        return enemy_move(veh_id); // fallback to enemy_move, special case for tutorials
    }
    VEH* veh = &Vehs[veh_id];
    MAP* sq;
    bool plr_unit = veh->plr_owner();
    debug("enemy_move %d %2d %2d %s\n", veh_id, veh->x, veh->y, veh->name());

    if (!(sq = mapsq(veh->x, veh->y))) {
        return VEH_SYNC;
    }
    if (!veh->faction_id && !(*MultiplayerActive && veh->faction_id == *CurrentPlayerFaction)) {
        return mod_alien_move(veh_id);
    }
    if (plr_unit) {
        if (!*CurrentBase) {
            int base_id = base_find_3(veh->x, veh->y, veh->faction_id, -1, -1, -1);
            if (base_id >= 0) {
                set_base(base_id);
            }
        }
        if (conf.manage_player_units && move_upkeep_faction != veh->faction_id) {
            move_upkeep(veh->faction_id, UM_Player);
        }
    }
    if (thinker_move_upkeep(veh->faction_id)) {
        int triad = veh->triad();
        if (plr_unit && veh->is_patrol_order()) {
            if (veh->need_refuel() && (sq->is_airbase()
            || mod_stack_check(veh_id, 6, ABL_CARRIER, -1, -1))) {
                veh->apply_refuel();
                return mod_veh_skip(veh_id);
            }
            // fallback to enemy_move
        } else if (!plr_unit && veh->flags & (VFLAG_LURKER|VFLAG_INVISIBLE)) {
            return mod_veh_skip(veh_id);
        } else if (veh->is_colony()) {
            return colony_move(veh_id);
        } else if (veh->is_former()) {
            return former_move(veh_id);
        } else if (veh->is_supply()) {
            return crawler_move(veh_id);
        } else if (veh->is_artifact()) {
            return artifact_move(veh_id);
        } else if (triad == TRIAD_SEA && veh_cargo(veh_id) > 0) {
            return trans_move(veh_id);
        } else if (veh->is_planet_buster()) {
            return nuclear_move(veh_id);
        } else {
            return combat_move(veh_id);
        }
    }
    int num = *VehCount;
    int iter = veh->iter_count;
    int value = enemy_move(veh_id);
    if (value == VEH_SKIP && veh->iter_count == iter && num == *VehCount) {
        // avoid infinite loops if enemy_move does not update the vehicle
        return VEH_SYNC;
    }
    return value;
}

int __cdecl mod_alien_base(int veh_id, int x, int y) {
    VEH* veh = &Vehs[veh_id];
    MAP* veh_sq = mapsq(x, y);
    int best_base_id = -1;
    int best_dist = 9999;
    *BaseFindDist = 9999;
    if (!veh_sq) {
        assert(0);
        return -1;
    }
    int cur_region = veh_sq->region;

    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (veh->unit_id == BSC_SEALURK) {
            if (cur_region < MaxRegionLandNum
            && cur_region != mapsq(base->x, base->y)->region
            && !base_on_sea(i, cur_region)) {
                continue;
            }
        } else if (cur_region < MaxRegionLandNum
        && cur_region != mapsq(base->x, base->y)->region) {
            continue;
        }
        int dist = 32 * vector_dist(x, y, base->x, base->y)
            / (base->mineral_intake_2 + base->energy_intake_2 + 32);
        if (stack_check(veh_at(base->x, base->y), 2, 12, -1, -1)) {
            dist /= 2;
        }
        if (i == veh->home_base_id) {
            dist /= 2;
        }
        if (veh->order_auto_type != base->faction_id
        && Factions[base->faction_id].SE_planet > 0
        && !base->eco_damage) {
            dist *= 2;
        }
        if (dist <= best_dist) {
            best_dist = dist;
            best_base_id = i;
        }
    }
    assert(best_base_id == alien_base(veh_id, x, y));
    assert(best_dist == *BaseFindDist);
    if (best_base_id >= 0) {
        *BaseFindDist = best_dist;
    }
    return best_base_id;
}

int __cdecl mod_alien_move(int veh_id) {
    VEH* const veh = &Vehs[veh_id];
    const int moves_left = veh_speed(veh_id, 0) - veh->moves_spent;
    if (moves_left <= 0) {
        return 0;
    }
    if (veh->flags & (VFLAG_INVISIBLE|VFLAG_LURKER)) {
        veh_skip(veh_id);
        return 0;
    }
    if (veh->unit_id == BSC_UNITY_SCOUT_CHOPPER) {
        veh_skip(veh_id);
        return 0;
    }
    const int player_id = *CurrentPlayerFaction;
    const int veh_x = veh->x;
    const int veh_y = veh->y;
    MAP* const veh_sq = mapsq(veh_x, veh_y);
    int base_id = mod_alien_base(veh_id, veh_x, veh_y);
    int base_faction = 0;
    int base_region = -1; // Fix: added placeholder value when no base found
    int move_offset = -1;

    if (base_id < 0) {
        if (!mod_zoc_any(veh_x, veh_y, 0)) {
            kill(veh_id);
            return 1;
        }
        *BaseFindDist = 99;
    } else {
        base_faction = Bases[base_id].faction_id;
        base_region = mapsq(Bases[base_id].x, Bases[base_id].y)->region;
    }
    veh->movement_turns++;

    if (veh_cargo(veh_id) && mod_stack_check(veh_id, 3, 0, -1, -1)) {
        // Any transport carrying land units
        if (veh->movement_turns > 24) {
            kill(veh_id);
            return 1;
        }
        // Fix: added base_id bounds checking
        if (base_id < 0) {
            assert(0);
            return 0;
        }
        BASE* base = &Bases[base_id];
        for (int dir = 0; dir < 8; dir++) {
            int nx = wrap(veh_x + BaseOffsetX[dir]);
            int ny = veh_y + BaseOffsetY[dir];
            if (!on_map(nx, ny)) {
                continue;
            }
            MAP* sq = mapsq(nx, ny);
            if (sq->veh_who() > 0) {
                if (is_ocean(sq)) {
                    veh->order = ORDER_AI_GO_TO;
                    veh->waypoint_x[0] = nx;
                    veh->waypoint_y[0] = ny;
                    return 0;
                }
            } else if (!is_ocean(sq) && sq->region == base_region) {
                if (base_id < 0 || map_range(nx, ny, base->x, base->y) <= 6) {
                    if (veh_id >= 0) {
                        int cur = veh_top(veh_id);
                        for (; cur >= 0; cur = Vehs[cur].next_veh_id_stack) {
                            Vehs[cur].order = ORDER_NONE;
                            Vehs[cur].visibility |= sq->visibility;
                        }
                    }
                    veh_skip(veh_id);
                    if (base_faction == player_id
                    && ((veh->faction_id == player_id
                    && (veh->flags & (VFLAG_INVISIBLE|VFLAG_LURKER)) != (VFLAG_INVISIBLE|VFLAG_LURKER))
                    || veh->visibility & (1 << player_id))) {
                        if (!Console_focus(MapWin, nx, ny, player_id)) {
                            draw_tile(nx, ny, 2);
                        }
                        parse_says(0, base->name, -1, -1);
                        if (*MultiplayerActive) {
                            if (veh->unit_id == BSC_SPORE_LAUNCHER) {
                                NetMsg_pop(NetMsg, "SPORELAUNCHSWARM", 3000, 0, 0);
                            } else {
                                NetMsg_pop(NetMsg, "MINDWORMSWARM", 3000, 0, 0);
                            }
                        } else {
                            if (veh->unit_id == BSC_SPORE_LAUNCHER) {
                                popp(ScriptFile, "SPORELAUNCHSWARM", 0, "sporlnch_sm.pcx", 0);
                            } else {
                                popp(ScriptFile, "MINDWORMSWARM", 0, "mindworm_sm.pcx", 0);
                            }
                        }
                    }
                    return 0;
                }
            }
        }
        set_course(veh_id, 'c', base->x, base->y);
        int tx = veh->waypoint_x[0];
        int ty = veh->waypoint_y[0];
        if (veh->order == ORDER_MOVE_TO) {
            if (!is_ocean(mapsq(tx, ty))) {
                if (map_range(veh_x, veh_y, tx, ty) <= 1) {
                    veh_skip(veh_id);
                }
            }
        }
        return 0;
    }
    if ((*BaseFindDist >= 8 || veh_sq->region != base_region)
    && !mod_zoc_any(veh_x, veh_y, 0)
    && veh->movement_turns > (veh->triad() != TRIAD_LAND ? 16 : 6)
    && !(((uint8_t)veh_id + (uint8_t)*CurrentTurn) & 7)) {
        kill(veh_id);
        return 1;
    }
    int move_angle = 0;
    if (base_id < 0
    || Factions[base_faction].base_count <= 2
    || *CurrentTurn <= 50
    || (veh->order_auto_type != base_faction
    && Factions[base_faction].SE_planet > 0
    && veh->unit_id != BSC_SEALURK)) {
        move_angle = veh->rotate_angle;
    } else {
        veh->waypoint_x[0] = Bases[base_id].x;
        veh->waypoint_y[0] = Bases[base_id].y;
        move_angle = Path_move(Paths, veh_id, 0);
    }
    int best_score = 0;
    int best_offset = -1;
    int score = 0;
    if (can_arty(veh->unit_id, 1)) {
        for (int i = 0; i < 25; i++) {
            int nx = wrap(veh_x + TableOffsetX[i]);
            int ny = veh_y + TableOffsetY[i];
            if (on_map(nx, ny)) {
                MAP* sq = mapsq(nx, ny);
                if (sq->anything_at() >= 0) {
                    int cur = stack_fix(veh_at(nx, ny));
                    if (cur >= 0) {
                        score = 8 * (mod_stack_check(cur, 6, ABL_ARTILLERY, -1, -1)
                            + mod_stack_check(cur, 2, 6, -1, -1));
                        if (!mod_zoc_veh(veh_x, veh_y, 0) || i < 9) {
                            score += mod_stack_check(cur, 4, -1, -1, -1);
                        }
                        if (map_range(veh_x, veh_y, nx, ny) > Rules->artillery_max_rng && score > 1) {
                            score = 1;
                        }
                        if (!i) {
                            score = 0;
                        }
                        if (score > best_score) {
                            best_score = score;
                            best_offset = i;
                        }
                    }
                } else if (!is_ocean(sq)) {
                    if (sq->items & (BIT_SENSOR|BIT_THERMAL_BORE|BIT_FOREST|BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_MINE|BIT_ROAD)) {
                        score = sq->items & BIT_SENSOR ? 32 : 2;
                        if (!i) {
                            score = 0;
                        }
                        if (score > best_score) {
                            best_score = score;
                            best_offset = i;
                        }
                    }
                }
            }
        }
        if (best_offset < 0) {
            if (mod_stack_check(veh_id, 15, -1, -1, -1) <= 1) {
                if (veh_sq->items & BIT_CANAL_COAST) {
                    move_offset = -1;
                }
            }
        } else {
            int tx = wrap(veh_x + TableOffsetX[best_offset]);
            int ty = veh_y + TableOffsetY[best_offset];
            MAP* sq = mapsq(tx, ty);
            assert(sq && sq != veh_sq);
            if (map_range(veh_x, veh_y, tx, ty) <= Rules->artillery_max_rng) {
                if (sq->anything_at() < 0) {
                    debug("alien_move_dest %d %d %d\n", veh_id, tx, ty);
                    action_destroy(veh_id, 0, tx, ty);
                    for (int i = 1; i < MaxPlayerNum; i++) { synch_bit(tx, ty, i); }
                    draw_tile(tx, ty, 2);
                    return 1;
                }
                if (best_offset >= 9) {
                    debug("alien_move_arty %d %d\n", veh_id, best_offset);
                    mod_battle_fight(veh_id, best_offset, 1, 1, 0);
                    return 1;
                }
            }
            if (best_offset >= 25 && sq->anything_at() >= 0) {
                move_offset = -1;
            }
        }
    }
    int best_move_score = 0;
    int morale_level = mod_morale_alien(-1, 0);
    for (int i = 0; i < 8; i++) {
        int tx = wrap(veh_x + BaseOffsetX[i]);
        int ty = veh_y + BaseOffsetY[i];
        if (!on_map(tx, ty)) {
            continue;
        }
        MAP* sq = mapsq(tx, ty);
        bool skip = is_ocean(sq) && veh->triad() != TRIAD_LAND;
        if (skip || !is_ocean(sq)) {
            if (!skip && veh->triad() == TRIAD_SEA) {
                if (veh->unit_id != BSC_SEALURK) {
                    continue;
                }
                if (base_at(tx, ty) < 0) {
                    continue;
                }
                score = 12345;
            }
            if (!skip && veh->triad() == TRIAD_LAND && is_ocean(veh_sq)) {
                if (sq->veh_who() > 0) {
                    continue;
                }
            }
            if (score != 12345) {
                score = 0;
            }
            int val = score;
            if (sq->base_who() > 0) {
                val += 9999;
            } else {
                int iter = veh_at(tx, ty);
                if (iter >= 0) {
                    if (Vehs[iter].faction_id) {
                        if (Vehs[iter].is_native_unit()
                        || (Vehs[iter].triad() == TRIAD_AIR && veh->triad() != TRIAD_AIR)) {
                            continue;
                        }
                        val += 999;
                    }
                }
            }
            val += 4 * bit_count(sq->items & (BIT_SENSOR|BIT_FOREST|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_ROAD));
            if (*DiffLevel > 2) {
                val += (sq->items & BIT_MONOLITH && !(veh->state & VSTATE_MONOLITH_UPGRADED) ? 8 : 0);
            }
            int rnd_val = game_rand() % 8 + val;
            int mov_val = (((uint8_t)move_angle ^ 0xFC) - (uint8_t)i) & 7;
            if (mov_val > 4) {
                mov_val = 8 - mov_val;
            }
            score = mov_val * (morale_level + 1) + rnd_val;
            if (i == move_angle) {
                score += morale_level * morale_level;
            }
            if (score > best_move_score) {
                best_move_score = score;
                move_offset = i;
            }
        }
    }
    int threat_level = 0;
    if (morale_level == 1) {
        threat_level = 1;
    } else if (morale_level == 2) {
        threat_level = 2;
    } else if (morale_level > 2) {
        threat_level = 4;
    }
    if (veh->damage_taken) {
        threat_level *= 2;
    }
    // Fix: added base_id bounds checking
    if (base_id >= 0 && is_human(Bases[base_id].faction_id) && threat_level) {
        if (game_rand() % 4 < (threat_level * (best_move_score < 999) != 0) + 1) {
            int diff_val = (*GameRules & RULES_INTENSE_RIVALRY
                ? 5 : Factions[Bases[base_id].faction_id].diff_level);
            if (clamp(moves_left, 0, 999) <= Rules->move_rate_roads
            && veh->triad() != TRIAD_AIR
            && game_randv(diff_val + 1)) {
                if (veh_sq->items & (BIT_SENSOR|BIT_ECH_MIRROR|BIT_CONDENSER|BIT_FOREST|\
                BIT_SOIL_ENRICHER|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE)) {
                    debug("alien_move_dest %d %d %d\n", veh_id, veh_x, veh_y);
                    action_destroy(veh_id, 0, -1, -1);
                    return 0;
                }
            }
        }
    }
    if (best_move_score < 999 && !veh->moves_spent && veh->damage_taken) {
        if (!veh_sq->is_fungus()) {
            threat_level = (threat_level + 1) / 2;
        }
        if (game_randv(threat_level + 1)) {
            move_offset = -1;
        }
    }
    if (best_move_score < 99) {
        if (veh_cargo(veh_id) && !mod_stack_check(veh_id, 3, 0, -1, -1)) {
            kill(veh_id);
            return 1;
        }
    }
    if (move_offset >= 0) {
        veh->order = ORDER_AI_GO_TO;
        veh->waypoint_x[0] = wrap(veh_x + BaseOffsetX[move_offset]);
        veh->waypoint_y[0] = veh_y + BaseOffsetY[move_offset];
        return 0;
    }
    veh->order = ORDER_NONE;
    veh->state &= ~VSTATE_UNK_10000;
    veh_skip(veh_id);
    return 0;
}

