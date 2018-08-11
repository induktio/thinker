
#include "terranx.h"


char* tx_version = (char*)0x691B40;
char* tx_date = (char*)0x691B4C;

int*  tx_current_base_id = (int*)0x689370;
BASE* tx_current_base_ptr = (BASE*)0x90EA30;
int* tx_human_players = (int*)0x9A64E8;
int* tx_current_turn = (int*)0x9A64D4;
int* tx_active_faction = (int*)0x9A6820;
int* tx_total_num_bases = (int*)0x9A64CC;
int* tx_total_num_vehicles = (int*)0x9A64C8;
int* tx_random_seed = (int*)0x949878;
int* tx_map_toggle_flat = (int*)0x94988C;
int* tx_map_area_sq_root = (int*)0x949888;
int* tx_map_axis_x = (int*)0x949870;
int* tx_map_axis_y = (int*)0x949874;
int* tx_map_half_x = (int*)0x68FAF0;
int* tx_climate_future_change = (int*)0x9A67D8;

byte* tx_tech_discovered = (byte*)0x9A6670;
int* tx_secret_projects = (int*)0x9A6514;
FactMeta* tx_factions_meta = (FactMeta *)0x946FEC;
Faction* tx_factions = (Faction *)0x96C9E0;
BASE* tx_bases = (BASE *)0x97D040;
UNIT* tx_units = (UNIT *)0x9AB868;
VEH* tx_vehicles = (VEH *)0x952828;
MAP** tx_map_ptr = (MAP**)0x94A30C;

R_Basic* tx_basic = (R_Basic *)0x949738;
R_Tech* tx_techs = (R_Tech *)0x94F358;
R_Facility* tx_facilities = (R_Facility *)0x9A4B68;
R_Ability* tx_ability = (R_Ability *)0x9AB538;
R_Chassis* tx_chassis = (R_Chassis *)0x94A330;
R_Citizen* tx_citizen = (R_Citizen *)0x946020;
R_Defense* tx_defense = (R_Defense *)0x94F278;
R_Reactor* tx_reactor = (R_Reactor *)0x9527F8;
R_Terraform* tx_terraform = (R_Terraform *)0x691878;
R_Weapon* tx_weapon = (R_Weapon *)0x94AE60;


fp_7intstr* tx_propose_proto = (fp_7intstr*)0x580860;
fp_4int* tx_veh_init = (fp_4int*)0x5C03D0;

fp_1int* tx_enemy_move = (fp_1int*)0x56B5B0;
fp_2int* tx_action_build = (fp_2int*)0x4C96E0;
fp_3int* tx_action_terraform = (fp_3int*)0x4C9B00;
fp_2int* tx_bonus_at = (fp_2int*)0x592030;
fp_2int* tx_can_convoy = (fp_2int*)0x564D90;
fp_4int* tx_contiguous = (fp_4int*)0x53A780;
fp_4int* tx_base_prod_choices = (fp_4int*)0x4F81A0;
fp_3int* tx_cost_factor = (fp_3int*)0x4E4430;


