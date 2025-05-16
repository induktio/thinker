#pragma once

#include "main.h"

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
int __cdecl on_map(int x, int y);
int __cdecl bad_reg(int region);
void __cdecl owner_set(int x, int y, int faction_id);
void __cdecl site_set(int x, int y, uint8_t site);
void __cdecl region_set(int x, int y, uint8_t region);
int __cdecl region_at(int x, int y);
int __cdecl base_at(int x, int y);
int __cdecl x_dist(int x1, int x2);
int __cdecl alt_at(int x, int y);
int __cdecl alt_detail_at(int x, int y);
void __cdecl alt_put_detail(int x, int y, uint8_t detail);
void __cdecl alt_set(int x, int y, int altitude);
void __cdecl alt_set_both(int x, int y, int altitude);
int __cdecl alt_natural(int x, int y);
int __cdecl elev_at(int x, int y);
void __cdecl world_alt_set(int x, int y, int altitude, int toggle);
void __cdecl world_raise_alt(int x, int y);
void __cdecl world_lower_alt(int x, int y);
void __cdecl world_alt_put_detail(int x, int y);
int __cdecl temp_at(int x, int y);
void __cdecl temp_set(int x, int y, uint8_t value);
void __cdecl climate_set(int x, int y, uint8_t rainfall);
void __cdecl rocky_set(int x, int y, uint8_t rocky);
uint32_t __cdecl bit_at(int x, int y);
void __cdecl bit_put(int x, int y, uint32_t items);
void __cdecl bit_set(int x, int y, uint32_t items, bool add);
uint32_t __cdecl bit2_at(int x, int y);
void __cdecl bit2_set(int x, int y, uint32_t items, bool add);
uint32_t __cdecl code_at(int x, int y);
void __cdecl code_set(int x, int y, int code);
void __cdecl synch_bit(int x, int y, int faction_id);
int __cdecl has_temple(int faction_id);
int __cdecl find_landmark(int x, int y, size_t range_offset);
int __cdecl new_landmark(int x, int y, const char* name);
int __cdecl valid_landmark(int x, int y, int faction_id);
void __cdecl kill_landmark(int x, int y);
void __cdecl map_wipe();
int __cdecl resource_yield(BaseResType type, int faction_id, int base_id, int x, int y);
int __cdecl mod_crop_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_mine_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_energy_yield(int faction_id, int base_id, int x, int y, int flag);
int __cdecl mod_hex_cost(int unit_id, int faction_id, int x1, int y1, int x2, int y2, int toggle);
int __cdecl mod_minerals_at(int x, int y);
int __cdecl mod_bonus_at(int x, int y);
int __cdecl mod_goody_at(int x, int y);
int __cdecl mod_base_find3(int x, int y, int faction_id, int region, int faction_id_2, int faction_id_3);
int __cdecl mod_whose_territory(int faction_id, int x, int y, int* base_id, int ignore_comm);
int bonus_yield(int res_type);
int total_yield(int x, int y, int faction_id);
int fungus_yield(int faction_id, ResType res_type);
int item_yield(int x, int y, int faction_id, int bonus, MapItem item);
void __cdecl find_start(int faction_id, int* tx, int* ty);
void __cdecl mod_time_warp();

