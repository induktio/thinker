#pragma once

#include "main.h"

bool un_charter();
bool victory_done();
bool valid_player(int faction);
bool valid_triad(int triad);
int __cdecl game_year(int n);
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask);
int __cdecl neural_amplifier_bonus(int value);
int __cdecl dream_twister_bonus(int value);
int __cdecl fungal_tower_bonus(int value);
int __cdecl psi_factor(int value, int faction_id, bool is_attack, bool is_fungal_twr);
int __cdecl fac_maint(int facility_id, int faction_id);
int __cdecl mod_cost_factor(int faction_id, int is_mineral, int base_id);
int __cdecl mod_upgrade_cost(int faction, int new_unit_id, int old_unit_id);
void __cdecl add_goal(int faction, int type, int priority, int x, int y, int base_id);
void __cdecl add_site(int faction, int type, int priority, int x, int y);
void __cdecl wipe_goals(int faction);
int has_goal(int faction, int type, int x, int y);
Goal* find_priority_goal(int faction, int type, int* px, int* py);

