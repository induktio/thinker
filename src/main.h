/*
 * Thinker - AI improvement mod for Sid Meier's Alpha Centauri.
 * https://github.com/induktio/thinker/
 *
 * Thinker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Thinker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Thinker.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#ifdef BUILD_REL
    #define MOD_VERSION "Thinker Mod v2.2"
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
#include <psapi.h>
#include <algorithm>
#include <set>
#include <list>
#include <string>

#define min(x, y) std::min(x, y)
#define max(x, y) std::max(x, y)
#define COMBAT 0
#define SYNC 0
#define NO_SYNC 1
#define ATT false
#define DEF true

const int MaxMapW = 512;
const int MaxMapH = 256;
const int MaxRegionNum = 128;

const int MaxPlayerNum = 8;
const int MaxGoalsNum = 75;
const int MaxSitesNum = 25;
const int MaxBaseNum = 512;
const int MaxVehNum = 2048;
const int MaxProtoNum = 512;
const int MaxProtoFactionNum = 64;

const int MaxTechnologyNum = 89;
const int MaxChassisNum = 9;
const int MaxWeaponNum = 26;
const int MaxArmorNum = 14;
const int MaxReactorNum = 4;
const int MaxAbilityNum = 29;

const int MaxFacilityNum = 134; // 0 slot unused
const int MaxSecretProjectNum = 64;
const int MaxSocialCatNum = 4;
const int MaxSocialModelNum = 4;
const int MaxSocialEffectNum = 11;


struct Config {
    // Set negative value to use the defaults from Alpha Centauri.ini.
    int directdraw = 0;
    int disable_opening_movie = 1;
    int cpu_idle_fix = 1;
    int smooth_scrolling = 1;
    int scroll_area = 40;
    int windowed = 0;
    int window_width = 1024;
    int window_height = 768;
    int smac_only = 0;
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
    float expansion_factor = 1;
    int limit_project_start = 3;
    int max_sat = 10;
    int faction_placement = 1;
    int nutrient_bonus = 1;
    int landmarks = 0xffff;
    int revised_tech_cost = 1;
    int auto_relocate_hq = 1;
    int ignore_reactor_power = 0;
    int territory_border_fix = 1;
    int eco_damage_fix = 1;
    int clean_minerals = 16;
    int collateral_damage_value = 3;
    int disable_planetpearls = 0;
    int disable_aquatic_bonus_minerals = 0;
    int patch_content_pop = 0;
    int content_pop_player[6] = {6,5,4,3,2,1};
    int content_pop_computer[6] = {3,3,3,3,3,3};
    int debug_mode = DEBUG;
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

enum crawl_resource_types {
    RES_NONE = 0,
    RES_NUTRIENT = 1,
    RES_MINERAL = 2,
    RES_ENERGY = 3,
};

enum nodeset_types {
    NODE_CONVOY = 1,
    NODE_BOREHOLE = 2,
    NODE_NEED_FERRY = 3,
    NODE_RAISE_LAND = 4,
    NODE_GOAL_RAISE_LAND = 5,
};

struct Point {
    int x;
    int y;
};

struct PointComp {
    bool operator()(const Point& p1, const Point& p2) const
    {
        return p1.x < p2.x || (p1.x == p2.x && p1.y < p2.y);
    }
};

struct MapNode {
    int x;
    int y;
    int type;
};

struct NodeComp {
    bool operator()(const MapNode& p1, const MapNode& p2) const
    {
        return p1.x < p2.x || (p1.x == p2.x && (
            p1.y < p2.y || (p1.y == p2.y && (p1.type < p2.type))
        ));
    }
};

typedef std::set<MapNode,NodeComp> NodeSet;
typedef std::set<Point,PointComp> Points;
typedef std::list<Point> PointList;

#include "terranx.h"
#include "patch.h"
#include "gui.h"
#include "game.h"
#include "move.h"
#include "tech.h"
#include "engine.h"
#include "test.h"

extern FILE* debug_log;
extern Config conf;
extern AIPlans plans[8];
extern NodeSet mapnodes;

DLL_EXPORT int ThinkerDecide();
HOOK_API int mod_turn_upkeep();
HOOK_API int mod_base_production(int id, int v1, int v2, int v3);
HOOK_API int mod_social_ai(int faction, int v1, int v2, int v3, int v4, int v5);

int need_defense(int id);
int need_psych(int id);
int consider_hurry(int id);
int find_project(int id);
int find_facility(int id);
int select_prod(int id);
int find_proto(int base_id, int triad, int mode, bool defend);
int select_combat(int id, bool sea_base, bool build_ships, int probes, int def_land);
int opt_list_parse(int* ptr, char* buf, int len, int min_val);



