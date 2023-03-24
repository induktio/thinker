#pragma once

#include "main.h"

bool __cdecl can_arty(int unit_id, bool allow_sea_arty);
bool __cdecl has_abil(int unit_id, VehAblFlag ability);
bool has_ability(int faction, VehAbl abl, VehChassis chs, VehWeapon wpn);

int __cdecl veh_stack(int x, int y);
int __cdecl proto_speed(int unit_id);
int __cdecl veh_speed(int veh_id, bool skip_morale);
int __cdecl veh_cargo(int veh_id);
int __cdecl mod_proto_cost(VehChassis chassis_id, VehWeapon weapon_id,
VehArmor armor_id, VehAblFlag ability, VehReactor reactor_id);
int __cdecl mod_base_cost(int unit_id);
int __cdecl mod_veh_cost(int unit_id, int base_id, int* has_proto_cost);
int __cdecl mod_upgrade_cost(int faction, int new_unit_id, int old_unit_id);

int __cdecl mod_veh_init(int unit_id, int faction, int x, int y);
int __cdecl mod_veh_kill(int veh_id);
int __cdecl mod_veh_skip(int veh_id);
int __cdecl mod_veh_wake(int veh_id);
int __cdecl find_return_base(int veh_id);
int __cdecl probe_return_base(int UNUSED(x), int UNUSED(y), int veh_id);
int __cdecl mod_name_proto(char* name, int unit_id, int faction_id,
VehChassis chs, VehWeapon wpn, VehArmor arm, VehAblFlag abls, VehReactor rec);

VehArmor best_armor(int faction, bool cheap);
VehWeapon best_weapon(int faction);
VehReactor best_reactor(int faction);
int offense_value(int unit_id);
int defense_value(int unit_id);

int set_move_to(int veh_id, int x, int y);
int set_move_next(int veh_id, int offset);
int set_road_to(int veh_id, int x, int y);
int set_action(int veh_id, int act, char icon);
int set_convoy(int veh_id, ResType res);
int set_board_to(int veh_id, int trans_veh_id);
int cargo_loaded(int veh_id);

