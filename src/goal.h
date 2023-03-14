#pragma once

#include "main.h"

void __cdecl add_goal(int faction, int type, int priority, int x, int y, int base_id);
void __cdecl add_site(int faction, int type, int priority, int x, int y);
void __cdecl wipe_goals(int faction);
int has_goal(int faction, int type, int x, int y);
Goal* find_priority_goal(int faction, int type, int* px, int* py);

