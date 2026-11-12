#pragma once

#include "main.h"

void __cdecl scan_prototypes(int faction_id, int faction_id_2);
void __cdecl mention_prototypes(int faction_id, int faction_id_2);
int __cdecl pact_withdraw(int faction_id, int faction_id_2);
void __cdecl pact_ends(int faction_id, int faction_id_2);
void __cdecl pact_of_brotherhood(int faction_id, int faction_id_2);
void __cdecl make_treaty(int faction_id, int faction_id_2);
void __cdecl pledge_truce(int faction_id, int faction_id_2, int flag);
void __cdecl introduce(int faction_id, int faction_id_2);
void __cdecl demands_withdrawal(int faction_id, int faction_id_2);
void __cdecl renounce_pact(int faction_id, int faction_id_2);
void __cdecl tech_analysis(int faction_id, int faction_id_2);
void __cdecl buy_council_vote(int faction_id, int faction_id_2, int proposal_id, int vote_id);
int __cdecl buy_tech(int faction_id, int faction_id_2, int counter_id, int high_price, int proposal_id);
int __cdecl mod_buy_tech(int faction_id, int faction_id_2, int counter_id, int high_price, int proposal_id);
int __cdecl energy_trade(int faction_id, int faction_id_2);
int __cdecl mod_energy_trade(int faction_id, int faction_id_2);
int __cdecl tech_trade(int faction_id, int faction_id_2, int rival_init, int counter_id, int proposal_id);
void __cdecl trade_maps(int faction_id, int faction_id_2);
int __cdecl propose_pact(int faction_id, int faction_id_2);
int __cdecl propose_treaty(int faction_id, int faction_id_2);
void __cdecl propose_trade_maps(int faction_id, int faction_id_2);
int __cdecl propose_attack(int faction_id, int faction_id_2);
void __cdecl make_gift(int faction_id, int faction_id_2);
int __cdecl dont_withdrawal(int faction_id, int faction_id_2);
int __cdecl do_withdrawal(int faction_id, int faction_id_2);
int __cdecl demand_withdrawal(int faction_id, int faction_id_2);
void __cdecl threaten(int faction_id, int faction_id_2);
int __cdecl suggest_plan(int faction_id, int faction_id_2);
int __cdecl attack_from(int base_id, int faction_id);
void __cdecl battle_routine(int base_id);
void __cdecl battle_plans(int faction_id, int faction_id_2);
void __cdecl call_off_vendetta(int faction_id, int faction_id_2);
int __cdecl value_of_base(int base_id, int faction_id, int faction_id_2, int low_price, int high_price);
void __cdecl give_a_base(int base_id, int faction_id);
int __cdecl base_swap(int faction_id, int faction_id_2);
int __cdecl mod_base_swap(int faction_id, int faction_id_2);

