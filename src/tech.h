#pragma once

#include "main.h"

extern int TechCostRatios[MaxDiffNum];

int __cdecl mod_tech_rate(int faction);
int __cdecl mod_tech_selection(int faction);
int __cdecl mod_tech_value(int tech, int faction, int flag);

