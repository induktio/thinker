#pragma once

#include "main.h"

void __cdecl add_goal(int faction_id, int type, int priority, int x, int y, int base_id);
void __cdecl add_site(int faction_id, int type, int priority, int x, int y);
void __cdecl wipe_goals(int faction_id);
void __cdecl clear_goals(int faction_id);
void __cdecl del_site(int faction_id, int type, int x, int y, int max_dist);
int has_goal(int faction_id, int type, int x, int y);
Goal* find_priority_goal(int faction_id, int type, int* px, int* py);

