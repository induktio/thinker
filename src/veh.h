#pragma once

#include "main.h"

int __cdecl can_arty(int unit_id, bool allow_sea_arty);
int __cdecl has_abil(int unit_id, VehAblFlag ability);
int __cdecl arty_range(int unit_id);
int __cdecl drop_range(int faction);
bool has_orbital_drops(int faction);
bool has_ability(int faction, VehAbl abl, VehChassis chs, VehWeapon wpn);

int __cdecl veh_stack(int x, int y);
int __cdecl proto_speed(int unit_id);
int __cdecl veh_speed(int veh_id, bool skip_morale);
int __cdecl veh_cargo(int veh_id);
int __cdecl want_monolith(uint32_t veh_id);
int __cdecl prototype_factor(int unit_id);
int __cdecl mod_proto_cost(VehChassis chassis_id, VehWeapon weapon_id,
    VehArmor armor_id, VehAblFlag ability, VehReactor reactor_id);
int __cdecl mod_base_cost(int unit_id);
int __cdecl mod_veh_cost(int unit_id, int base_id, int32_t* has_proto_cost);
int __cdecl mod_upgrade_cost(int faction_id, int new_unit_id, int old_unit_id);
int __cdecl mod_veh_avail(int unit_id, int faction_id, int base_id);
int __cdecl mod_stack_check(int veh_id, int type, int cond1, int cond2, int cond3);

int __cdecl mod_veh_init(int unit_id, int faction, int x, int y);
int __cdecl mod_veh_kill(int veh_id);
int __cdecl mod_veh_skip(int veh_id);
int __cdecl mod_veh_wake(int veh_id);
int __cdecl find_return_base(int veh_id);
int __cdecl probe_return_base(int UNUSED(x), int UNUSED(y), int veh_id);
int __cdecl create_proto(int faction, VehChassis chs, VehWeapon wpn, VehArmor arm,
    VehAblFlag abls, VehReactor rec, VehPlan ai_plan);
int __cdecl mod_is_bunged(int faction, VehChassis chs, VehWeapon wpn, VehArmor arm,
    VehAblFlag abls, VehReactor rec);
int __cdecl mod_name_proto(char* name, int unit_id, int faction_id,
    VehChassis chs, VehWeapon wpn, VehArmor arm, VehAblFlag abls, VehReactor rec);

VehArmor best_armor(int faction, int max_cost);
VehWeapon best_weapon(int faction);
VehReactor best_reactor(int faction);
int offense_value(int unit_id);
int defense_value(int unit_id);

int set_move_to(int veh_id, int x, int y);
int set_move_next(int veh_id, int offset);
int set_road_to(int veh_id, int x, int y);
int set_action(int veh_id, int act, char icon);
int set_order_none(int veh_id);
int set_convoy(int veh_id, ResType res);
int set_board_to(int veh_id, int trans_veh_id);
int cargo_loaded(int veh_id);

