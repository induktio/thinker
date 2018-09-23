using namespace std;

#include "main.h"
#include "game.h"
#include "move.h"

FILE* debug_log;
Config conf;

int base_mins[BASES];
set<pair <int, int>> convoys;
set<pair <int, int>> boreholes;

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
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE UNUSED(hinstDLL), DWORD fdwReason, LPVOID UNUSED(lpvReserved)) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            memset(base_mins, 0, BASES);
            conf.free_formers = 0;
            conf.satellites_nutrient = 0;
            conf.satellites_mineral = 0;
            conf.satellites_energy = 0;
            conf.design_units = 1;
            conf.factions_enabled = 7;
            conf.terraform_ai = 1;
            conf.production_ai = 1;
            conf.tech_balance = 1;
            debug_log = fopen("debug.txt", "w");
            if (!debug_log)
                return FALSE;
            if (ini_parse("thinker.ini", handler, &conf) < 0)
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

DLL_EXPORT int ThinkerDecide(int mode, int id, int val1, int val2) {
    assert(mode > 0 && val1 >= 0 && val2 > -128 && id >= 0 && id <= 2048);

    if (mode == 2) {
        return terraform_action(id, val1-4, val2);
    } else if (mode == 3) {
        return tech_value(id, val1, val2);
    } else if (mode == 4) {
        VEH* veh = &tx_vehicles[id];
        if (conf.terraform_ai && veh->faction_id <= conf.factions_enabled) {
            int w = tx_units[veh->proto_id].weapon_mode;
            if (w == WMODE_COLONIST) {
                return consider_base(id);
            } else if (w == WMODE_CONVOY) {
                return consider_convoy(id);
            } else if (w == WMODE_TERRAFORMER && unit_triad(veh->proto_id) != TRIAD_SEA) {
                return former_move(id);
            }
        }
        return tx_enemy_move(id);
    } else if (mode == 5) {
        return turn_upkeep();
    } else if (mode != 1) {
        return 0;
    }

    BASE base = tx_bases[id];
    int owner = base.faction_id;
    int last_choice = 0;

    if (DEBUG) {
        debuglog("[ turn: %d faction: %d base: %2d x: %2d y: %2d "\
        "pop: %d tal: %d dro: %d mins: %2d acc: %2d prod: %3d | %s | %s ]\n",
        *tx_current_turn, owner, id, base.x_coord, base.y_coord,
        base.pop_size, base.talent_total, base.drone_total,
        base.mineral_intake, base.minerals_accumulated,
        base.queue_production_id[0], prod_name(base.queue_production_id[0]),
        (char*)&(base.name));
    }

    if (1 << owner & *tx_human_players) {
        debuglog("skipping human base\n");
        last_choice = base.queue_production_id[0];
    } else if (!conf.production_ai || owner > conf.factions_enabled) {
        debuglog("skipping computer base\n");
        last_choice = tx_base_prod_choices(id, 0, 0, 0);
    } else {
        last_choice = select_prod(id);
        debuglog("choice: %d %s\n", last_choice, prod_name(last_choice));
    }
    base_mins[id] = tx_bases[id].minerals_accumulated;
    fflush(debug_log);
    return last_choice;
}

int turn_upkeep() {
    for (int i=1; i<8 && conf.design_units; i++) {
        if (1 << i & *tx_human_players || !tx_factions[i].current_num_bases)
            continue;
        int rec = best_reactor(i);
        if (has_weapon(i, WPN_PROBE_TEAM)) {
            if (has_chassis(i, CHS_FOIL)) {
                tx_propose_proto(i, CHS_FOIL, WPN_PROBE_TEAM, ARM_NO_ARMOR, 0,
                    rec, PLAN_INFO_WARFARE, "Foil Probe Team");
            }
            if (has_ability(i, ABL_ID_ALGO_ENHANCEMENT)) {
                int chs = has_chassis(i, CHS_HOVERTANK) ? CHS_HOVERTANK : CHS_SPEEDER;
                tx_propose_proto(i, chs, WPN_PROBE_TEAM, ARM_NO_ARMOR, ABL_ALGO_ENHANCEMENT,
                    rec, PLAN_INFO_WARFARE, "Enhanced Probe Team");
            }
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
    move_upkeep();

    return 0;
}

int tech_value(int tech, int fac, int value) {
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

int find_hq(int faction) {
    for(int i=0; i<*tx_total_num_bases; i++) {
        BASE* base = &tx_bases[i];
        if (base->faction_id == faction && has_facility(i, FAC_HEADQUARTERS)) {
            return i;
        }
    }
    return -1;
}

bool can_build(int base_id, int id) {
    BASE* base = &tx_bases[base_id];
    Faction* faction = &tx_factions[base->faction_id];
    if (id == FAC_STOCKPILE_ENERGY)
        return false;
    if (id == FAC_HEADQUARTERS && find_hq(base->faction_id) >= 0)
        return false;
    if (id == FAC_RECYCLING_TANKS && has_facility(base_id, FAC_PRESSURE_DOME))
        return false;
    if ((id == FAC_SKY_HYDRO_LAB && faction->satellites_nutrient >= MAX_SAT)
    || (id == FAC_ORBITAL_POWER_TRANS && faction->satellites_energy >= MAX_SAT)
    || (id == FAC_NESSUS_MINING_STATION && faction->satellites_mineral >= MAX_SAT)
    || (id == FAC_ORBITAL_DEFENSE_POD && faction->satellites_ODP >= MAX_SAT))
        return false;
    return knows_tech(base->faction_id, tx_facilities[id].preq)
    && !has_facility(base_id, id);
}

int find_project(int fac) {
    int bases = tx_factions[fac].current_num_bases;
    int nuke_limit = (tx_factions[fac].planet_busters < 2 ? 1 : 0);
    int projs = 0;
    int nukes = 0;

    bool build_nukes = (*tx_un_charter_repeals > *tx_un_charter_reinstates &&
        has_weapon(fac, WPN_PLANET_BUSTER));

    for (int i=0; i<*tx_total_num_bases; i++) {
        if (tx_bases[i].faction_id == fac) {
            int prod = tx_bases[i].queue_production_id[0];
            if (prod <= -70)
                projs++;
            else if (prod >= 0 && tx_units[prod].weapon_type == WPN_PLANET_BUSTER)
                nukes++;
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
        int projects[40];
        int n = 0;
        for (int i=70; i<107; i++) {
            if (tx_secret_projects[i-70] == -1 && knows_tech(fac, tx_facilities[i].preq)) {
                debuglog("find_project %d %d %s\n", fac, i, (char*)tx_facilities[i].name);
                projects[n++] = i;
            }
        }
        return (n > 0 ? -1*projects[random(n)] : 0);
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

int want_base(MAP* tile, int triad) {
    if (triad != TRIAD_SEA && tile->altitude >= ALTITUDE_MIN_LAND) {
        return true;
    } else if (triad == TRIAD_SEA && (tile->level >> 5) == LEVEL_OCEAN_SHELF) {
        return true;
    }
    return false;
}

int consider_base(int id) {
    VEH* veh = &tx_vehicles[id];
    MAP* tile = mapsq(veh->x_coord, veh->y_coord);
    if (!tile || (tile->rocks & TILE_ROCKY) || (tile->built_items & BASE_DISALLOWED)
    || bases_in_range(veh->x_coord, veh->y_coord, 2) > 0
    || !want_base(tile, unit_triad(veh->proto_id)))
        return tx_enemy_move(id);
    tx_action_build(id, 0);
    return SYNC;
}

int want_convoy(int fac, int x, int y, MAP* tile) {
    if (tile && tile->owner == fac && !convoys.count(mp(x, y))) {
        int bonus = tx_bonus_at(x, y);
        if (bonus == RES_ENERGY)
            return RES_NONE;
        else if (tile->built_items & TERRA_CONDENSER)
            return RES_NUTRIENT;
        else if (tile->built_items & TERRA_MINE && tile->rocks & TILE_ROCKY)
            return RES_MINERAL;
        else if (tile->built_items & TERRA_FOREST && ~tile->built_items & TERRA_RIVER
        && bonus != RES_NUTRIENT)
            return RES_MINERAL;
    }
    return RES_NONE;
}

int consider_convoy(int id) {
    VEH* veh = &tx_vehicles[id];
    MAP* tile = mapsq(veh->x_coord, veh->y_coord);
    if (!at_target(veh))
        return SYNC;
    if (veh->home_base_id < 0)
        return tx_veh_skip(id);
    BASE* base = &tx_bases[veh->home_base_id];
    int res = want_convoy(veh->faction_id, veh->x_coord, veh->y_coord, tile);
    bool prefer_min = base->nutrient_surplus > min(8, 4 + base->pop_size/2);
    bool forest_sq = tile->built_items & TERRA_FOREST && res == RES_MINERAL
        && veh->move_status == STATUS_CONVOY;

    if (res && !forest_sq) {
        return set_convoy(id, res);
    }
    int i = 0;
    int cx = -1;
    int cy = -1;
    TileSearch ts;
    ts.init(veh->x_coord, veh->y_coord, LAND_ONLY);

    while (i++ < 40 && (tile = ts.get_next()) != NULL) {
        int other = unit_in_tile(tile);
        if (other > 0 && tx_factions[veh->faction_id].diplo_status[other] & DIPLO_VENDETTA) {
            debuglog("convoy_skip %d %d %d %d %d %d\n", veh->x_coord, veh->y_coord,
                ts.cur_x, ts.cur_y, veh->faction_id, other);
            return tx_veh_skip(id);
        }
        res = want_convoy(veh->faction_id, ts.cur_x, ts.cur_y, tile);
        if (res) {
            if (prefer_min && res == RES_MINERAL && ~tile->built_items & TERRA_FOREST) {
                return veh_move_to(veh, ts.cur_x, ts.cur_y);
            } else if (!prefer_min && res == RES_NUTRIENT) {
                return veh_move_to(veh, ts.cur_x, ts.cur_y);
            }
            if (res == RES_MINERAL && cx == -1) {
                cx = ts.cur_x;
                cy = ts.cur_y;
            }
        }
    }
    if (forest_sq)
        return set_convoy(id, RES_MINERAL);
    if (cx >= 0)
        return veh_move_to(veh, cx, cy);
    return tx_veh_skip(id);
}

int terraform_action(int id, int action, int flag) {
    return tx_action_terraform(id, action, flag);
}

bool random_switch(int id1, int id2) {
    UNIT* u1 = &tx_units[id1];
    UNIT* u2 = &tx_units[id2];
    int rnd = (unit_speed(id1) > unit_speed(id2) ? 6 : 3);
    if ((u1->ability_flags & ABL_DROP_POD) > (u2->ability_flags & ABL_DROP_POD))
        rnd = 6;
    return random(rnd) == 0;
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
            bool is_def = offense_value(u) < defense_value(u);
            if (best == basic) {
                best = id;
                debuglog("===> %s\n", (char*)&(tx_units[best].name));
            } else if (mode || (defend == is_def)) {
                int v1 = (defend ? defense_value(u) : offense_value(u));
                int v2 = (defend ? defense_value(&tx_units[best]) : offense_value(&tx_units[best]));
                bool cheap = (defend && u->cost < tx_units[best].cost);
                if (v1 > v2 || (v1 == v2 && (cheap || random_switch(best, id)))) {
                    best = id;
                    debuglog("===> %s\n", (char*)&(tx_units[best].name));
                }
            }
        }
    }
    return best;
}

int find_facility(int base_id, int fac) {
    const int build_order[] = {
        FAC_RECREATION_COMMONS,
        FAC_CHILDREN_CRECHE,
        FAC_PERIMETER_DEFENSE,
        FAC_GENEJACK_FACTORY,
        FAC_NETWORK_NODE,
        FAC_TREE_FARM,
        FAC_HAB_COMPLEX,
        FAC_AEROSPACE_COMPLEX,
        FAC_COMMAND_CENTER,
        FAC_HYBRID_FOREST,
        FAC_FUSION_LAB,
        FAC_ENERGY_BANK,
        FAC_RESEARCH_HOSPITAL,
        FAC_HABITATION_DOME,
    };
    BASE* base = &tx_bases[base_id];
    int proj;
    int has_supply = has_weapon(fac, WPN_SUPPLY_TRANSPORT);
    int minerals = base->mineral_surplus;
    int extra = base->minerals_accumulated/10;
    int threshold = min(12, 4 + *tx_current_turn / (has_supply ? 20 : 30));
    int hab_complex_limit = tx_basic->pop_limit_wo_hab_complex
        - tx_factions_meta[fac].rule_population;

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
    if (minerals+extra >= threshold && (proj = find_project(fac)) != 0) {
        return proj;
    }
    if (minerals+extra >= threshold && knows_tech(fac, tx_facilities[FAC_SKY_HYDRO_LAB].preq)) {
        if (can_build(base_id, FAC_AEROSPACE_COMPLEX))
            return -FAC_AEROSPACE_COMPLEX;
        if (can_build(base_id, FAC_ORBITAL_DEFENSE_POD))
            return -FAC_ORBITAL_DEFENSE_POD;
        if (can_build(base_id, FAC_NESSUS_MINING_STATION))
            return -FAC_NESSUS_MINING_STATION;
        if (can_build(base_id, FAC_ORBITAL_POWER_TRANS))
            return -FAC_ORBITAL_POWER_TRANS;
        if (can_build(base_id, FAC_SKY_HYDRO_LAB))
            return -FAC_SKY_HYDRO_LAB;
    }
    if (minerals >= 5 && find_hq(fac) < 0 && bases_in_range(base->x_coord, base->y_coord, 4) >= 5)
        return -FAC_HEADQUARTERS;

    for (int f : build_order) {
        if (base->energy_surplus < 6 && (f == FAC_NETWORK_NODE || f == FAC_ENERGY_BANK
        || f == FAC_FUSION_LAB || f == FAC_RESEARCH_HOSPITAL))
            continue;
        if (f == FAC_GENEJACK_FACTORY && base->mineral_intake < 16)
            continue;
        if (f == FAC_HYBRID_FOREST && base->eco_damage < 2)
            continue;
        if (can_build(base_id, f)) {
            return -1*f;
        }
    }
    if (has_weapon(fac, WPN_PROBE_TEAM)) {
        return find_proto(fac, TRIAD_LAND, WMODE_INFOWAR, true);
    }
    return find_proto(fac, TRIAD_LAND, COMBAT, false);
}

int select_prod(int id) {
    BASE* base = &tx_bases[id];
    int owner = base->faction_id;
    int prod = base->queue_production_id[0];
    int minerals = base->mineral_surplus;
    int accumulated = base->minerals_accumulated;
    Faction* faction = &tx_factions[owner];

    if (prod < 0 && !can_build(id, abs(prod))) {
        debuglog("BUILD CHANGE\n");
        if (accumulated > 10)
            return find_facility(id, owner);
    } else if (prod < 0 || (accumulated > 10 && base_mins[id] < accumulated)) {
        debuglog("BUILD OLD\n");
        return prod;
    }

    int defenders = 0;
    int crawlers = 0;
    int formers = 0;
    int probes = 0;
    int pods = 0;
    int enemies = 0;
    int enemymask = 1;
    int enemyrange = 40;
    double enemymil = 0;
    double enemydist = 0;

    for (int i=1; i<8; i++) {
        if (i==owner || ~faction->diplo_status[i] & DIPLO_COMMLINK)
            continue;
        double mil = (1.0 * tx_factions[i].mil_strength_1)
            / max(1, faction->mil_strength_1);
        if (faction->diplo_status[i] & DIPLO_VENDETTA) {
            enemymask |= (1 << i);
            enemymil = max(enemymil, 1.0 * mil);
        } else if (~faction->diplo_status[i] & DIPLO_PACT) {
            enemymil = max(enemymil, 0.5 * mil);
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
        if (veh->faction_id != owner && (1 << veh->faction_id) & ~enemymask)
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

        if (veh->faction_id == owner && range <= 2) {
            if (unit_triad(veh->proto_id) == TRIAD_LAND && unit->weapon_type <= WPN_PSI_ATTACK) {
                if (range == 0)
                    defenders++;
                enemydist -= 0.2;
            }
        } else if ((1 << veh->faction_id) & enemymask) {
            if (range <= 10) {
                enemydist += (veh->faction_id == 0 ? 0.8 : 1.0) / max(1, range);
                enemies++;
            }
        }
    }
    bool has_formers = has_weapon(owner, WPN_TERRAFORMING_UNIT);
    bool has_supply = has_weapon(owner, WPN_SUPPLY_TRANSPORT);
    bool has_probes = has_weapon(owner, WPN_PROBE_TEAM);
    bool has_planes = has_chassis(owner, CHS_NEEDLEJET);
    bool can_build_ships = has_chassis(owner, CHS_FOIL)
        && count_sea_tiles(base->x_coord, base->y_coord, 25) >= 25;
    bool land_area_full = switch_to_sea(base->x_coord, base->y_coord);
    bool build_pods = (base->pop_size > 1 || base->nutrient_surplus > 1)
        && pods < (*tx_current_turn < 60 || (can_build_ships && land_area_full) ? 2 : 1)
        && faction->current_num_bases * 2 < min(80, *tx_map_area_sq_root);
    bool sea_base = water_base(id);

    enemymil = sqrt(enemymil / (2 + enemyrange)) * 5.0;
    if (land_area_full)
        enemymil = max((faction->AI_fight * 0.2 + 0.5), enemymil);
    int reserve = max(2, base->mineral_intake / 2);
    double w = (has_facility(id, FAC_PERIMETER_DEFENSE) ? 0.7 : 1.0);
    double threat = 1 - (1 / (1 + w * max(0.0, enemymil + enemydist)));

    debuglog("select_prod %d %d %d | %d %d %d %d %d %d %d %d | %d %d %.4f %.4f %.4f\n",
    *tx_current_turn, owner, id,
    defenders, formers, pods, crawlers, build_pods, land_area_full, minerals, reserve,
    enemyrange, enemies, enemymil, enemydist, threat);

    if (defenders == 0 && minerals > 2) {
        return find_proto(owner, TRIAD_LAND, COMBAT, true);
    } else if (minerals > reserve && random(100) < (int)(100.0 * threat)) {
        if (defenders > 2 && enemyrange < 12 && can_build(id, FAC_PERIMETER_DEFENSE))
            return -FAC_PERIMETER_DEFENSE;
        if (has_planes && faction->SE_police >= -3 && random(3) == 0)
            return find_proto(owner, TRIAD_AIR, COMBAT, false);
        else if (sea_base || (can_build_ships && enemydist < 0.5 && random(3) == 0))
            if (random(3) == 0)
                return find_proto(owner, TRIAD_SEA, WMODE_TRANSPORT, true);
            else
                return find_proto(owner, TRIAD_SEA, COMBAT, false);
        else
            return find_proto(owner, TRIAD_LAND, COMBAT, false);
    } else if (has_formers && formers <= min(1, base->pop_size/(sea_base ? 6 : 3))) {
        if (sea_base)
            return find_proto(owner, TRIAD_SEA, WMODE_TERRAFORMER, true);
        else
            return find_proto(owner, TRIAD_LAND, WMODE_TERRAFORMER, true);
    } else {
        if (has_supply && crawlers <= min(2, base->pop_size/3) && !sea_base)
            return BSC_SUPPLY_CRAWLER;
        if (build_pods && !can_build(id, FAC_RECYCLING_TANKS))
            if (can_build_ships && land_area_full)
                return find_proto(owner, TRIAD_SEA, WMODE_COLONIST, true);
            else
                return BSC_COLONY_POD;
        else if (has_probes && probes < 1 && random(3) == 0)
            if (can_build_ships)
                return find_proto(owner, TRIAD_SEA, WMODE_INFOWAR, true);
            else
                return find_proto(owner, TRIAD_LAND, WMODE_INFOWAR, true);
        else
            return find_facility(id, owner);
    }
}








