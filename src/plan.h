#pragma once

#include "main.h"

extern int base_enemy_range[MaxBaseNum];
extern int base_border_range[MaxBaseNum];
extern int plan_upkeep_turn;

void reset_state();
void design_units(int faction_id);
void former_plans(int faction_id);
void plans_upkeep(int faction_id);
bool need_police(int faction_id);
int psi_score(int faction_id);
int satellite_goal(int faction_id, int item_id);
int satellite_count(int faction_id, int item_id);
