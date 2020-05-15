
#include "main.h"
#include "game.h"
#include "tech.h"
#include "move.h"

typedef int __cdecl fp_ccici(const char*, const char*, int, const char*, int);
typedef int __cdecl fp_icii(int, const char*, int, int);
const char* ScriptTxtID = "SCRIPT";

fp_1int* social_upkeep = (fp_1int*)0x5B44D0;
fp_1int* repair_phase = (fp_1int*)0x526030;
fp_1int* production_phase = (fp_1int*)0x526E70;
fp_1int* allocate_energy = (fp_1int*)0x5267B0;
fp_1int* enemy_diplomacy = (fp_1int*)0x55F930;
fp_1int* enemy_strategy = (fp_1int*)0x561080;
fp_1int* corner_market = (fp_1int*)0x59EE50;
fp_1int* call_council = (fp_1int*)0x52C880;
fp_2int* eliminate_player = (fp_2int*)0x5B3380;
fp_2int* can_call_council = (fp_2int*)0x52C670;
fp_void* do_all_non_input = (fp_void*)0x5FCB20;
fp_void* auto_save = (fp_void*)0x5ABD20;
fp_2int* parse_num = (fp_2int*)0x625E30;
fp_icii* parse_says = (fp_icii*)0x625EC0;
fp_ccici* popp = (fp_ccici*)0x48C0A0;

int* game_settings = (int*)0x9A6490;
int* gender_default = (int*)0x9BBFEC;
int* plurality_default = (int*)0x9BBFF0;
int* current_player_faction = (int*)0x939284;
int* multiplayer_active = (int*)0x93F660;


bool victory_done() {
    // TODO: Check for scenario victory conditions
    return *tx_game_state & (STATE_VICTORY_CONQUER | STATE_VICTORY_DIPLOMATIC | STATE_VICTORY_ECONOMIC)
        || has_project(-1, FAC_ASCENT_TO_TRANSCENDENCE);
}

int game_year(int n) {
    return tx_basic->normal_start_year + n;
}

HOOK_API int faction_upkeep(int faction) {
    Faction* f = &tx_factions[faction];
    MetaFaction* m = &tx_metafactions[faction];

    debug("faction_upkeep %d %d\n", *tx_current_turn, faction);
    init_save_game(faction);
    if (ai_enabled(faction)) {
        move_upkeep(faction);
    }

    *(int*)0x93A934 = 1;
    social_upkeep(faction);
    do_all_non_input();
    repair_phase(faction);
    do_all_non_input();
    production_phase(faction);
    do_all_non_input();
    if (!(*tx_game_state & 1) || *tx_game_state & 8) {
        allocate_energy(faction);
        do_all_non_input();
        enemy_diplomacy(faction);
        do_all_non_input();
        enemy_strategy(faction);
        do_all_non_input();
        social_ai(faction, -1, -1, -1, -1, 0);
        do_all_non_input();
        if (!is_human(faction)) {
            int cost = corner_market(faction);
            if (!victory_done() && f->energy_credits > cost && f->corner_market_active < 1
            && has_tech(faction, tx_basic->tech_preq_economic_victory)
            && *tx_game_rules & RULES_VICTORY_MINE_ALL_MINE) {
                f->corner_market_turn = *tx_current_turn + tx_basic->turns_corner_global_energy_market;
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
    for (int i=0; i<*tx_total_num_bases; i++) {
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

    if (!(*tx_game_state & 1) || *tx_game_state & 8) {
        if (faction == *current_player_faction && !(*tx_game_state & 0x14000)
        && can_call_council(faction, 0) && !(*tx_game_state & 1)) {
            *tx_game_state |= 0x4000;
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

