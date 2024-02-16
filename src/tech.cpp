
#include "tech.h"


int __cdecl mod_tech_value(int tech, int faction, int flag) {
    int value = tech_val(tech, faction, flag);
    if (conf.tech_balance && thinker_enabled(faction)) {
        if (tech == Weapon[WPN_TERRAFORMING_UNIT].preq_tech
        || tech == Weapon[WPN_SUPPLY_TRANSPORT].preq_tech
        || tech == Weapon[WPN_PROBE_TEAM].preq_tech
        || tech == Facility[FAC_RECYCLING_TANKS].preq_tech
        || tech == Facility[FAC_CHILDREN_CRECHE].preq_tech
        || tech == Rules->tech_preq_allow_3_energy_sq
        || tech == Rules->tech_preq_allow_3_minerals_sq
        || tech == Rules->tech_preq_allow_3_nutrients_sq) {
            value += 50;
        }
    }
    debug("tech_value %d %d value: %3d tech: %2d %s\n",
        *CurrentTurn, faction, value, tech, Tech[tech].name);
    return value;
}

int tech_level(int tech, int lvl) {
    if (tech < 0 || tech > TECH_TranT || lvl > TECH_TranT) {
        return lvl;
    } else {
        int v1 = tech_level(Tech[tech].preq_tech1, lvl + 1);
        int v2 = tech_level(Tech[tech].preq_tech2, lvl + 1);
        return max(v1, v2);
    }
}

int tech_cost(int faction, int tech) {
    assert(valid_player(faction));
    MFaction* m = &MFactions[faction];
    int level = 1;
    int owners = 0;
    int our_techs = 0;

    if (tech >= 0) {
        level = tech_level(tech, 0);
        for (int i=1; i < MaxPlayerNum; i++) {
            if (i != faction && has_tech(tech, i)
            && has_treaty(faction, i, DIPLO_COMMLINK)) {
                owners += (has_treaty(faction, i, DIPLO_PACT|DIPLO_HAVE_INFILTRATOR) ? 2 : 1);
            }
        }
    }
    for (int i=Tech_ID_First; i <= Tech_ID_Last; i++) {
        if (Tech[i].preq_tech1 != TECH_Disable && has_tech(i, faction)) {
            our_techs++;
        }
    }
    double diff_factor = (is_human(faction) ? 1.0 : conf.tech_cost_factor[*DiffLevel] / 100.0);
    double cost_base;

    if (conf.cheap_early_tech) {
        cost_base = (5 * pow(level, 3) + 25 * level + 15 * our_techs)
            * clamp(our_techs + 6, 6, 16) / 16.0;
    } else {
        cost_base = (5 * pow(level, 3) + 75 * level);
    }
    double cost = cost_base
        * diff_factor
        * *MapAreaSqRoot / 56.0
        * m->rule_techcost / 100.0
        * (*GameRules & RULES_TECH_STAGNATION ? conf.tech_stagnate_rate / 100.0 : 1.0)
        * 100.0 / max(1, Rules->rules_tech_discovery_rate)
        * (1.0 - 0.05*min(6, owners));

    debug("tech_cost %d %d base: %8.4f diff: %8.4f cost: %8.4f "
    "level: %d our_techs: %d owners: %d tech: %2d %s\n",
    *CurrentTurn, faction, cost_base, diff_factor, cost,
    level, our_techs, owners, tech, (tech >= 0 ? Tech[tech].name : NULL));

    return max(2, (int)cost);
}

int __cdecl mod_tech_rate(int faction) {
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

int __cdecl mod_tech_selection(int faction) {
    MFaction* m = &MFactions[faction];
    int tech = tech_selection(faction);
    m->thinker_tech_cost = tech_cost(faction, tech);
    m->thinker_tech_id = tech;
    return tech;
}

