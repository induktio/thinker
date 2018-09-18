
#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef BUILD_REL
    #define VERSION "Thinker Mod v0.6"
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
#endif

#ifdef __GNUC__
    #define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
    #pragma GCC diagnostic ignored "-Wchar-subscripts"
#else
    #define UNUSED(x) UNUSED_ ## x
#endif

#ifdef BUILD_DLL
    #define DLL_EXPORT extern "C" __declspec(dllexport)
#else
    #define DLL_EXPORT extern "C" __declspec(dllimport)
#endif

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <math.h>
#include <set>
#include <limits>
#include "inih/ini.h"
#include "terranx.h"

DLL_EXPORT int ThinkerDecide(int mode, int id, int val1, int val2);

extern FILE* debug_log;

static_assert(sizeof(struct Faction) == 8396, "");
static_assert(sizeof(struct BASE) == 308, "");
static_assert(sizeof(struct VEH) == 52, "");
static_assert(sizeof(struct MAP) == 44, "");

#define QSIZE 512
#define BASES 512
#define COMBAT 0
#define MAX_SAT 8
#define SYNC 0
#define NO_SYNC 1
#define LAND_ONLY 0
#define WATER_ONLY 1
#define RES_NONE 0
#define RES_NUTRIENT 1
#define RES_MINERAL 2
#define RES_ENERGY 3
#define STATUS_CONVOY 3
#define STATUS_GOTO 24

#define BASE_DISALLOWED (TERRA_BASE_IN_TILE | TERRA_MONOLITH | TERRA_FUNGUS | TERRA_THERMAL_BORE)
#define ALTITUDE_MIN_LAND 60

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
};

int turn_upkeep();
int select_prod(int id);
int consider_base(int id);
int consider_convoy(int id);
int terraform_action(int veh, int action, int flag);
int tech_value(int tech, int fac, int value);
void print_veh(int id);

#endif // __MAIN_H__




