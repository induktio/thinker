#pragma once

#include "main.h"

extern const char* AlphaFile;
extern const char* ScriptFile;
extern const char* BlurbsxFile;
extern const char* OpeningFile;
extern const char* MovlistFile;
extern const char* TutorFile;
extern const char* PopupScriptFile;
extern const char* MovlistTxtFile;
extern const char* ExpMovlistTxtFile;
extern const char* ModAlphaFile;
extern const char* ModHelpFile;
extern const char* ModTutorFile;
extern const char* ModConceptsFile;
extern const char* ModAlphaTxtFile;
extern const char* ModHelpTxtFile;
extern const char* ModTutorTxtFile;
extern const char* ModConceptsTxtFile;

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

const char* alpha_file();
FILE* env_open(const char* path, const char* mode);
int __cdecl X_text_open(const char* filename, const char* label);
int __cdecl text_open(const char* filename, const char* label);
void __cdecl text_close();
char* text_buf_ptr();
char* __cdecl text_get();
char* __cdecl text_item();
char* __cdecl text_item_string();
int __cdecl text_item_number();
int __cdecl text_item_binary();
int __cdecl text_get_number(int min_val, int max_val);
int __cdecl tech_name(char* name);
int __cdecl chas_name(char* name);
int __cdecl weap_name(char* name);
int __cdecl arm_name(char* name);
int __cdecl tech_item();
int __cdecl read_basic_rules();
int __cdecl read_tech();
void __cdecl clear_faction(MFaction* plr);
void __cdecl read_faction2(int faction_id);
void __cdecl read_faction(MFaction* plr, int toggle);
int __cdecl read_factions();
size_t pick_random_faction(int faction_id);
void __cdecl noun_item(int32_t* gender, int32_t* plural);
int __cdecl read_units();
int __cdecl read_rules(int tgl_all_rules);
char* __cdecl prefs_get_strcpy(char* dst, const char* src);
char* __cdecl prefs_get(const char* key_name, const char* default_value, int use_default);
int __cdecl prefs_get_2(const char* key_name, int default_value, int use_default);
void prefs_read(char* buf, size_t buf_len, const char* key_name, const char* default_value, int use_default);
uint32_t __cdecl default_prefs();
uint32_t __cdecl default_prefs2();
uint32_t __cdecl default_warn();
uint32_t __cdecl default_rules();
void __cdecl prefs_fac_load();
void __cdecl prefs_load(int use_default);
void __cdecl prefs_put(const char* key_name, const char* value);
void __cdecl prefs_put_2(const char* key_name, int value, int tgl_binary);
void __cdecl prefs_save(int save_factions);
void __cdecl prefs_use();

