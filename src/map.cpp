#include "map.h"


static Points spawns;
static Points natives;
static Points goodtiles;

int* WorldAddTemperature = (int*)0x9B22E8;
int* WorldSkipTerritory = (int*)0x9B22EC;


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
    if (aquatic != is_ocean(sq) || (sq->landmarks & ~LM_FRESH && iter < 150)) {
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

float map_fractal(FastNoiseLite& noise, int x, int y) {
    float val = 1.0f * noise.GetNoise(2.0f*x, 1.5f*y)
        + 0.4f * (2 + *MapErosiveForces) * noise.GetNoise(-6.0f*x, -4.0f*y);
    if (x < 8 && *map_axis_x >= 32 && !(*map_toggle_flat & 1)) {
        val = x/8.0f * val + (1.0f - x/8.0f) * map_fractal(noise, *map_axis_x - x, y);
    }
    return val;
}

void __cdecl mod_world_monsoon() {
    typedef struct {
        uint8_t valid;
        uint8_t valid_near;
        uint8_t sea;
        uint8_t sea_near;
    } TileInfo;
    TileInfo* tiles = new TileInfo[*map_axis_x * *map_axis_y]{};
    world_rainfall();

    MAP* sq;
    int i = 0, j = 0, x = 0, y = 0, x2 = 0, y2 = 0, num = 0;
    const int w = *map_axis_x;
    const int y_a = *map_axis_y*3/8;
    const int y_b = *map_axis_y*5/8;
    const int limit = max(1024, *map_area_tiles) * (2 + *MapCloudCover) / 100;

    for (y = 0; y < *map_axis_y; y++) {
        for (x = y&1; x < *map_axis_x; x+=2) {
            if (!(sq = mapsq(x, y))) {
                continue;
            }
            if (is_ocean(sq)) {
                tiles[w*y + x].sea = Continents[sq->region].tile_count > 15;
                continue;
            }
            if (sq->alt_level() == ALT_SHORE_LINE || sq->alt_level() == ALT_ONE_ABOVE_SEA) {
                tiles[w*y + x].valid = !sq->landmarks;
            }
        }
    }
    for (y = 0; y < *map_axis_y; y++) {
        for (x = y&1; x < *map_axis_x; x+=2) {
            if (tiles[w*y + x].valid) {
                for (auto& p : iterate_tiles(x, y, 0, 45)) {
                    if (tiles[w*p.y + p.x].valid) {
                        tiles[w*y + x].valid_near++;
                    }
                    if (tiles[w*p.y + p.x].sea) {
                        tiles[w*y + x].sea_near++;
                    }
                }
            }
        }
    }
    for (i = 0; i < 256 && num < limit; i++) {
        y = (random(y_b - y_a) + y_a);
        x = wrap((random(*map_axis_x) &~1) + (y&1));
        if (!tiles[w*y + x].valid
        || tiles[w*y + x].sea_near < 8 - i/16
        || tiles[w*y + x].valid_near < 16 - i/32) {
            continue;
        }
        int goal = num + max(21, limit/4);
        for (j = 0; j < 32 && num < goal && num < limit; j++) {
            if (j % 4 == 0) {
                y2 = y;
                x2 = x;
            }
            y2 = y2 + random(8);
            x2 = wrap(((x2 + random(8)) &~1) + (y2&1));
            if (y2 >= y_a && y2 <= y_b && tiles[w*y2 + x2].valid
            && tiles[w*y2 + x2].valid_near > 8 - i/32) {
                for (auto& p : iterate_tiles(x2, y2, 0, 21)) {
                    if (tiles[w*p.y + p.x].valid && ~p.sq->landmarks & LM_JUNGLE) {
                        assert(!is_ocean(p.sq));
                        bit2_set(p.x, p.y, LM_JUNGLE, 1);
                        code_set(p.x, p.y, num % 121);
                        num++;
                    }
                }
            }
        }
    }
    delete[] tiles;
}

void __cdecl mod_world_build() {
    if (!conf.new_world_builder) {
        world_build();
        return;
    }
    if (DEBUG) {
        memset(pm_overlay, 0, sizeof(pm_overlay));
        *game_state |= STATE_DEBUG_MODE;
    }
    memcpy(AltNatural, AltNaturalDefault, 0x2Cu);
    *WorldAddTemperature = 1;
    *WorldSkipTerritory = 1;
    for (int i = 0; i < 7; i++) {
        if (MapSizePlanet[i] < 0) {
            MapSizePlanet[i] = random(3);
        }
    }
    map_wipe();
    if (*game_state & STATE_OMNISCIENT_VIEW) {
        MapWin_clear_terrain(MapWin);
        draw_map(1);
    }
    assert(*MapOceanCoverage >= 0 && *MapOceanCoverage < 3);
    MAP* sq;
    uint32_t seed = GetTickCount();
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(seed);
    srand(seed);
    *map_random_seed = (seed % 0x7fff) + 1; // Must be non-zero

    Points conts;
    size_t continents = clamp(*map_area_sq_root / 12, 4, 20);
    int levels[256] = {};
    int x = 0, y = 0, i = 0;

    if (conf.world_continents && *map_axis_y >= 32) {
        while (++i < 100 && conts.size() < continents) {
            y = (random(*map_axis_y - 16) + 8);
            x = (random(*map_axis_x) &~1) + (y&1);
            if (i & 1 && min_vector(conts, x, y) < *map_area_sq_root/5) {
                conts.insert({x, y});
            }
            if (~i & 1 && min_vector(conts, x, y) >= *map_area_sq_root/3) {
                conts.insert({x, y});
            }
        }
    }

    const float L = AltNatural[ALT_SHORE_LINE]; // Default shoreline altitude = 45
    const float Wmid = (50 - conf.world_sea_levels[*MapOceanCoverage])*0.01f;
    const float Wsea = 0.8f + 0.04f * clamp(WorldBuilder->deep_water - WorldBuilder->shelf, -16, 16);
    const float Wland = 0.5f + 0.25f * (*MapErosiveForces);
    const float Wdist = 1.0f / clamp(*map_area_sq_root * 0.1f, 1.0f, 15.0f);

    for (y = 0; y < *map_axis_y; y++) {
        float Wcaps = 1.0f - min(1.0f, (min(y, *map_axis_y - y) / (max(1.0f, *map_axis_y * 0.2f))));

        for (x = y&1; x < *map_axis_x; x+=2) {
            float Wcont = 1.5f - clamp(min_vector(conts, x, y) * Wdist, 0.75f, 1.5f);
            float value = map_fractal(noise, x, y) + Wmid - 0.5f*Wcaps;
            if (value > 0) {
                value = value * Wland;
            } else {
                value = value * Wsea;
            }
            if (conf.world_continents && value < 0.2f) {
                value += Wcont * Wland;
                if (DEBUG) pm_overlay[x][y] = (int)(10*Wcont);
            }
            sq = mapsq(x, y);
            sq->contour = clamp((int)(L + L*value), 0, 255);
            levels[sq->contour]++;
        }
    }

    int level_sum = 0;
    int level_mod = 0;
    for (i = 0; i < 256; i++) {
        level_sum += levels[i];
        if (level_sum >= *map_area_tiles * conf.world_sea_levels[*MapOceanCoverage] / 100) {
            level_mod = AltNatural[ALT_SHORE_LINE] - i;
            break;
        }
    }
    for (y = 0; y < *map_axis_y; y++) {
        for (x = y&1; x < *map_axis_x; x+=2) {
            sq = mapsq(x, y);
            sq->contour = clamp(sq->contour + level_mod, 0, 255);
        }
    }
    debug("world_build seed: %10u size: %d x: %d y: %d "\
    "ocean: %d erosion: %d cloud: %d deep: %d sea_level: %d level_mod: %d\n",
    seed, *MapSizePlanet, *map_axis_x, *map_axis_y, *MapOceanCoverage, *MapErosiveForces, *MapCloudCover,
    WorldBuilder->deep_water - WorldBuilder->shelf, conf.world_sea_levels[*MapOceanCoverage], level_mod);

    world_polar_caps();
    world_linearize_contours();
    world_shorelines();
    world_validate(); // Run Path::continents
    Points bridges;

    for (y = 3; y < *map_axis_y - 3; y++) {
        for (x = y&1; x < *map_axis_x; x+=2) {
            sq = mapsq(x, y);
            if (is_ocean(sq)) {
                continue;
            }
            uint64_t sea = 0;
            int land_count = Continents[sq->region].tile_count;
            if (land_count <= 4 || (land_count <= 24
            && pair_hash(seed, sq->region) & 0x3f > WorldBuilder->islands - 2)) {
                // islands=1 removes all continents under size 25
                bridges.insert({x, y});
                if (DEBUG) pm_overlay[x][y] = -1;
                continue;
            }
            for (auto& m : iterate_tiles(x, y, 1, 13)) {
                if (is_ocean(m.sq)) {
                    if (Continents[m.sq->region].tile_count >= 30) {
                        sea |= 1 << (m.sq->region & 0x3f);
                    }
                }
            }
            if (__builtin_popcount(sea) > 1) {
                if (land_count > *map_area_tiles/8 || pair_hash(seed^(x/8), y/8) & 1) {
                    bridges.insert({x, y});
                    if (DEBUG) pm_overlay[x][y] = -2;
                }
            }
        }
    }
    for (auto& p : bridges) {
        world_alt_set(p.x, p.y, ALT_OCEAN, 1);
        world_alt_set(p.x, p.y, ALT_OCEAN_SHELF, 1);
    }
    world_temperature();
    world_riverbeds();
    world_fungus();
    world_validate();

    int lm = conf.landmarks;
    if (lm & LM_JUNGLE) {
        if (conf.modified_landmarks) {
            mod_world_monsoon();
        } else {
            world_monsoon(-1, -1);
        }
    }
    if (lm & LM_CRATER) world_crater(-1, -1);
    if (lm & LM_VOLCANO) world_volcano(-1, -1, 0);
    if (lm & LM_MESA) world_mesa(-1, -1);
    if (lm & LM_RIDGE) world_ridge(-1, -1);
    if (lm & LM_URANIUM) world_diamond(-1, -1);
    if (lm & LM_RUINS) world_ruin(-1, -1);
    if (*expansion_enabled) {
        if (lm & LM_UNITY) world_unity(-1, -1);
        if (lm & LM_NEXUS) world_temple(-1, -1);
    }
    if (lm & LM_FOSSIL) world_fossil(-1, -1);
    if (lm & LM_CANYON) world_canyon_nessus(-1, -1);
    if (lm & LM_BOREHOLE) world_borehole(-1, -1);
    if (lm & LM_SARGASSO) world_sargasso(-1, -1);
    if (lm & LM_DUNES) world_dune(-1, -1);
    if (lm & LM_FRESH) world_fresh(-1, -1);
    if (lm & LM_GEOTHERMAL) world_geothermal(-1, -1);

    fixup_landmarks();
    *WorldAddTemperature = 0;
    *WorldSkipTerritory = 0; // If this flag is false, reset_territory is run in world_climate
    world_climate(); // Run Path::continents
    world_rocky();

    if (!*GameHalted) {
        MapWin_clear_terrain(MapWin);
    }
    fflush(debug_log);
}

