#pragma once

#include "main.h"

const uint32_t DefaultBasePref = PREF_ADV_RADIO_BTN_NOT_SEL_SING_CLK | PREF_AUTO_FORMER_BUILD_ADV
    | PREF_AUTO_FORMER_PLANT_FORESTS | PREF_AUTO_END_MOVE_SPOT_VEH_WAR
    | PREF_AUTO_END_MOVE_SPOT_VEH_TRUCE | PREF_AUTO_END_MOVE_SPOT_VEH_TREATY
    | PREF_AUTO_AIR_VEH_RET_HOME_FUEL_RNG | PREF_BSC_DONT_QUICK_MOVE_ALLY_VEH
    | PREF_BSC_AUTO_DESIGN_VEH | PREF_BSC_MOUSE_EDGE_SCROLL_VIEW | PREF_AV_BACKGROUND_MUSIC
    | PREF_AV_SOUND_EFFECTS | PREF_MAP_SHOW_GRID | PREF_UNK_10
    | PREF_BSC_DONT_QUICK_MOVE_ENEMY_VEH | PREF_BSC_AUTOSAVE_EACH_TURN
    | PREF_AUTO_WAKE_VEH_TRANS_REACH_LAND;

const uint32_t DefaultMorePref = MPREF_ADV_DETAIL_MAIN_MENUS | MPREF_BSC_AUTO_PRUNE_OBS_VEH
    | MPREF_AV_VOICEOVER_STOP_CLOSE_POPUP | MPREF_AV_VOICEOVER_TECH_FAC
    | MPREF_MAP_SHOW_BASE_NAMES | MPREF_MAP_SHOW_PROD_WITH_BASE_NAMES
    | MPREF_ADV_RIGHT_CLICK_POPS_UP_MENU | MPREF_ADV_QUICK_MOVE_VEH_ORDERS
    | MPREF_AUTO_FORMER_BUILD_SENSORS | MPREF_AUTO_FORMER_REMOVE_FUNGUS;

const uint32_t DefaultWarnPref = WARN_STOP_RANDOM_EVENT | WARN_STOP_ENERGY_SHORTAGE | WARN_STOP_MINERAL_SHORTAGE
    | WARN_STOP_STARVATION | WARN_STOP_BUILD_OUT_OF_DATE | WARN_STOP_UNK_100
    | WARN_STOP_NUTRIENT_SHORTAGE | WARN_STOP_GOLDEN_AGE | WARN_STOP_DRONE_RIOTS
    | WARN_STOP_NEW_FAC_BUILT;

const uint32_t DefaultRules = RULES_VICTORY_COOPERATIVE | RULES_VICTORY_TRANSCENDENCE | RULES_BLIND_RESEARCH
    | RULES_VICTORY_DIPLOMATIC | RULES_VICTORY_ECONOMIC | RULES_VICTORY_CONQUEST;

static_assert(DefaultBasePref == 0xA3E1DD16, "");
static_assert(DefaultMorePref == 0x327168, "");
static_assert(DefaultWarnPref == 0x3C3A9, "");
static_assert(DefaultRules == 0x1A0E, "");

char* __cdecl prefs_get2(const char* key_name, const char* default_value, int use_default);
uint32_t __cdecl default_prefs();
uint32_t __cdecl default_prefs2();
uint32_t __cdecl default_warn();
uint32_t __cdecl default_rules();
int __cdecl prefs_get(const char* key_name, int default_value, int use_default);
void __cdecl prefs_fac_load();
void __cdecl prefs_load(int use_default);
void __cdecl prefs_put2(const char* key_name, const char* value);
void __cdecl prefs_put(const char* key_name, int value, int tgl_binary);
void __cdecl prefs_save(int save_factions);
void __cdecl prefs_use();

