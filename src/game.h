#pragma once

#include "main.h"

// Always enabled settings for random maps started from the main menu
// Select only those settings that are not set in Special Scenario Rules
const uint32_t GAME_RULES_MASK = 0x7808FFFF;
const uint32_t GAME_MRULES_MASK = 0xFFFFFFF0;

bool un_charter();
bool global_trade_pact();
bool victory_done();
bool voice_of_planet();
bool valid_player(int faction);
bool valid_triad(int triad);
int __cdecl game_start_turn();
int __cdecl game_year(int n);
int __cdecl in_box(int x, int y, RECT* rc);
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask);
int __cdecl mod_cost_factor(int faction_id, BaseResType type, int base_id);
int __cdecl mod_black_market(int base_id, int energy, int* effic_energy_lost);

void show_rules_menu();
void init_world_config();
void init_save_game(int faction);
void __cdecl mod_load_map_daemon(int a1);
void __cdecl mod_load_daemon(int a1, int a2);
void __cdecl mod_auto_save();
int __cdecl mod_turn_upkeep();
int __cdecl mod_faction_upkeep(int faction);
void __cdecl mod_production_phase(int faction_id);
void __cdecl mod_name_base(int faction, char* name, bool save_offset, bool water);
int __cdecl load_music_strcmpi(const char* active, const char* label);

