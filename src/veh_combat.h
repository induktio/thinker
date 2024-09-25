#pragma once

#include "main.h"

enum VehCombatType {
    CT_CAN_ARTY = 0x1,
    CT_WEAPON_ONLY = 0x2,
    CT_AIR_DEFENSE = 0x8,
    CT_INTERCEPT = 0x10,
};

const int PulseArmorValue = 25;
const int ResonanceArmorValue = 25;
const int ResonanceWeaponValue = 25;
const int FlechetteDefenseValue = 50;
const int FlechetteDefenseRange = 2;
const int GeosyncSurveyPodRange = 3;

int __cdecl fungal_tower_bonus(int value);
int __cdecl dream_twister_bonus(int value);
int __cdecl neural_amplifier_bonus(int value);
int __cdecl psi_factor(int value, int faction_id, bool is_attack, bool is_fungal_twr);
int __cdecl battle_kill_credits(int veh_id);

int __cdecl mod_morale_alien(int veh_id, int faction_id_vs_native);
int __cdecl mod_morale_veh(int veh_id, int check_drone_riot, int faction_id_vs_native);
int __cdecl mod_get_basic_offense(int veh_id_atk, int veh_id_def, int psi_combat_type, bool is_bombard, bool unk_tgl);
int __cdecl mod_get_basic_defense(int veh_id_def, int veh_id_atk, int psi_combat_type, bool is_bombard);

int __cdecl mod_best_defender(int veh_id_def, int veh_id_atk, int check_arty);
int __cdecl battle_fight_parse_num(int index, int value);
int __cdecl terrain_defense(VEH* veh_def, VEH* veh_atk);
void __cdecl add_bat(int type, int modifier, const char* display_str);
void __cdecl mod_battle_compute(int veh_id_atk, int veh_id_def, int* offense_out, int* defense_out, int combat_type);
int __cdecl mod_battle_fight_2(int veh_id, int offset, int tx, int ty, int table_offset, int option, int* def_id);

