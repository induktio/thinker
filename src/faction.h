#pragma once

#include "main.h"

bool has_tech(int tech_id, int faction);
bool has_chassis(int faction, VehChassis chs);
bool has_weapon(int faction, VehWeapon wpn);
bool has_wmode(int faction, VehWeaponMode mode);
bool has_aircraft(int faction);
bool has_ships(int faction);
bool has_orbital_drops(int faction);
bool has_terra(int faction, FormerItem act, bool ocean);
bool has_project(FacilityId item_id, int faction);
bool has_free_facility(FacilityId item_id, int faction);
int facility_count(FacilityId item_id, int faction);
int prod_count(int item_id, int faction, int base_skip_id);

bool is_human(int faction);
bool is_alive(int faction);
bool is_alien(int faction);
bool thinker_enabled(int faction);
bool at_war(int faction1, int faction2);
bool has_pact(int faction1, int faction2);
bool has_treaty(int faction1, int faction2, uint32_t treaty);
bool has_agenda(int faction1, int faction2, uint32_t agenda);
bool want_revenge(int faction1, int faction2);
bool allow_expand(int faction);
bool has_colony_pods(int faction);
bool has_transport(int x, int y, int faction);
bool has_defenders(int x, int y, int faction);
int find_hq(int faction);
int manifold_nexus_owner();
int faction_might(int faction);

void __cdecl set_treaty(int faction1, int faction2, uint32_t treaty, bool add);
void __cdecl set_agenda(int faction1, int faction2, uint32_t agenda, bool add);
int __cdecl mod_setup_player(int faction, int a2, int a3);
int __cdecl SocialWin_social_ai(int faction, int a2, int a3, int a4, int a5, int a6);
int __cdecl mod_social_ai(int faction, int a2, int a3, int a4, int a5, int a6);

