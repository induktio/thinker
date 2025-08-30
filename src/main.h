/*
 * Thinker - AI improvement mod for Sid Meier's Alpha Centauri.
 * https://github.com/induktio/thinker/
 *
 * Thinker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) version 3 of the GPL.
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
    #define MOD_VERSION "Thinker Mod v5.1"
#else
    #define MOD_VERSION "Thinker Mod develop build"
#endif

#ifdef BUILD_DEBUG
    #define MOD_DATE __DATE__ " " __TIME__
    #define DEBUG 1
    #define debug(...) fprintf(debug_log, __VA_ARGS__);
    #define debugw(...) { fprintf(debug_log, __VA_ARGS__); \
        fflush(debug_log); }
    #define debug_ver(...) if (conf.debug_verbose) { fprintf(debug_log, __VA_ARGS__); }
    #define flushlog() fflush(debug_log);
#else
    #define MOD_DATE __DATE__
    #define DEBUG 0
    #define NDEBUG /* Disable assertions */
    #define debug(...) /* Nothing */
    #define debugw(...) /* Nothing */
    #define debug_ver(...) /* Nothing */
    #define flushlog()
    #ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    #endif
#endif

#ifdef __GNUC__
    #define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
    #pragma GCC diagnostic ignored "-Wchar-subscripts"
    #pragma GCC diagnostic ignored "-Wattributes"
    #pragma GCC diagnostic error "-Wreturn-type"
#else
    #define UNUSED(x) UNUSED_ ## x
#endif

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <windows.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <psapi.h>
#include <set>
#include <list>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>

#define DLL_EXPORT extern "C" __declspec(dllexport)
#define GameAppName "Alpha Centauri"
#define GameIniFile ".\\Alpha Centauri.ini"
#define ModAppName "thinker"
#define ModIniFile ".\\thinker.ini"

#ifdef BUILD_DEBUG
#ifdef assert
#undef assert
#define assert(_Expression) \
((!!(_Expression)) \
|| (fprintf(debug_log, "Assertion Failed: %s %s %d\n", #_Expression, __FILE__, __LINE__) \
&& (_assert(#_Expression, __FILE__, __LINE__), 0)))
#endif
#endif

const bool DEF = true;
const bool ATT = false;

const int MaxMapDist = 1024;
const int MaxNaturalNum = 16;
const int MaxLandmarkNum = 64;
const int MaxRegionNum = 128;
const int MaxRegionLandNum = 64;
const int RegionBounds = 63;

const int MaxDiffNum = 6;
const int MaxPlayerNum = 8;
const int MaxGoalsNum = 75;
const int MaxSitesNum = 25;
const int MaxBaseNum = 512;
const int MaxVehNum = 2048;
const int MaxProtoNum = 512;
const int MaxProtoFactionNum = 64;
const int MaxBaseNameLen = 25;
const int MaxProtoNameLen = 32;
const int MaxBasePopSize = 127;
const int MaxBaseSpecNum = 16;

const int MaxTechnologyNum = 89;
const int MaxChassisNum = 9;
const int MaxWeaponNum = 26;
const int MaxArmorNum = 14;
const int MaxReactorNum = 4;
const int MaxAbilityNum = 29;
const int MaxMoraleNum = 7;
const int MaxDefenseModeNum = 3;
const int MaxOffenseModeNum = 3;
const int MaxOrderNum = 30;
const int MaxPlanNum = 15;
const int MaxTriadNum = 3;

const int MaxResourceInfoNum = 9;
const int MaxTimeControlNum = 6;
const int MaxCompassNum = 8;
const int MaxResourceNum = 4;
const int MaxEnergyNum = 3;

const int MaxMandateNum = 4;
const int MaxFacilityNum = 64; // 0 slot unused
const int MaxSecretProjectNum = 64;
const int MaxTerrainNum = 20;
const int MaxSocialCatNum = 4;
const int MaxSocialModelNum = 4;
const int MaxSocialEffectNum = 11;
const int MaxSpecialistNum = 7;
const int MaxCitizenNum = 10;
const int MaxMoodNum = 9;
const int MaxReputeNum = 8;
const int MaxMightNum = 7;
const int MaxBonusNum = 8;
const int MaxBonusNameNum = 41;
const int MaxProposalNum = 11;

const int StrBufLen = 256;
const int LineBufLen = 128;
const int MaxEnemyRange = 50;

enum VideoMode {
    VM_Native = 0,
    VM_Custom = 1,
    VM_Window = 2,
};

struct LMConfig {
    int crater = 1;
    int volcano = 1;
    int jungle = 1;
    int uranium = 1;
    int sargasso = 1;
    int ruins = 1;
    int dunes = 1;
    int fresh = 1;
    int mesa = 1;
    int canyon = 0;
    int geothermal = 1;
    int ridge = 1;
    int borehole = 1;
    int nexus = 1;
    int unity = 1;
    int fossil = 1;
};

/*
Config parsed from thinker.ini. Alpha Centauri.ini related options
can be set negative values to use the defaults from Alpha Centauri.ini.
*/
struct Config {
    int video_mode = VM_Native;
    int window_width = 1024;
    int window_height = 768;
    int minimised = 0; // internal variable
    int video_player = 2; // internal variable
    int playing_movie = 0;  // internal variable
    int screen_width = 1024; // internal variable
    int screen_height = 768; // internal variable
    int directdraw = 0;
    int disable_opening_movie = -1;
    int smac_only = 0;
    int smooth_scrolling = 0;
    int scroll_area = 40;
    int auto_minimise = 0;
    int render_base_info = 1;
    int render_high_detail = 1; // unlisted option
    int autosave_interval = 1;
    int warn_on_former_replace = 1;
    int manage_player_bases = 0;
    int manage_player_units = 0;
    int render_probe_labels = 1;
    int foreign_treaty_popup = 0;
    int editor_free_units = 1;
    int new_base_names = 1;
    int new_unit_names = 1;
    int spawn_free_units[9] = {0,0,1,0,0,1,1,0,1};
    int player_colony_pods = 0;
    int computer_colony_pods = 0;
    int player_formers = 0;
    int computer_formers = 0;
    int player_satellites[3] = {0,0,0};
    int computer_satellites[3] = {0,0,0};
    int design_units = 1;
    int factions_enabled = 7;
    int social_ai = 1;
    int social_ai_bias = 10;
    int tech_balance = 0;
    int base_hurry = 0;
    int base_spacing = 3;
    int base_nearby_limit = -1;
    int expansion_limit = 100;
    int expansion_autoscale = 0;
    int limit_project_start = 0;
    int max_satellites = 20;
    int new_world_builder = 1;
    int world_sea_levels[3] = {46,58,70};
    int world_hills_mod = 40;
    int world_ocean_mod = 40;
    int world_islands_mod = 16;
    int world_continents = 0;
    int world_polar_caps = 1;
    int world_mirror_x = 0;
    int world_mirror_y = 0;
    int modified_landmarks = 0;
    int time_warp_mod = 1;
    int time_warp_techs = 5;
    int time_warp_projects = 1;
    int time_warp_start_turn = 40;
    int faction_placement = 1;
    int nutrient_bonus = 0;
    int rare_supply_pods = 0;
    int simple_cost_factor = 0;
    int revised_tech_cost = 1;
    int tech_cost_factor[MaxDiffNum] = {124,116,108,100,84,76};
    int tech_rate_modifier = 100; // internal variable
    int tech_stagnate_rate = 200;
    int fast_fungus_movement = 0;
    int magtube_movement_rate = 0;
    int road_movement_rate = 1; // internal variable
    int max_movement_rate = 255; // internal variable
    int chopper_attack_rate = 1;
    int base_psych = 1;
    int nerve_staple = 2;
    int nerve_staple_mod = -10;
    int delay_drone_riots = 0;
    int activate_skipped_units = 1; // unlisted option
    int counter_espionage = 0;
    int ignore_reactor_power = 0;
    int long_range_artillery = 0;
    int modify_upgrade_cost = 0;
    int modify_unit_support = 0;
    int modify_unit_limit = 0;
    int max_veh_num = MaxVehNum; // internal variable
    int skip_default_balance = 1; // unlisted option
    int early_research_start = 1; // unlisted option
    int facility_capture_fix = 1; // unlisted option
    int territory_border_fix = 1;
    int auto_relocate_hq = 1;
    int simple_hurry_cost = 1;
    int eco_damage_fix = 1;
    int clean_minerals = 16;
    int biology_lab_bonus = 2;
    int rebuild_secret_projects = 0;
    int spawn_fungal_towers = 1;
    int spawn_spore_launchers = 1;
    int spawn_sealurks = 1;
    int spawn_battle_ogres = 1;
    int planetpearls = 1;
    int altitude_limit = 6; // internal variable
    int tile_output_limit[3] = {2,2,2};
    int soil_improve_value = 0;
    int aquatic_bonus_minerals = 1;
    int alien_guaranteed_techs = 1;
    int alien_early_start = 0;
    int cult_early_start = 0;
    int normal_elite_moves = 1;
    int native_elite_moves = 0;
    int native_weak_until_turn = -1;
    int native_lifecycle_levels[6] = {40,80,120,160,200,240};
    int facility_defense_bonus[4] = {100,100,100,100};
    int neural_amplifier_bonus = 50;
    int dream_twister_bonus = 50;
    int fungal_tower_bonus = 50;
    int planet_defense_bonus = 0;
    int sensor_defense_ocean = 0;
    int intercept_max_range = 2;
    int collateral_damage_value = 3;
    int content_pop_player[MaxDiffNum] = {6,5,4,3,2,1};
    int content_pop_computer[MaxDiffNum] = {3,3,3,3,3,3};
    int unit_support_bonus[MaxDiffNum] = {0,0,0,0,0,0};
    int repair_minimal = 1;
    int repair_fungus = 2;
    int repair_friendly = 1;
    int repair_airbase = 1;
    int repair_bunker = 1;
    int repair_base = 1;
    int repair_base_native = 10;
    int repair_base_facility = 10;
    int repair_nano_factory = 10;
    int repair_battle_ogre = 0;
    LMConfig landmarks;
    int minimal_popups = 0; // unlisted option
    int diplo_patience = 0; // internal variable
    int skip_random_events = 0; // internal variable
    int skip_random_factions = 0; // internal variable
    int faction_file_count = 14; // internal variable
    int reduced_mode = 0; // internal variable
    int debug_mode = DEBUG; // internal variable
    int debug_verbose = DEBUG; // internal variable
};

/*
AIPlans contains several general purpose variables for AI decision-making
that are recalculated each turn. These values are not stored in the save game.
*/
struct AIPlans {
    int main_region = -1;
    int main_region_x = -1;
    int main_region_y = -1;
    int main_sea_region = -1;
    int target_land_region = 0;
    int prioritize_naval = 0;
    int naval_scout_x = -1;
    int naval_scout_y = -1;
    int naval_airbase_x = -1;
    int naval_airbase_y = -1;
    int naval_start_x = -1;
    int naval_start_y = -1;
    int naval_end_x = -1;
    int naval_end_y = -1;
    int naval_beach_x = -1;
    int naval_beach_y = -1;
    int defend_weights = 0;
    int land_combat_units = 0;
    int sea_combat_units = 0;
    int air_combat_units = 0;
    int probe_units = 0;
    int missile_units = 0;
    int transport_units = 0;
    int unknown_factions = 0;
    int contacted_factions = 0;
    int enemy_factions = 0;
    int build_tubes = 0;
    /*
    Amount of minerals a base needs to produce before it is allowed to build secret projects.
    All faction-owned bases are ranked each turn based on the surplus mineral production,
    and only the top third are selected for project building.
    */
    int project_limit = 5;
    int median_limit = 5;
    int energy_limit = 15;
    /*
    PSI combat units are only selected for production if this score is higher than zero.
    Higher values will make the prototype picker choose these units more often.
    */
    int psi_score = 0;
    int keep_fungus = 0;
    int plant_fungus = 0;
    int satellite_goal = 0;
    int enemy_odp = 0;
    int enemy_sat = 0;
    int mil_strength = 0;
    int defense_modifier = 0;
    float enemy_base_range = 0;
    float enemy_mil_factor = 0;
    int enemy_bases = 0;
    int captured_bases = 0;
};

#include "engine.h"
#include "config.h"
#include "strings.h"
#include "faction.h"
#include "random.h"
#include "patch.h"
#include "base.h"
#include "build.h"
#include "game.h"
#include "gui.h"
#include "gui_dialog.h"
#include "veh.h"
#include "veh_turn.h"
#include "veh_combat.h"
#include "net.h"
#include "map.h"
#include "mapgen.h"
#include "probe.h"
#include "path.h"
#include "plan.h"
#include "goal.h"
#include "move.h"
#include "tech.h"
#include "test.h"
#include "debug.h"

extern FILE* debug_log;
extern Config conf;
extern AIPlans plans[MaxPlayerNum];
extern set_str_t movedlabels;
extern map_str_t musiclabels;

DLL_EXPORT DWORD ThinkerModule();
bool FileExists(const char* path);
void exit_fail(int32_t addr);
void exit_fail();
int opt_handle_error(const char* section, const char* name);
int opt_list_parse(int32_t* dst, char* src, int num, int min_val);



