#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef BUILD_REL
    #define VERSION "Thinker Mod v0.7"
#else
    #define VERSION "Thinker Mod develop build"
#endif

#ifdef BUILD_DEBUG
    #define DEBUG 1
    #define debuglog(...) fprintf(debug_log, __VA_ARGS__);
#else
    #define DEBUG 0
    #define NDEBUG /* Disable assertions */
    #define debuglog(...) /* Nothing */
    #ifdef __GNUC__
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
#include <math.h>
#include <algorithm>
#include <set>
#include "terranx.h"

static_assert(sizeof(struct R_Basic) == 308, "");
static_assert(sizeof(struct R_Social) == 212, "");
static_assert(sizeof(struct FactMeta) == 1436, "");
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
#define MAX_SAT 8
#define SYNC 0
#define NO_SYNC 1
#define LAND_ONLY 0
#define WATER_ONLY 1
#define NEAR_ROADS 2
#define RES_NONE 0
#define RES_NUTRIENT 1
#define RES_MINERAL 2
#define RES_ENERGY 3
#define STATUS_IDLE 0
#define STATUS_CONVOY 3
#define STATUS_GOTO 24
#define STATUS_ROAD_TO 27
#define ALTITUDE_MIN_LAND 60
#define ATT false
#define DEF true

struct Config {
    int free_formers;
    int satellites_nutrient;
    int satellites_mineral;
    int satellites_energy;
    int design_units;
    int factions_enabled;
    int terraform_ai;
    int production_ai;
    int tech_balance;
    int load_expansion;
    int faction_placement;
    int landmarks;
};

struct AIPlans {
    int diplo_flags;
    int proj_limit;
    int psi_score;
    int keep_fungus;
    int plant_fungus;
    float enemy_range;
};

extern FILE* debug_log;
extern AIPlans plans[8];
extern std::set<std::pair<int,int>> convoys;
extern std::set<std::pair<int,int>> boreholes;
extern std::set<std::pair<int,int>> needferry;

DLL_EXPORT int ThinkerDecide();
HOOK_API int turn_upkeep();
HOOK_API int faction_upkeep(int fac);
HOOK_API int enemy_move(int id);
HOOK_API int base_production(int id, int v1, int v2, int v3);
HOOK_API int tech_value(int tech, int fac, int flag);
HOOK_API int social_ai(int fac, int v1, int v2, int v3, int v4, int v5);

int need_psych(int id);
int select_prod(int id);
int find_facility(int base_id);
int find_project(int base_id);

#endif // __MAIN_H__




