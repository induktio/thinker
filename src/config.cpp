
#include "config.h"

char** TextBufferPosition = (char**)0x9B7CF0;
char** TextBufferParsePtr = (char**)0x9B7CF8;
char** TextBufferGetPtr = (char**)0x9B7D00;
char** TextBufferItemPtr = (char**)0x9B7D04;


static char* prefs_get_binary(char* buf, int value) {
    buf[0] = '\0';
    for (int shift = 31, non_pad = 0; shift >= 0; shift--) {
        if ((1 << shift) & value) {
            non_pad = 1;
            strncat(buf, "1", 33);
        } else if (non_pad || shift < 8) {
            strncat(buf, "0", 33);
        }
    }
    return buf;
}

static char* Text_update() {
    *TextBufferPosition = *TextBufferParsePtr;
    return *TextBufferParsePtr;
}

static void __cdecl set_language(int value) {
    *GameLanguage = value;
}

/*
Attempt to read the setting's value from the ini file.
*/
char* __cdecl prefs_get2(const char* key_name, const char* default_value, int use_default) {
    if (use_default || GetPrivateProfileIntA(GameAppName, "Prefs Format", 0, GameIniFile) != 12) {
        strncpy(*TextBufferGetPtr, default_value, 256);
    } else {
        GetPrivateProfileStringA(GameAppName, key_name, default_value, *TextBufferGetPtr, 256, GameIniFile);
    }
    return Text_update();
}

/*
Get the default value for the 1st set of preferences.
*/
uint32_t __cdecl default_prefs() {
    uint32_t base_prefs = DefaultBasePref;
    return prefs_get("Laptop", 0, false) ? base_prefs :
        base_prefs | PREF_AV_SECRET_PROJECT_MOVIES | PREF_AV_SLIDING_WINDOWS | PREF_AV_MAP_ANIMATIONS;
}

/*
Get the default value for the 2nd set of preferences.
*/
uint32_t __cdecl default_prefs2() {
    uint32_t base_prefs2 = DefaultMorePref;
    return prefs_get("Laptop", 0, false) ? base_prefs2 : base_prefs2 | MPREF_AV_SLIDING_SCROLLBARS;
}

/*
Get the default value for the warning pop-up preferences.
*/
uint32_t __cdecl default_warn() {
    return DefaultWarnPref;
}

/*
Get the default value for the rules related preferences.
*/
uint32_t __cdecl default_rules() {
    return DefaultRules;
}

/*
Attempt to read the setting's value from the ini file.
Returns the key integer value from the ini or default if not set.
*/
int __cdecl prefs_get(const char* key_name, int default_value, int use_default) {
    snprintf(StrBuffer, StrBufLen, "%d", default_value);
    if (use_default) {
        strncpy(*TextBufferGetPtr, StrBuffer, StrBufLen);
    } else {
        GetPrivateProfileStringA(GameAppName, key_name, StrBuffer, *TextBufferGetPtr,
            StrBufLen, GameIniFile);
    }
    return atoi(Text_update());
}

/*
Read the faction filenames and search for keys from the ini file (SMACX only). This has
the added effect of forcing the player's search_key to be set to the filename value.
Contains multiple rewrites. Original version checked for ExpansionEnabled but this is not
necessary because smac_only allows changing active faction lists.
*/
void __cdecl prefs_fac_load() {
    if (GetPrivateProfileIntA(GameAppName, "Prefs Format", 0, GameIniFile) == 12) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            char name_buf[LineBufLen] = {};
            char buf[256] = {};
            snprintf(name_buf, LineBufLen, "Faction %d", i);
            GetPrivateProfileStringA(GameAppName, name_buf, MFactions[i].filename, buf, 256, GameIniFile);
            strncpy(MFactions[i].filename, buf, 24);
            MFactions[i].filename[23] = '\0';
            strncpy(MFactions[i].search_key, buf, 24);
            MFactions[i].search_key[23] = '\0';
            Text_update();
        }
    } else {
        // use separate loop rather than check "Prefs Format" value each time in single loop
        for (int i = 1; i < MaxPlayerNum; i++) {
            strncpy(MFactions[i].search_key, MFactions[i].filename, 24);
        }
    }
}

/*
Load the most common preferences from the game's ini to globals.
*/
void __cdecl prefs_load(int use_default) {
    set_language(prefs_get("Language", 0, false));
    prefs_get("Difficulty", 0, false);
    DefaultPrefs->difficulty = text_item_number();
    prefs_get("Map Type", 0, false);
    DefaultPrefs->map_type = text_item_number();
    prefs_get("Top Menu", 0, false);
    DefaultPrefs->top_menu = text_item_number();
    prefs_get("Faction", 1, false);
    DefaultPrefs->faction_id = text_item_number();
    uint32_t prefs = default_prefs();
    if (DefaultPrefs->difficulty < DIFF_TALENT) {
        prefs |= PREF_BSC_TUTORIAL_MSGS;
    }
    char buf[LineBufLen];
    prefs_get2("Preferences", prefs_get_binary(buf, prefs), use_default);
    AlphaIniPrefs->preferences = text_item_binary();
    prefs_get2("More Preferences", prefs_get_binary(buf, default_prefs2()), use_default);
    AlphaIniPrefs->more_preferences = text_item_binary();
    prefs_get2("Semaphore", "00000000", use_default);
    AlphaIniPrefs->semaphore = text_item_binary();
    prefs_get("Customize", 0, false);
    AlphaIniPrefs->customize = text_item_number();
    prefs_get2("Rules", prefs_get_binary(buf, default_rules()), use_default);
    AlphaIniPrefs->rules = text_item_binary();
    prefs_get2("Announce", prefs_get_binary(buf, default_warn()), use_default);
    AlphaIniPrefs->announce = text_item_binary();
    prefs_get2("Custom World", "2, 1, 1, 1, 1, 1, 1, ", use_default);
    for (int i = 0; i < 7; i++) {
        AlphaIniPrefs->custom_world[i] = text_item_number();
    }
    prefs_get("Time Controls", 1, use_default);
    AlphaIniPrefs->time_controls = text_item_number();
}

/*
Write the string value to the pref key of the ini.
*/
void __cdecl prefs_put2(const char* key_name, const char* value) {
    WritePrivateProfileStringA(GameAppName, key_name, value, GameIniFile);
}

/*
Write the value as either an integer or a binary string to the pref key inside the ini.
*/
void __cdecl prefs_put(const char* key_name, int value, int tgl_binary) {
    char buf[StrBufLen] = {};
    if (tgl_binary) {
        prefs_get_binary(buf, value);
    } else {
        snprintf(buf, StrBufLen, "%d", value);
    }
    WritePrivateProfileStringA(GameAppName, key_name, buf, GameIniFile);
}

/*
Save the most common preferences from memory to the game's ini.
*/
void __cdecl prefs_save(int save_factions) {
    prefs_put("Prefs Format", 12, false);
    prefs_put("Difficulty", DefaultPrefs->difficulty, false);
    prefs_put("Map Type", DefaultPrefs->map_type, false);
    prefs_put("Top Menu", DefaultPrefs->top_menu, false);
    prefs_put("Faction", DefaultPrefs->faction_id, false);
    prefs_put("Preferences", AlphaIniPrefs->preferences, true);
    prefs_put("More Preferences", AlphaIniPrefs->more_preferences, true);
    prefs_put("Semaphore", AlphaIniPrefs->semaphore, true);
    prefs_put("Announce", AlphaIniPrefs->announce, true);
    prefs_put("Rules", AlphaIniPrefs->rules, true);
    prefs_put("Customize", AlphaIniPrefs->customize, false);
    char buf[LineBufLen] = {};
    snprintf(buf, LineBufLen, "%d, %d, %d, %d, %d, %d, %d,",
        AlphaIniPrefs->custom_world[0], // MapSizePlanet
        AlphaIniPrefs->custom_world[1], // MapOceanCoverage
        AlphaIniPrefs->custom_world[2], // MapLandCoverage
        AlphaIniPrefs->custom_world[3], // MapErosiveForces
        AlphaIniPrefs->custom_world[4], // MapPlanetaryOrbit
        AlphaIniPrefs->custom_world[5], // MapCloudCover
        AlphaIniPrefs->custom_world[6]  // MapNativeLifeForms
    );
    prefs_put2("Custom World", buf);
    prefs_put("Time Controls", AlphaIniPrefs->time_controls, false);
    // Original version checked for ExpansionEnabled but this is not necessary.
    if (save_factions) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            snprintf(buf, LineBufLen, "Faction %d", i);
            prefs_put2(buf, MFactions[i].filename);
        }
    }
}

/*
Set the internal game preference globals from the ini setting globals.
*/
void __cdecl prefs_use() {
    *GamePreferences = AlphaIniPrefs->preferences;
    *GameMorePreferences = AlphaIniPrefs->more_preferences;
    *GameWarnings = AlphaIniPrefs->announce;
}



