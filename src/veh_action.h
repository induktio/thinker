#pragma once

#include "main.h"

int __cdecl action(int veh_id);
int __cdecl action_build(int veh_id, const char *name);
int __cdecl action_terraform(int veh_id, int item_id, int toggle);
void __cdecl action_go_to(net_int_t veh_id);
void __cdecl action_road_to(net_int_t veh_id);
void __cdecl action_destroy(int veh_id, int tgt_item, int tgt_x, int tgt_y);
void __cdecl action_arty(net_int_t veh_id, int tx, int ty);
void __cdecl action_destruct(int veh_id);
void __cdecl action_oblit(int veh_id, int base_id);
