#pragma once

#include "main.h"

const int GOV_ALLOW_COMBAT =
    (GOV_MAY_PROD_LAND_COMBAT | GOV_MAY_PROD_NAVAL_COMBAT | GOV_MAY_PROD_AIR_COMBAT);

void __cdecl mod_base_reset(int base_id, bool has_gov);
int __cdecl mod_base_build(int base_id, bool has_gov);
void __cdecl base_first(int base_id);
void __cdecl set_base(int base_id);
void __cdecl base_compute(bool update_prev);
int __cdecl mod_base_kill(int base_id);
int __cdecl mod_capture_base(int base_id, int faction, int is_probe);

char* prod_name(int item_id);
int prod_turns(int base_id, int item_id);
int mineral_cost(int base_id, int item_id);
int hurry_cost(int base_id, int item_id, int hurry_mins);
int unused_space(int base_id);
bool can_build(int base_id, int fac_id);
bool can_build_unit(int faction, int unit_id);
bool can_build_ships(int base_id);
bool maybe_riot(int base_id);
bool base_can_riot(int base_id, bool allow_staple);
bool base_pop_boom(int base_id);
bool can_use_teleport(int base_id);
void set_base_facility(int base_id, int facility_id, bool add);

int consider_staple(int base_id);
int consider_hurry();
int need_psych(int base_id);
int find_project(int base_id);
int select_production(int base_id);
int project_score(int faction, int proj, bool shuffle);
int find_proto(int base_id, Triad triad, VehWeaponMode mode, bool defend);
int select_combat(int base_id, int num_probes, bool sea_base, bool build_ships);
bool satellite_bonus(int base_id, int* nutrient, int* mineral, int* energy);

