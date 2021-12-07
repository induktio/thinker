#pragma once

#include "main.h"

int __cdecl mod_base_prod_choices(int base_id, int v1, int v2, int v3);
void __cdecl base_first(int base_id);
bool can_build_ships(int base_id);
int consider_staple(int base_id);
int consider_hurry();
int need_psych(int base_id);
int find_project(int base_id);
int select_production(int base_id);
int find_proto(int base_id, int triad, int mode, bool defend);
int select_combat(int base_id, int num_probes, bool sea_base, bool build_ships);
