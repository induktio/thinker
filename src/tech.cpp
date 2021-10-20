
#include "tech.h"

int TechCostRatios[MaxDiffNum] = {124,116,108,100,84,76};


HOOK_API int mod_tech_value(int tech, int faction, int flag) {
    int value = tech_val(tech, faction, flag);
    if (conf.tech_balance && ai_enabled(faction)) {
        if (tech == Weapon[WPN_TERRAFORMING_UNIT].preq_tech
        || tech == Weapon[WPN_SUPPLY_TRANSPORT].preq_tech
        || tech == Weapon[WPN_PROBE_TEAM].preq_tech
        || tech == Facility[FAC_RECYCLING_TANKS].preq_tech
        || tech == Facility[FAC_CHILDREN_CRECHE].preq_tech
        || tech == Rules->tech_preq_allow_3_energy_sq
        || tech == Rules->tech_preq_allow_3_minerals_sq
        || tech == Rules->tech_preq_allow_3_nutrients_sq) {
            value += 40;
        }
    }
    debug("tech_value %d %d value: %3d tech: %2d %s\n",
        *current_turn, faction, value, tech, Tech[tech].name);
    return value;
}

int tech_level(int id, int lvl) {
    if (id < 0 || id > TECH_TranT || lvl > TECH_TranT) {
        return lvl;
    } else {
        int v1 = tech_level(Tech[id].preq_tech1, lvl + 1);
        int v2 = tech_level(Tech[id].preq_tech2, lvl + 1);
        return max(v1, v2);
    }
}

int tech_cost(int faction, int tech) {
    assert(valid_player(faction));
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    int level = 1;
    int owned = 0;
    int links = 0;

    if (tech >= 0) {
        level = tech_level(tech, 0);
        for (int i=1; i < MaxPlayerNum; i++) {
            if (i != faction && f->diplo_status[i] & DIPLO_COMMLINK) {
                links |= 1 << i;
            }
        }
        owned = __builtin_popcount(TechOwners[tech] & links);
    }
    double diff_factor = 1.0;
    if (!is_human(faction)) {
        diff_factor = TechCostRatios[*diff_level] / 100.0;
    }
    if (conf.cheap_early_tech) {
        diff_factor *= 0.4 + 0.6 * min(10, max(1, f->tech_ranking / 2)) / 10.0;
    }
    double cost = (6 * pow(level, 3) + 74 * level - 20)
        * diff_factor
        * *map_area_tiles / 3200.0
        * m->rule_techcost / 100.0
        * (*game_rules & RULES_TECH_STAGNATION ? 1.5 : 1.0)
        * 100.0 / max(1, Rules->rules_tech_discovery_rate)
        * (owned > 0 ? (owned > 1 ? 0.75 : 0.85) : 1.0);

    debug("tech_cost %d %d diff: %.4f cost: %8.4f level: %d owned: %d tech: %d %s\n",
        *current_turn, faction, diff_factor, cost, level, owned, tech,
        (tech >= 0 ? Tech[tech].name : NULL));

    return max(2, (int)cost);
}

HOOK_API int mod_tech_rate(int faction) {
    /*
    Normally the game engine would recalculate research cost before the next tech
    is selected, but we need to wait until the tech is decided in tech_selection
    before recalculating the cost.
    */
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];

    if (m->thinker_tech_cost < 2) {
        init_save_game(faction);
    }
    if (f->tech_research_id != m->thinker_tech_id) {
        m->thinker_tech_cost = tech_cost(faction, f->tech_research_id);
        m->thinker_tech_id = f->tech_research_id;
    }
    return m->thinker_tech_cost;
}

HOOK_API int mod_tech_selection(int faction) {
    MFaction* m = &MFactions[faction];
    int tech = tech_selection(faction);
    m->thinker_tech_cost = tech_cost(faction, tech);
    m->thinker_tech_id = tech;
    return tech;
}

