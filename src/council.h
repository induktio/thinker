#pragma once

#include "main.h"

void __cdecl clear_council_notify(int faction_id);
void __cdecl clear_council_notify_2();
void __cdecl council(int faction_id, int proposal_id, int candidate_id);
int __cdecl wants_prop(int faction_id_1, int proposal_id, int faction_id_2);
int __cdecl council_get_vote(int faction_id, int proposal_id, int faction_id_2);
void __cdecl council_action(int proposal_id, int faction_id);
int __cdecl can_call_council(int faction_id, int is_player);
void __cdecl call_council(int faction_id);

