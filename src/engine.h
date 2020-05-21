#pragma once

#include "main.h"

typedef int __cdecl fp_ccici(const char*, const char*, int, const char*, int);
typedef int __cdecl fp_icii(int, const char*, int, int);


extern int*  current_base_id;
extern BASE* current_base_ptr;
extern int* game_settings;
extern int* game_state;
extern int* game_rules;
extern int* diff_level;
extern int* human_players;
extern int* current_turn;
extern int* active_faction;
extern int* total_num_bases;
extern int* total_num_vehicles;
extern int* map_random_seed;
extern int* map_toggle_flat;
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

HOOK_API int faction_upkeep(int faction);

