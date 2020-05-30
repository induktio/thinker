#pragma once

#ifdef BUILD_REL
    #define MOD_VERSION "Thinker Mod v1.0"
#else
    #define MOD_VERSION "Thinker Mod develop build"
#endif

#ifdef BUILD_DEBUG
    #define MOD_DATE __DATE__ " " __TIME__
    #define DEBUG 1
    #define debug(...) fprintf(debug_log, __VA_ARGS__);
#else
    #define MOD_DATE __DATE__
    #define DEBUG 0
    #define NDEBUG /* Disable assertions */
    #define debug(...) /* Nothing */
    #ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    #endif
#endif

#ifdef __GNUC__
    #define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
    #pragma GCC diagnostic ignored "-Wchar-subscripts"
#else
    #define UNUSED(x) UNUSED_ ## x
#endif

#define DLL_EXPORT extern "C" __declspec(dllexport)
#define HOOK_API extern "C"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <algorithm>
#include <set>
#include "terranx.h"

typedef std::set<std::pair<int,int>> Points;
#define mp(x, y) std::make_pair(x, y)
#define min(x, y) std::min(x, y)
#define max(x, y) std::max(x, y)

static_assert(sizeof(struct R_Basic) == 308, "");
static_assert(sizeof(struct R_Social) == 212, "");
static_assert(sizeof(struct R_Facility) == 48, "");
static_assert(sizeof(struct R_Tech) == 44, "");
static_assert(sizeof(struct MetaFaction) == 1436, "");
static_assert(sizeof(struct Faction) == 8396, "");
static_assert(sizeof(struct BASE) == 308, "");
static_assert(sizeof(struct UNIT) == 52, "");
static_assert(sizeof(struct VEH) == 52, "");
static_assert(sizeof(struct MAP) == 44, "");

#define MAPSZ 256
#define QSIZE 512
#define BASES 512
#define UNITS 2048
#define COMBAT 0
#define SYNC 0
#define NO_SYNC 1
#define NEAR_ROADS 3
#define TERRITORY_LAND 4
#define RES_NONE 0
#define RES_NUTRIENT 1
#define RES_MINERAL 2
#define RES_ENERGY 3
#define ATT false
#define DEF true

struct Config {
    int free_formers = 0;
    int free_colony_pods = 0;
    int satellites_nutrient = 0;
    int satellites_mineral = 0;
    int satellites_energy = 0;
    int design_units = 1;
    int factions_enabled = 7;
    int social_ai = 1;
    int tech_balance = 0;
    int hurry_items = 1;
    double expansion_factor = 1;
    int limit_project_start = 3;
    int max_sat = 10;
    int smac_only = 0;
    int faction_placement = 1;
    int nutrient_bonus = 1;
    int landmarks = 0xffff;
    int revised_tech_cost = 1;
    int auto_relocate_hq = 1;
    int eco_damage_fix = 1;
    int collateral_damage_value = 3;
    int disable_planetpearls = 0;
    int disable_aquatic_bonus_minerals = 0;
    int patch_content_pop = 0;
    int content_pop_player[6] = {6,5,4,3,2,1};
    int content_pop_computer[6] = {3,3,3,3,3,3};
};

/*
    AIPlans contains several general purpose variables for AI decision-making
    that are recalculated each turn. These values are not stored in the save game.
*/
struct AIPlans {
    int diplo_flags = 0;
    /*
    Amount of minerals a base needs to produce before it is allowed to build secret projects.
    All faction-owned bases are ranked each turn based on the surplus mineral production,
    and only the top third are selected for project building.
    */
    int proj_limit = 5;
    /*
    PSI combat units are only selected for production if this score is higher than zero.
    Higher values will make the prototype picker choose these units more often.
    */
    int psi_score = 0;
    int need_police = 1;
    int keep_fungus = 0;
    int plant_fungus = 0;
    /*
    Number of our bases captured by another faction we're currently at war with.
    Important heuristic in threat calculation.
    */
    int enemy_bases = 0;
};

extern FILE* debug_log;
extern Config conf;
extern AIPlans plans[8];
extern Points convoys;
extern Points boreholes;
extern Points needferry;

DLL_EXPORT int ThinkerDecide();
HOOK_API int turn_upkeep();
HOOK_API int base_production(int id, int v1, int v2, int v3);
HOOK_API int social_ai(int faction, int v1, int v2, int v3, int v4, int v5);

int need_defense(int id);
int need_psych(int id);
int consider_hurry(int id);
int find_project(int id);
int find_facility(int id);
int select_prod(int id);
int find_proto(int base_id, int triad, int mode, bool defend);
int select_combat(int id, bool sea_base, bool build_ships, int probes, int def_land);
int opt_list_parse(int* ptr, char* buf, int len, int min_val);




