
#include "move.h"

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

Non-combat units controlled by Thinker (formers, colony pods, crawlers)
use pm_safety table to decide if there are enemy units too close to them.
PM_SAFE defines the threshold below which the units will attempt to flee
to the square chosen by escape_move.
*/

PMTable pm_former;
PMTable pm_safety;
PMTable pm_target;
PMTable pm_roads;
PMTable pm_shore;
PMTable pm_unit_near;
PMTable pm_enemy;
PMTable pm_enemy_near;
PMTable pm_enemy_dist;
PMTable pm_overlay;

static Points nonally;
static int upkeep_faction = 0;
static int build_tubes = false;
static int region_flags[MaxRegionNum] = {};
int base_enemy_range[MaxBaseNum] = {};


void adjust_value(PMTable tbl, int x, int y, int range, int value) {
    assert(mapsq(x, y) && range > 0 && range < 9);
    for (const auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        int sum = tbl[m.x][m.y] + value;
        if (sum >= SHRT_MIN && sum <= SHRT_MAX) {
            tbl[m.x][m.y] = sum;
        }
    }
}

bool non_ally_in_tile(int x, int y, int faction) {
    assert(mapsq(x, y));
    static int prev_veh_num = 0;
    static int prev_faction = 0;
    if (prev_veh_num != *VehCount || prev_faction != faction) {
        nonally.clear();
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehicles[i];
            if (veh->faction_id != faction && !has_pact(faction, veh->faction_id)) {
                nonally.insert({veh->x, veh->y});
            }
        }
        debug("refresh %d %d %d\n", *CurrentTurn, faction, *VehCount);
        prev_veh_num = *VehCount;
        prev_faction = faction;
    }
    return nonally.count({x, y}) > 0;
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
    // Exclude moves that might attack enemy bases
    return !sq->is_owned() || sq->owner == faction
        || has_pact(faction, sq->owner)
        || (at_war(faction, sq->owner) && !sq->is_base());
}

bool allow_civ_move(int x, int y, int faction, int triad) {
    assert(valid_player(faction));
    assert(valid_triad(triad));
    MAP* sq;
    if (!(sq = mapsq(x, y)) || (sq->is_owned() && sq->owner != faction)) {
        return false;
    }
    if (triad != TRIAD_AIR && !sq->is_base() && is_ocean(sq) != (triad == TRIAD_SEA)) {
        return false;
    }
    if (!non_ally_in_tile(x, y, faction)) {
        return upkeep_faction != faction || pm_safety[x][y] >= PM_NEAR_SAFE;
    }
    return false;
}

bool stack_search(int x, int y, int faction, StackType type, VehWeaponMode mode) {
    bool found = false;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y) {
            if (type == ST_EnemyOneUnit && (found || !at_war(faction, veh->faction_id))) {
                return false;
            }
            found = true;
            if (type == ST_NeutralOnly && (faction == veh->faction_id
            || !both_neutral(faction, veh->faction_id))) {
                return false;
            }
            if (type == ST_NonPactOnly && (faction == veh->faction_id
            || has_pact(faction, veh->faction_id))) {
                return false;
            }
            if (mode == WMODE_COMBAT && veh->weapon_mode() > WMODE_MISSILE) {
                return false;
            }
            if (mode != WMODE_COMBAT && veh->weapon_mode() != mode) {
                return false;
            }
        }
    }
    return found;
}

// Any sea region or inland lakes
int coast_tiles(int x, int y) {
    assert(mapsq(x, y));
    return pm_shore[x][y] & ShoreTilesMask;
}

// Ocean refers only to main_sea_region
int ocean_coast_tiles(int x, int y) {
    assert(mapsq(x, y));
    return pm_shore[x][y] / ShoreLine;
}

bool near_ocean_coast(int x, int y) {
    assert(mapsq(x, y));
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

int __cdecl mod_enemy_move(int veh_id) {
    assert(veh_id >= 0 && veh_id < *VehCount);
    VEH* veh = &Vehicles[veh_id];
    debug("enemy_move %2d %2d %s\n", veh->x, veh->y, veh->name());

    if (thinker_enabled(veh->faction_id)) {
        int triad = veh->triad();
        if (!mapsq(veh->x, veh->y)) {
            return VEH_SYNC;
        } else if (veh->is_colony()) {
            return colony_move(veh_id);
        } else if (veh->is_supply()) {
            return crawler_move(veh_id);
        } else if (veh->is_former()) {
            return former_move(veh_id);
        } else if (triad == TRIAD_SEA && veh_cargo(veh_id) > 0) {
            return trans_move(veh_id);
        } else if (triad == TRIAD_AIR && veh->is_combat_unit() && !veh->is_planet_buster()) {
            return aircraft_move(veh_id);
        } else if (triad != TRIAD_AIR && (veh->is_combat_unit() || veh->is_probe())) {
            return combat_move(veh_id);
        }
    } else if (conf.manage_player_units && veh->plr_owner()
    && (veh->is_colony() || veh->is_former())) {
        if (!*CurrentBase) {
            int base_id = mod_base_find3(veh->x, veh->y, veh->faction_id, -1, -1, -1);
            if (base_id >= 0) {
                set_base(base_id);
            }
        }
        if (upkeep_faction != veh->faction_id) {
            former_plans(veh->faction_id);
            move_upkeep(veh->faction_id, UM_Player);
            upkeep_faction = veh->faction_id;
        }
        if (veh->is_colony()) {
            return colony_move(veh_id);
        } else if (veh->is_former()) {
            return former_move(veh_id);
        }
    }
    int num = *VehCount;
    int iter = veh->iter_count;
    int value = enemy_move(veh_id);
    if (value == VEH_SKIP && veh->iter_count == iter && num == *VehCount) {
        // Avoid infinite loops if enemy_move does not update the vehicle
        return VEH_SYNC;
    }
    return value;
}

int __cdecl veh_kill_lift(int veh_id) {
    // This function is called in veh_kill when a vehicle is removed/killed for any reason
    VEH* veh = &Vehicles[veh_id];
    if (thinker_enabled(upkeep_faction) && at_war(upkeep_faction, veh->faction_id)) {
        if (veh->x >= 0 && veh->y >= 0) {
            adjust_value(pm_enemy_near, veh->x, veh->y, 2, -1);
            pm_enemy[veh->x][veh->y]--;
            if (pm_target[veh->x][veh->y] > 0) {
                pm_target[veh->x][veh->y]--;
            }
        }
    }
    return veh_lift(veh_id);
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

void connect_roads(int x, int y, int faction) {
    if (!is_ocean(mapsq(x, y)) && pm_roads[x][y] < 1) {
        MAP* sq;
        TileSearch ts;
        ts.init(x, y, TRIAD_LAND, 1);
        while ((sq = ts.get_next()) != NULL && ts.dist < 10) {
            if (sq->is_base() && sq->owner == faction) {
                adjust_route(pm_roads, ts, 1);
                return;
            }
        }
    }
}

int route_distance(PMTable tbl, int x1, int y1, int x2, int y2) {
    Points visited;
    std::list<PathNode> items;
    items.push_back({x1, y1, 0, 0});
    int limit = max(8, map_range(x1, y1, x2, y2) * 2);
    int i = 0;

    while (items.size() > 0 && ++i <= 120) {
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
void land_raise_plan(int faction) {
    AIPlans& p = plans[faction];
    if (p.main_region < 0) {
        return;
    }
    if (!has_terra(faction, FORMER_RAISE_LAND, LAND) || Factions[faction].energy_credits < 100) {
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
    while (++i <= 1600 && (sq = ts.get_next()) != NULL) {
        assert(!is_ocean(sq));
        assert(sq->owner == faction);
        assert(sq->region == p.main_region);
        if (coast_tiles(ts.rx, ts.ry) > 0 && !sq->is_base()) {
            assert(is_shore_level(sq));
            path.push_back({ts.rx, ts.ry});
        }
    }
    debug("raise_plan %d region: %d tiles: %d expand: %d\n",
        faction, p.main_region, Continents[p.main_region].tile_count, expand);
    ts.init(path, TS_SEA_AND_SHORE, 4);

    while ((sq = ts.get_next()) != NULL && ts.dist <= 12) {
        if (sq->region == p.main_region || !sq->is_land_region() || (!sq->is_owned() && !expand)) {
            continue;
        }
        // Check if we should bridge to hostile territory
        if (sq->owner != faction && sq->is_owned() && !has_pact(faction, sq->owner)
        && faction_might(faction) < faction_might(sq->owner)) {
            continue;
        }
        int score = min(160, Continents[sq->region].tile_count) - ts.dist*ts.dist/2
            + (has_goal(faction, AI_GOAL_RAISE_LAND, ts.rx, ts.ry) > 0 ? 30 : 0);

        if (score > best_score) {
            ts.get_route(path);
            best_score = score;
            debug("raise_goal %2d %2d -> %2d %2d dist: %2d size: %3d owner: %d score: %d\n",
                path.begin()->x, path.begin()->y, ts.rx, ts.ry, ts.dist,
                Continents[sq->region].tile_count, sq->owner, score);

            for (auto& pp : path) {
                add_goal(faction, AI_GOAL_RAISE_LAND, 3, pp.x, pp.y, -1);
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
        if (is_human(sq->owner)) {
            score += (*GameRules & RULES_INTENSE_RIVALRY ? 400 : 200);
        }
        if (f.base_id_attack_target >= 0
        && Bases[f.base_id_attack_target].x == x
        && Bases[f.base_id_attack_target].y == y) {
            score += 400;
        }
        if (sq->is_base()) {
            score += (pm_roads[x][y] ? 150 : 0);
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
        if (i != faction && Factions[i].base_count) {
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

    for (int i=0; i < *BaseCount; i++) {
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
    if (!has_ships(faction)) {
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
        for (i=0; i < *BaseCount; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction) {
                int dist = map_range(base->x, base->y, p.naval_end_x, p.naval_end_y)
                    - (has_facility(FAC_AEROSPACE_COMPLEX, i) ? 5 : 0) + random(4);
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
        }
    }

    for (int i=0; i < *BaseCount; i++) {
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
    while (++i <= 800 && (sq = ts.get_next()) != NULL) {
        if (at_war(faction, sq->owner) && !is_ocean(sq) && ++k >= 10) {
            p.prioritize_naval = 0;
            return;
        }
    }
    int min_dist = INT_MAX;
    for (i=1; i < MaxPlayerNum; i++) {
        int dist = (plans[i].main_region_x < 0 ? MaxEnemyRange :
            map_range(p.main_region_x, p.main_region_y,
                plans[i].main_region_x, plans[i].main_region_y));
        if (at_war(faction, i) && dist < min_dist) {
            min_dist = dist;
            p.prioritize_naval = p.main_region != plans[i].main_region;
        }
    }
}

void move_upkeep(int faction, UpdateMode mode) {
    Faction& f = Factions[faction];
    AIPlans& p = plans[faction];
    if (!is_alive(faction)) {
        return;
    }
    if (((mode == UM_Player) != is_human(faction))
    || (mode != UM_Player && !thinker_enabled(faction))) {
        return;
    }
    int tile_count[MaxRegionNum] = {};
    upkeep_faction = faction;
    build_tubes = has_terra(faction, FORMER_MAGTUBE, LAND);
    memset(pm_former, 0, sizeof(pm_former));
    memset(pm_safety, 0, sizeof(pm_safety));
    memset(pm_target, 0, sizeof(pm_target));
    memset(pm_roads, 0, sizeof(pm_roads));
    memset(pm_shore, 0, sizeof(pm_shore));
    memset(pm_enemy, 0, sizeof(pm_enemy));
    memset(pm_unit_near, 0, sizeof(pm_unit_near));
    memset(pm_enemy_near, 0, sizeof(pm_enemy_near));
    memset(region_flags, 0, sizeof(region_flags));
    mapnodes.clear();
    update_main_region(faction);

    debug("move_upkeep %d region: %d x: %2d y: %2d naval: %d\n",
        faction, p.main_region, p.main_region_x, p.main_region_y, p.prioritize_naval);

    MAP* sq;
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x+=2) {
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
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x+=2) {
            if ((sq = mapsq(x, y)) && sq->region == p.main_sea_region) {
                adjust_value(pm_shore, x, y, 1, ShoreLine);
            }
            assert(goody_at(x, y) == mod_goody_at(x, y));
            assert(bonus_at(x, y) == mod_bonus_at(x, y));
        }
    }
    for (int i=0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        int triad = veh->triad();

        if (!(sq = mapsq(veh->x, veh->y))) {
            continue;
        }
        if (veh->order == ORDER_THERMAL_BOREHOLE) {
            mapnodes.insert({veh->x, veh->y, NODE_BOREHOLE});
        }
        if (veh->order == ORDER_TERRAFORM_UP) {
            mapnodes.insert({veh->x, veh->y, NODE_RAISE_LAND});
        }
        if (veh->order == ORDER_SENSOR_ARRAY) {
            mapnodes.insert({veh->x, veh->y, NODE_SENSOR_ARRAY});
        }
        if (veh->faction_id == faction) {
            if (veh->is_combat_unit()) {
                adjust_value(pm_safety, veh->x, veh->y, 1, 40);
                pm_safety[veh->x][veh->y] += 60;
            }
            if (triad == TRIAD_LAND && !is_ocean(sq)) {
                veh->state &= ~VSTATE_IN_TRANSPORT;
            }
            if ((veh->is_colony() || veh->is_former() || veh->is_supply()) && triad == TRIAD_LAND) {
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

        } else if (has_pact(faction, veh->faction_id) && veh->is_combat_unit()) {
            adjust_value(pm_safety, veh->x, veh->y, 1, 20);
            pm_safety[veh->x][veh->y] += 30;
        }
        // Check if we can evict neutral probe teams from home territory
        // Also check if we can capture artifacts from neutral or home territory
        if (veh->faction_id != faction) {
            if (veh->is_artifact()
            && (sq->owner == faction || !sq->is_owned())
            && stack_search(veh->x, veh->y, faction, ST_NonPactOnly, WMODE_ARTIFACT)) {
                mapnodes.insert({veh->x, veh->y, NODE_COMBAT_PATROL});
                debug("capture_artifact %2d %2d\n", veh->x, veh->y);
            }
            if (veh->is_probe() && sq->owner == faction
            && stack_search(veh->x, veh->y, faction, ST_NeutralOnly, WMODE_INFOWAR)) {
                mapnodes.insert({veh->x, veh->y, NODE_COMBAT_PATROL});
                debug("capture_probe %2d %2d\n", veh->x, veh->y);
            }
        }
    }
    TileSearch ts;
    PointList main_bases;
    bool enemy = false;
    for (int i=0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (!(sq = mapsq(base->x, base->y))) {
            continue;
        }
        if (base->faction_id == faction) {
            adjust_value(pm_former, base->x, base->y, 2, base->pop_size);
            pm_safety[base->x][base->y] += 10000;

            if (sq->veh_owner() < 0) { // Undefended base
                mapnodes.insert({base->x, base->y, NODE_PATROL});
            }
            if (sq->region == p.main_region) {
                main_bases.push_back({base->x, base->y});
            }
            if (sq->region < MaxRegionLandNum) {
                int j = 0;
                int k = 0;
                ts.init(base->x, base->y, TS_TERRITORY_LAND);
                while (++j <= 150 && k < 5 && (sq = ts.get_next()) != NULL) {
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
                enemy = true;
            } else if (has_pact(faction, base->faction_id)) {
                pm_safety[base->x][base->y] += 5000;
            }
            if (allow_probe(faction, base->faction_id, true)) {
                region_flags[sq->region] |= PM_PROBE;
            }
        }
    }
    if (f.base_count > 0 && enemy && mode != UM_Player) {
        int roads[MaxPlayerNum] = {};
        int total = 0;
        int max_dist = clamp(f.base_count, 10, 30);
        ts.init(main_bases, TS_TRIAD_LAND, 1);
        while ((sq = ts.get_next()) != NULL && ts.dist < max_dist && total < 4) {
            if (sq->is_base() && at_war(faction, sq->owner)) {
                total++;
                if (5*faction_might(faction) > 4*faction_might(sq->owner)
                && roads[sq->owner] < 2) {
                    adjust_route(pm_roads, ts, 1);
                    roads[sq->owner]++;
                }
            }
        }
    }
    if (f.base_count > 0 && mode == UM_Full) {
        int values[MaxBaseNum] = {};
        int sorter[MaxBaseNum] = {};
        int bases = 0;

        for (int i=0; i < *BaseCount; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction) {
                assert(base_enemy_range[i] > 0);
                values[i] = base->pop_size + 10*has_fac_built(FAC_HEADQUARTERS, i)
                    + 10*pm_enemy_near[base->x][base->y] - base_enemy_range[i];
                sorter[bases] = values[i];
                bases++;
                debug("defend_score %4d %s\n", values[i], base->name);
            }
        }
        std::sort(sorter, sorter+bases);
        p.defend_weights = 0;

        for (int i=0; i < *BaseCount; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction) {
                if (values[i] >= sorter[bases*15/16] && values[i] > 0) {
                    base->defend_goal = 5;
                } else if (values[i] >= sorter[bases*7/8]) {
                    base->defend_goal = 4;
                } else if (values[i] >= sorter[bases*3/4]) {
                    base->defend_goal = 3;
                } else if (values[i] >= sorter[bases*1/2]) {
                    base->defend_goal = 2;
                } else {
                    base->defend_goal = 1;
                }
                p.defend_weights += base->defend_goal;
                debug("defend_goal %d %s\n", base->defend_goal, base->name);
            }
        }
    }
    if (DEBUG && mode == UM_Visual) {
        static int k = 0;
        k = (k+1)%6;
        if (k<3) {
            for (int y = 0; y < *MapAreaY; y++) {
                for (int x = y&1; x < *MapAreaX; x+=2) {
                    sq = mapsq(x, y);
                    if (k==0) {
                        pm_overlay[x][y] = base_tile_score(x, y, faction, sq);
                    } else if (k==1) {
                        pm_overlay[x][y] = former_tile_score(x, y, faction, sq);
                    } else if (k==2) {
                        pm_overlay[x][y] = arty_value(x, y);
                    }
                }
            }
        } else if (k==3) {
            memcpy(pm_overlay, pm_safety, sizeof(pm_overlay));
        } else if (k==4) {
            invasion_plan(faction);
            memcpy(pm_overlay, pm_enemy_dist, sizeof(pm_overlay));
        } else if (k==5) {
            memcpy(pm_overlay, pm_roads, sizeof(pm_overlay));
        }
    }
    if (mode != UM_Full) {
        return;
    }
    land_raise_plan(faction);
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

bool has_base_sites(int x, int y, int faction, int triad) {
    assert(valid_player(faction));
    assert(valid_triad(triad));
    MAP* sq;
    int area = 0;
    int bases = 0;
    int i = 0;
    int limit = ((*CurrentTurn * x ^ y) & 7 ? 200 : 800);
    TileSearch ts;
    ts.init(x, y, triad, 2);
    while (++i <= limit && (sq = ts.get_next()) != NULL) {
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
                if (pm_safety[m.x][m.y] < PM_SAFE && ++n > 3) {
                    return true;
                }
            }
        }
    }
    return false;
}

int escape_score(int x, int y, int range, VEH* veh, MAP* sq) {
    return pm_safety[x][y] - 160*pm_target[x][y] - 120*range
        + (sq->items & BIT_MONOLITH && veh->need_monolith() ? 2000 : 0)
        + (sq->items & BIT_BUNKER ? 500 : 0)
        + (sq->items & BIT_FOREST ? 200 : 0)
        + (sq->is_rocky() ? 200 : 0)
        + (mapnodes.count({x, y, NODE_PATROL}) ? 400 : 0);
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
            debug("escape_score %2d %2d -> %2d %2d dist: %d score: %d\n",
                veh->x, veh->y, ts.rx, ts.ry, ts.dist, score);
        }
    }
    if (tx >= 0) {
        debug("escape_move  %2d %2d -> %2d %2d safety: %d score: %d %s\n",
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
    int best_score = escape_score(veh->x, veh->y, 0, veh, sq);
    int score;
    int tx = -1;
    int ty = -1;
    int type = (veh->triad() == TRIAD_SEA ? TS_SEA_AND_SHORE : veh->triad());
    TileSearch ts;
    ts.init(veh->x, veh->y, type, 1);
    while ((sq = ts.get_next()) != NULL && ts.dist <= 20) {
        if (sq->is_base() && (sq->owner == veh->faction_id
        || (ally && has_pact(veh->faction_id, sq->owner) && !random(3)))) {
            debug("move_to_base %2d %2d -> %2d %2d %s\n",
                veh->x, veh->y, ts.rx, ts.ry, veh->name());
            return set_move_to(id, ts.rx, ts.ry);

        } else if (ts.dist < 5 && allow_move(ts.rx, ts.ry, veh->faction_id, veh->triad())
        && (score = escape_score(ts.rx, ts.ry, ts.dist, veh, sq)) > best_score
        && !ts.has_zoc(veh->faction_id)) {
            tx = ts.rx;
            ty = ts.ry;
            best_score = score;
        }
    }
    if (tx >= 0) {
        return set_move_to(id, tx, ty);
    }
    return mod_veh_skip(id);
}

ResType want_convoy(int base_id, int x, int y, int* score) {
    MAP* sq;
    BASE* base = &Bases[base_id];
    ResType choice = RES_NONE;
    *score = 0;

    if ((sq = mapsq(x, y)) && !sq->is_base()
    && (sq->owner == base->faction_id || !sq->is_owned())
    && !mapnodes.count({x, y, NODE_CONVOY})) {
        int N = crop_yield(base->faction_id, base_id, x, y, 0);
        int M = mine_yield(base->faction_id, base_id, x, y, 0);
        int E = energy_yield(base->faction_id, base_id, x, y, 0);

        int Nw = (base->nutrient_surplus < 0 ? 8 : min(8, 2 + base_growth_goal(base_id)))
            - max(0, base->nutrient_surplus - 14);
        int Mw = max(3, (45 - base->mineral_intake_2) / 5);

        int Ns = Nw*N - (M+E)*(M+E)/2;
        int Ms = Mw*M - (N+E)*(N+E)/2;

        if (M > 1 && Ms > *score) {
            choice = RES_MINERAL;
            *score = Ms;
        }
        if (N > 1 && Ns > *score) {
            choice = RES_NUTRIENT;
            *score = Ns;
        }
    }
    return choice;
}

int crawler_move(const int id) {
    MAP* sq;
    VEH* veh = &Vehicles[id];
    assert(Bases[veh->home_base_id].faction_id == veh->faction_id);
    if (veh->home_base_id < 0) {
        return mod_veh_skip(id);
    }
    if (!veh->at_target()) {
        // Move status overrides any waypoint target when the unit is not at target
        if (veh->order == ORDER_CONVOY) {
            veh->order = ORDER_MOVE_TO;
        }
        return VEH_SYNC;
    }
    int best_score = 0;
    ResType best_choice = want_convoy(veh->home_base_id, veh->x, veh->y, &best_score);
    if (best_choice != RES_NONE && (*CurrentTurn + id) % 4) {
        return set_convoy(id, best_choice);
    }
    int i = 0;
    int tx = -1;
    int ty = -1;
    int score = 0;
    TileSearch ts;
    ts.init(veh->x, veh->y, veh->triad());

    while (++i <= 80 && (sq = ts.get_next()) != NULL) {
        if (pm_safety[ts.rx][ts.ry] < PM_SAFE
        || non_ally_in_tile(ts.rx, ts.ry, veh->faction_id)) {
            continue;
        }
        int choice = want_convoy(veh->home_base_id, ts.rx, ts.ry, &score);
        if (choice != RES_NONE && score - ts.dist > best_score) {
            best_score = score - ts.dist;
            tx = ts.rx;
            ty = ts.ry;
            debug("crawl_score %2d %2d res: %2d score: %2d %s\n",
                ts.rx, ts.ry, choice, best_score, Bases[veh->home_base_id].name);
        }
    }
    if (tx >= 0) {
        return set_move_to(id, tx, ty);
    }
    if (best_choice != RES_NONE) {
        return set_convoy(id, best_choice);
    }
    if (!random(3)) {
        return move_to_base(id, false);
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
    int tx = veh->waypoint_1_x;
    int ty = veh->waypoint_1_y;

    if (veh->at_target() || !can_build_base(tx, ty, veh->faction_id, veh->triad())) {
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
    if (!(sq = mapsq(x, y))) {// Invalid map coordinates
        return false;
    }
    if ((y < 3 || y >= *MapAreaY - 3) && *MapAreaTiles >= 1600) {
        return false;
    }
    if (map_is_flat() && (x < 2 || x >= *MapAreaX - 2) && *MapAreaTiles >= 1600) {
        return false;
    }
    if ((sq->is_rocky() && !is_ocean(sq)) || sq->volcano_center()
    || (sq->items & (BIT_BASE_DISALLOWED | BIT_ADVANCED))) {
        return false;
    }
    // Allow base building on smaller maps in owned territory if a new faction is spawning.
    if (both_non_enemy(faction, sq->owner) && Factions[faction].base_count > 0) {
        return false;
    }
    if (non_ally_in_tile(x, y, faction)) {
        return false;
    }
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
    }
    if ((triad == TRIAD_SEA || triad == TRIAD_AIR) && is_ocean_shelf(sq)) {
        return true;
    }
    return false;
}

int base_tile_score(int x, int y, int faction, MAP* sq) {
    const int priority[][2] = {
        {BIT_FUNGUS, -2},
        {BIT_FARM, 2},
        {BIT_FOREST, 2},
        {BIT_MONOLITH, 4},
    };
    bool sea_colony = is_ocean(sq);
    int score = min(20, 40 - 80*abs(*MapAreaY/2 - y)/(*MapAreaY));
    int land = 0;
    score += (sq->items & BIT_SENSOR ? 8 : 0);
    if (!sea_colony) {
        score += (ocean_coast_tiles(x, y) ? 12 : 0);
        score += (sq->items & BIT_RIVER ? 5 : 0);
    }
    for (const auto& m : iterate_tiles(x, y, 1, 21)) {
        assert(map_range(x, y, m.x, m.y) == 1 + (m.i > 8));
        if (!m.sq->is_base()) {
            score += (bonus_at(m.x, m.y) ? 6 : 0);
            if (m.sq->landmarks & ~(LM_DUNES|LM_SARGASSO|LM_UNITY)) {
                score += (m.sq->landmarks & LM_JUNGLE ? 3 : 2);
            }
            if (m.i <= 8) { // Only adjacent tiles
                if (sea_colony && m.sq->is_land_region()
                && Continents[m.sq->region].tile_count >= 20
                && (!m.sq->is_owned() || m.sq->owner == faction) && ++land < 3) {
                    score += (!m.sq->is_owned() ? 20 : 4);
                }
                if (is_ocean_shelf(m.sq)) {
                    score += (sea_colony ? 3 : 2);
                }
            }
            if (sea_colony != is_ocean(m.sq)
            && both_non_enemy(faction, m.sq->owner)) {
                score -= 5;
            }
            if (!is_ocean(m.sq)) {
                if (m.sq->is_rainy()) {
                    score += 2;
                }
                if (m.sq->is_moist() && m.sq->is_rolling()) {
                    score += 2;
                }
                if (m.sq->items & BIT_RIVER) {
                    score++;
                }
            }
            for (const int* p : priority) {
                if (m.sq->items & p[0]) score += p[1];
            }
        }
    }
    return score + min(0, (int)pm_safety[x][y]);
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
    if (pm_safety[veh->x][veh->y] < PM_SAFE && !veh->plr_owner()) {
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
            if (allow_civ_move(m.x, m.y, faction, triad)
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
        connect_roads(veh->waypoint_1_x, veh->waypoint_1_y, faction);
        return VEH_SYNC;
    }
    bool full_search = (*CurrentTurn + id) % 4 == 0;
    int limit = full_search ? 800 : 250;
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
    while (++i <= limit && k < 50 && (sq = ts.get_next()) != NULL) {
        if (mapnodes.count({ts.rx, ts.ry, NODE_BASE_SITE})
        || !can_build_base(ts.rx, ts.ry, faction, triad)
        || !safe_path(ts, faction)) {
            continue;
        }
        int score = base_tile_score(ts.rx, ts.ry, faction, sq) - 2*ts.dist;
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
        connect_roads(tx, ty, faction);
        return set_move_to(id, tx, ty);

    } else if (full_search && !veh->plr_owner() && *CurrentTurn > 20 + game_start_turn()) {
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
    if (is_ocean(sq) || is_human(faction)
    || !has_terra(faction, FORMER_RAISE_LAND, LAND)) {
        return false;
    }
    if (is_shore_level(sq) && mapnodes.count({x, y, NODE_GOAL_RAISE_LAND})) {
        return true;
    }
    if (coast_tiles(x, y) && sq->is_base_radius() && (x&1) && (y&2)) {
        for (auto& m : iterate_tiles(x, y, 1, 9)) {
            if (m.sq->region > MaxRegionLandNum
            && !m.sq->is_pole_tile()
            && Continents[m.sq->region].tile_count < 6) {
                return true;
            }
        }
    }
    if (coast_tiles(x, y) < 3) {
        return false;
    }
    const int range = 4;
    TileSearch ts;
    ts.init(x, y, TRIAD_LAND);
    int i = 0;
    while (++i <= 120 && ts.get_next() != NULL);

    int n = 0;
    for (auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        if (!is_ocean(m.sq) && m.y > 0 && m.y < *MapAreaY-1
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
    if (!has_terra(faction, FORMER_THERMAL_BORE, is_ocean(sq))) {
        return false;
    }
    if (sq->items & (BIT_BASE_IN_TILE | BIT_MONOLITH | BIT_THERMAL_BORE) || bonus == RES_NUTRIENT) {
        return false;
    }
    if (bonus == RES_NONE && sq->is_rolling() && sq->items & BIT_CONDENSER) {
        return false;
    }
    if (pm_former[x][y] < 4 && !mapnodes.count({x, y, NODE_BOREHOLE})) {
        return false;
    }
    int level = sq->alt_level();
    for (auto& m : iterate_tiles(x, y, 1, 9)) {
        if (m.sq->items & BIT_THERMAL_BORE || mapnodes.count({m.x, m.y, NODE_BOREHOLE})) {
            return false;
        }
        int level2 = m.sq->alt_level();
        if (level2 < level && level2 > ALT_OCEAN_SHELF) {
            return false;
        }
    }
    if (is_human(faction) && !(*GamePreferences & PREF_AUTO_FORMER_BUILD_ADV)) {
        return false;
    }
    return true;
}

bool can_farm(int x, int y, int faction, int bonus, MAP* sq) {
    bool has_nut = has_tech(Rules->tech_preq_allow_3_nutrients_sq, faction);
    bool sea = is_ocean(sq);
    if (!has_terra(faction, FORMER_FARM, sea) || sq->is_rocky() || sq->items & BIT_THERMAL_BORE) {
        return false;
    }
    if (bonus == RES_NUTRIENT && ~sq->items & BIT_FOREST
    && (sea || sq->is_rainy_or_moist() || sq->is_rolling())) {
        return true;
    }
    if (bonus == RES_ENERGY || bonus == RES_MINERAL || sq->landmarks & LM_VOLCANO) {
        return false;
    }
    if (!has_nut && bonus != RES_NUTRIENT && crop_yield(faction, -1, x, y, 0) >= 2) {
        return false;
    }
    return (sq->is_rolling()
        + sq->is_rainy_or_moist()
        + (nearby_items(x, y, 1, BIT_FARM | BIT_CONDENSER) < 2)
        + (sq->items & (BIT_FARM | BIT_CONDENSER) ? 1 : 0)
        + (sq->items & (BIT_FOREST) ? 0 : 2)
        + (sq->landmarks & LM_JUNGLE ? 0 : 1) > 4);
}

bool can_solar(int x, int y, int faction, int bonus, MAP* sq) {
    bool sea = is_ocean(sq);
    if (!has_terra(faction, FORMER_SOLAR, sea) || bonus == RES_MINERAL) {
        return false;
    }
    if (sq->is_rocky() && bonus != RES_ENERGY) {
        return false;
    }
    if (!has_tech(Rules->tech_preq_allow_3_energy_sq, faction)
    && bonus != RES_ENERGY && energy_yield(faction, -1, x, y, 0) >= 2) {
        return false;
    }
    if (!sea && has_terra(faction, FORMER_FOREST, sea) && ResInfo->forest_sq_energy > 0
    && !(sq->is_rocky() && bonus == RES_ENERGY && sq->alt_level() > ALT_TWO_ABOVE_SEA)
    && (sq->landmarks & LM_JUNGLE
    || (sq->is_rainy() + sq->is_rolling() + sq->is_rainy_or_moist()
    + (sq->items & BIT_FARM ? 1 : 0) < 3))) {
        return false;
    }
    if (sq->items & BIT_SENSOR && nearby_items(x, y, 1, BIT_SENSOR) < 2) {
        return false;
    }
    return !(sq->items & (BIT_MINE | BIT_FOREST | BIT_SOLAR | BIT_ADVANCED));
}

bool can_mine(int x, int y, int faction, int bonus, MAP* sq) {
    bool sea = is_ocean(sq);
    if (!has_terra(faction, FORMER_MINE, sea) || bonus == RES_NUTRIENT) {
        return false;
    }
    if (!sea && !sq->is_rocky()) {
        return false;
    }
    if (!has_tech(Rules->tech_preq_allow_3_minerals_sq, faction)
    && bonus != RES_MINERAL && mine_yield(faction, -1, x, y, 0) >= 2) {
        return false;
    }
    if (sq->items & BIT_SENSOR && nearby_items(x, y, 1, BIT_SENSOR) < 2) {
        return false;
    }
    return !(sq->items & (BIT_MINE | BIT_FOREST | BIT_SOLAR | BIT_ADVANCED));
}

bool can_forest(int x, int y, int faction, MAP* sq) {
    if (!has_terra(faction, FORMER_FOREST, is_ocean(sq))) {
        return false;
    }
    if (sq->is_rocky() || sq->landmarks & LM_VOLCANO) {
        return false;
    }
    if (!has_tech(Rules->tech_preq_allow_3_nutrients_sq, faction)
    && (sq->is_rolling() || sq->items & BIT_SOLAR)
    && crop_yield(faction, -1, x, y, 0) >= 2) {
        return false;
    }
    if (is_human(faction) && !(*GamePreferences & PREF_AUTO_FORMER_PLANT_FORESTS)) {
        return false;
    }
    return !(sq->items & BIT_FOREST);
}

bool can_sensor(int x, int y, int faction, MAP* sq) {
    if (!has_terra(faction, FORMER_SENSOR, is_ocean(sq))) {
        return false;
    }
    if (sq->items & (BIT_MINE | BIT_SOLAR | BIT_SENSOR | BIT_ADVANCED)) {
        return false;
    }
    if (sq->items & BIT_FUNGUS && !has_tech(Rules->tech_preq_improv_fungus, faction)) {
        return false;
    }
    int i = 1;
    for (auto& m : iterate_tiles(x, y, 1, 25)) {
        if (m.sq->owner == faction && (m.sq->items & BIT_SENSOR
        || mapnodes.count({m.x, m.y, NODE_SENSOR_ARRAY}))) {
            return false;
        }
        i++;
    }
    if (is_human(faction) && !(*GameMorePreferences & MPREF_AUTO_FORMER_BUILD_SENSORS)) {
        return false;
    }
    return true;
}

bool keep_fungus(int x, int y, int faction, MAP* sq) {
    return plans[faction].keep_fungus
        && !(sq->items & (BIT_BASE_IN_TILE | BIT_MONOLITH))
        && nearby_items(x, y, 1, BIT_FUNGUS)
        < (sq->items & BIT_FUNGUS ? 1 : 0) + plans[faction].keep_fungus;
}

bool plant_fungus(int x, int y, int faction, MAP* sq) {
    return plans[faction].plant_fungus
        && keep_fungus(x, y, faction, sq)
        && has_terra(faction, FORMER_PLANT_FUNGUS, is_ocean(sq));
}

bool can_level(int x, int y, int faction, int bonus, MAP* sq) {
    return sq->is_rocky() && has_terra(faction, FORMER_LEVEL_TERRAIN, is_ocean(sq))
        && (bonus == RES_NUTRIENT || (bonus == RES_NONE
        && !(sq->items & (BIT_MINE|BIT_FUNGUS|BIT_THERMAL_BORE))
        && sq->items & BIT_RIVER
        && !plans[faction].plant_fungus
        && nearby_items(x, y, 1, BIT_FARM|BIT_FOREST) < 2));
}

bool can_river(int x, int y, int faction, MAP* sq) {
    if (is_ocean(sq) || !has_terra(faction, FORMER_AQUIFER, LAND)) {
        return false;
    }
    if (sq->items & (BIT_BASE_IN_TILE | BIT_RIVER | BIT_THERMAL_BORE)) {
        return false;
    }
    return !(((*CurrentTurn / 4 * x) ^ y) & 15)
        && !coast_tiles(x, y)
        && nearby_items(x, y, 1, BIT_RIVER) < 2
        && nearby_items(x, y, 2, BIT_RIVER) < 6
        && nearby_items(x, y, 1, BIT_THERMAL_BORE) < 2;
}

bool can_road(int x, int y, int faction, MAP* sq) {
    if (!has_terra(faction, FORMER_ROAD, is_ocean(sq))
    || sq->items & (BIT_ROAD | BIT_BASE_IN_TILE)) {
        return false;
    }
    if (!sq->is_base_radius() && pm_roads[x][y] < 1) {
        return false;
    }
    if (sq->items & BIT_FUNGUS && (!has_tech(Rules->tech_preq_build_road_fungus, faction)
    || (!build_tubes && has_project(FAC_XENOEMPATHY_DOME, faction)))) {
        return false;
    }
    if (is_human(faction) && *GameMorePreferences & MPREF_AUTO_FORMER_CANT_BUILD_ROADS) {
        return false;
    }
    if (sq->owner != faction) {
        return pm_roads[x][y] > 0 && !both_neutral(faction, sq->owner);
    }
    if (mapnodes.count({x, y, NODE_GOAL_RAISE_LAND})) {
        return true;
    }
    if (pm_roads[x][y] > 0 || sq->items & (BIT_MINE | BIT_CONDENSER | BIT_THERMAL_BORE)) {
        return true;
    }
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
    if (!has_terra(faction, FORMER_MAGTUBE, is_ocean(sq))
    || sq->items & (BIT_MAGTUBE | BIT_BASE_IN_TILE)) {
        return false;
    }
    if (both_neutral(faction, sq->owner)) {
        return false;
    }
    if (is_human(faction) && *GameMorePreferences & MPREF_AUTO_FORMER_CANT_BUILD_ROADS) {
        return false;
    }
    return pm_roads[x][y] > 0 && sq->items & BIT_ROAD
        && (~sq->items & BIT_FUNGUS || has_tech(Rules->tech_preq_build_road_fungus, faction));
}

int select_item(int x, int y, int faction, FormerMode mode, MAP* sq) {
    assert(valid_player(faction));
    assert(mapsq(x, y));
    uint32_t items = sq->items;
    bool sea = is_ocean(sq);
    bool road = can_road(x, y, faction, sq);
    bool rem_fungus = has_terra(faction, FORMER_REMOVE_FUNGUS, sea)
        && (!is_human(faction) || *GameMorePreferences & MPREF_AUTO_FORMER_REMOVE_FUNGUS);

    if (sq->is_base() || sq->volcano_center() || (sea && !is_ocean_shelf(sq))) {
        return FORMER_NONE;
    }
    if (mode == FM_Auto_Sensors) {
        if (can_sensor(x, y, faction, sq)) {
            return FORMER_SENSOR;
        }
        return FORMER_NONE;
    }
    if (mode == FM_Remove_Fungus) {
        if (items & BIT_FUNGUS && rem_fungus) {
            return FORMER_REMOVE_FUNGUS;
        }
        return FORMER_NONE;
    }
    if ((mode == FM_Auto_Full || mode == FM_Auto_Tubes) && can_magtube(x, y, faction, sq)) {
        return FORMER_MAGTUBE;
    }
    if (mode == FM_Auto_Full && sq->owner == faction && can_bridge(x, y, faction, sq)) {
        if (mapnodes.count({x, y, NODE_RAISE_LAND})
        || terraform_cost(x, y, faction) < Factions[faction].energy_credits/8) {
            return (road ? FORMER_ROAD : FORMER_RAISE_LAND);
        }
    }
    if (road || sq->owner != faction || !sq->is_base_radius() || items & BIT_MONOLITH) {
        return (road ? FORMER_ROAD : FORMER_NONE);
    }
    if (mode != FM_Auto_Full) {
        return FORMER_NONE;
    }
    if (can_river(x, y, faction, sq)) {
        return FORMER_AQUIFER;
    }
    int bonus = bonus_at(x, y);
    int current = total_yield(x, y, faction);

    bool forest = has_terra(faction, FORMER_FOREST, sea) && ResInfo->forest_sq_energy > 0;
    bool borehole = has_terra(faction, FORMER_THERMAL_BORE, sea) && ResInfo->borehole_sq_energy > 2;
    bool condenser = has_terra(faction, FORMER_CONDENSER, sea);
    bool use_sensor = items & BIT_SENSOR && nearby_items(x, y, 1, BIT_SENSOR) < 2;
    bool allow_farm = items & BIT_FARM || can_farm(x, y, faction, bonus, sq);
    bool allow_forest = items & BIT_FOREST || can_forest(x, y, faction, sq);
    bool allow_fungus = items & BIT_FUNGUS || plant_fungus(x, y, faction, sq);
    bool allow_borehole = items & BIT_THERMAL_BORE || can_borehole(x, y, faction, bonus, sq);

    int farm_val = (sea && allow_farm ? item_yield(x, y, faction, bonus, BIT_FARM) : 0);
    int forest_val = (allow_forest ? item_yield(x, y, faction, bonus, BIT_FOREST) : 0);
    int fungus_val = (allow_fungus ? item_yield(x, y, faction, bonus, BIT_FUNGUS) : 0);
    int borehole_val = (allow_borehole ? item_yield(x, y, faction, bonus, BIT_THERMAL_BORE) : 0);
    int max_val = 0;
    int skip_val = (current > 7);
    int bonus_val = (bonus == RES_NUTRIENT) + skip_val
        + (~sq->items & BIT_CONDENSER && allow_farm && sq->is_rainy_or_moist() && sq->is_rolling());

    for (int v : {farm_val, forest_val, fungus_val, borehole_val}) {
        max_val = max(v, max_val);
        assert(current >= 0 && v >= 0);
    }
    if (farm_val == max_val && sea && max_val > 0) {
        if (items & BIT_FUNGUS) {
            return (rem_fungus ? FORMER_REMOVE_FUNGUS : FORMER_NONE);
        }
        if (farm_val > current && allow_farm && ~items & BIT_FARM) {
            return FORMER_FARM;
        }
    }
    if (forest_val == max_val && max_val > 0) {
        if (items & BIT_FUNGUS) {
            return (rem_fungus ? FORMER_REMOVE_FUNGUS : FORMER_NONE);
        }
        if (items & BIT_FOREST) {
            return (can_sensor(x, y, faction, sq) ? FORMER_SENSOR : FORMER_NONE);
        }
        if (forest_val > current + bonus_val && allow_forest) {
            return FORMER_FOREST;
        }
    }
    if (fungus_val == max_val && max_val > 0) {
        if (items & BIT_FUNGUS) {
            return (can_sensor(x, y, faction, sq) ? FORMER_SENSOR : FORMER_NONE);
        }
        if (fungus_val > current + (bonus != RES_NONE) && allow_fungus) {
            return FORMER_PLANT_FUNGUS;
        }
    }
    if (borehole_val == max_val && max_val > 0) {
        if (items & BIT_FUNGUS) {
            return (rem_fungus ? FORMER_REMOVE_FUNGUS : FORMER_NONE);
        }
        if (items & BIT_THERMAL_BORE) {
            return FORMER_NONE;
        }
        if (borehole_val > current && allow_borehole) {
            return FORMER_THERMAL_BORE;
        }
    }
    if (items & BIT_FUNGUS) {
        if (keep_fungus(x, y, faction, sq)) {
            return (can_sensor(x, y, faction, sq) ? FORMER_SENSOR : FORMER_NONE);
        }
        return (rem_fungus ? FORMER_REMOVE_FUNGUS : FORMER_NONE);
    }
    if (can_level(x, y, faction, bonus, sq)) {
        return FORMER_LEVEL_TERRAIN;
    }
    if (sea && bonus == RES_NONE && can_sensor(x, y, faction, sq)) {
        return FORMER_SENSOR;
    }

    int solar_need = (condenser ? 0 : 1) + (forest ? 0 : 2) + (borehole ? 0 : 3)
        + 2*max(0, sq->alt_level()-ALT_ONE_ABOVE_SEA) - nearby_items(x, y, 2, BIT_SOLAR);
    if (sea) {
        solar_need = (has_terra(faction, FORMER_MINE, sea) ?
            (ResInfo->improved_sea_energy - ResInfo->improved_sea_mineral)
            - has_tech(Rules->tech_preq_mining_platform_bonus, faction): 6)
            + nearby_items(x, y, 2, BIT_MINE) - nearby_items(x, y, 2, BIT_SOLAR);
    }

    if (can_solar(x, y, faction, bonus, sq) && solar_need > 0) {
        if (allow_farm && ~items & BIT_FARM) {
            return FORMER_FARM;
        }
        return FORMER_SOLAR;
    }
    if (can_mine(x, y, faction, bonus, sq)) {
        if (sea && allow_farm && ~items & BIT_FARM) {
            return FORMER_FARM;
        }
        return FORMER_MINE;
    }
    if (items & BIT_SOLAR && solar_need >= 0) {
        return FORMER_NONE;
    }
    if (allow_farm && ~items & BIT_FARM) {
        return FORMER_FARM;
    }
    if (!use_sensor && items & BIT_FARM && ~items & BIT_CONDENSER
    && has_terra(faction, FORMER_CONDENSER, sea)
    && (!is_human(faction) || *GamePreferences & PREF_AUTO_FORMER_BUILD_ADV)) {
        return FORMER_CONDENSER;
    }
    if (!use_sensor && items & BIT_FARM && ~items & BIT_SOIL_ENRICHER
    && has_terra(faction, FORMER_SOIL_ENR, sea)) {
        return FORMER_SOIL_ENR;
    }
    if (can_sensor(x, y, faction, sq)) {
        return FORMER_SENSOR;
    }
    if (forest_val > current + skip_val && can_forest(x, y, faction, sq)) {
        return FORMER_FOREST;
    }
    return FORMER_NONE;
}

int former_tile_score(int x, int y, int faction, MAP* sq) {
    const int priority[][2] = {
        {BIT_RIVER, 4},
        {BIT_FARM, -3},
        {BIT_SOLAR, -2},
        {BIT_FOREST, -4},
        {BIT_MINE, -4},
        {BIT_CONDENSER, -4},
        {BIT_SOIL_ENRICHER, -4},
        {BIT_THERMAL_BORE, -8},
    };
    int bonus = bonus_at(x, y);
    int score = (sq->landmarks & ~(LM_DUNES|LM_SARGASSO|LM_UNITY|LM_NEXUS) ? 4 : 0);
    uint32_t items = sq->items;

    if (bonus != RES_NONE && !(items & BIT_ADVANCED)) {
        score += ((items & BIT_SIMPLE) ? 3 : 5) * (bonus == RES_NUTRIENT ? 3 : 2);
    }
    for (const int* p : priority) {
        if (items & p[0]) {
            score += p[1];
        }
    }
    if (items & BIT_FUNGUS) {
        score += (items & BIT_ADVANCED ? 20 : 0);
        score += (plans[faction].keep_fungus ? -10 : -2);
        score += (plans[faction].plant_fungus && (items & BIT_ROAD) ? -8 : 0);
    } else if (plans[faction].plant_fungus) {
        score += 8;
    }
    if (items & (BIT_FOREST | BIT_SENSOR) && can_road(x, y, faction, sq)) {
        score += 8;
    }
    if (pm_roads[x][y] > 0 && (~items & BIT_ROAD || (build_tubes && ~items & BIT_MAGTUBE))) {
        score += 12;
    }
    if (is_shore_level(sq) && mapnodes.count({x, y, NODE_GOAL_RAISE_LAND})) {
        score += 20;
    }
    return score + min(8, (int)pm_former[x][y]) + min(0, (int)pm_safety[x][y]);
}

int former_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    bool safe = pm_safety[veh->x][veh->y] >= PM_SAFE;
    int faction = veh->faction_id;
    int item;
    int choice;
    FormerMode mode = FM_Auto_Full;
    if (sq->owner != faction && pm_roads[veh->x][veh->y] < 1) {
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
            if (allow_civ_move(m.x, m.y, faction, TRIAD_LAND) && !random(2)) {
                debug("former_trans %2d %2d -> %2d %2d\n", veh->x, veh->y, m.x, m.y);
                return set_move_to(id, m.x, m.y);
            }
        }
        return mod_veh_skip(id);
    }
    if (veh->plr_owner()) {
        if (veh->order_auto_type == ORDERA_TERRA_AUTO_MAGTUBE
        && has_terra(faction, FORMER_MAGTUBE, is_ocean(sq))) {
            mode = FM_Auto_Tubes;
        }
        else if (veh->order_auto_type == ORDERA_TERRA_AUTO_ROAD
        && has_terra(faction, FORMER_ROAD, is_ocean(sq))) {
            mode = FM_Auto_Roads;
        }
        else if (veh->order_auto_type == ORDERA_TERRA_AUTO_SENSOR
        && has_terra(faction, FORMER_SENSOR, is_ocean(sq))) {
            mode = FM_Auto_Sensors;
        }
        else if (veh->order_auto_type == ORDERA_TERRA_AUTO_FUNGUS_REM
        && has_terra(faction, FORMER_REMOVE_FUNGUS, is_ocean(sq))) {
            mode = FM_Remove_Fungus;
        }
    }
    int turns = (veh->order >= ORDER_FARM && veh->order < ORDER_MOVE_TO ?
        Terraform[veh->order - 4].rate : 0);

    if (safe || turns >= 12 || veh->plr_owner()) {
        if (turns > 0 && !(veh->order == ORDER_DRILL_AQUIFER
        && nearby_items(veh->x, veh->y, 1, BIT_RIVER) >= 4)) {
            return VEH_SYNC;
        }
        if (!veh->at_target() && !can_road(veh->x, veh->y, faction, sq)
        && !can_magtube(veh->x, veh->y, faction, sq)) {
            return VEH_SYNC;
        }
        item = select_item(veh->x, veh->y, faction, mode, sq);
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
    bool home_base_only = false;
    bool full_search = (*CurrentTurn + id) % 4 == 0;
    int limit = full_search ? 320 : 80;
    int best_score = INT_MIN;
    int score;
    int i = 0;
    int tx = -1;
    int ty = -1;
    int bx = veh->x;
    int by = veh->y;
    if (veh->home_base_id >= 0 && Bases[veh->home_base_id].faction_id == faction) {
        bx = Bases[veh->home_base_id].x;
        by = Bases[veh->home_base_id].y;
        if (veh->plr_owner()
        && veh->order_auto_type == ORDERA_TERRA_AUTOIMPROVE_BASE
        && region_at(veh->x, veh->y) == region_at(bx, by)) {
            home_base_only = true;
        }
    }
    TileSearch ts;
    ts.init(veh->x, veh->y, veh->triad());

    while (++i <= limit && (sq = ts.get_next()) != NULL) {
        if (sq->is_base()
        || (sq->owner != faction && pm_roads[ts.rx][ts.ry] < 1)
        || (home_base_only && map_range(bx, by, ts.rx, ts.ry) > 2)
        || (pm_former[ts.rx][ts.ry] < 1 && pm_roads[ts.rx][ts.ry] < 1)
        || pm_safety[ts.rx][ts.ry] < PM_SAFE
        || non_ally_in_tile(ts.rx, ts.ry, faction)) {
            continue;
        }
        if (mode == FM_Auto_Full) {
            score = former_tile_score(ts.rx, ts.ry, faction, sq)
                - map_range(bx, by, ts.rx, ts.ry)/2;
        } else {
            score = former_tile_score(ts.rx, ts.ry, faction, sq)
                - 2*map_range(veh->x, veh->y, ts.rx, ts.ry);
        }
        if (score > best_score && (choice = select_item(ts.rx, ts.ry, faction, mode, sq)) >= 0) {
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
    }

    debug("former_skip %2d %2d %s\n", veh->x, veh->y, veh->name());
    if (!full_search || veh->plr_owner()) {
        if (region_at(veh->x, veh->y) == region_at(bx, by) && !(veh->x == bx && veh->y == by)) {
            return set_move_to(id, bx, by);
        }
        return move_to_base(id, false);

    } else if (full_search && *CurrentTurn > 20 + game_start_turn()) {
        return mod_veh_kill(id);
    }
    return mod_veh_skip(id);
}

/*
Combat unit movement code starts here.
*/
bool allow_scout(int faction, MAP* sq) {
    return !sq->is_visible(faction)
        && sq->region != plans[faction].target_land_region
        && !sq->is_pole_tile()
        && (((!sq->is_owned() || sq->owner == faction) && !random(8))
        || (sq->is_owned() && sq->owner != faction
        && !has_treaty(faction, sq->owner, DIPLO_COMMLINK)));
}

bool allow_probe(int faction1, int faction2, bool is_enhanced) {
    int diplo = Factions[faction1].diplo_status[faction2];
    if (faction1 != faction2) {
        if (!(diplo & DIPLO_COMMLINK)) {
            return true;
        }
        if (!is_enhanced && has_project(FAC_HUNTER_SEEKER_ALGORITHM, faction2)) {
            return false;
        }
        if (at_war(faction1, faction2)) {
            return true;
        }
        if (!(diplo & DIPLO_PACT)) {
            int value = 0;
            if (diplo & DIPLO_TREATY)
                value--;
            if (plans[faction1].mil_strength*3 < 2*plans[faction2].mil_strength)
                value--;
            if (Factions[faction1].tech_ranking < Factions[faction2].tech_ranking)
                value += 2;
            if (Factions[faction1].AI_fight > 0)
                value++;
            if (Factions[faction1].AI_power > 0)
                value++;
            if (Factions[faction2].integrity_blemishes > 0)
                value++;
            if (*GameRules & RULES_INTENSE_RIVALRY && is_human(faction2))
                value += 2;
            return value > 2;
        }
    }
    return false;
}

bool allow_attack(int faction1, int faction2, bool is_probe, bool is_enhanced) {
    if (is_probe) {
        return allow_probe(faction1, faction2, is_enhanced);
    }
    return at_war(faction1, faction2);
}

bool allow_conv_missile(int veh_id, int enemy_veh_id, MAP* sq) {
    VEH* veh = &Vehicles[veh_id];
    VEH* enemy = &Vehicles[enemy_veh_id];
    if ((!enemy->is_combat_unit() && !enemy->is_probe()) || enemy->high_damage()) {
        return false;
    }
    if (!enemy->faction_id) {
        // Native life can be attacked but only rarely
        return sq->owner == veh->faction_id
            && !plans[veh->faction_id].enemy_factions
            && mod_morale_alien(enemy_veh_id, veh->faction_id) > random(32);
    }
    if (!enemy->is_combat_unit() && !enemy->is_armored()) {
        // Missiles can attack probes inside our own territory
        if (!enemy->is_probe() || sq->owner != veh->faction_id) {
            return false;
        }
    }
    return (sq->is_base() ? 3 : 0)
        + min(4, (int)pm_enemy[enemy->x][enemy->y])
        + min(4, arty_value(enemy->x, enemy->y)/16)
        + min(4, Units[enemy->unit_id].cost/2) >= 6 + random(6);
}

int garrison_goal(int x, int y, int faction, int triad) {
    assert(triad == TRIAD_LAND || triad == TRIAD_SEA);
    AIPlans& p = plans[faction];
    if (triad == TRIAD_LAND && p.transport_units > 0
    && p.naval_start_x == x && p.naval_start_y == y) {
        return clamp(p.land_combat_units/64 + p.transport_units, 4, 12);
    }
    for (int i=0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->x == x && base->y == y) {
            int goal = max(1,
                min(5, (p.land_combat_units + p.sea_combat_units)
                * max(1, (int)base->defend_goal) / max(1, p.defend_weights))
                - (triad == TRIAD_SEA ? 2 : 0)
                - (p.contacted_factions < 2 && p.unknown_factions > 1 ? 1 : 0)
                - (!random(4)));

            debug("garrison_goal %2d %2d %2d %s\n", x, y, goal, base->name);
            return goal;
        }
    }
    assert(false);
    return 0;
}

int garrison_count(int x, int y) {
    int num = 0;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->x == x && veh->y == y
        && veh->is_garrison_unit()
        && veh->order != ORDER_SENTRY_BOARD
        && veh->at_target()) {
            num++;
        }
    }
    return num;
}

int defender_count(int x, int y, int veh_skip_id) {
    int num = 0;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->x == x && veh->y == y
        && veh->triad() != TRIAD_AIR
        && veh->order != ORDER_SENTRY_BOARD
        && veh->at_target()
        && i != veh_skip_id) {
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
Thinker chooses these units randomly based on the Veh ID modulus. Even though
the IDs are not permanent, it is not a problem for the movement planning.
*/
bool invasion_unit(const int id) {
    AIPlans& p = plans[Vehicles[id].faction_id];
    VEH* veh = &Vehicles[id];
    MAP* sq;

    if (p.naval_start_x < 0 || !(sq = mapsq(veh->x, veh->y))) {
        return false;
    }
    if (veh->triad() == TRIAD_AIR) {
        return (id % 16) > (p.prioritize_naval ? 8 : 11);
    }
    if (veh->triad() == TRIAD_SEA) {
        return (id % 16) > (p.prioritize_naval ? 8 : 11)
            && ocean_coast_tiles(veh->x, veh->y);
    }
    return (id % 16) > (p.prioritize_naval ? 8 : 11) + (p.enemy_bases ? 2 : 0)
        && sq->region == p.main_region
        && sq->owner == veh->faction_id;
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

int make_landing(const int id) {
    VEH* veh = &Vehicles[id];
    AIPlans& p = plans[veh->faction_id];
    int x2 = -1;
    int y2 = -1;
    for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
        if (!allow_move(m.x, m.y, veh->faction_id, TRIAD_LAND)) {
            continue;
        }
        if (mapnodes.count({m.x, m.y, NODE_GOAL_NAVAL_BEACH})
        || (p.naval_end_x < 0 && !m.sq->is_owned())) {
            x2 = m.x;
            y2 = m.y;
            if (!random(3)) {
                break;
            }
        }
    }
    if (x2 >= 0) {
        return set_move_to(id, x2, y2); // Save Private Ryan
    }
    return VEH_SYNC;
}

int trans_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq;
    AIPlans& p = plans[veh->faction_id];
    if (!(sq = mapsq(veh->x, veh->y))
    || mapnodes.erase({veh->x, veh->y, NODE_NEED_FERRY}) > 0
    || (sq->is_base() && veh->need_heals())) {
        return mod_veh_skip(id);
    }
    int offset;
    int cargo = 0;
    int defenders = 0;
    int capacity = veh_cargo(id);

    for (int k=0; k < *VehCount; k++) {
        VEH* v = &Vehicles[k];
        if (veh->x == v->x && veh->y == v->y && k != id && v->triad() == TRIAD_LAND) {
            if (v->order == ORDER_SENTRY_BOARD && v->waypoint_1_x == id) {
                cargo++;
            } else if (v->is_combat_unit() || v->is_probe()) {
                defenders++;
            }
        }
    }
    debug("trans_move %2d %2d defenders: %d cargo: %d capacity: %d %s\n",
        veh->x, veh->y, defenders, cargo, veh_cargo(id), veh->name());

    if (is_ocean(sq)) {
        if (invasion_unit(id) && sq->region != p.main_sea_region
        && (offset = path_get_next(veh->x, veh->y, p.naval_start_x, p.naval_start_y,
        veh->unit_id, veh->faction_id)) > 0) {
            return set_move_next(id, offset);
        }
        if (cargo > 0 && near_landing(veh->x, veh->y, veh->faction_id)) {
            for (int k=0; k < *VehCount; k++) {
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
        for (int k=0; k < *VehCount; k++) {
            if (cargo >= capacity) {
                break;
            }
            VEH* v = &Vehicles[k];
            if (veh->faction_id == v->faction_id
            && veh->x == v->x && veh->y == v->y
            && (v->is_combat_unit() || v->is_probe())
            && v->triad() == TRIAD_LAND && v->order != ORDER_SENTRY_BOARD) {
                set_board_to(k, id);
                cargo++;
            }
        }
        if (cargo >= min(4, capacity)) {
            return set_move_to(id, p.naval_end_x, p.naval_end_y);
        }
        if (veh->iter_count < 4) {
            return VEH_SYNC;
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
        return VEH_SYNC;
    }
    int max_dist = ((*CurrentTurn + id) % 4 ? 7 : 16);
    int tx = -1;
    int ty = -1;
    TileSearch ts;
    ts.init(veh->x, veh->y, TS_SEA_AND_SHORE);

    while ((sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        if (allow_civ_move(ts.rx, ts.ry, veh->faction_id, TRIAD_SEA)) {
            bool return_to_base = sq->is_base() && sq->owner == veh->faction_id
                && (veh->need_heals() || (cargo > 0 && p.naval_end_x < 0
                && sq->region == p.main_region));

            if (mapnodes.erase({ts.rx, ts.ry, NODE_PATROL}) > 0
            || mapnodes.erase({ts.rx, ts.ry, NODE_NEED_FERRY}) > 0
            || return_to_base) {
                debug("trans_move %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
                return set_move_to(id, ts.rx, ts.ry);

            } else if (tx < 0 && allow_scout(veh->faction_id, sq)
            && !invasion_unit(id) && random(16) > ts.dist) {
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
    int faction = Vehicles[att_id].faction_id;
    int def_id = -1;
    if (!non_ally_in_tile(x, y, faction)) {
        return -1;
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->x == x && veh->y == y) {
            if (!at_war(faction, veh->faction_id)) {
                return -1;
            }
            def_id = i;
            break;
        }
    }
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
    mod_battle_compute(id1, id2, &s1, &s2, 0);
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
    assert(id1 >= 0 && id1 < *VehCount);
    assert(id2 >= 0 && id2 < *VehCount);
    assert(veh1->faction_id != veh2->faction_id);
    assert(veh1->offense_value() != 0 || veh1->is_probe());
    assert(map_range(veh1->x, veh1->y, veh2->x, veh2->y) > 0);
    UNIT* u1 = &Units[veh1->unit_id];
    UNIT* u2 = &Units[veh2->unit_id];
    bool non_psi = veh1->offense_value() >= 0 && veh2->defense_value() >= 0;
    bool stack_damage = !sq->is_base_or_bunker() && conf.collateral_damage_value > 0;
    bool neutral_tile = both_neutral(veh1->faction_id, sq->owner);
    int triad = veh1->triad();
    int cost = 0;
    int att_moves = 1;
    int mov_rate = 1;

    // Calculate actual movement cost for hasty penalties
    if (triad == TRIAD_LAND && non_psi) {
        cost = path_cost(veh1->x, veh1->y, veh2->x, veh2->y,
            veh1->unit_id, veh1->faction_id, moves);
        if (moves - cost > 0) {
            att_moves = min(moves - cost, Rules->move_rate_roads);
            mov_rate = Rules->move_rate_roads;
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
        + min(12, abs(offense_value(veh2->unit_id))) * (u2->speed() > 1 ? 0.02 : 0.01)
        + (triad == TRIAD_AIR ? 0.001 * min(400, arty_value(veh2->x, veh2->y)) : 0.0)
        - (neutral_tile ? 0.06 : 0.01)*dist;
    /*
    Fix: in rare cases the game engine might reject valid attack orders for unknown reason.
    In this case combat_move would repeat failed attack orders until iteration limit.
    */
    double v3 = min(!offense_value(veh2->unit_id) ? 1.3 : 1.6, v1) + clamp(v2, -0.5, 0.5)
        - 0.04*pm_target[veh2->x][veh2->y];

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
    const bool at_base = sq->is_base();
    const bool missile = u->is_missile();
    const int faction = veh->faction_id;
    const int moves = veh_speed(id, 0) - veh->moves_spent;
    const int max_range = max(0, moves / Rules->move_rate_roads);
    int max_dist = max_range; // can be modified during search

    if (!veh->at_target()) {
        return VEH_SYNC;
    }
    // Needlejets may end the turn outside base, in that case they have to refuel
    if (u->chassis_id == CHS_NEEDLEJET) {
        if (veh->iter_count == 0 && ++veh->iter_count && !at_base) {
            return move_to_base(id, true);
        }
    }
    if (u->chassis_id == CHS_COPTER && !at_base && veh->mid_damage()) {
        max_dist /= (veh->high_damage() ? 4 : 2);
    }
    if (at_base && veh->mid_damage() && !missile) {
        max_dist /= 2;
    }
    int i = 0;
    int tx = -1;
    int ty = -1;
    int bx = -1;
    int by = -1;
    int id2 = -1;
    double best_odds = 1.2 - 0.004 * min(100, p.air_combat_units);
    TileSearch ts;
    ts.init(veh->x, veh->y, TRIAD_AIR);

    while (++i < 625 && (sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        if ((id2 = choose_defender(ts.rx, ts.ry, id, sq)) >= 0) {
            VEH* veh2 = &Vehicles[id2];
            if (!sq->is_base() && veh2->chassis_type() == CHS_NEEDLEJET
            && !has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)) {
                continue;
            }
            if (missile && bx < 0 && ts.dist == 1) {
                bx = ts.rx;
                by = ts.ry;
            }
            if (missile && !allow_conv_missile(id, id2, sq)) {
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
        }
        else if (!missile && tx < 0 && mapnodes.count({ts.rx, ts.ry, NODE_COMBAT_PATROL})
        && ts.dist + 4*veh->mid_damage() + 8*veh->high_damage() < random(16)) {
            return set_move_to(id, ts.rx, ts.ry);
        }
        else if (missile || non_ally_in_tile(ts.rx, ts.ry, faction) || veh->high_damage()) {
            continue;
        }
        else if (tx < 0 && allow_scout(faction, sq)) {
            return set_move_to(id, ts.rx, ts.ry);
        }
    }
    if (!at_base && tx < 0 && bx >= 0 && max_range <= 1) {
        // Adjacent tile backup attack choice
        debug("aircraft_change %2d %2d -> %2d %2d\n", veh->x, veh->y, bx, by);
        return set_move_to(id, bx, by);
    }
    if (tx >= 0) {
        int range = map_range(veh->x, veh->y, tx, ty);
        if (range > 1 && (!missile || veh->moves_spent)) {
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
    if (at_base && veh->mid_damage()) {
        return mod_veh_skip(id);
    }
    bool move_naval = p.naval_airbase_x >= 0 && invasion_unit(id)
        && map_range(veh->x, veh->y, p.naval_airbase_x, p.naval_airbase_y) >= 20;
    bool move_other = (*CurrentTurn + id) % 8 < 3;
    int score;
    int best_score = INT_MIN;
    // Missiles have to always end their turn inside base
    max_dist = max_range * (missile || veh->mid_damage() ? 1 : 2);

    if (move_naval && !missile && (u->chassis_id != CHS_COPTER || !veh->mid_damage())) {
        debug("aircraft_invade %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
        return set_move_to(id, tx, ty);
    }
    for (i = 0; i < *BaseCount && (move_naval || move_other); i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction || has_pact(faction, base->faction_id)) {
            int base_value = clamp((base->faction_id == faction ? base->defend_goal : 2), 1, 4);
            int base_range = map_range(veh->x, veh->y, base->x, base->y);
            if (base_range > max_dist) {
                continue;
            }
            if (move_naval) {
                score = random(8) + min(6, arty_value(base->x, base->y)/8)
                    - map_range(base->x, base->y, p.naval_airbase_x, p.naval_airbase_y)
                    * (base->faction_id == faction ? 1 : 2);
            } else {
                score = random(8) + base_value * (missile ? 4 : 8)
                    + min(6, arty_value(base->x, base->y)/8)
                    - base_range * (base->faction_id == faction ? 1 : 2);
            }
            if (score > best_score) {
                tx = base->x;
                ty = base->y;
                best_score = score;
            }
        }
    }
    if (tx >= 0 && !(veh->x == tx && veh->y == ty)) {
        debug("aircraft_rebase %2d %2d -> %2d %2d score: %d\n", veh->x, veh->y, tx, ty, best_score);
        return set_move_to(id, tx, ty);
    }
    if (!at_base && (u->chassis_id != CHS_GRAVSHIP || !random(8))) {
        return move_to_base(id, true);
    }
    return mod_veh_skip(id);
}

bool prevent_airdrop(int x, int y, int faction) {
    for (int i = 0; i < *BaseCount; i++) {
        if (at_war(faction, Bases[i].faction_id)
        && map_range(Bases[i].x, Bases[i].y, x, y) <= 2
        && has_facility(FAC_AEROSPACE_COMPLEX, i)) {
            return true;
        }
    }
    return false;
}

bool airdrop_move(const int id, MAP* sq) {
    VEH* veh = &Vehicles[id];
    if (!has_abil(veh->unit_id, ABL_DROP_POD)
    || veh->state & VSTATE_MADE_AIRDROP
    || veh->moves_spent > 0
    || !sq->is_airbase()) {
        return false;
    }
    int faction = veh->faction_id;
    int max_range = (has_orbital_drops(faction) ? 9 + random(42) :
        Rules->max_airdrop_rng_wo_orbital_insert);
    int tx = -1;
    int ty = -1;
    int best_score = 0;
    int base_range;

    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        bool allow_defend = base->faction_id == faction && (*CurrentTurn + id) & 1;
        bool allow_attack = at_war(faction, base->faction_id);

        if ((allow_defend || allow_attack)
        && (sq = mapsq(base->x, base->y)) && !is_ocean(sq)
        && (base_range = map_range(veh->x, veh->y, base->x, base->y)) <= max_range
        && base_range >= 3) {
            if (allow_defend) {
                int enemy_diff = pm_enemy_near[base->x][base->y]
                    - pm_enemy_near[veh->x][veh->y];
                if (enemy_diff <= 0) {
                    continue;
                }
                // Skips bases reachable by roads or magtubes in less than one turn
                if (path_cost(veh->x, veh->y, base->x, base->y, veh->unit_id, veh->faction_id,
                Rules->move_rate_roads) >= 0) {
                    continue;
                }
                int score = random(4) + enemy_diff
                    + 5*(sq->region == plans[faction].target_land_region)
                    + 2*base->defend_goal
                    - base_range/4
                    - 2*pm_target[base->x][base->y];
                if (score > best_score && !prevent_airdrop(base->x, base->y, faction)) {
                    tx = base->x;
                    ty = base->y;
                    best_score = score;
                }
            }
            else if (allow_attack && veh_stack(base->x, base->y) < 0
            && base_range/4 < 2 + pm_unit_near[base->x][base->y]
            && (base_range < 8 || sq->is_visible(faction))
            && !prevent_airdrop(base->x, base->y, faction)) {
                // Prioritize closest undefended enemy bases
                tx = base->x;
                ty = base->y;
                break;
            }
        }
    }
    if (tx >= 0) {
        debug("airdrop_move %2d %2d -> %2d %2d score: %d %s\n",
            veh->x, veh->y, tx, ty, best_score, veh->name());
        pm_target[tx][ty]++;
        action_airdrop(id, tx, ty, 3);
        return true;
    }
    return false;
}

int combat_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq;
    MAP* veh_sq = mapsq(veh->x, veh->y);
    if (!veh_sq) {
        return VEH_SYNC;
    }
    AIPlans& p = plans[veh->faction_id];
    int faction = veh->faction_id;
    int triad = veh->triad();
    int moves = veh_speed(id, 0) - veh->moves_spent;
    int defenders = 0;
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
    bool at_home = veh_sq->owner == faction || has_pact(faction, veh_sq->owner);
    bool at_base = veh_sq->is_base();
    bool at_enemy = at_war(faction, veh_sq->owner);
    bool base_found = at_base;
    bool is_enhanced = veh->is_probe() && has_abil(veh->unit_id, ABL_ALGO_ENHANCEMENT);
    /*
    Scouting priority is mainly represented by maximum search distance.
    */
    int max_dist = min(10, max((veh->high_damage() ? 2 : 3),
        random(4) + (veh->speed() > 3 ? 4 : 0)
        + (at_base ? 0 : 2)
        + (at_enemy ? -3 : 0)
        + (veh->need_heals() ? -5 : 0)
        + (*CurrentTurn < 80 ? 2 : 0)
        + (p.unknown_factions / 2) + (p.contacted_factions ? 0 : 2)
    ));
    if (veh_sq->is_airbase() && airdrop_move(id, veh_sq)) {
        return VEH_SYNC;
    }
    if (triad == TRIAD_LAND && mapnodes.count({veh->x, veh->y, NODE_GOAL_NAVAL_END})) {
        return make_landing(id);
    }
    if (triad == TRIAD_LAND && is_ocean(veh_sq) && at_base && !has_transport(veh->x, veh->y, faction)) {
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
        bool replace_order = veh->is_probe()
            && (sq = mapsq(veh->waypoint_1_x, veh->waypoint_1_y))
            && sq->is_base()
            && sq->owner != faction
            && !has_pact(faction, sq->owner)
            && !allow_probe(faction, sq->owner, is_enhanced);
        if (!replace_order) {
            return VEH_SYNC;
        }
    }
    if (at_base) {
        if (veh->is_garrison_unit() && garrison_count(veh->x, veh->y) <= 1
        && *CurrentTurn > random(32) + game_start_turn()) {
            defenders = 0;
        } else {
            defenders = defender_count(veh->x, veh->y, id); // Excluding this unit
        }
        if (!defenders) {
            max_dist = 1;
        }
    }
    bool defend;
    if (triad == TRIAD_SEA) {
        defend = (at_home && (id % 11) < 3 && !veh->is_probe())
            || (!(region_flags[veh_sq->region] & (veh->is_probe() ? PM_PROBE : PM_ENEMY))
            && !(region_flags[p.main_sea_region] & (veh->is_probe() ? PM_PROBE : PM_ENEMY)));
    } else {
        defend = (at_home && (id % 11) < 5)
            || (veh->is_probe() && veh->speed() < 2)
            || !(region_flags[veh_sq->region] & (veh->is_probe() ? PM_PROBE : PM_ENEMY));
    }
    bool landing_unit = triad == TRIAD_LAND
        && (!at_base || defenders > 0)
        && invasion_unit(id);
    bool invasion_ship = triad == TRIAD_SEA
        && (!at_base || defenders > 0)
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
        - 0.008*min(50, (int)pm_unit_near[veh->x][veh->y])
        - 0.0005*min(500, p.land_combat_units + p.sea_combat_units);
    int best_score = arty_value(veh->x, veh->y);
    TileSearch ts;
    ts.init(veh->x, veh->y, type);

    while (combat && (sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        bool to_base = sq->is_base();
        assert(veh->x != ts.rx || veh->y != ts.ry);
        assert(map_range(veh->x, veh->y, ts.rx, ts.ry) <= ts.dist);
        assert((at_base && to_base) < ts.dist);

        // Choose defender skips tiles that have neutral or allied units only
        if (attack && (id2 = choose_defender(ts.rx, ts.ry, id, sq)) >= 0) {
            assert(at_war(faction, Vehs[id2].faction_id));
            if (!ignore_zocs) { // Avoid zones of control
                max_dist = ts.dist;
            }
            if (!to_base && Vehs[id2].chassis_type() == CHS_NEEDLEJET
            && !has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)) {
                continue;
            }
            double odds = battle_priority(id, id2, ts.dist - 1, moves, sq);

            if (odds > best_odds) {
                tx = ts.rx;
                ty = ts.ry;
                best_odds = odds;
            } else if (tx < 0 && ts.dist < 2 && Vehs[id2].faction_id == 0 && veh->iter_count > 1) {
                return escape_move(id);
            }

        } else if (to_base && at_war(faction, sq->owner)
        && (triad == TRIAD_SEA) == is_ocean(sq)
        && choose_defender(ts.rx, ts.ry, id, sq) < 0
        && pm_target[ts.rx][ts.ry] < 2 + random(16)) {
            return set_move_to(id, ts.rx, ts.ry);

        } else if (arty && veh->iter_count < 2
        && arty_value(ts.rx, ts.ry) - 4*ts.dist > best_score
        && allow_move(ts.rx, ts.ry, faction, triad)) {
            tx = ts.rx;
            ty = ts.ry;
            best_score = arty_value(ts.rx, ts.ry) - 4*ts.dist;

        } else if (tx < 0 && attack && mapnodes.count({ts.rx, ts.ry, NODE_COMBAT_PATROL})
        && ts.dist <= (at_base ? 1 + min(3, defenders/4) : 3)
        && path_distance(veh->x, veh->y, ts.rx, ts.ry, veh->unit_id, faction) <= veh->speed()) {
            return set_move_to(id, ts.rx, ts.ry);

        } else if (tx < 0 && mapnodes.count({ts.rx, ts.ry, NODE_PATROL})) {
            return set_move_to(id, ts.rx, ts.ry);

        } else if (px < 0 && allow_scout(faction, sq)) {
            px = ts.rx;
            py = ts.ry;
        }
    }
    if (veh->is_probe() && pm_enemy_dist[veh->x][veh->y] != 1) {
        Faction* f = &Factions[faction];
        max_dist = (at_base && defenders < 1 + random(8) ? 1 : veh->speed());
        while ((sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
            if (!sq->is_base()
            && (id2 = choose_defender(ts.rx, ts.ry, id, sq)) >= 0
            && Vehs[id2].triad() != TRIAD_AIR) {
                if (Vehs[id2].is_probe()) {
                    double odds = battle_priority(id, id2, ts.dist - 1, moves, sq);
                    if (odds > best_odds) {
                        tx = ts.rx;
                        ty = ts.ry;
                        best_odds = odds;
                    }
                } else if (ts.dist == 1
                && allow_probe(faction, Vehs[id2].faction_id, is_enhanced)
                && f->energy_credits > clamp(*CurrentTurn * f->base_count / 8, 100, 500)
                && stack_search(ts.rx, ts.ry, faction, ST_EnemyOneUnit, WMODE_COMBAT)
                && !has_abil(Vehs[id2].unit_id, ABL_POLY_ENCRYPTION)) {
                    int num = *VehCount;
                    int reserve = f->energy_credits;
                    debug("combat_probe %2d %2d -> %2d %2d reserve: %d %s %s",
                        veh->x, veh->y, ts.rx, ts.ry, reserve, veh->name(), Vehicles[id2].name());
                    if (probe(id, -1, id2, 1) || num != *VehCount) {
                        debug(" cost: %d\n", reserve - f->energy_credits);
                        return VEH_SKIP;
                    }
                    debug(" cost: %d\n", reserve - f->energy_credits);
                }
            }
            if (tx < 0 && mapnodes.count({ts.rx, ts.ry, NODE_PATROL})
            && allow_move(ts.rx, ts.ry, faction, triad)) {
                tx = ts.rx;
                ty = ts.ry;
            }
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
                for (int k=0; k < *VehCount; k++) {
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
                debug("arty_score %2d %2d -> %2d %2d score: %d %s\n",
                    veh->x, veh->y, tx, ty, score, veh->name());
                if (score > best_score) {
                    best_score = score;
                    offset = i;
                    tx = x2;
                    ty = y2;
                }
            }
        }
        if (tx >= 0 && ((at_base && !defenders) || random(500) < min(400, best_score))) {
            debug("combat_arty %2d %2d -> %2d %2d score: %d %s\n",
                veh->x, veh->y, tx, ty, best_score, veh->name());
            battle_fight_1(id, offset, 1, 1, 0);
            return VEH_SYNC;
        }
    }
    if (!arty && at_enemy && veh_sq->items & (BIT_SENSOR | BIT_AIRBASE | BIT_THERMAL_BORE)) {
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
    if (at_base && !veh->moves_spent && (*CurrentTurn + id) & 1) { // PSI Gate moves
        int source = -1;
        int target = -1;
        int base_id = base_at(veh->x, veh->y);
        BASE* b = &Bases[base_id];

        if (base_id >= 0 && b->faction_id == veh->faction_id && can_use_teleport(base_id)) {
            source = base_id;
            best_score = 128*b->defend_goal - pm_safety[b->x][b->y] + random(256);
        }
        for (base_id = 0; source >= 0 && base_id < *BaseCount; base_id++) {
            b = &Bases[base_id];
            if (b->faction_id == veh->faction_id && source != base_id
            && has_fac_built(FAC_PSI_GATE, base_id)
            && ((triad == TRIAD_LAND && !is_ocean(b))
            || (triad == TRIAD_SEA && coast_tiles(b->x, b->y)))) {
                int score = 128*b->defend_goal - pm_safety[b->x][b->y]
                    - 16*pm_target[b->x][b->y];
                if (score > best_score) {
                    best_score = score;
                    target = base_id;
                }
            }
        }
        if (target >= 0) {
            debug("action_gate %2d %2d %s -> %s\n", veh->x, veh->y, veh->name(), Bases[target].name);
            pm_target[ Bases[target].x ][ Bases[target].y ]++;
            action_gate(id, target);
            return VEH_SYNC;
        }
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
        } else if (defender_count(tx, ty, -1) < garrison_goal(tx, ty, faction, TRIAD_LAND)) {
            debug("combat_stack %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
            return set_move_to(id, tx, ty);
        }
    }
    // Provide cover for naval transports
    if (invasion_ship && !veh->moves_spent
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
            int score = arty_value(ts.rx, ts.ry) - ts.dist;
            if (score > best_score && allow_move(ts.rx, ts.ry, faction, triad)) {
                tx = ts.rx;
                ty = ts.ry;
                best_score = score;
            }
        }
        if (tx >= 0) {
            debug("combat_adjust %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
            return set_move_to(id, tx, ty);
        }
    }
    // Find a base to attack or defend own base on the same region
    int tolerance = (ignore_zocs ? 4 : 2) + (triad == TRIAD_SEA ? 2 : 0);
    if ((*CurrentTurn + id) % 4) {
        limit = QueueSize / (defend ? 20 : 4);
    } else {
        limit = QueueSize / (defend ? 5 : 1);
    }
    bool check_zocs = !ignore_zocs && pm_enemy_near[veh->x][veh->y];
    PathNode& path = ts.paths[0];
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
    while (++i <= limit && (sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        base_found = base_found || sq->is_base();
        if (!sq->is_base() || (check_zocs && ts.has_zoc(faction))) {
            continue;
        }
        if (defend && sq->owner == faction) {
            if (garrison_goal(ts.rx, ts.ry, faction, triad)
            > defender_count(ts.rx, ts.ry, -1) + pm_target[ts.rx][ts.ry]/2) {
                debug("combat_defend %2d %2d -> %2d %2d %s\n",
                veh->x, veh->y, ts.rx, ts.ry, veh->name());
                return set_move_to(id, ts.rx, ts.ry);
            }
        } else if (!defend && allow_attack(faction, sq->owner, veh->is_probe(), is_enhanced)) {
            assert(sq->owner != faction);
            if (tx < 0) {
                max_dist = ts.dist + tolerance;
            }
            int score = target_priority(ts.rx, ts.ry, faction, sq) - 16*ts.dist + random(80);
            if (score > best_score) {
                best_score = score;
                path = ts.paths[ts.current];
                tx = ts.rx;
                ty = ts.ry;
            }
        }
    }
    if (tx >= 0) {
        bool flank = !defend && path.dist > 1 && random(12) < min(8, (int)pm_target[tx][ty]);
        bool skip  = !defend && path.dist < 6 && !veh->is_probe() && (sq = mapsq(tx, ty))
            && (id2 = choose_defender(tx, ty, id, sq)) >= 0
            && battle_priority(id, id2, path.dist - 1, moves, sq) < 0.7;
        if (skip) {
            debug("combat_skip %2d %2d -> %2d %2d %s %s\n",
            veh->x, veh->y, ts.rx, ts.ry, veh->name(), Vehicles[id2].name());
        }
        if (flank || skip) {
            tx = -1;
            ty = -1;
            best_score = flank_score(veh->x, veh->y, veh_sq);
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
            PathNode& prev = ts.paths[path.prev];
            if (path.dist > 3 && random(10) < pm_enemy_near[tx][ty]) {
                Points tiles;
                tiles.insert({prev.x, prev.y});
                for (auto& m : iterate_tiles(prev.x, prev.y, 1, 9)) {
                    if (allow_move(m.x, m.y, faction, triad)) {
                        tiles.insert({m.x, m.y});
                    }
                }
                auto t = pick_random(tiles);
                tx = t.x;
                ty = t.y;
            } else {
                tx = prev.x;
                ty = prev.y;
            }
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



