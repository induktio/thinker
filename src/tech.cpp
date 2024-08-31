
#include "tech.h"


/*
Calculate the tech related bonuses for commerce and resource production in fungus.
*/
void __cdecl mod_tech_effects(int faction_id) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    f->tech_commerce_bonus = m->rule_commerce;
    f->tech_fungus_nutrient = 0;
    f->tech_fungus_mineral = 0;
    f->tech_fungus_energy = 0;
    f->tech_fungus_unused = 0;

    for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
        if (has_tech(tech_id, faction_id)) {
            uint32_t flags = Tech[tech_id].flags;
            if (flags & TFLAG_INC_NUTRIENT_FUNGUS) {
                f->tech_fungus_nutrient++;
            }
            if (flags & TFLAG_INC_MINERALS_FUNGUS) {
                f->tech_fungus_mineral++;
            }
            if (flags & TFLAG_INC_ENERGY_FUNGUS) {
                f->tech_fungus_energy++;
            }
            if (flags & TFLAG_INC_COMMERCE) {
                f->tech_commerce_bonus++;
            }
        }
    }
    for (int i = 0; i < m->faction_bonus_count; i++) {
        if (m->faction_bonus_id[i] == FCB_FUNGNUTRIENT) {
            f->tech_fungus_nutrient += m->faction_bonus_val1[i];
        } else if (m->faction_bonus_id[i] == FCB_FUNGMINERALS) {
            f->tech_fungus_mineral += m->faction_bonus_val1[i];
        } else if (m->faction_bonus_id[i] == FCB_FUNGENERGY) {
            f->tech_fungus_energy += m->faction_bonus_val1[i];
        }
    }
    f->tech_fungus_nutrient = max(0, f->tech_fungus_nutrient);
    f->tech_fungus_mineral = max(0, f->tech_fungus_mineral);
    f->tech_fungus_energy = max(0, f->tech_fungus_energy);

    if (f->SE_economy_pending > 2) {
        f->tech_commerce_bonus++;
        if (f->SE_economy_pending > 3) {
            f->tech_commerce_bonus++;
            if (f->SE_economy_pending > 4) {
                f->tech_commerce_bonus++;
            }
        }
    }
}

/*
Check to see whether provided faction can research a specific technology.
Includes checks to prevent SMACX specific techs from being researched in SMAC mode.
*/
int __cdecl mod_tech_avail(int tech_id, int faction_id) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum && tech_id >= 0);
    if (has_tech(tech_id, faction_id) || tech_id >= MaxTechnologyNum || (!*ExpansionEnabled
    && (tech_id == TECH_PrPsych || tech_id == TECH_FldMod || tech_id == TECH_AdapDoc
    || tech_id == TECH_AdapEco || tech_id == TECH_Bioadap || tech_id == TECH_SentRes
    || tech_id == TECH_SecMani || tech_id == TECH_NewMiss || tech_id == TECH_BFG9000))) {
        return false;
    }
    int preq_tech_1 = Tech[tech_id].preq_tech1;
    int preq_tech_2 = Tech[tech_id].preq_tech2;
    if (preq_tech_1 <= TECH_Disable || preq_tech_2 <= TECH_Disable) {
        return false;
    }
    return (has_tech(preq_tech_1, faction_id) && has_tech(preq_tech_2, faction_id));
}

/*
Original tech_research would apply undocumented AI research bonus to double the speed
if the player faction is nearing the end game according to various conditions.
This bonus is skipped when revised_tech_cost is enabled.
*/
void __cdecl mod_tech_research(int faction_id, int value) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum && value >= 0);
    Faction* f = &Factions[faction_id];
    if (!conf.revised_tech_cost) {
        tech_research(faction_id, value);
        return;
    }
    if (value > 0) {
        f->tech_accumulated += value;
    }
    if (value > 0 && (!*MultiplayerActive || f->tech_research_id >= 0)) {
        if (f->tech_accumulated >= mod_tech_rate(faction_id)) {
            debug("tech_research %d %d %s\n", *CurrentTurn, faction_id, Tech[f->tech_research_id].name);
            int tech_id = tech_advance(faction_id);
            if (tech_id >= 0) {
                mon_tech_discovered(faction_id, tech_id);
            }
            f->tech_accumulated = 0;
        }
        if (f->player_flags & PFLAG_FIRST_SECRETS) {
            f->player_flags &= ~PFLAG_FIRST_SECRETS;
            int tech_id = tech_advance(faction_id);
            if (tech_id >= 0) {
                debug("tech_secrets %d %d %s\n", *CurrentTurn, faction_id, Tech[tech_id].name);
                mon_tech_discovered(faction_id, tech_id);
            }
        }
    }
    if (f->tech_accumulated >= 0 && f->tech_research_id < 0 && *NetUpkeepState != 1) {
        tech_selection(faction_id);
    }
}

/*
Normally the game engine would recalculate research cost in tech_research
before the next tech is selected in tech_selection > tech_pick.
This version recalculates tech cost immediately after a new tech is selected.
Note that tech_pick is also used for TECHSTEAL faction ability.
*/
int __cdecl mod_tech_pick(int faction_id, int flag, int other_faction_id, const char* label) {
    Faction* f = &Factions[faction_id];
    int tech_id = tech_pick(faction_id, flag, other_faction_id, (int)label);
    if (other_faction_id < 0 && tech_id >= 0 && tech_id != f->tech_research_id) {
        if (conf.revised_tech_cost) {
            f->tech_cost = tech_cost(faction_id, tech_id);
        } else {
            f->tech_cost = -1; // Must be negative to reset the cost
            f->tech_cost = tech_rate(faction_id);
        }
        debugw("tech_pick %d %d %s\n", *CurrentTurn, faction_id, Tech[tech_id].name);
    }
    return tech_id;
}

int __cdecl mod_tech_rate(int faction_id) {
    Faction* f = &Factions[faction_id];
    if (!conf.revised_tech_cost) {
        return tech_rate(faction_id);
    }
    return (f->tech_cost > 0 ? f->tech_cost : 9999);
}

int __cdecl mod_tech_val(int tech_id, int faction_id, int flag) {
    int value = tech_val(tech_id, faction_id, flag);
    if (conf.tech_balance && thinker_enabled(faction_id)) {
        if (tech_id == Weapon[WPN_TERRAFORMING_UNIT].preq_tech
        || tech_id == Weapon[WPN_SUPPLY_TRANSPORT].preq_tech
        || tech_id == Weapon[WPN_PROBE_TEAM].preq_tech
        || tech_id == Facility[FAC_RECYCLING_TANKS].preq_tech
        || tech_id == Facility[FAC_CHILDREN_CRECHE].preq_tech
        || tech_id == Rules->tech_preq_allow_3_energy_sq
        || tech_id == Rules->tech_preq_allow_3_minerals_sq
        || tech_id == Rules->tech_preq_allow_3_nutrients_sq) {
            value += 50;
        }
    }
    if (conf.debug_verbose) {
        debug("tech_value %d %d value: %3d tech: %2d %s\n",
        *CurrentTurn, faction_id, value, tech_id, Tech[tech_id].name);
    }
    return value;
}

int __cdecl mod_tech_ai(int faction_id) {
    int tech_id = -1;
    int best_value = INT_MIN;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        if (mod_tech_avail(i, faction_id)) {
            int tech_value = mod_tech_val(i, faction_id, false);
            int value;
            if (*GameRules & RULES_BLIND_RESEARCH) {
                if (is_human(faction_id) && i == Units[BSC_FORMERS].preq_tech
                && (Factions[faction_id].AI_growth || Factions[faction_id].AI_wealth)) {
                    return i;
                }
                int preq = tech_level(i, 0); // Replaces tech_recurse
                tech_value = preq ? (tech_value << 8) / preq : 0;
            }
            // Multiple functions might be related to network multiplayer
            value = is_human(faction_id) ? game_random(0, tech_value + 1)
                : (tech_value > 0 ? game_rand() % (tech_value + 1) : 0);
            if (value > best_value) {
                best_value = value;
                tech_id = i;
            }
        }
    }
    return tech_id;
}

int tech_level(int tech_id, int lvl) {
    if (tech_id < 0 || tech_id > TECH_TranT || lvl > TECH_TranT) {
        return lvl;
    } else {
        int v1 = tech_level(Tech[tech_id].preq_tech1, lvl + 1);
        int v2 = tech_level(Tech[tech_id].preq_tech2, lvl + 1);
        return max(v1, v2);
    }
}

int tech_cost(int faction_id, int tech_id) {
    MFaction* m = &MFactions[faction_id];
    int level = 1;
    int owners = 0;
    int our_techs = 0;

    if (tech_id >= 0) {
        level = tech_level(tech_id, 0);
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && is_alive(i) && has_tech(tech_id, i)
            && has_treaty(faction_id, i, DIPLO_COMMLINK)) {
                owners += (has_treaty(faction_id, i, DIPLO_PACT|DIPLO_HAVE_INFILTRATOR) ? 2 : 1);
            }
        }
    }
    for (int i = Tech_ID_First; i <= Tech_ID_Last; i++) {
        if (Tech[i].preq_tech1 != TECH_Disable && has_tech(i, faction_id)) {
            our_techs++;
        }
    }
    double cost_base = (5*level*level*level + 25*level*level + 20*our_techs);
    double cost_diff = (is_human(faction_id) ? 1.0 : conf.tech_cost_factor[*DiffLevel] / 100.0)
        * clamp(our_techs + 6, 6, 16) / 16.0;

    double cost = cost_base
        * cost_diff
        * *MapAreaSqRoot / 56.0
        * max(1, m->rule_techcost) / 100.0
        * (*GameRules & RULES_TECH_STAGNATION ? conf.tech_stagnate_rate / 100.0 : 1.0)
        * 100.0 / max(1, Rules->rules_tech_discovery_rate)
        * (1.0 - 0.05*min(6, owners));

    debug("tech_cost %d %d base: %7.2f diff: %.2f cost: %7.2f "
    "level: %d our_techs: %d owners: %d tech: %2d %s\n",
    *CurrentTurn, faction_id, cost_base, cost_diff, cost,
    level, our_techs, owners, tech_id, (tech_id >= 0 ? Tech[tech_id].name : NULL));

    return clamp((int)cost, 1, 99999999);
}



