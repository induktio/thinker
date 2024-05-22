#pragma once

#include "main.h"

int __cdecl probe_veh_health(int veh_id);
int __cdecl probe_mind_control_range(int x1, int y1, int x2, int y2);
void __cdecl probe_thought_control(int faction_id_def, int faction_id_atk);
int probe_upkeep(int faction1);
int probe_active_turns(int faction1, int faction2);
int __thiscall probe_popup_start(Win* This, int veh_id1, int base_id, int a4, int a5, int a6, int a7);

