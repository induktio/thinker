#pragma once

#include "main.h"

#define THINKER_HEADER (short)0xACAC

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

HOOK_API int faction_upkeep(int faction);
HOOK_API int mod_base_find3(int x, int y, int faction1, int region, int faction2, int faction3);

