
#include "engine.h"

static int current_attacker_veh_id = -1;
static int current_defender_veh_id = -1;


void init_save_game(int faction) {
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    if (m->thinker_header != THINKER_HEADER) {
        // Clear old save game storage locations
        if (f->old_thinker_header == THINKER_HEADER) {
            memset(&f->old_thinker_header, 0, 12);
        }
        m->thinker_header = THINKER_HEADER;
        m->thinker_flags = 0;
        m->thinker_tech_id = f->tech_research_id;
        m->thinker_tech_cost = f->tech_cost;
        m->thinker_probe_lost = 0;
        m->thinker_last_mc_turn = 0;
    }
    m->thinker_unused = 0;
    assert(f->old_thinker_header != THINKER_HEADER);
}

int __cdecl mod_turn_upkeep() {
    turn_upkeep();
    debug("turn_upkeep %d bases: %d vehicles: %d\n",
        *current_turn, *total_num_bases, *total_num_vehicles);

    if (DEBUG) {
        if (conf.debug_mode) {
            *game_state |= STATE_DEBUG_MODE;
            *game_preferences |= PREF_ADV_FAST_BATTLE_RESOLUTION;
            *game_more_preferences |=
                (MPREF_ADV_QUICK_MOVE_VEH_ORDERS | MPREF_ADV_QUICK_MOVE_ALL_VEH);
        } else {
            *game_state &= ~STATE_DEBUG_MODE;
        }
    }
    if (*current_turn == 1 && conf.ignore_reactor_power) {
        *game_preferences &= ~PREF_BSC_AUTO_DESIGN_VEH;
        *game_more_preferences &= ~MPREF_BSC_AUTO_PRUNE_OBS_VEH;
    }
    if (*current_turn == 1) {
        ThinkerVars->game_time_spent = 0;
    }
    snprintf(ThinkerVars->build_date, 12, MOD_DATE);
    return 0;
}

/*
Original Offset: 005ABD20
*/
void __cdecl mod_auto_save() {
    if ((!*pbem_active || *multiplayer_active)
    && (!(*game_rules & RULES_IRONMAN) || *game_state & STATE_SCENARIO_EDITOR)) {
        if (conf.autosave_interval > 0 && !(*current_turn % conf.autosave_interval)) {
            char buf[256];
            snprintf(buf, sizeof(buf), "saves/auto/Autosave_%d.sav", game_year(*current_turn));
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
    debug("faction_upkeep %d %d\n", *current_turn, faction);

    if (conf.factions_enabled > 0 && (*map_axis_x > MaxMapW || *map_axis_y > MaxMapH)) {
        parse_says(0, MOD_VERSION, -1, -1);
        parse_says(1, "This map exceeds Thinker's maximum supported map size.", -1, -1);
        popp("modmenu", "GENERIC", 0, 0, 0);
        *ControlTurnA = 1; // Return to main menu
        *ControlTurnB = 1;
    }
    if (faction > 0) {
        init_save_game(faction);
        plans_upkeep(faction);
    }
    *ControlUpkeepA = 1;
    social_upkeep(faction);
    do_all_non_input();
    repair_phase(faction);
    do_all_non_input();
    production_phase(faction);
    do_all_non_input();
    if (!(*game_state & STATE_GAME_DONE) || *game_state & STATE_FINAL_SCORE_DONE) {
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
        move_upkeep(faction, M_Full);
        do_all_non_input();

        if (!is_human(faction)) {
            int cost = corner_market(faction);
            if (!victory_done() && f->energy_credits > cost && f->corner_market_active < 1
            && has_tech(faction, Rules->tech_preq_economic_victory)
            && *game_rules & RULES_VICTORY_ECONOMIC) {
                f->corner_market_turn = *current_turn + Rules->turns_corner_global_energy_market;
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
    for (int i=0; i<*total_num_bases; i++) {
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

    if (!(*game_state & STATE_GAME_DONE) || *game_state & STATE_FINAL_SCORE_DONE) {
        if (faction == MapWin->cOwner
        && !(*game_state & (STATE_COUNCIL_HAS_CONVENED | STATE_DISPLAYED_COUNCIL_AVAIL_MSG))
        && can_call_council(faction, 0) && !(*game_state & STATE_GAME_DONE)) {
            *game_state |= STATE_DISPLAYED_COUNCIL_AVAIL_MSG;
            popp("SCRIPT", "COUNCILOPEN", 0, "council_sm.pcx", 0);
        }
        if (!is_human(faction)) {
            call_council(faction);
        }
    }
    if (!*multiplayer_active && *game_preferences & PREF_BSC_AUTOSAVE_EACH_TURN
    && faction == MapWin->cOwner) {
        auto_save();
    }
    flushlog();
    return 0;
}

/*
Original Offset: 004E3D50
*/
int __cdecl mod_base_find3(int x, int y, int faction1, int region, int faction2, int faction3) {
    int dist = 9999;
    int result = -1;
    bool border_fix = conf.territory_border_fix && region >= MaxRegionNum/2;

    for (int i=0; i<*total_num_bases; ++i) {
        BASE* base = &Bases[i];
        MAP* bsq = mapsq(base->x, base->y);

        if (bsq && (region < 0 || bsq->region == region || border_fix)) {
            if ((faction1 < 0 && (faction2 < 0 || faction2 != base->faction_id))
            || (faction1 == base->faction_id)
            || (faction2 == -2 && Factions[faction1].diplo_status[base->faction_id] & DIPLO_PACT)
            || (faction2 >= 0 && faction2 == base->faction_id)) {
                if (faction3 < 0 || base->faction_id == faction3 || base->visibility & (1 << faction3)) {
                    int val = vector_dist(x, y, base->x, base->y);
                    if (conf.territory_border_fix ? val < dist : val <= dist) {
                        dist = val;
                        result = i;
                    }
                }
            }
        }
    }
    if (DEBUG && !conf.territory_border_fix) {
        int res = base_find3(x, y, faction1, region, faction2, faction3);
        debug("base_find3 x: %2d y: %2d r: %2d %2d %2d %2d %2d %4d\n",
              x, y, region, faction1, faction2, faction3, result, dist);
        assert(res == result);
        assert(*base_find_dist == dist);
    }
    *base_find_dist = 9999;
    if (result >= 0) {
        *base_find_dist = dist;
    }
    return result;
}

std::vector<std::string> read_txt_block(const char* filename, const char* section, unsigned max_len) {
    std::vector<std::string> lines;
    FILE* file;
    const int buf_size = 1024;
    char filename_txt[buf_size];
    char line[buf_size];
    bool start = false;
    snprintf(filename_txt, buf_size, "%s.txt", filename);
    file = fopen(filename_txt, "r");
    if (!file) {
        return lines;
    }
    while (fgets(line, buf_size, file) != NULL) {
        line[strlen(line) - 1] = '\0';
        if (!start && stricmp(line, section) == 0) {
            start = true;
        } else if (start && line[0] == '#') {
            break;
        } else if (start) {
            char* p = strstrip(line);
            // Truncate long lines to max_len (including null)
            if (strlen(p) > 0 && line[0] != ';') {
                if (strlen(p) >= max_len) {
                    p[max_len - 1] = '\0';
                }
                lines.push_back(p);
            }
        }
    }
    fclose(file);
    return lines;
}

/*
Generate a base name. If all base names from faction definition are used, the name
generated will be unique across all bases on the map, e.g. Alpha Beta Sector.
First land/sea base always uses the first available name from land/sea names list.
Vanilla version chooses sea base names in a sequential non-random order.
Original Offset: 004E4090
*/
void __cdecl mod_name_base(int faction, char* name, bool save_offset, bool sea_base) {
    const int MaxWordsNum = 24;
    const char* words[MaxWordsNum] = {
        "Alpha",
        "Beta",
        "Gamma",
        "Delta",
        "Epsilon",
        "Zeta",
        "Eta",
        "Theta",
        "Iota",
        "Kappa",
        "Lambda",
        "Mu",
        "Nu",
        "Xi",
        "Omicron",
        "Pi",
        "Rho",
        "Sigma",
        "Tau",
        "Upsilon",
        "Phi",
        "Chi",
        "Psi",
        "Omega"
    };
    Faction& f = Factions[faction];
    uint32_t offset = f.base_name_offset;
    uint32_t offset_sea = f.base_sea_name_offset;

    if (sea_base) {
        std::vector<std::string> sea_names = read_txt_block(
            MFactions[faction].filename, "#WATERBASES", MaxBaseNameLen);
        if (sea_names.size() > 0) {
            if (offset_sea > 0 && offset_sea < sea_names.size()) {
                uint32_t seed = ((*map_random_seed + faction) & 0xFE) | 1;
                uint32_t loop = 0;
                do {
                    if (seed & 1) {
                        seed ^= 0x170;
                    }
                    seed >>= 1;
                } while (seed >= sea_names.size() || ++loop != offset_sea);
                offset_sea = seed;
            }
            if (offset_sea < sea_names.size()) {
                std::vector<std::string>::const_iterator it(sea_names.begin());
                std::advance(it, offset_sea);
                strncpy(name, it->c_str(), MaxBaseNameLen);
                name[MaxBaseNameLen - 1] = '\0';
                if (save_offset) {
                    f.base_sea_name_offset++;
                }
                return;
            }
        }
    }
    std::vector<std::string> land_names = read_txt_block(
        MFactions[faction].filename, "#BASES", MaxBaseNameLen);
    if (save_offset) {
        f.base_name_offset++;
    }
    if (land_names.size() > 0) {
        if (offset > 0 && offset < land_names.size()) {
            uint32_t seed = ((*map_random_seed + faction) & 0xFE) | 1;
            uint32_t loop = 0;
            do {
                if (seed & 1) {
                    seed ^= 0x170;
                }
                seed >>= 1;
            } while (seed >= land_names.size() || ++loop != offset);
            offset = seed;
        }
        if (offset < land_names.size()) {
            std::vector<std::string>::const_iterator it(land_names.begin());
            std::advance(it, offset);
            strncpy(name, it->c_str(), MaxBaseNameLen);
            name[MaxBaseNameLen - 1] = '\0';
            return;
        }
    }
    std::set<std::string> all_names;
    for (int i = 0; i < *total_num_bases; i++) {
        all_names.insert(Bases[i].name);
    }
    int i = 0;
    uint32_t x = 0;
    uint32_t a = 0;
    uint32_t b = 0;

    while (i < 1024) {
        x = pair_hash(faction + 8*f.base_name_offset, *map_random_seed ^ ++i);
        a = x % MaxWordsNum;
        b = (x / 256) % MaxWordsNum;
        name[0] = '\0';
        snprintf(name, MaxBaseNameLen, "%s %s Sector", words[a], words[b]);
        if (a != b && strlen(name) >= 14 && !all_names.count(name)) {
            return;
        }
    }
}

int __cdecl mod_best_defender(int defender_veh_id, int attacker_veh_id, int bombardment)
{
    // store variables for modified odds dialog unless bombardment
    int best_id = best_defender(defender_veh_id, attacker_veh_id, bombardment);
    if (bombardment) {
        current_attacker_veh_id = -1;
        current_defender_veh_id = -1;
    } else {
        current_attacker_veh_id = attacker_veh_id;
        current_defender_veh_id = best_id;
    }
    return best_id;
}

int __cdecl battle_fight_parse_num(int index, int value)
{
    if (index > 9) {
        return 3;
    }
    ParseNumTable[index] = value;

    if (conf.ignore_reactor_power && index == 1
    && current_attacker_veh_id >= 0 && current_defender_veh_id >= 0) {
        VEH* veh1 = &Vehicles[current_attacker_veh_id];
        VEH* veh2 = &Vehicles[current_defender_veh_id];
        UNIT* u1 = &Units[veh1->unit_id];
        UNIT* u2 = &Units[veh2->unit_id];
        // calculate attacker and defender power
        // artifact gets 1 HP regardless of reactor
        int attackerPower = (u1->weapon_type == WPN_ALIEN_ARTIFACT ? 1 :
                             u1->reactor_type * 10 - veh1->damage_taken);
        int defenderPower = (u2->weapon_type == WPN_ALIEN_ARTIFACT ? 1 :
                             u2->reactor_type * 10 - veh2->damage_taken);
        // calculate firepower
        int attackerFP = u2->reactor_type;
        int defenderFP = u1->reactor_type;
        // calculate hitpoints
        int attackerHP = (attackerPower + (defenderFP - 1)) / defenderFP;
        int defenderHP = (defenderPower + (attackerFP - 1)) / attackerFP;
        // calculate correct odds
        if (Weapon[u1->weapon_type].offense_value >= 0
        && Armor[u2->armor_type].defense_value >= 0) {
            // psi combat odds are already correct
            // reverse engineer conventional combat odds in case of ignored reactor
            int attackerOdds = ParseNumTable[0] * attackerHP * defenderPower;
            int defenderOdds = ParseNumTable[1] * defenderHP * attackerPower;
            int gcd = std::__gcd(attackerOdds, defenderOdds);
            attackerOdds /= gcd;
            defenderOdds /= gcd;
            // reparse their odds into dialog
            ParseNumTable[0] = attackerOdds;
            ParseNumTable[1] = defenderOdds;
        }
    }
    return 0;
}

int probe_roll_value(int faction) {
    int techs = 0;
    for (int i=Tech_ID_First; i <= Tech_ID_Last; i++) {
        if (Tech[i].preq_tech1 != TECH_Disable && has_tech(faction, i)
        && Tech[i].flags & TFLAG_IMPROVE_PROBE) {
            techs++;
        }
    }
    return 2*techs + 2*clamp(Factions[faction].SE_probe, -3, 3)
        + clamp(Factions[faction].SE_probe_base, -3, 3)
        + clamp(Factions[faction].SE_police, -3, 3);
}

int probe_active_turns(int faction1, int faction2) {
    int value = clamp(15 + probe_roll_value(faction1) - probe_roll_value(faction2), 4, 50);
    value = value * (4 + (*map_area_tiles >= 4000) + (*map_area_tiles >= 7000)) / 4;
    value = value * (4 + (*diff_level < DIFF_TRANSCEND) + (*diff_level < DIFF_THINKER)) / 4;
    return clamp(value, 5, 50);
}

int probe_upkeep(int faction1) {
    if (faction1 <= 0 || !is_alive(faction1) || !conf.counter_espionage) {
        return 0;
    }
    /*
    Do not expire infiltration while the faction is the governor or has the Empath Guild.
    Status can be renewed once per turn and sets the flag DIPLO_RENEW_INFILTRATOR.
    This is checked in patched version of probe() game code.
    */
    for (int faction2=1; faction2 < MaxPlayerNum; faction2++) {
        if (faction1 != faction2 && is_alive(faction2)
        && faction2 != *GovernorFaction && !has_project(faction2, FAC_EMPATH_GUILD)) {
            if (has_treaty(faction2, faction1, DIPLO_HAVE_INFILTRATOR)) {
                if (!MFactions[faction2].thinker_probe_end_turn[faction1]) {
                    MFactions[faction2].thinker_probe_end_turn[faction1] =
                        *current_turn + probe_active_turns(faction2, faction1);
                }
                if (MFactions[faction2].thinker_probe_end_turn[faction1] <= *current_turn) {
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
    for (int faction2=1; faction2 < MaxPlayerNum; faction2++) {
        if (faction1 != faction2 && is_alive(faction2)) {
            debug("probe_upkeep %3d %d %d spying: %d ends: %d\n",
                *current_turn, faction1, faction2,
                has_treaty(faction1, faction2, DIPLO_HAVE_INFILTRATOR),
                MFactions[faction1].thinker_probe_end_turn[faction2]
            );
            if (faction1 != *GovernorFaction && !has_project(faction1, FAC_EMPATH_GUILD)) {
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
    if (base_id >= 0 && base_id < *total_num_bases) {
        int faction1 = Vehs[veh_id1].faction_id;
        int faction2 = Bases[base_id].faction_id;
        int turns = MFactions[faction1].thinker_probe_end_turn[faction2] - *current_turn;
        if (has_treaty(faction1, faction2, DIPLO_HAVE_INFILTRATOR) && turns > 0
        && faction1 != *GovernorFaction && !has_project(faction1, FAC_EMPATH_GUILD)) {
            ParseNumTable[0] =  turns;
            return Popup_start(This, "modmenu", "PROBE", a4, a5, a6, a7);
        }
    }
    return Popup_start(This, ScriptFile, "PROBE", a4, a5, a6, a7);
}


