
#include "game.h"


bool un_charter() {
    return *un_charter_repeals <= *un_charter_reinstates;
}

bool victory_done() {
    // TODO: Check for scenario victory conditions
    return *game_state & (STATE_VICTORY_CONQUER | STATE_VICTORY_DIPLOMATIC | STATE_VICTORY_ECONOMIC)
        || has_project(-1, FAC_ASCENT_TO_TRANSCENDENCE);
}

bool valid_player(int faction) {
    return faction > 0 && faction < MaxPlayerNum;
}

bool valid_triad(int triad) {
    return (triad == TRIAD_LAND || triad == TRIAD_SEA || triad == TRIAD_AIR);
}

/*
Original Offset: 005C89A0
*/
int __cdecl game_year(int n) {
    return Rules->normal_start_year + n;
}

/*
Purpose: Calculate the offset and bitmask for the specified input.
Original Offset: 0050BA00
Return Value: n/a
*/
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask) {
    *offset = input / 8;
    *mask = 1 << (input & 7);
}

int __cdecl neural_amplifier_bonus(int value) {
    return value * conf.neural_amplifier_bonus / 100;
}

int __cdecl dream_twister_bonus(int value) {
    return value * conf.dream_twister_bonus / 100;
}

int __cdecl fungal_tower_bonus(int value) {
    return value * conf.fungal_tower_bonus / 100;
}

/*
Purpose: Calculate the psi combat factor for an attacking or defending unit.
Original Offset: 00501500
*/
int __cdecl psi_factor(int value, int faction_id, bool is_attack, bool is_fungal_twr) {
    int rule_psi = MFactions[faction_id].rule_psi;
    if (rule_psi) {
        value = ((rule_psi + 100) * value) / 100;
    }
    if (is_attack) {
        if (has_project(faction_id, FAC_DREAM_TWISTER)) {
            value += value * conf.dream_twister_bonus / 100;
        }
    } else {
        if (has_project(faction_id, FAC_NEURAL_AMPLIFIER)) {
            value += value * conf.neural_amplifier_bonus / 100;
        }
        if (is_fungal_twr) {
            value += value * conf.fungal_tower_bonus / 100;
        }
    }
    return value;
}

/*
Purpose: Calculate facility maintenance cost taking into account faction bonus facilities.
Vanilla game calculates Command Center maintenance based on the current highest reactor level
which is unnecessary to implement here.
Original Offset: 004F6510
*/
int __cdecl fac_maint(int facility_id, int faction_id) {
    CFacility& facility = Facility[facility_id];
    MFaction& meta = MFactions[faction_id];

    for (int i=0; i < meta.faction_bonus_count; i++) {
        if (meta.faction_bonus_val1[i] == facility_id
        && (meta.faction_bonus_id[i] == FCB_FREEFAC
        || (meta.faction_bonus_id[i] == FCB_FREEFAC_PREQ
        && has_tech(faction_id, facility.preq_tech)))) {
            return 0;
        }
    }
    return facility.maint;
}

/*
Purpose: Calculate nutrient/mineral cost factors for base production.
In vanilla game mechanics, if the player faction is ranked first, then the AIs will get
additional growth/industry bonuses. This modified version removes them.
Original Offset: 004E4430
*/
int __cdecl mod_cost_factor(int faction_id, int is_mineral, int base_id) {
    int value;
    int multiplier = (is_mineral ? Rules->mineral_cost_multi : Rules->nutrient_cost_multi);

    if (is_human(faction_id)) {
        value = multiplier;
    } else {
        value = multiplier * CostRatios[*diff_level] / 10;
    }

    if (*MapSizePlanet == 0) {
        value = 8 * value / 10;
    } else if (*MapSizePlanet == 1) {
        value = 9 * value / 10;
    }
    if (is_mineral) {
        if (is_mineral == 1) {
            switch (Factions[faction_id].SE_industry_pending) {
                case -7:
                case -6:
                case -5:
                case -4:
                case -3:
                    return (13 * value + 9) / 10;
                case -2:
                    return (6 * value + 4) / 5;
                case -1:
                    return (11 * value + 9) / 10;
                case 0:
                    break;
                case 1:
                    return (9 * value + 9) / 10;
                case 2:
                    return (4 * value + 4) / 5;
                case 3:
                    return (7 * value + 9) / 10;
                case 4:
                    return (3 * value + 4) / 5;
                default:
                    return (value + 1) / 2;
            }
        }
        return value;
    } else {
        int growth = Factions[faction_id].SE_growth_pending;
        if (base_id >= 0) {
            if (has_facility(base_id, FAC_CHILDREN_CRECHE)) {
                growth += 2;
            }
            if (Bases[base_id].state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
                growth += 2;
            }
        }
        return (value * (10 - clamp(growth, -2, 5)) + 9) / 10;
    }
}

/*
Calculate unit upgrade cost in mineral rows. This version modifies crawler upgrade costs.
*/
int __cdecl mod_upgrade_cost(int faction, int new_unit_id, int old_unit_id) {
    UNIT* old_unit = &Units[old_unit_id];
    UNIT* new_unit = &Units[new_unit_id];
    int modifier = new_unit->cost;

    if (old_unit->is_supply()) {
        return 4*max(1, new_unit->cost - old_unit->cost);
    }
    if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
        modifier = ((new_unit->cost + 1) / 2 + new_unit->cost)
            * ((new_unit->cost + 1) / 2 + new_unit->cost + 1);
    }
    int cost = max(0, new_unit->speed() - old_unit->speed())
        + max(0, new_unit->armor_cost() - old_unit->armor_cost())
        + max(0, new_unit->weapon_cost() - old_unit->weapon_cost())
        + modifier;
    if (has_project(faction, FAC_NANO_FACTORY)) {
        cost /= 2;
    }
    return cost;
}

bool ignore_goal(int type) {
    return type == AI_GOAL_COLONIZE || type == AI_GOAL_TERRAFORM_LAND
        || type == AI_GOAL_TERRAFORM_WATER || type == AI_GOAL_CONDENSER
        || type == AI_GOAL_THERMAL_BOREHOLE || type == AI_GOAL_ECHELON_MIRROR
        || type == AI_GOAL_SENSOR_ARRAY || type == AI_GOAL_DEFEND;
}

/*
Original Offset: 00579A30
*/
void __cdecl add_goal(int faction, int type, int priority, int x, int y, int base_id) {
    if (!mapsq(x, y)) {
        return;
    }
    if (thinker_enabled(faction) && type < Thinker_Goal_ID_First) {
        return;
    }
    if (conf.debug_verbose) {
        debug("add_goal %d type: %3d pr: %2d x: %3d y: %3d base: %d\n",
            faction, type, priority, x, y, base_id);
    }
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.x == x && goal.y == y && goal.type == type) {
            if (goal.priority <= priority) {
                goal.priority = (int16_t)priority;
            }
            return;
        }
    }
    int goal_score = 0, goal_id = -1;
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];

        if (goal.type < 0 || goal.priority < priority) {
            int cmp = goal.type >= 0 ? 0 : 1000;
            if (!cmp) {
                cmp = goal.priority > 0 ? 20 - goal.priority : goal.priority + 100;
            }
            if (cmp > goal_score) {
                goal_score = cmp;
                goal_id = i;
            }
        }
    }
    if (goal_id >= 0) {
        Goal& goal = Factions[faction].goals[goal_id];
        goal.type = (int16_t)type;
        goal.priority = (int16_t)priority;
        goal.x = x;
        goal.y = y;
        goal.base_id = base_id;
    }
}

/*
Original Offset: 00579B70
*/
void __cdecl add_site(int faction, int type, int priority, int x, int y) {
    if ((x ^ y) & 1 && *game_state & STATE_DEBUG_MODE) {
        debug("Bad SITE %d %d %d\n", x, y, type);
    }
    if (thinker_enabled(faction)) {
        return;
    }
    if (conf.debug_verbose) {
        debug("add_site %d type: %3d pr: %2d x: %3d y: %3d\n",
            faction, type, priority, x, y);
    }
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& sites = Factions[faction].sites[i];
        if (sites.x == x && sites.y == y && sites.type == type) {
            if (sites.priority <= priority) {
                sites.priority = (int16_t)priority;
            }
            return;
        }
    }
    int priority_search = 0;
    int site_id = -1;
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& sites = Factions[faction].sites[i];
        int type_cmp = sites.type;
        int priority_cmp = sites.priority;
        if (type_cmp < 0 || priority_cmp < priority) {
            int cmp = type_cmp >= 0 ? 0 : 1000;
            if (!cmp) {
                cmp = 20 - priority_cmp;
            }
            if (cmp > priority_search) {
                priority_search = cmp;
                site_id = i;
            }
        }
    }
    if (site_id >= 0) {
        Goal &sites = Factions[faction].sites[site_id];
        sites.type = (int16_t)type;
        sites.priority = (int16_t)priority;
        sites.x = x;
        sites.y = y;
        add_goal(faction, type, priority, x, y, -1);
    }
}

/*
Original Offset: 0x579D80
*/
void __cdecl wipe_goals(int faction) {
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        // Mod: instead of always deleting decrement priority each turn
        if (goal.priority > 0) {
            goal.priority--;
        }
        if (goal.priority <= 0) {
            goal.type = AI_GOAL_UNUSED;
        }
    }
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& site = Factions[faction].sites[i];
        int16_t type = site.type;
        if (type >= 0) {
            add_goal(faction, type, site.priority, site.x, site.y, -1);
        }
    }
}

int has_goal(int faction, int type, int x, int y) {
    assert(valid_player(faction) && mapsq(x, y));
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.priority > 0 && goal.x == x && goal.y == y && goal.type == type) {
            return goal.priority;
        }
    }
    return 0;
}

Goal* find_priority_goal(int faction, int type, int* px, int* py) {
    int pp = 0;
    *px = -1;
    *py = -1;
    Goal* value = NULL;
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.type == type && goal.priority > pp && mapsq(goal.x, goal.y)) {
            value = &goal;
            pp = goal.priority;
            *px = goal.x;
            *py = goal.y;
        }
    }
    return value;
}





