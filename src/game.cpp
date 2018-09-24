
#include "game.h"


char* prod_name(int prod) {
    if (prod < 0)
        return (char*)(tx_facilities[-prod].name);
    else
        return (char*)&(tx_units[prod].name);
}

int mineral_cost(int fac, int prod) {
    if (prod >= 0)
        return tx_units[prod].cost * tx_cost_factor(fac, 1, -1);
    else
        return tx_facilities[-prod].cost * tx_cost_factor(fac, 1, -1);
}

bool knows_tech(int fac, int tech) {
    if (tech < 0)
        return true;
    return tx_tech_discovered[tech] & (1 << fac);
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
    && has_project(fac, FAC_WEATHER_PARADIGM))
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

int unit_triad(int id) {
    return tx_chassis[tx_units[id].chassis_type].triad;
}

int unit_speed(int id) {
    return tx_chassis[tx_units[id].chassis_type].speed;
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
    return rand() % (n != 0 ? n : 1);
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
    veh->waypoint_1_x_coord = x;
    veh->waypoint_1_y_coord = y;
    veh->move_status = STATUS_GOTO;
    veh->status_icon = 'G';
    return SYNC;
}

int set_road_to(int id, int x, int y) {
    VEH* veh = &tx_vehicles[id];
    veh->waypoint_1_x_coord = x;
    veh->waypoint_1_y_coord = y;
    veh->move_status = STATUS_ROAD_TO;
    veh->status_icon = 'R';
    return SYNC;
}

int set_action(int id, int act, char icon) {
    VEH* veh = &tx_vehicles[id];
    if (act == FORMER_THERMAL_BORE+4)
        boreholes.insert(mp(veh->x_coord, veh->y_coord));
    veh->move_status = act;
    veh->status_icon = icon;
    veh->flags_1 &= 0xFFFEFFFF;
    return SYNC;
}

int set_convoy(int id, int res) {
    VEH* veh = &tx_vehicles[id];
    convoys.insert(mp(veh->x_coord, veh->y_coord));
    veh->type_crawling = res-1;
    veh->move_status = STATUS_CONVOY;
    veh->status_icon = 'C';
    return tx_veh_skip(id);
}

int at_target(VEH* veh) {
    return (veh->waypoint_1_x_coord < 0 && veh->waypoint_1_y_coord < 0)
    || (veh->x_coord == veh->waypoint_1_x_coord && veh->y_coord == veh->waypoint_1_y_coord);
}

bool water_base(int id) {
    MAP* sq = mapsq(tx_bases[id].x_coord, tx_bases[id].y_coord);
    return (sq && sq->altitude < ALTITUDE_MIN_LAND);
}

bool workable_tile(int x, int y, int fac) {
    MAP* tile;
    for (const int* t : offset_20) {
        int x2 = wrap(x + t[0]);
        int y2 = y + t[1];
        tile = mapsq(x2, y2);
        if (tile && tile->owner == fac && tile->built_items & TERRA_BASE_IN_TILE) {
            return true;
        }
    }
    return false;
}

int nearby_items(int x, int y, int item) {
    int n = 0;
    for (const int* t : offset) {
        int x2 = wrap(x + t[0]);
        int y2 = y + t[1];
        MAP* tile = mapsq(x2, y2);
        if (tile && tile->built_items & item) {
            n++;
        }
    }
    return n;
}

int bases_in_range(int x, int y, int range) {
    MAP* tile;
    int n = 0;
    int bases = 0;
    for (int i=-range*2; i<=range*2; i++) {
        for (int j=-range*2 + abs(i); j<=range*2 - abs(i); j+=2) {
            tile = mapsq(wrap(x + i), y + j);
            n++;
            if (tile && tile->built_items & TERRA_BASE_IN_TILE)
                bases++;
        }
    }
    debuglog("bases_in_range %d %d %d %d %d\n", x, y, range, n, bases);
    return bases;
}

int coast_tiles(int x, int y) {
    MAP* tile;
    int n = 0;
    for (const int* t : offset) {
        tile = mapsq(wrap(x + t[0]), y + t[1]);
        if (tile && tile->altitude < ALTITUDE_MIN_LAND) {
            n++;
        }
    }
    return n;
}

void TileSearch::init(int x, int y, int tp) {
    head = 0;
    tail = 0;
    items = 0;
    type = tp;
    oldtiles.clear();
    tile = mapsq(x, y);
    if (tile) {
        items++;
        tail = 1;
        newtiles[0] = mp(x, y);
        oldtiles.insert(mp(x, y));
    }
}

int TileSearch::visited() {
    return oldtiles.size();
}

MAP* TileSearch::get_next() {
    while (items > 0) {
        bool first = oldtiles.size() == 1;
        cur_x = newtiles[head].first;
        cur_y = newtiles[head].second;
        head = (head + 1) % QSIZE;
        items--;
        if (!(tile = mapsq(cur_x, cur_y)))
            continue;
        bool skip = (type == LAND_ONLY && tile->altitude < ALTITUDE_MIN_LAND) ||
                    (type == WATER_ONLY && tile->altitude >= ALTITUDE_MIN_LAND);
        if (!first && skip)
            continue;
        for (const int* t : offset) {
            int x2 = wrap(cur_x + t[0]);
            int y2 = cur_y + t[1];
            if (items < QSIZE && y2 >= 0 && y2 < *tx_map_axis_y && oldtiles.count(mp(x2, y2)) == 0) {
                newtiles[tail] = mp(x2, y2);
                tail = (tail + 1) % QSIZE;
                items++;
                oldtiles.insert(mp(x2, y2));
            }
        }
        if (!first) {
            return tile;
        }
    }
    return NULL;
}



