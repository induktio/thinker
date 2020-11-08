
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

