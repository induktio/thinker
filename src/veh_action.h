#pragma once

#include "main.h"

int __cdecl terrain_avail(FormerItem frm_id, int ocean, int faction_id);
inline bool has_terra(FormerItem frm_id, int ocean, int faction_id) {
    return terrain_avail(frm_id, ocean, faction_id);
}
int __cdecl contribution(int veh_id, int frm_id);
int __cdecl terraform_cost(int x, int y, int faction_id);
int __cdecl action(int veh_id);
int __cdecl action_build(int veh_id, const char *name);
int __cdecl action_terraform(int veh_id, int item_id, int toggle);
void __cdecl action_go_to(net_int_t veh_id);
void __cdecl action_road_to(net_int_t veh_id);
void __cdecl action_destroy(int veh_id, int tgt_item, int tgt_x, int tgt_y);
int __cdecl action_home(int veh_id, int flag);
int __cdecl action_airdrop(int veh_id, int tx, int ty, int flags);
void __cdecl action_arty(net_int_t veh_id, int tx, int ty);
void __cdecl action_destruct(int veh_id);
void __cdecl action_oblit(int veh_id, int base_id);
int __cdecl valid_patrol(int veh_id, int tx, int ty);
int __cdecl action_patrol(int veh_id, int tx, int ty);
int __cdecl shoot_it(int faction_id_atk, int faction_id_def, int tx, int ty, int flag);
void __cdecl action_tectonic(int veh_id, int tx, int ty);
void __cdecl action_fungal(int veh_id, int tx, int ty);
void __cdecl action_give(int veh_id, int faction_id_tgt);
void __cdecl action_gate(int veh_id, int base_id);
int __cdecl order_veh(int veh_id, int offset, int flag);

