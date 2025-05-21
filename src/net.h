#pragma once

#include "main.h"

void __cdecl net_game_close();
void __cdecl net_treaty_on(int a1, int a2, int a3, int wait_diplo);
void __cdecl net_treaty_off(int a1, int a2, int a3, int wait_diplo);
void __cdecl net_set_treaty(int a1, int a2, int a3, int a4, int wait_diplo);
void __cdecl net_agenda_off(int a1, int a2, int a3, int wait_diplo);
void __cdecl net_set_agenda(int a1, int a2, int a3, int a4, int wait_diplo);
void __cdecl net_energy(int faction_id_1, int energy_val_1, int faction_id_2, int energy_val_2, int wait_diplo);
void __cdecl net_loan(int faction_id_1, int faction_id_2, int loan_total, int loan_payment);
void __cdecl net_maps(int a1, int a2, int wait_diplo);
void __cdecl net_tech(int a1, int a2, int a3, int wait_diplo);
void __cdecl net_pact_ends(int a1, int a2, int wait_diplo);
void __cdecl net_cede_base(int a1, int a2, int wait_diplo);
void __cdecl net_double_cross(int a1, int a2, int a3, int wait_diplo);
int __cdecl net_action_build(size_t veh_id, const char* name);
int __cdecl net_action_gate(size_t veh_id, size_t base_id);
int __cdecl net_action_destroy(size_t veh_id, int flag, int x, int y);

