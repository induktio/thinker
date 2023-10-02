
#include "path.h"

CPath* Path = (CPath*)0x945B00;
FPath_find Path_find = (FPath_find)0x59A530;

static TileSearch ts;

int CPath::find(int x1, int y1, int x2, int y2, int unit_id, int faction_id) {
    return Path_find(this, x1, y1, x2, y2, unit_id, faction_id, 0, -1);
}

int path_get_next(int x1, int y1, int x2, int y2, int unit_id, int faction_id) {
    return Path_find(Path, x1, y1, x2, y2, unit_id, faction_id, 0, -1);
}

/*
Return tile distance to destination if it is less than MaxMapH, otherwise return -1.
*/
int path_distance(int x1, int y1, int x2, int y2, int unit_id, int faction_id) {
    int px = x1;
    int py = y1;
    int val = 0;
    int dist = 0;
    memset(pm_overlay, 0, sizeof(pm_overlay));

    while (dist < MaxMapH && val >= 0) {
        pm_overlay[px][py]++;
        if (px == x2 && py == y2) {
            return dist;
        }
        val = Path_find(Path, px, py, x2, y2, unit_id, faction_id, 0, -1);
        if (val >= 0) {
            px = wrap(px + BaseOffsetX[val]);
            py = py + BaseOffsetY[val];
        }
        dist++;
        debug("path_dist %2d %2d -> %2d %2d / %2d %2d / %2d\n", x1,y1,x2,y2,px,py,dist);
    }
    flushlog();
    return -1;
}

/*
Return road move distance to destination if it is less than max_cost, otherwise return -1.
*/
int path_cost(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int max_cost) {
    int px = x1;
    int py = y1;
    int val = 0;
    int cost = 0;

    while (val >= 0 && cost < max_cost) {
        if (px == x2 && py == y2) {
            return cost;
        }
        val = Path_find(Path, px, py, x2, y2, unit_id, faction_id, 0, -1);
        if (val >= 0) {
            px = wrap(px + BaseOffsetX[val]);
            py = py + BaseOffsetY[val];
            cost += mod_hex_cost(unit_id, faction_id, x1, y1, px, py, 0);
        }
        debug("path_cost %2d %2d -> %2d %2d / %2d %2d / %2d\n", x1,y1,x2,y2,px,py,cost);
    }
    return -1;
}

int nearby_items(int x, int y, int range, uint32_t item) {
    assert(range >= 0 && range <= MaxTableRange);
    MAP* sq;
    int n = 0;
    for (int i = 0; i < TableRange[range]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if ((sq = mapsq(x2, y2)) && sq->items & item) {
            n++;
        }
    }
    return n;
}

bool nearby_tiles(int x, int y, TSType type, int limit) {
    MAP* sq;
    int n = 0;
    ts.init(x, y, type, 2);
    while ((sq = ts.get_next()) != NULL) {
        if (++n >= limit) {
            return true;
        }
    }
    return false;
}

std::vector<MapTile> iterate_tiles(int x, int y, size_t start_index, size_t end_index) {
    std::vector<MapTile> tiles;
    assert(start_index < end_index && end_index <= sizeof(TableOffsetX)/sizeof(int));

    for (size_t i = start_index; i < end_index; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            tiles.push_back({x2, y2, (int)i, sq});
        }
    }
    return tiles;
}

void TileSearch::reset() {
    type = 0;
    head = 0;
    tail = 0;
    roads = 0;
    y_skip = 0;
    faction = -1;
    oldtiles.clear();
}

void TileSearch::add_start(int x, int y) {
    assert(!((x + y)&1));
    assert(type >= TS_TRIAD_LAND && type <= MaxTileSearchType);
    if (tail < QueueSize/2 && (sq = mapsq(x, y)) && !oldtiles.count({x, y})) {
        paths[tail] = {x, y, 0, -1};
        oldtiles.insert({x, y});
        if (!is_ocean(sq)) {
            if (sq->items & (BIT_ROAD | BIT_BASE_IN_TILE)) {
                roads |= BIT_ROAD | BIT_BASE_IN_TILE;
            }
            if (sq->items & (BIT_RIVER | BIT_BASE_IN_TILE)) {
                roads |= BIT_RIVER | BIT_BASE_IN_TILE;
            }
        }
        tail++;
    }
}

void TileSearch::init(int x, int y, int tp) {
    reset();
    type = tp;
    add_start(x, y);
}

void TileSearch::init(int x, int y, int tp, int skip) {
    reset();
    type = tp;
    y_skip = skip;
    add_start(x, y);
}

void TileSearch::init(const PointList& points, TSType tp, int skip) {
    reset();
    type = tp;
    y_skip = skip;
    for (auto& p : points) {
        add_start(p.x, p.y);
    }
}

void TileSearch::get_route(PointList& pp) {
    pp.clear();
    int i = 0;
    int j = current;
    while (j >= 0 && ++i < PathLimit) {
        auto& p = paths[j];
        j = p.prev;
        pp.push_front({p.x, p.y});
    }
}

/*
Traverse current search path and check for zones of control.
*/
bool TileSearch::has_zoc(int faction_id) {
    bool prev_zoc = false;
    int i = 0;
    int j = current;
    while (j >= 0 && ++i < PathLimit) {
        auto& p = paths[j];
        j = p.prev;
        if (zoc_any(p.x, p.y, faction_id)) {
            if (prev_zoc) return true;
            prev_zoc = true;
        } else {
            prev_zoc = false;
        }
    }
    return false;
}

/*
Implement a breadth-first search of adjacent map tiles to iterate possible movement paths.
Pathnodes are also used to keep track of the route to reach the current destination.
*/
MAP* TileSearch::get_next() {
    while (head < tail) {
        bool first = !head;
        rx = paths[head].x;
        ry = paths[head].y;
        dist = paths[head].dist;
        current = head;
        head++;
        if (!(sq = mapsq(rx, ry))) {
            continue;
        }
        if (first) {
            faction = sq->veh_owner();
            if (faction < 0) {
                faction = sq->owner;
            }
        }
        bool our_base = sq->is_base() && (faction == sq->owner || has_pact(faction, sq->owner));
        bool skip = (sq->is_base() && !our_base) ||
                    (type == TS_TRIAD_LAND && is_ocean(sq)) ||
                    (type == TS_TRIAD_SEA && !is_ocean(sq) && !our_base) ||
                    (type == TS_SEA_AND_SHORE && !is_ocean(sq) && !our_base) ||
                    (type == TS_NEAR_ROADS && (is_ocean(sq) || !(sq->items & roads))) ||
                    (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction)) ||
                    (type == TS_TERRITORY_BORDERS && (is_ocean(sq) || sq->owner != faction));
        if (dist > 0 && skip) {
            if ((type == TS_NEAR_ROADS && is_ocean(sq))
            || (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction))
            || (type == TS_TRIAD_LAND && is_ocean(sq))
            || (type == TS_TRIAD_SEA && !is_ocean(sq))) {
                continue;
            }
            return sq;
        }
        for (const int* t : NearbyTiles) {
            int x2 = wrap(rx + t[0]);
            int y2 = ry + t[1];
            if (y2 >= y_skip && y2 < *map_axis_y - y_skip
            && x2 >= 0 && x2 < *map_axis_x
            && tail < QueueSize && dist < PathLimit
            && !oldtiles.count({x2, y2})) {
                paths[tail] = {x2, y2, dist+1, current};
                tail++;
                oldtiles.insert({x2, y2});
            }
        }
        if (dist > 0) {
            return sq;
        }
    }
    return NULL;
}

