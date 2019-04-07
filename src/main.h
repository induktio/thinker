#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef BUILD_REL
    #define MOD_VERSION "Thinker Mod v0.9"
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
#define SYNC 0
#define NO_SYNC 1
#define NEAR_ROADS 3
#define RES_NONE 0
#define RES_NUTRIENT 1
#define RES_MINERAL 2
#define RES_ENERGY 3
#define STATUS_IDLE 0
#define STATUS_CONVOY 3
#define STATUS_GOTO 24
#define STATUS_ROAD_TO 27
#define ATT false
#define DEF true

struct Config {
    int free_formers;
    int satellites_nutrient;
    int satellites_mineral;
    int satellites_energy;
    int design_units;
    int factions_enabled;
    int hurry_items;
    int social_ai;
    int tech_balance;
    int max_sat;
    int smac_only;
    int faction_placement;
    int nutrient_bonus;
    int landmarks;
};

struct AIPlans {
    int diplo_flags;
    int proj_limit;
    int psi_score;
    int keep_fungus;
    int plant_fungus;
    int enemy_bases;
    double enemy_range;
};

extern FILE* debug_log;
extern Config conf;
extern AIPlans plans[8];
extern Points convoys;
extern Points boreholes;
extern Points needferry;

DLL_EXPORT int ThinkerDecide();
HOOK_API int turn_upkeep();
HOOK_API int faction_upkeep(int fac);
HOOK_API int base_production(int id, int v1, int v2, int v3);
HOOK_API int tech_value(int tech, int fac, int flag);
HOOK_API int social_ai(int fac, int v1, int v2, int v3, int v4, int v5);

int need_psych(int id);
int consider_hurry(int id);
int select_prod(int id);
int find_facility(int base_id);
int find_project(int base_id);
int select_combat(int, bool, bool, int, int);

#endif // __MAIN_H__




