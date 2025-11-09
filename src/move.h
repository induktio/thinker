#pragma once

#include "main.h"

enum UpdateMode {UM_Full, UM_Visual, UM_Player};
enum StackType {ST_NeutralOnly, ST_NonPactOnly, ST_EnemyOnly, ST_EnemyOneUnit};
enum FormerMode {FM_Auto_Full, FM_Auto_Roads, FM_Auto_Tubes, FM_Auto_Sensors,
    FM_Remove_Fungus, FM_Farm_Road, FM_Mine_Road};

enum EnemyVehMove { // Return codes for enemy_veh processing
    VEH_SYNC = 0,
    VEH_SKIP = 1,
};

const int PM_SAFE = -20;
const int PM_NEAR_SAFE = -40;
const uint32_t BIT_SIMPLE = (BIT_FARM | BIT_MINE | BIT_SOLAR | BIT_FOREST);
const uint32_t BIT_ADVANCED = (BIT_CONDENSER | BIT_THERMAL_BORE);

extern PMTable mapdata;
extern NodeSet mapnodes;

int arty_value(int x, int y);
int base_tile_score(int x, int y, int faction_id, MAP* sq);
int former_tile_score(int x, int y, int faction_id, MAP* sq);
int select_item(int x, int y, int faction_id, FormerMode mode, MAP* sq);
bool ally_near_tile(int x, int y, int faction_id, int skip_veh_id, int max_range);
bool non_ally_in_tile(int x, int y, int faction_id);
bool allow_move(int x, int y, int faction_id, int triad);
bool allow_civ_move(int x, int y, int faction_id, int triad);
bool can_build_base(int x, int y, int faction_id, int triad);
bool stack_search(int x, int y, int faction_id, StackType type, VehWeaponMode mode);
int coast_tiles(int x, int y);
int ocean_coast_tiles(int x, int y);
bool near_ocean_coast(int x, int y);
bool near_sea_coast(int x, int y);
bool allow_scout(int faction_id, MAP* sq);
bool allow_probe(int faction1, int faction2, bool is_enhanced);
bool allow_attack(int faction1, int faction2, bool is_probe, bool is_enhanced);
bool allow_combat(int x, int y, int faction_id, MAP* sq);
bool allow_conv_missile(int veh_id, int enemy_veh_id, MAP* sq);
bool can_airdrop(int veh_id, MAP* sq);
bool allow_airdrop(int x, int y, int faction_id, bool combat, MAP* sq);
bool invasion_unit(int veh_id);
bool near_landing(int veh_id);
int make_landing(int veh_id);

int __cdecl mod_enemy_move(int veh_id);
int __cdecl veh_kill_lift(int veh_id);
void update_main_region(int faction_id);
void move_upkeep(int faction_id, UpdateMode mode);
int crawler_move(const int id);
int colony_move(const int id);
int former_move(const int id);
int artifact_move(const int id);
int trans_move(const int id);
int nuclear_move(const int id);
int airdrop_move(const int id, MAP* sq);
int combat_move(const int id);

