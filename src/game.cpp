
#include "game.h"


char* prod_name(int prod) {
    if (prod >= 0)
        return (char*)&(tx_units[prod].name);
    else
        return (char*)(tx_facility[-prod].name);
}

int mineral_cost(int fac, int prod) {
    if (prod >= 0)
        return tx_units[prod].cost * tx_cost_factor(fac, 1, -1);
    else
        return tx_facility[-prod].cost * tx_cost_factor(fac, 1, -1);
}

bool knows_tech(int fac, int tech) {
    if (tech == TECH_None)
        return true;
    return tech >= 0 && tx_tech_discovered[tech] & (1 << fac);
}

bool has_ability(int fac, int abl) {
    return knows_tech(fac, tx_ability[abl].preq_tech);
}

bool has_chassis(int fac, int chs) {
    return knows_tech(fac, tx_chassis[chs].preq_tech);
}

bool has_weapon(int fac, int wpn) {
    return knows_tech(fac, tx_weapon[wpn].preq_tech);
}

bool has_terra(int fac, int act) {
    if (act >= FORMER_CONDENSER && act <= FORMER_LOWER_LAND
    && act != FORMER_THERMAL_BORE && has_project(fac, FAC_WEATHER_PARADIGM))
        return true;
    return knows_tech(fac, tx_terraform[act].preq_tech);
}

bool has_project(int fac, int id) {
    int i = tx_secret_projects[id-70];
    if (i >= 0 && tx_bases[i].faction_id == fac)
        return true;
    return false;
}

bool has_facility(int base_id, int id) {
    if (id >= 70)
        return tx_secret_projects[id-70] != -1;
    int fac = tx_bases[base_id].faction_id;
    const int freebies[] = {
        FAC_COMMAND_CENTER, FAC_COMMAND_NEXUS,
        FAC_NAVAL_YARD, FAC_MARITIME_CONTROL_CENTER,
        FAC_ENERGY_BANK, FAC_PLANETARY_ENERGY_GRID,
        FAC_PERIMETER_DEFENSE, FAC_CITIZENS_DEFENSE_FORCE,
        FAC_AEROSPACE_COMPLEX, FAC_CLOUDBASE_ACADEMY,
        FAC_BIOENHANCEMENT_CENTER, FAC_CYBORG_FACTORY};
    for (int i=0; i<12; i+=2) {
        if (freebies[i] == id && has_project(fac, freebies[i+1]))
            return true;
    }
    int val = tx_bases[base_id].facilities_built[ id/8 ] & (1 << (id % 8));
    return val != 0;
}

bool can_build(int base_id, int id) {
    BASE* base = &tx_bases[base_id];
    Faction* fact = &tx_factions[base->faction_id];
    if (id == FAC_STOCKPILE_ENERGY)
        return false;
    if (id == FAC_HEADQUARTERS && find_hq(base->faction_id) >= 0)
        return false;
    if (id == FAC_RECYCLING_TANKS && has_facility(base_id, FAC_PRESSURE_DOME))
        return false;
    if (id == FAC_HOLOGRAM_THEATRE && (has_project(base->faction_id, FAC_VIRTUAL_WORLD)
    || !has_facility(base_id, FAC_RECREATION_COMMONS)))
        return false;
    if (id == FAC_ASCENT_TO_TRANSCENDENCE)
        return has_facility(-1, FAC_VOICE_OF_PLANET)
            && !has_facility(-1, FAC_ASCENT_TO_TRANSCENDENCE);
    if ((id == FAC_SKY_HYDRO_LAB && fact->satellites_nutrient >= MAX_SAT)
    || (id == FAC_ORBITAL_POWER_TRANS && fact->satellites_energy >= MAX_SAT)
    || (id == FAC_NESSUS_MINING_STATION && fact->satellites_mineral >= MAX_SAT)
    || (id == FAC_ORBITAL_DEFENSE_POD && fact->satellites_ODP >= MAX_SAT))
        return false;
    return knows_tech(base->faction_id, tx_facility[id].preq_tech) && !has_facility(base_id, id);
}

bool at_war(int a, int b) {
    if (a == b || a < 0 || b < 0)
        return false;
    return !a || !b || tx_factions[a].diplo_status[b] & DIPLO_VENDETTA;
}

int find_hq(int fac) {
    for(int i=0; i<*tx_total_num_bases; i++) {
        BASE* base = &tx_bases[i];
        if (base->faction_id == fac && has_facility(i, FAC_HEADQUARTERS)) {
            return i;
        }
    }
    return -1;
}

int veh_triad(int id) {
    return unit_triad(tx_vehicles[id].proto_id);
}

int veh_speed(int id) {
    return tx_veh_speed(id, 0);
}

int unit_triad(int id) {
    int tr = tx_chassis[tx_units[id].chassis_type].triad;
    assert(tr == TRIAD_LAND || tr == TRIAD_SEA || tr == TRIAD_AIR);
    return tr;
}

int unit_speed(int id) {
    return tx_chassis[tx_units[id].chassis_type].speed;
}

int best_armor(int fac, bool cheap) {
    int ci = ARM_NO_ARMOR;
    int cv = 4;
    for (int i=ARM_SYNTHMETAL_ARMOR; i<=ARM_RESONANCE_8_ARMOR; i++) {
        if (knows_tech(fac, tx_defense[i].preq_tech)) {
            int val = tx_defense[i].defense_value;
            int cost = tx_defense[i].cost;
            if (cheap && (cost > 6 || cost > val))
                continue;
            int iv = val * (i >= ARM_PULSE_3_ARMOR ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return ci;
}

int best_weapon(int fac) {
    int ci = WPN_HAND_WEAPONS;
    int cv = 4;
    for (int i=WPN_LASER; i<=WPN_STRING_DISRUPTOR; i++) {
        if (knows_tech(fac, tx_weapon[i].preq_tech)) {
            int iv = tx_weapon[i].offense_value *
                (i == WPN_RESONANCE_LASER || i == WPN_RESONANCE_BOLT ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return ci;
}

int best_reactor(int fac) {
    for (const int r : {REC_SINGULARITY, REC_QUANTUM, REC_FUSION}) {
        if (knows_tech(fac, tx_reactor[r - 1].preq_tech)) {
            return r;
        }
    }
    return REC_FISSION;
}

int offense_value(UNIT* u) {
    if (u->weapon_type == WPN_CONVENTIONAL_PAYLOAD)
        return tx_factions[*tx_active_faction].best_weapon_value * u->reactor_type;
    return tx_weapon[u->weapon_type].offense_value * u->reactor_type;
}

int defense_value(UNIT* u) {
    return tx_defense[u->armor_type].defense_value * u->reactor_type;
}

int random(int n) {
    return (n != 0 ? rand() % n : 0);
}

int wrap(int a) {
    if (!*tx_map_toggle_flat)
        return (a < 0 ? a + *tx_map_axis_x : a % *tx_map_axis_x);
    else
        return a;
}

int map_range(int x1, int y1, int x2, int y2) {
    int xd = abs(x1-x2);
    int yd = abs(y1-y2);
    if (!*tx_map_toggle_flat && xd > *tx_map_axis_x/2)
        xd = *tx_map_axis_x - xd;
    return (xd + yd)/2;
}

MAP* mapsq(int x, int y) {
    if (x >= 0 && y >= 0 && x < *tx_map_axis_x && y < *tx_map_axis_y) {
        int i = x/2 + (*tx_map_half_x) * y;
        return &((*tx_map_ptr)[i]);
    } else {
        return NULL;
    }
}

int unit_in_tile(MAP* sq) {
    if (!sq || (sq->flags & 0xf) == 0xf)
        return -1;
    return sq->flags & 0xf;
}

int set_move_to(int id, int x, int y) {
    VEH* veh = &tx_vehicles[id];
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->move_status = STATUS_GOTO;
    veh->status_icon = 'G';
    return SYNC;
}

int set_road_to(int id, int x, int y) {
    VEH* veh = &tx_vehicles[id];
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->move_status = STATUS_ROAD_TO;
    veh->status_icon = 'R';
    return SYNC;
}

int set_action(int id, int act, char icon) {
    VEH* veh = &tx_vehicles[id];
    if (act == FORMER_THERMAL_BORE+4)
        boreholes.insert(mp(veh->x, veh->y));
    veh->move_status = act;
    veh->status_icon = icon;
    veh->flags_1 &= 0xFFFEFFFF;
    return SYNC;
}

int set_convoy(int id, int res) {
    VEH* veh = &tx_vehicles[id];
    convoys.insert(mp(veh->x, veh->y));
    veh->type_crawling = res-1;
    veh->move_status = STATUS_CONVOY;
    veh->status_icon = 'C';
    return tx_veh_skip(id);
}

int veh_skip(int id) {
    VEH* veh = &tx_vehicles[id];
    veh->waypoint_1_x = veh->x;
    veh->waypoint_1_y = veh->y;
    veh->status_icon = '-';
    if (veh->damage_taken) {
        MAP* sq = mapsq(veh->x, veh->y);
        if (sq && sq->items & TERRA_MONOLITH)
            tx_monolith(id);
    }
    return tx_veh_skip(id);
}

int at_target(VEH* veh) {
    return (veh->waypoint_1_x < 0 && veh->waypoint_1_y < 0)
        || (veh->x == veh->waypoint_1_x && veh->y == veh->waypoint_1_y);
}

bool is_ocean(MAP* sq) {
    return (!sq || (sq->level >> 5) < LEVEL_SHORE_LINE);
}

bool is_ocean_shelf(MAP* sq) {
    return (sq && (sq->level >> 5) == LEVEL_OCEAN_SHELF);
}

bool water_base(int id) {
    MAP* sq = mapsq(tx_bases[id].x, tx_bases[id].y);
    return is_ocean(sq);
}

bool workable_tile(int x, int y, int fac) {
    MAP* sq;
    for (int i=0; i<20; i++) {
        sq = mapsq(wrap(x + offset_tbl[i][0]), y + offset_tbl[i][1]);
        if (sq && sq->owner == fac && sq->items & TERRA_BASE_IN_TILE) {
            return true;
        }
    }
    return false;
}

int nearby_items(int x, int y, int range, uint32_t item) {
    MAP* sq;
    int n = 0;
    for (int i=-range*2; i<=range*2; i++) {
        for (int j=-range*2 + abs(i); j<=range*2 - abs(i); j+=2) {
            sq = mapsq(wrap(x + i), y + j);
            if (sq && sq->items & item) {
                n++;
            }
        }
    }
    //debuglog("nearby_items %08x %d %d %d %d\n", item, x, y, range, n);
    return n;
}

int bases_in_range(int x, int y, int range) {
    return nearby_items(x, y, range, TERRA_BASE_IN_TILE);
}

int nearby_tiles(int x, int y, int type, int limit) {
    MAP* sq;
    int n = 0;
    TileSearch ts;
    ts.init(x, y, type, 2);
    while (n < limit && (sq = ts.get_next()) != NULL) {
        n++;
    }
    //debuglog("nearby_tiles %d %d %d %d\n", x, y, type, n);
    return n;
}

int coast_tiles(int x, int y) {
    MAP* sq;
    int n = 0;
    for (const int* t : offset) {
        sq = mapsq(wrap(x + t[0]), y + t[1]);
        if (is_ocean(sq)) {
            n++;
        }
    }
    return n;
}

void print_map(int x, int y) {
    MAP m = *mapsq(x, y);
    debuglog("MAP %2d %2d | %2d %d | %02x %02x %02x | %04x %02x | %02x %02x %02x | %08x %08x\n",
        x, y, m.owner, tx_bonus_at(x, y), m.level, m.altitude, m.rocks,
        m.flags, m.visibility, m.unk_1, m.unk_2, m.art_ref_id,
        m.items, m.landmarks);
}

void print_veh(int id) {
    VEH v = tx_vehicles[id];
    debuglog("VEH %20s %3d %d | %08x %04x %02x | %2d %3d | %2d %2d %2d %2d | %d %d %d %d %d %d\n",
        tx_units[v.proto_id].name, id, v.faction_id,
        v.flags_1, v.flags_2, v.visibility, v.move_status, v.status_icon,
        v.x, v.y, v.waypoint_1_x, v.waypoint_1_y,
        v.morale, v.damage_taken, v.iter_count, v.unk5, v.unk8, v.unk9);
}

void TileSearch::init(int x, int y, int tp) {
    head = 0;
    tail = 0;
    items = 0;
    roads = 0;
    y_skip = 0;
    type = tp;
    oldtiles.clear();
    sq = mapsq(x, y);
    if (sq) {
        items++;
        tail = 1;
        newtiles[0] = {x, y, 0, -1};
        oldtiles.insert({x, y});
        if (!is_ocean(sq)) {
            if (sq->items & (TERRA_ROAD | TERRA_BASE_IN_TILE))
                roads |= TERRA_ROAD;
            if (sq->items & (TERRA_RIVER | TERRA_BASE_IN_TILE))
                roads |= TERRA_RIVER;
        }
    }
}

void TileSearch::init(int x, int y, int tp, int skip) {
    init(x, y, tp);
    y_skip = skip;
}

bool TileSearch::has_zoc(int fac) {
    int zoc = 0;
    int i = 0;
    int j = head;
    while (j >= 0 && i++ < 20) {
        auto p = newtiles[j];
        if (tx_zoc_any(p.x, p.y, fac))
            zoc++;
        j = p.prev;
    }
    return zoc > 1;
}

MAP* TileSearch::prev_tile() {
    return (prev >= 0 ? mapsq(newtiles[prev].x, newtiles[prev].y) : NULL);
}

MAP* TileSearch::get_next() {
    while (items > 0) {
        bool first = oldtiles.size() == 1;
        rx = newtiles[head].x;
        ry = newtiles[head].y;
        dist = newtiles[head].dist;
        prev = head;
        head = (head + 1) % QSIZE;
        items--;
        if (!(sq = mapsq(rx, ry)))
            continue;
        bool skip = (type == LAND_ONLY && is_ocean(sq)) ||
                    (type == WATER_ONLY && !is_ocean(sq)) ||
                    (type == NEAR_ROADS && (is_ocean(sq) || !(sq->items & roads)));
        if (!first && skip) {
            if (type == NEAR_ROADS && !is_ocean(sq))
                return sq;
            continue;
        }
        for (const int* t : offset) {
            int x2 = wrap(rx + t[0]);
            int y2 = ry + t[1];
            if (y2 >= y_skip && y2 < *tx_map_axis_y - y_skip
            && items < QSIZE && !oldtiles.count({x2, y2})) {
                newtiles[tail] = {x2, y2, dist+1, prev};
                tail = (tail + 1) % QSIZE;
                items++;
                oldtiles.insert({x2, y2});
            }
        }
        if (!first) {
            return sq;
        }
    }
    return NULL;
}



