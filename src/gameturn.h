#pragma once

#include "main.h"

void __cdecl control_turn();
void __cdecl clear_council_notify(int faction_id);
void __cdecl clear_council_notify_2();
void __cdecl random_events(int flag);
void __cdecl alien_fauna();
void __cdecl do_fungal_towers();
void __cdecl turn_upkeep();
void __cdecl faction_upkeep(int faction_id);
void __cdecl repair_phase(int faction_id);
void __cdecl production_phase(int faction_id);
void __cdecl allocate_energy(int faction_id);

