#pragma once

typedef unsigned char byte;

#include "terranx_enums.h"
#include "terranx_types.h"

typedef int __cdecl fp_void();
typedef int __cdecl fp_1int(int);
typedef int __cdecl fp_2int(int, int);
typedef int __cdecl fp_3int(int, int, int);
typedef int __cdecl fp_4int(int, int, int, int);
typedef int __cdecl fp_5int(int, int, int, int, int);
typedef int __cdecl fp_6int(int, int, int, int, int, int);
typedef int __cdecl fp_7int(int, int, int, int, int, int, int);
typedef int __cdecl fp_7intstr(int, int, int, int, int, int, int, const char*);
typedef int __cdecl fp_ccici(const char*, const char*, int, const char*, int);
typedef int __cdecl fp_icii(int, const char*, int, int);

/* Temporarily disable warnings for thiscall parameter type. */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif

typedef int __thiscall tc_1int(int);
typedef int __thiscall tc_2int(int, int);
typedef int __thiscall tc_3int(int, int, int);
typedef int __thiscall tc_4int(int, int, int, int);
typedef int __thiscall tc_5int(int, int, int, int, int);
typedef int __thiscall tc_6int(int, int, int, int, int, int);
typedef int __thiscall tc_7int(int, int, int, int, int, int, int);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

/*
Thinker functions that are replacements to the SMACX binary versions 
should be prefixed with 'mod_' if their equivalent is also listed here.
*/

extern const char** engine_version;
extern const char** engine_date;
extern BASE** current_base_ptr;
extern int* current_base_id;
extern int* game_preferences;
extern int* game_more_preferences;
extern int* game_state;
extern int* game_rules;
extern int* diff_level;
extern int* smacx_enabled;
extern int* human_players;
extern int* current_turn;
extern int* active_faction;
extern int* total_num_bases;
extern int* total_num_vehicles;
extern int* map_random_seed;
extern int* map_toggle_flat;
extern int* map_area_tiles;
extern int* map_area_sq_root;
extern int* map_axis_x;
extern int* map_axis_y;
extern int* map_half_x;
extern int* climate_future_change;
extern int* un_charter_repeals;
extern int* un_charter_reinstates;
extern int* gender_default;
extern int* plurality_default;
extern int* current_player_faction;
extern int* multiplayer_active;
extern int* pbem_active;
extern int* sunspot_duration;
extern int* diplo_active_faction;
extern int* diplo_current_friction;
extern int* diplo_opponent_faction;
extern int* base_find_dist;
extern int* veh_attack_flags;
extern int* game_not_started;

extern int* dword_93A934;
extern int* dword_945B18;
extern int* dword_945B1C;
extern int* screen_width;
extern int* screen_height;

extern byte* tx_tech_discovered;
extern int* tx_secret_projects;
extern int* tx_cost_ratios;
extern short (*tx_faction_rankings)[8];
extern MetaFaction* tx_metafactions;
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

extern fp_7intstr* propose_proto;
extern fp_4int* veh_init;
extern fp_1int* veh_skip;
extern fp_2int* veh_at;
extern fp_2int* veh_speed;
extern fp_3int* zoc_any;
extern fp_1int* monolith;
extern fp_2int* action_build;
extern fp_3int* action_terraform;
extern fp_3int* terraform_cost;
extern fp_2int* bonus_at;
extern fp_2int* goody_at;
extern fp_3int* cost_factor;
extern fp_3int* site_set;
extern fp_3int* world_site;
extern fp_1int* set_base;
extern fp_1int* base_compute;
extern fp_4int* base_prod_choices;
extern fp_void* turn_upkeep;
extern fp_1int* faction_upkeep;
extern fp_1int* action_staple;
extern fp_1int* social_upkeep;
extern fp_1int* repair_phase;
extern fp_1int* production_phase;
extern fp_1int* allocate_energy;
extern fp_1int* enemy_diplomacy;
extern fp_1int* enemy_strategy;
extern fp_1int* corner_market;
extern fp_1int* call_council;
extern fp_3int* setup_player;
extern fp_2int* eliminate_player;
extern fp_2int* can_call_council;
extern fp_void* do_all_non_input;
extern fp_void* auto_save;
extern fp_2int* parse_num;
extern fp_icii* parse_says;
extern fp_ccici* popp;
extern fp_3int* capture_base;
extern fp_1int* base_kill;
extern fp_5int* crop_yield;
extern fp_6int* base_draw;
extern fp_6int* base_find3;
extern fp_3int* draw_tile;
extern tc_2int* font_width;
extern tc_4int* buffer_box;
extern tc_3int* buffer_fill3;
extern tc_5int* buffer_write_l;
extern fp_6int* social_ai;
extern fp_1int* social_set;
extern fp_1int* pop_goal;
extern fp_1int* consider_designs;
extern fp_3int* tech_val;
extern fp_1int* tech_rate;
extern fp_1int* tech_selection;
extern fp_1int* enemy_move;
extern fp_3int* best_defender;
extern fp_5int* battle_compute;
extern fp_6int* battle_kill;
extern fp_7int* battle_fight_2;
extern fp_void* draw_cursor;

