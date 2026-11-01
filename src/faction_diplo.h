#pragma once

#include "main.h"

int __cdecl auto_contact();
void __cdecl diplomacy_caption(int faction_id, int faction_id_2);
void __cdecl diplomacy_check(int faction_id, int faction_id_2, int full_check);
void __cdecl diplomacy_ends(int faction_id, int faction_id_2);
int __cdecl wants_to_speak(int faction_id, int faction_id_2);
int __cdecl diplomacy_menu(int faction_id, int faction_id_2);
int __cdecl proposal_menu(int faction_id, int faction_id_2);
void __cdecl make_a_proposal(int faction_id, int faction_id_2, int skip_menu);
void __cdecl commlink_attempt(int faction_id);
int __cdecl commlink_attempter(int faction_id, int faction_id_2);
void __cdecl communicate(int faction_id, int faction_id_2, int flag);

