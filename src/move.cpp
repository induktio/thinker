
#include "move.h"

int pm_former[MAPSZ][MAPSZ];
int pm_safety[MAPSZ][MAPSZ];

void adjust_value(int x, int y, int range, int value, int tbl[MAPSZ][MAPSZ]) {
    for (int i=-range*2; i<=range*2; i++) {
        for (int j=-range*2 + abs(i); j<=range*2 - abs(i); j+=2) {
            int x2 = wrap(x + i);
            int y2 = y + j;
            if (mapsq(x2, y2)) {
                tbl[x2][y2] += value;
            }
        }
    }
}

bool other_in_tile(int fac, MAP* sq) {
    int u = unit_in_tile(sq);
    return (u > 0 && u != fac);
}

void move_upkeep() {
    convoys.clear();
    boreholes.clear();
    memset(pm_former, 0, sizeof(int)*MAPSZ*MAPSZ);
    memset(pm_safety, 0, sizeof(int)*MAPSZ*MAPSZ);

    for (int i=0; i<*tx_total_num_vehicles; i++) {
        VEH* veh = &tx_vehicles[i];
        if (veh->move_status == 18) {
            boreholes.insert(mp(veh->x_coord, veh->y_coord));
        }
        if (veh->faction_id == 0) {
            int v = (veh->proto_id == BSC_SPORE_LAUNCHER ? -400 : -100);
            adjust_value(veh->x_coord, veh->y_coord, 1, v, pm_safety);
            adjust_value(veh->x_coord, veh->y_coord, 2, v/5, pm_safety);
        }
    }
    for (int i=0; i<*tx_total_num_bases; i++) {
        BASE* base = &tx_bases[i];
        adjust_value(base->x_coord, base->y_coord, 2, base->pop_size, pm_former);
    }
}

bool can_bridge(int x, int y) {
    if (coast_tiles(x, y) < 3)
        return false;
    const int range = 4;
    MAP* sq;
    TileSearch ts;
    ts.init(x, y, LAND_ONLY);
    while (ts.visited() < 120 && ts.get_next() != NULL);

    int n = 0;
    for (int i=-range*2; i<=range*2; i++) {
        for (int j=-range*2 + abs(i); j<=range*2 - abs(i); j+=2) {
            int x2 = wrap(x + i);
            int y2 = y + j;
            sq = mapsq(x2, y2);
            if (sq && y2 > 0 && y2 < *tx_map_axis_y-1 && sq->altitude >= ALTITUDE_MIN_LAND
            && ts.oldtiles.count(mp(x2, y2)) == 0) {
                n++;
            }
        }
    }
    debuglog("bridge %d %d %d %d\n", x, y, ts.visited(), n);
    return n > 4;
}

bool can_borehole(int x, int y, int bonus) {
    MAP* sq = mapsq(x, y);
    if (!sq || sq->built_items & BASE_DISALLOWED || bonus == RES_NUTRIENT)
        return false;
    if (bonus == RES_NONE && sq->rocks & TILE_ROLLING)
        return false;
    if (pm_former[x][y] < 4 || !workable_tile(x, y, sq->owner))
        return false;
    int level = sq->level >> 5;
    for (const int* t : offset) {
        int x2 = wrap(x + t[0]);
        int y2 = y + t[1];
        sq = mapsq(x2, y2);
        if (!sq || sq->built_items & TERRA_THERMAL_BORE || boreholes.count(mp(x2, y2)))
            return false;
        int level2 = sq->level >> 5;
        if (level2 < level && level2 > LEVEL_OCEAN_SHELF)
            return false;
    }
    return true;
}

bool can_farm(int x, int y, int bonus, bool has_eco, MAP* sq) {
    if (bonus == RES_NUTRIENT)
        return true;
    if (bonus == RES_ENERGY || bonus == RES_MINERAL)
        return false;
    if (sq->landmarks & LM_JUNGLE && !has_eco)
        return false;
    if (nearby_items(x, y, TERRA_FOREST) < (sq->built_items & TERRA_FOREST ? 3 : 1))
        return false;
    if (sq->level & TILE_RAINY && sq->rocks & TILE_ROLLING)
        return true;
    return (has_eco && !(x % 2) && !(y % 2) && !(abs(x-y) % 4));
}

int select_item(int x, int y, int fac, MAP* sq) {
    int items = sq->built_items;
    int bonus = tx_bonus_at(x, y);
    bool rocky_sq = sq->rocks & TILE_ROCKY;
    bool has_eco = has_terra(fac, FORMER_CONDENSER);

    if (items & TERRA_BASE_IN_TILE)
        return -1;
    if (items & TERRA_FUNGUS)
        return FORMER_REMOVE_FUNGUS;
    if (~items & TERRA_ROAD)
        return FORMER_ROAD;
    if (has_terra(fac, FORMER_RAISE_LAND) && can_bridge(x, y)) {
        int cost = tx_terraform_cost(x, y, fac);
        if (cost < tx_factions[fac].energy_credits/10) {
            debuglog("bridge_cost %d %d %d %d\n", x, y, fac, cost);
            tx_factions[fac].energy_credits -= cost;
            return FORMER_RAISE_LAND;
        }
    }
    if (items & BASE_DISALLOWED || !workable_tile(x, y, fac))
        return -1;

    if (has_terra(fac, FORMER_THERMAL_BORE) && can_borehole(x, y, bonus))
        return FORMER_THERMAL_BORE;
    if (rocky_sq && (bonus == RES_NUTRIENT || sq->landmarks & LM_JUNGLE))
        return FORMER_LEVEL_TERRAIN;
    if (rocky_sq && ~items & TERRA_MINE && (has_eco || bonus == RES_MINERAL))
        return FORMER_MINE;
    if (!rocky_sq && can_farm(x, y, bonus, has_eco, sq)) {
        if (~items & TERRA_FARM && (has_eco || bonus == RES_NUTRIENT))
            return FORMER_FARM;
        else if (has_eco && ~items & TERRA_CONDENSER)
            return FORMER_CONDENSER;
        else if (has_terra(fac, FORMER_SOIL_ENR) && ~items & TERRA_SOIL_ENR)
            return FORMER_SOIL_ENR;
        else if (!has_eco && !(items & (TERRA_CONDENSER | TERRA_SOLAR)))
            return FORMER_SOLAR;
    } else if (!rocky_sq && !(items & (TERRA_FARM | TERRA_CONDENSER))) {
        if (!(x % 3) && !(y % 3) && ~items & TERRA_SENSOR)
            return FORMER_SENSOR;
        else if (~items & TERRA_FOREST)
            return FORMER_FOREST;
    }
    return -1;
}

int tile_score(int x1, int y1, int x2, int y2, MAP* sq) {
    const int priority[][2] = {
        {TERRA_RIVER, 2},
        {TERRA_RIVER_SRC, 2},
        {TERRA_FUNGUS, -2},
        {TERRA_ROAD, -5},
        {TERRA_FARM, -2},
        {TERRA_FOREST, -4},
        {TERRA_MINE, -4},
        {TERRA_CONDENSER, -4},
        {TERRA_SOIL_ENR, -4},
        {TERRA_THERMAL_BORE, -8},
    };
    int bonus = tx_bonus_at(x2, y2);
    int range = map_range(x1, y1, x2, y2);
    int items = sq->built_items;
    int score = (sq->landmarks ? 3 : 0);

    if (bonus && !(items & IMP_ADVANCED)) {
        int w = (items & IMP_SIMPLE ? 1 : 5);
        score += w * (bonus == RES_NUTRIENT ? 3 : 2);
    }
    for (const int* p : priority) {
        if (items & p[0]) {
            score += p[1];
        }
    }
    return score - range + min(8, pm_former[x2][y2]) + pm_safety[x2][y2];
}

int former_move(int id) {
    VEH* veh = &tx_vehicles[id];
    int fac = veh->faction_id;
    int x = veh->x_coord;
    int y = veh->y_coord;
    MAP* sq = mapsq(x, y);
    if (!sq || sq->owner != fac) {
        return tx_enemy_move(id);
    }
    if (pm_safety[x][y] >= PM_SAFE) {
        if (veh->move_status >= 4 && veh->move_status < 24) {
            return SYNC;
        }
        int item = select_item(x, y, fac, sq);
        if (item >= 0) {
            pm_former[x][y] -= 2;
            debuglog("former_action %d %d %d %d %d\n", x, y, fac, id, item);
            return set_action(id, item+4, *tx_terraform[item].shortcuts);
        }
    }
    int i = 0;
    int tscore = INT_MIN;
    int tx = -1;
    int ty = -1;
    TileSearch ts;
    ts.init(x, y, LAND_ONLY);

    while (i++ < 40 && (sq = ts.get_next()) != NULL) {
        if (sq->owner != fac || sq->built_items & TERRA_BASE_IN_TILE
        || pm_safety[ts.cur_x][ts.cur_y] < PM_SAFE
        || pm_former[ts.cur_x][ts.cur_y] < 1
        || other_in_tile(fac, sq))
            continue;
        int score = tile_score(x, y, ts.cur_x, ts.cur_y, sq);
        if (score > tscore) {
            tx = ts.cur_x;
            ty = ts.cur_y;
            tscore = score;
        }
    }
    if (tx >= 0) {
        pm_former[tx][ty] -= 2;
        debuglog("former_move %d %d -> %d %d %d %d %d\n", x, y, tx, ty, fac, id, tscore);
        return set_road_to(id, tx, ty);
    }
    return tx_veh_skip(id);
}



