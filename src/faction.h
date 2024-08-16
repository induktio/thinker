#pragma once

#include "main.h"

bool has_tech(int tech_id, int faction);
bool has_chassis(int faction, VehChassis chs);
bool has_weapon(int faction, VehWeapon wpn);
bool has_wmode(int faction, VehWeaponMode mode);
bool has_aircraft(int faction);
bool has_ships(int faction);
bool has_terra(int faction, FormerItem act, bool ocean);
bool has_project(FacilityId item_id);
bool has_project(FacilityId item_id, int faction);
int project_base(FacilityId item_id);
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
bool both_neutral(int faction1, int faction2);
bool both_non_enemy(int faction1, int faction2);
bool want_revenge(int faction1, int faction2);
bool allow_expand(int faction);
bool has_active_veh(int faction, VehPlan plan);
bool has_transport(int x, int y, int faction);
bool has_defenders(int x, int y, int faction);
int find_hq(int faction);
int faction_might(int faction);

void __cdecl set_treaty(int faction1, int faction2, uint32_t treaty, bool add);
void __cdecl set_agenda(int faction1, int faction2, uint32_t agenda, bool add);
int __cdecl mod_social_cost(int faction_id, int* choices);
int __cdecl mod_society_avail(int soc_category, int soc_model, int faction_id);
int __cdecl mod_setup_player(int faction, int a2, int a3);
int __cdecl SocialWin_social_ai(int faction, int a2, int a3, int a4, int a5, int a6);
int __cdecl mod_social_ai(int faction, int a2, int a3, int a4, int a5, int a6);
int __cdecl mod_wants_to_attack(int faction_id, int faction_id_tgt, int faction_id_unk);

