#pragma once

#include "main.h"

void reader_file(vec_str_t& lines, const char* filename, const char* section, uint32_t max_len);
void reader_path(vec_str_t& lines, const char* filename, const char* section, uint32_t max_len);
void init_save_game(int faction);
void __cdecl mod_auto_save();
int __cdecl mod_turn_upkeep();
int __cdecl mod_faction_upkeep(int faction);
int __cdecl mod_base_find3(int x, int y, int faction1, int region, int faction2, int faction3);
void __cdecl mod_name_base(int faction, char* name, bool save_offset, bool water);
int __cdecl mod_best_defender(int defender_veh_id, int attacker_veh_id, int bombardment);
int __cdecl battle_fight_parse_num(int index, int value);
int probe_upkeep(int faction);
int probe_active_turns(int faction1, int faction2);
int __thiscall probe_popup_start(Win* This, int veh_id1, int base_id, int a4, int a5, int a6, int a7);

