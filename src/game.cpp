
#include "game.h"

static uint32_t custom_game_rules = 0;
static uint32_t custom_more_rules = 0;


bool un_charter() {
    return *UNCharterRepeals <= *UNCharterReinstates;
}

bool victory_done() {
    // TODO: Check for scenario victory conditions
    return *GameState & (STATE_VICTORY_CONQUER | STATE_VICTORY_DIPLOMATIC | STATE_VICTORY_ECONOMIC)
        || has_project(FAC_ASCENT_TO_TRANSCENDENCE, -1);
}

bool valid_player(int faction) {
    return faction > 0 && faction < MaxPlayerNum;
}

bool valid_triad(int triad) {
    return (triad == TRIAD_LAND || triad == TRIAD_SEA || triad == TRIAD_AIR);
}

int __cdecl game_start_turn() {
    // TODO: If config is changed, may return incorrect start turn
    return min(*CurrentTurn, (*GameRules & RULES_TIME_WARP ? conf.time_warp_start_turn : 0));
}

int __cdecl game_year(int n) {
    return Rules->normal_start_year + n;
}

bool __cdecl in_box(int x, int y, RECT* rc) {
    return x >= rc->left && x < rc->right
        && y >= rc->top && y < rc->bottom;
}

/*
Calculate the offset and bitmask for the specified input.
*/
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask) {
    *offset = input / 8;
    *mask = 1 << (input & 7);
}

/*
Calculate nutrient/mineral cost factors for base production.
In vanilla game mechanics, if the player faction is ranked first, then the AIs
will get additional growth/industry bonuses. This modified version removes them.
Original Offset: 004E4430
*/
int __cdecl mod_cost_factor(int faction_id, int is_mineral, int base_id) {
    int value;
    int multiplier = (is_mineral ? Rules->mineral_cost_multi : Rules->nutrient_cost_multi);

    if (is_human(faction_id)) {
        value = multiplier;
    } else {
        value = multiplier * CostRatios[*DiffLevel] / 10;
    }

    if (*MapSizePlanet == 0) {
        value = 8 * value / 10;
    } else if (*MapSizePlanet == 1) {
        value = 9 * value / 10;
    }
    if (is_mineral) {
        if (is_mineral == 1) {
            switch (Factions[faction_id].SE_industry_pending) {
                case -7:
                case -6:
                case -5:
                case -4:
                case -3:
                    return (13 * value + 9) / 10;
                case -2:
                    return (6 * value + 4) / 5;
                case -1:
                    return (11 * value + 9) / 10;
                case 0:
                    break;
                case 1:
                    return (9 * value + 9) / 10;
                case 2:
                    return (4 * value + 4) / 5;
                case 3:
                    return (7 * value + 9) / 10;
                case 4:
                    return (3 * value + 4) / 5;
                default:
                    return (value + 1) / 2;
            }
        }
        return value;
    } else {
        int growth = Factions[faction_id].SE_growth_pending;
        if (base_id >= 0) {
            if (has_facility(FAC_CHILDREN_CRECHE, base_id)) {
                growth += 2;
            }
            if (Bases[base_id].state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
                growth += 2;
            }
        }
        return (value * (10 - clamp(growth, -2, 5)) + 9) / 10;
    }
}

void init_world_config() {
    ThinkerVars->game_time_spent = 0;
    /*
    Adjust Special Scenario Rules if any are selected from the mod menu.
    This also overrides settings for any scenarions unless all custom options are left empty.
    */
    if ((custom_game_rules || custom_more_rules) && !*MultiplayerActive) {
        *GameRules = (*GameRules & GAME_RULES_MASK) | custom_game_rules;
        *GameMoreRules = (*GameMoreRules & GAME_MRULES_MASK) | custom_more_rules;
    }
}

void show_rules_menu() {
    *GameRules = (*GameRules & GAME_RULES_MASK) | custom_game_rules;
    *GameMoreRules = (*GameMoreRules & GAME_MRULES_MASK) | custom_more_rules;
    Console_editor_scen_rules(MapWin);
    custom_game_rules = *GameRules & ~GAME_RULES_MASK;
    custom_more_rules = *GameMoreRules & ~GAME_MRULES_MASK;
}

void init_save_game(int faction) {
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    if (!faction) {
        return;
    }
    if (m->thinker_header != THINKER_HEADER) {
        m->thinker_header = THINKER_HEADER;
        m->thinker_flags = 0;
        m->thinker_tech_id = f->tech_research_id;
        m->thinker_tech_cost = f->tech_cost;
        m->thinker_probe_lost = 0;
        m->thinker_last_mc_turn = 0;
    }
    m->thinker_unused = 0;
    /*
    Remove invalid prototypes from the savegame.
    This also attempts to repair invalid vehicle stacks to prevent game crashes.
    Stack iterators should never contain infinite loops or vehicles from multiple tiles.
    */
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        int unit_id = faction*MaxProtoFactionNum + i;
        UNIT* u = &Units[unit_id];
        if (strlen(u->name) >= MaxProtoNameLen
        || u->chassis_id < CHS_INFANTRY
        || u->chassis_id > CHS_MISSILE) {
            for (int j = *VehCount-1; j >= 0; j--) {
                if (Vehicles[j].unit_id == unit_id) {
                    veh_kill(j);
                }
            }
            for (int j = 0; j < *BaseCount; j++) {
                if (Bases[j].queue_items[0] == unit_id) {
                    Bases[j].queue_items[0] = -FAC_STOCKPILE_ENERGY;
                }
            }
            memset(u, 0, sizeof(UNIT));
        }
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        int prev_id = veh->prev_veh_id_stack;
        int next_id = veh->next_veh_id_stack;
        bool adjust = false;

        if (prev_id == i || (prev_id >= 0 && (Vehs[prev_id].x != veh->x || Vehs[prev_id].y != veh->y))) {
            print_veh_stack(veh->x, veh->y);
            veh->prev_veh_id_stack = -1;
            adjust = true;
        }
        if (next_id == i || (next_id >= 0 && (Vehs[next_id].x != veh->x || Vehs[next_id].y != veh->y))) {
            print_veh_stack(veh->x, veh->y);
            veh->next_veh_id_stack = -1;
            adjust = true;
        }
        if (adjust) {
            stack_fix(i);
        }
    }
}

int __cdecl mod_turn_upkeep() {
    // Turn number is incremented in turn_upkeep
    if (*CurrentTurn == 0) {
        init_world_config();
    }
    turn_upkeep();
    debug("turn_upkeep %d bases: %d vehicles: %d\n",
        *CurrentTurn, *BaseCount, *VehCount);

    if (DEBUG) {
        if (conf.debug_mode) {
            *GameState |= STATE_DEBUG_MODE;
            *GamePreferences |= PREF_ADV_FAST_BATTLE_RESOLUTION;
        } else {
            *GameState &= ~STATE_DEBUG_MODE;
        }
    }
    snprintf(ThinkerVars->build_date, 12, MOD_DATE);
    return 0;
}

/*
Original Offset: 005ABD20
*/
void __cdecl mod_auto_save() {
    if ((!*PbemActive || *MultiplayerActive)
    && (!(*GameRules & RULES_IRONMAN) || *GameState & STATE_SCENARIO_EDITOR)) {
        if (conf.autosave_interval > 0 && !(*CurrentTurn % conf.autosave_interval)) {
            char buf[256];
            snprintf(buf, sizeof(buf), "saves/auto/Autosave_%d.sav", game_year(*CurrentTurn));
            save_daemon(buf);
        }
    }
}

/*
Original Offset: 00527290
*/
int __cdecl mod_faction_upkeep(int faction) {
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    debug("faction_upkeep %d %d\n", *CurrentTurn, faction);

    if (conf.factions_enabled > 0 && (*MapAreaX > MaxMapAreaX || *MapAreaY > MaxMapAreaY)) {
        parse_says(0, MOD_VERSION, -1, -1);
        parse_says(1, "This map exceeds Thinker's maximum supported map size.", -1, -1);
        popp("modmenu", "GENERIC", 0, 0, 0);
        *ControlTurnA = 1; // Return to main menu
        *ControlTurnB = 1;
    }
    init_save_game(faction);
    plans_upkeep(faction);
    reset_netmsg_status();
    *ControlUpkeepA = 1;
    social_upkeep(faction);
    do_all_non_input();

    if (conf.activate_skipped_units) {
        for (int i = 0; i < *VehCount; i++) {
            if (Vehs[i].faction_id == faction) {
                Vehs[i].flags &= ~VFLAG_FULL_MOVE_SKIPPED;
            }
        }
    }
    repair_phase(faction);
    do_all_non_input();
    production_phase(faction);
    do_all_non_input();
    if (!(*GameState & STATE_GAME_DONE) || *GameState & STATE_FINAL_SCORE_DONE) {
        allocate_energy(faction);
        do_all_non_input();
        enemy_diplomacy(faction);
        do_all_non_input();
        enemy_strategy(faction);
        do_all_non_input();
        /*
        Thinker-specific AI planning routines.
        Note that move_upkeep is only updated after all the production is done,
        so that the movement code can utilize up-to-date priority maps.
        This means we cannot use move_upkeep maps or variables in production phase.
        */
        mod_social_ai(faction, -1, -1, -1, -1, 0);
        probe_upkeep(faction);
        move_upkeep(faction, UM_Full);
        do_all_non_input();

        if (!is_human(faction)) {
            int cost = corner_market(faction);
            if (!victory_done() && f->energy_credits > cost && f->corner_market_active < 1
            && has_tech(Rules->tech_preq_economic_victory, faction)
            && *GameRules & RULES_VICTORY_ECONOMIC) {
                f->corner_market_turn = *CurrentTurn + Rules->turns_corner_global_energy_market;
                f->corner_market_active = cost;
                f->energy_credits -= cost;

                *gender_default = m->noun_gender;
                *plurality_default = 0;
                parse_says(0, m->title_leader, -1, -1);
                parse_says(1, m->name_leader, -1, -1);
                *plurality_default = m->is_noun_plural;
                parse_says(2, m->noun_faction, -1, -1);
                parse_says(3, m->adj_name_faction, -1, -1);
                ParseNumTable[0] = game_year(f->corner_market_turn);
                popp("SCRIPT", "CORNERWARNING", 0, "econwin_sm.pcx", 0);
            }
        }
    }
    for (int i = 0; i<*BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction) {
            base->state_flags &= ~(BSTATE_UNK_1 | BSTATE_HURRY_PRODUCTION);
        }
    }
    f->energy_credits -= f->energy_cost;
    f->energy_cost = 0;
    if (f->energy_credits < 0) {
        f->energy_credits = 0;
    }
    if (!f->base_count && !has_colony_pods(faction)) {
        eliminate_player(faction, 0);
    }
    *ControlUpkeepA = 0;
    Path->xDst = -1;
    Path->yDst = -1;

    if (!(*GameState & STATE_GAME_DONE) || *GameState & STATE_FINAL_SCORE_DONE) {
        if (faction == MapWin->cOwner
        && !(*GameState & (STATE_COUNCIL_HAS_CONVENED | STATE_DISPLAYED_COUNCIL_AVAIL_MSG))
        && can_call_council(faction, 0) && !(*GameState & STATE_GAME_DONE)) {
            *GameState |= STATE_DISPLAYED_COUNCIL_AVAIL_MSG;
            popp("SCRIPT", "COUNCILOPEN", 0, "council_sm.pcx", 0);
        }
        if (!is_human(faction)) {
            call_council(faction);
        }
    }
    if (!*MultiplayerActive && *GamePreferences & PREF_BSC_AUTOSAVE_EACH_TURN
    && faction == MapWin->cOwner) {
        auto_save();
    }
    flushlog();
    return 0;
}

uint32_t offset_next(int32_t faction, uint32_t position, uint32_t amount) {
    if (!position) {
        return 0;
    }
    uint32_t loop = 0;
    uint32_t offset = ((*MapRandomSeed + faction) & 0xFE) | 1;
    do {
        if (offset & 1) {
            offset ^= 0x170;
        }
        offset >>= 1;
    } while (offset >= amount || ++loop != position);
    return offset;
}

/*
Generate a base name. Uses additional base names from basenames folder.
When faction base names are used, creates additional variations from basenames/generic.txt.
First land/sea base always uses the first available name from land/sea names list.
Vanilla name_base chooses sea base names in a sequential non-random order (this version is random).
Original Offset: 004E4090
*/
void __cdecl mod_name_base(int faction, char* name, bool save_offset, bool sea_base) {
    Faction& f = Factions[faction];
    uint32_t offset = f.base_name_offset;
    uint32_t offset_sea = f.base_sea_name_offset;
    const int buf_size = 1024;
    char file_name_1[buf_size];
    char file_name_2[buf_size];
    set_str_t all_names;
    vec_str_t sea_names;
    vec_str_t land_names;
    snprintf(file_name_1, buf_size, "%s.txt", MFactions[faction].filename);
    snprintf(file_name_2, buf_size, "basenames\\%s.txt", MFactions[faction].filename);
    strlwr(file_name_1);
    strlwr(file_name_2);

    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction) {
            all_names.insert(Bases[i].name);
        }
    }
    if (sea_base) {
        reader_path(sea_names, file_name_1, "#WATERBASES", MaxBaseNameLen);
        reader_path(sea_names, file_name_2, "#WATERBASES", MaxBaseNameLen);

        if (sea_names.size() > 0 && offset_sea < sea_names.size()) {
            for (uint32_t i = 0; i < sea_names.size(); i++) {
                uint32_t seed = offset_next(faction, offset_sea + i, sea_names.size());
                if (seed < sea_names.size()) {
                    vec_str_t::const_iterator it(sea_names.begin());
                    std::advance(it, seed);
                    if (!has_item(all_names, it->c_str())) {
                        strncpy(name, it->c_str(), MaxBaseNameLen);
                        name[MaxBaseNameLen - 1] = '\0';
                        if (save_offset) {
                            f.base_sea_name_offset++;
                        }
                        return;
                    }
                }
            }
        }
    }
    land_names.clear();
    reader_path(land_names, file_name_1, "#BASES", MaxBaseNameLen);
    reader_path(land_names, file_name_2, "#BASES", MaxBaseNameLen);

    if (save_offset) {
        f.base_name_offset++;
    }
    if (land_names.size() > 0 && offset < land_names.size()) {
        for (uint32_t i = 0; i < land_names.size(); i++) {
            uint32_t seed = offset_next(faction, offset + i, land_names.size());
            if (seed < land_names.size()) {
                vec_str_t::const_iterator it(land_names.begin());
                std::advance(it, seed);
                if (!has_item(all_names, it->c_str())) {
                    strncpy(name, it->c_str(), MaxBaseNameLen);
                    name[MaxBaseNameLen - 1] = '\0';
                    return;
                }
            }
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        all_names.insert(Bases[i].name);
    }
    uint32_t x = 0;
    uint32_t a = 0;
    uint32_t b = 0;

    land_names.clear();
    sea_names.clear();
    reader_path(land_names, "basenames\\generic.txt", "#BASESA", MaxBaseNameLen);
    if (land_names.size() > 0) {
        reader_path(sea_names, "basenames\\generic.txt", "#BASESB", MaxBaseNameLen);
    } else {
        reader_path(land_names, "basename.txt", "#GENERIC", MaxBaseNameLen);
    }

    for (int i = 0; i < 2*MaxBaseNum && land_names.size() > 0; i++) {
        x = pair_hash(faction + MaxPlayerNum*f.base_name_offset, *MapRandomSeed + i);
        a = ((x & 0xffff) * land_names.size()) >> 16;
        name[0] = '\0';

        if (sea_names.size() > 0) {
            b = ((x >> 16) * sea_names.size()) >> 16;
            snprintf(name, MaxBaseNameLen, "%s %s", land_names[a].c_str(), sea_names[b].c_str());
        } else {
            snprintf(name, MaxBaseNameLen, "%s", land_names[a].c_str());
        }
        if (strlen(name) >= 2 && !all_names.count(name)) {
            return;
        }
    }
    int i = *BaseCount;
    while (i < 2*MaxBaseNum) {
        i++;
        name[0] = '\0';
        snprintf(name, MaxBaseNameLen, "Sector %d", i);
        if (!all_names.count(name)) {
            return;
        }
    }
}

int probe_roll_value(int faction)
{
    int techs = 0;
    for (int i = Tech_ID_First; i <= Tech_ID_Last; i++) {
        if (Tech[i].preq_tech1 != TECH_Disable && has_tech(i, faction)
        && Tech[i].flags & TFLAG_IMPROVE_PROBE) {
            techs++;
        }
    }
    return 2*techs + 2*clamp(Factions[faction].SE_probe, -3, 3)
        + clamp(Factions[faction].SE_probe_base, -3, 3)
        + clamp(Factions[faction].SE_police, -3, 3);
}

int probe_active_turns(int faction1, int faction2)
{
    int value = clamp(15 + probe_roll_value(faction1) - probe_roll_value(faction2), 5, 50);
    value = value * (4 + (*MapAreaTiles >= 4000) + (*MapAreaTiles >= 8000)) / 4;
    value = value * (4 + (*DiffLevel < DIFF_TRANSCEND) + (*DiffLevel < DIFF_THINKER)) / 4;
    return clamp(value, 5, 50);
}

int probe_upkeep(int faction1)
{
    if (faction1 <= 0 || !is_alive(faction1) || !conf.counter_espionage) {
        return 0;
    }
    /*
    Do not expire infiltration while the faction is the governor or has the Empath Guild.
    Status can be renewed once per turn and sets the flag DIPLO_RENEW_INFILTRATOR.
    This is checked in patched version of probe() game code.
    */
    for (int faction2 = 1; faction2 < MaxPlayerNum; faction2++) {
        if (faction1 != faction2 && is_alive(faction2)
        && faction2 != *GovernorFaction && !has_project(FAC_EMPATH_GUILD, faction2)) {
            if (has_treaty(faction2, faction1, DIPLO_HAVE_INFILTRATOR)) {
                if (!MFactions[faction2].thinker_probe_end_turn[faction1]) {
                    MFactions[faction2].thinker_probe_end_turn[faction1] =
                        *CurrentTurn + probe_active_turns(faction2, faction1);
                }
                if (MFactions[faction2].thinker_probe_end_turn[faction1] <= *CurrentTurn) {
                    set_treaty(faction2, faction1, DIPLO_HAVE_INFILTRATOR, 0);
                    MFactions[faction2].thinker_probe_lost |= (1 << faction1);
                    if (faction1 == MapWin->cOwner) {
                        parse_says(0, MFactions[faction2].adj_name_faction, -1, -1);
                        popp("modmenu", "SPYFOUND", 0, "infil_sm.pcx", 0);
                    }
                }
            }
        }
    }
    for (int faction2 = 1; faction2 < MaxPlayerNum; faction2++) {
        if (faction1 != faction2 && is_alive(faction2)) {
            debug("probe_upkeep %3d %d %d spying: %d ends: %d\n",
                *CurrentTurn, faction1, faction2,
                has_treaty(faction1, faction2, DIPLO_HAVE_INFILTRATOR),
                MFactions[faction1].thinker_probe_end_turn[faction2]
            );
            if (faction1 != *GovernorFaction && !has_project(FAC_EMPATH_GUILD, faction1)) {
                set_treaty(faction1, faction2, DIPLO_RENEW_INFILTRATOR, 0);
            }
            if (faction1 == MapWin->cOwner && MFactions[faction1].thinker_probe_lost & (1 << faction2)) {
                parse_says(0, MFactions[faction2].noun_faction, -1, -1);
                popp("modmenu", "SPYLOST", 0, "capture_sm.pcx", 0);
            }
        }
    }
    MFactions[faction1].thinker_probe_lost = 0;
    return 0;
}

int __thiscall probe_popup_start(Win* This, int veh_id1, int base_id, int a4, int a5, int a6, int a7)
{
    if (base_id >= 0 && base_id < *BaseCount) {
        int faction1 = Vehs[veh_id1].faction_id;
        int faction2 = Bases[base_id].faction_id;
        int turns = MFactions[faction1].thinker_probe_end_turn[faction2] - *CurrentTurn;
        bool always_active = faction1 == *GovernorFaction || has_project(FAC_EMPATH_GUILD, faction1);

        if (!always_active) {
            if (has_treaty(faction1, faction2, DIPLO_HAVE_INFILTRATOR) && turns > 0) {
                ParseNumTable[0] =  turns;
                return Popup_start(This, "modmenu", "PROBE", a4, a5, a6, a7);
            }
            // Sometimes this flag is set even when infiltration is not active
            set_treaty(faction1, faction2, DIPLO_RENEW_INFILTRATOR, 0);
        }
    }
    return Popup_start(This, ScriptFile, "PROBE", a4, a5, a6, a7);
}

/*
Game engine uses these defaults for ambient music tracks.
This patches load_music comparison function such that it maps
the user config to any available music tracks.
If none of the config values match current faction, aset1.amb is used.

DEFAULT, aset1.amb
GAIANS, gset1.amb
HIVE, mset1.amb
UNIV, uset2.amb
MORGAN, mset1.amb
SPARTANS, sset1.amb
BELIEVE, bset1.amb
PEACE, gset1.amb
*/

int __cdecl load_music_strcmpi(const char* active, const char* label)
{
    bool fallback = true;
    char lookup[StrBufLen] = {};

    for (const auto& pair : musiclabels) {
        if (!strcmpi(pair.first.c_str(), active)) {
            if (!strcmpi(pair.second.c_str(), "gset1.amb")) {
                strncpy(lookup, "gaians", StrBufLen);
            } else if (!strcmpi(pair.second.c_str(), "mset1.amb")) {
                strncpy(lookup, "hive", StrBufLen);
            } else if (!strcmpi(pair.second.c_str(), "uset2.amb")) {
                strncpy(lookup, "univ", StrBufLen);
            } else if (!strcmpi(pair.second.c_str(), "sset1.amb")) {
                strncpy(lookup, "spartans", StrBufLen);
            } else if (!strcmpi(pair.second.c_str(), "bset1.amb")) {
                strncpy(lookup, "believe", StrBufLen);
            } else {
                break; // aset1.amb or incorrect filename is used
            }
            fallback = false;
            break;
        }
    }
    debug("load_music %d %s %s %s\n", fallback, active, label, lookup);
    flushlog();
    if (fallback) {
        return 1; // Skip all other choices and select original fallback default
    }
    return strcmpi(label, lookup);
}


