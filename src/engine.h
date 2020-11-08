#pragma once

#include "main.h"

#define THINKER_HEADER (short)0xACAC

void init_save_game(int faction);
HOOK_API int mod_faction_upkeep(int faction);
HOOK_API int mod_base_find3(int x, int y, int faction1, int region, int faction2, int faction3);

typedef int __cdecl fp_void();
typedef int __cdecl fp_1int(int);
typedef int __cdecl fp_2int(int, int);
typedef int __cdecl fp_3int(int, int, int);
typedef int __cdecl fp_4int(int, int, int, int);
typedef int __cdecl fp_5int(int, int, int, int, int);
typedef int __cdecl fp_6int(int, int, int, int, int, int);
typedef int __cdecl fp_7int(int, int, int, int, int, int, int);
typedef int __cdecl fp_7intstr(int, int, int, int, int, int, int, const char*);

/* Temporarily disable warnings for thiscall parameter type. */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif

typedef int __cdecl fp_ccici(const char*, const char*, int, const char*, int);
typedef int __cdecl fp_icii(int, const char*, int, int);
typedef int __thiscall tc_2int(int, int);
typedef int __thiscall tc_3int(int, int, int);
typedef int __thiscall tc_4int(int, int, int, int);
typedef int __thiscall tc_5int(int, int, int, int, int);
typedef int __thiscall tc_6int(int, int, int, int, int, int);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

extern BASE** current_base_ptr;
extern int* current_base_id;
extern int* game_settings;
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
extern fp_3int* setup_player;
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

