
#include "path.h"


int path_get_next(int x1, int y1, int x2, int y2, int unit_id, int faction_id) {
    return Path_find(Paths, x1, y1, x2, y2, unit_id, faction_id, 0, -1);
}

/*
Return tile distance to destination if it is less than MaxMapDist, otherwise return -1.
*/
int path_distance(int x1, int y1, int x2, int y2, int unit_id, int faction_id) {
    int px = x1;
    int py = y1;
    int val = 0;
    int dist = 0;
    refresh_overlay(clear_overlay);

    while (val >= 0 && dist <= MaxMapDist) {
        if (DEBUG) { mapdata[{px, py}].overlay = dist; }
        if (px == x2 && py == y2) {
            return dist;
        }
        val = Path_find(Paths, px, py, x2, y2, unit_id, faction_id, 0, -1);
        if (val >= 0) {
            px = wrap(px + BaseOffsetX[val]);
            py = py + BaseOffsetY[val];
        }
        dist++;
        debug("path_dist %2d %2d -> %2d %2d / %2d %2d / %2d\n", x1, y1, x2, y2, px, py, dist);
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

    while (val >= 0 && cost <= max_cost) {
        if (px == x2 && py == y2) {
            return cost;
        }
        val = Path_find(Paths, px, py, x2, y2, unit_id, faction_id, 0, -1);
        if (val >= 0) {
            px = wrap(px + BaseOffsetX[val]);
            py = py + BaseOffsetY[val];
            cost += mod_hex_cost(unit_id, faction_id, x1, y1, px, py, 0);
        }
        debug_ver("path_cost %2d %2d -> %2d %2d / %2d %2d / %2d\n", x1, y1, x2, y2, px, py, cost);
    }
    return -1;
}

void update_path(PMTable& tbl, int veh_id, int tx, int ty) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(tx, ty);
    if (!is_human(veh->faction_id)
    && sq && (sq->owner == veh->faction_id || (at_war(veh->faction_id, sq->owner)
    && 2*faction_might(veh->faction_id) > faction_might(sq->owner)))
    && veh->triad() == TRIAD_LAND && map_range(veh->x, veh->y, tx, ty) > 2
    && has_terra(FORMER_RAISE_LAND, TRIAD_LAND, veh->faction_id)
    && (tbl[{tx, ty}].enemy_dist > 0 || (sq->is_base() && at_war(veh->faction_id, sq->owner)))
    && tbl[{tx, ty}].enemy_dist < 15) {
        debug("update_path %2d %2d -> %2d %2d %s\n", veh->x, veh->y, tx, ty, veh->name());
        int val = 0;
        int dist = 0;
        int px = veh->x;
        int py = veh->y;
        while (val >= 0 && ++dist <= PathLimit) {
            mapdata[{px, py}].unit_path++;
            if (px == tx && py == ty) {
                return;
            }
            val = Path_find(Paths, px, py, tx, ty, veh->unit_id, veh->faction_id, 0, -1);
            if (val >= 0) {
                px = wrap(px + BaseOffsetX[val]);
                py = py + BaseOffsetY[val];
            }
        }
    }
}

void TileSearch::reset() {
    type = 0;
    head = 0;
    tail = 0;
    y_skip = 0;
    faction_id = -1;
    oldtiles.clear();
}

void TileSearch::add_start(int x, int y) {
    assert(type >= 0 && type <= MaxTileSearchType);
    if (tail < QueueSize/2 && (sq = mapsq(x, y)) && !oldtiles.count({x, y})) {
        paths[tail] = {x, y, 0, -1};
        oldtiles.insert({x, y});
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

void TileSearch::adjust_roads(PMTable& tbl, int value) {
    int i = 0;
    int j = current;
    while (j >= 0 && ++i < PathLimit) {
        auto& p = paths[j];
        j = p.prev;
        tbl[{p.x, p.y}].roads += value;
    }
}

void TileSearch::connect_roads(PMTable& tbl, int x, int y, int pact_id) {
    if (!is_ocean(mapsq(x, y)) && tbl[{x, y}].roads < 1) {
        MAP* cur;
        init(x, y, TRIAD_LAND, 1);
        while ((cur = get_next()) != NULL && dist < 10) {
            if (cur->is_base() && cur->owner == pact_id) {
                adjust_roads(tbl, 1);
                return;
            }
        }
    }
}

/*
Traverse current search path and check for zones of control.
*/
bool TileSearch::has_zoc(int pact_id) {
    bool prev_zoc = false;
    int i = 0;
    int j = current;
    while (j >= 0 && ++i < PathLimit) {
        auto& p = paths[j];
        j = p.prev;
        if (mod_zoc_any(p.x, p.y, pact_id)) {
            if (prev_zoc) return true;
            prev_zoc = true;
        } else {
            prev_zoc = false;
        }
    }
    return false;
}

PathNode& TileSearch::get_prev() {
    return paths[paths[current].prev];
}

PathNode& TileSearch::get_node() {
    return paths[current];
}

/*
Implement a breadth-first search of adjacent map tiles to iterate possible movement paths.
Pathnodes are also used to keep track of the route to reach the current destination.
*/

MAP* TileSearch::get_next() {
    while (head < tail) {
        rx = paths[head].x;
        ry = paths[head].y;
        dist = paths[head].dist;
        current = head;
        head++;
        if (!(sq = mapsq(rx, ry))) {
            continue;
        }
        if (!current && faction_id < 0) {
            faction_id = sq->veh_owner();
            if (faction_id < 0) {
                faction_id = sq->owner;
            }
        }
        bool foreign = sq->is_owned() && sq->owner != faction_id && !has_pact(faction_id, sq->owner);
        bool our_base = sq->is_base() && !foreign;
        bool skip = (sq->is_base() && !our_base)
            || (type == TS_TRIAD_LAND && is_ocean(sq))
            || (type == TS_TRIAD_SEA && !is_ocean(sq) && !our_base)
            || (type == TS_SEA_AND_SHORE && !is_ocean(sq) && !our_base)
            || (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction_id))
            || (type == TS_TERRITORY_SHORE && (is_ocean(sq) || sq->owner != faction_id))
            || (type == TS_TERRITORY_PACT && (is_ocean(sq) || foreign));
        if (dist > 0 && skip) {
            if ((type == TS_TRIAD_LAND && is_ocean(sq))
            || (type == TS_TRIAD_SEA && !is_ocean(sq))
            || (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction_id))
            || (type == TS_TERRITORY_PACT && foreign)) {
                continue;
            }
            return sq;
        }
        for (const auto& p : NearbyTiles) {
            int x2 = wrap(rx + p[0]);
            int y2 = ry + p[1];
            if (y2 >= y_skip && y2 < *MapAreaY - y_skip
            && x2 >= 0 && x2 < *MapAreaX
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

/*
If zone of control exists on the tile, these functions return faction id + 1.
Otherwise return value is 0 (however seems to be treated as boolean).
*/
int __cdecl mod_zoc_any(int x, int y, int faction_id) {
    for (const auto& p : NearbyTiles) {
        int x2 = wrap(x + p[0]);
        int y2 = y + p[1];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            int owner = sq->anything_at();
            if (owner >= 0 && owner != faction_id && !has_pact(faction_id, owner)) {
                return owner + 1;
            }
        }
    }
    return 0;
}

int __cdecl mod_zoc_veh(int x, int y, int faction_id) {
    int value = 0;
    for (const auto& p : NearbyTiles) {
        int x2 = wrap(x + p[0]);
        int y2 = y + p[1];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            int owner = sq->veh_who();
            if (owner >= 0 && owner != faction_id && !has_pact(faction_id, owner)) {
                owner++;
                if (value <= owner) {
                    value = owner;
                }
            }
        }
    }
    return value;
}

int __cdecl mod_zoc_sea(int x, int y, int faction_id) {
    bool sea_tile = is_ocean(mapsq(x, y));
    for (const auto& p : NearbyTiles) {
        int x2 = wrap(x + p[0]);
        int y2 = y + p[1];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            int owner = sq->veh_who();
            if (owner >= 0 && owner != faction_id && is_ocean(sq) == sea_tile
            && !has_pact(faction_id, owner)) {
                for (int k = veh_at(x2, y2); k >= 0; k = Vehs[k].next_veh_id_stack) {
                    if (Vehs[k].faction_id != faction_id
                    && (Vehs[k].visibility & (1 << faction_id)
                    || (!*MultiplayerActive && !(Vehs[k].flags & VFLAG_INVISIBLE)))) {
                        return owner + 1;
                    }
                }
            }
        }
    }
    return 0;
}

int __cdecl mod_zoc_move(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    if (sq && !sq->is_base()) {
        return mod_zoc_sea(x, y, faction_id);
    }
    return 0;
}

std::vector<MapTile> iterate_tiles(int x, int y, size_t start_index, size_t end_index) {
    std::vector<MapTile> tiles;
    assert(start_index < end_index && end_index <= (size_t)TableRange[MaxTableRange]);

    for (size_t i = start_index; i < end_index; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            tiles.push_back({x2, y2, (int)i, sq});
        }
    }
    assert(tiles.size() > 0);
    return tiles;
}

int nearby_items(int x, int y, size_t start_index, size_t end_index, uint32_t item) {
    assert(start_index < end_index && end_index <= (size_t)TableRange[MaxTableRange]);
    int n = 0;
    for (size_t i = start_index; i < end_index; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (bit_at(x2, y2) & item) {
            n++;
        }
    }
    return n;
}

bool defend_tile(VEH* veh, MAP* sq) {
    if (!sq || (sq->items & BIT_MONOLITH && veh->need_monolith())) {
        return true;
    }
    if (sq->items & BIT_FUNGUS && veh->is_native_unit() && veh->need_heals()) {
        return true;
    }
    if (sq->items & (BIT_BASE_IN_TILE | BIT_BUNKER)) {
        if (veh->need_heals()) {
            return true;
        }
        if (!veh->is_combat_unit()) {
            int n = 0;
            for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
                if (mapdata[{m.x, m.y}].safety < PM_SAFE && ++n > 3) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool safe_path(TileSearch& ts, int faction_id, bool skip_owner) {
    MAP* sq;
    int i = 0;
    const PathNode* node = &ts.paths[ts.current];

    while (node->dist > 0 && ++i < PathLimit) {
        if (!(sq = mapsq(node->x, node->y)) || node->prev < 0) {
            assert(0);
            return false;
        }
        if (mapdata[{node->x, node->y}].safety < PM_SAFE) {
            debug_ver("path_unsafe %d %d %d\n", faction_id, node->x, node->y);
            return false;
        }
        if (!skip_owner && sq->is_owned()
        && sq->owner != faction_id && !has_pact(faction_id, sq->owner)) {
            debug_ver("path_foreign %d %d %d\n", faction_id, node->x, node->y);
            return false;
        }
        node = &ts.paths[node->prev];
    }
    return true;
}

bool has_base_sites(TileSearch& ts, int x, int y, int faction_id, int triad) {
    assert(valid_player(faction_id));
    assert(valid_triad(triad));
    MAP* sq;
    int area = 0;
    int bases = 0;
    int i = 0;
    int limit = ((*CurrentTurn * x ^ y) & 7 ? 200 : 800);
    ts.init(x, y, triad, 1);
    while (++i <= limit && (sq = ts.get_next()) != NULL) {
        if (sq->is_base()) {
            bases++;
        } else if (can_build_base(ts.rx, ts.ry, faction_id, triad)) {
            area++;
        }
    }
    debug("has_base_sites %2d %2d triad: %d area: %d bases: %d\n", x, y, triad, area, bases);
    return area > min(10, bases+4);
}

int route_distance(PMTable& tbl, int x1, int y1, int x2, int y2) {
    Points visited;
    std::list<PathNode> items;
    items.push_back({x1, y1, 0, 0});
    int limit = max(8, map_range(x1, y1, x2, y2) * 2);
    int i = 0;

    while (items.size() > 0 && ++i < PathLimit) {
        PathNode cur = items.front();
        items.pop_front();
        if (cur.x == x2 && cur.y == y2 && cur.dist <= limit) {
            debug_ver("route_distance %d %d -> %d %d = %d %d\n", x1, y1, x2, y2, i, cur.dist);
            return cur.dist;
        }
        for (const auto& t : NearbyTiles) {
            int rx = wrap(cur.x + t[0]);
            int ry = cur.y + t[1];
            if (mapsq(rx, ry) && tbl[{rx, ry}].roads > 0 && !visited.count({rx, ry})) {
                items.push_back({rx, ry, cur.dist + 1, 0});
                visited.insert({rx, ry});
            }
        }
    }
    return -1;
}

int cargo_capacity(int x, int y, int faction_id) {
    int num = 0;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y && veh->is_transport()
        && veh->faction_id == faction_id) {
            num += veh_cargo(i) - veh_cargo_loaded(i);
        }
    }
    return num;
}

int move_to_base(int veh_id, bool ally) {
    VEH* veh = &Vehs[veh_id];
    int tx;
    int ty;
    TileSearch ts;
    search_base(ts, veh_id, ally, &tx, &ty);
    if (tx >= 0) {
        debug("move_to_base %2d %2d -> %2d %2d %s\n",
            veh->x, veh->y, tx, ty, veh->name());
        return set_move_to(veh_id, tx, ty);
    }
    return mod_veh_skip(veh_id);
}

int escape_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (defend_tile(veh, sq)) {
        return set_order_none(id);
    }
    int tx = -1;
    int ty = -1;
    TileSearch ts;
    if (search_escape(ts, id, &tx, &ty)) {
        return set_move_to(id, tx, ty);
    }
    if (veh->is_combat_unit()) {
        return enemy_move(id);
    }
    return mod_veh_skip(id);
}

static int escape_score(int x, int y, int range, VEH* veh, MAP* sq) {
    return mapdata[{x, y}].safety - 160*mapdata[{x, y}].target - 120*range
        + (sq->items & BIT_MONOLITH && veh->need_monolith() ? 2000 : 0)
        + (sq->items & BIT_BUNKER ? 500 : 0)
        + (sq->items & BIT_FOREST ? 200 : 0)
        + (sq->is_rocky() ? 200 : 0)
        + (mapnodes.count({x, y, NODE_PATROL}) ? 400 : 0);
}

int search_escape(TileSearch& ts, int veh_id, int* tx, int* ty) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    *tx = -1;
    *ty = -1;
    if (!sq || veh_id < 0) {
        assert(0);
        return false;
    }
    int best_score = escape_score(veh->x, veh->y, 0, veh, sq);
    int score;
    *tx = -1;
    *ty = -1;
    ts.init(veh->x, veh->y, veh->triad());
    while ((sq = ts.get_next()) != NULL && ts.dist <= 8) {
        if (ts.dist > 2 && best_score > 500) {
            break;
        }
        if (!non_ally_in_tile(ts.rx, ts.ry, veh->faction_id)
        && (score = escape_score(ts.rx, ts.ry, ts.dist, veh, sq)) > best_score
        && !ts.has_zoc(veh->faction_id)) { // Always check for zocs just in case
            *tx = ts.rx;
            *ty = ts.ry;
            best_score = score;
            debug("escape_score %2d %2d -> %2d %2d dist: %d score: %d\n",
                veh->x, veh->y, ts.rx, ts.ry, ts.dist, score);
        }
    }
    return *tx >= 0;
}

int search_base(TileSearch& ts, int veh_id, bool ally, int* tx, int* ty) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    *tx = -1;
    *ty = -1;
    if (!sq || veh_id < 0) {
        assert(0);
        return 0;
    }
    if (sq->is_base() && (sq->owner == veh->faction_id || ally)) {
        return 0;
    }
    int type = (veh->triad() == TRIAD_SEA ? TS_SEA_AND_SHORE : veh->triad());
    int best_score = escape_score(veh->x, veh->y, 0, veh, sq);
    int score;
    ts.init(veh->x, veh->y, type, 1);
    while ((sq = ts.get_next()) != NULL && ts.dist <= 20) {
        if (sq->is_base() && (sq->owner == veh->faction_id
        || (ally && has_pact(veh->faction_id, sq->owner)))) {
            *tx = ts.rx;
            *ty = ts.ry;
            if (veh->triad() == TRIAD_AIR || random(2)) {
                break;
            }
        } else if (*tx < 0 && ts.dist < 5
        && allow_move(ts.rx, ts.ry, veh->faction_id, veh->triad())
        && (score = escape_score(ts.rx, ts.ry, ts.dist, veh, sq)) > best_score
        && !ts.has_zoc(veh->faction_id)) {
            *tx = ts.rx;
            *ty = ts.ry;
            best_score = score;
        }
    }
    return *tx >= 0;
}

static int route_score(VEH* veh, int x, int y, int modifier, MAP* sq) {
    AIPlans& plan = plans[veh->faction_id];
    bool sea = is_ocean(sq);
    int score = (sea ? 0 : min(16, Continents[sq->region].tile_count/32))
        + 32*(sq->region == plan.main_region)
        + 16*(plan.main_region != plan.target_land_region || sq->region != plan.target_land_region)
        - modifier * (sea ? 2 : 1) * map_range(veh->x, veh->y, x, y)
        - 4*mapdata[{x, y}].target;
    if (veh->is_artifact() && !mapnodes.count({x, y, NODE_NAVAL_START})) {
        score += 64*can_link_artifact(base_at(x, y));
    }
    if (veh->is_combat_unit() && !is_ocean(sq)) {
        score += 2*max(0, Continents[sq->region].pods - Continents[sq->region].tile_count/32);
    }
    return score;
}

int search_route(TileSearch& ts, int veh_id, int* tx, int* ty) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    AIPlans& plan = plans[veh->faction_id];
    debug("search_route %2d %2d %s\n", veh->x, veh->y, veh->name());
    *tx = -1;
    *ty = -1;
    if (!sq || veh_id < 0) {
        assert(0);
        return false;
    }
    int veh_reg = sq->region;
    bool combat = veh->is_combat_unit();
    bool scout = combat && !bad_reg(veh_reg)
        && Continents[veh_reg].pods > Continents[veh_reg].tile_count/32;
    bool at_base = sq->is_base() && sq->owner == veh->faction_id;
    bool same_reg = false;
    bool has_gate = false;
    int best_score = INT_MIN;
    int px = -1;
    int py = -1;

    if (veh->triad() == TRIAD_AIR) {
        *tx = plan.main_region_x;
        *ty = plan.main_region_y;
        return *tx >= 0 && (*tx != veh->x || *ty != veh->y);
    }
    if (veh->triad() == TRIAD_SEA) {
        if (!veh->is_transport()) {
            return false;
        }
        ts.init(veh->x, veh->y, TS_SEA_AND_SHORE);
        while ((sq = ts.get_next()) != NULL) {
            if (sq->is_base() && sq->owner == veh->faction_id) {
                if (ts.dist < 8 && !safe_path(ts, veh->faction_id, true)) {
                    continue;
                }
                if (map_range(ts.rx, ts.ry, plan.naval_end_x, plan.naval_end_y) < 4) {
                    continue;
                }
                int score = route_score(veh, ts.rx, ts.ry, 1, sq);
                if (score > best_score) {
                    *tx = ts.rx;
                    *ty = ts.ry;
                    best_score = score;
                }
                if (*tx >= 0 && ts.dist >= 25) {
                    break;
                }
            }
        }
        return *tx >= 0;
    }
    if (!combat && !veh->in_transit()
    && mapdata[{veh->x, veh->y}].safety < PM_SAFE
    && search_escape(ts, veh_id, tx, ty)) {
        return true;
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == veh->faction_id && (sq = mapsq(base->x, base->y))) {
            int score = route_score(veh, base->x, base->y, 4, sq);
            if (score > best_score) {
                px = base->x;
                py = base->y;
                best_score = score;
            }
            if (veh->x == base->x && veh->y == base->y && can_use_teleport(i)) {
                has_gate = true;
            }
        }
    }
    ts.init(veh->x, veh->y, TS_TERRITORY_PACT);
    best_score = INT_MIN;
    if (at_base && veh->is_artifact()) {
        best_score = route_score(veh, veh->x, veh->y, 1, sq);
    }
    while ((sq = ts.get_next()) != NULL) {
        if (ts.dist == 1 && (!combat || !scout)
        && mapnodes.count({ts.rx, ts.ry, NODE_NAVAL_PICK})
        && allow_move(ts.rx, ts.ry, veh->faction_id, TRIAD_SEA)) {
            if (cargo_capacity(ts.rx, ts.ry, veh->faction_id) > 0) {
                debug("search_route load %2d %2d\n", ts.rx, ts.ry);
                *tx = ts.rx;
                *ty = ts.ry;
                return true;
            }
            if (!veh->iter_count || random(4)) {
                debug("search_route skip %2d %2d\n", ts.rx, ts.ry);
                return false;
            }
        }
        if (sq->is_base() && sq->owner == veh->faction_id) {
            if (!combat && !safe_path(ts, veh->faction_id, true)) {
                continue;
            }
            int score = route_score(veh, ts.rx, ts.ry, 1, sq);
            if (score > best_score) {
                px = ts.rx;
                py = ts.ry;
                best_score = score;
                same_reg = sq->region == veh_reg
                    && ts.dist < 8 + 2*map_range(veh->x, veh->y, ts.rx, ts.ry);
            }
            if (px >= 0 && ts.dist >= 25) {
                break;
            }
        }
    }
    if (px >= 0 && has_gate) {
        int tgt_id = -1;
        best_score = -20;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == veh->faction_id && has_fac_built(FAC_PSI_GATE, i)
            && region_at(base->x, base->y) == region_at(px, py)) {
                int score = base->pop_size - map_range(px, py, base->x, base->y);
                if (score > best_score) {
                    tgt_id = i;
                    best_score = score;
                }
            }
        }
        if (tgt_id >= 0) {
            debug("action_gate %2d %2d %s -> %s\n",
            veh->x, veh->y, veh->name(), Bases[tgt_id].name);
            action_gate(veh_id, tgt_id);
            *tx = px;
            *ty = py;
            return true;
        }
    }
    if (px >= 0 && !same_reg && (!at_base || !veh->is_artifact())) {
        PointList start;
        start.push_front({px, py});
        ts.init(px, py, TS_TERRITORY_PACT);
        while ((sq = ts.get_next()) != NULL) {
            if (is_ocean(sq)) {
                start.push_front({ts.rx, ts.ry});
            }
            if (veh->x == ts.rx && veh->y == ts.ry
            && ts.dist < 8 + 2*map_range(veh->x, veh->y, px, py)) {
                *tx = px;
                *ty = py;
                debug("search_route %2d %2d\n", *tx, *ty);
                return true;
            }
            if (ts.dist >= 16 && start.size() >= 32) {
                break;
            }
        }
        int best_route = 0;
        best_score = INT_MIN;
        ts.init(start, TS_SEA_AND_SHORE, 1);

        while ((sq = ts.get_next()) != NULL) {
            assert(ts.current > ts.paths[ts.current].prev);
            PathNode& p = ts.get_prev();
            MAP* psq = mapsq(p.x, p.y);
            if (sq->region == veh_reg && ts.dist > 3
            && !sq->is_base() && psq && !psq->is_base()
            && allow_civ_move(ts.rx, ts.ry, veh->faction_id, TRIAD_LAND)
            && allow_civ_move(p.x, p.y, veh->faction_id, TRIAD_SEA)) {
                int dist = map_range(veh->x, veh->y, ts.rx, ts.ry);
                int score = 16*mapnodes.count({p.x, p.y, NODE_NAVAL_PICK})
                    + 8*(sq->is_fungus() == veh->is_native_unit())
                    + min(0, mapdata[{ts.rx, ts.ry}].safety/32)
                    - ts.dist - 2*dist;
                if (score > best_score) {
                    debug("search_route %2d %2d dist: %d %d score: %d\n",
                    ts.rx, ts.ry, ts.dist, dist, score);
                    best_route = ts.current;
                    best_score = score;
                }
            }
        }
        if (best_route > 0) {
            PathNode& node = ts.paths[best_route];
            PathNode& prev = ts.paths[node.prev];
            mapnodes.insert({prev.x, prev.y, NODE_NAVAL_PICK}); // Persistent
            mapnodes.insert({prev.x, prev.y, NODE_NEED_FERRY}); // Movement priority
            add_goal(veh->faction_id, AI_GOAL_NAVAL_PICK, 3, prev.x, prev.y, -1);
            if (cargo_capacity(prev.x, prev.y, veh->faction_id) > 0) {
                *tx = prev.x;
                *ty = prev.y;
                debug("search_route load %2d %2d\n", *tx, *ty);
                return true;
            }
            *tx = node.x;
            *ty = node.y;
            debug("search_route wait %2d %2d\n", *tx, *ty);
            return true;
        }
    }
    if (same_reg) {
        *tx = px;
        *ty = py;
        debug("search_route move %2d %2d\n", *tx, *ty);
    }
    return *tx >= 0;
}


