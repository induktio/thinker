
#include "engine.h"

const char* ScriptTxtID = "SCRIPT";
static int currentAttackerVehicleId = -1;
static int currentDefenderVehicleId = -1;

int probe_upkeep(int faction);


void init_save_game(int faction) {
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    if (m->thinker_header != THINKER_HEADER) {
        m->thinker_header = THINKER_HEADER;
        m->thinker_flags = 0;
        /* Convert old save game to the new format. */
        if (f->old_thinker_header == THINKER_HEADER) {
            m->thinker_tech_id = f->old_thinker_tech_id;
            m->thinker_tech_cost = f->old_thinker_tech_cost;
            m->thinker_enemy_range = f->old_thinker_enemy_range;
            memset(&f->old_thinker_header, 0, 12);
        } else {
            m->thinker_tech_id = f->tech_research_id;
            m->thinker_tech_cost = f->tech_cost;
            m->thinker_enemy_range = MaxEnemyRange/2;
        }
    } else {
        assert(f->old_thinker_header != THINKER_HEADER);
    }
    if (m->thinker_enemy_range < 2 || m->thinker_enemy_range > MaxEnemyRange) {
        m->thinker_enemy_range = MaxEnemyRange/2;
    }
}

bool victory_done() {
    // TODO: Check for scenario victory conditions
    return *game_state & (STATE_VICTORY_CONQUER | STATE_VICTORY_DIPLOMATIC | STATE_VICTORY_ECONOMIC)
        || has_project(-1, FAC_ASCENT_TO_TRANSCENDENCE);
}

bool has_colony_pods(int faction) {
    for (int i=0; i<*total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction && veh->is_colony()) {
            return true;
        }
    }
    return false;
}

/*
Original Offset: 005C89A0
*/
int __cdecl game_year(int n) {
    return Rules->normal_start_year + n;
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
    if (faction > 0) {
        init_save_game(faction);
        plans_upkeep(faction);
    }

    *dword_93A934 = 1;
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
        This means we mostly cannot use move_upkeep variables in production phase.
        */
        mod_social_ai(faction, -1, -1, -1, -1, 0);
        probe_upkeep(faction);
        move_upkeep(faction, false);
        do_all_non_input();

        if (!is_human(faction)) {
            int cost = corner_market(faction);
            if (!victory_done() && f->energy_credits > cost && f->corner_market_active < 1
            && has_tech(faction, Rules->tech_preq_economic_victory)
            && *game_rules & RULES_VICTORY_ECONOMIC) {
                f->corner_market_turn = *current_turn + Rules->turns_corner_global_energy_market;
                f->corner_market_active = cost;
                f->energy_credits -= cost;

                gender_default = &m->noun_gender;
                plurality_default = 0;
                parse_says(0, m->title_leader, -1, -1);
                parse_says(1, m->name_leader, -1, -1);
                plurality_default = &m->is_noun_plural;
                parse_says(2, m->noun_faction, -1, -1);
                parse_says(3, m->adj_name_faction, -1, -1);
                parse_num(0, game_year(f->corner_market_turn));
                popp(ScriptTxtID, "CORNERWARNING", 0, "econwin_sm.pcx", 0);
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
    if (!f->current_num_bases && !has_colony_pods(faction)) {
        eliminate_player(faction, 0);
    }
    *dword_93A934 = 0;
    *dword_945B18 = -1;
    *dword_945B1C = -1;

    if (!(*game_state & STATE_GAME_DONE) || *game_state & STATE_FINAL_SCORE_DONE) {
        if (faction == *current_player_faction
        && !(*game_state & (STATE_COUNCIL_HAS_CONVENED | STATE_DISPLAYED_COUNCIL_AVAIL_MSG))
        && can_call_council(faction, 0) && !(*game_state & STATE_GAME_DONE)) {
            *game_state |= STATE_DISPLAYED_COUNCIL_AVAIL_MSG;
            popp(ScriptTxtID, "COUNCILOPEN", 0, "council_sm.pcx", 0);
        }
        if (!is_human(faction)) {
            call_council(faction);
        }
    }
    if (!*multiplayer_active && *game_preferences & PREF_BSC_AUTOSAVE_EACH_TURN
    && faction == *current_player_faction) {
        auto_save();
    }
    fflush(debug_log);
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
    const int buf_size = 256;
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
            if (strlen(p) > 0 && strlen(p) < max_len) {
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
Original Offset: 004E4090
*/
void __cdecl mod_name_base(int faction, char* name, bool save_offset, bool water) {
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

    std::vector<std::string> faction_names = read_txt_block(
        MFactions[faction].filename, (water ? "#WATERBASES" : "#BASES"), MaxBaseNameLen);
    std::set<std::string> all_names;
    uint32_t offset = (water ? f.base_sea_name_offset : f.base_name_offset);
    uint32_t total = faction_names.size();

    if (offset > 0 && offset < total) {
        uint32_t seed = ((*map_random_seed + faction) & 0xFE) | 1;
        uint32_t loop = 0;
        do {
            if (seed & 1) {
                seed ^= 0x170;
            }
            seed >>= 1;
        } while (seed >= total || ++loop != offset);
        offset = seed;
    }
    if (offset < total) {
        std::vector<std::string>::const_iterator it(faction_names.begin());
        std::advance(it, offset);
        strncpy(name, it->c_str(), MaxBaseNameLen);
        name[MaxBaseNameLen - 1] = '\0';

    } else {
        for (int i=0; i < *total_num_bases; i++) {
            all_names.insert(Bases[i].name);
        }
        int i = 0;
        uint32_t x = 0;
        uint32_t a = 0;
        uint32_t b = 0;

        while (i < 1024) {
            x = map_hash(faction + 8*offset, ++i);
            a = x % MaxWordsNum;
            b = (x / 256) % MaxWordsNum;
            name[0] = '\0';
            snprintf(name, MaxBaseNameLen, "%s %s Sector", words[a], words[b]);
            if (a != b && strlen(name) >= 14 && !all_names.count(name)) {
                break;
            }
        }
    }
    if (save_offset) {
        if (water) {
            f.base_sea_name_offset++;
        } else {
            f.base_name_offset++;
        }
    }
}

int __cdecl mod_best_defender(int defenderVehicleId, int attackerVehicleId, int bombardment)
{
    // store variables for modified odds dialog unless bombardment
    int best_id = best_defender(defenderVehicleId, attackerVehicleId, bombardment);
    if (bombardment) {
        currentAttackerVehicleId = -1;
        currentDefenderVehicleId = -1;
    } else {
        currentAttackerVehicleId = attackerVehicleId;
        currentDefenderVehicleId = best_id;
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
    && currentAttackerVehicleId >= 0 && currentDefenderVehicleId >= 0) {
        VEH* veh1 = &Vehicles[currentAttackerVehicleId];
        VEH* veh2 = &Vehicles[currentDefenderVehicleId];
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

int probe_rating(int faction) {
    return Factions[faction].SE_probe_pending
        + has_project(faction, FAC_HUNTER_SEEKER_ALGO)
        + has_project(faction, FAC_NETHACK_TERMINUS);
}

int probe_upkeep(int faction) {
    if (faction > 0 && conf.counter_espionage) {
        for (int i=1; i < MaxPlayerNum; i++) {
            if (faction != i
            && Factions[i].current_num_bases > 0
            && has_treaty(i, faction, DIPLO_HAVE_INFILTRATOR)
            && !has_treaty(i, faction, DIPLO_PACT)) {
                int rnd = map_hash(faction + 8*i, *current_turn) % 1000;
                int val = max(-7, min(7,
                    (is_human(faction) ? 2 : 0) + probe_rating(faction) - probe_rating(i)));
                debug("probe_upkeep %3d %d %d value: %d roll: %d\n",
                    *current_turn, faction, i, val, rnd);

                if (rnd < 12 + (val > 0 ? 3 : 1) * val) {
                    set_treaty(i, faction, DIPLO_HAVE_INFILTRATOR, 0);
                    MFactions[i].thinker_probe_flags |= (1 << faction);

                    if (faction == *current_player_faction) {
                        parse_says(0, MFactions[i].adj_name_faction, -1, -1);
                        popp("modmenu", "SPYFOUND", 0, "infil_sm.pcx", 0);
                    }
                }
            }
        }
        for (int i=1; i < MaxPlayerNum; i++) {
            if (faction == *current_player_faction
            && Factions[i].current_num_bases > 0
            && MFactions[faction].thinker_probe_flags & (1 << i)) {
                parse_says(0, MFactions[i].noun_faction, -1, -1);
                popp("modmenu", "SPYLOST", 0, "capture_sm.pcx", 0);
            }
        }
    }
    if (faction > 0) {
        MFactions[faction].thinker_probe_flags = 0;
    }
    return 0;
}


