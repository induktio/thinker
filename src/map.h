#pragma once

#include "main.h"
#include "lib/FastNoiseLite.h"

const int ManifoldHarmonicsBonus[][3] = {
    {0,0,0}, // Planet < 0
    {0,0,1}, // Planet = 0
    {1,0,1}, // Planet = 1
    {1,1,1}, // Planet = 2
    {1,1,2}, // Planet > 2
};

MAP* mapsq(int x, int y);
int wrap(int x);
int map_range(int x1, int y1, int x2, int y2);
int vector_dist(int x1, int y1, int x2, int y2);
int min_range(const Points& S, int x, int y);
int min_vector(const Points& S, int x, int y);
double avg_range(const Points& S, int x, int y);
bool is_ocean(MAP* sq);
bool is_ocean(BASE* base);
bool is_ocean_shelf(MAP* sq);
bool is_shore_level(MAP* sq);
bool map_is_flat();
bool adjacent_region(int x, int y, int owner, int threshold, bool ocean);
int clear_overlay(int x, int y);
void refresh_overlay(std::function<int(int, int)> tile_value);

template <class A, class B>
int map_range(const A* a, const B* b) {
    return map_range(a->x, a->y, b->x, b->y);
}

int __cdecl is_coast(int x, int y, bool is_base_radius);
int __cdecl is_port(int base_id, bool is_base_radius);
int __cdecl bad_reg(int region);
int __cdecl region_at(int x, int y);
int __cdecl base_at(int x, int y);
int __cdecl x_dist(int x1, int x2);
uint32_t __cdecl bit_at(int x, int y);
void __cdecl bit_put(int x, int y, uint32_t items);
void __cdecl bit_set(int x, int y, uint32_t items, bool add);
uint32_t __cdecl bit2_at(int x, int y);
void __cdecl bit2_set(int x, int y, uint32_t items, bool add);
uint32_t __cdecl code_at(int x, int y);
void __cdecl code_set(int x, int y, int code);
int __cdecl has_temple(int faction_id);
int __cdecl near_landmark(int x, int y);
void __cdecl map_wipe();
int __cdecl resource_yield(BaseResType type, int faction_id, int base_id, int x, int y);
int __cdecl mod_crop_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_mine_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_energy_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_hex_cost(int unit_id, int faction_id, int x1, int y1, int x2, int y2, int toggle);
int __cdecl mod_bonus_at(int x, int y);
int __cdecl mod_goody_at(int x, int y);
int __cdecl mod_base_find3(int x, int y, int faction_id, int region, int faction_id_2, int faction_id_3);
int __cdecl mod_whose_territory(int faction_id, int x, int y, int* base_id, int ignore_comm);
int total_yield(int x, int y, int faction);
int fungus_yield(int faction, ResType res_type);
int item_yield(int x, int y, int faction, int bonus, MapItem item);
bool locate_landmark(int* x, int* y, bool ocean);
void __cdecl mod_world_monsoon(int x, int y);
void __cdecl mod_world_borehole(int x, int y);
void __cdecl mod_world_fossil(int x, int y);
void world_generate(uint32_t seed);
void console_world_generate(uint32_t seed);
void __cdecl find_start(int faction, int* tx, int* ty);
void __cdecl mod_world_build();
void __cdecl mod_time_warp();

