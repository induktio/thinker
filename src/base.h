#pragma once

#include "main.h"

const int GOV_ALLOW_COMBAT =
    (GOV_MAY_PROD_LAND_COMBAT | GOV_MAY_PROD_NAVAL_COMBAT | GOV_MAY_PROD_AIR_COMBAT);

void __cdecl mod_base_reset(int base_id, bool has_gov);
int __cdecl mod_base_build(int base_id, bool has_gov);
void __cdecl base_first(int base_id);
bool can_build_ships(int base_id);
int consider_staple(int base_id);
int consider_hurry();
int need_psych(int base_id);
int find_project(int base_id);
int select_production(int base_id);
int find_proto(int base_id, Triad triad, VehWeaponMode mode, bool defend);
int select_combat(int base_id, int num_probes, bool sea_base, bool build_ships);
