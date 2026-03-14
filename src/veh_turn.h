#pragma once

#include "main.h"

enum EnemyVehMove { // return codes for enemy_veh processing
    VEH_SYNC = 0,
    VEH_SKIP = 1,
};

void __cdecl mod_enemy_turn(int faction_id);
int __cdecl mod_enemy_veh(int veh_id);
int __cdecl mod_enemy_move(int veh_id);
int __cdecl mod_alien_base(int veh_id, int x, int y);
int __cdecl mod_alien_move(int veh_id);
void __cdecl mod_alien_fauna();
void __cdecl mod_do_fungal_towers();
