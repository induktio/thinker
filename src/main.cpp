
#include "main.h"
#include "patch.h"
#include "game.h"
#include "move.h"

FILE* debug_log;
Config conf;
int proj_limit[8];
int diplo_flags[8];

std::set<std::pair <int, int>> convoys;
std::set<std::pair <int, int>> boreholes;

static int handler(void* user, const char* section, const char* name, const char* value) {
    Config* pconfig = (Config*)user;
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("thinker", "free_formers")) {
        pconfig->free_formers = atoi(value);
    } else if (MATCH("thinker", "satellites_nutrient")) {
        pconfig->satellites_nutrient = max(0, atoi(value));
    } else if (MATCH("thinker", "satellites_mineral")) {
        pconfig->satellites_mineral = max(0, atoi(value));
    } else if (MATCH("thinker", "satellites_energy")) {
        pconfig->satellites_energy = max(0, atoi(value));
    } else if (MATCH("thinker", "design_units")) {
        pconfig->design_units = atoi(value);
    } else if (MATCH("thinker", "factions_enabled")) {
        pconfig->factions_enabled = atoi(value);
    } else if (MATCH("thinker", "terraform_ai")) {
        pconfig->terraform_ai = atoi(value);
    } else if (MATCH("thinker", "production_ai")) {
        pconfig->production_ai = atoi(value);
    } else if (MATCH("thinker", "tech_balance")) {
        pconfig->tech_balance = atoi(value);
    } else if (MATCH("thinker", "load_expansion")) {
        pconfig->load_expansion = atoi(value);
    } else if (MATCH("thinker", "faction_placement")) {
        pconfig->faction_placement = atoi(value);
    } else {
        for (int i=0; i<16; i++) {
            if (MATCH("thinker", lm_params[i])) {
                pconfig->landmarks &= ~((atoi(value) ? 0 : 1) << i);
                return 1;
            }
        }
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE UNUSED(hinstDLL), DWORD fdwReason, LPVOID UNUSED(lpvReserved)) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            conf.free_formers = 0;
            conf.satellites_nutrient = 0;
            conf.satellites_mineral = 0;
            conf.satellites_energy = 0;
            conf.design_units = 1;
            conf.factions_enabled = 7;
            conf.terraform_ai = 1;
            conf.production_ai = 1;
            conf.tech_balance = 1;
            conf.load_expansion = 1;
            conf.faction_placement = 1;
            conf.landmarks = 0xffff;
            debug_log = fopen("debug.txt", "w");
            if (!debug_log)
                return FALSE;
            if (ini_parse("thinker.ini", handler, &conf) < 0)
                return FALSE;
            if (!patch_setup(&conf))
                return FALSE;
            *tx_version = VERSION;
            *tx_date = __DATE__;
            break;

        case DLL_PROCESS_DETACH:
            fclose(debug_log);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

DLL_EXPORT int ThinkerDecide() {
    return 0;
}

HOOK_API int base_production(int id, int v1, int v2, int v3) {
    assert(id >= 0 && id < BASES);
    BASE* base = &tx_bases[id];
    int prod = base->queue_production_id[0];
    int owner = base->faction_id;
    int choice = 0;

    if (DEBUG) {
        debuglog("[ turn: %d faction: %d base: %2d x: %2d y: %2d "\
        "pop: %d tal: %d dro: %d mins: %2d acc: %2d prod: %3d | %s | %s ]\n",
        *tx_current_turn, owner, id, base->x_coord, base->y_coord,
        base->pop_size, base->talent_total, base->drone_total,
        base->mineral_intake, base->minerals_accumulated,
        prod, prod_name(prod), (char*)&(base->name));
    }

    if (1 << owner & *tx_human_players) {
        debuglog("skipping human base\n");
        choice = base->queue_production_id[0];
    } else if (!conf.production_ai || owner > conf.factions_enabled) {
        debuglog("skipping computer base\n");
        choice = tx_base_prod_choices(id, v1, v2, v3);
    } else {
        tx_set_base(id);
        tx_base_compute(1);
        if ((choice = need_psych(id)) != 0) {
            debuglog("BUILD PSYCH\n");
        } else if (prod < 0 && !can_build(id, abs(prod))) {
            debuglog("BUILD CHANGE\n");
            if (base->minerals_accumulated > tx_basic->retool_exemption)
                choice = find_facility(id);
            else
                choice = select_prod(id);
        } else if (base->status_flags & BASE_PRODUCTION_DONE) {
            choice = select_prod(id);
        } else {
            debuglog("BUILD OLD\n");
            choice = prod;
        }
        debuglog("choice: %d %s\n", choice, prod_name(choice));
    }
    fflush(debug_log);
    return choice;
}

HOOK_API int enemy_move(int id) {
    assert(id >= 0 && id < UNITS);
    VEH* veh = &tx_vehicles[id];
    int fac = veh->faction_id;
    if (conf.terraform_ai && fac <= conf.factions_enabled && (1 << fac) & ~*tx_human_players) {
        int w = tx_units[veh->proto_id].weapon_mode;
        if (w == WMODE_COLONIST) {
            return colony_move(id);
        } else if (w == WMODE_CONVOY) {
            return crawler_move(id);
        } else if (w == WMODE_TERRAFORMER && unit_triad(veh->proto_id) != TRIAD_SEA) {
            return former_move(id);
        }
    }
    return tx_enemy_move(id);
}

HOOK_API int turn_upkeep() {
    tx_turn_upkeep();

    for (int i=1; i<8 && conf.design_units; i++) {
        if (1 << i & *tx_human_players || !tx_factions[i].current_num_bases)
            continue;
        int rec = best_reactor(i);
        int arm = best_armor(i);
        int abl = has_ability(i, ABL_ID_TRANCE) ? ABL_TRANCE : 0;
        bool twoabl = knows_tech(i, tx_basic->tech_preq_allow_2_spec_abil);
        char* arm_n = tx_defense[arm].name_short;
        char nm[100];

        if (has_weapon(i, WPN_PROBE_TEAM)) {
            int chs = has_chassis(i, CHS_HOVERTANK) ? CHS_HOVERTANK : CHS_SPEEDER;
            int algo = has_ability(i, ABL_ID_ALGO_ENHANCEMENT) ? ABL_ALGO_ENHANCEMENT : 0;
            if (has_chassis(i, CHS_FOIL)) {
                tx_propose_proto(i, CHS_FOIL, WPN_PROBE_TEAM, (rec >= REC_FUSION ? arm : 0),
                (rec >= REC_FUSION ? abl | (twoabl ? algo : 0) : 0),
                rec, PLAN_INFO_WARFARE, "Foil Probe Team");
            }
            if (arm != ARM_NO_ARMOR && rec >= REC_FUSION) {
                tx_propose_proto(i, chs, WPN_PROBE_TEAM, arm, abl | (twoabl ? algo : 0),
                rec, PLAN_INFO_WARFARE,
                (strlen(arm_n) < 50 ? strcat(strcpy(nm, arm_n), " Probe Team") : NULL));
            }
        }
        if (has_ability(i, ABL_ID_AAA) && arm != ARM_NO_ARMOR) {
            int abls = ABL_AAA | (twoabl && has_ability(i, ABL_ID_COMM_JAMMER) ?
                ABL_COMM_JAMMER : 0);
            tx_propose_proto(i, CHS_INFANTRY, WPN_LASER, arm, abls, rec, PLAN_DEFENSIVE, NULL);
        }
    }
    if (*tx_current_turn == 1) {
        int bonus = ~(*tx_human_players) & 0xfe;
        for (int i=0; i<*tx_total_num_vehicles; i++) {
            VEH* v = &tx_vehicles[i];
            if (1 << v->faction_id & bonus) {
                bonus &= ~(1 << v->faction_id);
                MAP* sq = mapsq(v->x_coord, v->y_coord);
                int unit = (sq && sq->altitude < ALTITUDE_MIN_LAND ?
                    BSC_SEA_FORMERS : BSC_FORMERS);
                for (int j=0; j<conf.free_formers; j++) {
                    int veh = tx_veh_init(unit, v->faction_id, v->x_coord, v->y_coord);
                    if (veh >= 0)
                        tx_vehicles[veh].home_base_id = -1;
                }
            }
        }
        for (int i=1; i<8; i++) {
            if (1 << i & ~*tx_human_players) {
                tx_factions[i].satellites_nutrient = conf.satellites_nutrient;
                tx_factions[i].satellites_mineral = conf.satellites_mineral;
                tx_factions[i].satellites_energy = conf.satellites_energy;
            }
        }
    }
    if (DEBUG) {
        *tx_game_rules_basic |= RULES_DEBUG_MODE;
    }
    int minerals[BASES];
    for (int i=1; i<8; i++) {
        diplo_flags[i] = 0;
        for (int j=1; j<8; j++) {
            int* st = &(tx_factions[i].diplo_status[j]);
            if (i != j && *st & DIPLO_COMMLINK) {
                diplo_flags[i] |= *st;
            }
        }
        int n = 0;
        for (int j=0; j<*tx_total_num_bases; j++) {
            BASE* b = &tx_bases[j];
            if (b->faction_id == i) {
                minerals[n++] = b->mineral_surplus;
            }
        }
        std::sort(minerals, minerals+n);
        proj_limit[i] = max(5, minerals[n*2/3]);
    }
    move_upkeep();
    fflush(debug_log);

    return 0;
}

HOOK_API int tech_value(int tech, int fac, int flag) {
    int value = tx_tech_val(tech, fac, flag);
    if (conf.tech_balance && fac <= conf.factions_enabled) {
        if (tech == tx_weapon[WPN_TERRAFORMING_UNIT].preq_tech
        || tech == tx_weapon[WPN_SUPPLY_TRANSPORT].preq_tech
        || tech == tx_basic->tech_preq_allow_3_energy_sq
        || tech == tx_basic->tech_preq_allow_3_minerals_sq
        || tech == tx_basic->tech_preq_allow_3_nutrients_sq) {
            value *= 2;
        }
    }
    debuglog("tech_value %d %d %d %s\n", tech, fac, value, tx_techs[tech].name);
    return value;
}

void print_veh(int id) {
    VEH v = tx_vehicles[id];
    debuglog("VEH %16s %2d | %08x %04x | %2d %3d | %2d %2d %2d %2d | %d %d %d %d %d\n",
        (char*)&(tx_units[v.proto_id].name), id,
        v.flags_1, v.flags_2, v.move_status, v.status_icon,
        v.x_coord, v.y_coord, v.waypoint_1_x_coord, v.waypoint_1_y_coord,
        v.unk4, v.unk5, v.unk6, v.unk8, v.unk9);
}

int project_score(int fac, int proj) {
    R_Facility* p = &tx_facility[proj];
    Faction* f = &tx_factions[fac];
    return random(3) + f->AI_fight * p->AI_fight
        + (f->AI_growth+1) * p->AI_growth + (f->AI_power+1) * p->AI_power
        + (f->AI_tech+1) * p->AI_tech + (f->AI_wealth+1) * p->AI_wealth;
}

int find_project(int fac) {
    Faction* fact = &tx_factions[fac];
    int bases = fact->current_num_bases;
    int nuke_limit = (fact->planet_busters < fact->AI_fight + 2 ? 1 : 0);
    int projs = 0;
    int nukes = 0;

    bool repeal = *tx_un_charter_repeals > *tx_un_charter_reinstates;
    bool build_nukes = has_weapon(fac, WPN_PLANET_BUSTER) &&
        (repeal || diplo_flags[fac] & DIPLO_ATROCITY_VICTIM ||
        (fact->AI_fight > 0 && fact->AI_power > 0));

    for (int i=0; i<*tx_total_num_bases; i++) {
        if (tx_bases[i].faction_id == fac) {
            int prod = tx_bases[i].queue_production_id[0];
            if (prod <= -70 || prod == FAC_SUBSPACE_GENERATOR) {
                projs++;
            } else if (prod >= 0 && tx_units[prod].weapon_type == WPN_PLANET_BUSTER) {
                nukes++;
            }
        }
    }
    if (build_nukes && nukes < nuke_limit && nukes < bases/8) {
        int best = 0;
        for(int i=0; i<64; i++) {
            int id = fac*64 + i;
            UNIT* u = &tx_units[id];
            if (u->weapon_type == WPN_PLANET_BUSTER && strlen(u->name) > 0
            && offense_value(u) > offense_value(&tx_units[best])) {
                debuglog("find_project %d %d %s\n", fac, id, (char*)tx_units[id].name);
                best = id;
            }
        }
        if (best)
            return best;
    }
    if (projs+nukes < (build_nukes ? 4 : 3) && projs+nukes < bases/4) {
        bool alien = tx_factions_meta[fac].rule_flags & FACT_ALIEN;
        if (alien && knows_tech(fac, tx_facility[FAC_SUBSPACE_GENERATOR].preq_tech)) {
            return -FAC_SUBSPACE_GENERATOR;
        }
        int score = INT_MIN;
        int choice = 0;
        for (int i=70; i<107; i++) {
            bool ascent = (i == FAC_ASCENT_TO_TRANSCENDENCE && has_facility(-1, FAC_VOICE_OF_PLANET));
            if (alien && (i == FAC_ASCENT_TO_TRANSCENDENCE || i == FAC_VOICE_OF_PLANET))
                continue;
            if (tx_secret_projects[i-70] == -1 && (ascent || knows_tech(fac, tx_facility[i].preq_tech))) {
                int sc = project_score(fac, i);
                choice = (sc > score ? i : choice);
                score = max(score, sc);
                debuglog("find_project %d %d %d %s\n", fac, i, sc, (char*)tx_facility[i].name);
            }
        }
        return (projs > 0 || score > 2 ? -choice : 0);
    }
    return 0;
}

int count_sea_tiles(int x, int y, int limit) {
    MAP* tile = mapsq(x, y);
    if (!tile || (tile->level >> 5) > LEVEL_SHORE_LINE) {
        return -1;
    }
    int n = 0;
    TileSearch ts;
    ts.init(x, y, WATER_ONLY);
    while (n < limit && (tile = ts.get_next()) != NULL) {
        n++;
    }
    debuglog("count_sea_tiles %d %d %d\n", x, y, n);
    return n;
}

bool switch_to_sea(int x, int y) {
    const int limit = 250;
    MAP* tile = mapsq(x, y);
    if (tile && tile->altitude < ALTITUDE_MIN_LAND) {
        return true;
    }
    int land = 0;
    int bases = 0;
    TileSearch ts;
    ts.init(x, y, LAND_ONLY);
    while (ts.visited() < limit && (tile = ts.get_next()) != NULL) {
        if (ts.cur_y != 0 && ts.cur_y != *tx_map_axis_y-1)
            land++;
        if (tile->built_items & TERRA_BASE_IN_TILE)
            bases++;
    }
    debuglog("switch_to_sea %d %d %d %d\n", x, y, land, bases);
    return land / max(1, bases) < 14;
}

int unit_score(int id, bool def) {
    const int abls[][2] = {
        {ABL_AAA, 5},
        {ABL_AIR_SUPERIORITY, 3},
        {ABL_ALGO_ENHANCEMENT, 5},
        {ABL_AMPHIBIOUS, -4},
        {ABL_ARTILLERY, -2},
        {ABL_DROP_POD, 5},
        {ABL_EMPATH, 2},
        {ABL_TRANCE, 2},
        {ABL_TRAINED, 1},
        {ABL_COMM_JAMMER, 3},
        {ABL_BLINK_DISPLACER, 3},
        {ABL_SUPER_TERRAFORMER, 8},
    };
    int v = 2 * (def ? defense_value(&tx_units[id]) : offense_value(&tx_units[id]));
    if (unit_triad(id) == TRIAD_LAND)
        v += 3 * unit_speed(id);
    for (const int* i : abls) {
        if (tx_units[id].ability_flags & i[0]) {
            v += i[1];
        }
    }
    return v;
}

bool unit_switch(int id1, int id2, bool def) {
    return random(16) > 8 + unit_score(id1, def) - unit_score(id2, def);
}

int find_proto(int fac, int triad, int mode, bool defend) {
    int basic = BSC_SCOUT_PATROL;
    debuglog("find_proto fac: %d triad: %d mode: %d def: %d\n", fac, triad, mode, defend);
    if (mode == WMODE_COLONIST)
        basic = BSC_COLONY_POD;
    else if (mode == WMODE_TERRAFORMER)
        basic = (triad == TRIAD_SEA ? BSC_SEA_FORMERS : BSC_FORMERS);
    else if (mode == WMODE_TRANSPORT)
        basic = BSC_TRANSPORT_FOIL;
    else if (mode == WMODE_INFOWAR)
        basic = BSC_PROBE_TEAM;
    int best = basic;
    for(int i=0; i<64; i++) {
        int id = fac*64 + i;
        UNIT* u = &tx_units[id];
        if (unit_triad(id) == triad && strlen(u->name) > 0) {
            if ((mode && u->weapon_mode != mode)
            || (!mode && tx_weapon[u->weapon_type].offense_value <= 0)
            || (!mode && defend && u->chassis_type != CHS_INFANTRY)
            || u->weapon_type == WPN_PLANET_BUSTER)
                continue;
            bool valid = mode || (defend == (offense_value(u) < defense_value(u)));
            if (best == basic || (valid && unit_switch(best, id, defend))) {
                best = id;
                debuglog("===> %s\n", (char*)&(tx_units[best].name));
            }
        }
    }
    return best;
}

int need_psych(int id) {
    BASE* b = &tx_bases[id];
    int fac = b->faction_id;
    int unit = unit_in_tile(mapsq(b->x_coord, b->y_coord));
    if (unit != fac || has_project(fac, FAC_TELEPATHIC_MATRIX))
        return 0;
    if (b->drone_total > b->talent_total || b->specialist_total > b->talent_total+1) {
        if (can_build(id, FAC_RECREATION_COMMONS))
            return -FAC_RECREATION_COMMONS;
        if (has_project(fac, FAC_VIRTUAL_WORLD) && can_build(id, FAC_NETWORK_NODE))
            return -FAC_NETWORK_NODE;
        if (!b->assimilation_turns_left && b->pop_size > 7 && b->energy_surplus >= 12
        && can_build(id, FAC_HOLOGRAM_THEATRE))
            return -FAC_HOLOGRAM_THEATRE;
    }
    return 0;
}

int find_facility(int base_id) {
    const int build_order[] = {
        FAC_RECREATION_COMMONS,
        FAC_CHILDREN_CRECHE,
        FAC_PERIMETER_DEFENSE,
        FAC_GENEJACK_FACTORY,
        FAC_NETWORK_NODE,
        FAC_AEROSPACE_COMPLEX,
        FAC_TREE_FARM,
        FAC_HAB_COMPLEX,
        FAC_COMMAND_CENTER,
        FAC_FUSION_LAB,
        FAC_ENERGY_BANK,
        FAC_RESEARCH_HOSPITAL,
        FAC_HABITATION_DOME,
    };
    BASE* base = &tx_bases[base_id];
    int fac = base->faction_id;
    int proj;
    int minerals = base->mineral_surplus;
    int extra = base->minerals_accumulated/10;
    int pop_rule = tx_factions_meta[fac].rule_population;
    int hab_complex_limit = tx_basic->pop_limit_wo_hab_complex - pop_rule;
    int hab_dome_limit = tx_basic->pop_limit_wo_hab_dome - pop_rule;
    Faction* fact = &tx_factions[fac];

    if (*tx_climate_future_change > 0) {
        MAP* tile = mapsq(base->x_coord, base->y_coord);
        if (tile && (tile->level >> 5) == LEVEL_SHORE_LINE && can_build(base_id, FAC_PRESSURE_DOME))
            return -FAC_PRESSURE_DOME;
    }
    if (base->drone_total > 0 && can_build(base_id, FAC_RECREATION_COMMONS))
        return -FAC_RECREATION_COMMONS;
    if (can_build(base_id, FAC_RECYCLING_TANKS))
        return -FAC_RECYCLING_TANKS;
    if (base->pop_size >= hab_complex_limit && can_build(base_id, FAC_HAB_COMPLEX))
        return -FAC_HAB_COMPLEX;
    if (minerals+extra >= proj_limit[fac] && (proj = find_project(fac)) != 0) {
        return proj;
    }
    if (minerals+extra >= proj_limit[fac] && has_facility(base_id, FAC_AEROSPACE_COMPLEX)) {
        if (can_build(base_id, FAC_ORBITAL_DEFENSE_POD))
            return -FAC_ORBITAL_DEFENSE_POD;
        if (can_build(base_id, FAC_NESSUS_MINING_STATION))
            return -FAC_NESSUS_MINING_STATION;
        if (can_build(base_id, FAC_ORBITAL_POWER_TRANS))
            return -FAC_ORBITAL_POWER_TRANS;
        if (can_build(base_id, FAC_SKY_HYDRO_LAB))
            return -FAC_SKY_HYDRO_LAB;
    }
    if (minerals >= proj_limit[fac] && find_hq(fac) < 0
    && bases_in_range(base->x_coord, base->y_coord, 4) >= 5) {
        return -FAC_HEADQUARTERS;
    }
    for (int f : build_order) {
        if (base->energy_surplus < 6 && (f == FAC_NETWORK_NODE || f == FAC_ENERGY_BANK))
            continue;
        if (base->energy_surplus < 16 && (f == FAC_FUSION_LAB || f == FAC_RESEARCH_HOSPITAL))
            continue;
        if (f == FAC_COMMAND_CENTER && fact->AI_fight < 1 && fact->AI_power < 1)
            continue;
        if (f == FAC_GENEJACK_FACTORY && base->mineral_intake < 16)
            continue;
        if (f == FAC_HABITATION_DOME && base->pop_size < hab_dome_limit)
            continue;
        if (can_build(base_id, f)) {
            return -1*f;
        }
    }
    debuglog("******\n");
    bool sea_base = count_sea_tiles(base->x_coord, base->y_coord, 50) >= 50;
    if (has_weapon(fac, WPN_PROBE_TEAM) && random(minerals) < base->mineral_intake/2) {
        return find_proto(fac, (sea_base ? TRIAD_SEA : TRIAD_LAND), WMODE_INFOWAR, DEF);
    }
    if (has_chassis(fac, CHS_NEEDLEJET) && fact->SE_police >= -3 && random(3) == 0) {
        return find_proto(fac, TRIAD_AIR, COMBAT, ATT);
    }
    if (has_chassis(fac, CHS_FOIL) && sea_base) {
        return find_proto(fac, TRIAD_SEA, (random(3) == 0 ? WMODE_TRANSPORT : COMBAT), ATT);
    }
    return find_proto(fac, TRIAD_LAND, COMBAT, ATT);
}

int faction_might(int fac) {
    Faction* f = &tx_factions[fac];
    return max(1, f->mil_strength_1 + f->mil_strength_2 + f->pop_total * 2);
}

int select_prod(int id) {
    BASE* base = &tx_bases[id];
    int fac = base->faction_id;
    int minerals = base->mineral_surplus;
    Faction* fact = &tx_factions[fac];

    int defenders = 0;
    int crawlers = 0;
    int formers = 0;
    int probes = 0;
    int pods = 0;
    int enemymask = 1;
    int enemyrange = 40;
    double enemymil = 0;

    for (int i=1; i<8; i++) {
        if (i==fac || ~fact->diplo_status[i] & DIPLO_COMMLINK)
            continue;
        double mil = (1.0 * faction_might(i)) / faction_might(fac);
        if (fact->diplo_status[i] & DIPLO_VENDETTA) {
            enemymask |= (1 << i);
            enemymil = max(enemymil, 1.0 * mil);
        } else if (~fact->diplo_status[i] & DIPLO_PACT) {
            enemymil = max(enemymil, 0.3 * mil);
        }
    }
    for (int i=0; i<*tx_total_num_bases; i++) {
        BASE* b = &tx_bases[i];
        if ((1 << b->faction_id) & enemymask) {
            enemyrange = min(enemyrange,
                map_range(base->x_coord, base->y_coord, b->x_coord, b->y_coord));
        }
    }

    for (int i=0; i<*tx_total_num_vehicles; i++) {
        VEH* veh = &tx_vehicles[i];
        UNIT* unit = &tx_units[veh->proto_id];
        if (veh->faction_id != fac)
            continue;
        if (veh->home_base_id == id) {
            if (unit->weapon_type == WPN_TERRAFORMING_UNIT)
                formers++;
            else if (unit->weapon_type == WPN_COLONY_MODULE)
                pods++;
            else if (unit->weapon_type == WPN_PROBE_TEAM)
                probes++;
            else if (unit->weapon_type == WPN_SUPPLY_TRANSPORT)
                crawlers += (veh->move_status == STATUS_CONVOY ? 1 : 5);
        }
        int range = map_range(base->x_coord, base->y_coord, veh->x_coord, veh->y_coord);
        if (range <= 1) {
            if (unit_triad(veh->proto_id) == TRIAD_LAND && unit->weapon_type <= WPN_PSI_ATTACK) {
                defenders++;
            }
        }
    }
    int reserve = max(2, base->mineral_intake / 2);
    double base_ratio = 2.0 * fact->current_num_bases / min(80, *tx_map_area_sq_root);
    bool has_formers = has_weapon(fac, WPN_TERRAFORMING_UNIT);
    bool has_supply = has_weapon(fac, WPN_SUPPLY_TRANSPORT);
    bool can_build_ships = has_chassis(fac, CHS_FOIL)
        && count_sea_tiles(base->x_coord, base->y_coord, 25) >= 25;
    bool land_area_full = switch_to_sea(base->x_coord, base->y_coord);
    bool build_pods = (base->pop_size > 1 || base->nutrient_surplus > 1) && base_ratio < 1.0
        && pods < (*tx_current_turn < 60 || !land_area_full ? 2 : 1);
    bool sea_base = water_base(id);

    double w1 = min(1.0, 1.0 * minerals / proj_limit[fac]);
    double w2 = enemymil / (enemyrange * 0.1 + 0.1) - max(0, defenders-1) * 0.3
        + min(1.0, (fact->AI_fight * 0.2 + 0.8) * base_ratio);
    double threat = 1 - (1 / (1 + max(0.0, w1 * w2)));

    debuglog("select_prod "\
    "%d %d %2d %2d | %d %d %d %d %d %d %d | %2d %2d %2d | %2d %.4f %.4f %.4f %.4f\n",
    *tx_current_turn, fac, base->x_coord, base->y_coord,
    defenders, formers, pods, probes, crawlers, build_pods, land_area_full,
    minerals, reserve, proj_limit[fac], enemyrange, enemymil, w1, w2, threat);

    if (defenders == 0 && minerals > 2) {
        return find_proto(fac, TRIAD_LAND, COMBAT, DEF);
    } else if (minerals > reserve && random(100) < (int)(100.0 * threat)) {
        if (defenders > 2 && enemyrange < 12 && can_build(id, FAC_PERIMETER_DEFENSE))
            return -FAC_PERIMETER_DEFENSE;
        if (has_chassis(fac, CHS_NEEDLEJET) && fact->SE_police >= -3 && random(3) == 0)
            return find_proto(fac, TRIAD_AIR, COMBAT, ATT);
        else if (has_weapon(fac, WPN_PROBE_TEAM) && probes < 1 && random(3) == 0)
            if (can_build_ships)
                return find_proto(fac, TRIAD_SEA, WMODE_INFOWAR, DEF);
            else
                return find_proto(fac, TRIAD_LAND, WMODE_INFOWAR, DEF);
        else if (sea_base || (can_build_ships && defenders > 1 && random(3) == 0))
            if (random(3) == 0)
                return find_proto(fac, TRIAD_SEA, WMODE_TRANSPORT, DEF);
            else
                return find_proto(fac, TRIAD_SEA, COMBAT, ATT);
        else
            return find_proto(fac, TRIAD_LAND, COMBAT, ATT);
    } else if (has_formers && formers <= min(1, base->pop_size/(sea_base ? 6 : 3))) {
        if (sea_base)
            return find_proto(fac, TRIAD_SEA, WMODE_TERRAFORMER, DEF);
        else
            return find_proto(fac, TRIAD_LAND, WMODE_TERRAFORMER, DEF);
    } else {
        if (has_supply && crawlers <= min(2, base->pop_size/3) && !sea_base)
            return BSC_SUPPLY_CRAWLER;
        if (build_pods && !can_build(id, FAC_RECYCLING_TANKS))
            if (can_build_ships && land_area_full)
                return find_proto(fac, TRIAD_SEA, WMODE_COLONIST, DEF);
            else
                return BSC_COLONY_POD;
        else
            return find_facility(id);
    }
}








