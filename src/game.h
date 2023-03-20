#pragma once

#include "main.h"

/*
This header includes the main turn processing functions and
various other game features that don't clearly belong elsewhere.
*/

bool un_charter();
bool victory_done();
bool valid_player(int faction);
bool valid_triad(int triad);
int __cdecl game_year(int n);
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask);
int __cdecl mod_cost_factor(int faction_id, int is_mineral, int base_id);
int __cdecl mod_upgrade_cost(int faction, int new_unit_id, int old_unit_id);

void init_save_game(int faction);
void __cdecl mod_auto_save();
int __cdecl mod_turn_upkeep();
int __cdecl mod_faction_upkeep(int faction);
void __cdecl mod_name_base(int faction, char* name, bool save_offset, bool water);
int probe_upkeep(int faction);
int probe_active_turns(int faction1, int faction2);
int __thiscall probe_popup_start(Win* This, int veh_id1, int base_id, int a4, int a5, int a6, int a7);

