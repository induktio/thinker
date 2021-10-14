
#include "move.h"

const int IMP_SIMPLE = (BIT_FARM | BIT_MINE | BIT_FOREST);
const int IMP_ADVANCED = (BIT_CONDENSER | BIT_THERMAL_BORE);
const int PM_SAFE = -20;
const int PM_NEAR_SAFE = -40;
const int ShoreLine = 256;
const int ShoreTilesMask = 255;

/*
Priority Map Tables contain values calculated for each map square
to guide the AI in its move planning.

The tables contain only ephemeral values: they are recalculated each turn
for every active faction, and the values are never saved with the save games.

For example, pm_former counter is decremented whenever a former starts
work on a given square to prevent too many of them converging on the same spot.
It is also used as a shortcut to prioritize improving tiles that are reachable
by the most friendly base workers, usually tiles in between bases.

Non-combat units controlled by Thinker (formers, colony pods, land-based crawlers)
use pm_safety table to decide if there are enemy units too close to them.
PM_SAFE defines the threshold below which the units will attempt to flee
to the square chosen by escape_move.
*/

PMTable pm_former;
PMTable pm_safety;
PMTable pm_roads;
PMTable pm_shore;
PMTable pm_unit_near;
PMTable pm_enemy;
PMTable pm_enemy_near;
PMTable pm_enemy_dist;
PMTable pm_overlay;

int last_faction = 0;
int build_tubes = false;
int defend_goal[MaxBaseNum] = {};
int region_flags[MaxRegionNum] = {};

int arty_value(int x, int y);
int garrison_goal(int x, int y, int faction, int triad);
int base_tile_score(int x1, int y1, int range, int triad);
bool invasion_unit(int id);
bool allow_probe(int faction1, int faction2);


void adjust_value(PMTable tbl, int x, int y, int range, int value) {
    for (const auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        tbl[m.x][m.y] += value;
    }
}

bool non_ally_in_tile(int x, int y, int faction) {
    return faction == last_faction && mapnodes.count({x, y, NODE_NONALLY}) > 0;
}

// Any sea region or inland lakes
int coast_tiles(int x, int y) {
    return pm_shore[x][y] & ShoreTilesMask;
}

// Ocean refers only to main_sea_region
int ocean_coast_tiles(int x, int y) {
    return pm_shore[x][y] / ShoreLine;
}

bool near_ocean_coast(int x, int y) {
    if (pm_shore[x][y] >= ShoreLine) {
        return false;
    }
    for (const auto& m : iterate_tiles(x, y, 1, 9)) {
        if (pm_shore[m.x][m.y] >= ShoreLine) {
            return true;
        }
    }
    return false;
}

int __cdecl mod_enemy_move(const int id) {
    assert(id >= 0 && id < *total_num_vehicles);
    VEH* veh = &Vehicles[id];
    debug("enemy_move %2d %2d %s\n", veh->x, veh->y, veh->name());

    if (ai_enabled(veh->faction_id)) {
        int triad = veh->triad();
        if (!mapsq(veh->x, veh->y)) {
            return SYNC;
        } else if (veh->is_colony()) {
            return colony_move(id);
        } else if (veh->is_supply()) {
            return crawler_move(id);
        } else if (veh->is_former()) {
            return former_move(id);
        } else if (triad == TRIAD_SEA && cargo_capacity(id) > 0) {
            return trans_move(id);
        } else if (triad == TRIAD_AIR && veh->is_combat_unit()) {
            return aircraft_move(id);
        } else if (triad != TRIAD_AIR && (veh->is_combat_unit() || veh->is_probe())) {
            return combat_move(id);
        }
    }
    return enemy_move(id);
}

int __cdecl log_veh_kill(int UNUSED(ptr), int id, int UNUSED(owner), int UNUSED(unit_id)) {
    /* This logging function is called whenever a vehicle is removed/killed for any reason. */
    VEH* veh = &Vehicles[id];
    if (ai_enabled(*active_faction) && at_war(*active_faction, veh->faction_id)) {
        if (veh->x >= 0 && veh->y >= 0) {
            adjust_value(pm_enemy_near, veh->x, veh->y, 2, -1);
            pm_enemy[veh->x][veh->y]--;
        }
    }
    return 0;
}

void adjust_route(PMTable tbl, const TileSearch& ts, int value) {
    int i = 0;
    int j = ts.current;
    while (j >= 0 && ++i < PathLimit) {
        auto& p = ts.paths[j];
        j = p.prev;
        tbl[p.x][p.y] += value;
    }
}

int route_distance(PMTable tbl, int x1, int y1, int x2, int y2) {
    Points visited;
    std::list<PathNode> items;
    items.push_back({x1, y1, 0, 0});
    int limit = max(8, map_range(x1, y1, x2, y2) * 2);
    int i = 0;

    while (items.size() > 0 && ++i < 120) {
        PathNode cur = items.front();
        items.pop_front();
        if (cur.x == x2 && cur.y == y2 && cur.dist <= limit) {
//            debug("route_distance %d %d -> %d %d = %d %d\n", x1, y1, x2, y2, i, cur.dist);
            return cur.dist;
        }
        for (const int* t : NearbyTiles) {
            int rx = wrap(cur.x + t[0]);
            int ry = cur.y + t[1];
            if (mapsq(rx, ry) && tbl[rx][ry] > 0 && !visited.count({rx, ry})) {
                items.push_back({rx, ry, cur.dist + 1, 0});
                visited.insert({rx, ry});
            }
        }
    }
    return -1;
}

/*
Evaluate possible land bridge routes from home continent to other regions.
Planned routes are stored in the faction goal struct and persist in save games.
*/
void land_raise_plan(int faction, bool visual) {
    AIPlans& p = plans[faction];
    if (visual) {
        update_main_region(faction);
    }
    if (p.main_region < 0) {
        return;
    }
    bool expand = allow_expand(faction);
    int best_score = 20;
    int goal_count = 0;
    int i = 0;
    PointList path;
    TileSearch ts;
    MAP* sq;

    ts.init(p.main_region_x, p.main_region_y, TS_TERRITORY_LAND, 4);
    while (++i < 1000 && (sq = ts.get_next()) != NULL) {
        assert(!is_ocean(sq));
        assert(sq->owner == faction);
        if (pm_shore[ts.rx][ts.ry] > 0 && !sq->is_base()) {
            path.push_back({ts.rx, ts.ry});
        }
    }
    debug("raise_plan %d region: %d tiles: %d\n",
        faction, p.main_region, Continents[p.main_region].tile_count);

    i = 0;
    ts.init(path, TS_SEA_AND_SHORE, 4);
    while (++i < 2400 && (sq = ts.get_next()) != NULL && ts.dist <= 12) {
        if (sq->region == p.main_region || !sq->is_land_region() || (!sq->is_owned() && !expand)) {
            continue;
        }
        // Check if we should bridge to hostile territory
        if (sq->owner != faction && sq->is_owned() && !has_pact(faction, sq->owner)
        && faction_might(faction) < faction_might(sq->owner)) {
            continue;
        }
        ts.get_route(path);
        int score = min(100, Continents[sq->region].tile_count) - ts.dist*ts.dist/2;

        if (score > best_score) {
            best_score = score;
            debug("raise_goal %2d %2d -> %2d %2d dist: %2d size: %3d owner: %d score: %d\n",
                path.begin()->x, path.begin()->y, ts.rx, ts.ry, ts.dist,
                Continents[sq->region].tile_count, sq->owner, score);

            for (auto& pp : path) {
                if (visual) {
                    pm_overlay[pp.x][pp.y] += score;
                } else {
                    add_goal(faction, AI_GOAL_RAISE_LAND, 3, pp.x, pp.y, -1);
                }
                goal_count++;
            }
        }
        if (goal_count > 15) {
            return;
        }
    }
}

int target_priority(int x, int y, int faction, MAP* sq) {
    Faction& f = Factions[faction];
    AIPlans& p1 = plans[faction];
    AIPlans& p2 = plans[sq->owner];
    int score = 0;

    if (sq->is_owned()) {
        if (sq->region == p2.main_region) {
            score += 150;
        }
        if (p1.main_region == p2.main_region) {
            score += 200;
        }
        if (*game_rules & RULES_INTENSE_RIVALRY && is_human(sq->owner)) {
            score += 300;
        }
        if (f.base_id_attack_target >= 0
        && Bases[f.base_id_attack_target].x == x
        && Bases[f.base_id_attack_target].y == y) {
            score += 400;
        }
        if (sq->is_base()) {
            return score;
        }
    }
    score += (sq->items & BIT_ROAD ? 60 : 0)
        + (sq->items & BIT_FUNGUS && p1.keep_fungus ? 60 : 0)
        + (sq->items & BIT_SENSOR && at_war(faction, sq->owner) ? 90 : 0)
        + (sq->is_rocky() || sq->items & (BIT_BUNKER | BIT_FOREST) ? 50 : 0);

    return score;
}

/*
Pick a random faction and send some extra scout ships to their territory.
*/
int pick_scout_target(int faction) {
    AIPlans& p = plans[faction];
    int target = 0;
    int best_score = 15;
    if (p.enemy_mil_factor > 2 || p.enemy_factions > 1) {
        return 0;
    }
    for (int i=1; i < MaxPlayerNum; i++) {
        if (i != faction && Factions[i].current_num_bases) {
            int score = random(16)
                + (has_treaty(faction, i, DIPLO_PACT|DIPLO_TREATY) ? -10 : 0)
                + Factions[faction].diplo_friction[i]
                + 8*Factions[i].diplo_stolen_techs[faction]
                + 5*Factions[i].integrity_blemishes
                + 5*Factions[i].atrocities
                + min(16, 4*faction_might(faction) / max(1, faction_might(i)));
            if (score > best_score) {
                target = i;
                best_score = score;
            }
        }
    }
    return target;
}

/*
Evaluate possible naval invasion routes to other continents.
*/
void invasion_plan(int faction) {
    AIPlans& p = plans[faction];
    MAP* sq;
    TileSearch ts;
    PointList path;
    PointList enemy_bases;

    if (p.main_region < 0) {
        return;
    }
    int scout_target = 0;
    int px = -1;
    int py = -1;
    find_priority_goal(faction, AI_GOAL_NAVAL_END, &px, &py);
    find_priority_goal(faction, AI_GOAL_NAVAL_SCOUT, &p.naval_scout_x, &p.naval_scout_y);

    for (int i=0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (!(sq = mapsq(base->x, base->y))) {
            continue;
        }
        if (sq->region == p.main_region && base->faction_id == faction
        && ocean_coast_tiles(base->x, base->y)) {
            path.push_back({base->x, base->y});

        } else if (at_war(faction, base->faction_id) && !is_ocean(sq)) {
            enemy_bases.push_back({base->x, base->y});
        }
    }
    memset(pm_enemy_dist, 0, sizeof(pm_enemy_dist));
    int i = 0;
    ts.init(enemy_bases, TS_TRIAD_LAND, 1);
    while (++i < QueueSize && (sq = ts.get_next()) != NULL) {
        pm_enemy_dist[ts.rx][ts.ry] = ts.dist;
    }
    if (p.naval_scout_x < 0) {
        scout_target = pick_scout_target(faction);
    }
    if (!has_chassis(faction, CHS_FOIL) && !has_chassis(faction, CHS_CRUISER)) {
        return;
    }
    if (!p.enemy_factions && !scout_target) {
        return;
    }
    ts.init(path, TS_SEA_AND_SHORE, 0);
    i = 0;
    while (++i < QueueSize && (sq = ts.get_next()) != NULL) {
        PathNode& prev = ts.paths[ts.paths[ts.current].prev];
        if (sq->is_land_region()
        && pm_enemy_dist[ts.rx][ts.ry] > 0
        && pm_enemy_dist[ts.rx][ts.ry] < 10
        && ocean_coast_tiles(ts.rx, ts.ry)
        && allow_move(prev.x, prev.y, faction, TRIAD_SEA)
        && allow_move(ts.rx, ts.ry, faction, TRIAD_LAND)) {
            ts.get_route(path);

            if (sq->region == p.main_region
            && (ts.dist < 8 || sq->owner == faction
            || map_range(ts.rx, ts.ry, path.begin()->x, path.begin()->y) + 1 < ts.dist)) {
                continue;
            }
            int score = min(0, pm_safety[ts.rx][ts.ry] / 2)
                + target_priority(ts.rx, ts.ry, faction, sq)
                + (ocean_coast_tiles(prev.x, prev.y) < 7 ? 100 : 0)
                + (sq->region == p.main_region ? 0 : 300)
                - 30*pm_enemy_dist[ts.rx][ts.ry]
                - 16*ts.dist + random(32);
            if (px >= 0) {
                score -= 4*map_range(px, py, prev.x, prev.y);
            }
            if (score > p.naval_score) {
                p.target_land_region = sq->region;
                p.naval_score = score;
                p.naval_start_x = path.begin()->x;
                p.naval_start_y = path.begin()->y;
                p.naval_end_x = prev.x;
                p.naval_end_y = prev.y;
                p.naval_beach_x = ts.rx;
                p.naval_beach_y = ts.ry;

                debug("invasion %d -> %d start: %2d %2d end: %2d %2d ocean: %d dist: %2d score: %d\n",
                    faction, sq->owner, path.begin()->x, path.begin()->y, ts.rx, ts.ry,
                    ocean_coast_tiles(prev.x, prev.y), ts.dist, score);
            }
        }
        if (p.naval_scout_x < 0 && sq->is_land_region() && sq->owner == scout_target
        && sq->region == plans[scout_target].main_region && sq->is_base_radius()) {
            sq = mapsq(prev.x, prev.y);
            if (sq && sq->is_base_radius() && sq->owner == scout_target && !random(8)) {
                p.naval_scout_x = prev.x;
                p.naval_scout_y = prev.y;
                add_goal(faction, AI_GOAL_NAVAL_SCOUT, 5, prev.x, prev.y, -1);
            }
        }
    }
    if (p.naval_end_x >= 0) {
        int min_dist = 25;
        for (i=0; i < *total_num_bases; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction) {
                int dist = map_range(base->x, base->y, p.naval_end_x, p.naval_end_y)
                    - (has_facility(i, FAC_AEROSPACE_COMPLEX) ? 5 : 0) + random(4);
                if (dist < min_dist) {
                    p.naval_airbase_x = base->x;
                    p.naval_airbase_y = base->y;
                    min_dist = dist;
                }
            }
        }
    }
}

/*
For every faction, the continent with the oldest (lowest ID) land base
is the main region. This base is usually but not always the headquarters.
*/
void update_main_region(int faction) {
    for (int i=1; i < MaxPlayerNum; i++) {
        AIPlans& p = plans[i];
        p.main_region = -1;
        p.main_region_x = -1;
        p.main_region_y = -1;
        if (i == faction) {
            p.prioritize_naval = 0;
            p.main_sea_region = 0;
            p.target_land_region = 0;
            p.naval_score = -1000;
            p.naval_scout_x = -1;
            p.naval_scout_y = -1;
            p.naval_airbase_x = -1;
            p.naval_airbase_y = -1;
            p.naval_start_x = -1;
            p.naval_start_y = -1;
            p.naval_end_x = -1;
            p.naval_end_y = -1;
            p.naval_beach_x = -1;
            p.naval_beach_y = -1;
            p.land_combat_units = 0;
            p.air_combat_units = 0;
            p.transport_units = 0;
        }
    }

    for (int i=0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        MAP* sq = mapsq(base->x, base->y);
        AIPlans& p = plans[base->faction_id];
        if (p.main_region < 0 && !is_ocean(sq)) {
            p.main_region = sq->region;
            p.main_region_x = base->x;
            p.main_region_y = base->y;
        }
    }
    AIPlans& p = plans[faction];
    if (p.main_region < 0) {
        return;
    }
    // Prioritize naval invasions if the closest enemy is on another region
    MAP* sq;
    TileSearch ts;
    ts.init(p.main_region_x, p.main_region_y, TS_TERRITORY_BORDERS, 2);
    int i = 0;
    int k = 0;
    while (++i < 800 && (sq = ts.get_next()) != NULL) {
        if (at_war(faction, sq->owner) && !is_ocean(sq) && ++k >= 10) {
            p.prioritize_naval = 0;
            return;
        }
    }
    int min_dist = INT_MAX;
    for (i=1; i < MaxPlayerNum; i++) {
        int dist = map_range(p.main_region_x, p.main_region_y,
            plans[i].main_region_x, plans[i].main_region_y);
        if (at_war(faction, i) && dist < min_dist) {
            min_dist = dist;
            p.prioritize_naval = p.main_region != plans[i].main_region;
        }
    }
}

void move_upkeep(int faction, bool visual) {
    Faction& f = Factions[faction];
    AIPlans& p = plans[faction];
    if (!ai_enabled(faction)) {
        return;
    }
    int tile_count[MaxRegionNum] = {};
    last_faction = faction;
    build_tubes = has_terra(faction, FORMER_MAGTUBE, LAND);
    p.land_combat_units = 0;
    p.sea_combat_units = 0;
    p.air_combat_units = 0;
    p.transport_units = 0;
    mapnodes.clear();
    memset(pm_former, 0, sizeof(pm_former));
    memset(pm_safety, 0, sizeof(pm_safety));
    memset(pm_roads, 0, sizeof(pm_roads));
    memset(pm_shore, 0, sizeof(pm_shore));
    memset(pm_enemy, 0, sizeof(pm_enemy));
    memset(pm_unit_near, 0, sizeof(pm_unit_near));
    memset(pm_enemy_near, 0, sizeof(pm_enemy_near));
    memset(region_flags, 0, sizeof(region_flags));

    debug("move_upkeep %d region: %d x: %2d y: %2d naval: %d\n",
        faction, p.main_region, p.main_region_x, p.main_region_y, p.prioritize_naval);

    MAP* sq;
    for (int y = 0; y < *map_axis_y; y++) {
        for (int x = y&1; x < *map_axis_x; x+=2) {
            if (!(sq = mapsq(x, y))) {
                continue;
            }
            if (sq->region < MaxRegionNum) {
                tile_count[sq->region]++;
                if (is_ocean(sq)) {
                    adjust_value(pm_shore, x, y, 1, 1);
                    pm_shore[x][y]--;
                    if (tile_count[sq->region] > tile_count[p.main_sea_region]) {
                        p.main_sea_region = sq->region;
                    }
                }
                // Note linear tile iteration order
                assert(pm_shore[x][y] <= 5);
            }
            if (sq->owner == faction && ((!build_tubes && sq->items & BIT_ROAD)
            || (build_tubes && sq->items & BIT_MAGTUBE))) {
                pm_roads[x][y]++;
            } else if (goody_at(x, y)) {
                mapnodes.insert({x, y, NODE_PATROL});
            }
        }
    }
    for (int y = 0; y < *map_axis_y; y++) {
        for (int x = y&1; x < *map_axis_x; x+=2) {
            if ((sq = mapsq(x, y)) && sq->region == p.main_sea_region) {
                adjust_value(pm_shore, x, y, 1, ShoreLine);
            }
            assert(goody_at(x, y) == mod_goody_at(x, y));
            assert(bonus_at(x, y) == mod_bonus_at(x, y));
        }
    }
    for (int i=0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        int triad = veh->triad();

        if (!(sq = mapsq(veh->x, veh->y))) {
            continue;
        }
        if (veh->move_status == ORDER_THERMAL_BOREHOLE) {
            mapnodes.insert({veh->x, veh->y, NODE_BOREHOLE});
        }
        if (veh->move_status == ORDER_TERRAFORM_UP) {
            mapnodes.insert({veh->x, veh->y, NODE_RAISE_LAND});
        }
        if (veh->move_status == ORDER_SENSOR_ARRAY) {
            mapnodes.insert({veh->x, veh->y, NODE_SENSOR_ARRAY});
        }
        if (veh->faction_id == faction) {
            if (veh->is_combat_unit()) {
                adjust_value(pm_safety, veh->x, veh->y, 1, 40);
                pm_safety[veh->x][veh->y] += 60;
            }
            if (veh->is_combat_unit() || (veh->is_probe() && veh->is_armored())) {
                if (triad == TRIAD_AIR) {
                    p.air_combat_units++;
                } else if (triad == TRIAD_SEA) {
                    p.sea_combat_units++;
                } else {
                    p.land_combat_units++;
                }
            }
            if (veh->is_transport() && triad == TRIAD_SEA) {
                p.transport_units++;
            }
            if (triad == TRIAD_LAND && !is_ocean(sq)) {
                veh->state &= ~VSTATE_IN_TRANSPORT;
            }
            if ((veh->is_colony() || veh->is_former()) && triad == TRIAD_LAND) {
                if (is_ocean(sq) && sq->is_base() && coast_tiles(veh->x, veh->y) < 8) {
                    mapnodes.insert({veh->x, veh->y, NODE_NEED_FERRY});
                }
            }
            if (veh->waypoint_1_x >= 0) {
                mapnodes.erase({veh->waypoint_1_x, veh->waypoint_1_y, NODE_PATROL});
            }
            adjust_value(pm_unit_near, veh->x, veh->y, 3, (triad == TRIAD_LAND ? 2 : 1));

        } else if (at_war(faction, veh->faction_id)) {
            int value = (veh->is_combat_unit() ? -100 : (veh->is_probe() ? -24 : -12));
            int range = (veh->speed() > 1 ? 3 : 2) + (triad == TRIAD_AIR ? 2 : 0);
            if (value == -100) {
                adjust_value(pm_safety, veh->x, veh->y, range, value/(range > 2 ? 4 : 8));
            }
            pm_enemy[veh->x][veh->y]++;
            adjust_value(pm_safety, veh->x, veh->y, 1, value);
            if (veh->unit_id != BSC_FUNGAL_TOWER) {
                adjust_value(pm_enemy_near, veh->x, veh->y, 2, 1);
            }
        }
        if (veh->faction_id != faction &&
        (at_war(faction, veh->faction_id) || !has_pact(faction, veh->faction_id))) {
            if (veh->is_artifact()) {
                mapnodes.insert({veh->x, veh->y, NODE_ARTIFACT});
            } else {
                mapnodes.insert({veh->x, veh->y, NODE_NONALLY});
            }
        }
    }
    for (const MapNode& m : mapnodes) {
        if (m.type == NODE_ARTIFACT && !mapnodes.count({m.x, m.y, NODE_NONALLY})) {
            mapnodes.insert({m.x, m.y, NODE_COMBAT_PATROL});
            debug("artifact %2d %2d\n", m.x, m.y);
        }
    }
    TileSearch ts;
    for (int i=0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (!(sq = mapsq(base->x, base->y))) {
            continue;
        }
        if (base->faction_id == faction) {
            adjust_value(pm_former, base->x, base->y, 2, base->pop_size);
            pm_safety[base->x][base->y] += 10000;

            if (unit_in_tile(sq) < 0) { // Undefended base
                mapnodes.insert({base->x, base->y, NODE_PATROL});
            }
            if (sq->region < MaxRegionLandNum) {
                int j = 0;
                int k = 0;
                ts.init(base->x, base->y, TS_TERRITORY_LAND);
                while (++j < 150 && k < 5 && (sq = ts.get_next()) != NULL) {
                    if (sq->is_base()) {
                        if (route_distance(pm_roads, base->x, base->y, ts.rx, ts.ry) < 0) {
//                            debug("route_add %d %d -> %d %d\n", base->x, base->y, ts.rx, ts.ry);
                            adjust_route(pm_roads, ts, 1);
                        }
                        k++;
                    }
                }
            }
        } else {
            if (at_war(faction, base->faction_id)) {
                region_flags[sq->region] |= PM_ENEMY;
            }
            if (allow_probe(faction, base->faction_id)) {
                region_flags[sq->region] |= PM_PROBE;
            }
        }
    }
    for (int i=0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        pm_former[base->x][base->y] = 0; // Special counter
    }
    if (f.current_num_bases > 0) {
        memset(defend_goal, 1, sizeof(defend_goal));
        int values[MaxBaseNum] = {};
        int sorter[MaxBaseNum] = {};
        int bases = 0;

        for (int i=0; i < *total_num_bases; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction && (sq = mapsq(base->x, base->y))) {
                values[i] = INT_MIN + base->pop_size;
                int region = sq->region;
                for (int k=0; k < *total_num_bases; k++) {
                    BASE* b = &Bases[k];
                    if (faction != b->faction_id
                    && !has_pact(faction, b->faction_id)
                    && (sq = mapsq(b->x, b->y))) {
                        int value = base->pop_size
                            + (has_facility(i, FAC_HEADQUARTERS) ? 10 : 0)
                            + 10 * pm_enemy_near[base->x][base->y]
                            - (sq->region == region && sq->region < MaxRegionLandNum ? 1 : 2)
                            * (at_war(faction, b->faction_id) ? 1 : 3)
                            * map_range(base->x, base->y, b->x, b->y);
                        if (value > values[i]) {
                            values[i] = value;
                        }
                    }
                }
                sorter[bases] = values[i];
                bases++;
                debug("defend_score %4d %s\n", values[i], base->name);
            }
        }
        std::sort(sorter, sorter+bases);
        int k1 = sorter[bases*1/2];
        int k2 = sorter[bases*3/4];
        int k3 = sorter[bases*7/8];
        p.defend_weights = 0;

        for (int i=0; i < *total_num_bases; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction) {
                if (values[i] >= k3) {
                    defend_goal[i] = 4;
                } else if (values[i] >= k2) {
                    defend_goal[i] = 3;
                } else if (values[i] >= k1) {
                    defend_goal[i] = 2;
                } else {
                    defend_goal[i] = 1;
                }
                p.defend_weights += defend_goal[i];
                debug("defend_goal %d %s\n", defend_goal[i], base->name);
            }
        }
        debug("defend_upkeep %3d %d %8s bases: %3d land: %3d sea: %3d air: %3d\n",
            *current_turn, faction, MFactions[faction].filename, bases,
            p.land_combat_units, p.sea_combat_units, p.air_combat_units);
    }
    if (visual) {
        static int k = 0;
        k = (k+1)%4;
        if (k<2) {
            for (int y = 0; y < *map_axis_y; y++) {
                for (int x = y&1; x < *map_axis_x; x+=2) {
                    pm_overlay[x][y] = (k ? arty_value(x, y) :
                        base_tile_score(x, y, 0, TRIAD_SEA));
                }
            }
        } else if (k==2) {
            memcpy(pm_overlay, pm_safety, sizeof(pm_overlay));
        } else if (k==3) {
            invasion_plan(faction);
            memcpy(pm_overlay, pm_enemy_dist, sizeof(pm_overlay));
        }
        return;
    }

    if (has_terra(faction, FORMER_RAISE_LAND, LAND) && f.energy_credits > 100) {
        land_raise_plan(faction, false);
    }
    invasion_plan(faction);
    if (p.naval_end_x >= 0) {
        add_goal(faction, AI_GOAL_NAVAL_START, 3, p.naval_start_x, p.naval_start_y, -1);
        add_goal(faction, AI_GOAL_NAVAL_END, 3, p.naval_end_x, p.naval_end_y, -1);

        for (auto& m : iterate_tiles(p.naval_end_x, p.naval_end_y, 1, 9)) {
            if (allow_move(m.x, m.y, faction, TRIAD_LAND)
            && m.sq->region == p.target_land_region) {
                add_goal(faction, AI_GOAL_NAVAL_BEACH, 3, m.x, m.y, -1);
            }
        }
    }
    for (int i=0; i < MaxGoalsNum; i++) {
        Goal& goal = f.goals[i];
        if (goal.priority <= 0) {
            continue;
        }
        if (goal.type == AI_GOAL_RAISE_LAND) {
            mapnodes.insert({goal.x, goal.y, NODE_GOAL_RAISE_LAND});
            pm_former[goal.x][goal.y] += 8;
            pm_roads[goal.x][goal.y] += 2;
        }
        if (goal.type == AI_GOAL_NAVAL_START) {
            mapnodes.insert({goal.x, goal.y, NODE_GOAL_NAVAL_START});
        }
        if (goal.type == AI_GOAL_NAVAL_END) {
            mapnodes.insert({goal.x, goal.y, NODE_GOAL_NAVAL_END});
        }
        if (goal.type == AI_GOAL_NAVAL_BEACH) {
            mapnodes.insert({goal.x, goal.y, NODE_GOAL_NAVAL_BEACH});
        }
    }
}

bool has_transport(int x, int y, int faction) {
    assert(valid_player(faction));
    for (int i=0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction && veh->x == x && veh->y == y
        && veh->weapon_type() == WPN_TROOP_TRANSPORT) {
            return true;
        }
    }
    return false;
}

bool allow_move(int x, int y, int faction, int triad) {
    assert(valid_player(faction));
    assert(valid_triad(triad));
    MAP* sq;
    if (!(sq = mapsq(x, y)) || non_ally_in_tile(x, y, faction)) {
        return false;
    }
    if (triad != TRIAD_AIR && is_ocean(sq) != (triad == TRIAD_SEA)) {
        return false;
    }
    return !sq->is_owned() || sq->owner == faction
        || has_pact(faction, sq->owner)
        || (at_war(faction, sq->owner) && !sq->is_base());
}

bool non_combat_move(int x, int y, int faction, int triad) {
    assert(valid_player(faction));
    assert(valid_triad(triad));
    MAP* sq = mapsq(x, y);
    if (!sq || (sq->is_owned() && sq->owner != faction)) {
        return false;
    }
    if (triad != TRIAD_AIR && !sq->is_base() && is_ocean(sq) != (triad == TRIAD_SEA)) {
        return false;
    }
    if (!non_ally_in_tile(x, y, faction)) {
        return last_faction != faction || pm_safety[x][y] >= PM_NEAR_SAFE;
    }
    return false;
}

bool has_base_sites(int x, int y, int faction, int triad) {
    assert(valid_player(faction));
    assert(valid_triad(triad));
    MAP* sq;
    int area = 0;
    int bases = 0;
    int i = 0;
    int limit = ((*current_turn * x ^ y) & 7 ? 200 : 500);
    TileSearch ts;
    ts.init(x, y, triad, 2);
    while (++i < limit && (sq = ts.get_next()) != NULL) {
        if (sq->is_base()) {
            bases++;
        } else if (can_build_base(ts.rx, ts.ry, faction, triad)) {
            area++;
        }
    }
    debug("has_base_sites %2d %2d triad: %d area: %d bases: %d\n", x, y, triad, area, bases);
    return area > min(14, bases+4);
}

/*
Determine if the unit should hold on the current tile and skip the turn.
*/
bool defend_tile(VEH* veh, MAP* sq) {
    if (sq && sq->items & BIT_MONOLITH && veh->need_monolith()) {
        return true;
    }
    if (sq && sq->items & (BIT_BASE_IN_TILE | BIT_BUNKER)) {
        if (veh->need_heals()) {
            return true;
        }
        if (!veh->is_combat_unit()) {
            int n = 0;
            for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
                if (pm_safety[m.x][m.y] < PM_SAFE && ++n > 3) {
                    return true;
                }
            }
        }
    }
    return false;
}

int escape_score(int x, int y, int range, VEH* veh, MAP* sq) {
    return pm_safety[x][y] - range*120
        + (sq->items & BIT_MONOLITH && veh->need_monolith() ? 2000 : 0)
        + (sq->items & BIT_BUNKER ? 500 : 0)
        + (sq->items & BIT_FOREST ? 100 : 0)
        + (sq->is_rocky() ? 100 : 0);
}

int escape_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (defend_tile(veh, sq)) {
        return mod_veh_skip(id);
    }
    int best_score = escape_score(veh->x, veh->y, 0, veh, sq);
    int score;
    int tx = -1;
    int ty = -1;
    TileSearch ts;
    ts.init(veh->x, veh->y, veh->triad());
    while ((sq = ts.get_next()) != NULL && ts.dist <= 8) {
        if (ts.dist > 2 && best_score > 500) {
            break;
        }
        if (!non_ally_in_tile(ts.rx, ts.ry, veh->faction_id)
        && (score = escape_score(ts.rx, ts.ry, ts.dist, veh, sq)) > best_score
        && !ts.has_zoc(veh->faction_id)) { // Always check for zocs just in case
            tx = ts.rx;
            ty = ts.ry;
            best_score = score;
            debug("escape_score %2d %2d -> %2d %2d | %d %d\n",
                veh->x, veh->y, ts.rx, ts.ry, ts.dist, score);
        }
    }
    if (tx >= 0) {
        debug("escape_move  %2d %2d -> %2d %2d | %d %d %s\n",
            veh->x, veh->y, tx, ty, pm_safety[veh->x][veh->y], best_score, veh->name());
        return set_move_to(id, tx, ty);
    }
    if (veh->is_combat_unit()) {
        return enemy_move(id);
    }
    return mod_veh_skip(id);
}

int move_to_base(const int id, bool ally) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (sq->is_base()) {
        return mod_veh_skip(id);
    }
    int i = 0;
    int type = (veh->triad() == TRIAD_SEA ? TS_SEA_AND_SHORE : veh->triad());
    TileSearch ts;
    ts.init(veh->x, veh->y, type, 1);
    while (++i < 800 && (sq = ts.get_next()) != NULL) {
        if (sq->is_base() && (sq->owner == veh->faction_id
        || (ally && has_pact(veh->faction_id, sq->owner) && !random(3)))) {
            debug("move_to_base %2d %2d -> %2d %2d %s\n",
                veh->x, veh->y, ts.rx, ts.ry, veh->name());
            return set_move_to(id, ts.rx, ts.ry);
        }
    }
    return mod_veh_skip(id);
}

int want_convoy(int x, int y, int faction, MAP* sq) {
    if (sq && sq->owner == faction && !mapnodes.count({x, y, NODE_CONVOY})
    && !(sq->items & BASE_DISALLOWED)) {
        int bonus = bonus_at(x, y);
        if (bonus == RES_ENERGY)
            return RES_NONE;
        else if (sq->items & (BIT_CONDENSER | BIT_SOIL_ENRICHER))
            return RES_NUTRIENT;
        else if (bonus == RES_NUTRIENT)
            return RES_NONE;
        else if (sq->items & BIT_MINE && sq->is_rocky())
            return RES_MINERAL;
        else if (sq->items & BIT_FOREST && ~sq->items & BIT_RIVER)
            return RES_MINERAL;
    }
    return RES_NONE;
}

int crawler_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (!sq || veh->home_base_id < 0) {
        return mod_veh_skip(id);
    }
    if (!veh->at_target()) {
        return SYNC;
    }
    BASE* base = &Bases[veh->home_base_id];
    int res = want_convoy(veh->x, veh->y, veh->faction_id, sq);
    bool prefer_min = base->nutrient_surplus > 5;
    int v1 = 0;
    if (res) {
        v1 = (sq->items & BIT_FOREST ? 1 : 2);
    }
    if (res && v1 > 1) {
        return set_convoy(id, res);
    }
    int i = 0;
    int tx = -1;
    int ty = -1;
    TileSearch ts;
    ts.init(veh->x, veh->y, TS_TRIAD_LAND);

    while (++i < 50 && (sq = ts.get_next()) != NULL) {
        if (pm_safety[ts.rx][ts.ry] < PM_SAFE
        || non_ally_in_tile(ts.rx, ts.ry, veh->faction_id)) {
            continue;
        }
        res = want_convoy(ts.rx, ts.ry, veh->faction_id, sq);
        if (res) {
            int v2 = (sq->items & BIT_FOREST ? 1 : 2);
            if (prefer_min && res == RES_MINERAL && v2 > 1) {
                return set_move_to(id, ts.rx, ts.ry);
            } else if (!prefer_min && res == RES_NUTRIENT) {
                return set_move_to(id, ts.rx, ts.ry);
            }
            if (res == RES_MINERAL && v2 > v1) {
                tx = ts.rx;
                ty = ts.ry;
                v1 = v2;
            }
        }
    }
    if (tx >= 0) {
        return set_move_to(id, tx, ty);
    }
    if (v1 > 0) {
        return set_convoy(id, RES_MINERAL);
    }
    return mod_veh_skip(id);
}

bool safe_path(const TileSearch& ts, int faction) {
    int i = 0;
    const PathNode* node = &ts.paths[ts.current];

    while (node->dist > 0 && ++i < PathLimit) {
        assert(node->prev >= 0);
        MAP* sq = mapsq(node->x, node->y);
        if (pm_safety[node->x][node->y] < PM_SAFE) {
//            debug("path_unsafe %d %d %d\n", faction, node->x, node->y);
            return false;
        }
        if (sq && sq->is_owned() && sq->owner != faction && !has_pact(faction, sq->owner)) {
//            debug("path_foreign %d %d %d\n", faction, node->x, node->y);
            return false;
        }
        node = &ts.paths[node->prev];
    }
    return true;
}

bool valid_colony_path(int id) {
    VEH* veh = &Vehicles[id];
    int faction = veh->faction_id;
    int tx = veh->waypoint_1_x;
    int ty = veh->waypoint_1_y;

    if (veh->at_target() || !can_build_base(tx, ty, faction, veh->triad())) {
        return false;
    }
    if (~veh->state & VSTATE_UNK_40000 && (veh->state & VSTATE_UNK_2000
    || map_range(veh->x, veh->y, tx, ty) > 10)) {
        return false;
    }
    return true;
}

bool can_build_base(int x, int y, int faction, int triad) {
    assert(valid_player(faction));
    assert(valid_triad(triad));
    MAP* sq;
    if (!(sq = mapsq(x, y))) // Invalid map coordinates
        return false;
    if (y < 3 || y >= *map_axis_y - 3)
        return false;
    if (*map_toggle_flat & 1 && (x < 2 || x >= *map_axis_x - 2))
        return false;
    if ((sq->is_rocky() && !is_ocean(sq)) || (sq->items & (BASE_DISALLOWED | IMP_ADVANCED)))
        return false;
    if (sq->is_owned() && faction != sq->owner && !at_war(faction, sq->owner))
        return false;
    if (non_ally_in_tile(x, y, faction))
        return false;
    if (sq->landmarks & LM_VOLCANO && sq->art_ref_id == 0)
        return false;

    int range = conf.base_spacing - (conf.base_nearby_limit < 0 ? 1 : 0);
    int num = 0;
    int i = 0;
    for (auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        if (m.sq->is_base()) {
            if (conf.base_nearby_limit < 0 || i < TableRange[range - 1]
            || ++num > conf.base_nearby_limit) {
                return false;
            }
        }
        i++;
    }
    if (triad != TRIAD_SEA && !is_ocean(sq)) {
        return true;
    } else if ((triad == TRIAD_SEA || triad == TRIAD_AIR) && is_ocean_shelf(sq)) {
        return true;
    }
    return false;
}

int base_tile_score(int x1, int y1, int range, int triad) {
    const int priority[][2] = {
        {BIT_RIVER, 1},
        {BIT_FUNGUS, -2},
        {BIT_FARM, 2},
        {BIT_FOREST, 2},
        {BIT_MONOLITH, 4},
    };
    MAP* sq = mapsq(x1, y1);
    int score = min(20, 40 - 80*abs(*map_axis_y/2 - y1)/(*map_axis_y));
    int land = 0;

    if (!is_ocean(sq)) {
        score += (ocean_coast_tiles(x1, y1) ? 14 : 0);
        score += (sq->items & BIT_RIVER ? 5 : 0);
    }
    for (int i=1; i <= 20; i++) {
        int x2 = wrap(x1 + TableOffsetX[i]);
        int y2 = y1 + TableOffsetY[i];

        if ((sq = mapsq(x2, y2)) && !sq->is_base()) {
            assert(map_range(x1, y1, x2, y2) == 1 + (i > 8));
            score += (bonus_at(x2, y2) ? 6 : 0);
            if (sq->landmarks && !(sq->landmarks & (LM_DUNES | LM_SARGASSO | LM_UNITY))) {
                score += (sq->landmarks & LM_JUNGLE ? 3 : 2);
            }
            if (i <= 8) { // Only adjacent tiles
                if (triad == TRIAD_SEA && sq->is_land_region()
                && Continents[sq->region].tile_count >= 20 && ++land < 3) {
                    score += (!sq->is_owned() ? 20 : 3);
                }
                if (is_ocean_shelf(sq)) {
                    score += (triad == TRIAD_SEA ? 3 : 2);
                }
            }
            if (!is_ocean(sq)) {
                if (sq->is_rainy()) {
                    score += 2;
                }
                if (sq->is_moist() && sq->is_rolling()) {
                    score += 2;
                }
            }
            for (const int* p : priority) if (sq->items & p[0]) score += p[1];
        }
    }
    return score - 2*range + min(0, pm_safety[x1][y1]);
}

int colony_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    int veh_region = sq->region;
    int faction = veh->faction_id;
    int triad = veh->triad();
    if (defend_tile(veh, sq) || veh->iter_count > 3) {
        return mod_veh_skip(id);
    }
    if (pm_safety[veh->x][veh->y] < PM_SAFE) {
        return escape_move(id);
    }
    if (can_build_base(veh->x, veh->y, faction, triad)) {
        if (triad == TRIAD_LAND && (veh->at_target() || !near_ocean_coast(veh->x, veh->y))) {
            return action_build(id, 0);
        } else if (triad == TRIAD_SEA && (veh->at_target() || !random(12))) {
            return action_build(id, 0);
        } else if (triad == TRIAD_AIR) {
            return action_build(id, 0);
        }
    }
    if (is_ocean(sq) && triad == TRIAD_LAND) {
        if (!has_transport(veh->x, veh->y, faction)) {
            mapnodes.insert({veh->x, veh->y, NODE_NEED_FERRY});
            return mod_veh_skip(id);
        }
        for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
            if (non_combat_move(m.x, m.y, faction, triad)
            && has_base_sites(m.x, m.y, faction, triad) && !random(2)) {
                debug("colony_trans %2d %2d -> %2d %2d\n", veh->x, veh->y, m.x, m.y);
                return set_move_to(id, m.x, m.y);
            }
        }
        return mod_veh_skip(id);
    }
    if (valid_colony_path(id)) {
        for (auto& m : iterate_tiles(veh->waypoint_1_x, veh->waypoint_1_y, 0, 9)) {
            mapnodes.insert({m.x, m.y, NODE_BASE_SITE});
        }
        return SYNC;
    }
    bool full_search = (*current_turn + id) % 5 == 0;
    int limit = full_search ? 800 : 200;
    int best_score = INT_MIN;
    int i = 0;
    int k = 0;
    int tx = -1;
    int ty = -1;
    TileSearch ts;
    if (triad == TRIAD_SEA && invasion_unit(id) && ocean_coast_tiles(veh->x, veh->y)) {
        ts.init(plans[faction].naval_end_x, plans[faction].naval_end_y, triad, 2);
    } else {
        ts.init(veh->x, veh->y, triad, 2);
    }
    while (++i < limit && k < 50 && (sq = ts.get_next()) != NULL) {
        if (mapnodes.count({ts.rx, ts.ry, NODE_BASE_SITE})
        || !can_build_base(ts.rx, ts.ry, faction, triad)
        || !safe_path(ts, faction)) {
            continue;
        }
        int score = base_tile_score(ts.rx, ts.ry, ts.dist, triad);
        k++;
        if (score > best_score) {
            tx = ts.rx;
            ty = ts.ry;
            best_score = score;
        }
    }
    if (tx >= 0) {
        debug("colony_move %d %d -> %d %d score: %d\n", veh->x, veh->y, tx, ty, best_score);
        for (auto& m : iterate_tiles(tx, ty, 0, TableRange[conf.base_spacing - 1])) {
            mapnodes.insert({m.x, m.y, NODE_BASE_SITE});
        }
        // Set these flags to disable any non-Thinker unit automation.
        veh->state |= VSTATE_UNK_40000;
        veh->state &= ~VSTATE_UNK_2000;
        return set_move_to(id, tx, ty);

    } else if (full_search && *current_turn > 20) {
        AIPlans& p = plans[faction];
        if (p.naval_start_x >= 0 && veh_region == p.main_region
        && veh->x != p.naval_start_x && veh->y != p.naval_start_y) {
            return set_move_to(id, p.naval_start_x, p.naval_start_y);
        }
        if (!invasion_unit(id)) {
            return mod_veh_kill(id);
        }
    }
    return mod_veh_skip(id);
}

bool can_bridge(int x, int y, int faction, MAP* sq) {
    if (is_ocean(sq) || !has_terra(faction, FORMER_RAISE_LAND, LAND)) {
        return false;
    }
    if (is_shore_level(sq) && mapnodes.count({x, y, NODE_GOAL_RAISE_LAND})) {
        return true;
    }
    if (coast_tiles(x, y) < 3) {
        return false;
    }
    const int range = 4;
    TileSearch ts;
    ts.init(x, y, TRIAD_LAND);
    int k = 0;
    while (++k < 120 && ts.get_next() != NULL);

    int n = 0;
    for (auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        if (!is_ocean(m.sq) && m.y > 0 && m.y < *map_axis_y-1
        && !ts.oldtiles.count({m.x, m.y})) {
            n++;
            if (m.sq->is_owned() && m.sq->owner != faction
            && faction_might(faction) < faction_might(m.sq->owner)) {
                return false;
            }
        }
    }
    debug("bridge %d x: %d y: %d tiles: %d\n", faction, x, y, n);
    return n > 4;
}

bool can_borehole(int x, int y, int faction, int bonus, MAP* sq) {
    if (!has_terra(faction, FORMER_THERMAL_BORE, is_ocean(sq)))
        return false;
    if (!sq || sq->items & (BASE_DISALLOWED | IMP_ADVANCED) || bonus == RES_NUTRIENT)
        return false;
    // Planet factions should build boreholes only in reduced numbers.
    if (Factions[faction].SE_planet_base > 0 && (map_hash(x, y) & 0xff > 30))
        return false;
    if (bonus == RES_NONE && sq->is_rolling())
        return false;
    if (pm_former[x][y] < 4 && !mapnodes.count({x, y, NODE_BOREHOLE}))
        return false;
    int level = sq->alt_level();
    for (const int* t : NearbyTiles) {
        int x2 = wrap(x + t[0]);
        int y2 = y + t[1];
        sq = mapsq(x2, y2);
        if (!sq || sq->items & BIT_THERMAL_BORE || mapnodes.count({x2, y2, NODE_BOREHOLE})) {
            return false;
        }
        int level2 = sq->alt_level();
        if (level2 < level && level2 > ALT_OCEAN_SHELF) {
            return false;
        }
    }
    return true;
}

bool can_farm(int x, int y, int faction, int bonus, bool has_nut, MAP* sq) {
    bool rain = sq->is_rainy_or_moist();
    if (!has_terra(faction, FORMER_FARM, is_ocean(sq)))
        return false;
    if (bonus == RES_NUTRIENT && (has_nut || (rain && sq->is_rolling())))
        return true;
    if (bonus == RES_ENERGY || bonus == RES_MINERAL)
        return false;
    if (sq->landmarks & LM_JUNGLE && !has_nut)
        return false;
    if (nearby_items(x, y, 1, BIT_FOREST) < (sq->items & BIT_FOREST ? 4 : 1)
    && has_terra(faction, FORMER_FOREST, is_ocean(sq)))
        return false;
    if (!has_nut && nearby_items(x, y, 1, BIT_FARM | BIT_SOLAR) > 2)
        return false;
    if (rain && sq->is_rolling())
        return nearby_items(x, y, 1, BIT_FARM | BIT_CONDENSER) < (sq->items & BIT_FARM ? 4 : 3);
    return (has_nut && !(x % 2) && !(y % 2) && !(abs(x-y) % 4));
}

bool can_fungus(int x, int y, int faction, int bonus, MAP* sq) {
    if (sq->items & (BIT_BASE_IN_TILE | BIT_MONOLITH | IMP_ADVANCED))
        return false;
    return !can_borehole(x, y, faction, bonus, sq) && nearby_items(x, y, 1, BIT_FUNGUS) < 5;
}

bool can_river(int x, int y, int faction, MAP* sq) {
    if (!has_terra(faction, FORMER_AQUIFER, is_ocean(sq)))
        return false;
    if (sq->items & (BIT_BASE_IN_TILE | BIT_RIVER | BIT_THERMAL_BORE))
        return false;
    return !((*current_turn * x ^ y) & 15)
        && !coast_tiles(x, y) && nearby_items(x, y, 1, BIT_RIVER) < 2
        && nearby_items(x, y, 1, BIT_THERMAL_BORE) < 2;
}

bool can_road(int x, int y, int faction, MAP* sq) {
    if (is_ocean(sq) || !has_terra(faction, FORMER_ROAD, LAND)
    || sq->items & (BIT_ROAD | BIT_BASE_IN_TILE))
        return false;
    if (~sq->items & BIT_BASE_RADIUS && pm_roads[x][y] < 1)
        return false;
    if (sq->items & BIT_FUNGUS && (!has_tech(faction, Rules->tech_preq_build_road_fungus)
    || (!build_tubes && has_project(faction, FAC_XENOEMPATHY_DOME))))
        return false;
    if (mapnodes.count({x, y, NODE_GOAL_RAISE_LAND}))
        return true;
    if (pm_roads[x][y] > 0 || sq->items & (BIT_MINE | BIT_CONDENSER | BIT_THERMAL_BORE))
        return true;
    int i = 0;
    int r[] = {0,0,0,0,0,0,0,0};
    for (const int* t : NearbyTiles) {
        sq = mapsq(wrap(x + t[0]), y + t[1]);
        if (!is_ocean(sq) && sq->owner == faction) {
            if (sq->items & (BIT_ROAD | BIT_BASE_IN_TILE)) {
                r[i] = 1;
            }
        }
        i++;
    }
    // Determine if we should connect roads on opposite sides of the tile
    if ((r[0] && r[4] && !r[2] && !r[6])
    || (r[2] && r[6] && !r[0] && !r[4])
    || (r[1] && r[5] && !((r[2] && r[4]) || (r[0] && r[6])))
    || (r[3] && r[7] && !((r[0] && r[2]) || (r[4] && r[6])))) {
        return true;
    }
    return false;
}

bool can_magtube(int x, int y, int faction, MAP* sq) {
    if (is_ocean(sq) || pm_roads[x][y] < 1 || sq->items & (BIT_MAGTUBE | BIT_BASE_IN_TILE)) {
        return false;
    }
    return has_terra(faction, FORMER_MAGTUBE, 0) && sq->items & BIT_ROAD
        && (~sq->items & BIT_FUNGUS || has_tech(faction, Rules->tech_preq_build_road_fungus));
}

bool can_sensor(int x, int y, int faction, MAP* sq) {
    if (!has_terra(faction, FORMER_SENSOR, is_ocean(sq)) || sq->items & BIT_SENSOR)
        return false;
    if (sq->items & BIT_FUNGUS && !has_tech(faction, Rules->tech_preq_improv_fungus))
        return false;
    int i = 1;
    for (auto& m : iterate_tiles(x, y, 1, 25)) {
        if (m.sq->owner == faction && (m.sq->items & BIT_SENSOR
        || (i < 9 && mapnodes.count({m.x, m.y, NODE_SENSOR_ARRAY})))) {
            return false;
        }
        i++;
    }
    return true;
}

int select_item(int x, int y, int faction, MAP* sq) {
    assert(valid_player(faction));
    assert(mapsq(x, y));
    int items = sq->items;
    bool sea = is_ocean(sq);
    if (sq->is_base() || (sea && !is_ocean_shelf(sq))
    || (sq->landmarks & LM_VOLCANO && sq->art_ref_id == 0)) {
        return -1;
    }
    int bonus = bonus_at(x, y);
    bool rocky_sq = sq->is_rocky();
    bool has_eco = has_terra(faction, FORMER_CONDENSER, LAND);
    bool has_min = has_tech(faction, Rules->tech_preq_allow_3_minerals_sq);
    bool has_nut = has_tech(faction, Rules->tech_preq_allow_3_nutrients_sq);
    bool road = can_road(x, y, faction, sq);

    if (can_magtube(x, y, faction, sq)) {
        return FORMER_MAGTUBE;
    }
    if (items & BIT_FUNGUS) {
        if (plans[faction].keep_fungus && can_fungus(x, y, faction, bonus, sq)) {
            if (road)
                return FORMER_ROAD;
            else if (can_sensor(x, y, faction, sq))
                return FORMER_SENSOR;
            return -1;
        }
        return FORMER_REMOVE_FUNGUS;
    }
    if (can_bridge(x, y, faction, sq)) {
        if (mapnodes.count({x, y, NODE_RAISE_LAND})
        || terraform_cost(x, y, faction) < Factions[faction].energy_credits/8) {
            return (road ? FORMER_ROAD : FORMER_RAISE_LAND);
        }
    }
    if (~items & BIT_BASE_RADIUS || items & (BIT_MONOLITH | BIT_FUNGUS)) {
        return (road ? FORMER_ROAD : -1);
    }
    if (road) {
        return FORMER_ROAD;
    }
    if (plans[faction].plant_fungus && can_fungus(x, y, faction, bonus, sq)) {
        return FORMER_PLANT_FUNGUS;
    }
    if (sea) {
        bool improved = items & (BIT_MINE | BIT_SOLAR | BIT_SENSOR);
        if (~items & BIT_FARM && has_terra(faction, FORMER_FARM, sea))
            return FORMER_FARM;
        else if (!improved && bonus == RES_NONE && can_sensor(x, y, faction, sq))
            return FORMER_SENSOR;
        else if (!improved && has_terra(faction, FORMER_SOLAR, sea) && bonus != RES_MINERAL
        && (bonus == RES_NUTRIENT || nearby_items(x, y, 1, BIT_SOLAR)+1 < nearby_items(x, y, 1, BIT_MINE)))
            return FORMER_SOLAR;
        else if (!improved && has_terra(faction, FORMER_MINE, sea))
            return FORMER_MINE;
        return -1;
    }
    if ((has_min || bonus != RES_NONE) && can_borehole(x, y, faction, bonus, sq))
        return FORMER_THERMAL_BORE;
    if (items & BIT_THERMAL_BORE)
        return -1;
    if (can_river(x, y, faction, sq))
        return FORMER_AQUIFER;
    if (rocky_sq && ~items & BIT_MINE && (has_min || bonus == RES_MINERAL)
    && has_terra(faction, FORMER_MINE, sea))
        return FORMER_MINE;
    if (rocky_sq && (bonus == RES_NUTRIENT || sq->landmarks & LM_JUNGLE)
    && has_terra(faction, FORMER_LEVEL_TERRAIN, sea))
        return FORMER_LEVEL_TERRAIN;
    if (!rocky_sq && can_farm(x, y, faction, bonus, has_nut, sq)) {
        bool improved = items & (BIT_CONDENSER | BIT_SOLAR);
        if (!has_nut) {
            if (!improved && sq->is_rainy() && has_terra(faction, FORMER_SOLAR, sea))
                return FORMER_SOLAR;
            else if (~items & BIT_FARM && sq->is_moist() && sq->is_rolling())
                return FORMER_FARM;
        }
        if (has_nut || bonus == RES_NUTRIENT) {
            if (~items & BIT_FARM)
                return FORMER_FARM;
            else if (~items & BIT_CONDENSER && has_eco)
                return FORMER_CONDENSER;
            else if (~items & BIT_SOIL_ENRICHER && has_terra(faction, FORMER_SOIL_ENR, sea))
                return FORMER_SOIL_ENR;
            else if (!has_eco && !improved && has_terra(faction, FORMER_SOLAR, sea)
            && (bonus != RES_NONE || items & BIT_RIVER))
                return FORMER_SOLAR;
        }
    } else if (!rocky_sq && !(items & IMP_ADVANCED)) {
        if (can_sensor(x, y, faction, sq))
            return FORMER_SENSOR;
        else if (~items & BIT_FOREST && has_terra(faction, FORMER_FOREST, sea))
            return FORMER_FOREST;
    }
    return -1;
}

int former_tile_score(int x1, int y1, int x2, int y2, int faction, MAP* sq) {
    const int priority[][2] = {
        {BIT_RIVER, 2},
        {BIT_FARM, -3},
        {BIT_SOLAR, -2},
        {BIT_FOREST, -4},
        {BIT_MINE, -4},
        {BIT_CONDENSER, -4},
        {BIT_SOIL_ENRICHER, -4},
        {BIT_THERMAL_BORE, -8},
    };
    int bonus = bonus_at(x2, y2);
    int range = map_range(x1, y1, x2, y2);
    int items = sq->items;
    int score = (sq->landmarks ? 3 : 0);

    if (bonus && !(items & IMP_ADVANCED)) {
        bool bh = (bonus == RES_MINERAL || bonus == RES_ENERGY) && can_borehole(x2, y2, faction, bonus, sq);
        int w = (bh || !(items & IMP_SIMPLE) ? 5 : 1);
        score += w * (bonus == RES_NUTRIENT ? 3 : 2);
    }
    for (const int* p : priority) {
        if (items & p[0]) {
            score += p[1];
        }
    }
    if (items & BIT_FUNGUS) {
        score += (items & IMP_ADVANCED ? 20 : 0);
        score += (plans[faction].keep_fungus ? -10 : -2);
        score += (plans[faction].plant_fungus && (items & BIT_ROAD) ? -8 : 0);
    }
    if (items & (BIT_FOREST | BIT_SENSOR) && can_road(x2, y2, faction, sq)) {
        score += 8;
    }
    if (pm_roads[x2][y2] > 0 && (~items & BIT_ROAD || (build_tubes && ~items & BIT_MAGTUBE))) {
        score += 12;
    }
    if (is_shore_level(sq) && mapnodes.count({x2, y2, NODE_GOAL_RAISE_LAND})) {
        score += 20;
    }
    return score - range/2 + min(8, pm_former[x2][y2]) + min(0, pm_safety[x2][y2]);
}

int former_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    int faction = veh->faction_id;
    bool safe = pm_safety[veh->x][veh->y] >= PM_SAFE;
    int item;
    int choice;
    if (sq->owner != faction) {
        return move_to_base(id, false);
    }
    if (defend_tile(veh, sq)) {
        return mod_veh_skip(id);
    }
    if (is_ocean(sq) && veh->triad() == TRIAD_LAND) {
        if (!has_transport(veh->x, veh->y, faction)) {
            mapnodes.insert({veh->x, veh->y, NODE_NEED_FERRY});
            return mod_veh_skip(id);
        }
        for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
            if (non_combat_move(m.x, m.y, faction, TRIAD_LAND) && !random(2)) {
                debug("former_trans %2d %2d -> %2d %2d\n", veh->x, veh->y, m.x, m.y);
                return set_move_to(id, m.x, m.y);
            }
        }
        return mod_veh_skip(id);
    }
    int turns = (veh->move_status >= ORDER_FARM && veh->move_status < ORDER_MOVE_TO ?
        Terraform[veh->move_status - 4].rate : 0);

    if (safe || turns >= 12) {
        if (turns > 0) {
            return SYNC;
        }
        if (!veh->at_target() && !can_road(veh->x, veh->y, faction, sq)
        && !can_magtube(veh->x, veh->y, faction, sq)) {
            return SYNC;
        }
        item = select_item(veh->x, veh->y, faction, sq);
        if (item >= 0) {
            int cost = 0;
            pm_former[veh->x][veh->y] -= 2;
            if (item == FORMER_RAISE_LAND && !mapnodes.count({veh->x, veh->y, NODE_RAISE_LAND})) {
                cost = terraform_cost(veh->x, veh->y, faction);
                Factions[faction].energy_credits -= cost;
            }
            debug("former_action %2d %2d cost: %d %s\n",
                veh->x, veh->y, cost, Terraform[item].name);
            return set_action(id, item+4, *Terraform[item].shortcuts);
        }
    } else if (!safe) {
        return escape_move(id);
    }
    bool full_search = (*current_turn + id) % 5 == 0;
    int limit = full_search ? 250 : 50;
    int best_score = INT_MIN;
    int i = 0;
    int tx = -1;
    int ty = -1;
    int bx = veh->x;
    int by = veh->y;
    if (veh->home_base_id >= 0) {
        bx = Bases[veh->home_base_id].x;
        by = Bases[veh->home_base_id].y;
    }
    TileSearch ts;
    ts.init(veh->x, veh->y, veh->triad());

    while (++i < limit && (sq = ts.get_next()) != NULL) {
        if (sq->owner != faction
        || sq->is_base()
        || pm_safety[ts.rx][ts.ry] < PM_SAFE
        || (pm_former[ts.rx][ts.ry] < 1 && pm_roads[ts.rx][ts.ry] < 1)
        || non_ally_in_tile(ts.rx, ts.ry, faction)) {
            continue;
        }
        int score = former_tile_score(bx, by, ts.rx, ts.ry, faction, sq);
        if (score > best_score && (choice = select_item(ts.rx, ts.ry, faction, sq)) >= 0) {
            tx = ts.rx;
            ty = ts.ry;
            best_score = score;
            item = choice;
        }
    }
    if (tx >= 0) {
        pm_former[tx][ty] -= 2;
        debug("former_move %2d %2d -> %2d %2d score: %d %s\n",
            veh->x, veh->y, tx, ty, best_score, (item >= 0 ? Terraform[item].name : ""));
        return set_move_to(id, tx, ty);
    } else if (!random(4)) {
        return set_move_to(id, bx, by);
    } else if (full_search && *current_turn > 20) {
        return mod_veh_kill(id);
    }
    debug("former_skip %2d %2d %s\n", veh->x, veh->y, veh->name());
    return mod_veh_skip(id);
}

bool allow_scout(int faction, MAP* sq) {
    return !sq->is_visible(faction)
        && sq->region != plans[faction].target_land_region
        && !sq->is_pole_tile()
        && (((!sq->is_owned() || sq->owner == faction) && !random(8))
        || (sq->is_owned() && sq->owner != faction
        && !has_treaty(faction, sq->owner, DIPLO_COMMLINK)));
}

bool allow_probe(int faction1, int faction2) {
    int diplo = Factions[faction1].diplo_status[faction2];
    if (faction1 != faction2) {
        if (!(diplo & DIPLO_COMMLINK)) {
            return true;
        }
        if (has_project(faction2, FAC_HUNTER_SEEKER_ALGO)
        && !has_ability(faction1, ABL_ID_ALGO_ENHANCEMENT)) {
            return false;
        }
        if (at_war(faction1, faction2)) {
            return true;
        }
        if (!(diplo & DIPLO_PACT)) {
            int value = 0;
            if (diplo & DIPLO_TREATY)
                value--;
            if (Factions[faction1].tech_ranking < Factions[faction2].tech_ranking)
                value += 2;
            if (Factions[faction1].AI_fight > 0)
                value++;
            if (Factions[faction1].AI_power > 0)
                value++;
            if (Factions[faction2].integrity_blemishes > 0)
                value++;
            if (*game_rules & RULES_INTENSE_RIVALRY && is_human(faction2))
                value += 2;
            return value > 2;
        }
    }
    return false;
}

bool allow_attack(int faction1, int faction2, bool is_probe) {
    return (is_probe ? allow_probe(faction1, faction2) : at_war(faction1, faction2));
}

int garrison_goal(int x, int y, int faction, int triad) {
    assert(triad == TRIAD_LAND || triad == TRIAD_SEA);
    AIPlans& p = plans[faction];
    if (triad == TRIAD_LAND && p.naval_start_x == x && p.naval_start_y == y) {
        return 8;
    }
    for (int i=0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->x == x && base->y == y) {
            int goal = max(1, min(5,
                (p.land_combat_units + p.sea_combat_units) * defend_goal[i] / p.defend_weights)
                - (triad == TRIAD_SEA ? 2 : 0)
                - (*current_turn < 80 ? 1 : 0)
                - (p.contacted_factions < 2 && p.unknown_factions > 1 ? 1 : 0)
                - (!random(4) * (p.enemy_mil_factor < 1 ? 2 : 1)));

            debug("garrison_goal %2d %2d %2d %s\n", x, y, goal, base->name);
            return goal;
        }
    }
    assert(false);
    return 0;
}

int defender_count(int x, int y) {
    int num = 0;
    for (int k=0; k < *total_num_vehicles; k++) {
        VEH* veh = &Vehicles[k];
        if (veh->x == x && veh->y == y
        && veh->move_status != ORDER_SENTRY_BOARD
        && veh->at_target()) {
            if (veh->is_combat_unit()) {
                num += 2;
            } else if (veh->is_probe()) {
                num += (veh->is_armored() ? 2 : 1);
            }
        }
    }
    return (num > 2 ? (num+1)/2 : num/2);
}

/*
For artillery units, Thinker tries to first determine the best stackup location
before deciding any attack targets. This important heuristic chooses
squares that have the most friendly AND enemy units in 2-tile range.
It is also used to prioritize aircraft attack targets.
*/
int arty_value(int x, int y) {
    return pm_unit_near[x][y] * pm_enemy_near[x][y];
}

/*
Assign some predefined proportion of units into the active naval invasion plan.
Thinker chooses these units randomly based on the Veh ID modulus. This guarantees
that we'll always have enough units following a plan without having to store any
extra information in the Veh structure. Even though the IDs are not permanent,
it is not a problem for the movement planning in practise.
*/
bool invasion_unit(int id) {
    AIPlans& p = plans[Vehicles[id].faction_id];
    MFaction& m = MFactions[Vehicles[id].faction_id];

    if (p.naval_start_x < 0) {
        return false;
    }
    return (id % 8) > (p.prioritize_naval ? 1 : 3)
        + (Vehicles[id].triad() == TRIAD_SEA ? 0 : 2)
        + (m.thinker_enemy_range >= 14 ? 0 : 1);
}

bool near_landing(int x, int y, int faction) {
    AIPlans& p = plans[faction];
    if (mapnodes.count({x, y, NODE_GOAL_NAVAL_END})) {
        return true;
    }
    for (auto& m : iterate_tiles(x, y, 1, 9)) {
        if (!allow_move(m.x, m.y, faction, TRIAD_LAND)) {
            continue;
        }
        if (p.naval_end_x < 0 && !m.sq->is_owned()) {
            return !random(4);
        }
    }
    return false;
}

int make_landing(int id) {
    VEH* veh = &Vehicles[id];
    AIPlans& p = plans[veh->faction_id];
    int x2 = -1;
    int y2 = -1;
    for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
        if (!allow_move(m.x, m.y, veh->faction_id, TRIAD_LAND)) {
            continue;
        }
        if (mapnodes.count({m.x, m.y, NODE_GOAL_NAVAL_BEACH})) {
            x2 = m.x;
            y2 = m.y;
            if (!random(3)) {
                break;
            }
        }
        if (p.naval_end_x < 0 && !m.sq->is_owned()) {
            x2 = m.x;
            y2 = m.y;
        }
    }
    if (x2 >= 0) {
        return set_move_to(id, x2, y2); // Save Private Ryan
    }
    return SYNC;
}

int trans_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq;
    AIPlans& p = plans[veh->faction_id];
    static int iter = 0;
    if (!(sq = mapsq(veh->x, veh->y))
    || mapnodes.erase({veh->x, veh->y, NODE_NEED_FERRY}) > 0
    || (sq->is_base() && veh->need_heals())) {
        return mod_veh_skip(id);
    }
    if (++iter % 3) {
        return SYNC;
    }
    int offset;
    int cargo = 0;
    int defenders = 0;
    int capacity = cargo_capacity(id);

    for (int k=0; k < *total_num_vehicles; k++) {
        VEH* v = &Vehicles[k];
        if (veh->x == v->x && veh->y == v->y && k != id && v->triad() == TRIAD_LAND) {
            if (v->move_status == ORDER_SENTRY_BOARD && v->waypoint_1_x == id) {
                cargo++;
            } else if (v->is_combat_unit() || v->is_probe()) {
                defenders++;
            }
        }
    }
    debug("trans_move %2d %2d defenders: %d cargo: %d capacity: %d %s\n",
        veh->x, veh->y, defenders, cargo, cargo_capacity(id), veh->name());

    if (is_ocean(sq)) {
        if (invasion_unit(id) && sq->region != p.main_sea_region
        && (offset = path_get_next(veh->x, veh->y, p.naval_start_x, p.naval_start_y,
        veh->unit_id, veh->faction_id)) > 0) {
            return set_move_next(id, offset);
        }
        if (cargo > 0 && near_landing(veh->x, veh->y, veh->faction_id)) {
            for (int k=0; k < *total_num_vehicles; k++) {
                VEH* v = &Vehicles[k];
                if (veh->x == v->x && veh->y == v->y && k != id && v->triad() == TRIAD_LAND) {
                    make_landing(k);
                }
            }
            return mod_veh_skip(id);
        }
        if (!cargo && mapnodes.count({veh->x, veh->y, NODE_GOAL_NAVAL_END})) {
            return set_move_to(id, p.naval_start_x, p.naval_start_y);
        }
    }
    if (invasion_unit(id) && mapnodes.count({veh->x, veh->y, NODE_GOAL_NAVAL_START})) {
        for (int k=0; k < *total_num_vehicles; k++) {
            if (cargo >= capacity) {
                break;
            }
            VEH* v = &Vehicles[k];
            if (veh->faction_id == v->faction_id
            && veh->x == v->x && veh->y == v->y
            && (v->is_combat_unit() || v->is_probe())
            && v->triad() == TRIAD_LAND && v->move_status != ORDER_SENTRY_BOARD) {
                set_board_to(k, id);
                cargo++;
            }
        }
        if (cargo >= min(4, capacity)) {
            return set_move_to(id, p.naval_end_x, p.naval_end_y);
        }
        if (veh->iter_count < 4) {
            return SYNC;
        }
        return mod_veh_skip(id); 
    }
    if (cargo > 0 && !sq->is_base() && sq->region == p.main_sea_region && p.naval_end_x >= 0) {
        if (veh->x == p.naval_end_x && veh->y == p.naval_end_y) {
            return mod_veh_skip(id);
        }
        return set_move_to(id, p.naval_end_x, p.naval_end_y);
    }
    if (!veh->at_target() && veh->iter_count < 2) {
        if (!pm_enemy_near[veh->x][veh->y] && !veh->need_heals()) {
            for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
                if (mapnodes.count({m.x, m.y, NODE_PATROL})
                && allow_move(m.x, m.y, veh->faction_id, veh->triad())) {
                    return set_move_to(id, m.x, m.y);
                }
            }
        }
        return SYNC;
    }
    int max_dist = ((*current_turn + id) % 5 ? 7 : 16);
    int tx = -1;
    int ty = -1;
    TileSearch ts;
    ts.init(veh->x, veh->y, TS_SEA_AND_SHORE);

    while ((sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        if (non_combat_move(ts.rx, ts.ry, veh->faction_id, TRIAD_SEA)) {
            bool return_to_base = sq->is_base() && sq->owner == veh->faction_id
                && (veh->need_heals() || (cargo > 0 && p.naval_end_x < 0
                && sq->region == p.main_region));

            if (mapnodes.erase({ts.rx, ts.ry, NODE_PATROL}) > 0
            || mapnodes.erase({ts.rx, ts.ry, NODE_NEED_FERRY}) > 0
            || return_to_base) {
                debug("trans_move %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
                return set_move_to(id, ts.rx, ts.ry);

            } else if (ts.dist < 12 && allow_scout(veh->faction_id, sq) && !invasion_unit(id)) {
                tx = ts.rx;
                ty = ts.ry;
            }
        }
    }
    if (p.naval_start_x >= 0 && invasion_unit(id) && !cargo) {
        return set_move_to(id, p.naval_start_x, p.naval_start_y);
    }
    if (tx >= 0) {
        return set_move_to(id, tx, ty);
    }
    return mod_veh_skip(id);
}

int choose_defender(int x, int y, int att_id, MAP* sq) {
    int def_id = veh_at(x, y);
    int faction = Vehicles[att_id].faction_id;
    if (def_id < 0 || (sq->owner != faction && !sq->is_base()
    && !Vehicles[def_id].is_visible(faction))) {
        return -1;
    }
    def_id = best_defender(def_id, att_id, 0);
    if (def_id >= 0 && !at_war(faction, Vehicles[def_id].faction_id)) {
        return -1;
    }
    return def_id;
}

int combat_value(int id, int power, int moves, int mov_rate) {
    VEH* veh = &Vehicles[id];
    int damage = veh->damage_taken / veh->reactor_type();
    assert(damage >= 0 && damage < 10);
    assert(moves > 0 && moves <= mov_rate);
    return max(1, power * (10 - damage) * moves / mov_rate);
}

int flank_score(int x, int y, MAP* sq) {
    return random(16) - 4*pm_enemy_dist[x][y]
        + (sq && sq->items & (BIT_RIVER | BIT_ROAD | BIT_FOREST) ? 3 : 0)
        + (sq && sq->items & (BIT_SENSOR | BIT_BUNKER) ? 3 : 0);
}

double battle_eval(int id1, int id2, int moves, int mov_rate) {
    int s1;
    int s2;
    battle_compute(id1, id2, (int)&s1, (int)&s2, 0);
    int v1 = combat_value(id1, s1, moves, mov_rate);
    int v2 = combat_value(id2, s2, mov_rate, mov_rate);
    return 1.0*v1/v2;
}

double battle_priority(int id1, int id2, int dist, int moves, MAP* sq) {
    if (!sq) {
        return 0;
    }
    VEH* veh1 = &Vehicles[id1];
    VEH* veh2 = &Vehicles[id2];
    assert(id1 >= 0 && id1 < *total_num_vehicles);
    assert(id2 >= 0 && id2 < *total_num_vehicles);
    assert(veh1->faction_id != veh2->faction_id);
    assert((dist == 0) != (map_range(veh1->x, veh1->y, veh2->x, veh2->y) > 1));
    UNIT* u1 = &Units[veh1->unit_id];
    UNIT* u2 = &Units[veh2->unit_id];
    bool non_psi = veh1->offense_value() >= 0 && veh2->defense_value() >= 0;
    bool stack_damage = !sq->is_base_or_bunker() && conf.collateral_damage_value > 0;
    int triad = veh1->triad();
    int cost = 0;
    int att_moves = 1;
    int mov_rate = 1;

    // Calculate actual movement cost for hasty penalties
    if (triad == TRIAD_LAND && non_psi) {
        int x1 = veh1->x;
        int y1 = veh1->y;
        int x2 = -1;
        int y2 = -1;
        int val = 0;
        while (dist > 0 && val >= 0 && cost < moves) {
            val = path_get_next(x1, y1, veh2->x, veh2->y, veh1->unit_id, veh1->faction_id);
            if (val >= 0) {
                x2 = wrap(x1 + BaseOffsetX[val]);
                y2 = y1 + BaseOffsetY[val];
                if (x2 == veh2->x && y2 == veh2->y) {
                    break;
                }
                cost += hex_cost(veh1->unit_id, veh1->faction_id, x1, y1, x2, y2, 0);
                x1 = x2;
                y1 = y2;
            }
        }
        if (moves - cost > 0) {
            att_moves = min(moves - cost, Rules->mov_rate_along_roads);
            mov_rate = Rules->mov_rate_along_roads;
        } else { // Tile is not reachable during this turn
            att_moves = 2;
            mov_rate = 3;
        }
    }
    double v1 = battle_eval(
        id1,
        id2,
        att_moves,
        mov_rate
    );
    double v2 = (sq->owner == veh1->faction_id ? (sq->is_base_radius() ? 0.15 : 0.1) : 0.0)
        + (stack_damage ? 0.03 * (pm_enemy[veh2->x][veh2->y]) : 0.0)
        + min(12, abs(offense_value(u2))) * (u2->chassis_type == CHS_INFANTRY ? 0.01 : 0.02)
        + (triad == TRIAD_AIR ? 0.001 * min(400, arty_value(veh2->x, veh2->y)) : 0.0)
        - 0.01*dist;
    double v3 = min(!offense_value(u2) ? 1.3 : 1.6, v1) + min(0.5, max(-0.5, v2));

    debug("combat_odds %2d %2d -> %2d %2d dist: %2d moves: %2d cost: %2d "\
        "v1: %.4f v2: %.4f odds: %.4f | %d %d %s | %d %d %s\n",
        veh1->x, veh1->y, veh2->x, veh2->y, dist, moves, cost,
        v1, v2, v3, id1, veh1->faction_id, u1->name, id2, veh2->faction_id, u2->name);
    return v3;
}

int aircraft_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    UNIT* u = &Units[veh->unit_id];
    AIPlans& p = plans[veh->faction_id];
    bool at_base = sq->is_base();
    int faction = veh->faction_id;
    int moves = mod_veh_speed(id) - veh->road_moves_spent;
    int max_dist = max(0, moves / Rules->mov_rate_along_roads);
    assert(moves >= Rules->mov_rate_along_roads);

    if (!veh->at_target()) {
        return SYNC;
    }
    if (u->chassis_type == CHS_NEEDLEJET || u->chassis_type == CHS_MISSILE) {
        if (veh->iter_count == 0 && ++veh->iter_count && !at_base) {
            return move_to_base(id, true);
        }
    }
    if (u->chassis_type == CHS_COPTER && !at_base && veh->mid_damage()) {
        max_dist = (veh->high_damage() ? 1 : 3);
    }
    if (at_base && veh->mid_damage()) {
        max_dist = min(5, max(1, pm_unit_near[veh->x][veh->y] / 2));
    }
    int i = 0;
    int tx = -1;
    int ty = -1;
    int id2 = -1;
    double best_odds = 1.2 - 0.005 * min(100, p.air_combat_units);
    TileSearch ts;
    ts.init(veh->x, veh->y, TRIAD_AIR);

    while (++i < 625 && (sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        bool to_base = sq->is_base();

        if (at_war(faction, unit_in_tile(sq))
        && (id2 = choose_defender(ts.rx, ts.ry, id, sq)) >= 0) {
            VEH* veh2 = &Vehicles[id2];
            if (!to_base && veh2->chassis_type() == CHS_NEEDLEJET
            && !has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)) {
                continue;
            }
            double odds = battle_priority(id, id2, ts.dist - 1, moves, sq);

            if (odds > best_odds) {
                if (tx < 0) {
                    max_dist = min(ts.dist + 2, max_dist);
                }
                tx = ts.rx;
                ty = ts.ry;
                best_odds = odds;
            }
        } else if (non_ally_in_tile(ts.rx, ts.ry, faction)) {
            continue;
        } else if (tx < 0 && mapnodes.count({ts.rx, ts.ry, NODE_COMBAT_PATROL})) {
            return set_move_to(id, ts.rx, ts.ry);
        } else if (tx < 0 && allow_scout(faction, sq)) {
            return set_move_to(id, ts.rx, ts.ry);
        }
    }
    if (tx >= 0) {
        int range = map_range(veh->x, veh->y, tx, ty);
        if (range > 1) {
            for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
                if (!m.sq->is_base() && !non_ally_in_tile(m.x, m.y, faction)
                && map_range(m.x, m.y, tx, ty) < range) {
                    return set_move_to(id, m.x, m.y);
                }
            }
        }
        debug("aircraft_attack %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
        return set_move_to(id, tx, ty);
    }
    if (p.naval_airbase_x >= 0 && id % (p.prioritize_naval ? 5 : 9) == 0
    && map_range(veh->x, veh->y, p.naval_airbase_x, p.naval_airbase_y) >= 20) {
        debug("aircraft_invade %2d %2d -> %2d %2d\n",
            veh->x, veh->y, p.naval_airbase_x, p.naval_airbase_y);
        return set_move_to(id, p.naval_airbase_x, p.naval_airbase_y);
    }
    if ((*current_turn + id) % 4 == 0) {
        int best_score = INT_MIN;
        for (i=0; i < *total_num_bases; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction && defend_goal[i] >= 3) {
                int score = defend_goal[i]*20 + random(8)
                    - map_range(veh->x, veh->y, base->x, base->y);
                if (score > best_score) {
                    tx = base->x;
                    ty = base->y;
                    best_score = score;
                }
            }
        }
        if (tx >= 0 && !(veh->x == tx && veh->y == ty)) {
            debug("aircraft_rebase %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
            return set_move_to(id, tx, ty);
        }
    }
    if (!at_base && (u->chassis_type != CHS_GRAVSHIP || !random(4))) {
        return move_to_base(id, true);
    }
    return veh_skip(id);
}

bool airdrop_move(const int id, MAP* sq) {
    VEH* veh = &Vehicles[id];
    if (!has_abil(veh->unit_id, ABL_DROP_POD) || veh->state & VSTATE_MADE_AIRDROP) {
        return false;
    }
    int faction = veh->faction_id;
    int veh_region = sq->region;
    bool invade = invasion_unit(id) && veh_region != plans[faction].target_land_region;
    int tx = -1;
    int ty = -1;

    for (int i = 9; i < TableRange[Rules->max_airdrop_rng_wo_orbital_insert]; i++) {
        int x2 = wrap(veh->x + TableOffsetX[i]);
        int y2 = veh->y + TableOffsetY[i];
        if ((sq = mapsq(x2, y2)) && sq->is_base() && !non_ally_in_tile(x2, y2, faction)) {
            if (at_war(faction, sq->owner)) {
                tx = x2;
                ty = y2;
                break;
            } else if (sq->owner == faction) {
                if (invade && sq->region == plans[faction].target_land_region) {
                    tx = x2;
                    ty = y2;
                }
                if (i > TableRange[2]
                && pm_enemy_near[x2][y2] > 1 + pm_enemy_near[veh->x][veh->y]
                && defender_count(x2, y2) < 4) {
                    tx = x2;
                    ty = y2;
                }
                if (tx >= 0 && !random(4)) {
                    break;
                }
            }
        }
    }
    if (tx >= 0) {
        for (int i=0; i < *total_num_bases; i++) {
            if (at_war(faction, Bases[i].faction_id)
            && map_range(Bases[i].x, Bases[i].y, tx, ty) <= 2
            && has_facility(i, FAC_AEROSPACE_COMPLEX)) {
                return false;
            }
        }
        action_airdrop(id, tx, ty, 3);
        return true;
    }
    return false;
}

int combat_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    AIPlans& p = plans[veh->faction_id];
    int faction = veh->faction_id;
    int triad = veh->triad();
    int veh_region = sq->region;
    int moves = mod_veh_speed(id) - veh->road_moves_spent;
    assert(veh->is_combat_unit() || veh->is_probe());
    assert(triad == TRIAD_LAND || triad == TRIAD_SEA);
    /*
    Ships have both normal and artillery attack modes available.
    For land-based artillery, skip normal attack evaluation.
    */
    bool combat = veh->is_combat_unit();
    bool attack = combat && !can_arty(veh->unit_id, false);
    bool arty   = combat && can_arty(veh->unit_id, true);
    bool ignore_zocs = triad == TRIAD_SEA || veh->is_probe();
    uint32_t enemy_items = (at_war(faction, sq->owner) ? sq->items : 0);
    bool at_home = sq->owner == faction || has_pact(faction, sq->owner);
    bool at_base = sq->is_base();
    bool at_enemy = at_war(faction, sq->owner);
    bool base_found = at_base;
    /*
    Scouting priority is mainly represented by maximum search distance.
    */
    int max_dist = min(8, max((veh->high_damage() ? 2 : 3),
        random(4) + (triad == TRIAD_SEA ? 2 : 0) + (at_base ? 0 : 2)
        + (veh->need_heals() ? -5 : 0) + (at_enemy ? -3 : 0)
        + max(0, 3 - *current_turn/40)
        + (p.unknown_factions / 2) + (p.contacted_factions ? 0 : 2)
    ));
    int defenders = 0;

    if (at_base && airdrop_move(id, sq)) {
        return SYNC;
    }
    if (triad == TRIAD_LAND && mapnodes.count({veh->x, veh->y, NODE_GOAL_NAVAL_END})) {
        return make_landing(id);
    }
    if (triad == TRIAD_LAND && is_ocean(sq) && at_base && !has_transport(veh->x, veh->y, faction)) {
        return mod_veh_skip(id);
    }
    if (veh->is_probe() && veh->need_heals()) {
        return escape_move(id);
    }
    if (!veh->at_target() && veh->iter_count < 2) {
        if (!pm_enemy_near[veh->x][veh->y] && !veh->need_heals()) {
            for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
                if (mapnodes.count({m.x, m.y, NODE_PATROL})
                && allow_move(m.x, m.y, faction, triad)) {
                    return set_move_to(id, m.x, m.y);
                }
            }
        }
        return SYNC;
    }
    if (at_base) {
        defenders = max(0, defender_count(veh->x, veh->y) - 1); // Excluding this unit
        if (!defenders) {
            max_dist = 1;
        }
    }
    bool defend;
    if (triad == TRIAD_SEA) {
        defend = (at_home && (id % 11) < 3 && !veh->is_probe())
            || (!(region_flags[veh_region] & (veh->is_probe() ? PM_PROBE : PM_ENEMY))
            && !(region_flags[p.main_sea_region] & (veh->is_probe() ? PM_PROBE : PM_ENEMY)));
    } else {
        defend = (at_home && (id % 11) < 5)
            || (veh->is_probe() && veh->chassis_type() == CHS_INFANTRY)
            || !(region_flags[veh_region] & (veh->is_probe() ? PM_PROBE : PM_ENEMY));
    }
    bool landing_unit = triad == TRIAD_LAND
        && (!at_base || defenders > 0)
        && veh_region == p.main_region
        && invasion_unit(id);
    bool invasion_ship = triad == TRIAD_SEA
        && (!at_base || defenders > 0)
        && veh_region == p.main_sea_region
        && !veh->is_probe()
        && invasion_unit(id);
    int i;
    int limit;
    int tx = -1;
    int ty = -1;
    int px = -1;
    int py = -1;
    int id2 = -1;
    int type = (veh->unit_id == BSC_SEALURK ? TS_SEA_AND_SHORE : triad);
    /*
    Current minimum odds for the unit to engage in any combat.
    Tolerate worse odds if the faction has many more expendable units available.
    */
    double best_odds = (at_base && defenders < 1 ? 1.4 : 1.2)
        - (at_enemy ? 0.15 : 0)
        - 0.008*min(50, pm_unit_near[veh->x][veh->y])
        - 0.0005*min(500, p.land_combat_units + p.sea_combat_units);
    int best_score = arty_value(veh->x, veh->y);
    TileSearch ts;
    ts.init(veh->x, veh->y, type);

    while (combat && (sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        bool to_base = sq->is_base();
        base_found = base_found || to_base;
        assert(veh->x != ts.rx || veh->y != ts.ry);
        assert(map_range(veh->x, veh->y, ts.rx, ts.ry) <= ts.dist);
        assert((at_base && to_base) < ts.dist);

        if (attack && at_war(faction, unit_in_tile(sq))
        && (id2 = choose_defender(ts.rx, ts.ry, id, sq)) >= 0) {
            VEH* veh2 = &Vehicles[id2];
            if (!ignore_zocs) { // Avoid zones of control
                max_dist = ts.dist;
            }
            if (!to_base && veh2->chassis_type() == CHS_NEEDLEJET
            && !has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)) {
                continue;
            }
            double odds = battle_priority(id, id2, ts.dist - 1, moves, sq);

            if (odds > best_odds) {
                tx = ts.rx;
                ty = ts.ry;
                best_odds = odds;
            } else if (tx < 0 && ts.dist < 2 && veh2->faction_id == 0 && veh->iter_count > 1) {
                return escape_move(id);
            }
        } else if (to_base && at_war(faction, sq->owner) && pm_enemy[ts.rx][ts.ry] < 1) {
            return set_move_to(id, ts.rx, ts.ry);
        } else if (non_ally_in_tile(ts.rx, ts.ry, faction)) {
            continue;
        } else if (arty && veh->iter_count < 2
        && arty_value(ts.rx, ts.ry) - 4*ts.dist > best_score
        && allow_move(ts.rx, ts.ry, faction, triad)) {
            tx = ts.rx;
            ty = ts.ry;
            best_score = arty_value(ts.rx, ts.ry) - 4*ts.dist;
        } else if (tx < 0 && attack && mapnodes.count({ts.rx, ts.ry, NODE_COMBAT_PATROL})) {
            return set_move_to(id, ts.rx, ts.ry);
        } else if (tx < 0 && mapnodes.count({ts.rx, ts.ry, NODE_PATROL})) {
            return set_move_to(id, ts.rx, ts.ry);
        } else if (px < 0 && allow_scout(faction, sq)) {
            px = ts.rx;
            py = ts.ry;
        }
    }
    if (tx >= 0) {
        debug("combat_attack %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
        return set_move_to(id, tx, ty);
    }
    if (px >= 0) {
        debug("combat_scout %2d %2d -> %2d %2d\n", veh->x, veh->y, px, py);
        return set_move_to(id, px, py);
    }
    if (arty) {
        int offset = 0;
        best_score = 0;
        tx = -1;
        ty = -1;
        for (i = 1; i < TableRange[Rules->artillery_max_rng]; i++) {
            int x2 = wrap(veh->x + TableOffsetX[i]);
            int y2 = veh->y + TableOffsetY[i];
            if ((sq = mapsq(x2, y2)) && pm_enemy[x2][y2] > 0) {
                int arty_limit;
                if (sq->is_base_or_bunker()) {
                    arty_limit = Rules->max_dmg_percent_arty_base_bunker/10;
                } else {
                    arty_limit = (is_ocean(sq) ?
                        Rules->max_dmg_percent_arty_sea : Rules->max_dmg_percent_arty_open)/10;
                }
                int score = 0;
                for (int k=0; k < *total_num_vehicles; k++) {
                    VEH* v = &Vehicles[k];
                    if (v->x == x2 && v->y == y2 && at_war(faction, v->faction_id)
                    && (v->triad() != TRIAD_AIR || sq->is_base())) {
                        int damage = v->damage_taken / v->reactor_type();
                        assert(damage >= 0 && damage < 10);
                        score += max(0, arty_limit - damage)
                            * (v->faction_id > 0 ? 2 : 1)
                            * (v->is_combat_unit() ? 2 : 1)
                            * (sq->owner == faction ? 2 : 1);
                    }
                }
                if (!score) {
                    continue;
                }
                score = score * (sq->is_base() ? 2 : 3) * (arty_limit == 10 ? 2 : 1) + random(32);
                if (score > best_score) {
                    best_score = score;
                    offset = i;
                    tx = x2;
                    ty = y2;
                }
            }
        }
        if (tx >= 0 && ((at_base && !defenders)
        || random(100) < 100*best_score/(best_score+100))) {
            debug("combat_arty %2d %2d -> %2d %2d score: %d %s\n",
                veh->x, veh->y, tx, ty, best_score, veh->name());
            battle_fight_1(id, offset, 1, 1, 0);
            return SYNC;
        }
    } else if (enemy_items & (BIT_SENSOR | BIT_AIRBASE | BIT_THERMAL_BORE)) {
        return action_destroy(id, 0, -1, -1);
    }
    if (at_base && (!defenders || defenders < garrison_goal(veh->x, veh->y, faction, triad))) {
        debug("combat_defend %2d %2d %s\n", veh->x, veh->y, veh->name());
        return mod_veh_skip(id);
    }
    if (veh->need_heals()) {
        return escape_move(id);
    }
    if (!veh->at_target() && veh->iter_count > 2 && at_base) {
        return mod_veh_skip(id);
    }
    if (triad == TRIAD_SEA && p.naval_scout_x >= 0
    && (p.naval_end_x < 0 || map_range(veh->x, veh->y, p.naval_end_x, p.naval_end_y) > 15)
    && !random(p.enemy_factions ? 16 : 8)) {
        for (auto& m : iterate_tiles(p.naval_scout_x, p.naval_scout_y, 0, 9)) {
            if (allow_move(m.x, m.y, faction, TRIAD_SEA) && !random(4)) {
                debug("combat_patrol %2d %2d -> %2d %2d\n", veh->x, veh->y, m.x, m.y);
                return set_move_to(id, m.x, m.y);
            }
        }
    }
    // Check if the unit should move to stackup point or board naval transport
    if (landing_unit) {
        tx = p.naval_start_x;
        ty = p.naval_start_y;

        if (veh->x == tx && veh->y == ty) {
            return mod_veh_skip(id);
        } else if (defender_count(tx, ty) < 12) {
            debug("combat_stack %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
            return set_move_to(id, tx, ty);
        }
    }
    // Provide cover for naval transports
    if (invasion_ship && !veh->road_moves_spent
    && arty_value(veh->x, veh->y) < arty_value(p.naval_end_x, p.naval_end_y)) {
        debug("combat_escort %2d %2d -> %2d %2d\n", veh->x, veh->y, p.naval_end_x, p.naval_end_y);
        return set_move_to(id, p.naval_end_x, p.naval_end_y);
    }
    tx = -1;
    ty = -1;
    // Continue same TileSearch instance
    if (triad == TRIAD_SEA && arty) {
        max_dist = max_dist + random(4);
        best_score = arty_value(veh->x, veh->y);
        while ((sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
            if (arty_value(ts.rx, ts.ry) - ts.dist > best_score
            && allow_move(ts.rx, ts.ry, faction, triad)) {
                tx = ts.rx;
                ty = ts.ry;
            }
        }
        if (tx >= 0) {
            debug("combat_adjust %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
            return set_move_to(id, tx, ty);
        }
    }
    // Find a base to attack or defend own base on the same region
    int tolerance = (ignore_zocs ? 3 : 1) + (triad == TRIAD_SEA ? 2 : 0)
        + (!pm_enemy_near[veh->x][veh->y] ? 2 : 0);
    if (defend) {
        limit = (((*current_turn+id) % 4) + 1) * QueueSize/32;
    } else {
        limit = (((*current_turn+id) % 4) + 1) * QueueSize/4;
    }
    bool flank = false;
    bool check_zocs = !ignore_zocs && pm_enemy_near[veh->x][veh->y];
    PathNode& current = ts.paths[0];
    max_dist = PathLimit;
    best_score = INT_MIN;
    i = 0;
    tx = -1;
    ty = -1;
    if (triad == TRIAD_SEA && (veh->is_probe() || veh->unit_id == BSC_SEALURK)) {
        ts.init(veh->x, veh->y, TS_SEA_AND_SHORE);
    } else {
        ts.init(veh->x, veh->y, triad, (triad == TRIAD_LAND ? 1 : 0)); // Skip pole tiles
    }
    while (++i < limit && (sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        base_found = base_found || sq->is_base();
        if (!sq->is_base() || (check_zocs && ts.has_zoc(faction))) {
            continue;
        }
        if (defend && sq->owner == faction) {
            if (garrison_goal(ts.rx, ts.ry, faction, triad) - defender_count(ts.rx, ts.ry)
            >= ++pm_former[ts.rx][ts.ry]/3) {
                debug("combat_defend %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
                return set_move_to(id, ts.rx, ts.ry);
            }
        } else if (!defend && allow_attack(faction, sq->owner, veh->is_probe())) {
            assert(sq->owner != faction);
            if (tx < 0) {
                max_dist = ts.dist + tolerance;
            }
            int score = target_priority(ts.rx, ts.ry, faction, sq) - 16*ts.dist + random(120);
            if (score > best_score) {
                best_score = score;
                current = ts.paths[ts.current];
                tx = ts.rx;
                ty = ts.ry;
                flank = !veh->is_probe() && ts.dist < 6
                    && (id2 = choose_defender(ts.rx, ts.ry, id, sq)) >= 0
                    && battle_priority(id, id2, ts.dist - 1, moves, sq) < 0.7;
                if (flank) {
                    debug("combat_skip %2d %2d -> %2d %2d %s %s\n",
                    veh->x, veh->y, ts.rx, ts.ry, veh->name(), Vehicles[id2].name());
                }
            }
        }
    }
    if (tx >= 0) {
        if (flank || (!defend && current.dist > 1 && min(9, ++pm_former[tx][ty]) >= random(12))) {
            tx = -1;
            ty = -1;
            best_score = flank_score(veh->x, veh->y, mapsq(veh->x, veh->y));
            ts.init(veh->x, veh->y, triad);
            while ((sq = ts.get_next()) != NULL && ts.dist <= 2) {
                if (!sq->is_base() && allow_move(ts.rx, ts.ry, faction, triad)) {
                    int score = flank_score(ts.rx, ts.ry, sq);
                    if (score > best_score) {
                        tx = ts.rx;
                        ty = ts.ry;
                        best_score = score;
                    }
                }
            }
            if (tx >= 0) {
                debug("combat_flank %2d %2d -> %2d %2d %s\n", veh->x, veh->y, tx, ty, veh->name());
                return set_move_to(id, tx, ty);
            }
        } else if (!defend && !veh->is_probe()) {
            PathNode& prev = ts.paths[current.prev];
            if (current.dist > 3 && pm_enemy_near[tx][ty] > 1 + random(8)) {
                prev = ts.paths[prev.prev];
            }
            tx = prev.x;
            ty = prev.y;
        }
        debug("combat_search %2d %2d -> %2d %2d %s\n", veh->x, veh->y, tx, ty, veh->name());
        return set_move_to(id, tx, ty);
    }
    if (!base_found && !pm_enemy_near[veh->x][veh->y] && veh->home_base_id >= 0 && !random(4)) {
        return mod_veh_kill(id);
    }
    if (!at_base && !random(4)) {
        return move_to_base(id, true);
    }
    return mod_veh_skip(id);
}



