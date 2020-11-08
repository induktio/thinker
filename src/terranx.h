#pragma once

typedef unsigned char byte;

#include "terranx_enums.h"
#include "terranx_types.h"

extern const char** tx_version;
extern const char** tx_date;

extern byte* tx_tech_discovered;
extern int* tx_secret_projects;
extern int* tx_cost_ratios;
extern short (*tx_faction_rankings)[8];
extern MetaFaction* tx_metafactions;
extern Faction* tx_factions;
extern BASE* tx_bases;
extern UNIT* tx_units;
extern VEH* tx_vehicles;
extern MAP** tx_map_ptr;

extern R_Basic* tx_basic;
extern R_Tech* tx_techs;
extern R_Social* tx_social;
extern R_Facility* tx_facility;
extern R_Ability* tx_ability;
extern R_Chassis* tx_chassis;
extern R_Citizen* tx_citizen;
extern R_Defense* tx_defense;
extern R_Reactor* tx_reactor;
extern R_Resource* tx_resource;
extern R_Terraform* tx_terraform;
extern R_Weapon* tx_weapon;

