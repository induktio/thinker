
#include "move.h"

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


PMTable mapdata;
NodeSet mapnodes;
static Points nonally;
static std::set<int> region_enemy;
static std::set<int> region_probe;
static int build_tubes = false;
int move_upkeep_faction = 0;


static void adjust_former(int x, int y, int range, int value) {
    assert(range > 0 && range < 9);
    for (const auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        mapdata[{m.x, m.y}].former += value;
    }
}

static void adjust_safety(int x, int y, int range, int value) {
    assert(range > 0 && range < 9);
    for (const auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        mapdata[{m.x, m.y}].safety += value;
    }
}

static void adjust_shore(int x, int y, int range, int value) {
    assert(range > 0 && range < 9);
    for (const auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        mapdata[{m.x, m.y}].shore += value;
    }
}

static void adjust_unit_near(int x, int y, int range, int value) {
    assert(range > 0 && range < 9);
    for (const auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        mapdata[{m.x, m.y}].unit_near += value;
    }
}

static void adjust_enemy_near(int x, int y, int range, int value) {
    assert(range > 0 && range < 9);
    for (const auto& m : iterate_tiles(x, y, 0, TableRange[range])) {
        mapdata[{m.x, m.y}].enemy_near += value;
    }
}

static bool reg_enemy_at(int region, bool is_probe) {
    return is_probe ? region_probe.count(region) : region_enemy.count(region);
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
        return move_upkeep_faction != faction || mapdata[{x, y}].safety >= PM_NEAR_SAFE;
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
    return mapdata[{x, y}].shore & ShoreTilesMask;
}

// Ocean refers only to main_sea_region
int ocean_coast_tiles(int x, int y) {
    assert(mapsq(x, y));
    return mapdata[{x, y}].shore / ShoreLine;
}

bool near_ocean_coast(int x, int y) {
    assert(mapsq(x, y));
    if (mapdata[{x, y}].shore >= ShoreLine) {
        return false;
    }
    for (const auto& m : iterate_tiles(x, y, 1, 9)) {
        if (mapdata[{m.x, m.y}].shore >= ShoreLine) {
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
        } else if (veh->is_artifact()) {
            return artifact_move(veh_id);
        } else if (triad == TRIAD_SEA && veh_cargo(veh_id) > 0) {
            return trans_move(veh_id);
        } else if (triad == TRIAD_AIR && veh->is_combat_unit() && !veh->is_planet_buster()) {
            return aircraft_move(veh_id);
        } else if (triad != TRIAD_AIR && (veh->is_combat_unit() || veh->is_probe())) {
            return combat_move(veh_id);
        }
    } else if (conf.manage_player_units && veh->plr_owner()
    && (veh->is_colony() || veh->is_former() || veh->is_supply())) {
        if (!*CurrentBase) {
            int base_id = mod_base_find3(veh->x, veh->y, veh->faction_id, -1, -1, -1);
            if (base_id >= 0) {
                set_base(base_id);
            }
        }
        if (move_upkeep_faction != veh->faction_id) {
            former_plans(veh->faction_id);
            move_upkeep(veh->faction_id, UM_Player);
            move_upkeep_faction = veh->faction_id;
        }
        if (veh->is_colony()) {
            return colony_move(veh_id);
        } else if (veh->is_former()) {
            return former_move(veh_id);
        } else if (veh->is_supply()) {
            return crawler_move(veh_id);
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
    if (thinker_enabled(move_upkeep_faction) && at_war(move_upkeep_faction, veh->faction_id)) {
        if (veh->x >= 0 && veh->y >= 0) {
            adjust_enemy_near(veh->x, veh->y, 2, -1);
            mapdata[{veh->x, veh->y}].enemy--;
            if (mapdata[{veh->x, veh->y}].target > 0) {
                mapdata[{veh->x, veh->y}].target--;
            }
        }
    }
    return veh_lift(veh_id);
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
    if (!has_terra(faction, FORMER_RAISE_LAND, TRIAD_LAND) || Factions[faction].energy_credits < 100) {
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
            int base_id = base_at(x, y);
            if (base_id >= 0) {
                score += (has_fac_built(FAC_HEADQUARTERS, base_id) ? 100 : 0);
                score += (Bases[base_id].is_objective() ? 100 : 0);
                score += clamp(4*Bases[base_id].pop_size, 4, 64);
            }
            score += (mapdata[{x, y}].roads ? 150 : 0);
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
    for (int i = 1; i < MaxPlayerNum; i++) {
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

    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (!(sq = mapsq(base->x, base->y))) {
            continue;
        }
        if (sq->region == p.main_region && base->faction_id == faction
        && coast_tiles(base->x, base->y)) {
            path.push_back({base->x, base->y});

        } else if (at_war(faction, base->faction_id) && !is_ocean(sq)) {
            enemy_bases.push_back({base->x, base->y});
        }
    }
    int i = 0;
    ts.init(enemy_bases, TS_TRIAD_LAND, 1);
    while (++i <= QueueSize && (sq = ts.get_next()) != NULL) {
        assert(!(mapdata[{ts.rx, ts.ry}].enemy_dist));
        mapdata[{ts.rx, ts.ry}].enemy_dist = ts.dist;
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
    int best_score = -1000;
    i = 0;
    while (++i <= QueueSize && (sq = ts.get_next()) != NULL) {
        PathNode& prev = ts.get_prev();
        if (sq->is_land_region()
        && mapdata[{ts.rx, ts.ry}].enemy_dist > 0
        && mapdata[{ts.rx, ts.ry}].enemy_dist < 10
        && allow_move(prev.x, prev.y, faction, TRIAD_SEA)
        && allow_move(ts.rx, ts.ry, faction, TRIAD_LAND)) {
            ts.get_route(path);
            if (sq->region == p.main_region
            && (ts.dist < 6 || sq->owner == faction
            || map_range(ts.rx, ts.ry, path.begin()->x, path.begin()->y) + 1 < ts.dist)) {
                continue;
            }
            int score = min(0, mapdata[{ts.rx, ts.ry}].safety / 2)
                + target_priority(ts.rx, ts.ry, faction, sq)
                + (coast_tiles(prev.x, prev.y) < 7 ? 200 : 0)
                + (ocean_coast_tiles(prev.x, prev.y) > 0 ? 600 : 0)
                + (sq->region == p.main_region ? 0 : 400)
                - 32*mapdata[{ts.rx, ts.ry}].enemy_dist
                - 16*ts.dist + random(32);
            if (px >= 0) {
                score -= 4*map_range(px, py, prev.x, prev.y);
            }
            if (score > best_score) {
                best_score = score;
                p.target_land_region = sq->region;
                p.naval_start_x = path.begin()->x;
                p.naval_start_y = path.begin()->y;
                p.naval_end_x = prev.x;
                p.naval_end_y = prev.y;
                p.naval_beach_x = ts.rx;
                p.naval_beach_y = ts.ry;

                debug("invasion %d -> %d start: %2d %2d end: %2d %2d "\
                "coast: %d ocean: %d dist: %2d score: %d\n",
                faction, sq->owner, path.begin()->x, path.begin()->y, ts.rx, ts.ry,
                coast_tiles(prev.x, prev.y), ocean_coast_tiles(prev.x, prev.y), ts.dist, score);
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
        for (i = 0; i < *BaseCount; i++) {
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

    for (int i = 0; i < *BaseCount; i++) {
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
    ts.init(p.main_region_x, p.main_region_y, TS_TERRITORY_SHORE, 2);
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
    int tile_count[MaxRegionNum] = {};
    Faction& f = Factions[faction];
    AIPlans& p = plans[faction];
    if (!is_alive(faction)) {
        return;
    }
    if ((mode == UM_Full && !thinker_enabled(faction))
    || (mode == UM_Player && !is_human(faction))) {
        return;
    }
    move_upkeep_faction = faction;
    build_tubes = has_terra(faction, FORMER_MAGTUBE, TRIAD_LAND);
    mapdata.clear();
    mapnodes.clear();
    region_enemy.clear();
    region_probe.clear();
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
                    adjust_shore(x, y, 1, 1);
                    if (p.main_sea_region < 0
                    || tile_count[sq->region] > tile_count[p.main_sea_region]) {
                        p.main_sea_region = sq->region;
                    }
                }
            }
            if (sq->owner == faction && ((!build_tubes && sq->items & BIT_ROAD)
            || (build_tubes && sq->items & BIT_MAGTUBE))) {
                mapdata[{x, y}].roads++;
            } else if (goody_at(x, y)) {
                mapnodes.insert({x, y, NODE_PATROL});
                if (!sq->is_owned()) {
                    for (auto& m : iterate_tiles(x, y, 0, 9)) {
                        mapnodes.insert({m.x, m.y, NODE_SCOUT_SITE});
                    }
                }
            }
        }
    }
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x+=2) {
            if ((sq = mapsq(x, y)) && sq->region == p.main_sea_region) {
                adjust_shore(x, y, 1, ShoreLine);
            }
            assert(goody_at(x, y) == mod_goody_at(x, y));
            assert(bonus_at(x, y) == mod_bonus_at(x, y));
            assert(zoc_any(x, y, faction) == mod_zoc_any(x, y, faction));
            assert(zoc_sea(x, y, faction) == mod_zoc_sea(x, y, faction));
            assert(zoc_veh(x, y, faction) == mod_zoc_veh(x, y, faction));
            assert(zoc_move(x, y, faction) == mod_zoc_move(x, y, faction));
        }
    }
    for (int i = 0; i < *VehCount; i++) {
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
                adjust_safety(veh->x, veh->y, 1, 40);
                mapdata[{veh->x, veh->y}].safety += 60;
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
            adjust_unit_near(veh->x, veh->y, 3, (triad == TRIAD_LAND ? 2 : 1));

        } else if (at_war(faction, veh->faction_id)) {
            int value = (veh->is_combat_unit() ? -100 : (veh->is_probe() ? -24 : -12));
            int range = (veh->speed() > 1 ? 3 : 2) + (triad == TRIAD_AIR ? 2 : 0);
            if (value == -100) {
                adjust_safety(veh->x, veh->y, range, value/(range > 2 ? 4 : 8));
            }
            mapdata[{veh->x, veh->y}].enemy++;
            adjust_safety(veh->x, veh->y, 1, value);
            if (veh->unit_id != BSC_FUNGAL_TOWER) {
                adjust_enemy_near(veh->x, veh->y, 2, 1);
            }

        } else if (has_pact(faction, veh->faction_id) && veh->is_combat_unit()) {
            adjust_safety(veh->x, veh->y, 1, 20);
            mapdata[{veh->x, veh->y}].safety += 30;
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
            && stack_search(veh->x, veh->y, faction, ST_NeutralOnly, WMODE_PROBE)) {
                mapnodes.insert({veh->x, veh->y, NODE_COMBAT_PATROL});
                debug("capture_probe %2d %2d\n", veh->x, veh->y);
            }
        }
    }
    TileSearch ts;
    PointList main_bases;
    bool enemy = false;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (!(sq = mapsq(base->x, base->y))) {
            continue;
        }
        if (base->faction_id == faction) {
            adjust_former(base->x, base->y, 2, base->pop_size);
            mapdata[{base->x, base->y}].safety += 10000;

            if (sq->veh_who() < 0) { // Undefended base
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
                        if (f.region_total_bases[sq->region] < 4
                        || route_distance(mapdata, base->x, base->y, ts.rx, ts.ry) < 0) {
                            ts.adjust_roads(mapdata, 1);
                        }
                        k++;
                    }
                }
            }
        } else {
            if (at_war(faction, base->faction_id)) {
                region_enemy.insert(sq->region);
                enemy = true;
            } else if (has_pact(faction, base->faction_id)) {
                mapdata[{base->x, base->y}].safety += 5000;
            }
            if (allow_probe(faction, base->faction_id, true)) {
                region_probe.insert(sq->region);
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
                    ts.adjust_roads(mapdata, 1);
                    roads[sq->owner]++;
                }
            }
        }
    }
    if (f.base_count > 0 && mode == UM_Full) {
        int values[MaxBaseNum] = {};
        int sorter[MaxBaseNum] = {};
        int bases = 0;

        for (int i = 0; i < *BaseCount; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction) {
                values[i] = base->pop_size + (base->is_objective() ? 16 : 0)
                    + 16*has_fac_built(FAC_HEADQUARTERS, i)
                    + 8*mapdata[{base->x, base->y}].enemy_near
                    + (base->state_flags & BSTATE_COMBAT_LOSS_LAST_TURN ? 8 : 0)
                    - (base->defend_range > 0 ? base->defend_range : MaxEnemyRange/2);
                sorter[bases] = values[i];
                bases++;
                debug("defend_score %4d %s\n", values[i], base->name);
            }
        }
        std::sort(sorter, sorter+bases);
        p.defend_weights = 0;

        for (int i = 0; i < *BaseCount; i++) {
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
                        mapdata[{x, y}].overlay = base_tile_score(x, y, faction, sq);
                    } else if (k==1) {
                        mapdata[{x, y}].overlay = former_tile_score(x, y, faction, sq);
                    } else if (k==2) {
                        mapdata[{x, y}].overlay = arty_value(x, y);
                    }
                }
            }
        } else {
            if (k==4) {
                invasion_plan(faction);
            }
            for (auto& m : mapdata) {
                if (k==3) {
                    m.second.overlay = m.second.safety;
                } else if (k==4) {
                    m.second.overlay = m.second.enemy_dist;
                } else if (k==5) {
                    m.second.overlay = m.second.roads;
                }
            }
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
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = f.goals[i];
        if (goal.priority <= 0) {
            continue;
        }
        if (goal.type == AI_GOAL_RAISE_LAND) {
            mapnodes.insert({goal.x, goal.y, NODE_GOAL_RAISE_LAND});
            mapdata[{goal.x, goal.y}].former += 8;
            mapdata[{goal.x, goal.y}].roads += 2;
        }
        if (goal.type == AI_GOAL_NAVAL_START) {
            mapnodes.insert({goal.x, goal.y, NODE_NAVAL_START});
        }
        if (goal.type == AI_GOAL_NAVAL_END) {
            mapnodes.insert({goal.x, goal.y, NODE_NAVAL_END});
        }
        if (goal.type == AI_GOAL_NAVAL_BEACH) {
            mapnodes.insert({goal.x, goal.y, NODE_NAVAL_BEACH});
        }
        if (goal.type == AI_GOAL_NAVAL_PICK) {
            mapnodes.insert({goal.x, goal.y, NODE_NAVAL_PICK});
            mapnodes.insert({goal.x, goal.y, NODE_NEED_FERRY});
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

ResType want_convoy(int veh_id, int x, int y, int* score, MAP* sq) {
    VEH* veh = &Vehs[veh_id];
    BASE* base = &Bases[veh->home_base_id];
    ResType choice = RES_NONE;
    int base_id = veh->home_base_id;
    *score = 0;

    if (!sq->is_base() && base_id >= 0
    && (sq->owner == veh->faction_id || !sq->is_owned())) {
        for (int i = 0; i < *VehCount; i++) {
            if (veh_id != i && Vehs[i].x == x && Vehs[i].y == y
            && Vehs[i].is_supply() && Vehs[i].order == ORDER_CONVOY) {
                mapnodes.insert({x, y, NODE_CONVOY_SITE});
                return RES_NONE;
            }
        }
        int N = mod_crop_yield(veh->faction_id, base_id, x, y, 0);
        int M = mod_mine_yield(veh->faction_id, base_id, x, y, 0);
        int E = mod_energy_yield(veh->faction_id, base_id, x, y, 0);

        int Nw = (base->nutrient_surplus < 0 ? 8 : min(8, 2 + base_growth_goal(base_id)))
            - max(0, base->nutrient_surplus - 14) + (base->pop_size < 4 ? 2 : 0);
        int Mw = max(3, (50 - base->mineral_intake_2) / 5);
        int Ew = max(3, Mw - 1);
        int B = sq->is_base_radius() ? 2 : 4;

        int Ns = Nw*N - (M+E)*(M+E)/B;
        int Ms = Mw*M - (N+E)*(N+E)/B;
        int Es = Ew*E - (N+M)*(N+M)/B;

        if (M > 1 && Ms > *score) {
            choice = RES_MINERAL;
            *score = Ms;
        }
        if (N > 1 && Ns > *score) {
            choice = RES_NUTRIENT;
            *score = Ns;
        }
        if (E > 1 && Es > *score
        && base->energy_inefficiency*2 < base->energy_surplus
        && base->mineral_surplus > min(20, 4 + base->pop_size)
        && !has_fac_built(FAC_PUNISHMENT_SPHERE, base_id)
        && (has_fac_built(FAC_NETWORK_NODE, base_id)
        || has_fac_built(FAC_TREE_FARM, base_id)
        || project_base(FAC_SUPERCOLLIDER) == base_id
        || project_base(FAC_THEORY_OF_EVERYTHING) == base_id)) {
            choice = RES_ENERGY;
            *score = Es;
        }
    }
    if (sq->owner == veh->faction_id) {
        *score += 8;
    }
    return choice;
}

int crawler_move(const int id) {
    VEH* veh = &Vehs[id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (!sq) {
        return mod_veh_skip(id);
    }
    if (veh->home_base_id < 0 || Bases[veh->home_base_id].faction_id != veh->faction_id) {
        if (is_human(veh->faction_id)){
            return mod_veh_skip(id);
        }
        if (sq->is_base() && sq->owner == veh->faction_id) {
            veh->home_base_id = base_at(veh->x, veh->y);
        } else if (random(2)) {
            return move_to_base(id, false);
        }
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
    ResType best_choice = want_convoy(id, veh->x, veh->y, &best_score, sq);
    if (best_choice != RES_NONE && (*CurrentTurn + id) % 4) {
        mapnodes.insert({veh->x, veh->y, NODE_CONVOY_SITE});
        return set_convoy(id, best_choice);
    }
    int i = 0;
    int tx = -1;
    int ty = -1;
    int score = 0;
    int limit = (best_choice != RES_NONE ? 80 : 120);
    TileSearch ts;
    ts.init(veh->x, veh->y, veh->triad());

    while (++i <= limit && (sq = ts.get_next()) != NULL) {
        if (mapdata[{ts.rx, ts.ry}].safety < PM_SAFE
        || non_ally_in_tile(ts.rx, ts.ry, veh->faction_id)
        || mapnodes.count({ts.rx, ts.ry, NODE_CONVOY_SITE})) {
            continue;
        }
        int choice = want_convoy(id, ts.rx, ts.ry, &score, sq);
        if (choice != RES_NONE && score - ts.dist > best_score) {
            best_score = score - ts.dist;
            tx = ts.rx;
            ty = ts.ry;
            debug("crawl_score %2d %2d res: %2d score: %2d %s\n",
                ts.rx, ts.ry, choice, best_score, Bases[veh->home_base_id].name);
        }
    }
    if (tx >= 0) {
        mapnodes.insert({tx, ty, NODE_CONVOY_SITE});
        return set_move_to(id, tx, ty);
    }
    if (best_choice != RES_NONE) {
        mapnodes.insert({veh->x, veh->y, NODE_CONVOY_SITE});
        return set_convoy(id, best_choice);
    }
    if (!veh->plr_owner() && !random(4)) {
        return move_to_base(id, false);
    }
    return mod_veh_skip(id);
}

bool can_build_base(int x, int y, int faction, int triad) {
    assert(valid_player(faction));
    assert(valid_triad(triad));
    MAP* sq;
    if (!(sq = mapsq(x, y)) || *BaseCount >= MaxBaseNum) {
        return false;
    }
    if ((y < 3 || y >= *MapAreaY - 3) && *MapAreaTiles >= 1600) {
        return false;
    }
    if (map_is_flat() && (x < 2 || x >= *MapAreaX - 2) && *MapAreaTiles >= 1600) {
        return false;
    }
    if (sq->is_rocky() || sq->volcano_center()
    || (sq->items & (BIT_BASE_DISALLOWED | BIT_ADVANCED))) {
        return false;
    }
    // Allow base building on smaller maps in owned territory if a new faction is spawning.
    if (sq->owner >= 0 && sq->owner != faction
    && Factions[faction].base_count > 0
    && !has_treaty(faction, sq->owner, DIPLO_VENDETTA)
    && has_treaty(faction, sq->owner, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
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
    int alt = sq->alt_level();
    if (triad != TRIAD_SEA && alt >= ALT_SHORE_LINE) {
        return true;
    }
    if ((triad == TRIAD_SEA || triad == TRIAD_AIR)
    && (alt == ALT_OCEAN || alt == ALT_OCEAN_SHELF)) {
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
    int score = min(min(y, *MapAreaY - y), min(*MapAreaY/4, 24)) / 2;
    int land = 0;
    score += (sq->items & BIT_SENSOR ? 8 : 0);
    if (!sea_colony) {
        score += (ocean_coast_tiles(x, y) ? 12 : 0);
        score += (sq->items & BIT_RIVER ? 5 : 0);
    }
    for (auto& m : iterate_tiles(x, y, 1, 21)) {
        assert(map_range(x, y, m.x, m.y) == 1 + (m.i > 8));
        if (!m.sq->is_base()) {
            int alt = sq->alt_level();
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
                if (alt == ALT_OCEAN_SHELF) {
                    score += (sea_colony ? 3 : 2);
                }
                if (alt <= ALT_OCEAN) {
                    score -= (alt == ALT_OCEAN_TRENCH ? 4 : 2);
                }
            }
            if (sea_colony != (alt < ALT_SHORE_LINE)
            && both_non_enemy(faction, m.sq->owner)) {
                score -= 5;
            }
            if (alt >= ALT_SHORE_LINE) {
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
    return score + min(0, mapdata[{x, y}].safety);
}

int colony_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    int veh_region = sq->region;
    int faction = veh->faction_id;
    int triad = veh->triad();
    TileSearch ts;
    if (defend_tile(veh, sq) || veh->iter_count > 3) {
        return mod_veh_skip(id);
    }
    if (mapdata[{veh->x, veh->y}].safety < PM_SAFE && !veh->plr_owner()) {
        return escape_move(id);
    }
    if (can_build_base(veh->x, veh->y, faction, triad)) {
        if (triad == TRIAD_LAND && (veh->at_target() || !near_ocean_coast(veh->x, veh->y))) {
            return action_build(id, 0);
        } else if (triad == TRIAD_SEA && veh->at_target()) {
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
    if (!veh->at_target() && (veh->state & VSTATE_UNK_40000 || !(veh->state & VSTATE_UNK_2000))
    && can_build_base(veh->waypoint_1_x, veh->waypoint_1_y, veh->faction_id, triad)) {
        for (auto& m : iterate_tiles(veh->waypoint_1_x, veh->waypoint_1_y, 0, 9)) {
            mapnodes.insert({m.x, m.y, NODE_BASE_SITE});
        }
        ts.connect_roads(mapdata, veh->waypoint_1_x, veh->waypoint_1_y, faction);
        return VEH_SYNC;
    }
    bool at_base = sq->is_base() && sq->owner == faction;
    bool skip_owner = sq->is_owned() && sq->owner != faction && !has_pact(faction, sq->owner);
    int airdrop = can_airdrop(id, sq) ? drop_range(faction) : 0;
    int best_score = INT_MIN;
    int i = 0;
    int k = 0;
    int tx = -1;
    int ty = -1;
    if (triad == TRIAD_SEA && invasion_unit(id)) {
        ts.init(plans[faction].naval_end_x, plans[faction].naval_end_y, triad, 1);
    } else if (airdrop) {
        ts.init(veh->x, veh->y, TRIAD_AIR, 1);
    } else {
        ts.init(veh->x, veh->y, triad, 1);
    }
    while (++i <= 1600 && (sq = ts.get_next()) != NULL) {
        if (mapnodes.count({ts.rx, ts.ry, NODE_BASE_SITE})
        || !can_build_base(ts.rx, ts.ry, faction, triad)
        || !safe_path(ts, faction, skip_owner)
        || (airdrop && ts.dist > airdrop && veh_region != sq->region)
        || (airdrop && !allow_airdrop(ts.rx, ts.ry, faction, true, sq))) {
            continue;
        }
        int score = base_tile_score(ts.rx, ts.ry, faction, sq) - 2*ts.dist;
        if (score > best_score) {
            tx = ts.rx;
            ty = ts.ry;
            best_score = score;
        }
        if (++k >= 25 && ts.dist >= 8 && score >= 20
        && (sq->owner != faction || !sq->is_visible(faction))) {
            break;
        }
    }
    if (tx >= 0) {
        for (auto& m : iterate_tiles(tx, ty, 0, TableRange[conf.base_spacing - 1])) {
            mapnodes.insert({m.x, m.y, NODE_BASE_SITE});
        }
        if (airdrop && map_range(veh->x, veh->y, tx, ty) <= airdrop
        && (veh_region != region_at(tx, ty)
        || path_cost(veh->x, veh->y, tx, ty, veh->unit_id, faction, veh_speed(id, 0)) < 0)) {
            debug("colony_drop %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
            action_airdrop(id, tx, ty, 3);
            return VEH_SKIP;
        }
        debug("colony_move %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
        // Set these flags to disable any non-Thinker unit automation.
        veh->state |= VSTATE_UNK_40000;
        veh->state &= ~VSTATE_UNK_2000;
        ts.connect_roads(mapdata, tx, ty, faction);
        return set_move_to(id, tx, ty);
    }
    if (!at_base) {
        if (search_base(ts, id, false, &tx, &ty)) {
            debug("colony_base %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
            return set_move_to(id, tx, ty);
        }
    }
    if (!veh->plr_owner()) {
        AIPlans& p = plans[faction];
        if (p.naval_start_x >= 0 && veh_region == p.main_region
        && veh->x != p.naval_start_x && veh->y != p.naval_start_y) {
            debug("colony_naval %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
            return set_move_to(id, p.naval_start_x, p.naval_start_y);
        }
        if (search_route(ts, id, &tx, &ty)) {
            return set_move_to(id, tx, ty);
        }
        if ((veh->home_base_id >= 0 || *BaseCount > 16 + random(256))
        && (!at_base || garrison_count(veh->x, veh->y) > random(8))) {
            return mod_veh_kill(id);
        }
    }
    return mod_veh_skip(id);
}

bool can_bridge(int x, int y, int faction, MAP* sq) {
    if (is_ocean(sq) || is_human(faction)
    || !has_terra(faction, FORMER_RAISE_LAND, TRIAD_LAND)) {
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
    if (mapdata[{x, y}].former < 4 && !mapnodes.count({x, y, NODE_BOREHOLE})) {
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
    if (bonus == RES_NUTRIENT && !(sq->items & BIT_FOREST)
    && (sea || sq->is_rainy_or_moist() || sq->is_rolling())) {
        return true;
    }
    if (bonus == RES_ENERGY || bonus == RES_MINERAL || sq->landmarks & LM_VOLCANO) {
        return false;
    }
    if (!has_nut && bonus != RES_NUTRIENT && mod_crop_yield(faction, -1, x, y, 0) >= 2) {
        return false;
    }
    return (sq->is_rolling()
        + sq->is_rainy_or_moist()
        + (nearby_items(x, y, 0, 9, BIT_FARM | BIT_CONDENSER) < 2)
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
    && bonus != RES_ENERGY && mod_energy_yield(faction, -1, x, y, 0) >= 2) {
        return false;
    }
    if (!sea && has_terra(faction, FORMER_FOREST, sea) && ResInfo->forest_sq_energy > 0
    && !(sq->is_rocky() && bonus == RES_ENERGY && sq->alt_level() > ALT_TWO_ABOVE_SEA)
    && (sq->landmarks & LM_JUNGLE
    || (sq->is_rainy() + sq->is_rolling() + sq->is_rainy_or_moist()
    + (sq->items & BIT_FARM ? 1 : 0) < 3))) {
        return false;
    }
    if (sq->items & BIT_SENSOR && nearby_items(x, y, 0, 9, BIT_SENSOR) < 2) {
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
    && bonus != RES_MINERAL && mod_mine_yield(faction, -1, x, y, 0) >= 2) {
        return false;
    }
    if (sq->items & BIT_SENSOR && nearby_items(x, y, 0, 9, BIT_SENSOR) < 2) {
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
    && mod_crop_yield(faction, -1, x, y, 0) >= 2) {
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
        && nearby_items(x, y, 0, 9, BIT_FUNGUS)
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
        && nearby_items(x, y, 0, 9, BIT_FARM|BIT_FOREST) < 2));
}

bool can_river(int x, int y, int faction, MAP* sq) {
    if (is_ocean(sq) || !has_terra(faction, FORMER_AQUIFER, TRIAD_LAND)) {
        return false;
    }
    if (sq->items & (BIT_BASE_IN_TILE | BIT_RIVER | BIT_THERMAL_BORE)) {
        return false;
    }
    return !(((*CurrentTurn / 4 * x) ^ y) & 15)
        && !coast_tiles(x, y)
        && nearby_items(x, y, 1, 9, BIT_RIVER|BIT_THERMAL_BORE) < 2
        && nearby_items(x, y, 1, 25, BIT_RIVER) < 6;
}

bool can_road(int x, int y, int faction, MAP* sq) {
    if (!has_terra(faction, FORMER_ROAD, is_ocean(sq))
    || sq->items & (BIT_ROAD | BIT_BASE_IN_TILE)) {
        return false;
    }
    if (!sq->is_base_radius() && mapdata[{x, y}].roads < 1) {
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
        return mapdata[{x, y}].roads > 0 && !both_neutral(faction, sq->owner);
    }
    if (mapnodes.count({x, y, NODE_GOAL_RAISE_LAND})) {
        return true;
    }
    if (mapdata[{x, y}].roads > 0 || sq->items & (BIT_MINE | BIT_CONDENSER | BIT_THERMAL_BORE)) {
        return true;
    }
    int i = 0;
    int r[] = {0,0,0,0,0,0,0,0};
    for (const auto& t : NearbyTiles) {
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
    return mapdata[{x, y}].roads > 0 && sq->items & BIT_ROAD
        && (!(sq->items & BIT_FUNGUS) || has_tech(Rules->tech_preq_build_road_fungus, faction));
}

int select_item(int x, int y, int faction, FormerMode mode, MAP* sq) {
    assert(valid_player(faction));
    assert(mapsq(x, y));
    uint32_t items = sq->items;
    int alt = sq->alt_level();
    bool sea = alt < ALT_SHORE_LINE;
    bool road = can_road(x, y, faction, sq);
    bool rem_fungus = has_terra(faction, FORMER_REMOVE_FUNGUS, sea)
        && (!is_human(faction) || *GameMorePreferences & MPREF_AUTO_FORMER_REMOVE_FUNGUS);

    if (sq->is_base() || sq->volcano_center()) {
        return FORMER_NONE;
    }
    // Improvements on ocean possible for aquatic factions after Adv. Ecological Engineering
    if (alt < ALT_OCEAN_SHELF && (!MFactions[faction].is_aquatic() || !has_tech(TECH_EcoEng2, faction))) {
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
    if (mode == FM_Farm_Road || mode == FM_Mine_Road) {
        if (items & BIT_FUNGUS) {
            if (has_terra(faction, FORMER_REMOVE_FUNGUS, sea)) {
                return FORMER_REMOVE_FUNGUS;
            }
            return FORMER_NONE;
        }
        if (!(items & BIT_ROAD) && has_terra(faction, FORMER_ROAD, sea)) {
            return FORMER_ROAD;
        }
        if (items & BIT_MONOLITH) {
            return FORMER_NONE;
        }
    }
    if (mode == FM_Farm_Road) {
        if ((sea || !sq->is_rocky()) && !(items & BIT_FARM) && has_terra(faction, FORMER_FARM, sea)) {
            return FORMER_FARM;
        }
        if (!(items & BIT_SOLAR) && has_terra(faction, FORMER_SOLAR, sea)) {
            return FORMER_SOLAR;
        }
    }
    if (mode == FM_Mine_Road) {
        if (!(items & BIT_MINE) && has_terra(faction, FORMER_MINE, sea)) {
            return FORMER_MINE;
        }
    }
    if (mode != FM_Auto_Full) { // Skip non-automated player formers
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
    bool use_sensor = items & BIT_SENSOR && nearby_items(x, y, 0, 9, BIT_SENSOR) < 2;
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
        + (!(items & BIT_CONDENSER) && allow_farm && sq->is_rainy_or_moist() && sq->is_rolling());

    for (int v : {farm_val, forest_val, fungus_val, borehole_val}) {
        max_val = max(v, max_val);
        assert(current >= 0 && v >= 0);
    }
    if (farm_val == max_val && sea && max_val > 0) {
        if (items & BIT_FUNGUS) {
            return (rem_fungus ? FORMER_REMOVE_FUNGUS : FORMER_NONE);
        }
        if (farm_val > current && allow_farm && !(items & BIT_FARM)) {
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
        + 2*max(0, sq->alt_level()-ALT_ONE_ABOVE_SEA) - nearby_items(x, y, 0, 25, BIT_SOLAR);
    if (sea) {
        solar_need = (has_terra(faction, FORMER_MINE, sea) ?
            (ResInfo->improved_sea_energy - ResInfo->improved_sea_mineral)
            - has_tech(Rules->tech_preq_mining_platform_bonus, faction): 6)
            + nearby_items(x, y, 0, 25, BIT_MINE) - nearby_items(x, y, 0, 25, BIT_SOLAR);
    }

    if (can_solar(x, y, faction, bonus, sq) && solar_need > 0) {
        if (allow_farm && !(items & BIT_FARM)) {
            return FORMER_FARM;
        }
        return FORMER_SOLAR;
    }
    if (can_mine(x, y, faction, bonus, sq)) {
        if (sea && allow_farm && !(items & BIT_FARM)) {
            return FORMER_FARM;
        }
        return FORMER_MINE;
    }
    if (items & BIT_SOLAR && solar_need >= 0) {
        return FORMER_NONE;
    }
    if (allow_farm && !(items & BIT_FARM)) {
        return FORMER_FARM;
    }
    if (!use_sensor && items & BIT_FARM && !(items & BIT_CONDENSER)
    && has_terra(faction, FORMER_CONDENSER, sea)
    && (!is_human(faction) || *GamePreferences & PREF_AUTO_FORMER_BUILD_ADV)) {
        return FORMER_CONDENSER;
    }
    if (!use_sensor && items & BIT_FARM && !(items & BIT_SOIL_ENRICHER)
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
    if (mapdata[{x, y}].roads > 0 && (!(items & BIT_ROAD) || (build_tubes && !(items & BIT_MAGTUBE)))) {
        score += 12;
    }
    if (is_shore_level(sq) && mapnodes.count({x, y, NODE_GOAL_RAISE_LAND})) {
        score += 20;
    }
    return score + min(8, mapdata[{x, y}].former) + min(0, mapdata[{x, y}].safety);
}

int former_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    int item = -1;
    int choice = -1;
    int faction = veh->faction_id;
    bool at_base = sq->is_base() && sq->owner == faction;
    bool safe = mapdata[{veh->x, veh->y}].safety >= PM_SAFE;
    FormerMode mode = FM_Auto_Full;
    if (sq && sq->owner != faction && mapdata[{veh->x, veh->y}].roads < 1) {
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
        else if (veh->order_auto_type == ORDERA_TERRA_FARM_SOLAR_ROAD) {
            mode = FM_Farm_Road;
        }
        else if (veh->order_auto_type == ORDERA_TERRA_FARM_MINE_ROAD) {
            mode = FM_Mine_Road;
        }
    }
    int turns = (veh->order >= ORDER_FARM && veh->order < ORDER_MOVE_TO ?
        Terraform[veh->order - 4].rate : 0);

    if (safe || turns >= 12 || veh->plr_owner()) {
        if (turns > 0 && !(veh->order == ORDER_DRILL_AQUIFER
        && nearby_items(veh->x, veh->y, 0, 9, BIT_RIVER) >= 4)) {
            return VEH_SYNC;
        }
        if (!veh->at_target() && !can_road(veh->x, veh->y, faction, sq)
        && !can_magtube(veh->x, veh->y, faction, sq)) {
            return VEH_SYNC;
        }
        item = select_item(veh->x, veh->y, faction, mode, sq);
        if (item >= 0) {
            int cost = 0;
            mapdata[{veh->x, veh->y}].former -= 2;
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
    if (mode == FM_Farm_Road || mode == FM_Mine_Road) {
        veh->state &= ~VSTATE_ON_ALERT; // Request new automation orders
        veh->order = ORDER_NONE;
        return VEH_SYNC;
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
        || (sq->owner != faction && mapdata[{ts.rx, ts.ry}].roads < 1)
        || (home_base_only && map_range(bx, by, ts.rx, ts.ry) > 2)
        || (mapdata[{ts.rx, ts.ry}].former < 1 && mapdata[{ts.rx, ts.ry}].roads < 1)
        || mapdata[{ts.rx, ts.ry}].safety < PM_SAFE
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
        mapdata[{tx, ty}].former -= 2;
        debug("former_move %2d %2d -> %2d %2d score: %d %s\n",
            veh->x, veh->y, tx, ty, best_score, (item >= 0 ? Terraform[item].name : ""));
        return set_move_to(id, tx, ty);
    }

    debug("former_skip %2d %2d %s\n", veh->x, veh->y, veh->name());
    if (region_at(veh->x, veh->y) == region_at(bx, by)
    && !(veh->x == bx && veh->y == by)
    && (home_base_only || map_range(veh->x, veh->y, bx, by) < random(32))) {
        return set_move_to(id, bx, by);
    }
    if (!at_base && search_base(ts, id, false, &tx, &ty)) {
        return set_move_to(id, tx, ty);
    }
    if (full_search && !veh->plr_owner()) {
        if (search_route(ts, id, &tx, &ty)) {
            return set_move_to(id, tx, ty);
        }
        if (veh->home_base_id >= 0 && !random(4)) {
            return mod_veh_kill(id);
        }
    }
    return mod_veh_skip(id);
}

int artifact_move(const int id) {
    VEH* veh = &Vehicles[id];
    int base_id = base_at(veh->x, veh->y);
    if (base_id >= 0 && Bases[base_id].faction_id == veh->faction_id
    && can_link_artifact(base_id)) {
        debug("artifact_link %2d %2d %s\n", veh->x, veh->y, Bases[base_id].name);
        study_artifact(id);
        return VEH_SKIP;
    }
    if (!veh->at_target() && veh->iter_count < 2
    && mapdata[{veh->x, veh->y}].safety >= PM_SAFE) {
        return VEH_SYNC;
    }
    TileSearch ts;
    int tx = veh->x;
    int ty = veh->y;
    if (search_route(ts, id, &tx, &ty)) {
        debug("artifact_move %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
        return set_move_to(id, tx, ty);
    }
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
        + min(4, mapdata[{enemy->x, enemy->y}].enemy)
        + min(4, arty_value(enemy->x, enemy->y)/16)
        + min(4, Units[enemy->unit_id].cost/2) >= 6 + random(6);
}

bool can_airdrop(int veh_id, MAP* sq) {
    VEH* veh = &Vehicles[veh_id];
    return has_abil(veh->unit_id, ABL_DROP_POD)
        && !(veh->state & VSTATE_MADE_AIRDROP)
        && !veh->moves_spent
        && sq->is_airbase();
}

bool allow_airdrop(int x, int y, int faction, bool combat, MAP* sq) {
    if (!sq || is_ocean(sq) || faction < 0) {
        assert(0);
        return false;
    }
    // Aerospace Complex or Air Superiority unit stationed inside base prevents all drops
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (at_war(faction, base->faction_id)
        && map_range(base->x, base->y, x, y) <= 2
        && (has_facility(FAC_AEROSPACE_COMPLEX, i)
        || mod_stack_check(veh_at(base->x, base->y), 2, PLAN_AIR_SUPERIORITY, -1, -1))) {
            return false;
        }
    }
    // Non-combat units may not be dropped into enemy bases or zones of control.
    // However dropping units into own bases is allowed regardless of zocs.
    if (sq->is_base()) {
        if (sq->owner == faction) {
            return true;
        } else if (!combat && !has_pact(faction, sq->owner)) {
            return false;
        }
    }
    if (!combat && mod_zoc_move(x, y, faction)) {
        return false;
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->x == x && veh->y == y
        && veh->faction_id != faction && !has_pact(faction, veh->faction_id)) {
            return false;
        }
    }
    return true;
}

int garrison_goal(int x, int y, int faction, int triad) {
    assert(triad == TRIAD_LAND || triad == TRIAD_SEA);
    AIPlans& p = plans[faction];
    if (triad == TRIAD_LAND && p.transport_units > 0
    && p.naval_start_x == x && p.naval_start_y == y) {
        return clamp(p.land_combat_units/64 + p.transport_units, 4, 12);
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->x == x && base->y == y) {
            int goal = clamp((p.land_combat_units + p.sea_combat_units)
                * max(1, (int)base->defend_goal) / max(1, p.defend_weights)
                - (triad == TRIAD_SEA ? 2 : 0)
                - (p.unknown_factions > 1 && p.contacted_factions < 2)
                - random(2), 1, 5);
            debug("garrison_goal %2d %2d %2d %s\n", x, y, goal, base->name);
            return goal;
        }
    }
    assert(0);
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
    return (num > 1 ? (num+1)/2 : 0);
}

/*
For artillery units, Thinker tries to first determine the best stackup location
before deciding any attack targets. This important heuristic chooses
squares that have the most friendly AND enemy units in 2-tile range.
It is also used to prioritize aircraft attack targets.
*/
int arty_value(int x, int y) {
    return mapdata[{x, y}].unit_near * mapdata[{x, y}].enemy_near;
}

/*
Assign some predefined proportion of units into the active naval invasion plan.
Thinker chooses these units randomly based on the Veh ID modulus. Even though
the IDs are not permanent, it is not a problem for the movement planning.
*/
bool invasion_unit(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    AIPlans& p = plans[veh->faction_id];
    MAP* sq;

    if (p.naval_start_x < 0 || !(sq = mapsq(veh->x, veh->y))) {
        return false;
    }
    if (veh->triad() == TRIAD_AIR) {
        return (veh_id % 16) > (p.prioritize_naval ? 8 : 11);
    }
    if (veh->triad() == TRIAD_SEA) {
        return (veh_id % 16) > (p.prioritize_naval ? 8 : 11)
            && (ocean_coast_tiles(veh->x, veh->y)
            || region_at(veh->x, veh->y) == region_at(p.naval_end_x, p.naval_end_y));
    }
    return (veh_id % 16) > (p.prioritize_naval ? 8 : 11) + (p.enemy_bases ? 2 : 0)
        && sq->region == p.main_region
        && sq->owner == veh->faction_id;
}

bool near_landing(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    if (mapnodes.count({veh->x, veh->y, NODE_NAVAL_END})
    || mapnodes.count({veh->x, veh->y, NODE_NAVAL_PICK})
    || mapnodes.count({veh->x, veh->y, NODE_SCOUT_SITE})) {
        return true;
    }
    return false;
}

int make_landing(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    int best_score = 0;
    int tx = -1;
    int ty = -1;
    for (auto& m : iterate_tiles(veh->x, veh->y, 1, 9)) {
        if (!allow_move(m.x, m.y, veh->faction_id, TRIAD_LAND)
        || has_pact(veh->faction_id, m.sq->owner)) {
            continue;
        }
        if (veh->is_combat_unit()
        && !mapnodes.count({m.x, m.y, NODE_SCOUT_SITE})
        && !mapnodes.count({m.x, m.y, NODE_NAVAL_BEACH})
        && (random(8) && Continents[m.sq->region].pods <= Continents[m.sq->region].tile_count/32)) {
            continue;
        }
        int score = 16*mapnodes.count({m.x, m.y, NODE_NAVAL_BEACH})
            + 4*(veh->is_colony() && !m.sq->is_owned())
            + min(8, Continents[m.sq->region].pods) + random(8);
        if (score > best_score) {
            tx = m.x;
            ty = m.y;
            best_score = score;
        }
    }
    if (tx >= 0) {
        debug("make_landing %2d %2d -> %2d %2d %s\n", veh->x, veh->y, tx, ty, veh->name());
        return set_move_to(veh_id, tx, ty); // Save Private Ryan
    }
    return VEH_SYNC;
}

int trans_move(const int id) {
    VEH* veh = &Vehs[id];
    MAP* sq = mapsq(veh->x, veh->y);
    AIPlans& p = plans[veh->faction_id];
    Faction* f = &Factions[veh->faction_id];
    if (!sq || (sq->is_base() && veh->need_heals())) {
        return mod_veh_skip(id);
    }
    int cargo = 0;
    int nearby = 0;
    int artifact = 0;
    int capacity = veh_cargo(id);
    int at_base = sq->is_base() && sq->owner == veh->faction_id;
    int tx = -1;
    int ty = -1;
    TileSearch ts;

    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehs[i];
        if (veh->faction_id == v->faction_id && v->triad() == TRIAD_LAND && i != id) {
            if (veh->x == v->x && veh->y == v->y) {
                if (v->order == ORDER_SENTRY_BOARD && v->waypoint_1_x == id) {
                    cargo++;
                    if (v->is_artifact()) {
                        artifact++;
                    }
                    if (at_base && !mapnodes.count({veh->x, veh->y, NODE_NAVAL_START})) {
                        mod_veh_wake(i);
                    }
                }
            } else if (map_range(veh->x, veh->y, v->x, v->y) == 1) {
                nearby++;
            }
        }
    }
    debug("trans_move %2d %2d cargo: %d artifact: %d nearby: %d capacity: %d %s\n",
    veh->x, veh->y, cargo, artifact, nearby, veh_cargo(id), veh->name());

    if (cargo < capacity && mapnodes.erase({veh->x, veh->y, NODE_NEED_FERRY}) > 0) {
        return mod_veh_skip(id);
    }
    if (!veh->at_target()) {
        if (artifact || (cargo
        && (mapnodes.count({veh->waypoint_1_x, veh->waypoint_1_y, NODE_NAVAL_END})
        || mapnodes.count({veh->waypoint_1_x, veh->waypoint_1_y, NODE_NAVAL_PICK})
        || mapnodes.count({veh->waypoint_1_x, veh->waypoint_1_y, NODE_NEED_FERRY})
        || mapnodes.count({veh->waypoint_1_x, veh->waypoint_1_y, NODE_SCOUT_SITE})))) {
            return VEH_SYNC; // Continue previous orders
        }
        if (!cargo && mapnodes.count({veh->waypoint_1_x, veh->waypoint_1_y, NODE_NAVAL_START})) {
            return VEH_SYNC;
        }
    }
    if (mapnodes.count({veh->x, veh->y, NODE_NAVAL_PICK})) {
        if (!artifact && cargo < capacity) {
            if (veh->iter_count < 2) {
                return VEH_SYNC;
            } else if (nearby) {
                return mod_veh_skip(id);
            }
        }
        if (artifact || !nearby) {
            mapnodes.erase({veh->x, veh->y, NODE_NAVAL_PICK});
        }
        if (cargo >= capacity && search_route(ts, id, &tx, &ty)) {
            debug("trans_drop %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
            return set_move_to(id, tx, ty);
        }
    }
    if (!at_base) {
        if (cargo > artifact && near_landing(id)) {
            for (int i = 0; i < *VehCount; i++) {
                VEH* v = &Vehs[i];
                if (veh->x == v->x && veh->y == v->y && i != id
                && v->triad() == TRIAD_LAND && !v->is_artifact()) {
                    make_landing(i);
                }
            }
            if (veh->iter_count < 2) {
                return VEH_SYNC;
            }
            return mod_veh_skip(id);
        }
        if (mapdata[{veh->x, veh->y}].safety < PM_SAFE
        && !mapnodes.count({veh->x, veh->y, NODE_NAVAL_PICK})
        && !mapnodes.count({veh->x, veh->y, NODE_NAVAL_END})
        && map_range(veh->x, veh->y, p.naval_end_x, p.naval_end_y) > 3) {
            return escape_move(id);
        }
    }
    if (at_base && mapnodes.count({veh->x, veh->y, NODE_NAVAL_START})) {
        for (int i = 0; i < *VehCount; i++) {
            if (cargo >= capacity) {
                break;
            }
            VEH* v = &Vehs[i];
            if (veh->faction_id == v->faction_id
            && veh->x == v->x && veh->y == v->y
            && (v->is_combat_unit() || v->is_probe())
            && v->triad() == TRIAD_LAND && v->order != ORDER_SENTRY_BOARD) {
                set_board_to(i, id);
                cargo++;
            }
        }
        if (cargo >= min(4, capacity)) {
            debug("trans_invade %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
            return set_move_to(id, p.naval_end_x, p.naval_end_y);
        }
        if (veh->iter_count < 2) {
            return VEH_SYNC;
        }
        return mod_veh_skip(id);
    }

    int best_score = INT_MIN;
    int max_dist = ((*CurrentTurn + id) % 4 ? 8 : 16) + (artifact ? 20 : 0);

    if (mapnodes.count({veh->x, veh->y, NODE_NAVAL_START})) {
        max_dist = 4;
    }
    if (cargo > artifact && invasion_unit(id)
    && sq->region == region_at(p.naval_end_x, p.naval_end_y)) {
        max_dist = 4;
    }
    ts.init(veh->x, veh->y, TS_SEA_AND_SHORE);

    while ((sq = ts.get_next()) != NULL && ts.dist <= max_dist) {
        bool own_base = sq->is_base() && sq->owner == veh->faction_id;
        bool can_move = own_base || allow_move(ts.rx, ts.ry, veh->faction_id, TRIAD_SEA);
        if (!can_move) {
            continue;
        }
        if (ts.dist <= 2 && !veh->need_heals() && goody_at(ts.rx, ts.ry)) {
            debug("trans_heals %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
            return set_move_to(id, ts.rx, ts.ry);
        }
        if (own_base) {
            if (artifact || veh->need_heals() || (cargo > artifact && p.naval_end_x < 0)) {
                if (artifact && can_link_artifact(base_at(ts.rx, ts.ry))) {
                    debug("trans_link %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
                    return set_move_to(id, ts.rx, ts.ry);
                }
                int score = (sq->is_land_region() ? 4 : 1)
                    * f->region_total_bases[sq->region] - ts.dist;
                if (score > best_score) {
                    tx = ts.rx;
                    ty = ts.ry;
                    best_score = score;
                }
            }
        } else if (!artifact) {
            if (tx < 0 && allow_scout(veh->faction_id, sq)
            && mapdata[{ts.rx, ts.ry}].safety > PM_SAFE && ts.dist < random(16)) {
                tx = ts.rx;
                ty = ts.ry;
            }
            if (mapnodes.count({ts.rx, ts.ry, NODE_SCOUT_SITE})
            && !at_base && cargo > 0 && cargo < 4 && ts.dist < random(16)) {
                for (auto& m : iterate_tiles(ts.rx, ts.ry, 1, 9)) {
                    if (!is_ocean(m.sq) && mapnodes.count({m.x, m.y, NODE_PATROL})) {
                        debug("trans_scout %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
                        return set_move_to(id, ts.rx, ts.ry);
                    }
                }
            }
            if (mapnodes.count({ts.rx, ts.ry, NODE_PATROL})) {
                debug("trans_patrol %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
                return set_move_to(id, ts.rx, ts.ry);
            }
            if (mapnodes.count({ts.rx, ts.ry, NODE_NAVAL_END})
            && cargo && tx < 0 && ts.dist > 3 && ts.dist < random(32)) {
                tx = ts.rx;
                ty = ts.ry;
            }
        }
        if (mapnodes.count({ts.rx, ts.ry, NODE_NEED_FERRY})
        && !cargo && mapdata[{ts.rx, ts.ry}].target <= random(8)) {
            debug("trans_ferry %2d %2d -> %2d %2d\n", veh->x, veh->y, ts.rx, ts.ry);
            return set_move_to(id, ts.rx, ts.ry);
        }
        if (mapnodes.count({ts.rx, ts.ry, NODE_NAVAL_START})
        && !cargo && tx < 0 && ts.dist > 3 && ts.dist < random(32)) {
            tx = ts.rx;
            ty = ts.ry;
        }
    }
    if (tx >= 0) {
        debug("trans_move %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
        return set_move_to(id, tx, ty);
    }
    if (mapnodes.count({veh->x, veh->y, NODE_NAVAL_START})) {
        return mod_veh_skip(id);
    }
    if (!cargo && mapnodes.count({veh->x, veh->y, NODE_NAVAL_END})) {
        debug("trans_start %2d %2d -> %2d %2d\n", veh->x, veh->y, p.naval_start_x, p.naval_start_y);
        return set_move_to(id, p.naval_start_x, p.naval_start_y);
    }
    if (!cargo && p.naval_start_x >= 0 && invasion_unit(id)
    && cargo_capacity(p.naval_start_x, p.naval_start_y, veh->faction_id) < 16 + random(32)) {
        debug("trans_start %2d %2d -> %2d %2d\n", veh->x, veh->y, p.naval_start_x, p.naval_start_y);
        return set_move_to(id, p.naval_start_x, p.naval_start_y);
    }
    if (!at_base && search_route(ts, id, &tx, &ty)) {
        debug("trans_route %2d %2d -> %2d %2d\n", veh->x, veh->y, tx, ty);
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
    assert(moves > 0 && moves <= mov_rate);
    return max(1, power * (10 - damage) * moves / mov_rate);
}

int flank_score(int x, int y, MAP* sq) {
    return random(16) - 4*mapdata[{x, y}].enemy_dist
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
        + (stack_damage ? 0.03 * mapdata[{veh2->x, veh2->y}].enemy : 0.0)
        + min(12, abs(offense_value(veh2->unit_id))) * (u2->speed() > 1 ? 0.02 : 0.01)
        + (triad == TRIAD_AIR ? 0.001 * min(400, arty_value(veh2->x, veh2->y)) : 0.0)
        - (neutral_tile ? 0.06 : 0.01)*dist;
    /*
    Fix: in rare cases the game engine might reject valid attack orders for unknown reason.
    In this case combat_move would repeat failed attack orders until iteration limit.
    */
    double v3 = min(!offense_value(veh2->unit_id) ? 1.3 : 1.6, v1) + clamp(v2, -0.5, 0.5)
        - 0.04*mapdata[{veh2->x, veh2->y}].target;

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
        debug("aircraft_invade %2d %2d -> %2d %2d\n",
            veh->x, veh->y, p.naval_airbase_x, p.naval_airbase_y);
        return set_move_to(id, p.naval_airbase_x, p.naval_airbase_y);
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
        debug("aircraft_rebase %2d %2d -> %2d %2d score: %d\n",
            veh->x, veh->y, tx, ty, best_score);
        return set_move_to(id, tx, ty);
    }
    if (!at_base && (u->chassis_id != CHS_GRAVSHIP || !random(8))) {
        return move_to_base(id, true);
    }
    if (at_base && has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)
    && !veh->high_damage() && random(2)) {
        return set_order_none(id);
    }
    return mod_veh_skip(id);
}

bool airdrop_move(const int id, MAP* sq) {
    VEH* veh = &Vehicles[id];
    if (!can_airdrop(id, sq)) {
        return false;
    }
    int faction = veh->faction_id;
    int max_range = max(Rules->max_airdrop_rng_wo_orbital_insert,
        (has_orbital_drops(faction) ? random(64) : 0));
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
        && base_range >= 3
        && allow_airdrop(base->x, base->y, faction, true, sq)) {
            if (allow_defend) {
                int enemy_diff = mapdata[{base->x, base->y}].enemy_near
                    - mapdata[{veh->x, veh->y}].enemy_near;
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
                    - 2*mapdata[{base->x, base->y}].target;
                if (score > best_score) {
                    tx = base->x;
                    ty = base->y;
                    best_score = score;
                }
            }
            else if (allow_attack && veh_stack(base->x, base->y) < 0
            && base_range/4 < 2 + mapdata[{base->x, base->y}].unit_near
            && (base_range < 8 || sq->is_visible(faction))) {
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
        mapdata[{tx, ty}].target++;
        action_airdrop(id, tx, ty, 3);
        return true;
    }
    return false;
}

int combat_move(const int id) {
    VEH* veh = &Vehicles[id];
    MAP* sq = mapsq(veh->x, veh->y);
    MAP* veh_sq = sq;
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
    int max_dist = clamp(random(4 + 4*(veh->speed() > 1))
        + (at_base ? 0 : 2)
        + (at_enemy ? -4 : 0)
        + (veh_sq->is_owned() ? 0 : 2)
        + (veh->need_heals() ? -6 : 0)
        + (p.unknown_factions / 2)
        + (p.contacted_factions ? 0 : 4),
        (veh->high_damage() ? 2 : 3), 8 + 4*(veh->speed() > 1)
    );
    if (veh_sq->is_airbase() && airdrop_move(id, veh_sq)) {
        return VEH_SKIP;
    }
    if (triad == TRIAD_LAND && mapnodes.count({veh->x, veh->y, NODE_NAVAL_END})) {
        return make_landing(id);
    }
    if (triad == TRIAD_LAND && is_ocean(veh_sq) && at_base && !has_transport(veh->x, veh->y, faction)) {
        return mod_veh_skip(id);
    }
    if (veh->is_probe() && veh->need_heals()) {
        return escape_move(id);
    }
    if (!veh->at_target() && veh->iter_count < 2) {
        if (!mapdata[{veh->x, veh->y}].enemy_near && !veh->need_heals()) {
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
        defend = (at_home && (*CurrentTurn + id) % 8 < 3 && !veh->is_probe())
            || (!reg_enemy_at(veh_sq->region, veh->is_probe())
            && !reg_enemy_at(p.main_sea_region, veh->is_probe()));
    } else {
        defend = (at_home && (*CurrentTurn + id) % 8 < 4)
            || (veh->is_probe() && veh->speed() < 2)
            || !reg_enemy_at(veh_sq->region, veh->is_probe());
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
        - 0.008*min(50, mapdata[{veh->x, veh->y}].unit_near)
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
        && mapdata[{ts.rx, ts.ry}].target < 2 + random(16)) {
            return set_move_to(id, ts.rx, ts.ry);

        } else if (arty && veh->iter_count < 2
        && arty_value(ts.rx, ts.ry) - 4*ts.dist > best_score
        && allow_move(ts.rx, ts.ry, faction, triad)) {
            tx = ts.rx;
            ty = ts.ry;
            best_score = arty_value(ts.rx, ts.ry) - 4*ts.dist;

        } else if (tx < 0 && attack && mapnodes.count({ts.rx, ts.ry, NODE_COMBAT_PATROL})
        && ts.dist <= (at_base ? 1 + min(3, defenders/4) : 3)
        && path_cost(veh->x, veh->y, ts.rx, ts.ry, veh->unit_id, faction, moves) >= 0) {
            return set_move_to(id, ts.rx, ts.ry);

        } else if (tx < 0 && mapnodes.count({ts.rx, ts.ry, NODE_PATROL})) {
            return set_move_to(id, ts.rx, ts.ry);

        } else if (px < 0 && allow_scout(faction, sq)) {
            px = ts.rx;
            py = ts.ry;
        }
    }
    if (veh->is_probe() && mapdata[{veh->x, veh->y}].enemy_dist != 1) {
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
                    int value = probe(id, -1, id2, 1);
                    debug("combat_probe %2d %2d -> %2d %2d cost: %d value: %d %s %s\n",
                        veh->x, veh->y, ts.rx, ts.ry, reserve - f->energy_credits,
                        value, veh->name(), Vehs[id2].name());
                    if (value || num != *VehCount) {
                        return VEH_SKIP;
                    }
                    return VEH_SYNC;
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
        for (i = 1; i < TableRange[arty_range(veh->unit_id)]; i++) {
            int x2 = wrap(veh->x + TableOffsetX[i]);
            int y2 = veh->y + TableOffsetY[i];
            if ((sq = mapsq(x2, y2)) && mapdata[{x2, y2}].enemy > 0) {
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
                    veh->x, veh->y, x2, y2, score, veh->name());
                if (score > best_score) {
                    best_score = score;
                    offset = i;
                    tx = x2;
                    ty = y2;
                }
            }
        }
        if (tx >= 0 && ((at_base && !defenders) || random(512) < min(420, best_score))) {
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
            best_score = 128*b->defend_goal - mapdata[{b->x, b->y}].safety + random(256);
        }
        for (base_id = 0; source >= 0 && base_id < *BaseCount; base_id++) {
            b = &Bases[base_id];
            if (b->faction_id == veh->faction_id && source != base_id
            && has_fac_built(FAC_PSI_GATE, base_id)
            && ((triad == TRIAD_LAND && !is_ocean(b))
            || (triad == TRIAD_SEA && coast_tiles(b->x, b->y)))) {
                int score = 128*b->defend_goal - mapdata[{b->x, b->y}].safety
                    - 16*mapdata[{b->x, b->y}].target;
                if (score > best_score) {
                    best_score = score;
                    target = base_id;
                }
            }
        }
        if (target >= 0) {
            debug("action_gate %2d %2d %s -> %s\n", veh->x, veh->y, veh->name(), Bases[target].name);
            mapdata[{Bases[target].x, Bases[target].y}].target++;
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
    bool check_zocs = !ignore_zocs && mapdata[{veh->x, veh->y}].enemy_near;
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
            > defender_count(ts.rx, ts.ry, -1) + mapdata[{ts.rx, ts.ry}].target/2) {
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
        bool flank = !defend && path.dist > 1 && random(12) < min(8, mapdata[{tx, ty}].target);
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
        } else if (!defend && !veh->is_probe() && path.prev > 0) {
            PathNode& prev = ts.paths[path.prev];
            if (path.dist > 3 && random(10) < mapdata[{tx, ty}].enemy_near) {
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
    if (!base_found || !veh_sq->is_owned()) {
        if (triad == TRIAD_LAND && search_route(ts, id, &tx, &ty)) {
            debug("combat_route %2d %2d -> %2d %2d %s\n", veh->x, veh->y, tx, ty, veh->name());
            return set_move_to(id, tx, ty);
        }
    }
    if (!base_found && triad == TRIAD_SEA && !random(4)) {
        return mod_veh_kill(id);
    }
    if (base_found && !at_base && !random(4)) {
        return move_to_base(id, true);
    }
    return mod_veh_skip(id);
}



