
#include "engine.h"
#include "game.h"
#include "tech.h"
#include "move.h"

const char* ScriptTxtID = "SCRIPT";

BASE** current_base_ptr = (BASE**)0x90EA30;
int* current_base_id = (int*)0x689370;
int* game_settings = (int*)0x9A6490;
int* game_state = (int*)0x9A64C0;
int* game_rules = (int*)0x9A649C;
int* diff_level = (int*)0x9A64C4;
int* smacx_enabled = (int*)0x9A6488;
int* human_players = (int*)0x9A64E8;
int* current_turn = (int*)0x9A64D4;
int* active_faction = (int*)0x9A6820;
int* total_num_bases = (int*)0x9A64CC;
int* total_num_vehicles = (int*)0x9A64C8;
int* map_random_seed = (int*)0x949878;
int* map_toggle_flat = (int*)0x94988C;
int* map_area_tiles = (int*)0x949884;
int* map_area_sq_root = (int*)0x949888;
int* map_axis_x = (int*)0x949870;
int* map_axis_y = (int*)0x949874;
int* map_half_x = (int*)0x68FAF0;
int* climate_future_change = (int*)0x9A67D8;
int* un_charter_repeals = (int*)0x9A6638;
int* un_charter_reinstates = (int*)0x9A663C;
int* gender_default = (int*)0x9BBFEC;
int* plurality_default = (int*)0x9BBFF0;
int* current_player_faction = (int*)0x939284;
int* multiplayer_active = (int*)0x93F660;
int* pbem_active = (int*)0x93A95C;
int* sunspot_duration = (int*)0x9A6800;
int* diplo_active_faction = (int*)0x93F7CC;
int* diplo_current_friction = (int*)0x93FA74;
int* diplo_opponent_faction = (int*)0x8A4164;
int* base_find_dist = (int*)0x90EA04;

fp_1int* social_upkeep = (fp_1int*)0x5B44D0;
fp_1int* repair_phase = (fp_1int*)0x526030;
fp_1int* production_phase = (fp_1int*)0x526E70;
fp_1int* allocate_energy = (fp_1int*)0x5267B0;
fp_1int* enemy_diplomacy = (fp_1int*)0x55F930;
fp_1int* enemy_strategy = (fp_1int*)0x561080;
fp_1int* corner_market = (fp_1int*)0x59EE50;
fp_1int* call_council = (fp_1int*)0x52C880;
fp_3int* setup_player = (fp_3int*)0x5B0E00;
fp_2int* eliminate_player = (fp_2int*)0x5B3380;
fp_2int* can_call_council = (fp_2int*)0x52C670;
fp_void* do_all_non_input = (fp_void*)0x5FCB20;
fp_void* auto_save = (fp_void*)0x5ABD20;
fp_2int* parse_num = (fp_2int*)0x625E30;
fp_icii* parse_says = (fp_icii*)0x625EC0;
fp_ccici* popp = (fp_ccici*)0x48C0A0;
fp_3int* capture_base = (fp_3int*)0x50C510;
fp_1int* base_kill = (fp_1int*)0x4E5250;
fp_5int* crop_yield = (fp_5int*)0x4E6E50;
fp_6int* base_draw = (fp_6int*)0x55AF20;
fp_6int* base_find3 = (fp_6int*)0x4E3D50;
fp_3int* draw_tile = (fp_3int*)0x46AF40;
tc_2int* font_width = (tc_2int*)0x619280;
tc_4int* buffer_box = (tc_4int*)0x5E3203;
tc_3int* buffer_fill3 = (tc_3int*)0x5DFCD0;
tc_5int* buffer_write_l = (tc_5int*)0x5DCEA0;
fp_6int* social_ai = (fp_6int*)0x5B4790;
fp_1int* social_set = (fp_1int*)0x5B4600;
fp_1int* pop_goal = (fp_1int*)0x4EF090;
fp_1int* consider_designs = (fp_1int*)0x581260;
fp_3int* tech_val = (fp_3int*)0x5BCBE0;
fp_1int* tech_rate = (fp_1int*)0x5BE6B0;
fp_1int* tech_selection = (fp_1int*)0x5BE380;
fp_1int* enemy_move = (fp_1int*)0x56B5B0;
fp_3int* best_defender = (fp_3int*)0x5044D0;
fp_5int* battle_compute = (fp_5int*)0x501DA0;
fp_6int* battle_kill = (fp_6int*)0x505D80;
fp_7int* battle_fight_2 = (fp_7int*)0x506AF0;


void init_save_game(int faction) {
    Faction* f = &tx_factions[faction];
    MetaFaction* m = &tx_metafactions[faction];

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

/*
Original Offset: 005C89A0
*/
HOOK_API int game_year(int n) {
    return tx_basic->normal_start_year + n;
}

/*
Original Offset: 00527290
*/
HOOK_API int faction_upkeep(int faction) {
    Faction* f = &tx_factions[faction];
    MetaFaction* m = &tx_metafactions[faction];

    debug("faction_upkeep %d %d\n", *current_turn, faction);
    init_save_game(faction);

    *(int*)0x93A934 = 1;
    social_upkeep(faction);
    do_all_non_input();
    repair_phase(faction);
    do_all_non_input();
    production_phase(faction);
    do_all_non_input();
    if (!(*game_state & STATE_UNK_1) || *game_state & STATE_UNK_8) {
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
        */
        mod_social_ai(faction, -1, -1, -1, -1, 0);
        move_upkeep(faction);
        do_all_non_input();

        if (!is_human(faction)) {
            int cost = corner_market(faction);
            if (!victory_done() && f->energy_credits > cost && f->corner_market_active < 1
            && has_tech(faction, tx_basic->tech_preq_economic_victory)
            && *game_rules & RULES_VICTORY_ECONOMIC) {
                f->corner_market_turn = *current_turn + tx_basic->turns_corner_global_energy_market;
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
        BASE* base = &tx_bases[i];
        if (base->faction_id == faction) {
            base->status_flags &= ~(BASE_UNK_1 | BASE_HURRY_PRODUCTION);
        }
    }
    f->energy_credits -= f->energy_cost;
    f->energy_cost = 0;
    if (f->energy_credits < 0) {
        f->energy_credits = 0;
    }
    if (!f->current_num_bases && !f->units_active[BSC_COLONY_POD] && !f->units_active[BSC_SEA_ESCAPE_POD]) {
        eliminate_player(faction, 0);
    }
    *(int*)0x93A934 = 0;
    *(int*)0x945B18 = -1;
    *(int*)0x945B1C = -1;

    if (!(*game_state & STATE_UNK_1) || *game_state & STATE_UNK_8) {
        if (faction == *current_player_faction && !(*game_state & (STATE_UNK_10000 | STATE_UNK_4000))
        && can_call_council(faction, 0) && !(*game_state & STATE_UNK_1)) {
            *game_state |= STATE_UNK_4000;
            popp(ScriptTxtID, "COUNCILOPEN", 0, "council_sm.pcx", 0);
        }
        if (!is_human(faction)) {
            call_council(faction);
        }
    }
    if (!*multiplayer_active && *game_settings & 2 && faction == *current_player_faction) {
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
    MAP* sq = mapsq(x, y);
    bool ocean = conf.territory_border_fix && sq && is_ocean(sq) && sq->items & TERRA_BASE_RADIUS;

    for (int i=0; i<*total_num_bases; ++i) {
        BASE* base = &tx_bases[i];
        MAP* bsq = mapsq(base->x, base->y);

        if (bsq && (region < 0 || bsq->region == region || ocean)) {
            if ((faction1 < 0 && (faction2 < 0 || faction2 != base->faction_id))
            || (faction1 == base->faction_id)
            || (faction2 == -2 && tx_factions[faction1].diplo_status[base->faction_id] & DIPLO_PACT)
            || (faction2 >= 0 && faction2 == base->faction_id)) {
                if (faction3 < 0 || base->faction_id == faction3 || base->factions_spotted_flags & (1 << faction3)) {
                    int dx = abs(x - base->x);
                    int dy = abs(y - base->y);
                    if (!(*map_toggle_flat & 1 || dx <= *map_half_x)) {
                        dx = *map_axis_x - dx;
                    }
                    int val = max(dx, dy) - ((((dx + dy) / 2) - min(dx, dy) + 1) / 2);
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



