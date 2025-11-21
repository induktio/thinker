
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
                            mod_veh_skip(veh_id);
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

int __cdecl mod_enemy_veh(size_t veh_id) {
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
                mod_veh_skip(veh_id);
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

