
#include "goal.h"


bool ignore_goal(int type) {
    return type == AI_GOAL_COLONIZE || type == AI_GOAL_TERRAFORM_LAND
        || type == AI_GOAL_TERRAFORM_WATER || type == AI_GOAL_CONDENSER
        || type == AI_GOAL_THERMAL_BOREHOLE || type == AI_GOAL_ECHELON_MIRROR
        || type == AI_GOAL_SENSOR_ARRAY || type == AI_GOAL_DEFEND;
}

void __cdecl add_goal(int faction, int type, int priority, int x, int y, int base_id) {
    if (thinker_enabled(faction) && type < Thinker_Goal_ID_First) {
        return;
    }
    if (conf.debug_verbose) {
        debug("add_goal %d type: %3d pr: %2d x: %3d y: %3d base: %d\n",
            faction, type, priority, x, y, base_id);
    }
    if (!mapsq(x, y)) {
        debug("AddGoalError %d %d %d\n", x, y, type);
        return;
    }
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.x == x && goal.y == y && goal.type == type) {
            goal.priority = max(goal.priority, (int16_t)priority);
            return;
        }
    }
    int best_value = 0, goal_id = -1;
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.type < 0 || goal.priority < priority) {
            int value = goal.type >= 0 ? 0 : 1000;
            if (!value) {
                value = goal.priority > 0 ? (20 - goal.priority) : goal.priority + 100;
            }
            if (value > best_value) {
                best_value = value;
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

void __cdecl add_site(int faction, int type, int priority, int x, int y) {
    if (thinker_enabled(faction)) {
        return;
    }
    if (conf.debug_verbose) {
        debug("add_site %d type: %3d pr: %2d x: %3d y: %3d\n",
            faction, type, priority, x, y);
    }
    if (!mapsq(x, y)) {
        debug("AddSiteError %d %d %d\n", x, y, type);
        return;
    }
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& sites = Factions[faction].sites[i];
        if (sites.x == x && sites.y == y && sites.type == type) {
            sites.priority = max(sites.priority, (int16_t)priority);
            return;
        }
    }
    int best_value = 0;
    int site_id = -1;
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& sites = Factions[faction].sites[i];
        if (sites.type < 0 || sites.priority < priority) {
            int value = sites.type >= 0 ? (20 - sites.priority) : 1000;
            if (value > best_value) {
                best_value = value;
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
        if (site.type >= 0) {
            add_goal(faction, site.type, site.priority, site.x, site.y, -1);
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



