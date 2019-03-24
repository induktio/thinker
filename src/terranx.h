
#ifndef __TERRANX_H__
#define __TERRANX_H__

typedef unsigned char byte;

#include "terranx_enums.h"
#include "terranx_types.h"

extern const char** tx_version;
extern const char** tx_date;

extern int*  tx_current_base_id;
extern BASE* tx_current_base_ptr;
extern int* tx_game_rules;
extern int* tx_scen_rules;
extern int* tx_diff_level;
extern int* tx_human_players;
extern int* tx_current_turn;
extern int* tx_active_faction;
extern int* tx_total_num_bases;
extern int* tx_total_num_vehicles;
extern int* tx_random_seed;
extern int* tx_map_toggle_flat;
extern int* tx_map_area_sq_root;
extern int* tx_map_axis_x;
extern int* tx_map_axis_y;
extern int* tx_map_half_x;
extern int* tx_climate_future_change;
extern int* tx_un_charter_repeals;
extern int* tx_un_charter_reinstates;
extern int* tx_cost_ratios;

extern byte* tx_tech_discovered;
extern int* tx_secret_projects;
extern FactMeta* tx_factions_meta;
extern Faction* tx_factions;
extern BASE* tx_bases;
extern UNIT* tx_units;
extern VEH* tx_vehicles;
extern MAP** tx_map_ptr;

extern R_Basic* tx_basic;
extern R_Tech* tx_techs;
extern R_Social* tx_social;
extern R_Facility* tx_facility;
extern R_Ability* tx_ability;
extern R_Chassis* tx_chassis;
extern R_Citizen* tx_citizen;
extern R_Defense* tx_defense;
extern R_Reactor* tx_reactor;
extern R_Resource* tx_resource;
extern R_Terraform* tx_terraform;
extern R_Weapon* tx_weapon;

typedef int __cdecl fp_void();
typedef int __cdecl fp_1int(int);
typedef int __cdecl fp_2int(int, int);
typedef int __cdecl fp_3int(int, int, int);
typedef int __cdecl fp_4int(int, int, int, int);
typedef int __cdecl fp_5int(int, int, int, int, int);
typedef int __cdecl fp_6int(int, int, int, int, int, int);
typedef int __cdecl fp_7intstr(int, int, int, int, int, int, int, const char*);

// params: faction, chassis, module, armor, specials, reactor, unit_plan, name
extern fp_7intstr* tx_propose_proto;

// params: prototype_id, faction, x, y
extern fp_4int* tx_veh_init;

extern fp_1int* tx_veh_skip;
extern fp_2int* tx_veh_at;
extern fp_2int* tx_veh_speed;
extern fp_3int* tx_zoc_any;
extern fp_3int* tx_best_defender;
extern fp_5int* tx_battle_compute;
extern fp_1int* tx_enemy_move;
extern fp_1int* tx_monolith;
extern fp_2int* tx_action_build;
extern fp_3int* tx_action_terraform;
extern fp_3int* tx_terraform_cost;
extern fp_2int* tx_bonus_at;
extern fp_2int* tx_goody_at;
extern fp_3int* tx_cost_factor;
extern fp_3int* tx_site_set;
extern fp_3int* tx_world_site;
extern fp_1int* tx_set_base;
extern fp_1int* tx_base_compute;
extern fp_4int* tx_base_prod_choices;
extern fp_void* tx_turn_upkeep;
extern fp_1int* tx_faction_upkeep;
extern fp_3int* tx_tech_val;
extern fp_5int* tx_crop_yield;
extern fp_6int* tx_social_ai;
extern fp_1int* tx_social_set;
extern fp_1int* tx_pop_goal;
extern fp_1int* tx_consider_designs;
extern fp_1int* tx_action_staple;


#endif // __TERRANX_H__

