#pragma once

#include "main.h"

int tech_level(int id, int lvl);
int tech_cost(int faction, int tech);
int __cdecl mod_tech_rate(int faction);
int __cdecl mod_tech_selection(int faction);
int __cdecl mod_tech_value(int tech, int faction, int flag);

