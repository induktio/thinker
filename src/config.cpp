
#include "config.h"

const char* AlphaFile = "alphax";
const char* ScriptFile = "script";
const char* ModAlphaFile = "smac_mod\\alphax";
const char* ModHelpFile = "smac_mod\\helpx";
const char* ModTutorFile = "smac_mod\\tutor";
const char* ModConceptsFile = "smac_mod\\conceptsx";
const char* ModAlphaTxtFile = "smac_mod\\alphax.txt";
const char* ModHelpTxtFile = "smac_mod\\helpx.txt";
const char* ModTutorTxtFile = "smac_mod\\tutor.txt";
const char* ModConceptsTxtFile = "smac_mod\\conceptsx.txt";
const char* OpeningFile = "opening";
const char* MovlistFile = "movlist";
const char* MovlistTxtFile = "movlist.txt";
const char* ExpMovlistTxtFile = "movlistx.txt";

char*  const TextBufferFileName = (char* )0x9B7BA0;
char*  const TextBufferFilePath = (char* )0x9B7BF0;
char** const TextBufferPosition = (char**)0x9B7CF0;
FILE** const TextBufferFile     = (FILE**)0x9B7CF4;
char** const TextBufferParsePtr = (char**)0x9B7CF8;
char** const TextBufferParseTmp = (char**)0x9B7CFC;
char** const TextBufferGetPtr   = (char**)0x9B7D00;
char** const TextBufferItemPtr  = (char**)0x9B7D04;

class TextBuffer {
public:
    char FileName[80]; // Engine limit
    char FilePath[256]; // Engine limit
    char Srcbuf[1024];
    char Dstbuf[1024];
    char* Position;
    FILE* File;
    char* const SrcPtr = (char*)&Srcbuf; // ParsePtr / GetPtr
    char* const DstPtr = (char*)&Dstbuf; // ParseTmp
};

static TextBuffer Text = {};

static const char* alpha_file() {
    return conf.smac_only ? ModAlphaFile : AlphaFile;
}

static void __cdecl set_language(int value) {
    *GameLanguage = value;
}

static char* prefs_get_binary(char* buf, int value) {
    size_t index = 0;
    for (int shift = 31, non_pad = 0; shift >= 0; shift--) {
        if ((1 << shift) & value) {
            non_pad = 1;
            buf[index++] = '1';
        } else if (non_pad || shift < 8) {
            buf[index++] = '0';
        }
    }
    buf[index] = '\0';
    return buf;
}

static FILE* env_open(const char* path, const char* mode) {
    char* path_alt = filefind_get(path);
    return fopen(path_alt ?  path_alt : path, mode);
}

static void text_close() {
    if (Text.File) {
        fclose(Text.File);
        Text.File = NULL;
    }
}

static int text_open(const char* filename, const char* label) {
    debug("text_open %s / %s\n", filename, label);
    bool is_seeking = false;
    if (filename) {
        if (!strchr(filename, '.')) {
            snprintf(Text.FileName, 80, "%s.txt", filename);
        } else {
            snprintf(Text.FileName, 80, filename);
        }
        text_close();
        Text.File = env_open(Text.FileName, "rt");
        if (!Text.File) {
            return true;
        }
        snprintf(Text.FilePath, 256, FileFind->last_path);
    } else if (Text.File) {
        is_seeking = true;
    } else {
        Text.File = env_open(Text.FileName, "rt");
        if (!Text.File) {
            return true;
        }
        snprintf(Text.FilePath, 256, FileFind->last_path);
    }
    if (!label) {
        return false;
    }
    // Removed text_search_index as redundant
    char sect_header[StrBufLen];
    snprintf(sect_header, StrBufLen, "#%s", label);
    do {
        if (feof(Text.File)) {
            if (is_seeking) {
                is_seeking = false;
                rewind(Text.File);
            } else {
                text_close();
                return true;
            }
        }
        if (!fgets(Text.SrcPtr, 511, Text.File)) {
            text_close();
            return true;
        }
        kill_lf(Text.SrcPtr);
        purge_spaces(Text.SrcPtr);
    } while (_stricmp(sect_header, Text.SrcPtr));
    Text.Position = Text.SrcPtr;
    return false;
}

static char* text_update() {
    Text.Position = Text.SrcPtr;
    return Text.SrcPtr;
}

static char* text_get() {
    if (feof(Text.File)) {
        Text.SrcPtr[0] = '\0';
        return NULL;
    } else if (fgets(Text.SrcPtr, 511, Text.File)) {
        kill_lf(Text.SrcPtr);
        purge_spaces(Text.SrcPtr);
        Text.Position = Text.SrcPtr;
        return Text.SrcPtr;
    } else {
        Text.SrcPtr[0] = '\0';
        return Text.SrcPtr;
    }
}

static char* text_item() {
    char* dst = Text.DstPtr;
    char** src = &Text.Position;
    while (**src != '\0' && **src != ',') {
        *dst = **src;
        dst++;
        (*src)++;
    }
    *dst = '\0';
    if (**src) {
        (*src)++;
    }
    purge_spaces(Text.DstPtr);
    return Text.DstPtr;
}

static char* text_item_string() {
    return Strings_put(TextTable, text_item());
}

static int text_item_number() {
    return stoi(text_item());
}

static int text_item_binary() {
    return btoi(text_item());
}

static int text_get_number(int min_val, int max_val) {
    text_get();
    return clamp(text_item_number(), min_val, max_val);
}

/*
Convert the tech name string to a numeric tech_id.
Return Value: tech_id; 'None' (-1); 'Disabled' (-2); or error (-2)
*/
int __cdecl tech_name(char* name) {
    strtrail(name); // Replace function purge_trailing
    if (!_stricmp(name, "None")) {
        return TECH_None;
    }
    if (!_stricmp(name, "Disable")) {
        return TECH_Disable;
    }
    for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
        if (!_stricmp(name, Tech[tech_id].short_name)) {
            return tech_id;
        }
    }
    parse_says(0, Text.FilePath, -1, -1);
    parse_says(1, name, -1, -1);
    parse_says(2, Text.SrcPtr, -1, -1);
    X_pop2("BADTECHKEY", 0);
    exit_fail(); // Game would crash after parser errors
    return TECH_Disable;
}

/*
Convert the chassis name string to a numeric chassis_id.
Return Value: chassis_id; 'None' (-1); 'Disabled' (-2); or error (0)
*/
static int __cdecl get_chas_name(char* name, bool check) {
    strtrail(name);
    if (!check && !_stricmp(name, "None")) {
        return TECH_None;
    }
    if (!check && !_stricmp(name, "Disable")) {
        return TECH_Disable;
    }
    for (int chas_id = 0; chas_id < MaxChassisNum; chas_id++) {
        if (!_stricmp(name, Chassis[chas_id].offsv1_name)) {
            return chas_id;
        }
    }
    parse_says(0, Text.FilePath, -1, -1);
    parse_says(1, name, -1, -1);
    parse_says(2, Text.SrcPtr, -1, -1);
    X_pop2("BADCHASKEY", 0);
    exit_fail();
    return 0;
}

int __cdecl chas_name(char* name) {
    return get_chas_name(name, false); // Allows None/Disable
}

/*
Convert the weapon name string to a numeric weapon_id.
Return Value: weapon_id; 'None' (-1); 'Disabled' (-2); or error (0)
*/
static int __cdecl get_weap_name(char* name, bool check) {
    strtrail(name);
    if (!check && !_stricmp(name, "None")) {
        return TECH_None;
    }
    if (!check && !_stricmp(name, "Disable")) {
        return TECH_Disable;
    }
    for (int wpn_id = 0; wpn_id < MaxWeaponNum; wpn_id++) {
        if (!_stricmp(name, Weapon[wpn_id].name_short)) {
            return wpn_id;
        }
    }
    parse_says(0, Text.FilePath, -1, -1);
    parse_says(1, name, -1, -1);
    parse_says(2, Text.SrcPtr, -1, -1);
    X_pop2("BADWEAPKEY", 0);
    exit_fail();
    return 0;
}

int __cdecl weap_name(char* name) {
    return get_weap_name(name, false); // Allows None/Disable
}

/*
Convert the armor name string to a numeric armor_id.
Return Value: armor_id; 'None' (-1); 'Disabled' (-2); or error (0)
*/
static int __cdecl get_arm_name(char* name, bool check) {
    strtrail(name);
    if (!check && !_stricmp(name, "None")) {
        return TECH_None;
    }
    if (!check && !_stricmp(name, "Disable")) {
        return TECH_Disable;
    }
    for (int arm_id = 0; arm_id < MaxArmorNum; arm_id++) {
        if (!_stricmp(name, Armor[arm_id].name_short)) {
            return arm_id;
        }
    }
    parse_says(0, Text.FilePath, -1, -1);
    parse_says(1, name, -1, -1);
    parse_says(2, Text.SrcPtr, -1, -1);
    X_pop2("BADARMKEY", 0);
    exit_fail();
    return 0;
}

int __cdecl arm_name(char* name) {
    return get_arm_name(name, false); // Allows None/Disable
}

/*
Parse the current tech name inside the Txt item buffer into a tech_id.
Return Value: tech_id
*/
int __cdecl tech_item() {
    text_get();
    return tech_name(text_item());
}

/*
Parse the #RULES & #WORLDBUILDER sections inside the alpha(x).txt.
Return Value: Was there an error? true/false
*/
int __cdecl read_basic_rules() {
    if (text_open(alpha_file(), "RULES")) {
        return true;
    }
    Rules->move_rate_roads = text_get_number(1, 100);
    Rules->nutrient_intake_req_citizen = text_get_number(0, 100);
    text_get(); // Read two numbers from same line
    Rules->artillery_dmg_numerator = clamp(text_item_number(), 1, 1000);
    Rules->artillery_dmg_denominator = clamp(text_item_number(), 1, 1000);
    Rules->artillery_max_rng = text_get_number(1, 8);
    Rules->max_airdrop_rng_wo_orbital_insert = text_get_number(1, 500);
    Rules->nutrient_cost_multi = text_get_number(1, 100);
    Rules->mineral_cost_multi = text_get_number(1, 100);
    Rules->tech_discovery_rate = text_get_number(0, 1000);
    Rules->limit_mineral_inc_for_mine_wo_road = text_get_number(0, 100);
    Rules->nutrient_effect_mine_sq = text_get_number(-1, 0);
    Rules->min_base_size_specialists = text_get_number(0, 100);
    Rules->drones_induced_genejack_factory = text_get_number(0, 100);
    Rules->pop_limit_wo_hab_complex = text_get_number(1, 100);
    Rules->pop_limit_wo_hab_dome = text_get_number(1, 100);
    Rules->extra_cost_prototype_land = text_get_number(0, 500);
    Rules->extra_cost_prototype_sea = text_get_number(0, 500);
    Rules->extra_cost_prototype_air = text_get_number(0, 500);
    text_get();
    Rules->psi_combat_ratio_atk[TRIAD_LAND] = clamp(text_item_number(), 1, 1000);
    Rules->psi_combat_ratio_def[TRIAD_LAND] = clamp(text_item_number(), 1, 1000);
    text_get();
    Rules->psi_combat_ratio_atk[TRIAD_SEA] = clamp(text_item_number(), 1, 1000);
    Rules->psi_combat_ratio_def[TRIAD_SEA] = clamp(text_item_number(), 1, 1000);
    text_get();
    Rules->psi_combat_ratio_atk[TRIAD_AIR] = clamp(text_item_number(), 1, 1000);
    Rules->psi_combat_ratio_def[TRIAD_AIR] = clamp(text_item_number(), 1, 1000);
    Rules->starting_energy_reserve = text_get_number(0, 1000);
    Rules->combat_bonus_intrinsic_base_def = text_get_number(-100, 1000);
    Rules->combat_bonus_atk_road = text_get_number(-100, 1000);
    Rules->combat_bonus_atk_higher_elevation = text_get_number(-100, 1000);
    Rules->combat_penalty_atk_lower_elevation = text_get_number(-100, 1000);
    Rules->combat_mobile_open_ground = text_get_number(-100, 1000);
    Rules->combat_mobile_def_in_rough = text_get_number(-100, 1000);
    Rules->combat_infantry_vs_base = text_get_number(-100, 1000);
    Rules->combat_penalty_atk_airdrop = text_get_number(-100, 1000);
    Rules->combat_bonus_fanatic = text_get_number(-100, 1000);
    Rules->combat_land_vs_sea_artillery = text_get_number(-100, 1000);
    Rules->combat_artillery_bonus_altitude = text_get_number(-100, 1000);
    Rules->combat_bonus_trance_vs_psi = text_get_number(-100, 1000);
    Rules->combat_bonus_empath_song_vs_psi = text_get_number(-100, 1000);
    Rules->combat_penalty_air_supr_vs_ground = text_get_number(-100, 1000);
    Rules->combat_bonus_air_supr_vs_air = text_get_number(-100, 1000);
    Rules->combat_penalty_non_combat_vs_combat = text_get_number(-100, 1000);
    Rules->combat_comm_jammer_vs_mobile = text_get_number(-100, 1000);
    Rules->combat_bonus_vs_ship_port = text_get_number(-100, 1000);
    Rules->combat_aaa_bonus_vs_air = text_get_number(-100, 1000);
    Rules->combat_defend_sensor = text_get_number(-100, 1000);
    Rules->combat_psi_bonus_per_planet = text_get_number(-100, 1000);
    Rules->retool_penalty_prod_change = text_get_number(0, 100);
    Rules->retool_strictness = text_get_number(0, 3);
    Rules->retool_exemption = text_get_number(0, 1000);
    Rules->min_turns_between_councils = text_get_number(0, 1000);
    Rules->minerals_harvesting_forest = text_get_number(0, 100);
    Rules->territory_max_dist_base = text_get_number(0, 100);
    Rules->turns_corner_global_energy_market = text_get_number(1, 100);
    Rules->tech_preq_improv_fungus = tech_item();
    Rules->tech_preq_ease_fungus_mov = tech_item();
    Rules->tech_preq_build_road_fungus = tech_item();
    Rules->tech_preq_allow_2_spec_abil = tech_item();
    Rules->tech_preq_allow_3_nutrients_sq = tech_item();
    Rules->tech_preq_allow_3_minerals_sq = tech_item();
    Rules->tech_preq_allow_3_energy_sq = tech_item();
    Rules->tech_preq_orb_insert_wo_space = tech_item();
    Rules->tech_preq_mining_platform_bonus = tech_item();
    Rules->tech_preq_economic_victory = tech_item();
    Rules->tgl_probe_steal_tech = text_get_number(0, 1); // Fix: Set min param to 0
    Rules->tgl_humans_always_contact_net = text_get_number(0, 1); // Fix: Set min param to 0
    Rules->tgl_humans_always_contact_pbem = text_get_number(0, 1); // Fix: Set min param to 0
    Rules->max_dmg_percent_arty_base_bunker = text_get_number(10, 100);
    Rules->max_dmg_percent_arty_open = text_get_number(10, 100);
    Rules->max_dmg_percent_arty_sea = text_get_number(10, 100);
    text_get();
    Rules->freq_global_warming_numerator = clamp(text_item_number(), 0, 1000);
    Rules->freq_global_warming_denominator = clamp(text_item_number(), 1, 1000);
    Rules->normal_start_year = text_get_number(0, 999999);
    Rules->normal_end_year_low_three_diff = text_get_number(0, 999999);
    Rules->normal_end_year_high_three_diff = text_get_number(0, 999999);
    Rules->tgl_oblit_base_atrocity = text_get_number(0, 1); // Fix: Set min param to 0
    Rules->base_size_subspace_gen = text_get_number(1, 999); // SMACX only
    Rules->subspace_gen_req = text_get_number(1, 999); // SMACX only
    if (text_open(alpha_file(), "WORLDBUILDER")) {
        return true;
    }
    WorldBuilder->land_base = text_get_number(50, 4000);
    WorldBuilder->land_mod = text_get_number(0, 2000);
    WorldBuilder->continent_base = text_get_number(5, 1000);
    WorldBuilder->continent_mod = text_get_number(5, 1000);
    WorldBuilder->hills_base = text_get_number(0, 100);
    WorldBuilder->hills_mod = text_get_number(0, 100);
    WorldBuilder->plateau_base = text_get_number(0, 500);
    WorldBuilder->plateau_mod = text_get_number(0, 500);
    WorldBuilder->rivers_base = text_get_number(0, 100);
    WorldBuilder->rivers_rain_mod = text_get_number(0, 100);
    WorldBuilder->solar_energy = text_get_number(1, 64);
    WorldBuilder->thermal_band = text_get_number(1, 64);
    WorldBuilder->thermal_deviance = text_get_number(1, 64);
    WorldBuilder->global_warming = text_get_number(1, 64);
    WorldBuilder->sea_level_rises = text_get_number(1, 100);
    WorldBuilder->cloudmass_peaks = text_get_number(0, 20);
    WorldBuilder->cloudmass_hills = text_get_number(0, 20);
    WorldBuilder->rainfall_coeff = text_get_number(0, 8);
    WorldBuilder->deep_water = text_get_number(-100, 100);
    WorldBuilder->shelf = text_get_number(-100, 100);
    WorldBuilder->plains = text_get_number(-100, 100);
    WorldBuilder->beach = text_get_number(-100, 100);
    WorldBuilder->hills = text_get_number(0, 100);
    WorldBuilder->peaks = text_get_number(-100, 100);
    WorldBuilder->fungus = text_get_number(0, 5);
    text_get();
    WorldBuilder->cont_size_ratio1 = text_item_number();
    WorldBuilder->cont_size_ratio2 = text_item_number();
    WorldBuilder->cont_size_ratio3 = text_item_number();
    WorldBuilder->cont_size_ratio4 = text_item_number();
    WorldBuilder->cont_size_ratio5 = text_item_number();
    WorldBuilder->islands = text_get_number(1, 500);
    // Magtube movement speed options
    if (DEBUG && conf.magtube_movement_rate > 0) {
        int reactor_value = REC_FISSION;
        for (VehReactor r : {REC_FUSION, REC_QUANTUM, REC_SINGULARITY}) {
            if (Reactor[r - 1].preq_tech != TECH_Disable) {
                reactor_value = r;
            }
        }
        for (int i = 0; i < MaxChassisNum; i++) {
            if (Chassis[i].speed < 1 || Chassis[i].preq_tech == TECH_Disable) {
                continue;
            }
            int multiplier = Rules->move_rate_roads * conf.magtube_movement_rate;
            int base_speed = Chassis[i].speed + 1; // elite bonus
            if (Chassis[i].triad == TRIAD_AIR) {
                base_speed += 2*reactor_value; // triad bonus
                if (!Chassis[i].missile) {
                    if (Facility[FAC_CLOUDBASE_ACADEMY].preq_tech != TECH_Disable) {
                        base_speed += 2;
                    }
                    if (Ability[ABL_ID_FUEL_NANOCELLS].preq_tech != TECH_Disable) {
                        base_speed += 2;
                    }
                    if (Ability[ABL_ID_ANTIGRAV_STRUTS].preq_tech != TECH_Disable) {
                        base_speed += 2*reactor_value;
                    }
                }
            } else {
                base_speed += 1; // antigrav struts
            }
            debug("chassis_speed %d %d %d %s\n",
                i, base_speed, base_speed * multiplier, Chassis[i].offsv1_name);
        }
    }
    if (conf.magtube_movement_rate > 0) {
        conf.road_movement_rate = conf.magtube_movement_rate;
        Rules->move_rate_roads *= conf.magtube_movement_rate;
    }
    if (!conf.revised_tech_cost) {
        conf.tech_stagnate_rate = 150;
    }
    // Maximum speed is rounded down to nearest full movement point
    conf.max_movement_rate = 255 - (255 % Rules->move_rate_roads);
    conf.tech_rate_modifier = 10000 / max(1, Rules->tech_discovery_rate);
    return false;
}

/*
Parse the #TECHNOLOGY section inside the alpha(x).txt with a duplicate entry check.
Return Value: Was there an error? true/false
*/
int __cdecl read_tech() {
    if (text_open(alpha_file(), "TECHNOLOGY")) {
        return true;
    }
    for (int i = 0; i < MaxTechnologyNum; i++) {
        text_get();
        text_item();
        strcpy_n(Tech[i].short_name, 8, text_item());
        for (int j = 0; j < i; j++) {
            if (!strcmp(Tech[i].short_name, Tech[j].short_name)) {
                parse_num(0, i);
                parse_num(1, j);
                parse_says(0, Tech[i].short_name, -1, -1);
                parse_says(1, Text.FilePath, -1, -1); // Changed pointer to directly reference Text class
                parse_says(2, Text.SrcPtr, -1, -1);
                X_pop2("DUPLICATETECH", 0);
                exit_fail();
            }
        }
    }
    if (text_open(alpha_file(), "TECHNOLOGY")) {
        return true;
    }
    *TechValidCount = 0;
    *TechCommerceCount = 0;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        text_get();
        Tech[i].name = text_item_string();
        text_item();
        Tech[i].AI_power = text_item_number();
        Tech[i].AI_tech = text_item_number();
        Tech[i].AI_wealth = text_item_number();
        Tech[i].AI_growth = text_item_number();
        Tech[i].preq_tech1 = tech_name(text_item());
        Tech[i].preq_tech2 = tech_name(text_item());
        Tech[i].flags = text_item_binary();
        if (Tech[i].preq_tech1 != TECH_Disable
        && Tech[i].preq_tech2 != TECH_Disable) {
            *TechValidCount += 1;
            if (Tech[i].flags & TFLAG_INC_COMMERCE) {
                *TechCommerceCount += 1;
            }
        }
    }
    return false;
}

/*
Clear the rule values for the specified player.
*/
void __cdecl clear_faction(MFaction* plr) {
    plr->rule_tech_selected = 0;
    plr->rule_morale = 0;
    plr->rule_research = 0;
    plr->rule_drone = 0;
    plr->rule_talent = 0;
    plr->rule_energy = 0;
    plr->rule_interest = 0;
    plr->rule_population = 0;
    plr->rule_hurry = 100;
    plr->rule_techcost = 100;
    plr->rule_psi = 0;
    plr->rule_sharetech = 0;
    plr->rule_commerce = 0;
    plr->rule_flags = 0;
    plr->faction_bonus_count = 0;
}

void __cdecl read_faction(MFaction* plr, int toggle);

/*
Parse the faction definition and graphics for the specified faction_id.
*/
void __cdecl read_faction2(int faction_id) {
    if (faction_id) {
        read_faction(&MFactions[faction_id], 0);
        load_faction_art(faction_id);
    }
}

static bool is_bonus(const char* src, FactionBonusName index) {
    return !_stricmp(src, BonusName[index].key);
}

/*
Parse the 1st eight lines of the specified faction's file into a player structure. The
toggle parameter will end the function early if set to 2 (original code never uses this).
*/
void __cdecl read_faction(MFaction* plr, int toggle) {
    clear_faction(plr);
    if (text_open(plr->filename, plr->search_key)
    && text_open(plr->filename, plr->filename)) {
        parse_says(0, plr->search_key, -1, -1);
        parse_says(1, plr->filename, -1, -1);
        X_pop2("PLAYERFILE", 0);
        return;
    }
    text_get();
    strcpy_n(plr->formal_name_faction, 40, text_item());
    strcpy_n(plr->desc_name_faction, 24, text_item());
    strcpy_n(plr->noun_faction, 24, text_item());
    char* gender = text_item();
    plr->noun_gender = GENDER_MALE;
    if (gender[0] == 'F' || gender[0] == 'f') {
        plr->noun_gender = GENDER_FEMALE;
    } else if (gender[0] == 'N' || gender[0] == 'n') {
        plr->noun_gender = GENDER_NEUTRAL;
    }
    plr->is_noun_plural = clamp(text_item_number() - 1, 0, 1); // original value: 1 or 2
    strcpy_n(plr->name_leader, 24, text_item());
    gender = text_item();
    plr->is_leader_female = (gender[0] == 'F' || gender[0] == 'f');
    if (toggle == 2) {
        return;
    }
    // Fix: added variable bounding for AI priorities since original code will not work otherwise
    plr->AI_fight = clamp(text_item_number(), -1, 1);
    plr->AI_power = clamp(text_item_number(), 0, 1);
    plr->AI_tech = clamp(text_item_number(), 0, 1);
    plr->AI_wealth = clamp(text_item_number(), 0, 1);
    plr->AI_growth = clamp(text_item_number(), 0, 1);
    text_get();
    char* parse_check;
    while (parse_check = text_item(), strlen(parse_check) > 0) {
        char parse_rule[StrBufLen];
        strcpy_n(parse_rule, StrBufLen, parse_check);
        char* parse_param = text_item();
        if (is_bonus(parse_rule, BN_TECH)) {
            // TODO: this will have issues if custom tech abbreviations starting with numbers are used
            int player_selected = atoi(parse_param);
            if (!player_selected && plr->faction_bonus_count < MaxBonusNum) {
                plr->faction_bonus_id[plr->faction_bonus_count] = RULE_TECH;
                plr->faction_bonus_val1[plr->faction_bonus_count] = tech_name(parse_param);
                plr->faction_bonus_count++;
            } else {
                plr->rule_tech_selected = player_selected;
            }
        } else if (is_bonus(parse_rule, BN_MORALE)) {
            if (parse_param[0] == '+') {
                parse_param++;
            }
            plr->rule_morale = atoi(parse_param);
            // 0 indicates an exemption from negative modifiers from other sources
            if (!plr->rule_morale) {
                plr->rule_flags |= RFLAG_MORALE;
            }
        } else if (is_bonus(parse_rule, BN_FACILITY) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_FACILITY;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_RESEARCH)) {
            plr->rule_research = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_DRONE)) {
            plr->rule_drone = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_TALENT)) {
            plr->rule_talent = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_ENERGY)) {
            plr->rule_energy = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_INTEREST)) {
            plr->rule_flags |= RFLAG_INTEREST;
            plr->rule_interest = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_COMMERCE)) {
            plr->rule_commerce = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_POPULATION)) {
            plr->rule_population = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_HURRY)) {
            plr->rule_hurry = clamp(atoi(parse_param), 1, 1000);
        } else if (is_bonus(parse_rule, BN_TECHCOST)) {
            plr->rule_techcost = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_UNIT) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_UNIT;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_PSI)) {
            plr->rule_psi = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_SHARETECH)) {
            plr->rule_sharetech = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_TERRAFORM)) {
            plr->rule_flags |= RFLAG_TERRAFORM;
        } else if ((is_bonus(parse_rule, BN_SOCIAL)
        || is_bonus(parse_rule, BN_ROBUST)
        || is_bonus(parse_rule, BN_IMMUNITY)) && plr->faction_bonus_count < MaxBonusNum) {
            // Moved faction_bonus_count check to start rather than inner loop
            int value = 0;
            while (parse_param[0] == '+' || parse_param[0] == '-') {
                (parse_param[0] == '+') ? value++ : value--;
                parse_param++;
            }
            if (!value) { // cannot be zero
                value = 1;
            }
            for (int j = 0; j < MaxSocialEffectNum; j++) {
                if (!_stricmp(SocialParam[j].set1, parse_param)) {
                    if (is_bonus(parse_rule, BN_ROBUST)) {
                        plr->faction_bonus_id[plr->faction_bonus_count] = RULE_ROBUST;
                    } else {
                        plr->faction_bonus_id[plr->faction_bonus_count] =
                            is_bonus(parse_rule, BN_SOCIAL) ? RULE_SOCIAL : RULE_IMMUNITY;
                    }
                    plr->faction_bonus_val1[plr->faction_bonus_count] = j; // soc effect id
                    plr->faction_bonus_val2[plr->faction_bonus_count] = value; // value mod
                    plr->faction_bonus_count++;
                    break;
                }
            }
        } else if ((is_bonus(parse_rule, BN_IMPUNITY)
        || is_bonus(parse_rule, BN_PENALTY)) && plr->faction_bonus_count < MaxBonusNum) {
            // Moved faction_bonus_count check to start rather than inner loop
            for (int j = 0; j < MaxSocialCatNum; j++) {
                for (int k = 0; k < MaxSocialModelNum; k++) {
                    if (!_stricmp(parse_param, SocialField[j].soc_name[k])) {
                        plr->faction_bonus_id[plr->faction_bonus_count] =
                            is_bonus(parse_rule, BN_IMPUNITY) ? RULE_IMPUNITY : RULE_PENALTY;
                        plr->faction_bonus_val1[plr->faction_bonus_count] = j; // category id
                        plr->faction_bonus_val2[plr->faction_bonus_count] = k; // model id
                        plr->faction_bonus_count++;
                    }
                }
            }
        } else if (is_bonus(parse_rule, BN_FUNGNUTRIENT) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_FUNGNUTRIENT;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_FUNGMINERALS) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_FUNGMINERALS;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_FUNGENERGY) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_FUNGENERGY;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_COMMFREQ)) {
            plr->rule_flags |= RFLAG_COMMFREQ;
        } else if (is_bonus(parse_rule, BN_MINDCONTROL)) {
            plr->rule_flags |= RFLAG_MINDCONTROL;
        } else if (is_bonus(parse_rule, BN_FANATIC)) {
            plr->rule_flags |= RFLAG_FANATIC;
        } else if (is_bonus(parse_rule, BN_VOTES) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_VOTES;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_FREEPROTO)) {
            plr->rule_flags |= RFLAG_FREEPROTO;
        } else if (is_bonus(parse_rule, BN_AQUATIC)) {
            plr->rule_flags |= RFLAG_AQUATIC;
        } else if (is_bonus(parse_rule, BN_ALIEN)) {
            plr->rule_flags |= RFLAG_ALIEN;
        } else if (is_bonus(parse_rule, BN_FREEFAC) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_FREEFAC;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_REVOLT) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_REVOLT;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_NODRONE) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_NODRONE;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_WORMPOLICE)) {
            plr->rule_flags |= RFLAG_WORMPOLICE;
        } else if (is_bonus(parse_rule, BN_FREEABIL) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_FREEABIL;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_PROBECOST) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_PROBECOST;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_DEFENSE) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_DEFENSE;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_OFFENSE) && plr->faction_bonus_count < MaxBonusNum) {
            plr->faction_bonus_id[plr->faction_bonus_count] = RULE_OFFENSE;
            plr->faction_bonus_val1[plr->faction_bonus_count] = atoi(parse_param);
            plr->faction_bonus_count++;
        } else if (is_bonus(parse_rule, BN_TECHSHARE)) {
            plr->rule_flags |= RFLAG_TECHSHARE;
            plr->rule_sharetech = atoi(parse_param);
        } else if (is_bonus(parse_rule, BN_TECHSTEAL)) {
            plr->rule_flags |= RFLAG_TECHSTEAL;
        } else {
            debug("read_faction %s skip: %s\n", plr->filename, parse_rule);
        }
    }
    debug("read_faction %s bonus: %d\n", plr->filename, plr->faction_bonus_count);
    // Societal Ideology + Anti-Ideology
    for (int i = 0; i < 2; i++) {
        *(&plr->soc_priority_category + i) = -1;
        *(&plr->soc_priority_model + i) = 0;
        *(&plr->soc_priority_effect + i) = -1;
        text_get();
        char* soc_category = text_item();
        for (int j = 0; j < MaxSocialCatNum; j++) {
            char* check_cat_type = SocialField[j].field_name;
            if (*GameLanguage ? !_strnicmp(soc_category, check_cat_type, 4)
            : !_stricmp(soc_category, check_cat_type)) {
                *(&plr->soc_priority_category + i) = j;
                break;
            }
        }
        char* soc_model = text_item();
        int soc_cat_num = *(&plr->soc_priority_category + i);
        if (soc_cat_num >= 0) {
            for (int j = 0; j < MaxSocialModelNum; j++) {
                char* check_model = SocialField[soc_cat_num].soc_name[j];
                if (*GameLanguage ?
                !_strnicmp(soc_model, check_model, 4) : !_stricmp(soc_model, check_model)) {
                    *(&plr->soc_priority_model + i) = j;
                    break;
                }
            }
        }
        // Fix: Original code sets this value to -1, disabling AI Social Effect "Emphasis" value.
        // No indication this was intentional.
        char* soc_effect = text_item();
        for (int j = 0; j < MaxSocialEffectNum; j++) {
            if (!_stricmp(SocialParam[j].set1, soc_effect)) {
                *(&plr->soc_priority_effect + i) = j;
                break;
            }
        }
    }
    // Faction and Leader related strings
    text_get(); // skips 2nd value in this line, abbreviation unused?
    strcpy_n(plr->adj_name_faction, 128, text_item());
    text_get();
    strcpy_n(plr->assistant_name, 24, text_item());
    strcpy_n(plr->scientist_name, 24, text_item());
    strcpy_n(plr->assistant_city, 24, text_item());
    text_get();
    strcpy_n(plr->title_leader, 24, text_item());
    strcpy_n(plr->adj_leader, 128, text_item());
    strcpy_n(plr->adj_insult_leader, 128, text_item());
    strcpy_n(plr->adj_faction, 128, text_item());
    strcpy_n(plr->adj_insult_faction, 128, text_item());
    text_get();
    strcpy_n(plr->insult_leader, 24, text_item());
    text_close(); // Make sure file handle is closed
}

/*
Parse the #BONUSNAMES, #FACTIONS, and #NEWFACTIONS sections inside the alpha(x).txt.
Return Value: Was there an error? true/false
*/
int __cdecl read_factions() {
    if (text_open(alpha_file(), "BONUSNAMES")) {
        return true;
    }
    for (int i = 0; i < MaxBonusNameNum; i++) {
        if (!(i % 8)) { // 8 entries per line
            text_get();
        }
        strcpy_n(BonusName[i].key, 24, text_item());
    }
    if (text_open(alpha_file(), conf.smac_only ? "FACTIONS" : "NEWFACTIONS")) {
        return true;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        text_get();
        strcpy_n(MFactions[i].filename, 24, text_item());
        strcpy_n(MFactions[i].search_key, 24, text_item());
    }
    // Previously SMACX only: override any values parsed from alphax.txt #NEWFACTIONS if set in ini.
    // Original version checked for ExpansionEnabled but this is not
    // necessary because smac_only allows changing active faction lists.
    prefs_fac_load();
    conf.faction_file_count = 14;
    if (!text_open(alpha_file(), "CUSTOMFACTIONS")) { // get count of custom factions
        text_get();
        for (char* custom = text_item(); *custom; custom = text_item()) {
            conf.faction_file_count++;
            text_get();
        }
    }
    debug("read_factions count: %d\n", conf.faction_file_count);
    for (int i = 1; i < MaxPlayerNum; i++) { // Skip MFactions[0] used for native units
        if (!strcmp(MFactions[i].filename, "JENN282")) {
            int choice;
            do {
                int rand_val = 0;
                for (int j = 0; j < 1000; j++) {
                    rand_val = random(conf.faction_file_count);
                    if ((1 << rand_val) & ~conf.skip_random_factions) {
                        break;
                    }
                }
                const char* label;
                if (rand_val < 7) {
                    label = "FACTIONS";
                    choice = rand_val;
                } else if (rand_val < 14) {
                    label = "NEWFACTIONS";
                    choice = rand_val - 7;
                } else {
                    label = "CUSTOMFACTIONS";
                    choice = rand_val - 14;
                }
                if (text_open(alpha_file(), label)) {
                    return true;
                }
                for (int j = choice; j >= 0; j--) {
                    text_get();
                }
                // Original version copied filename twice?
                // This is most likely redundant debug feature that was left in the game
                strcpy_n(MFactions[i].filename, 24, text_item());
                strcpy_n(MFactions[i].search_key, 24, text_item());
                for (int k = 1; k < MaxPlayerNum; k++) {
                    if (i != k && !strcmp(MFactions[i].filename, MFactions[k].filename)) {
                        choice = -1;
                        break;
                    }
                }
                if (choice >= 0) {
                    read_faction(&MFactions[i], 0);
                    load_faction_art(i);
                }
            } while (choice < 0);
        } else {
            read_faction(&MFactions[i], 0);
            load_faction_art(i);
        }
    }
    text_close(); // Make sure file handle is closed
    return false;
}

/*
Parse and set the noun item gender and plurality from the Text buffer.
*/
void __cdecl noun_item(int32_t* gender, int32_t* plural) {
    char* noun = text_item();
    *gender = 0; // defaults to male ('m' || 'M')
    *plural = false; // defaults to singular ('1')
    if (noun[0] == 'f' || noun[0] == 'F') {
        *gender = 1;
    } else if (noun[0] == 'n' || noun[0] == 'N') {
        *gender = 2;
    }
    if (noun[1] == '2') {
        *plural = true;
    }
}

/*
Parse the #UNITS section inside the alpha(x).txt.
Return Value: Was there an error? true/false
*/
int __cdecl read_units() {
    if (text_open(alpha_file(), "UNITS")) {
        return true;
    }
    int total_units = text_get_number(0, MaxProtoFactionNum);
    for (int unit_id = 0; unit_id < total_units; unit_id++) {
        text_get();
        strcpy_n(Units[unit_id].name, 32, text_item());
        int chas_id = get_chas_name(text_item(), true);
        int weap_id = get_weap_name(text_item(), true);
        int armor_id = get_arm_name(text_item(), true);
        int plan = text_item_number();
        int cost = text_item_number();
        int carry = text_item_number();
        Units[unit_id].preq_tech = (int16_t)tech_name(text_item());
        int icon = text_item_number();
        int ability = text_item_binary();
        // Add ability to set custom reactor for #UNITS
        int reactor_id = clamp(text_item_number(), 0, MaxReactorNum);
        if (!reactor_id) { // If not set or 0, default behavior
            switch (unit_id) {
              case BSC_BATTLE_OGRE_MK2:
                reactor_id = REC_FUSION;
                break;
              case BSC_BATTLE_OGRE_MK3:
                reactor_id = REC_QUANTUM;
                break;
              default: // Also applies to BSC_BATTLE_OGRE_MK1
                reactor_id = REC_FISSION;
                break;
            }
        }
        mod_make_proto(unit_id, (VehChassis)chas_id, (VehWeapon)weap_id, (VehArmor)armor_id,
            (VehAblFlag)ability, (VehReactor)reactor_id);
        // If set, override auto calculated values from make_proto()
        if (plan != -1) { // plan auto calculate: -1
            Units[unit_id].plan = (uint8_t)plan;
        }
        if (cost) { // cost auto calculate: 0
            Units[unit_id].cost = (uint8_t)cost;
        }
        if (carry) { // carry auto calculate: 0
            Units[unit_id].carry_capacity = (uint8_t)carry;
        }
        Units[unit_id].icon_offset = (int8_t)icon;
    }
    return false;
}

/*
Parse all the game rules from alpha(x).txt. If tgl_all_rules is set to true, parse also
#UNITS & #FACTIONS sections when starting a new game. Otherwise, skip both.
Return Value: Was there an error? true/false
*/
int __cdecl read_rules(int tgl_all_rules) {
    Strings_init(TextTable, 49952);
    if (labels_init()) {
        return true;
    }
    // Removed text_clear_index / text_make_index as redundant
    if (read_tech() || read_basic_rules() || text_open(alpha_file(), "TERRAIN")) {
        return true;
    }
    for (int i = 0; i < MaxTerrainNum; i++) {
        text_get();
        // Land + sea terraforming
        Terraform[i].name = text_item_string();
        Terraform[i].preq_tech = tech_name(text_item());
        Terraform[i].name_sea = text_item_string();
        Terraform[i].preq_tech_sea = tech_name(text_item());
        Terraform[i].rate = text_item_number();
        // Add in bits & incompatible bits vs hard coded constant struct
        Terraform[i].bit = TerraformRules[i][0];
        Terraform[i].bit_incompatible = TerraformRules[i][1];
        // Land + sea orders
        char buf[StrBufLen];
        char* order_str = text_item();
        buf[0] = '\0';
        parse_says(0, Terraform[i].name, -1, -1); // parse_say
        parse_string(order_str, buf);
        Order[i + 4].order = Strings_put(TextTable, buf);
        buf[0] = '\0';
        parse_says(0, Terraform[i].name_sea, -1, -1); // parse_say
        parse_string(order_str, buf);
        Order[i + 4].order_sea = Strings_put(TextTable, buf);
        // Shortcuts
        Order[i + 4].letter = text_item_string();
        Terraform[i].shortcuts = text_item_string();
    }
    if (text_open(alpha_file(), "RESOURCEINFO")) {
        return true;
    }
    for (int i = 0; i < MaxResourceInfoNum; i++) {
        text_get();
        text_item();
        ResValue* res = (ResValue*)ResInfo;
        res[i].nutrient = text_item_number();
        res[i].mineral = text_item_number();
        res[i].energy = text_item_number();
        res[i].unused = text_item_number();
    }
    if (text_open(alpha_file(), "TIMECONTROLS")) {
        return true;
    }
    for (int i = 0; i < MaxTimeControlNum; i++) {
        text_get();
        TimeControl[i].name = text_item_string();
        TimeControl[i].turn = text_item_number();
        TimeControl[i].base = text_item_number();
        TimeControl[i].unit = text_item_number();
        TimeControl[i].event = text_item_number();
        TimeControl[i].extra = text_item_number();
        TimeControl[i].refresh = text_item_number();
        TimeControl[i].accumulated = text_item_number();
    }
    if (text_open(alpha_file(), "CHASSIS")) {
        return true;
    }
    for (int i = 0; i < MaxChassisNum; i++) {
        text_get();
        Chassis[i].offsv1_name = text_item_string();
        noun_item(&Chassis[i].offsv1_gender, &Chassis[i].offsv1_is_plural);
        Chassis[i].offsv2_name = text_item_string();
        noun_item(&Chassis[i].offsv2_gender, &Chassis[i].offsv2_is_plural);
        Chassis[i].defsv1_name = text_item_string();
        noun_item(&Chassis[i].defsv1_gender, &Chassis[i].defsv1_is_plural);
        Chassis[i].defsv2_name = text_item_string();
        noun_item(&Chassis[i].defsv2_gender, &Chassis[i].defsv2_is_plural);
        Chassis[i].speed = (uint8_t)text_item_number();
        Chassis[i].triad = (uint8_t)text_item_number();
        Chassis[i].range = (uint8_t)text_item_number();
        Chassis[i].missile = (uint8_t)text_item_number();
        Chassis[i].cargo = (uint8_t)text_item_number();
        Chassis[i].cost = (uint8_t)text_item_number();
        Chassis[i].preq_tech = (int16_t)tech_name(text_item());
        Chassis[i].offsv_name_lrg = text_item_string();
        noun_item(&Chassis[i].offsv_gender_lrg, &Chassis[i].offsv_is_plural_lrg);
        Chassis[i].defsv_name_lrg = text_item_string();
        noun_item(&Chassis[i].defsv_gender_lrg, &Chassis[i].defsv_is_plural_lrg);
    }
    if (text_open(alpha_file(), "REACTORS")) {
        return true;
    }
    for (int i = 0; i < MaxReactorNum; i++) {
        text_get();
        Reactor[i].name = text_item_string();
        Reactor[i].name_short = text_item_string();
        // Fix: original function skips power value and is left as zero. It is not
        // referenced elsewhere in code. Likely because default power value is sequential.
        // This will allow future modifications.
        Reactor[i].power = (uint16_t)text_item_number();
        Reactor[i].preq_tech = (int16_t)tech_name(text_item());
    }
    if (text_open(alpha_file(), "WEAPONS")) {
        return true;
    }
    for (int i = 0; i < MaxWeaponNum; i++) {
        text_get();
        Weapon[i].name = text_item_string();
        Weapon[i].name_short = text_item_string();
        Weapon[i].offense_value = (int8_t)text_item_number();
        Weapon[i].mode = (uint8_t)text_item_number();
        Weapon[i].cost = (uint8_t)text_item_number();
        Weapon[i].icon = (int8_t)text_item_number();
        Weapon[i].preq_tech = (int16_t)tech_name(text_item());
    }
    if (text_open(alpha_file(), "DEFENSES")) {
        return true;
    }
    for (int i = 0; i < MaxArmorNum; i++) {
        text_get();
        Armor[i].name = text_item_string();
        Armor[i].name_short = text_item_string();
        Armor[i].defense_value = (char)text_item_number();
        Armor[i].mode = (uint8_t)text_item_number();
        Armor[i].cost = (uint8_t)text_item_number();
        Armor[i].preq_tech = (int16_t)tech_name(text_item());
    }
    if (text_open(alpha_file(), "ABILITIES")) {
        return true;
    }
    for (int i = 0; i < MaxAbilityNum; i++) {
        text_get();
        Ability[i].name = text_item_string();
        Ability[i].cost = text_item_number();
        Ability[i].preq_tech = (int16_t)tech_name(text_item());
        Ability[i].abbreviation = text_item_string();
        Ability[i].flags = text_item_binary();
        Ability[i].description = text_item_string();
    }
    if (text_open(alpha_file(), "MORALE")) {
        return true;
    }
    for (int i = 0; i < MaxMoraleNum; i++) {
        text_get();
        Morale[i].name = text_item_string();
        Morale[i].name_lifecycle = text_item_string();
    }
    if (text_open(alpha_file(), "DEFENSEMODES")) {
        return true;
    }
    for (int i = 0; i < MaxDefenseModeNum; i++) {
        text_get();
        DefenseMode[i].name = text_item_string();
        DefenseMode[i].hyphened = text_item_string();
        DefenseMode[i].abbrev = text_item_string();
        DefenseMode[i].letter = text_item_string();
    }
    if (text_open(alpha_file(), "OFFENSEMODES")) {
        return true;
    }
    for (int i = 0; i < MaxOffenseModeNum; i++) {
        text_get();
        OffenseMode[i].name = text_item_string();
        OffenseMode[i].hyphened = text_item_string();
        OffenseMode[i].abbrev = text_item_string();
        OffenseMode[i].letter = text_item_string();
    }
    // Units basic prototypes (only if new game tgl_all_rules boolean is set)
    // Fix: original version only cleared name, group_id and unit_flags fields (potential bug).
    if (tgl_all_rules) {
        memset(Units, 0, MaxProtoNum * sizeof(UNIT));
        if (read_units()) {
            return true;
        }
    }
    if (text_open(alpha_file(), "FACILITIES")) {
        return true;
    }
    for (int i = 1; i <= SP_ID_Last; i++) { // Facility[0] is not used
        text_get();
        Facility[i].name = text_item_string();
        Facility[i].cost = text_item_number();
        Facility[i].maint = text_item_number();
        Facility[i].preq_tech = tech_name(text_item());
        /*
        Enhancement: The original code explicitly sets this value to disabled (-2) overriding
        alpha/x.txt. It states in #FACILITIES alpha/x.txt: "Free = No longer supported".
        The original AC manual in Appendix 2 and official strategy guide both list the specific
        facilities being free with certain tech. However, this mechanic could have been removed
        for balance reasons. Or maybe was dropped due to time constraints. There is code that
        checks this value and sets the free facility only for new bases built after discovering the
        tech. It looks like existing bases do not get it.
        */
        Facility[i].free_tech = tech_name(text_item());
        Facility[i].effect = text_item_string();
        if (i >= SP_ID_First) {
            Facility[i].AI_fight = text_item_number();
            Facility[i].AI_power = text_item_number();
            Facility[i].AI_tech = text_item_number();
            Facility[i].AI_wealth = text_item_number();
            Facility[i].AI_growth = text_item_number();
        }
    }
    if (text_open(alpha_file(), "ORDERS")) { // Basic
        return true;
    }
    for (int i = 0; i < MaxOrderNum; i++) {
        if (i < 4 || i > 23) { // Skipping over orders set by #TERRAIN
            text_get();
            Order[i].order = text_item_string();
            // Potential enhancement: Have separate string for sea
            Order[i].order_sea = Order[i].order;
            Order[i].letter = text_item_string();
        }
    }
    if (text_open(alpha_file(), "COMPASS")) {
        return true;
    }
    for (int i = 0; i < MaxCompassNum; i++) {
        text_get();
        Compass[i] = text_item_string();
    }
    if (text_open(alpha_file(), "PLANS")) {
        return true;
    }
    for (int i = 0; i < MaxPlanNum; i++) {
        text_get();
        // Future clean-up: Create structure with both short & full name vs split memory
        PlansShortName[i] = text_item_string();
        PlansFullName[i] = text_item_string();
    }
    if (text_open(alpha_file(), "TRIAD")) {
        return true;
    }
    for (int i = 0; i < MaxTriadNum; i++) {
        text_get();
        TriadName[i] = text_item_string();
    }
    if (text_open(alpha_file(), "RESOURCES")) {
        return true;
    }
    for (int i = 0; i < MaxResourceNum; i++) {
        text_get();
        ResName[i].name_singular = text_item_string();
        ResName[i].name_plural = text_item_string();
    }
    if (text_open(alpha_file(), "ENERGY")) {
        return true;
    }
    for (int i = 0; i < MaxEnergyNum; i++) {
        text_get();
        Energy[i].abbrev = text_item_string();
        Energy[i].name = text_item_string();
    }
    if (text_open(alpha_file(), "CITIZENS")) {
        return true;
    }
    for (int i = 0; i < MaxCitizenNum; i++) {
        text_get();
        Citizen[i].singular_name = text_item_string();
        Citizen[i].plural_name = text_item_string();
        if (i < 7) {
            Citizen[i].preq_tech = tech_name(text_item());
            Citizen[i].obsol_tech = tech_name(text_item());
            Citizen[i].econ_bonus = text_item_number();
            Citizen[i].psych_bonus = text_item_number();
            Citizen[i].labs_bonus = text_item_number();
        }
    }
    if (text_open(alpha_file(), "SOCIO")) {
        return true;
    }
    text_get();
    for (int i = 0; i < MaxSocialEffectNum; i++) {
        strcpy_n(SocialParam[i].set1, 24, text_item());
    }
    text_get();
    for (int i = 0; i < MaxSocialEffectNum; i++) {
        strcpy_n(SocialParam[i].set2, 24, text_item());
    }
    text_get();
    for (int i = 0; i < MaxSocialCatNum; i++) {
        SocialField[i].field_name = text_item_string();
    }
    for (int i = 0; i < MaxSocialCatNum; i++) {
        for (int j = 0; j < MaxSocialModelNum; j++) {
            text_get();
            SocialField[i].soc_name[j] = text_item_string();
            SocialField[i].soc_preq_tech[j] = tech_name(text_item());
            memset(&SocialField[i].soc_effect[j], 0, sizeof(CSocialEffect));
            char* mod_value = text_item();
            size_t mod_len = strlen(mod_value);
            while (mod_len) {
                int value = 0;
                while (mod_value[0] == '+' || mod_value[0] == '-') {
                    (mod_value[0] == '+') ? value++ : value--;
                    mod_value++;
                }
                for (int k = 0; k < MaxSocialEffectNum; k++) {
                    if (!_stricmp(mod_value, SocialParam[k].set1)) {
                        *(&SocialField[i].soc_effect[j].economy + k) = value;
                        break;
                    }
                }
                mod_value = text_item();
                mod_len = strlen(mod_value);
            }
        }
    }
    if (text_open(alpha_file(), "DIFF")) { // Difficulty
        return true;
    }
    for (int i = 0; i < MaxDiffNum; i++) {
        text_get();
        Difficulty[i] = text_item_string();
    }
    if (tgl_all_rules && read_factions()) {
        return true;
    }
    if (text_open(alpha_file(), "MANDATE")) {
        return true;
    }
    for (int i = 0; i < MaxMandateNum; i++) {
        text_get();
        Mandate[i].name = text_item_string();
        Mandate[i].name_caps = text_item_string();
    }
    if (text_open(alpha_file(), "MOOD")) {
        return true;
    }
    for (int i = 0; i < MaxMoodNum; i++) {
        text_get();
        Mood[i] = text_item_string();
    }
    if (text_open(alpha_file(), "REPUTE")) {
        return true;
    }
    for (int i = 0; i < MaxReputeNum; i++) {
        text_get();
        Repute[i] = text_item_string();
    }
    if (text_open(alpha_file(), "MIGHT")) {
        return true;
    }
    for (int i = 0; i < MaxMightNum; i++) {
        text_get();
        Might[i].adj_start = text_item_string();
        Might[i].adj = text_item_string();
    }
    if (text_open(alpha_file(), "PROPOSALS")) {
        return true;
    }
    for (int i = 0; i < MaxProposalNum; i++) {
        text_get();
        Proposal[i].name = text_item_string();
        Proposal[i].preq_tech = tech_name(text_item());
        Proposal[i].description = text_item_string();
    }
    if (text_open(alpha_file(), "NATURAL")) {
        return true;
    }
    for (int i = 0; i < MaxNaturalNum; i++) {
        text_get();
        Natural[i].name = text_item_string();
        Natural[i].name_short = text_item_string();
    }
    // Revised original nested loop code for more clarity. Buttons used by "Edit Map" menus.
    for (int i = 0, j = 17; i < MaxTerrainNum; i++) {
        // Excludes: Fungus Removal, Aquifer, Raise Land, Lower Land, Level Terrain
        if (Terraform[i].bit) {
            BaseButton_set_bubble_text(&FlatButtons[j], Terraform[i].name); // 17-31
            j++;
        }
    }
    BaseButton_set_bubble_text(&FlatButtons[32], Natural[2].name_short); // LM_JUNGLE
    BaseButton_set_bubble_text(&FlatButtons[33], Natural[6].name_short); // LM_DUNES
    BaseButton_set_bubble_text(&FlatButtons[34], Natural[3].name_short); // LM_URANIUM
    BaseButton_set_bubble_text(&FlatButtons[35], Natural[10].name_short); // LM_GEOTHERMAL
    text_close(); // Make sure file handle is closed
    return false;
}

char* __cdecl prefs_get_strcpy(char* dst, const char* src) {
    if ((dst == &ParseStrBuffer[0].str[0] || dst == &ParseStrBuffer[1].str[0])
    && src == *TextBufferGetPtr) {
        return strcpy_n(dst, StrBufLen, Text.SrcPtr);
    }
    return strcpy_n(dst, StrBufLen, src);
}

/*
Deprecated function. Attempt to read the setting string value from the ini file.
*/
char* __cdecl prefs_get2(const char* key_name, const char* default_value, int use_default) {
    if (use_default || GetPrivateProfileIntA(GameAppName, "Prefs Format", 0, GameIniFile) != 12) {
        snprintf(Text.SrcPtr, 256, "%s", default_value);
    } else {
        GetPrivateProfileStringA(GameAppName, key_name, default_value, Text.SrcPtr, 256, GameIniFile);
    }
    return text_update();
}

/*
Attempt to read the setting integer value from the ini file. This version removes
references on Text_update / TextBufferGetPtr because the result is returned as integer.
*/
int __cdecl prefs_get(const char* key_name, int default_value, int use_default) {
    if (conf.directdraw >= 0 && !strcmp(key_name, "DirectDraw")) {
        return (conf.directdraw ? 1 : 0);
    }
    if (conf.disable_opening_movie >= 0 && !strcmp(key_name, "DisableOpeningMovie")) {
        return (conf.disable_opening_movie ? 1 : 0);
    }
    if (use_default) {
        return default_value;
    } else {
        return GetPrivateProfileIntA(GameAppName, key_name, default_value, GameIniFile);
    }
}

void prefs_read(char* buf, size_t buf_len, const char* key_name, const char* default_value, int use_default) {
    buf[0] = '\0';
    if (use_default || GetPrivateProfileIntA(GameAppName, "Prefs Format", 0, GameIniFile) != 12) {
        snprintf(buf, buf_len, "%s", default_value);
    } else {
        GetPrivateProfileStringA(GameAppName, key_name, default_value, buf, buf_len, GameIniFile);
    }
}

/*
Get the default value for the 1st set of preferences.
*/
uint32_t __cdecl default_prefs() {
    uint32_t base_prefs = DefaultBasePref;
    if (conf.ignore_reactor_power) {
        base_prefs &= ~PREF_BSC_AUTO_DESIGN_VEH;
    }
    return prefs_get("Laptop", 0, false) ? base_prefs :
        base_prefs | PREF_AV_SECRET_PROJECT_MOVIES | PREF_AV_SLIDING_WINDOWS | PREF_AV_MAP_ANIMATIONS;
}

/*
Get the default value for the 2nd set of preferences.
*/
uint32_t __cdecl default_prefs2() {
    uint32_t base_prefs2 = DefaultMorePref;
    if (conf.ignore_reactor_power) {
        // Skip possibly redundant upgrade prototype dialogs when new reactor techs are discovered
        base_prefs2 &= ~MPREF_BSC_AUTO_PRUNE_OBS_VEH;
    }
    return prefs_get("Laptop", 0, false) ? base_prefs2 : base_prefs2 | MPREF_AV_SLIDING_SCROLLBARS;
}

/*
Get the default value for the warning popup preferences.
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
Read the faction filenames and search for keys from the ini file (SMACX only). This has
the added effect of forcing the player's search_key to be set to the filename value.
Contains multiple rewrites. Original version checked for ExpansionEnabled but this is not
necessary because smac_only allows changing active faction lists.
*/
void __cdecl prefs_fac_load() {
    if (GetPrivateProfileIntA(GameAppName, "Prefs Format", 0, GameIniFile) == 12) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            char buf[LineBufLen] = {};
            char name_buf[LineBufLen] = {};
            snprintf(name_buf, LineBufLen, "Faction %d", i);
            GetPrivateProfileStringA(GameAppName, name_buf, MFactions[i].filename, buf, LineBufLen, GameIniFile);
            strcpy_n(MFactions[i].filename, 24, buf);
            strcpy_n(MFactions[i].search_key, 24, buf);
            text_update();
        }
    } else {
        // use separate loop rather than check "Prefs Format" value each time in single loop
        for (int i = 1; i < MaxPlayerNum; i++) {
            strcpy_n(MFactions[i].search_key, 24, MFactions[i].filename);
        }
    }
}

/*
Load the most common preferences from the game's ini to globals.
*/
void __cdecl prefs_load(int use_default) {
    char dst[StrBufLen];
    char src[StrBufLen];
    set_language(prefs_get("Language", 0, false));
    DefaultPrefs->difficulty = prefs_get("Difficulty", 0, false);
    DefaultPrefs->map_type = prefs_get("Map Type", 0, false);
    DefaultPrefs->top_menu = prefs_get("Top Menu", 0, false);
    DefaultPrefs->faction_id = prefs_get("Faction", 1, false);
    uint32_t prefs = default_prefs();
    if (DefaultPrefs->difficulty < DIFF_TALENT) {
        prefs |= PREF_BSC_TUTORIAL_MSGS;
    }
    prefs_read(dst, StrBufLen, "Preferences", prefs_get_binary(src, prefs), use_default);
    AlphaIniPrefs->preferences = btoi(strtrim(dst));
    prefs_read(dst, StrBufLen, "More Preferences", prefs_get_binary(src, default_prefs2()), use_default);
    AlphaIniPrefs->more_preferences = btoi(strtrim(dst));
    prefs_read(dst, StrBufLen, "Semaphore", "00000000", use_default);
    AlphaIniPrefs->semaphore = btoi(strtrim(dst));
    AlphaIniPrefs->customize = prefs_get("Customize", 0, false);
    prefs_read(dst, StrBufLen, "Rules", prefs_get_binary(src, default_rules()), use_default);
    AlphaIniPrefs->rules = btoi(strtrim(dst));
    prefs_read(dst, StrBufLen, "Announce", prefs_get_binary(src, default_warn()), use_default);
    AlphaIniPrefs->announce = btoi(strtrim(dst));
    prefs_read(dst, StrBufLen, "Custom World", "2, 1, 1, 1, 1, 1, 1,", use_default);
    opt_list_parse(&AlphaIniPrefs->custom_world[0], dst, 7, 0);
    AlphaIniPrefs->time_controls = prefs_get("Time Controls", 1, use_default);
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
    char buf[LineBufLen] = {};
    if (tgl_binary) {
        prefs_get_binary(buf, value);
    } else {
        snprintf(buf, LineBufLen, "%d", value);
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



