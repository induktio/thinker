
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
            int base_id = mod_base_find3(veh->x, veh->y, veh->faction_id, -1, -1, -1);
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

void __cdecl mod_alien_fauna() {
    const int player_id = *CurrentPlayerFaction;
    int growth_limit = *MapAreaTiles / (*CurrentTurn / 4 + 32);
    for (int iter = 0; iter < growth_limit; iter++) {
        int tx = (*MapAreaX > 1) ? game_rand() % *MapAreaX : 0;
        int ty = (*MapAreaY > 1) ? game_rand() % *MapAreaY : 0;
        tx = tx + (ty & 1) - (tx & 1);
        if (!on_map(tx, ty)) {
            continue;
        }
        MAP* sq = mapsq(tx, ty);
        bool is_sea = is_ocean(sq);
        if (!is_sea) {
            if (!(sq->items & BIT_FOREST)) {
                continue;
            }
        } else if (!(sq->items & BIT_FARM)) {
            continue;
        }
        int best_score = 0;
        int best_x = -1;
        int best_y = -1;
        for (int i = 0; i < 8; i++) {
            int nx = wrap(tx + BaseOffsetX[i]);
            int ny = ty + BaseOffsetY[i];
            MAP* nsq = mapsq(nx, ny);
            if (nsq && is_ocean(nsq) == is_sea
            && nsq->alt_level() >= ALT_OCEAN_SHELF
            && (nsq->alt_level() <= ALT_ONE_ABOVE_SEA || nsq->climate & 0x18)
            && (nsq->landmarks & (LM_DISABLE|LM_VOLCANO)) != LM_VOLCANO
            && (nsq->landmarks & (LM_DISABLE|LM_DUNES)) != LM_DUNES) {
                int score = 4 * (3 - ((nsq->climate >> 3) & 3)) - (nsq->val3 >> 6);
                if ((nsq->val3 & 0xC0) <= 64
                && !(nsq->items & (BIT_FOREST|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_BASE_IN_TILE))
                && score > best_score) {
                    best_score = score;
                    best_x = nx;
                    best_y = ny;
                }
            }
        }
        if (best_x >= 0) {
            MAP* nsq = mapsq(best_x, best_y);
            uint32_t new_item = (is_sea ? BIT_FARM : BIT_FOREST);
            if (!(nsq->items & new_item)) {
                nsq->items = (new_item | nsq->items) & ~BIT_FUNGUS;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (sq->is_known(i) // source tile visible or revealed
                    && sq->visible_items[i - 1] & new_item) {
                        nsq->visible_items[i - 1] = (new_item | nsq->visible_items[i - 1]) & ~BIT_FUNGUS;
                        if (i == player_id) {
                            int base_id = mod_base_find3(tx, ty, -1, -1, -1, player_id);
                            if (base_id < 0 || Bases[base_id].faction_id != player_id
                            || (*BaseFindDist > 3 && mod_whose_territory(player_id, tx, ty, 0, 0) != player_id)) {
                                draw_tiles(tx, ty, 2);
                            } else {
                                if (!MapWin_focus(MapWinPtr[0], tx, ty)) {
                                    draw_tiles(tx, ty, 2);
                                }
                                parse_says(0, Bases[base_id].name, -1, -1);
                                if (conf.game_event_popup) {
                                    if (is_sea) {
                                        POP2("KELPGROWS", "kelp_sm.pcx", -1);
                                    } else {
                                        POP2("FORESTGROWS", "forgr_sm.pcx", -1);
                                    }
                                } else {
                                    if (is_sea) {
                                        NetMsg_pop(NetMsg, "KELPGROWS", 5000, 0, "kelp_sm.pcx");
                                    } else {
                                        NetMsg_pop(NetMsg, "FORESTGROWS", 5000, 0, "forgr_sm.pcx");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    bool allow_locusts = (TechOwners[TECH_CentPsi] || TechOwners[TECH_Quantum]);
    bool early_limit = (*CurrentTurn < 2 * (10 * (3 - *MapNativeLifeForms) - *DiffLevel));
    if (*GameRules & RULES_SCN_NO_NATIVE_LIFE) {
        return;
    }
    int veh_limit = (conf.modify_unit_limit ? (MaxVehModNum*3)/4 : 1792);
    if (project_base(FAC_VOICE_OF_PLANET) != SP_Unbuilt || *VehCount >= veh_limit) {
        return;
    }
    if (!(*CurrentTurn % (2 * (5 - *MapNativeLifeForms)))) {
        int iter = 0;
        int mx;
        int my;
        while (true) {
            mx = (*MapAreaX > 1) ? game_rand() % *MapAreaX : 0;
            my = (*MapAreaY > 1) ? game_rand() % *MapAreaY : 0;
            mx = mx + (my & 1) - (mx & 1);
            MAP* sq = mapsq(mx, my);
            assert(sq);
            if (++iter >= 1000) {
                break; // skip next block
            }
            if (sq->is_fungus()) {
                break;
            }
            if (is_ocean(sq) && sq->anything_at() < 0 && !bad_reg(sq->region)
            && Continents[sq->region].tile_count >= 50) {
                break;
            }
        }
        if (iter < 1000) {
            int spawn_val = *CurrentTurn * (((*GameState & STATE_PERIHELION_ACTIVE) != 0) + 1)
                / (4 * (4 * (8 - *MapNativeLifeForms) - *DiffLevel)) + *MapNativeLifeForms + 1;
            if (early_limit) {
                spawn_val = 1;
            }
            if (conf.spawn_sealurks && *ExpansionEnabled && game_rand() % 2) {
                // replace timeGetTime() random values
                int veh_id = veh_init(BSC_SEALURK, 0, mx, my);
                if (veh_id >= 0) {
                    veh_promote(veh_id);
                    if (allow_locusts) {
                        veh_init(BSC_LOCUSTS_OF_CHIRON, 0, mx, my);
                    }
                }
            } else {
                int veh_id = veh_init(BSC_ISLE_OF_THE_DEEP, 0, mx, my);
                if (veh_id >= 0) {
                    if (spawn_val >= veh_cargo(veh_id)) {
                        spawn_val = veh_cargo(veh_id);
                    }
                    while (spawn_val > 0) {
                        if (game_rand() % 5 || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                            sleep(veh_init(BSC_MIND_WORMS, 0, mx, my));
                        } else {
                            sleep(veh_init(BSC_SPORE_LAUNCHER, 0, mx, my));
                        }
                        --spawn_val;
                    }
                    veh_promote(veh_id);
                    if (allow_locusts) {
                        veh_init(BSC_LOCUSTS_OF_CHIRON, 0, mx, my);
                    }
                }
            }
        }
    }
    auto is_visible_veh = [](int veh_id, int faction_id) {
        return (Vehs[veh_id].faction_id == faction_id
            && !Vehs[veh_id].is_invisible_lurker())
            || Vehs[veh_id].is_visible(faction_id);
    };
    int spawn_base_id = -1;
    int spawn_region = -1;
    int spawn_x = -1;
    int spawn_y = -1;
    int loop_limit = 2 * *MapNativeLifeForms + 2;

    for (int iter = 0; iter < loop_limit; iter++) {
        int rx = (*MapAreaX > 1) ? game_rand() % *MapAreaX : 0;
        int ry = (*MapAreaY > 1) ? game_rand() % *MapAreaY : 0;
        rx = rx + (ry & 1) - (rx & 1);
        MAP* sq = mapsq(rx, ry);
        assert(sq);
        if (is_ocean(sq) || !sq->is_fungus()) {
            continue;
        }
        if (bad_reg(sq->region) || sq->base_who() >= 0) {
            continue;
        }
        int base_id = mod_base_find3(rx, ry, -1, sq->region, -1, -1);
        int veh_owner_id = sq->veh_who();
        if (veh_owner_id > 0 && !(base_id >= 0 && *BaseFindDist <= 2)) {
            for (int i = 0; i < 16; i++) {
                int px = wrap(rx + TableOffsetX[i + 9]);
                int py = ry + TableOffsetY[i + 9];
                MAP* psq = mapsq(px, py);
                if (psq && !is_ocean(psq)
                && !psq->is_known(veh_owner_id)
                && psq->anything_at() < 0
                && !mod_zoc_any(px, py, 0)) {
                    int veh_id;
                    if (game_rand() % 5 || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                        veh_id = veh_init(BSC_MIND_WORMS, 0, px, py);
                    } else {
                        veh_id = veh_init(BSC_SPORE_LAUNCHER, 0, px, py);
                    }
                    if (veh_id >= 0 && is_visible_veh(veh_id, player_id)) {
                        draw_tile(px, py, 2);
                    }
                    break;
                }
            }
        }
        if (veh_owner_id < 0
        && *BaseFindDist > 1 && *BaseFindDist < 6
        && sq->is_fungus() && !goody_at(rx, ry)) {
            spawn_base_id = base_id; // Fix: update only when spawning
            spawn_region = sq->region;
            spawn_x = rx;
            spawn_y = ry;
            int veh_id;
            if (game_rand() % 5 || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                veh_id = veh_init(BSC_MIND_WORMS, 0, rx, ry);
            } else {
                veh_id = veh_init(BSC_SPORE_LAUNCHER, 0, rx, ry);
            }
            if (veh_id >= 0 && is_visible_veh(veh_id, player_id)) {
                draw_tile(rx, ry, 2);
            }
        }
    }
    // Enable XMINDWORMS event spawning additional native life with minor rewrites
    // Previously this event was never triggered due to flawed original code
    if (spawn_base_id >= 0 && *DiffLevel >= DIFF_LIBRARIAN
    && *CurrentTurn >= 10 * (5 - *MapNativeLifeForms)) {
        BASE* base = &Bases[spawn_base_id];
        Faction* plr = &Factions[base->faction_id];
        int spawn_val = clamp(((*MapNativeLifeForms + 1)
            * (((*GameState & STATE_PERIHELION_ACTIVE) != 0) + 1)
            * (base->pop_size + 3)) / 4, 1 + allow_locusts, 16);
        if (plr->base_count < clamp(*CurrentTurn / 8, 4, 20)) {
            spawn_val = (spawn_val + 1) / 2;
        }
        if (early_limit) {
            spawn_val = 1 + allow_locusts;
        }
        int show_popup = 0;
        int px;
        int py;
        int i = 0;
        MAP* sq;
        while (true) {
            px = wrap(spawn_x + TableOffsetX[i]);
            py = spawn_y + TableOffsetY[i];
            sq = mapsq(px, py);
            if (sq
            && sq->is_fungus()
            && sq->region == spawn_region
            && sq->anything_at() <= 0
            && !goody_at(px, py)
            && game_rand() % 2) {
                break;
            }
            if (++i >= 21) {
                return; // skip event, suitable tiles missing
            }
        }
        for (int iter = 0; iter < spawn_val; iter++) {
            int veh_id;
            if (!iter && allow_locusts) {
                veh_id = veh_init(BSC_LOCUSTS_OF_CHIRON, 0, px, py);
            } else if (game_rand() % 5 || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                veh_id = veh_init(BSC_MIND_WORMS, 0, px, py);
            } else {
                veh_id = veh_init(BSC_SPORE_LAUNCHER, 0, px, py);
            }
            if (veh_id >= 0 && is_visible_veh(veh_id, player_id)) {
                draw_tile(px, py, 2);
            }
            if (veh_id >= 0 && sq->is_known(player_id) && base->faction_id == player_id) {
                // skip is_visible_veh check since units are hidden on fungus by default
                show_popup = 1;
            }
        }
        if (show_popup) {
            if (!MapWin_focus(MapWinPtr[0], base->x, base->y)) {
                draw_tiles(base->x, base->y, 2);
            }
            parse_says(0, base->name, -1, -1);
            NetMsg_pop(NetMsg, "XMINDWORMS", -5000, 0, "mindworm_sm.pcx");
        }
    }
}

void __cdecl mod_do_fungal_towers() {
    auto spread_fungus = [](int x, int y) {
        bit_set(x, y, BIT_FUNGUS, 1);
        bit_set(x, y, BIT_FOREST|BIT_SOIL_ENRICHER|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD, 0);
        for (int i = 1; i < MaxPlayerNum; i++) {
            synch_bit(x, y, i);
        }
        draw_tile(x, y, 2);
    };
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->unit_id == BSC_FUNGAL_TOWER) {
            veh->state |= VSTATE_HAS_MOVED;
            int x = veh->x;
            int y = veh->y;
            if (!(game_rand() % 5)) {
                int dir = game_rand() % 8;
                int x2 = wrap(x + TableOffsetX[dir + 1]);
                int y2 = y + TableOffsetY[dir + 1];
                if (on_map(x2, y2)) {
                    spread_fungus(x2, y2);
                }
            }
            spread_fungus(x, y);
            veh->morale = mod_morale_alien(i, 0);
            spot_all(i, 1);
        }
    }
}



