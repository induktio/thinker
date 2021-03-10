#pragma once

#include "main.h"

typedef int PMTable[MaxMapW][MaxMapH];

const int BASE_DISALLOWED = (TERRA_BASE_IN_TILE | TERRA_MONOLITH | TERRA_FUNGUS | TERRA_THERMAL_BORE);

extern PMTable pm_overlay;

HOOK_API int mod_enemy_move(const int id);
HOOK_API int log_veh_kill(int a, int b, int c, int d);
void update_main_region(int faction);
void move_upkeep(int faction, bool visual);
void land_raise_plan(int faction, bool visual);
bool ocean_shoreline(int x, int y);
bool need_formers(int x, int y, int faction);
bool has_transport(int x, int y, int faction);
bool allow_move(int x, int y, int faction, int triad);
bool non_combat_move(int x, int y, int faction, int triad);
bool can_build_base(int x, int y, int faction, int triad);
bool has_base_sites(int x, int y, int faction, int triad);
int select_item(int x, int y, int faction, MAP* sq);
int crawler_move(const int id);
int colony_move(const int id);
int former_move(const int id);
int trans_move(const int id);
int aircraft_move(const int id);
int combat_move(const int id);

