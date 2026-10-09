#pragma once

#include "main.h"

void __cdecl monument(int is_new_event);
void __cdecl clear_monuments();
void __cdecl mon_colony_founded(int faction_id, char* base_name);
void __cdecl mon_tech_discovered(int faction_id, int tech_id);
void __cdecl mon_secrets_of_tech(int faction_id);
void __cdecl mon_prototype_built(int faction_id);
void __cdecl mon_facility_built(int faction_id, char* facility_name);
void __cdecl mon_secret_project(int faction_id, int project_id);
void __cdecl mon_enemy_destroyed(int faction_id, int enemy_id);
void __cdecl mon_conquer_base(int faction_id, char* base_name);
void __cdecl mon_naval_unit_built(int faction_id);
void __cdecl mon_air_unit_built(int faction_id);
void __cdecl mon_native_life_bred(int faction_id);
void __cdecl mon_first_in_space(int faction_id);
void __cdecl mon_built_preserve(int faction_id);
void __cdecl mon_winning_unify(int faction_id, int winner_id);
void __cdecl mon_winning_trans(int faction_id, int winner_id);
void __cdecl mon_killed_faction(int faction_id, int killed_id);
