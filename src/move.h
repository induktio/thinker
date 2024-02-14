#pragma once

#include "main.h"

typedef int16_t PMTable[MaxMapW][MaxMapH];

enum UpdateMode {M_Full, M_Visual, M_Player};
enum StackType {S_NeutralOnly, S_NonPactOnly, S_EnemyOnly, S_EnemyOneUnit};

enum EnemyVehMove { // Return codes for enemy_veh processing
    VEH_SYNC = 0,
    VEH_SKIP = 1,
};

enum RegionFlag {
    PM_ENEMY = 1,
    PM_PROBE = 2,
};

enum FormerAuto {
    FM_AUTO_FULL = 0,
    FM_AUTO_ROADS = 1,
    FM_AUTO_TUBES = 2,
};

const uint32_t BIT_SIMPLE = (BIT_FARM | BIT_MINE | BIT_SOLAR | BIT_FOREST);
const uint32_t BIT_ADVANCED = (BIT_CONDENSER | BIT_THERMAL_BORE);
const uint32_t BIT_BASE_DISALLOWED = (BIT_BASE_IN_TILE | BIT_MONOLITH | BIT_FUNGUS | BIT_THERMAL_BORE);

extern PMTable pm_target;
extern PMTable pm_overlay;
extern int base_enemy_range[MaxBaseNum];

int arty_value(int x, int y);
int base_tile_score(int x, int y, int range, int triad);
int former_tile_score(int x, int y, int faction, MAP* sq);
int select_item(int x, int y, int faction, FormerAuto mode, MAP* sq);
bool allow_probe(int faction1, int faction2, bool is_enhanced_probe);
bool invasion_unit(const int id);

int __cdecl mod_enemy_move(const int id);
int __cdecl log_veh_kill(int a, int b, int c, int d);
int ocean_coast_tiles(int x, int y);
void update_main_region(int faction);
void move_upkeep(int faction, UpdateMode mode);
bool allow_move(int x, int y, int faction, int triad);
bool allow_civ_move(int x, int y, int faction, int triad);
bool can_build_base(int x, int y, int faction, int triad);
bool has_base_sites(int x, int y, int faction, int triad);
int crawler_move(const int id);
int colony_move(const int id);
int former_move(const int id);
int trans_move(const int id);
int aircraft_move(const int id);
int combat_move(const int id);

