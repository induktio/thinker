#pragma once

#include "main.h"

void __cdecl mod_tech_effects(int faction_id);
int __cdecl mod_tech_avail(int tech_id, int faction_id);
void __cdecl mod_tech_research(int faction_id, int value);
int __cdecl mod_tech_pick(int faction_id, int flag, int probe_faction_id, const char* label);
int __cdecl mod_tech_rate(int faction_id);
int __cdecl mod_tech_val(int tech_id, int faction_id, int flag);
int __cdecl mod_tech_ai(int faction_id);
int tech_level(int tech_id, int lvl);
int tech_cost(int faction_id, int tech_id);

