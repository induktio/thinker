#pragma once

#include "main.h"

typedef int PMTable[MaxMapW][MaxMapH];

constexpr int BASE_DISALLOWED = (TERRA_BASE_IN_TILE | TERRA_MONOLITH | TERRA_FUNGUS | TERRA_THERMAL_BORE);

extern PMTable pm_overlay;

HOOK_API int mod_enemy_move(int id);
HOOK_API int log_veh_kill(int a, int b, int c, int d);
void move_upkeep(int faction, bool visual);
void land_raise_plan(int faction, bool visual);
bool need_formers(int x, int y, int faction);
bool has_transport(int x, int y, int faction);
bool non_combat_move(int x, int y, int faction, int triad);
bool can_build_base(int x, int y, int faction, int triad);
bool has_base_sites(int x, int y, int faction, int triad);
int select_item(int x, int y, int faction, MAP* sq);
int crawler_move(int id);
int colony_move(int id);
int former_move(int id);
int trans_move(int id);
int combat_move(int id);

