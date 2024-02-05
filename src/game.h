#pragma once

#include "main.h"

/*
This header includes the main turn processing functions and
various other game features that don't clearly belong elsewhere.
*/

// Select only those settings that are not set in Special Scenario Rules
const uint32_t GAME_RULES_MASK = 0x7808FFFF;
const uint32_t GAME_MRULES_MASK = 0xFFFFFFF0;

bool un_charter();
bool victory_done();
bool valid_player(int faction);
bool valid_triad(int triad);
int __cdecl game_year(int n);
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask);
int __cdecl mod_cost_factor(int faction_id, int is_mineral, int base_id);

void show_rules_menu();
void init_world_config();
void init_save_game(int faction);
void __cdecl mod_auto_save();
int __cdecl mod_turn_upkeep();
int __cdecl mod_faction_upkeep(int faction);
void __cdecl mod_name_base(int faction, char* name, bool save_offset, bool water);
int probe_upkeep(int faction);
int probe_active_turns(int faction1, int faction2);
int __thiscall probe_popup_start(Win* This, int veh_id1, int base_id, int a4, int a5, int a6, int a7);

