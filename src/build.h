#pragma once

#include "main.h"

int __cdecl mod_base_hurry();
int hurry_item(int base_id, int mins, int cost);
int consider_staple(int base_id);
bool redundant_project(int faction_id, int item_id);
int find_satellite(int base_id);
int find_missile(int base_id);
int find_project(int base_id, WItem& Wgov);
int need_scouts(int base_id, Triad triad);
bool unit_is_better(int unit_id1, int unit_id2);
int unit_score(BASE* base, int unit_id, int psi_score, int psi_atk, int psi_def, bool defend);
int find_proto(int base_id, TriadFlag triad, VehWeaponMode mode, bool defend);
int select_colony(int base_id, int num_colony, bool build_ships);
int select_combat(int base_id, bool sea_base, bool build_ships);
int select_build(int base_id);

