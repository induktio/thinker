#pragma once

#include "main.h"
#include "lib/FastNoiseLite.h"

int total_yield(int x, int y, int faction);
int fungus_yield(int faction, int res_type);
int item_yield(int x, int y, int faction, int bonus, MAP* sq, uint32_t item);
void __cdecl find_start(int faction, int* tx, int* ty);
void __cdecl mod_world_build();



