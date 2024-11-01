#pragma once

#include "main.h"

bool has_chassis(int faction_id, VehChassis chs);
bool has_weapon(int faction_id, VehWeapon wpn);
bool has_wmode(int faction_id, VehWeaponMode mode);
bool has_aircraft(int faction_id);
bool has_ships(int faction_id);
bool has_terra(FormerItem frm_id, bool ocean, int faction_id);
bool has_project(FacilityId item_id);
bool has_project(FacilityId item_id, int faction_id);
int project_base(FacilityId item_id);
int facility_count(FacilityId item_id, int faction_id);
int prod_count(int item_id, int faction_id, int base_skip_id);

bool is_human(int faction_id);
bool is_alive(int faction_id);
bool is_alien(int faction_id);
bool thinker_enabled(int faction_id);
bool at_war(int faction1, int faction2);
bool has_pact(int faction1, int faction2);
bool both_neutral(int faction1, int faction2);
bool both_non_enemy(int faction1, int faction2);
bool want_revenge(int faction1, int faction2);
bool allow_expand(int faction_id);
bool has_transport(int x, int y, int faction_id);
bool has_defenders(int x, int y, int faction_id);
bool has_active_veh(int faction_id, VehPlan plan);
int veh_count(int faction_id, int unit_id);
int find_hq(int faction_id);
int faction_might(int faction_id);

void __cdecl set_treaty(int faction1, int faction2, uint32_t treaty, bool add);
void __cdecl set_agenda(int faction1, int faction2, uint32_t agenda, bool add);
int __cdecl has_treaty(int faction1, int faction2, uint32_t treaty);
int __cdecl has_agenda(int faction1, int faction2, uint32_t agenda);
int __cdecl council_votes(int faction_id);
int __cdecl eligible(int faction_id);
int __cdecl great_beelzebub(int faction_id, int is_aggressive);
int __cdecl great_satan(int faction_id, int is_aggressive);
int __cdecl aah_ooga(int faction_id, int pact_faction_id);
int __cdecl climactic_battle();
int __cdecl at_climax(int faction_id);
void __cdecl cause_friction(int faction_id, int faction_id_with, int friction);
int __cdecl get_mood(int friction);
int __cdecl reputation(int faction_id, int faction_id_with);
int __cdecl get_patience(int faction_id_with, int faction_id);
int __cdecl energy_value(int loan_principal);
void __cdecl social_calc(CSocialCategory* category, CSocialEffect* effect,
int faction_id, int toggle, int is_quick_calc);
void __cdecl social_upkeep(int faction_id);
int __cdecl social_upheaval(int faction_id, CSocialCategory* choices);
int __cdecl society_avail(int soc_category, int soc_model, int faction_id);
int __cdecl mod_setup_player(int faction_id, int a2, int a3);
int __cdecl SocialWin_social_ai(int faction_id, int a2, int a3, int a4, int a5, int a6);
int __cdecl mod_social_ai(int faction_id, int a2, int a3, int a4, int a5, int a6);
int __cdecl mod_wants_to_attack(int faction_id, int faction_id_tgt, int faction_id_unk);

