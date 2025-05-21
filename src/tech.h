#pragma once

#include "main.h"

const char* tech_str(int tech_id);
int __cdecl has_tech(int tech_id, int faction_id);
int __cdecl tech_level(int tech_id, int lvl);
int __cdecl tech_category(int tech_id);
int __cdecl mod_tech_avail(int tech_id, int faction_id);
void __cdecl mod_tech_effects(int faction_id);
void __cdecl mod_tech_research(int faction_id, int value);
int __cdecl mod_tech_selection(int faction_id);
int __cdecl mod_tech_pick(int faction_id, int flag, int probe_faction_id, const char* label);
int __cdecl mod_tech_rate(int faction_id);
void __cdecl say_tech(char* output, int tech_id, int incl_category);
int __cdecl tech_is_preq(int preq_tech_id, int parent_tech_id, int range);
int __cdecl mod_tech_val(int tech_id, int faction_id, int flag);
int __cdecl mod_tech_ai(int faction_id);
int tech_cost(int tech_id, int faction_id);
int __cdecl tech_alt_val(int tech_id, int faction_id, int flag);

