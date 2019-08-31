
#include "terranx.h"


const char** tx_version = (const char**)0x691870;
const char** tx_date = (const char**)0x691874;

int*  tx_current_base_id = (int*)0x689370;
BASE* tx_current_base_ptr = (BASE*)0x90EA30;
int* tx_game_rules = (int*)0x9A64C0;
int* tx_scen_rules = (int*)0x9A649C;
int* tx_diff_level = (int*)0x9A64C4;
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
int* tx_un_charter_repeals = (int*)0x9A6638;
int* tx_un_charter_reinstates = (int*)0x9A663C;
int* tx_cost_ratios = (int*)0x689378;

byte* tx_tech_discovered = (byte*)0x9A6670;
int* tx_secret_projects = (int*)0x9A6514;
FactMeta* tx_factions_meta = (FactMeta *)0x946A50;
Faction* tx_factions = (Faction *)0x96C9E0;
BASE* tx_bases = (BASE *)0x97D040;
UNIT* tx_units = (UNIT *)0x9AB868;
VEH* tx_vehicles = (VEH *)0x952828;
MAP** tx_map_ptr = (MAP**)0x94A30C;

R_Basic* tx_basic = (R_Basic *)0x949738;
R_Tech* tx_techs = (R_Tech *)0x94F358;
R_Social* tx_social = (R_Social *)0x94B000;
R_Facility* tx_facility = (R_Facility *)0x9A4B68;
R_Ability* tx_ability = (R_Ability *)0x9AB538;
R_Chassis* tx_chassis = (R_Chassis *)0x94A330;
R_Citizen* tx_citizen = (R_Citizen *)0x946020;
R_Defense* tx_defense = (R_Defense *)0x94F278;
R_Reactor* tx_reactor = (R_Reactor *)0x9527F8;
R_Resource* tx_resource = (R_Resource *)0x945F50;
R_Terraform* tx_terraform = (R_Terraform *)0x691878;
R_Weapon* tx_weapon = (R_Weapon *)0x94AE60;


fp_7intstr* tx_propose_proto = (fp_7intstr*)0x580860;
fp_4int* tx_veh_init = (fp_4int*)0x5C03D0;
fp_1int* tx_veh_skip = (fp_1int*)0x5C1D20;
fp_2int* tx_veh_at = (fp_2int*)0x5BFE90;
fp_2int* tx_veh_speed = (fp_2int*)0x5C1540;
fp_3int* tx_zoc_any = (fp_3int*)0x5C89F0;
fp_3int* tx_best_defender = (fp_3int*)0x5044D0;
fp_5int* tx_battle_compute = (fp_5int*)0x501DA0;
fp_1int* tx_enemy_move = (fp_1int*)0x56B5B0;
fp_1int* tx_monolith = (fp_1int*)0x57A050;
fp_2int* tx_action_build = (fp_2int*)0x4C96E0;
fp_3int* tx_action_terraform = (fp_3int*)0x4C9B00;
fp_3int* tx_terraform_cost = (fp_3int*)0x4C9420;
fp_2int* tx_bonus_at = (fp_2int*)0x592030;
fp_2int* tx_goody_at = (fp_2int*)0x592140;
fp_3int* tx_cost_factor = (fp_3int*)0x4E4430;
fp_3int* tx_site_set = (fp_3int*)0x591B50;
fp_3int* tx_world_site = (fp_3int*)0x5C4FD0;
fp_1int* tx_set_base = (fp_1int*)0x4E39D0;
fp_1int* tx_base_compute = (fp_1int*)0x4EC3B0;
fp_4int* tx_base_prod_choices = (fp_4int*)0x4F81A0;
fp_void* tx_turn_upkeep = (fp_void*)0x5258C0;
fp_1int* tx_faction_upkeep = (fp_1int*)0x527290;
fp_3int* tx_tech_val = (fp_3int*)0x5BCBE0;
fp_5int* tx_crop_yield = (fp_5int*)0x4E6E50;
fp_6int* tx_social_ai = (fp_6int*)0x5B4790;
fp_1int* tx_social_set = (fp_1int*)0x5B4600;
fp_1int* tx_pop_goal = (fp_1int*)0x4EF090;
fp_1int* tx_consider_designs = (fp_1int*)0x581260;
fp_1int* tx_action_staple = (fp_1int*)0x4CA7F0;
fp_1int* tx_tech_rate = (fp_1int*)0x5BE6B0;
fp_1int* tx_tech_selection = (fp_1int*)0x5BE380;

