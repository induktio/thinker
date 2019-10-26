
#include "tech.h"


void init_values(int fac) {
    Faction* f = &tx_factions[fac];
    if (f->thinker_header != THINKER_HEADER) {
        f->thinker_header = THINKER_HEADER;
        f->thinker_flags = 0;
        f->thinker_tech_id = f->tech_research_id;
        f->thinker_tech_cost = f->tech_cost;
    }
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

int tech_cost(int fac, int tech) {
    assert(fac >= 0 && fac < 8);
    Faction* f = &tx_factions[fac];
    FactMeta* m = &tx_factions_meta[fac];
    int level = 1;
    int owned = 0;
    int links = 0;

    if (tech >= 0) {
        level = tech_level(tech, 0);
        for (int i=1; i<8; i++) {
            if (i != fac && f->diplo_status[i] & DIPLO_COMMLINK) {
                links |= 1 << i;
            }
        }
        owned = __builtin_popcount(tx_tech_discovered[tech] & links);
    }
    double base = 3 * pow(level, 3) + 117 * level;
    double dw;
    double cost = base * *tx_map_area_sq_root / 56
        * m->rule_techcost / 100
        * (10 - min(5, max(-5, f->SE_research))) / 10
        * (2 * min(10, max(1, f->tech_ranking/2)) + 1) / 21
        * (*tx_scen_rules & RULES_TECH_STAGNATION ? 1.5 : 1.0)
        * tx_basic->rules_tech_discovery_rate / 100
        * (owned > 0 ? (owned > 1 ? 0.75 : 0.85) : 1.0);

    if (is_human(fac)) {
        dw = (1.0 + 0.1 * (*tx_diff_level +
            (*tx_diff_level < DIFF_LIBRARIAN ? 1 : 0) - DIFF_LIBRARIAN));
    } else {
        dw = (1.0 + 0.1 * (tx_cost_ratios[*tx_diff_level] - 10));
    }
    cost *= dw;
    debug("tech_cost %d %d | %8.4f %8.4f %8.4f %d %d %d %s\n", *tx_current_turn, fac,
        base, dw, cost, level, owned, tech, (tech >= 0 ? tx_techs[tech].name : NULL));

    return max(2, (int)cost);
}

HOOK_API int tech_rate(int fac) {
    /*
    Normally the game engine would recalculate research cost before the next tech
    is selected, but we need to wait until the tech is decided in tech_selection
    before recalculating the cost.
    */
    Faction* f = &tx_factions[fac];
    init_values(fac);

    if (f->tech_research_id != f->thinker_tech_id) {
        f->thinker_tech_cost = tech_cost(fac, f->tech_research_id);
        f->thinker_tech_id = f->tech_research_id;
    }
    return f->thinker_tech_cost;
}

HOOK_API int tech_selection(int fac) {
    Faction* f = &tx_factions[fac];
    int tech = tx_tech_selection(fac);
    init_values(fac);
    f->thinker_tech_cost = tech_cost(fac, tech);
    f->thinker_tech_id = tech;
    return tech;
}

