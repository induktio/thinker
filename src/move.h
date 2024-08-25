#pragma once

#include "main.h"

enum UpdateMode {UM_Full, UM_Visual, UM_Player};
enum StackType {ST_NeutralOnly, ST_NonPactOnly, ST_EnemyOnly, ST_EnemyOneUnit};
enum FormerMode {FM_Auto_Full, FM_Auto_Roads, FM_Auto_Tubes, FM_Auto_Sensors, FM_Remove_Fungus};

enum EnemyVehMove { // Return codes for enemy_veh processing
    VEH_SYNC = 0,
    VEH_SKIP = 1,
};

const int PM_SAFE = -20;
const int PM_NEAR_SAFE = -40;
const uint32_t BIT_SIMPLE = (BIT_FARM | BIT_MINE | BIT_SOLAR | BIT_FOREST);
const uint32_t BIT_ADVANCED = (BIT_CONDENSER | BIT_THERMAL_BORE);
const uint32_t BIT_BASE_DISALLOWED = (BIT_BASE_IN_TILE | BIT_MONOLITH | BIT_FUNGUS | BIT_THERMAL_BORE);

extern PMTable mapdata;
extern NodeSet mapnodes;
extern int move_upkeep_faction;

int arty_value(int x, int y);
int base_tile_score(int x, int y, int faction, MAP* sq);
int former_tile_score(int x, int y, int faction, MAP* sq);
int select_item(int x, int y, int faction, FormerMode mode, MAP* sq);
bool non_ally_in_tile(int x, int y, int faction);
bool allow_move(int x, int y, int faction, int triad);
bool allow_civ_move(int x, int y, int faction, int triad);
bool can_build_base(int x, int y, int faction, int triad);
bool has_base_sites(int x, int y, int faction, int triad);
bool allow_scout(int faction, MAP* sq);
bool allow_probe(int faction1, int faction2, bool is_enhanced);
bool allow_attack(int faction1, int faction2, bool is_probe, bool is_enhanced);
bool allow_conv_missile(int veh_id, int enemy_veh_id, MAP* sq);
bool can_airdrop(int veh_id, MAP* sq);
bool allow_airdrop(int x, int y, int faction, bool combat, MAP* sq);
bool invasion_unit(int veh_id);
bool near_landing(int veh_id);
int make_landing(int veh_id);
int garrison_goal(int x, int y, int faction, int triad);
int garrison_count(int x, int y);
int defender_count(int x, int y, int veh_skip_id);

int __cdecl mod_enemy_move(int veh_id);
int __cdecl veh_kill_lift(int veh_id);
void update_main_region(int faction);
void move_upkeep(int faction, UpdateMode mode);
int crawler_move(const int id);
int colony_move(const int id);
int former_move(const int id);
int artifact_move(const int id);
int trans_move(const int id);
int aircraft_move(const int id);
int combat_move(const int id);

