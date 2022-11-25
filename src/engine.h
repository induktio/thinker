#pragma once

#include "main.h"

std::vector<std::string> read_txt_block(const char* filename, const char* section, unsigned max_len);
int probe_active_turns(int faction1, int faction2);
int probe_upkeep(int faction);
void init_save_game(int faction);
void __cdecl mod_auto_save();
int __cdecl mod_turn_upkeep();
int __cdecl mod_faction_upkeep(int faction);
int __cdecl mod_base_find3(int x, int y, int faction1, int region, int faction2, int faction3);
void __cdecl mod_name_base(int faction, char* name, bool save_offset, bool water);
int __cdecl mod_best_defender(int defender_veh_id, int attacker_veh_id, int bombardment);
int __cdecl battle_fight_parse_num(int index, int value);

