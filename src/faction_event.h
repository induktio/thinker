#pragma once

#include "main.h"

int __cdecl DiploPop_spying(int faction_id);
void __cdecl pact_attacks(int faction_id_1, int faction_id_2);
void __cdecl pact_unpact(int faction_id_1, int faction_id_2);
void __cdecl pact_truce(int faction_id_1, int faction_id_2);
void __cdecl pact_treaty(int faction_id_1, int faction_id_2);
void __cdecl enemies_war(int faction_id_1, int faction_id_2);
void __cdecl enemies_unpact(int faction_id_1, int faction_id_2);
void __cdecl enemies_team_up(int faction_id_1, int faction_id_2, int faction_id_3);
void __cdecl enemies_trade_tech(int faction_id_1, int faction_id_2);
void __cdecl enemies_treaty(int faction_id_1, int faction_id_2, int trade_tech);
void __cdecl comm_check(int faction_id_1, int faction_id_2);
void __cdecl encounter(int faction_id_1, int faction_id_2, int x, int y, int fungal_tower, int native_unit);
int __cdecl territory(int faction_id_1, int faction_id_2, int flags, int* out_base_id, int* out_count);
void __cdecl enemy_diplomacy(int faction_id);
void __cdecl enemy_capabilities(int faction_id);
void __cdecl enemy_strategy(int faction_id);

