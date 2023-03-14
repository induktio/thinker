#pragma once

#include "main.h"

bool __cdecl can_arty(int unit_id, bool allow_sea_arty);
bool __cdecl has_abil(int unit_id, VehAbilityFlag ability);
bool has_ability(int faction, VehAbility abl, VehChassis chs, VehWeapon wpn);

int __cdecl veh_stack(int x, int y);
int __cdecl proto_speed(int unit_id);
int __cdecl veh_speed(int veh_id, bool skip_morale);
int __cdecl veh_speed(int veh_id);
int __cdecl veh_cargo(int veh_id);

int __cdecl mod_get_basic_offense(int veh_id_atk, int veh_id_def, int psi_combat_type, bool is_bombard, bool unk_tgl);
int __cdecl mod_get_basic_defense(int veh_id_def, int veh_id_atk, int psi_combat_type, bool is_bombard);
int __cdecl psi_factor(int value, int faction_id, bool is_attack, bool is_fungal_twr);
int __cdecl neural_amplifier_bonus(int value);
int __cdecl dream_twister_bonus(int value);
int __cdecl fungal_tower_bonus(int value);

int __cdecl mod_veh_init(int unit_id, int faction, int x, int y);
int __cdecl mod_veh_kill(int veh_id);
int __cdecl mod_veh_skip(int veh_id);
int __cdecl mod_veh_wake(int veh_id);
int __cdecl find_return_base(int veh_id);
int __cdecl probe_return_base(int UNUSED(x), int UNUSED(y), int veh_id);

VehArmor best_armor(int faction, bool cheap);
VehWeapon best_weapon(int faction);
VehReactor best_reactor(int faction);
int offense_value(UNIT* u);
int defense_value(UNIT* u);

int set_move_to(int veh_id, int x, int y);
int set_move_next(int veh_id, int offset);
int set_road_to(int veh_id, int x, int y);
int set_action(int veh_id, int act, char icon);
int set_convoy(int veh_id, ResType res);
int set_board_to(int veh_id, int trans_veh_id);
int cargo_loaded(int veh_id);

