#pragma once

#include <stdint.h>
typedef uint8_t byte;
typedef struct char256 { char str[256]; } char256;

#include "terranx_enums.h"
#include "terranx_types.h"

typedef int (__cdecl *fp_void)();
typedef int (__cdecl *fp_1int)(int);
typedef int (__cdecl *fp_2int)(int, int);
typedef int (__cdecl *fp_3int)(int, int, int);
typedef int (__cdecl *fp_4int)(int, int, int, int);
typedef int (__cdecl *fp_5int)(int, int, int, int, int);
typedef int (__cdecl *fp_6int)(int, int, int, int, int, int);
typedef int (__cdecl *fp_7int)(int, int, int, int, int, int, int);


/* Temporarily disable warnings for thiscall parameter type. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

typedef int (__thiscall *tc_1int)(int);
typedef int (__thiscall *tc_2int)(int, int);
typedef int (__thiscall *tc_3int)(int, int, int);
typedef int (__thiscall *tc_4int)(int, int, int, int);
typedef int (__thiscall *tc_5int)(int, int, int, int, int);
typedef int (__thiscall *tc_6int)(int, int, int, int, int, int);
typedef int (__thiscall *tc_7int)(int, int, int, int, int, int, int);

#pragma GCC diagnostic pop

/*
pad_1 in MFaction is reserved for faction specific variables.
pad_2 in MFaction[0] is reserved for global game state.
*/
struct ThinkerData {
    uint64_t reserved;
    uint64_t game_time_spent;
    char build_date[12];
    int8_t padding[148];
};

static_assert(sizeof(struct CRules) == 308, "");
static_assert(sizeof(struct CSocial) == 212, "");
static_assert(sizeof(struct CFacility) == 48, "");
static_assert(sizeof(struct CTech) == 44, "");
static_assert(sizeof(struct MFaction) == 1436, "");
static_assert(sizeof(struct Faction) == 8396, "");
static_assert(sizeof(struct BASE) == 308, "");
static_assert(sizeof(struct UNIT) == 52, "");
static_assert(sizeof(struct VEH) == 52, "");
static_assert(sizeof(struct MAP) == 44, "");
static_assert(sizeof(struct ThinkerData) == 176, "");

/*
Thinker functions that are replacements to the SMACX binary versions 
should be prefixed with 'mod_' if their equivalent is also listed here.
*/

extern const char** engine_version;
extern const char** engine_date;
extern const char* last_save_path;
extern BASE** current_base_ptr;
extern int* current_base_id;
extern int* game_preferences;
extern int* game_more_preferences;
extern int* game_warnings;
extern int* game_state;
extern int* game_rules;
extern int* diff_level;
extern int* smacx_enabled;
extern int* human_players;
extern int* current_turn;
extern int* active_faction;
extern int* total_num_bases;
extern int* total_num_vehicles;
extern int* map_random_seed;
extern int* map_toggle_flat;
extern int* map_area_tiles;
extern int* map_area_sq_root;
extern int* map_axis_x;
extern int* map_axis_y;
extern int* map_half_x;
extern int* MapSizePlanet;
extern int* MapOceanCoverage;
extern int* MapLandCoverage; // 2 - MapOceanCoverage
extern int* MapErosiveForces;
extern int* MapPlanetaryOrbit; // affects temp
extern int* MapCloudCover; // affects rainfall, rivers
extern int* MapNativeLifeForms;
extern int* MapLandmarkCount;
extern int* climate_future_change;
extern int* un_charter_repeals;
extern int* un_charter_reinstates;
extern int* gender_default;
extern int* plurality_default;
extern int* multiplayer_active;
extern int* pbem_active;
extern int* sunspot_duration;
extern int* current_player_faction;
extern int* diplo_second_faction;
extern int* diplo_third_faction;
extern int* diplo_tech_faction;
extern int* diplo_current_friction;
extern int* reportwin_opponent_faction;
extern int* base_find_dist;
extern int* veh_attack_flags;
extern int* game_not_started;
extern int* screen_width;
extern int* screen_height;
extern char256* ParseStrBuffer;
extern int* ParseNumTable;
extern int* ParseStrPlurality;
extern int* ParseStrGender;

// TODO: rename dwords
extern int* dword_915620;
extern int* dword_9B2068;
extern int* dword_9B7AE4;
extern int* dword_93A934;
extern int* dword_945B18;
extern int* dword_945B1C;

extern MFaction* MFactions;
extern Faction* Factions;
extern BASE* Bases;
extern UNIT* Units;
extern VEH* Vehicles;
extern VEH* Vehs;
extern MAP** MapPtr;
extern Continent* Continents; // [128]
extern Landmark *Landmarks; // [64]
extern uint8_t* TechOwners;
extern int* SecretProjects;
extern int* CostRatios;
extern int16_t (*FactionRankings)[8]; // Per turn might ratings for each faction
extern ThinkerData* ThinkerVars;

// Rules parsed from alphax.txt
extern CRules* Rules;
extern CTech* Tech;
extern CSocial* Social;
extern CFacility* Facility;
extern CAbility* Ability;
extern CChassis* Chassis;
extern CCitizen* Citizen;
extern CArmor* Armor;
extern CReactor* Reactor;
extern CResource* Resource;
extern CTerraform* Terraform;
extern CWeapon* Weapon;
extern CNatural* Natural;

typedef int(__cdecl *Fbattle_fight_1)(int veh_id, int offset, bool use_table_offset, int v1, int v2);
typedef int(__cdecl *Fpropose_proto)(int faction, int chassis, int weapon, int armor, 
    int abilities, int reactor, int ai_plan, const char* name);
typedef int(__cdecl *Faction_airdrop)(int veh_id, int x, int y, int veh_attack_flags);
typedef int(__cdecl *Faction_destroy)(int veh_id, int flags, int x, int y);
typedef int(__cdecl *Fhas_abil)(int unit_id, int ability_flag);
typedef int(__cdecl *Fparse_says)(int index, const char* text, int v1, int v2);
typedef int(__cdecl *Fpopp)(const char* file_name, const char* label, int v1, 
    const char* pcx_file_name, int v2);
typedef int(__cdecl *Fhex_cost)(int unit_id, int faction, int x1, int y1, int x2, int y2, int a7);
typedef void(__cdecl *Fname_base)(int faction, char* name, int flags, int water);
typedef int(__cdecl *Fveh_cost)(int item_id, int base_id, int* ptr);

extern Fbattle_fight_1 battle_fight_1;
extern Fpropose_proto propose_proto;
extern Faction_airdrop action_airdrop;
extern Faction_destroy action_destroy;
extern Fhas_abil has_abil;
extern Fparse_says parse_says;
extern Fpopp popp;
extern Fhex_cost hex_cost;
extern Fname_base name_base;
extern Fveh_cost veh_cost;

extern fp_4int veh_init;
extern fp_1int veh_skip;
extern fp_2int veh_at;
extern fp_2int veh_speed;
extern fp_1int veh_kill;
extern fp_1int veh_wake;
extern fp_1int stack_fix;
extern fp_2int stack_veh;
extern fp_3int zoc_any;
extern fp_1int monolith;
extern fp_2int action_build;
extern fp_3int action_terraform;
extern fp_3int terraform_cost;
extern fp_2int bonus_at;
extern fp_2int goody_at;
extern fp_3int cost_factor;
extern fp_3int site_set;
extern fp_3int world_site;
extern fp_1int set_base;
extern fp_1int base_compute;
extern fp_4int base_prod_choices;
extern fp_void turn_upkeep;
extern fp_1int faction_upkeep;
extern fp_1int action_staple;
extern fp_1int social_upkeep;
extern fp_1int repair_phase;
extern fp_1int production_phase;
extern fp_1int allocate_energy;
extern fp_1int enemy_diplomacy;
extern fp_1int enemy_strategy;
extern fp_1int corner_market;
extern fp_1int call_council;
extern fp_3int setup_player;
extern fp_2int eliminate_player;
extern fp_2int can_call_council;
extern fp_3int wants_to_attack;
extern fp_void do_all_non_input;
extern fp_void auto_save;
extern fp_2int parse_num;
extern fp_3int capture_base;
extern fp_1int base_kill;
extern fp_5int crop_yield;
extern fp_6int base_draw;
extern fp_6int base_find3;
extern fp_3int draw_tile;
extern tc_2int font_width;
extern tc_4int buffer_box;
extern tc_3int buffer_fill3;
extern tc_5int buffer_write_l;
extern fp_6int social_ai;
extern fp_1int social_set;
extern fp_1int pop_goal;
extern fp_1int consider_designs;
extern fp_3int tech_val;
extern fp_1int tech_rate;
extern fp_1int tech_selection;
extern fp_1int enemy_move;
extern fp_3int best_defender;
extern fp_5int battle_compute;
extern fp_6int battle_kill;
extern fp_7int battle_fight_2;
extern fp_void draw_cursor;
extern fp_1int draw_map;


