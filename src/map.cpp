#include "map.h"

static Points spawns;
static Points natives;
static Points goodtiles;


void process_map(int faction, int k) {
    spawns.clear();
    natives.clear();
    goodtiles.clear();
    /*
    How many tiles a particular region has. Regions are disjoint land/water areas
    and the partitions are already calculated by the game engine.
    */
    int region_count[MaxRegionNum] = {};
    /*
    This value defines how many tiles an area needs to have before it's
    considered sufficiently large to be a faction starting location.
    Map area square root values: Tiny = 33, Standard = 56, Huge = 90
    */
    int limit = clamp(*map_area_tiles, 1024, 8192) / 40;

    for (int y = 0; y < *map_axis_y; y++) {
        for (int x = y&1; x < *map_axis_x; x+=2) {
            MAP* sq = mapsq(x, y);
            if (sq) {
                assert(sq->region >= 0 && sq->region < MaxRegionNum);
                region_count[sq->region]++;
            }
        }
    }
    for (int y = 0; y < *map_axis_y; y++) {
        for (int x = y&1; x < *map_axis_x; x+=2) {
            if (y < k || y >= *map_axis_y - k) {
                continue;
            }
            MAP* sq = mapsq(x, y);
            if (!is_ocean(sq) && !sq->is_rocky() && ~sq->items & BIT_FUNGUS
            && !(sq->landmarks & ~LM_FRESH) && region_count[sq->region] >= limit) {
                goodtiles.insert({x, y});
            }
        }
    }
    for (int i = 0; i < *total_num_vehicles; i++) {
        VEH* v = &Vehicles[i];
        if (v->faction_id == 0) {
            natives.insert({v->x, v->y});
        }
        if (v->faction_id != 0 && v->faction_id != faction) {
            spawns.insert({v->x, v->y});
        }
    }
    if ((int)goodtiles.size() < *map_area_sq_root * 8) {
        goodtiles.clear();
    }
    debug("process_map x: %d y: %d sqrt: %d tiles: %d good: %d\n",
        *map_axis_x, *map_axis_y, *map_area_sq_root, *map_area_tiles, goodtiles.size());
}

bool valid_start(int faction, int iter, int x, int y, bool aquatic, bool need_bonus) {
    MAP* sq = mapsq(x, y);
    int limit = (*map_area_sq_root < 40 ? max(5, 8 - iter/50) : 8);
    int min_sc = 80 - iter/4;
    int pods = 0;
    int sea = 0;
    int sc = 0;
    int xd = 0;
    int yd = 0;

    if (!sq || sq->items & BIT_BASE_DISALLOWED || (sq->is_rocky() && !is_ocean(sq))) {
        return false;
    }
    // LM_FRESH landmark is incorrectly used on some maps
    if (aquatic != is_ocean(sq) || (sq->landmarks & ~LM_FRESH && iter < 250)) {
        return false;
    }
    if ((goody_at(x, y) > 0 || bonus_at(x, y) > 0) && iter < 200) {
        return false;
    }
    if (min_range(natives, x, y) < max(4, 8 - iter/32)) {
        return false;
    }
    if (min_range(spawns, x, y) < max(limit, *map_area_sq_root/4 + 8 - iter/8)) {
        return false;
    }
    if (!aquatic) {
        for (int i = 2; i < 20; i+=2) {
            if (is_ocean(mapsq(wrap(x-i), y)) || is_ocean(mapsq(wrap(x+i), y))) {
                break;
            }
            xd++;
        }
        for (int i = 2; i < 20; i+=2) {
            if (is_ocean(mapsq(x, y-i)) || is_ocean(mapsq(x, y+i))) {
                break;
            }
            yd++;
        }
    }
    for (int i = 0; i < 45; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if ((sq = mapsq(x2, y2))) {
            int bonus = bonus_at(x2, y2);
            if (is_ocean(sq)) {
                if (!is_ocean_shelf(sq)) {
                    sc--;
                } else if (aquatic) {
                    sc += 4;
                }
                sea++;
            } else {
                sc += (sq->is_rainy_or_moist() ? 3 : 1);
                if (sq->items & BIT_RIVER) {
                    sc += 5;
                }
                if (sq->items & BIT_FOREST) {
                    sc += 4;
                }
                if (sq->is_rolling()) {
                    sc += 1;
                }
            }
            if (bonus != RES_NONE) {
                if (i <= 20 && bonus == RES_NUTRIENT && !is_ocean(sq)) {
                    pods++;
                }
                sc += (i <= 20 ? 15 : 10);
            }
            if (goody_at(x2, y2) > 0) {
                sc += 15;
                if (!is_ocean(sq)) {
                    pods++;
                }
            }
            if (sq->items & BIT_FUNGUS) {
                sc -= (i <= 20 ? 4 : 2) * (is_ocean(sq) ? 1 : 2);
            }
            if (sq->items & BIT_MONOLITH) {
                sc += 8;
            }
        }
    }
    debug("find_score %d %d x: %3d y: %3d xd: %d yd: %d pods: %d min: %d score: %d\n",
        faction, iter, x, y, xd, yd, pods, min_sc, sc);

    if (need_bonus && pods < 2 && iter < 150 && ~*game_rules & RULES_NO_UNITY_SCATTERING) {
        return false;
    }
    if (!aquatic && iter < 100) { // Avoid spawns without sufficient land nearby
        if (sea > 20) {
            return false;
        }
        if (max(xd, yd) < 4) {
            return false;
        }
    }
    return sc >= min_sc;
}

void apply_nutrient_bonus(int faction, int* x, int* y) {
    MAP* sq;
    Points pods;
    Points rivers;
    bool adjust = true;
    int nutrient = 0;
    int num = 0;

    for (int i = 0; i < 45; i++) {
        int x2 = wrap(*x + TableOffsetX[i]);
        int y2 = *y + TableOffsetY[i];
        if ((sq = mapsq(x2, y2)) && !is_ocean(sq) && (i <= 20 || pods.size() < 2)) {
            int bonus = bonus_at(x2, y2);
            if (goody_at(x2, y2) > 0) {
                pods.insert({x2, y2});
            } else if (bonus == RES_MINERAL || bonus == RES_ENERGY) {
                if (nutrient + pods.size() < 2) {
                    pods.insert({x2, y2});
                }
            } else if (bonus == RES_NUTRIENT) {
                nutrient++;
            } else if (sq->items & BIT_RIVER) {
                if (i == 0) {
                    adjust = false;
                }
                if (adjust && i <= 20 && sq->region == region_at(*x, *y)
                && can_build_base(x2, y2, faction, TRIAD_LAND)) {
                    rivers.insert({x2, y2});
                }
            }
        }
    }
    while (pods.size() > 0 && nutrient + num < 2) {
        Points::const_iterator it(pods.begin());
        std::advance(it, random(pods.size()));
        sq = mapsq(it->x, it->y);
        pods.erase(it);
        sq->items &= ~(BIT_FUNGUS | BIT_MINERAL_RES | BIT_ENERGY_RES);
        sq->items |= (BIT_SUPPLY_REMOVE | BIT_BONUS_RES | BIT_NUTRIENT_RES);
        num++;
    }
    if (rivers.size() > 0 && *map_area_sq_root > 30) { // Adjust position to adjacent river
        Points::const_iterator it(rivers.begin());
        std::advance(it, random(rivers.size()));
        *x = it->x;
        *y = it->y;
    }
}

void __cdecl find_start(int faction, int* tx, int* ty) {
    bool aquatic = MFactions[faction].rule_flags & RFLAG_AQUATIC;
    bool need_bonus = !aquatic && conf.nutrient_bonus > is_human(faction);
    int x = 0;
    int y = 0;
    int i = 0;
    int k = (*map_axis_y < 80 ? 4 : 8);
    process_map(faction, k/2);

    while (++i <= 300) {
        if (!aquatic && goodtiles.size() > 0 && i <= 200) {
            Points::const_iterator it(goodtiles.begin());
            std::advance(it, random(goodtiles.size()));
            x = it->x;
            y = it->y;
        } else {
            y = (random(*map_axis_y - k*2) + k);
            x = (random(*map_axis_x) &~1) + (y&1);
        }
        debug("find_iter  %d %d x: %3d y: %3d\n", faction, i, x, y);
        if (valid_start(faction, i, x, y, aquatic, need_bonus)) {
            if (need_bonus) {
                apply_nutrient_bonus(faction, &x, &y);
            }
            *tx = x;
            *ty = y;
            break;
        }
    }
    site_set(*tx, *ty, world_site(*tx, *ty, 0));
    debug("find_start %d %d x: %3d y: %3d range: %d\n", faction, i, *tx, *ty, min_range(spawns, *tx, *ty));
    fflush(debug_log);
}

