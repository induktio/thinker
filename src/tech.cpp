
#include "tech.h"


static bool revised_tech_cost() {
    // Not supported during multiplayer
    return conf.revised_tech_cost && !*MultiplayerActive;
}

const char* tech_str(int tech_id) {
    if (tech_id >= 0 && Tech[tech_id].name != NULL) {
        return Tech[tech_id].name;
    }
    return "-";
}

/*
Craft an output string related to a specific technology. For techs outside the standard
range, craft a string related to world map, comm links or prototypes.
*/
void __cdecl say_tech(char* output, int tech_id, int incl_category) {
    size_t prev = strnlen(output, StrBufLen);
    char* buf = output + prev;
    size_t len = StrBufLen - prev;
    if (tech_id < -1) {
        snprintf(buf, len, "%s", label_get(310)); // Not Available
    } else if (tech_id < 0) {
        snprintf(buf, len, "%s", label_get(25)); // NONE
    } else if (tech_id == 9999) {
        snprintf(buf, len, "%s", label_get(306)); // World Map
    } else if (tech_id < MaxTechnologyNum) {
        if (incl_category) {
            snprintf(buf, len, "%s (%s%d)", Tech[tech_id].name,
                label_get(629 + tech_category(tech_id)), // 'E#', 'D#', 'B#', 'C#'
                tech_level(tech_id, 0));
        } else {
            snprintf(buf, len, "%s", Tech[tech_id].name);
        }
    } else if (tech_id < 97) {
        int faction_id = tech_id - MaxTechnologyNum;
        if (*GameLanguage) {
            snprintf(buf, len, "%s (%s)", label_get(487), // Comm Frequency
                parse_set(faction_id));
        } else {
            snprintf(buf, len, "%s %s",
                MFactions[faction_id].adj_name_faction, label_get(487)); // Comm Frequency
        }
    } else {
        snprintf(buf, len, "%s %s", Units[tech_id - 97].name, label_get(185)); // Prototype
    }
}

int __cdecl has_tech(int tech_id, int faction_id) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    assert(tech_id >= TECH_Disable && tech_id <= TECH_TranT);
    if (faction_id <= 0) {
        return false;
    }
    if (tech_id == TECH_None) {
        return true;
    }
    if (tech_id < 0 || tech_id >= TECH_TranT
    || Tech[tech_id].preq_tech1 == TECH_Disable
    || (Tech[tech_id].preq_tech2 == TECH_Disable
    && Tech[tech_id].preq_tech1 != TECH_None)) {
        return false;
    }
    return (TechOwners[tech_id] & (1 << faction_id)) != 0;
}

/*
Calculate technology level for tech_id. Replaces function tech_recurse.
*/
int __cdecl tech_level(int tech_id, int lvl) {
    if (tech_id < 0 || tech_id > TECH_TranT || lvl > TECH_TranT) {
        return lvl;
    } else {
        int v1 = tech_level(Tech[tech_id].preq_tech1, lvl + 1);
        int v2 = tech_level(Tech[tech_id].preq_tech2, lvl + 1);
        return max(v1, v2);
    }
}

/*
Determine what category is the most important for tech_id.
If there is a tie, the order of precedence is as follows: growth > tech > wealth > power.
Return Value: Tech category id: growth (0), tech (1), wealth (2) or power (3).
*/
int __cdecl tech_category(int tech_id) {
    assert(tech_id >= TECH_None && tech_id <= TECH_TranT);
    if (tech_id < 0 || tech_id > TECH_TranT) {
        return TCAT_GROWTH;
    }
    CTech& tech = Tech[tech_id];
    int label = TCAT_GROWTH;
    int value = tech.AI_growth;
    if (tech.AI_tech > value) {
        label = TCAT_TECH;
        value = tech.AI_tech;
    }
    if (tech.AI_wealth > value) {
        label = TCAT_WEALTH;
        value = tech.AI_wealth;
    }
    return (tech.AI_power > value) ? TCAT_POWER : label;
}

/*
Check to see whether provided faction can research a specific technology.
Includes checks to prevent SMACX specific techs from being researched in SMAC mode.
*/
int __cdecl tech_avail(int tech_id, int faction_id) {
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
Calculate the tech related bonuses for commerce and resource production in fungus.
*/
void __cdecl tech_effects(int faction_id) {
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
        if (m->faction_bonus_id[i] == RULE_FUNGNUTRIENT) {
            f->tech_fungus_nutrient += m->faction_bonus_val1[i];
        } else if (m->faction_bonus_id[i] == RULE_FUNGMINERALS) {
            f->tech_fungus_mineral += m->faction_bonus_val1[i];
        } else if (m->faction_bonus_id[i] == RULE_FUNGENERGY) {
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

void __cdecl tech_research(int faction_id, int value) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum && value >= 0);
    CFacility& fac_psigate = Facility[FAC_PSI_GATE];
    CFacility& fac_ascent = Facility[FAC_ASCENT_TO_TRANSCENDENCE];
    Faction& plr = Factions[faction_id];
    if (value > 0) {
        if (!is_human(faction_id) && climactic_battle()) {
            // Original also applied this hidden AI bonus for faction_id <= 0.
            // The reason for this is unclear and it may not be useful so it is removed.
            // PSI Gate checks are related to conditions on climactic_battle.
            if ((fac_psigate.preq_tech != TECH_None
            && (fac_psigate.preq_tech < 0 || fac_psigate.preq_tech >= TECH_TranT
            || (Tech[fac_psigate.preq_tech].preq_tech1 < TECH_None)
            || (Tech[fac_psigate.preq_tech].preq_tech2 < TECH_None
            && Tech[fac_psigate.preq_tech].preq_tech1 != TECH_None)
            || !(TechOwners[fac_psigate.preq_tech] & (1 << faction_id))))
            || (fac_ascent.preq_tech != TECH_None
            && (fac_ascent.preq_tech < 0 || fac_ascent.preq_tech >= TECH_TranT
            || (Tech[fac_ascent.preq_tech].preq_tech1 < TECH_None)
            || (Tech[fac_ascent.preq_tech].preq_tech2 < TECH_None
            && Tech[fac_ascent.preq_tech].preq_tech1 != TECH_None)
            || !(TechOwners[fac_ascent.preq_tech] & (1 << faction_id))))) {
                value *= 2;
            }
        }
        plr.tech_accumulated += value;
        if (*MultiplayerActive || plr.tech_research_id >= 0) {
            if (plr.tech_research_id >= 89) { // Possibly unused game mechanic
                plr.tech_accumulated += value;
            }
            if (plr.tech_accumulated >= mod_tech_rate(faction_id)) {
                int tech_id = tech_advance(faction_id);
                debug("tech_advance %d %d %d %s\n", *CurrentTurn, faction_id, tech_id, tech_str(tech_id));
                if (tech_id >= 0) {
                    mon_tech_discovered(faction_id, tech_id);
                }
                plr.tech_accumulated = 0;
                if (!revised_tech_cost()) {
                    plr.tech_cost = -1; // Must be negative to reset the cost
                    plr.tech_cost = mod_tech_rate(faction_id);
                }
            }
            if (plr.player_flags & PFLAG_FIRST_SECRETS) {
                plr.player_flags &= ~PFLAG_FIRST_SECRETS;
                int tech_id = tech_advance(faction_id);
                debug("tech_secrets %d %d %d %s\n", *CurrentTurn, faction_id, tech_id, tech_str(tech_id));
                if (tech_id >= 0) {
                    mon_tech_discovered(faction_id, tech_id);
                }
            }
        }
        if (plr.tech_accumulated >= 0 && plr.tech_research_id < 0 && *NetUpkeepState != 1
        && (!is_human(faction_id) || faction_id == *CurrentPlayerFaction || !*PbemActive)) {
            plr.tech_research_id = mod_tech_selection(faction_id);
        }
    }
}

int __cdecl mod_tech_selection(int faction_id) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    Faction& plr = Factions[faction_id];
    int tech_id;
    if (*MultiplayerActive && is_human(faction_id) && faction_id != *CurrentPlayerFaction) {
        bool done = false;
        while (plr.tech_research_id < 0 && !*ControlTurnA) {
            NetDaemon_net_tasks(NetState);
            if (!done) {
                done = true;
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                // Note that this script label does not display title/name by default
                NetMsg_pop(NetMsg, "PICKINGTECH", 0, 0, 0);
            }
        }
        NetMsg_close(NetMsg);
        tech_id = plr.tech_research_id;
    } else {
        tech_id = mod_tech_pick(faction_id, 0, -1, 0);
        plr.tech_research_id = tech_id;
        if (*MultiplayerActive && is_human(faction_id)) {
            synch_researching(faction_id);
            plr.tech_research_id = -1;
            while (plr.tech_research_id < 0 && !*ControlTurnA) {
                NetDaemon_net_tasks(NetState);
            }
        }
    }
    return tech_id;
}

/*
Normally the game engine would recalculate research cost in tech_research
before the next tech is selected in tech_selection > tech_pick.
This version recalculates tech cost immediately after a new tech is selected.
Note that tech_pick is also used for TECHSTEAL faction ability.
*/
int __cdecl mod_tech_pick(int faction_id, int flag, int other_faction_id, const char* label) {
    Faction& plr = Factions[faction_id];
    int tech_id = tech_pick(faction_id, flag, other_faction_id, label);
    debug("tech_pick %d %d %d prev: %d %d tech: %d %s\n", *CurrentTurn, faction_id, other_faction_id,
        plr.tech_research_id, plr.tech_cost, tech_id, tech_str(tech_id));
    if (other_faction_id < 0 && revised_tech_cost()) {
        if (tech_id >= 0 && (tech_id != plr.tech_research_id || plr.tech_cost <= 0)) {
            plr.tech_cost = tech_alt_cost(tech_id, faction_id);
        }
    }
    flushlog();
    return tech_id;
}

int __cdecl mod_tech_rate(int faction_id) {
    Faction& plr = Factions[faction_id];
    if (revised_tech_cost()) {
        return (plr.tech_cost > 0 ? plr.tech_cost : 9999);
    }
    // Calculate the original game tech cost values
    if (plr.tech_cost >= 0) {
        assert(plr.tech_cost == tech_rate(faction_id));
        return plr.tech_cost;
    }
    if (!Rules->tech_discovery_rate) {
        assert(999999999 == tech_rate(faction_id));
        return 999999999;
    }
    int plr_tech_rating = clamp(plr.tech_ranking + 2 * plr.earned_techs_saved - plr.unk_27, 2, 9999);
    int max_tech = 0;
    for (int i = 0; i < MaxPlayerNum; ++i) {
        max_tech = max(max_tech, Factions[i].tech_ranking + 2 * Factions[i].earned_techs_saved);
    }
    bool is_plr = is_human(faction_id);
    bool is_stagnate = *GameRules & RULES_TECH_STAGNATION;
    int half_max_tech = max_tech / 2;
    int half_plr_tech = plr_tech_rating / 2;
    int diff = is_plr ? plr.diff_level : *DiffLevel;
    int diff_adj = (diff < 3) ? (diff + 1) : diff;
    int diff_val = is_plr ? 3 : *DiffLevel;
    int factor = is_plr ? (4 * diff_adj + 8) : (29 - 3 * diff_adj);
    factor = clamp(factor, 12 - half_plr_tech, 12 + half_plr_tech);
    int turn_penalty = half_plr_tech - (*CurrentTurn / (is_stagnate ? 12 : 8));
    turn_penalty = clamp(turn_penalty, 0, (factor * (is_stagnate ? 3 : 2)) / 2);
    int factor_adj = turn_penalty + factor;
    int reduction = (half_max_tech - diff_val - half_plr_tech + 7) / (8 - diff_val);
    reduction = clamp(reduction, 0, (diff_val * factor_adj / 10) + 1);
    int tech_multiplier = clamp(half_plr_tech - clamp(plr.SE_research_base, -1, 1), 1, 99999);
    // Fix: avoid possible overflow issues with custom rules by casting values to int64_t
    int64_t cost = int64_t(factor_adj - reduction) * tech_multiplier;
    if (Rules->tech_discovery_rate != 100) {
        cost = (cost * 100) / Rules->tech_discovery_rate;
    }
    if (MFactions[faction_id].rule_techcost != 100) {
        cost = (cost * MFactions[faction_id].rule_techcost) / 100;
    }
    cost = cost * *MapAreaSqRoot / 56;
    if (is_stagnate) {
        cost += cost / 2;
    }
    cost = clamp(cost, int64_t(1), int64_t(99999999));
    assert(cost == tech_rate(faction_id));
    return cost;
}

/*
Determine if preq_tech_id is a prerequisite of parent_tech_id within descending range.
Return Value: Is preq_tech_id prerequisite of parent_tech_id? true/false
*/
int __cdecl tech_is_preq(int preq_tech_id, int parent_tech_id, int range) {
    if (preq_tech_id < 0 || parent_tech_id < 0) {
        return false;
    }
    if (preq_tech_id == parent_tech_id) {
        return true;
    }
    if (range <= 0) {
        return false;
    }
    return tech_is_preq(preq_tech_id, Tech[parent_tech_id].preq_tech1, range - 1)
        || tech_is_preq(preq_tech_id, Tech[parent_tech_id].preq_tech2, range - 1);
}

/*
Determine how valuable the specified tech_id is to a faction. This tech_id either corresponds
to a technology (0-88), another faction (89-96) or a prototype (97-608). The 3rd parameter
determines whether a simplistic or extended calculation is required for technology.
Return Value: Value of tech_id to the specified faction
*/
int __cdecl mod_tech_val(int tech_id, int faction_id, int simple_calc) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    if (tech_id == 9999) {
        return 2;
    }
    int value;
    if (tech_id < MaxTechnologyNum) {
        CTech* tech = &Tech[tech_id];
        int enemy_count = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && has_treaty(faction_id, i, DIPLO_VENDETTA)) {
                enemy_count++;
            }
        }
        int factor_ai = 1;
        if (!simple_calc) {
            factor_ai = (*GameRules & RULES_BLIND_RESEARCH) ? 4 : 2;
        }
        value = tech->AI_growth * (factor_ai * f->AI_growth + 1)
            + tech->AI_wealth * (factor_ai * f->AI_wealth + 1)
            + tech->AI_tech * (factor_ai * f->AI_tech + 1)
            + tech->AI_power * (factor_ai * f->AI_power + 1);
        int base_count = Factions[faction_id].base_count;
        if ((!tech->AI_power || (!f->AI_power && !enemy_count))
        && (!tech->AI_tech || !f->AI_tech)
        && (!tech->AI_wealth || !f->AI_wealth)
        && (!tech->AI_growth || (!f->AI_growth && base_count >= 4))) {
            value = (value + 1) / 2;
        }
        bool is_player = is_human(faction_id);
        int tech_id_lvl = tech_level(tech_id, 0);
        if (!is_player && !has_tech(tech_id, faction_id) && simple_calc) {
            int owners = __builtin_popcount(TechOwners[tech_id]);
            if (owners > 1) {
                value += 2 - 2 * owners;
            }
            int search_lvl = 1;
            for (int i = 0; i < MaxTechnologyNum; i++) {
                if (has_tech(i, faction_id)) {
                    int current_lvl = tech_level(i, 0);
                    if (search_lvl < current_lvl) {
                        search_lvl = current_lvl;
                    }
                }
            }
            if (tech_id_lvl < search_lvl) {
                value = value * (tech_id_lvl + 1) / (search_lvl + 1);
            }
            if (value < 1) {
                value = 1;
            }
        }
        if (simple_calc) {
            assert(value == tech_val(tech_id, faction_id, simple_calc));
            return value;
        }
        if (base_count) {
            for (int region = 1; region < MaxRegionLandNum; region++) {
                if (!bad_reg(region)) {
                    int pwr_base = f->region_total_bases[region] * tech->AI_power;
                    int plan = f->region_base_plan[region];
                    if (plan == PLAN_NAVAL_TRANSPORT && enemy_count && !is_player) {
                        value += (pwr_base / base_count);
                    } else if (plan == PLAN_DEFENSE) {
                        value += (pwr_base * 4) / (base_count * (is_player + 1));
                    } else if (plan == PLAN_OFFENSE) {
                        value += (pwr_base * ((f->best_weapon_value
                            >= f->enemy_best_weapon_value) ? 2 : 4))
                             / (base_count * (is_player + 1));
                    } else {
                        for (int i = 1; i < MaxPlayerNum; i++) {
                            if (i != faction_id && Factions[i].region_total_bases[region]
                            && f->region_total_bases[region]
                            && has_treaty(faction_id, i, DIPLO_COMMLINK)
                            && (!has_treaty(faction_id, i, DIPLO_PACT | DIPLO_TREATY)
                            || has_treaty(faction_id, i, DIPLO_WANT_REVENGE))) {
                                value += (pwr_base / (base_count * (is_player + 1)));
                            }
                        }
                    }
                }
            }
        }
        if (has_tech(tech_id, faction_id)) {
            return value;
        }
        if (climactic_battle()
        && tech_is_preq(tech_id, Facility[FAC_ASCENT_TO_TRANSCENDENCE].preq_tech, 2)) {
            value *= 4;
        }
        if (f->SE_planet_base > 0 && f->AI_growth) {
            if (tech_is_preq(tech_id, TECH_CentMed, 9999)) {
                value *= 3;
            }
            if (tech_is_preq(tech_id, TECH_PlaEcon, 9999)) {
                value *= 2;
            }
            if (tech_is_preq(tech_id, TECH_AlphCen, 3)) {
                value *= 2;
            }
        }
        if (f->SE_probe_base <= 0) {
            if (tech_is_preq(tech_id, Facility[FAC_HUNTER_SEEKER_ALGORITHM].preq_tech, f->AI_tech + 2)) {
                if (!f->AI_power) {
                    value *= 2;
                }
                if (f->AI_tech) {
                    value *= 2;
                }
            }
        }
        if (f->AI_growth && tech_is_preq(tech_id, TECH_DocInit, 2)) {
            value *= 2;
        }
        if ((f->AI_wealth || !*MapCloudCover) && tech_is_preq(tech_id, TECH_EnvEcon, 9999)) {
            value *= 2;
        }
        if (Tech[tech_id].flags & TFLAG_SECRETS && !TechOwners[tech_id]
        && !(*GameRules & RULES_BLIND_RESEARCH)) {
            value *= (f->AI_power + 1) * 2;
        }
        if (m->rule_psi > 0) {
            if (tech_is_preq(tech_id, Facility[FAC_DREAM_TWISTER].preq_tech, 9999)) {
                value *= 2;
            }
        } else {
            int preq_tech_fusion = Reactor[REC_FUSION - 1].preq_tech;
            if (tech_id == preq_tech_fusion) {
                value *= 2;
            }
            if (tech_id == Reactor[REC_QUANTUM - 1].preq_tech) {
                value *= 2;
            }
            if (tech_is_preq(tech_id, preq_tech_fusion, 9999)) {
                value++;
            }
            if (tech_is_preq(tech_id, preq_tech_fusion, 1)
            && !(*GameRules & RULES_BLIND_RESEARCH)) {
                value *= 2;
            }
        }
        int eco_dmg_unk = f->unk_47 / clamp(base_count, 1, 9999);
        if (eco_dmg_unk > f->AI_power
        && (tech_is_preq(tech_id, Facility[FAC_HYBRID_FOREST].preq_tech, 9999)
        || tech_is_preq(tech_id, Facility[FAC_TREE_FARM].preq_tech, 9999)
        || tech_is_preq(tech_id, Facility[FAC_CENTAURI_PRESERVE].preq_tech, 9999)
        || tech_is_preq(tech_id, Facility[FAC_TEMPLE_OF_PLANET].preq_tech, 9999))) {
            value += eco_dmg_unk;
        }
        if (m->rule_population > 0) {
            if (tech_is_preq(tech_id, Facility[FAC_HAB_COMPLEX].preq_tech, 9999)) {
                value *= 2;
            } else if (*CurrentTurn > 250
            && tech_is_preq(tech_id, Facility[FAC_HABITATION_DOME].preq_tech, 9999)) {
                value = (value * 3) / 2;
            }
        }
        if (f->AI_power) {
            for (int i = 0; i < MaxWeaponNum; i++) {
                if (Weapon[i].offense_value) {
                    int weap_preq_tech = Weapon[i].preq_tech;
                    if (tech_id == weap_preq_tech) {
                        value *= (is_player + 3);
                    } else if (tech_is_preq(tech_id, weap_preq_tech, 1)) {
                        value *= (is_player + 2);
                    }
                }
            }
        }
        if (f->AI_tech || !f->AI_power) {
            for (int i = 0; i < MaxTechnologyNum; i++) {
                if (!has_tech(i, faction_id) && Tech[i].flags & TFLAG_SECRETS
                && !TechOwners[i] && tech_is_preq(tech_id, i, 1)) {
                    value *= 3;
                }
            }
        }
        if (tech_is_preq(tech_id, Units[BSC_FORMERS].preq_tech, 9999)
        && !has_tech(Units[BSC_FORMERS].preq_tech, faction_id)) {
            value *= 2;
            if (is_player) {
                value *= 2;
            }
        }
        if (tech_is_preq(tech_id, Chassis[CHS_FOIL].preq_tech, 9999)
        && !has_tech(Chassis[CHS_FOIL].preq_tech, faction_id)) {
            bool toggle = false;
            for (int region = 1; region < MaxRegionLandNum; region++) {
                if (f->region_total_bases[region]) {
                    for (int i = 1; i < MaxPlayerNum; i++) {
                        if (faction_id != i && !Factions[i].region_total_bases[region]) {
                            toggle = true;
                            break;
                        }
                    }
                    if (toggle && f->region_visible_tiles[region]
                    >= Continents[region].tile_count) {
                        value *= 3;
                        if (is_player) {
                            value *= 2;
                        }
                        break;
                    }
                }
            }
            if (toggle) {
                value = (value * 2) + 4;
            }
        }
        assert(value == tech_val(tech_id, faction_id, simple_calc));
        if (conf.tech_balance) {
            bool high_cost = revised_tech_cost() && tech_id_lvl > 2;
            if (tech_id == Weapon[WPN_TERRAFORMING_UNIT].preq_tech) {
                value += (high_cost ? 60 : 120);
            } else if (tech_id == Weapon[WPN_SUPPLY_TRANSPORT].preq_tech
            || tech_id == Facility[FAC_RECYCLING_TANKS].preq_tech
            || tech_id == Facility[FAC_CHILDREN_CRECHE].preq_tech
            || tech_id == Facility[FAC_RECREATION_COMMONS].preq_tech
            || tech_id == Rules->tech_preq_allow_3_nutrients_sq
            || tech_id == Rules->tech_preq_allow_3_minerals_sq
            || tech_id == Rules->tech_preq_allow_3_energy_sq) {
                value += (high_cost ? 20 : 40);
            }
        }
    } else if (tech_id < 97) { // Factions
        int factor = 1;
        int faction_id_2 = tech_id - MaxTechnologyNum;
        if (!mod_wants_to_attack(faction_id, faction_id_2, 0)) {
            factor++;
        }
        if (!mod_wants_to_attack(faction_id_2, faction_id, 0)) {
            factor++;
        }
        value = factor * (factor / (f->AI_fight + 2));
        assert(value == tech_val(tech_id, faction_id, simple_calc));
    } else {  // Prototypes
        UNIT* u = &Units[tech_id - 97];
        value = clamp(u->offense_value(), 1, 2)
            + clamp(u->defense_value(), 1, 2)
            + clamp((int)u->speed(), 1, 2)
            + u->reactor_id - 2;
        assert(value == tech_val(tech_id, faction_id, simple_calc));
    }
    return value;
}

int __cdecl mod_tech_ai(int faction_id) {
    int tech_id = -1;
    int best_value = INT_MIN;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        if (tech_avail(i, faction_id)) {
            int tech_value = mod_tech_val(i, faction_id, false);
            debug("tech_val %d %d value: %3d tech: %2d %s\n",
            *CurrentTurn, faction_id, tech_value, i, Tech[i].name);
            if (*GameRules & RULES_BLIND_RESEARCH) {
                if (is_human(faction_id) && i == Units[BSC_FORMERS].preq_tech
                && (Factions[faction_id].AI_growth || Factions[faction_id].AI_wealth)) {
                    return i;
                }
                int preq = tech_level(i, 0); // Replaces tech_recurse
                tech_value = preq ? (tech_value << 8) / preq : 0;
            }
            // Replace previous functions game_random (for players) and game_rand (for AIs)
            int value = random_get(0, tech_value + 1);
            if (value > best_value) {
                best_value = value;
                tech_id = i;
            }
        }
    }
    return tech_id;
}

int __cdecl tech_advance(int faction_id) {
    Faction* plr = &Factions[faction_id];
    MFaction* fac = &MFactions[faction_id];
    log_say("Earning a Tech", fac->name_leader, plr->tech_research_id, 0, 0);
    if (*MultiplayerActive && *NetUpkeepState == 1) {
        log_say("Saving Earned Tech for Later", fac->name_leader, faction_id, 0, 0);
        ++plr->earned_techs_saved;
        return -1;
    }
    if (*PbemActive && is_human(faction_id) && faction_id != MapWin->cOwner) {
        ++plr->earned_techs_saved;
        return -1;
    }
    int tech_id = plr->tech_research_id;
    if (tech_id >= 0 || (tech_id = mod_tech_selection(faction_id), tech_id >= 0)) {
        plr->tech_research_id = -1;
        tech_achieved(faction_id, tech_id, 0, 0);
        if (!is_human(faction_id)) {
            int base_id = *CurrentBaseID;
            mod_bases_reset(-1, faction_id, 0);
            if (*CurrentTurn) {
                if (base_id >= 0 && base_id < *BaseCount) {
                    set_base(base_id);
                    base_compute(0);
                }
            }
        }
        if ((*MultiplayerActive || faction_id == MapWin->cOwner)
        && !*SkipTechScreenA && !*SkipTechScreenB && plr->tech_research_id < 0) {
            plr->tech_research_id = mod_tech_selection(faction_id);
        }
    }
    return tech_id;
}

/*
Applies all side effects of gaining a tech: notifications, prototype designs,
free facility grants, map reveal, social engineering prompts, and propagation
to other factions via Planetary Datalinks and TECHSHARE faction ability.
Also multiplexes several unrelated events through tech_id's range.

faction_id   - Faction receiving the item. Must be in range 0..7.
tech_id      - 0..88    : a real technology (TECH_TranT == 88 is the last one).
               89..96   : grant comm-frequency with faction (tech_id - 89).
               97..9998 : propose prototype for unit_id (tech_id - 97).
               >= 9999  : gift/trade world map from faction_id_2.
faction_id_2 - Meaningful only when tech_id < 89 or >= 9999:
               >0 : source faction_id, trade/gift/steal.
                0 : no other source, researched normally.
               -1 : internal, granted via Planetary Datalinks propagation.
               -2 : internal, granted via TECHSHARE propagation.
is_share     - Non-zero for a shared/leaked tech (changes popup text).
*/
void __cdecl tech_achieved(int faction_id, int tech_id, int faction_id_2, int is_share) {
    if (!(faction_id >= 0 && faction_id < MaxPlayerNum)) {
        assert(0);
        return;
    }
    if (tech_id < 0) {
        return;
    }
    Faction* plr = &Factions[faction_id];
    MFaction* fac = &MFactions[faction_id];
    const int bm_fac = 1 << faction_id;
    Popup cur_popup = {};
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });

    log_say("Gaining a TECH", fac->name_leader, faction_id, tech_id, faction_id_2);
    if (is_human(faction_id)) {
        ambience(201);
    }
    if (tech_id >= 9999) {
        if (!(faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum)) {
            assert(0);
            return;
        }
        const int bm_tgt = 1 << faction_id_2;
        plr->traded_maps |= bm_tgt | Factions[faction_id_2].traded_maps;
        for (int i = 0; i < *MapAreaTiles; ++i) {
            MAP* sq = &(*MapTiles)[i];
            if (sq->visibility & bm_tgt) {
                sq->visibility |= bm_fac;
                if (faction_id && faction_id_2) {
                    sq->visible_items[faction_id - 1] |= sq->items & sq->visible_items[faction_id_2 - 1];
                }
            }
        }
        for (int i = 0; i < *BaseCount; ++i) {
            if ((Bases[i].visibility & bm_tgt) || Bases[i].faction_id == faction_id_2) {
                spot_base(i, faction_id);
            }
        }
        if (faction_id == MapWin->cOwner) {
            draw_map(1);
            *GameDrawState |= 4u;
            GraphicWin_redraw(WorldWin);
        }
        return;
    }
    if (tech_id >= 89) {
        if (tech_id < 97) {
            treaty_on(faction_id, tech_id - 89, DIPLO_COMMLINK);
            return;
        }
        if (tech_id - 97 >= MaxProtoNum) {
            assert(0);
            return;
        }
        UNIT* src = &Units[tech_id - 97];
        int uid = mod_propose_proto(
            faction_id,
            (VehChassis)src->chassis_id,
            (VehWeapon)src->weapon_id,
            (VehArmor)src->armor_id,
            (VehAblFlag)src->ability_flags,
            (VehReactor)src->reactor_id,
            PLAN_PROTOTYPE_DONE,
            src->name);
        if (uid >= 0) {
            if (*MultiplayerActive && is_human(faction_id)) {
                Units[uid].unit_flags |= UNIT_UNK_100;
            } else {
                prune_protos(faction_id, uid, 0);
                upgrade_any_prototypes(faction_id);
            }
        }
        return;
    }
    if (plr->tech_research_id == tech_id) {
        plr->tech_research_id = -1;
    }
    plr->tech_trade_source[tech_id] = faction_id_2 <= 0 ? 0 : faction_id_2;
    int is_first = TechOwners[tech_id] == 0;
    TechOwners[tech_id] |= (1 << faction_id);
    ++plr->tech_achieved;
    if (!*CurrentTurn || *SkipTechScreenA) {
        return;
    }
    ++plr->tech_ranking;
    if (plr->unk_27) {
        --plr->unk_27;
    }
    ++plr->tech_ranking;
    if (plr->unk_27) {
        --plr->unk_27;
    }
    if (tech_id == TECH_TranT) {
        ++plr->tech_count_transcendent;
    }
    int best_rc_id = 0;
    for (int rc_id = REC_FUSION; rc_id <= REC_SINGULARITY; ++rc_id) {
        if (tech_id == Reactor[rc_id - 1].preq_tech) {
            for (int i = 0; i < MaxProtoFactionNum; ++i) {
                int unit_id = i + (faction_id * MaxProtoFactionNum);
                // The original game does not check for MPREF_BSC_AUTO_PRUNE_OBS_VEH when discovering
                // better reactors, usually resulting in dozens of popups for each reactor tech
                UNIT* cur = &Units[unit_id];
                if ((cur->unit_flags & UNIT_ACTIVE)
                && !(cur->obsolete_factions & bm_fac) && cur->reactor_id < rc_id
                && (!conf.ignore_reactor_power || !is_human(faction_id)
                || *GameMorePreferences & MPREF_BSC_AUTO_PRUNE_OBS_VEH)) {
                    // Fix: remove redundant loop code
                    int uid = mod_propose_proto(
                        faction_id,
                        (VehChassis)cur->chassis_id,
                        (VehWeapon)cur->weapon_id,
                        (VehArmor)cur->armor_id,
                        (VehAblFlag)cur->ability_flags,
                        (VehReactor)rc_id,
                        (cur->unit_flags & UNIT_PROTOTYPED) ? PLAN_PROTOTYPE_DONE : PLAN_AUTO_CALCULATE,
                        0);
                    if (uid != unit_id) {
                        if (*MultiplayerActive && is_human(faction_id)) {
                            Units[uid].unit_flags |= UNIT_UNK_100;
                        } else {
                            prune_protos(faction_id, uid, 0);
                            upgrade_any_prototypes(faction_id);
                        }
                    }
                    best_rc_id = rc_id;
                }
            }
        }
    }
    if (best_rc_id && faction_id == MapWin->cOwner && !*MultiplayerActive) {
        parse_says(0, Reactor[best_rc_id - 1].name, -1, -1);
        X_pop("REACTORUPGRADE", 0);
    }
    *dword_68F0F0 = -1;
    if (*MultiplayerActive) {
        plr->player_flags |= PFLAG_MULTI_TECH_ACHIEVED;
    } else if (!is_human(faction_id) || *GamePreferences & PREF_BSC_AUTO_DESIGN_VEH) {
        for (int i = 0; i < MaxProtoFactionNum; ++i) {
            Units[(faction_id * MaxProtoFactionNum) + i].unit_flags &= ~0xC0u;
        }
        consider_designs(faction_id);
        if (faction_id == MapWin->cOwner) {
            for (int i = 0; i < MaxProtoFactionNum; ++i) {
                UNIT* unit = &Units[i];
                if ((unit->unit_flags & UNIT_ACTIVE) && tech_id == unit->preq_tech && *dword_68F0F0 < 0) {
                    *dword_68F0F0 = i;
                }
            }
        }
    }
    *dword_7591A4 = -1;
    *dword_7591A8 = -1;
    if (faction_id == MapWin->cOwner || *GameState & STATE_OMNISCIENT_VIEW) {
        StrBuffer[0] = 0;
        say_tech(StrBuffer, tech_id, 0);
        parse_says(0, StrBuffer, -1, -1);
        parse_says(1, get_noun(faction_id), -1, -1);
        if (faction_id_2 <= 0) {
            if (faction_id_2 < 0) {
                if (faction_id_2 == -1) {
                    parse_says(2, Facility[FAC_PLANETARY_DATALINKS].name, -1, -1);
                } else {
                    parse_says(2, label_get(215), -1, -1); // Data Links
                }
            }
        } else {
            parse_says(2, get_noun(faction_id_2), -1, -1);
        }
        int pop_flag = 0;
        if (faction_id == MapWin->cOwner) {
            snprintf(StrBuffer, StrBufLen, "TECH%d", tech_id);
            BasePop_init(&cur_popup, 0, 0);
            BasePop_set_width(&cur_popup, 500);
            Popup_start(&cur_popup, BlurbsxFile, StrBuffer, -1, 0, 128, 0);
            BasePop_string(&cur_popup, "^");
            BasePop_string(&cur_popup, "^------");
            BasePop_string(&cur_popup, "^");
            pop_flag = 128;
        }
        StrBuffer[0] = 0;
        if (faction_id == MapWin->cOwner) {
            strcat(StrBuffer, "WE");
        } else {
            strcat(StrBuffer, "THEY");
        }
        if (faction_id_2) {
            strcat(StrBuffer, "ACQUIRE");
        } else {
            strcat(StrBuffer, "DEVELOP");
        }
        if (is_share) {
            StrBuffer[0] = 0;
            strcat(StrBuffer, "WEGETSHARETECH");
        }
        cur_popup.field_2144 = (cur_popup.dialogs.spriteBox[13] == 0 ? TechIcons[tech_id] : 0);
        SpriteBox_sprite((SpriteBox*)&cur_popup.dialogs.spriteBox, TechIcons[tech_id], 0, tech_id);
        cur_popup.field_3104 = 1;
        cur_popup.field_3108 = 2;
        Popup_start(&cur_popup, PopupScriptFile, StrBuffer, -1, 0, pop_flag, 0);
        if (faction_id == MapWin->cOwner) {
            help_tech_info_2(&cur_popup, tech_id, faction_id);
        }
        FX_play(Sounds, 51);
        if (faction_id == MapWin->cOwner) {
            if (*MultiplayerActive && (*DiploWinState || *dword_7492CC || *ControlWaitLoop)) {
                parse_says(0, Tech[tech_id].name, -1, -1);
                NetMsg_pop(NetMsg, "TECHOBTAINED", 5000, 0, 0);
            } else {
                NewTechWin_exec(NetTechWin, tech_id, faction_id_2);
            }
        } else if (!*MultiplayerActive) {
            BasePop_exec_3(&cur_popup, 0, 0);
        }
    }
    int has_freefac = 0;
    for (int i = 0; i < fac->faction_bonus_count; ++i) {
        if (fac->faction_bonus_id[i] == RULE_FREEFAC
        && tech_id == Facility[fac->faction_bonus_val1[i]].preq_tech) {
            has_freefac = 1;
            for (int base_id = 0; base_id < *BaseCount; ++base_id) {
                if (Bases[base_id].faction_id == faction_id) {
                    set_fac((FacilityId)fac->faction_bonus_val1[i], base_id, 1);
                }
            }
            if (faction_id == MapWin->cOwner) {
                parse_says(0, Tech[tech_id].name, -1, -1);
                parse_says(1, Facility[fac->faction_bonus_val1[i]].name, -1, -1);
                NetMsg_pop(NetMsg, "FREEFACTECH", -5000, 0, 0);
            }
        }
    }
    if (has_freefac) {
        NetDaemon_synch(NetState, 20, 0, 0, 0, 0, 1u, 0x2101);
    }
    for (int i = 0; i < fac->faction_bonus_count; ++i) {
        if (fac->faction_bonus_id[i] == RULE_FREEABIL
        && tech_id == Ability[fac->faction_bonus_val1[i]].preq_tech
        && faction_id == MapWin->cOwner) {
            parse_says(0, Tech[tech_id].name, -1, -1);
            parse_says(1, Ability[fac->faction_bonus_val1[i]].name, -1, -1);
            NetMsg_pop(NetMsg, "FREEABILTECH", -5000, 0, 0);
        }
    }
    if (Tech[tech_id].flags & TFLAG_SECRETS) {
        mon_secrets_of_tech(faction_id);
    }
    if (faction_id == MapWin->cOwner
    && tech_id == Terraform[FORMER_FOREST].preq_tech && !*MultiplayerActive) {
        parse_says(0, Tech[tech_id].name, -1, -1);
        X_pop_2(TutorFile, "TREETIME", 0);
    }
    if (faction_id == MapWin->cOwner
    && (*GamePreferences & PREF_BSC_AUTO_DESIGN_VEH)
    && *dword_68F0F0 >= 0 && !*MultiplayerActive) {
        int choice = 0;
        *dword_90EA3C = 0;
        *dword_73396C = 1;
        for (int i = 0; i < MaxProtoFactionNum; ++i) {
            // Fix: remove redundant Spore Launcher reference
            if ((Units[i].unit_flags & UNIT_ACTIVE)
            && tech_id == Units[i].preq_tech && !is_native_unit(i)) {
                if (choice) {
                    if (choice > 0) {
                        design_new_veh(-faction_id, i);
                    }
                } else {
                    if (!X_pop("ASKSEEDESIGN", 0)) {
                        choice = -1;
                    } else {
                        choice = 1;
                        design_new_veh(-faction_id, i);
                    }
                }
            }
        }
        for (int i = 0; i < MaxProtoFactionNum; ++i) {
            int uid = i + (faction_id * MaxProtoFactionNum);
            UNIT* cur = &Units[uid];
            if (cur->unit_flags & UNIT_UNK_80 && !(cur->obsolete_factions & bm_fac)) {
                if (choice) {
                    if (choice > 0) {
                        design_new_veh(-faction_id, uid);
                    }
                } else {
                    if (!X_pop("ASKSEEDESIGN", 0)) {
                        choice = -1;
                    } else {
                        choice = 1;
                        design_new_veh(-faction_id, uid);
                    }
                }
                cur->unit_flags &= ~UNIT_UNK_80;
            }
        }
        DesignWin_shutdown(DesignWin);
        for (int i = 0; i < MaxProtoFactionNum; ++i) {
            int uid = i + (faction_id * MaxProtoFactionNum);
            UNIT* cur = &Units[uid];
            if (!(cur->obsolete_factions & bm_fac)
            && cur->unit_flags & UNIT_UNK_100 && (!*MultiplayerActive || !is_human(faction_id))) {
                upgrade_prototypes(faction_id, uid);
            }
        }
    }
    // Modify diplomacy to skip social engineering choices dialog SOCIETY
    // while DiploWinState is active, otherwise use the previous conditions.
    if (faction_id == MapWin->cOwner && plr->diff_level > 1
    && !*MultiplayerActive && !*DiploWinState) {
        for (int sf = 0; sf < 4; ++sf) {
            for (int sm = 0; sm < 4; ++sm) {
                if (tech_id == SocialField[sf].soc_preq_tech[sm] && society_avail(sf, sm, faction_id)) {
                    int choice;
                    do {
                        parse_says(0, Tech[tech_id].name, -1, -1);
                        parse_says(1, SocialField[sf].soc_name[sm], -1, -1);
                        choice = X_pop_2(TutorFile, "SOCIETY", 0);
                        if (choice) {
                            social_select(faction_id);
                        }
                    } while (choice == 2);
                }
            }
        }
    }
    if ((Tech[tech_id].flags & TFLAG_REVEALS_MAP)
    && !(plr->player_flags & PFLAG_MAP_REVEALED)) {
        for (int i = 0; i < *MapAreaTiles; ++i) {
            (*MapTiles)[i].visibility |= bm_fac;
            if (faction_id) {
                (*MapTiles)[i].visible_items[faction_id - 1] = (*MapTiles)[i].items;
            }
        }
        for (int i = 0; i < *BaseCount; ++i) {
            Bases[i].visibility |= bm_fac;
            Bases[i].factions_pop_size_intel[faction_id] = Bases[i].pop_size;
        }
        plr->player_flags |= PFLAG_MAP_REVEALED;
        if (*CurrentTurn && faction_id == MapWin->cOwner) {
            parse_says(0, Tech[tech_id].name, -1, -1);
            popp(ScriptFile, "SEEMAP2", 0, "native_sm.pcx", 0);
            draw_map(1);
            set_dirty();
            GraphicWin_redraw(WorldWin);
        }
    }
    auto tech_owner = [](int cur_id, int plr_id) {
        assert(plr_id >= 0 && plr_id < MaxPlayerNum);
        return cur_id >= 0 && cur_id <= TECH_TranT && TechOwners[cur_id] & (1 << plr_id);
    };
    tech_effects(faction_id);
    if (faction_id == MapWin->cOwner) {
        if (tech_id == Facility[FAC_BIOLOGY_LAB].preq_tech) {
            interlude(1, 0, 1, 0);
        }
        if (tech_id == TECH_CentMed) {
            interlude(2, 0, 1, 0);
        }
        if (project_base(FAC_VOICE_OF_PLANET) != SP_Unbuilt && faction_id > 0
        && Tech[TECH_HAL9000].preq_tech1 >= -1
        && (Tech[TECH_HAL9000].preq_tech2 >= -1 || Tech[TECH_HAL9000].preq_tech1 == -1)) {
            if (Tech[TECH_Matter].preq_tech1 >= -1
            && (Tech[TECH_Matter].preq_tech2 >= -1 || Tech[TECH_Matter].preq_tech1 == -1)) {
                if (tech_owner(TECH_HAL9000, faction_id)
                && tech_owner(TECH_Matter, faction_id)) {
                    interlude(15, 0, 1, 0);
                    interlude(16, 0, 0, 0);
                }
            }
        }
        if (tech_id == TECH_Psych && fac->is_alien()) {
            for (int i = 1; i < MaxPlayerNum; ++i) {
                if (faction_id != i && !MFactions[i].is_alien()
                && (plr->diplo_status[i] & DIPLO_COMMLINK)) {
                    interlude(36, 0, 1, 0);
                }
            }
        }
    }
    // Fix: remove redundant conditions that check for tech_id == -1 that will never occur
    // TECH_TranT seems to be excluded by the following code on purpose
    auto is_valid_tech = [](int cur_id) {
        return cur_id >= 0 && cur_id < TECH_TranT
            && Tech[cur_id].preq_tech1 >= -1
            && (Tech[cur_id].preq_tech2 >= -1 || Tech[cur_id].preq_tech1 == -1);
    };
    int links_base_id = project_base(FAC_PLANETARY_DATALINKS);
    int links_owner_id;
    if (faction_id > 0 && links_base_id >= 0
    && (links_owner_id = Bases[links_base_id].faction_id, links_owner_id > 0)) {
        if (is_valid_tech(tech_id) && tech_owner(tech_id, faction_id)) {
            if (!tech_owner(tech_id, links_owner_id)) {
                int num = 0;
                for (int fc_id = 1; fc_id < MaxPlayerNum; ++fc_id) {
                    if (tech_owner(tech_id, fc_id)) {
                        ++num;
                    }
                }
                if (num >= PlanetaryDatalinksCount) {
                    tech_achieved(links_owner_id, tech_id, -1, 0);
                }
            }
        }
    }
    if (faction_id > 0 && is_valid_tech(tech_id) && tech_owner(tech_id, faction_id)) {
        for (int fc_id = 1; fc_id < MaxPlayerNum; ++fc_id) {
            if (MFactions[fc_id].rule_sharetech > 0 && !tech_owner(tech_id, fc_id)) {
                int num = 0;
                for (int tgt_id = 1; tgt_id < MaxPlayerNum; ++tgt_id) {
                    if (tech_owner(tech_id, tgt_id)) {
                        /*
                        Fix issue where TECHSHARE faction ability always skips the checks
                        for infiltration conditions while smac_only mode is activated.
                        Spying by probe team, pact, governor or Empath Guild is required.
                        Inverted RFLAG_TECHSHARE condition preserved for compatibility
                        but not possible without additional modding.
                        */
                        if (!(MFactions[fc_id].rule_flags & RFLAG_TECHSHARE)
                        || Factions[fc_id].diplo_status[tgt_id] & DIPLO_HAVE_INFILTRATOR
                        || Factions[fc_id].diplo_status[tgt_id] & DIPLO_PACT
                        || has_project(FAC_EMPATH_GUILD, fc_id)
                        || (fc_id == *GovernorFaction && !MFactions[tgt_id].is_alien())) {
                            ++num;
                        }
                    }
                }
                if (num >= MFactions[fc_id].rule_sharetech) {
                    tech_achieved(fc_id, tech_id, -2, 1);
                }
            }
        }
    }
    if (*CurrentTurn && is_first && Tech[tech_id].flags & TFLAG_SECRETS) {
        plr->player_flags |= PFLAG_FIRST_SECRETS;
        parse_says(0, Tech[tech_id].name, -1, -1);
        parse_says(1, fac->adj_name_faction, -1, -1);
        snprintf(StrBuffer, StrBufLen, "THESECRET%d", faction_id != MapWin->cOwner);
        X_pop(StrBuffer, 0);
        log_say("Learning a SECRET", fac->name_leader, faction_id, tech_id, TechOwners[tech_id]);
    }
}

int __cdecl tech_alt_cost(int tech_id, int faction_id) {
    assert(valid_player(faction_id));
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
        * conf.tech_rate_modifier / 100.0
        * (*GameRules & RULES_TECH_STAGNATION ? conf.tech_stagnate_rate / 100.0 : 1.0)
        * (1.0 - 0.05*min(6, owners));

    debug("tech_cost %d %d base: %7.2f diff: %.2f cost: %7.2f "
    "level: %d our_techs: %d owners: %d tech: %2d %s\n",
    *CurrentTurn, faction_id, cost_base, cost_diff, cost,
    level, our_techs, owners, tech_id, tech_str(tech_id));

    return clamp((int)cost, 1, 99999999);
}

/*
Replace tech_val for some parts of the diplomacy dialog related to tech trading.
*/
int __cdecl tech_alt_val(int tech_id, int faction_id, int flag) {
    CTech& tech = Tech[tech_id];
    Faction& plr = Factions[faction_id];
    assert(valid_player(faction_id));
    if (faction_id < 0 || tech_id < 0 || tech_id >= MaxTechnologyNum) {
        return 0;
    }
    int def_value = plr.AI_fight + flag;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id && is_alive(i) && at_war(i, faction_id)) {
            def_value += (Factions[i].base_count > plr.base_count);
            def_value += (Factions[i].mil_strength_1 > plr.mil_strength_1);
            def_value += (Factions[i].best_weapon_value > plr.best_weapon_value);
        }
    }
    int level = clamp(tech_level(tech_id, 0), 1, 16);
    int owners = __builtin_popcount(TechOwners[tech_id]);
    int weights = (tech.flags & (TFLAG_IMPROVE_PROBE|TFLAG_INC_COMMERCE) ? 4 : 0)
        + 4*(tech_id == Weapon[WPN_TERRAFORMING_UNIT].preq_tech)
        + 4*(tech_id == Rules->tech_preq_allow_3_nutrients_sq)
        + 4*(tech_id == Rules->tech_preq_allow_3_minerals_sq)
        + 4*(tech_id == Rules->tech_preq_allow_3_energy_sq)
        + 4*(tech_id == Facility[FAC_SKY_HYDRO_LAB].preq_tech)
        + 4*(tech_id == Facility[FAC_ORBITAL_POWER_TRANS].preq_tech)
        + 4*(tech_id == Facility[FAC_NESSUS_MINING_STATION].preq_tech)
        + 4*(tech_id == Facility[FAC_ORBITAL_DEFENSE_POD].preq_tech)
        + 4*(tech_id == Facility[FAC_AEROSPACE_COMPLEX].preq_tech)
        + 4*(tech_id == Facility[FAC_HAB_COMPLEX].preq_tech
        && MFactions[faction_id].rule_population > 0)
        + 2*((plr.AI_growth + 1 + (plr.base_count < 8 + *MapAreaSqRoot/4)) * tech.AI_growth
        + (plr.AI_tech + 1 + (plr.SE_research > 0)) * tech.AI_tech
        + (plr.AI_wealth + 1 + (plr.SE_economy > 0)) * tech.AI_wealth
        + (plr.AI_power + 1 + (def_value > 2)) * tech.AI_power);

    for (int i = 0; i < MaxChassisNum; i++) {
        if (Chassis[i].preq_tech == tech_id) {
            weights += (Chassis[i].triad == TRIAD_AIR ? 8 : 4);
        }
    }
    for (int i = 0; i < MaxAbilityNum; i++) {
        if (Ability[i].preq_tech == tech_id && i != ABL_ID_SLOW && i != ABL_ID_CARRIER
        && i != ABL_ID_REPAIR && i != ABL_ID_DEEP_PRESSURE_HULL && i != ABL_ID_HEAVY_TRANSPORT) {
            weights += 4;
        }
    }
    for (int i = 0; i < MaxTerrainNum; i++) {
        if (Terraform[i].preq_tech == tech_id) {
            weights += 2;
        }
    }
    for (int i = 0; i < MaxWeaponNum; i++) {
        if (Weapon[i].preq_tech == tech_id
        && (!Weapon[i].offense_value || Weapon[i].offense_value >= plr.best_weapon_value)) {
            weights += 8;
        }
    }
    for (int i = 0; i < MaxArmorNum; i++) {
        if (Armor[i].preq_tech == tech_id && Armor[i].defense_value >= plr.best_armor_value) {
            weights += 4;
        }
    }
    int limit = max(100, *CurrentTurn * *BaseCount / 10);
    int value = (50*level + 8*level*level + clamp(*BaseCount / 4, 0, 80))
        * clamp(weights + 32, 32, 96) / 32
        * clamp(*MapAreaSqRoot, 30, 80) / 64
        * clamp(100 + (conf.tech_rate_modifier - 100)/2, 50, 200) / 100;
    value = (value - max(0, value - limit)/2);

    if (*GameRules & RULES_TECH_STAGNATION) {
        value = value * clamp(100 + (conf.tech_stagnate_rate - 100)/2, 50, 200) / 100;
    }
    if (owners > 1) {
        value = value * max(25, 100 - 15*owners) / 100;
    }
    debugw("tech_alt_val %d level: %d owners: %d weights: %d value: %d tech: %d %s\n",
    faction_id, level, owners, weights, value, tech_id, tech_str(tech_id));
    return value;
}


