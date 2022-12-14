#pragma once

#include "main.h"

bool __cdecl can_arty(int unit_id, bool allow_sea_arty);
int __cdecl veh_cargo(int veh_id);
int __cdecl mod_veh_speed(int veh_id);
int __cdecl mod_veh_init(int unit_id, int faction, int x, int y);
int __cdecl mod_veh_kill(int veh_id);
int __cdecl mod_veh_skip(int veh_id);
int __cdecl mod_veh_wake(int veh_id);

