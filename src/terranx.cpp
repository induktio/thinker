
#include "terranx.h"


const char** tx_version = (const char**)0x691870;
const char** tx_date = (const char**)0x691874;

byte* tx_tech_discovered = (byte*)0x9A6670;
int* tx_secret_projects = (int*)0x9A6514;
int* tx_cost_ratios = (int*)0x689378;
short (*tx_faction_rankings)[8] = (short (*)[8])(0x9A68AC);
MetaFaction* tx_metafactions = (MetaFaction*)0x946A50;
Faction* tx_factions = (Faction*)0x96C9E0;
BASE* tx_bases = (BASE*)0x97D040;
UNIT* tx_units = (UNIT*)0x9AB868;
VEH* tx_vehicles = (VEH*)0x952828;
MAP** tx_map_ptr = (MAP**)0x94A30C;

R_Basic* tx_basic = (R_Basic*)0x949738;
R_Tech* tx_techs = (R_Tech*)0x94F358;
R_Social* tx_social = (R_Social*)0x94B000;
R_Facility* tx_facility = (R_Facility*)0x9A4B68;
R_Ability* tx_ability = (R_Ability*)0x9AB538;
R_Chassis* tx_chassis = (R_Chassis*)0x94A330;
R_Citizen* tx_citizen = (R_Citizen*)0x946020;
R_Defense* tx_defense = (R_Defense*)0x94F278;
R_Reactor* tx_reactor = (R_Reactor*)0x9527F8;
R_Resource* tx_resource = (R_Resource*)0x945F50;
R_Terraform* tx_terraform = (R_Terraform*)0x691878;
R_Weapon* tx_weapon = (R_Weapon*)0x94AE60;


fp_7intstr* tx_propose_proto = (fp_7intstr*)0x580860;
fp_4int* tx_veh_init = (fp_4int*)0x5C03D0;
fp_1int* tx_veh_skip = (fp_1int*)0x5C1D20;
fp_2int* tx_veh_at = (fp_2int*)0x5BFE90;
fp_2int* tx_veh_speed = (fp_2int*)0x5C1540;
fp_3int* tx_zoc_any = (fp_3int*)0x5C89F0;
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
fp_1int* tx_action_staple = (fp_1int*)0x4CA7F0;

