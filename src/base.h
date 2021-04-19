#pragma once

#include "main.h"

int __cdecl mod_base_prod_choices(int id, int v1, int v2, int v3);
int need_defense(int id);
int need_psych(int id);
int consider_hurry(int id);
int find_project(int id);
int find_facility(int id);
int select_production(int id);
int select_combat(int base_id, bool sea_base, bool build_ships);
int find_proto(int base_id, int triad, int mode, bool defend);
