#pragma once

#include "main.h"

int __cdecl action(int veh_id);
int __cdecl action_terraform(int veh_id, int item_id, int toggle);
void __cdecl action_go_to(net_int_t veh_id);
void __cdecl action_road_to(net_int_t veh_id);

