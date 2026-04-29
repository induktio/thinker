#pragma once

#include "main.h"

std::vector<int> captured_leaders(int faction_id);
void reset_captured_leader(int faction_id, int capture_id);
int __cdecl vulnerable(int faction_id, int x, int y);
int __cdecl corner_market(int faction_id);
int __cdecl steal_energy(int base_id);
int __cdecl mod_mind_control(int base_id, int faction_id, int is_corner_market);
int __cdecl mod_success_rates(size_t index, int morale, int diff_modifier, int base_id);
void __cdecl probe_renew_set(int faction_id, int faction_id_tgt, int turns);
int __cdecl probe_has_renew(int faction_id, int faction_id_tgt);
int probe_roll_value(int faction_id);
int probe_upkeep(int faction1);
int probe_active_turns(int faction1, int faction2);
int __thiscall probe_popup_start(Popup* This, int veh_id, int base_id, int a4, char* a5, int a6, GraphicWin* a7);
int __cdecl probe(int veh_id, int tgt_base_id, int tgt_veh_id, int toggle);

