#pragma once

#include "main.h"

int __cdecl get_rating(int faction_id, int score);
int __cdecl is_objective(int base_id);
int __cdecl num_objectives(int faction_id, int incl_pact);
int __cdecl most_objectives(int* winner_id, int* tied_rivals);
void __cdecl rankings(int flag);
void __cdecl compute_score(int faction_id, int* score_val, int* table_val, int flag);

