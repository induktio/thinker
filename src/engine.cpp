
#include "engine.h"

const char* ScriptTxtID = "SCRIPT";


void init_save_game(int faction) {
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    if (m->thinker_header != THINKER_HEADER) {
        m->thinker_header = THINKER_HEADER;
        m->thinker_flags = 0;
        /* Convert old save game to the new format. */
        if (f->old_thinker_header == THINKER_HEADER) {
            m->thinker_tech_id = f->old_thinker_tech_id;
            m->thinker_tech_cost = f->old_thinker_tech_cost;
            m->thinker_enemy_range = f->old_thinker_enemy_range;
            memset(&f->old_thinker_header, 0, 12);
        } else {
            m->thinker_tech_id = f->tech_research_id;
            m->thinker_tech_cost = f->tech_cost;
            m->thinker_enemy_range = 20;
        }
    } else {
        assert(f->old_thinker_header != THINKER_HEADER);
    }
    if (m->thinker_enemy_range < 2 || m->thinker_enemy_range > 40) {
        m->thinker_enemy_range = 20;
    }
}

bool victory_done() {
    // TODO: Check for scenario victory conditions
    return *game_state & (STATE_VICTORY_CONQUER | STATE_VICTORY_DIPLOMATIC | STATE_VICTORY_ECONOMIC)
        || has_project(-1, FAC_ASCENT_TO_TRANSCENDENCE);
}

bool has_colony_pods(int faction) {
    for (int i=0; i<*total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction && Units[veh->unit_id].weapon_type == WPN_COLONY_MODULE) {
            return true;
        }
    }
    return false;
}

/*
Original Offset: 005C89A0
*/
HOOK_API int game_year(int n) {
    return Rules->normal_start_year + n;
}

/*
Original Offset: 00527290
*/
HOOK_API int mod_faction_upkeep(int faction) {
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];

    debug("faction_upkeep %d %d\n", *current_turn, faction);
    if (faction > 0) {
        init_save_game(faction);
        plans_upkeep(faction);
    }

    *dword_93A934 = 1;
    social_upkeep(faction);
    do_all_non_input();
    repair_phase(faction);
    do_all_non_input();
    production_phase(faction);
    do_all_non_input();
    if (!(*game_state & STATE_GAME_DONE) || *game_state & STATE_FINAL_SCORE_DONE) {
        allocate_energy(faction);
        do_all_non_input();
        enemy_diplomacy(faction);
        do_all_non_input();
        enemy_strategy(faction);
        do_all_non_input();
        /*
        Thinker-specific AI planning routines.
        If the new social_ai is disabled from the config old version gets called instead.
        Player factions always skip the new social_ai function.
        Note that move_upkeep is updated after all the production is done,
        so that the movement code can utilize up-to-date priority maps.
        This means we mostly cannot use move_upkeep variables in production phase.
        */
        mod_social_ai(faction, -1, -1, -1, -1, 0);
        move_upkeep(faction, false);
        do_all_non_input();

        if (!is_human(faction)) {
            int cost = corner_market(faction);
            if (!victory_done() && f->energy_credits > cost && f->corner_market_active < 1
            && has_tech(faction, Rules->tech_preq_economic_victory)
            && *game_rules & RULES_VICTORY_ECONOMIC) {
                f->corner_market_turn = *current_turn + Rules->turns_corner_global_energy_market;
                f->corner_market_active = cost;
                f->energy_credits -= cost;

                gender_default = &m->noun_gender;
                plurality_default = 0;
                parse_says(0, m->title_leader, -1, -1);
                parse_says(1, m->name_leader, -1, -1);
                plurality_default = &m->is_noun_plural;
                parse_says(2, m->noun_faction, -1, -1);
                parse_says(3, m->adj_name_faction, -1, -1);
                parse_num(0, game_year(f->corner_market_turn));
                popp(ScriptTxtID, "CORNERWARNING", 0, "econwin_sm.pcx", 0);
            }
        }
    }
    for (int i=0; i<*total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction) {
            base->status_flags &= ~(BASE_UNK_1 | BASE_HURRY_PRODUCTION);
        }
    }
    f->energy_credits -= f->energy_cost;
    f->energy_cost = 0;
    if (f->energy_credits < 0) {
        f->energy_credits = 0;
    }
    if (!f->current_num_bases && !has_colony_pods(faction)) {
        eliminate_player(faction, 0);
    }
    *dword_93A934 = 0;
    *dword_945B18 = -1;
    *dword_945B1C = -1;

    if (!(*game_state & STATE_GAME_DONE) || *game_state & STATE_FINAL_SCORE_DONE) {
        if (faction == *current_player_faction
        && !(*game_state & (STATE_COUNCIL_HAS_CONVENED | STATE_DISPLAYED_COUNCIL_AVAIL_MSG))
        && can_call_council(faction, 0) && !(*game_state & STATE_GAME_DONE)) {
            *game_state |= STATE_DISPLAYED_COUNCIL_AVAIL_MSG;
            popp(ScriptTxtID, "COUNCILOPEN", 0, "council_sm.pcx", 0);
        }
        if (!is_human(faction)) {
            call_council(faction);
        }
    }
    if (!*multiplayer_active && *game_preferences & PREF_UNK_2 && faction == *current_player_faction) {
        auto_save();
    }
    fflush(debug_log);
    return 0;
}

/*
Original Offset: 004E3D50
*/
HOOK_API int mod_base_find3(int x, int y, int faction1, int region, int faction2, int faction3) {
    int dist = 9999;
    int result = -1;
    bool border_fix = conf.territory_border_fix && region >= MaxRegionNum/2;

    for (int i=0; i<*total_num_bases; ++i) {
        BASE* base = &Bases[i];
        MAP* bsq = mapsq(base->x, base->y);

        if (bsq && (region < 0 || bsq->region == region || border_fix)) {
            if ((faction1 < 0 && (faction2 < 0 || faction2 != base->faction_id))
            || (faction1 == base->faction_id)
            || (faction2 == -2 && Factions[faction1].diplo_status[base->faction_id] & DIPLO_PACT)
            || (faction2 >= 0 && faction2 == base->faction_id)) {
                if (faction3 < 0 || base->faction_id == faction3 || base->factions_spotted_flags & (1 << faction3)) {
                    int val = vector_dist(x, y, base->x, base->y);
                    if (conf.territory_border_fix ? val < dist : val <= dist) {
                        dist = val;
                        result = i;
                    }
                }
            }
        }
    }
    if (DEBUG && !conf.territory_border_fix) {
        int res = base_find3(x, y, faction1, region, faction2, faction3);
        debug("base_find3 x: %2d y: %2d r: %2d %2d %2d %2d %2d %4d\n",
              x, y, region, faction1, faction2, faction3, result, dist);
        assert(res == result);
        assert(*base_find_dist == dist);
    }
    *base_find_dist = 9999;
    if (result >= 0) {
        *base_find_dist = dist;
    }
    return result;
}



