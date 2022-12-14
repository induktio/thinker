#pragma once

#include "main.h"
#include "lib/FastNoiseLite.h"

MAP* mapsq(int x, int y);
int wrap(int x);
int map_range(int x1, int y1, int x2, int y2);
int vector_dist(int x1, int y1, int x2, int y2);
int min_range(const Points& S, int x, int y);
int min_vector(const Points& S, int x, int y);
double avg_range(const Points& S, int x, int y);
int unit_in_tile(MAP* sq);
int __cdecl region_at(int x, int y);
int __cdecl mod_hex_cost(int unit_id, int faction, int x1, int y1, int x2, int y2, int a7);
int __cdecl mod_bonus_at(int x, int y);
int __cdecl mod_goody_at(int x, int y);
int total_yield(int x, int y, int faction);
int fungus_yield(int faction, int res_type);
int item_yield(int x, int y, int faction, int bonus, MapItem item);
void __cdecl find_start(int faction, int* tx, int* ty);
void __cdecl mod_world_build();
void __cdecl world_generate(uint32_t seed);



