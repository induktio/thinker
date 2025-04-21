#pragma once

#include "main.h"

int __cdecl vulnerable(int faction_id, int x, int y);
int __cdecl corner_market(int faction_id);
int __cdecl steal_energy(int base_id);
int __cdecl mod_mind_control(int base_id, int faction_id, int is_corner_market);
int __cdecl mod_success_rates(size_t index, int morale, int diff_modifier, int base_id);
int __cdecl probe_veh_health(int veh_id);
int __cdecl probe_mind_control_range(int x1, int y1, int x2, int y2);
void __cdecl probe_thought_control(int faction_id_def, int faction_id_atk);
void __cdecl probe_renew_set(int faction_id, int faction_id_tgt, int turns);
int __cdecl probe_has_renew(int faction_id, int faction_id_tgt);
int probe_roll_value(int faction_id);
int probe_upkeep(int faction1);
int probe_active_turns(int faction1, int faction2);
int __thiscall probe_popup_start(Win* This, int veh_id1, int base_id, int a4, int a5, int a6, int a7);

