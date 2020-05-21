
#include "tech.h"

void init_save_game(int faction) {
    Faction* f = &tx_factions[faction];
    check_zeros((int*)&f->thinker_unused, sizeof(f->thinker_unused));

    if (f->thinker_header != THINKER_HEADER) {
        f->thinker_header = THINKER_HEADER;
        f->thinker_flags = 0;
        f->thinker_tech_id = f->tech_research_id;
        f->thinker_tech_cost = f->tech_cost;
        memset(&f->thinker_unused, 0, sizeof(f->thinker_unused));
    }
    if (f->thinker_enemy_range < 2 || f->thinker_enemy_range > 40) {
        f->thinker_enemy_range = 20;
    }
}

HOOK_API int tech_value(int tech, int faction, int flag) {
    int value = tx_tech_val(tech, faction, flag);
    if (conf.tech_balance && ai_enabled(faction)) {
        if (tech == tx_weapon[WPN_TERRAFORMING_UNIT].preq_tech
        || tech == tx_weapon[WPN_SUPPLY_TRANSPORT].preq_tech
        || tech == tx_weapon[WPN_PROBE_TEAM].preq_tech
        || tech == tx_facility[FAC_RECYCLING_TANKS].preq_tech
        || tech == tx_facility[FAC_CHILDREN_CRECHE].preq_tech
        || tech == tx_basic->tech_preq_allow_3_energy_sq
        || tech == tx_basic->tech_preq_allow_3_minerals_sq
        || tech == tx_basic->tech_preq_allow_3_nutrients_sq) {
            value += 40;
        }
    }
    debug("tech_value %d %d %d %s\n", tech, faction, value, tx_techs[tech].name);
    return value;
}

int tech_level(int id, int lvl) {
    if (id < 0 || id > TECH_TranT || lvl >= 20) {
        return lvl;
    } else {
        int v1 = tech_level(tx_techs[id].preq_tech1, lvl + 1);
        int v2 = tech_level(tx_techs[id].preq_tech2, lvl + 1);
        return max(v1, v2);
    }
}

int tech_cost(int faction, int tech) {
    assert(faction >= 0 && faction < 8);
    Faction* f = &tx_factions[faction];
    MetaFaction* m = &tx_metafactions[faction];
    int level = 1;
    int owned = 0;
    int links = 0;

    if (tech >= 0) {
        level = tech_level(tech, 0);
        for (int i=1; i<8; i++) {
            if (i != faction && f->diplo_status[i] & DIPLO_COMMLINK) {
                links |= 1 << i;
            }
        }
        owned = __builtin_popcount(tx_tech_discovered[tech] & links);
    }
    double base = 6 * pow(level, 3) + 114 * level;
    double dw;
    double cost = base * *map_area_sq_root / 56
        * m->rule_techcost / 100
        * (10 - min(5, max(-5, f->SE_research))) / 10
        * (2 * min(10, max(1, f->tech_ranking/2)) + 1) / 21
        * (*game_rules & RULES_TECH_STAGNATION ? 1.5 : 1.0)
        * tx_basic->rules_tech_discovery_rate / 100
        * (owned > 0 ? (owned > 1 ? 0.75 : 0.85) : 1.0);

    if (is_human(faction)) {
        dw = (1.0 + 0.1 * (*diff_level +
            (*diff_level < DIFF_LIBRARIAN ? 1 : 0) - DIFF_LIBRARIAN));
    } else {
        dw = (1.0 + 0.1 * (tx_cost_ratios[*diff_level] - 10));
    }
    cost *= dw;
    debug("tech_cost %d %d | %8.4f %8.4f %8.4f %d %d %d %s\n", *current_turn, faction,
        base, dw, cost, level, owned, tech, (tech >= 0 ? tx_techs[tech].name : NULL));

    return max(2, (int)cost);
}

HOOK_API int tech_rate(int faction) {
    /*
    Normally the game engine would recalculate research cost before the next tech
    is selected, but we need to wait until the tech is decided in tech_selection
    before recalculating the cost.
    */
    Faction* f = &tx_factions[faction];

    if (f->tech_research_id != f->thinker_tech_id) {
        f->thinker_tech_cost = tech_cost(faction, f->tech_research_id);
        f->thinker_tech_id = f->tech_research_id;
    }
    return f->thinker_tech_cost;
}

HOOK_API int tech_selection(int faction) {
    Faction* f = &tx_factions[faction];
    int tech = tx_tech_selection(faction);
    f->thinker_tech_cost = tech_cost(faction, tech);
    f->thinker_tech_id = tech;
    return tech;
}

