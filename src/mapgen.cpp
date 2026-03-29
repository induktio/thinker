
#include "mapgen.h"
#include "lib/FastNoiseLite.h"


static void name_landmark(int x, int y, MapLandmark lm) {
    new_landmark(x, y, Natural[__builtin_ctz(lm)].name);
}

int __cdecl near_landmark(int x, int y) {
    for (int i = 0; i < TableRange[8]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (code_at(x2, y2)) {
            return true;
        }
    }
    return false;
}

bool locate_landmark(int* x, int* y, bool ocean) {
    int attempts = 0;
    if (!mapsq(*x, *y)) {
        do {
            if (*MapAreaY > 17) {
                *y = map_rand.get(*MapAreaY - 16) + 8;
            } else {
                *y = 0;
            }
            if (*MapAreaX > 1) {
                *x = map_rand.get(*MapAreaX);
                *x = ((*x ^ *y) & 1) ^ *x;
            } else {
                *x = 0;
            }
            if (++attempts >= 1000) {
                return false;
            }
        } while (is_ocean(mapsq(*x, *y)) != ocean || near_landmark(*x, *y));
    }
    return true;
}

/*
Refresh climate effects each time terrain related features are changed on the map.
Note that world_temperature and world_rivers are refactored to use local random
instead of the global state since changing it would not be necessary.
*/
void __cdecl world_climate() {
    debug("world_climate_start\n");
    mod_world_shorelines();
    mod_world_temperature();
    mod_world_rivers();
    mod_world_rainfall();
    Path_continents(Paths);
    mod_world_analysis();
    if (!*WorldSkipTerritory) {
        reset_territory();
    }
    if (!*GameHalted) {
        for (int i = 0; i < 8; i++) {
            if (MapWinPtr[i]->iDrawToggleA) {
                MapWin_clear_terrain(MapWinPtr[i]);
            }
        }
        WorldWin_clear_terrain(WorldWin);
    }
    do_all_non_input();
    debug("world_climate_done\n");
}

void __cdecl mod_world_temperature() {
    GameRandom loc_rnd;
    loc_rnd.reseed(*MapRandomSeed + 17);
    int solar_scale = *MapAreaY / WorldBuilder->solar_energy;
    int thermal_scale = *MapAreaY / WorldBuilder->thermal_band;
    int deviance_scale = *MapAreaY / WorldBuilder->thermal_deviance;
    int warming_scale = *MapAreaY / WorldBuilder->global_warming;

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if ((sq->landmarks & (LM_DISABLE | LM_CRATER)) == LM_CRATER && sq->code_at() < 21) {
                continue;
            }
            int dev_rand = loc_rnd.get(0, 2 * deviance_scale);
            int orb_rand = loc_rnd.get(0, *MapPlanetaryOrbit + 1);
            int lat_dist = abs(deviance_scale + *MapAreaY / 2 - dev_rand - y);
            int temp_raw = (thermal_scale / 2
                + 2 * (lat_dist - solar_scale * (*MapPlanetaryOrbit - 1)
                - warming_scale * *MapSeaLevelCouncil)) / thermal_scale;
            int temp_base;
            if (temp_raw > 2) {
                temp_base = (temp_raw <= 9) ? 2 : 1;
            } else {
                temp_base = 3;
            }
            int alt = sq->alt_level();
            int temp = temp_base;
            for (int i = 0; i < 9; i++) {
                int nx = wrap(x + TableOffsetX[i]);
                int ny = y + TableOffsetY[i];
                MAP* nsq = mapsq(nx, ny);
                if (nsq && (nsq->items & BIT_THERMAL_BORE)
                && (alt <= 4 || nsq->alt_level() >= alt)) {
                    temp = temp_base + 1;
                    break;
                }
            }
            if (temp > 1) {
                if (alt <= 5) {
                    if (alt > 4 && !orb_rand) {
                        temp--;
                    }
                } else if (*MapPlanetaryOrbit < 2 || !loc_rnd.get(0, 2)) {
                    temp--;
                }
            }
            temp_set(x, y, clamp(temp - 1, 0, 2));
        }
    }
}

void __cdecl mod_world_rivers() {
    GameRandom loc_rnd;
    const int n_tiles = *MapAreaTiles;
    int river_count = 0;

    for (int i = 0; i < n_tiles; i++) {
        (*MapTiles)[i].items &= ~(BIT_RIVER_LAKE | BIT_RIVER);
    }
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* const sq = mapsq(x, y);
            if (is_ocean(sq) || !(sq->items & BIT_RIVER_SRC)) {
                continue;
            }
            for (int i = 0; i < n_tiles; i++) {
                (*MapTiles)[i].landmarks &= ~LM_UNK_800000;
            }
            loc_rnd.reseed(31 * x + 47 * y);
            int dir = loc_rnd.get(0, 4);
            int cur_x = x;
            int cur_y = y;
            int cur_alt = sq->alt_level();
            int cur_contour = sq->contour;
            sq->landmarks |= LM_UNK_800000;

            int steps = 0;
            int in_water = 0;
            int alt_streak = 1;
            int make_lake = !(sq->items & BIT_THERMAL_BORE);

            while (make_lake && ++steps <= 1000) {
                int pref_dir = (dir - (steps & 1) + loc_rnd.get(0, 2)) & 3;
                int back_dir = dir ^ 2;
                int min_uphill = 9999;
                int dir_counter = 8;
                int num_choices = 0;
                int total_weight = 0;
                int choice_dir[3] = {};
                int choice_water[3] = {};
                int choice_weight[3] = {};

                for (int nd = 0; nd < 4; nd++) {
                    int nx = wrap(cur_x + NearOffsetX[nd]);
                    int ny = cur_y + NearOffsetY[nd];
                    MAP* const nsq = mapsq(nx, ny);
                    if (!nsq || nd == back_dir) {
                        continue;
                    }
                    if (nsq->items & BIT_RIVER) {
                        num_choices = 0;
                        make_lake = 0;
                        break;
                    }
                    if (nsq->items & BIT_RIVER_SRC) {
                        int src_count = 0;
                        for (int i = 0; i < 4; i++) {
                            int px = wrap(nx + NearOffsetX[i]);
                            int py = ny + NearOffsetY[i];
                            MAP* psq = mapsq(px, py);
                            if (psq && psq->items & BIT_RIVER_SRC) {
                                src_count++;
                            }
                        }
                        if (src_count > 1) {
                            num_choices = 0;
                            make_lake = 0;
                            break;
                        }
                    }
                    if (nsq->alt_level() > cur_alt) {
                        continue;
                    }
                    int nsq_contour = nsq->contour;
                    int score = 32 * clamp(cur_contour - nsq_contour + 2, 1, 9999)
                        + (nd == pref_dir ? 31 : 0);
                    int water_flag = 0;
                    int uphill_adj = 0;
                    if (nsq_contour > cur_contour) {
                        if (nsq_contour >  min_uphill) { continue; }
                        min_uphill = nsq_contour;
                        score = dir_counter++;
                    }
                    bool next_loop = false;
                    for (int i = 0; i < 4; i++) {
                        if (i == (nd ^ 2)) { continue; }
                        int px = wrap(nx + NearOffsetX[i]);
                        int py = ny + NearOffsetY[i];
                        MAP* psq = mapsq(px, py);
                        if (psq) {
                            if (is_ocean(psq) && !is_ocean(nsq) && alt_streak < 5) {
                                score /= 2;
                            }
                            if (psq->contour > nsq_contour) {
                                uphill_adj++;
                                if (psq->items & BIT_RIVER) {
                                    next_loop = true;
                                    break;
                                }
                            }
                            if (psq->landmarks & LM_UNK_800000) {
                                if (in_water || is_ocean(psq)) {
                                    next_loop = true;
                                    break;
                                }
                                water_flag = 1;
                                score /= 8;
                            }
                        }
                    }
                    if (next_loop) {
                        continue;
                    }
                    if (score) {
                        if (uphill_adj == 3) { score = 1; }
                        total_weight += score;
                        assert(num_choices < 3);
                        choice_weight[num_choices] = score;
                        choice_water[num_choices] = water_flag;
                        choice_dir[num_choices] = nd;
                        num_choices++;
                    }
                }
                if (num_choices <= 0) {
                    break;
                }
                int pick = loc_rnd.get(0, total_weight);
                int* wp = choice_weight;
                int sel = 0;
                dir = 0;
                while (sel < num_choices && pick >= *wp) {
                    pick -= *wp;
                    sel++;
                    wp++;
                }
                if (sel < num_choices) {
                    dir = choice_dir[sel];
                    in_water = choice_water[sel];
                }
                cur_x = wrap(cur_x + NearOffsetX[dir]);
                cur_y = cur_y + NearOffsetY[dir];
                MAP* cur_sq = mapsq(cur_x, cur_y);
                if (cur_alt != cur_sq->alt_level()) {
                    alt_streak = 0;
                }
                cur_alt = cur_sq->alt_level();
                int new_contour = cur_sq->contour;
                if (new_contour > cur_contour) {
                    new_contour = ElevDetail[cur_alt];
                    if (cur_contour - 1 > new_contour) {
                        new_contour = cur_contour - 1;
                    }
                    alt_put_detail(cur_x, cur_y, new_contour);
                }
                cur_contour = new_contour;
                cur_sq->landmarks |= LM_UNK_800000;
                alt_streak++;
                if (cur_sq->items & BIT_THERMAL_BORE) {
                    make_lake = 0;
                    break;
                }
            }
            if (make_lake && cur_alt >= 3) {
                MAP* cur_sq = mapsq(cur_x, cur_y);
                if (!(cur_sq->items & BIT_RIVER) && cur_sq->anything_at() < 0) {
                    cur_sq->items |= BIT_RIVER_LAKE;
                }
            }
            river_count++;
            for (int i = 0; i < n_tiles; i++) {
                if ((*MapTiles)[i].landmarks & LM_UNK_800000) {
                    (*MapTiles)[i].items |= BIT_RIVER;
                }
            }
        }
    }
    for (int i = 0; i < n_tiles; i++) {
        (*MapTiles)[i].landmarks &= ~LM_UNK_800000;
    }
}

void __cdecl mod_world_rainfall() {
    int* const dword_9B22E0 = (int*)0x9B22E0;
    int* const dword_9B22DC = (int*)0x9B22DC;
    const int player_id = *CurrentPlayerFaction;
    const int n_tiles = *MapAreaTiles;
    *dword_9B22E0 = -1;
    *dword_9B22DC = -1;

    for (int i = 0; i < n_tiles; i++) {
        (*MapTiles)[i].unk_1 = 0;
    }

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* const sq = mapsq(x, y);
            if (sq->items & BIT_CONDENSER) {
                for (int i = 0; i < 9; i++) {
                    int nx = wrap(x + TableOffsetX[i]);
                    int ny = y + TableOffsetY[i];
                    MAP* nsq = mapsq(nx, ny);
                    if (nsq) {
                        nsq->unk_1 |= (nsq->unk_1 & 1 ? 4 : 1);
                    }
                }
            }
            if (is_ocean(sq)) {
                for (int i = 0, px = x; i < 3; i++, px -= 2) {
                    MAP* nsq = mapsq(wrap(px - 2), y);
                    MAP* csq = mapsq(wrap(px), y);
                    if (nsq && csq
                    && nsq->alt_level() >= csq->alt_level()
                    && i <= *MapCloudCover) {
                        nsq->unk_1 &= 0xEF;
                    }
                }
                MAP* psq = mapsq(wrap(x + 2), y);
                if (psq) {
                    psq->unk_1 |= 0x80;
                }
                if (*MapCloudCover > 0) {
                    MAP* nsq = mapsq(wrap(x + 4), y);
                    if (nsq && ((nsq->climate & 7u) > 1 || *MapCloudCover > 1)) {
                        nsq->unk_1 |= 0x80;
                        if (*MapCloudCover > 1) {
                            MAP* csq = mapsq(wrap(x + 6), y);
                            if (csq && csq->climate & 7u) {
                                csq->unk_1 |= 0x80;
                            }
                        }
                    }
                }
            }
            int alt = sq->alt_level();
            if (alt <= 4) {
                if (alt >= 3 && sq->items & BIT_FOREST) {
                    for (int i = 0; i < 3; i++) {
                        MAP* nsq = mapsq(wrap(x + BaseOffsetX[i]), y + BaseOffsetY[i]);
                        if (nsq) {
                            nsq->unk_1 |= 0x40;
                        }
                    }
                    if (!(x & 1)) {
                        sq->unk_1 |= 0x40;
                    }
                }
            } else {
                int cloudmass = (alt <= 5) ? WorldBuilder->cloudmass_hills : WorldBuilder->cloudmass_peaks;
                int rainfall = WorldBuilder->rainfall_coeff * (*MapCloudCover - 1);
                int max_back_steps = cloudmass + rainfall;
                if (max_back_steps >= 1) {
                    for (int step = 1, bx = x - 2; step <= max_back_steps; ++step, bx -= 2) {
                        MAP* bsq = mapsq(wrap(bx), y);
                        if (!bsq || is_ocean(bsq) || bsq->alt_level() >= alt) {
                            break;
                        }
                        bsq->unk_1 |= 0x20;
                    }
                }
                int max_fwd_steps = cloudmass - rainfall;
                if (max_fwd_steps >= 1) {
                    for (int step = 1, fx = x + 2; step <= max_fwd_steps; ++step, fx += 2) {
                        MAP* fsq = mapsq(wrap(fx), y);
                        if (!fsq || is_ocean(fsq) || fsq->alt_level() >= alt) {
                            break;
                        }
                        fsq->unk_1 |= 0x10;
                    }
                }
            }
        }
    }

    int rain_steps = 0;
    int rain_flag = 0;
    int dry_steps = 0;

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* const sq = mapsq(x, y);
            if (y <= 0 || y >= *MapAreaY - 1) {
                sq->climate &= 0xE7u;
                continue;
            }
            int alt = sq->alt_level();
            int landmarks = sq->landmarks;
            int temperature = sq->climate & 7;
            int moisture_mod = 0;
            int moisture = 0;
            int sloping = 0;
            if ((landmarks & (LM_DISABLE | LM_DUNES)) == LM_DUNES
            || (landmarks & (LM_DISABLE | LM_MESA))  == LM_MESA) {
                rain_flag = 1;
            }
            if (sq->unk_1 & 0x20) {
                moisture = 1;
                rain_flag = 1;
                dry_steps = 0;
                sloping = 1;
            }
            if (sq->unk_1 & 0x80) {
                rain_flag = (temperature > 1) ? 1 : 0;
                rain_steps = 0;
                dry_steps = 0;
                if (*MapCloudCover >= 1) {
                    moisture_mod = 1;
                }
            }
            if (sq->unk_1 & 0x10) {
                if (rain_flag) { --rain_flag; }
                --moisture_mod;
            }
            if ((sq->unk_1 & 0x40) && alt <= 4) {
                rain_steps = 0;
                dry_steps = 0;
                moisture_mod = max(0, moisture_mod);
                moisture = sloping;
            }
            if (alt > 3) {
                if (++rain_steps > 4 * *MapCloudCover * WorldBuilder->rainfall_coeff + 1 && rain_flag) {
                    --rain_flag;
                }
                moisture = sloping;
            } else {
                bool reset_moist = false;
                if (alt == 3) {
                    if (rain_flag) {
                        reset_moist = true;
                        if (rain_steps == 0 && ++dry_steps > 4 * *MapCloudCover + 2) {
                            rain_flag = 0;
                            dry_steps = 0;
                        }
                    }
                } else {
                    rain_flag = 1;
                    dry_steps = 0;
                }
                if (rain_steps > 0) {
                    reset_moist = true;
                    rain_steps = rain_steps - 1 - *MapCloudCover;
                    if (rain_steps < 1) {
                        rain_steps = 0;
                        dry_steps = 0;
                        if (*MapCloudCover > 0 || alt < 3) {
                            rain_flag = 1;
                        }
                    }
                }
                if (reset_moist) {
                    moisture = sloping;
                }
            }
            if (sq->items & BIT_RIVER) {
                if ((x >> 1) % 3 && moisture_mod < 0) {
                    moisture_mod = 0;
                }
                rain_flag = 1;
                dry_steps = 0;
            }
            if (sq->unk_1 & 2) {
                --moisture;
                rain_flag = 0;
            } else if (rain_flag) {
                moisture_mod += rain_flag;
            }
            if (alt <= (temperature != 0) + 4) {
                moisture += moisture_mod;
            } else if (*MapCloudCover - alt + temperature + 5 >= 2 && moisture_mod > 0) {
                ++moisture;
            }
            if ((landmarks & (LM_DISABLE | LM_JUNGLE)) == LM_JUNGLE && !is_ocean(sq) && alt < 6) {
                int code = landmarks >> 24;
                if (code < 0) { code = -code; }
                if (code >= 49) {
                    if (code >= 81 || alt >= 5) { ++moisture; }
                    else { moisture += 2; }
                } else {
                    moisture += 2;
                }
            }
            if ((landmarks & (LM_DISABLE | LM_DUNES)) == LM_DUNES) {
                moisture = 0;
            }
            if ((landmarks & (LM_DISABLE | LM_UNITY)) == LM_UNITY || moisture < 0) {
                moisture = 0;
            } else if (moisture > 2) {
                moisture = 2;
            }
            if (sq->unk_1 & 1) {
                rain_flag = 1;
                if (moisture < 2) { ++moisture; }
            }
            if (sq->unk_1 & 4) {
                rain_flag = 1;
                if (moisture < 2) { ++moisture; }
            }
            if (*CurrentTurn) {
                // action_terraform related popup TERRAMINE
                // remove unused dword_9B22DC base_find with the popup changes
                int old_moisture = (sq->climate >> 3) & 3;
                if (old_moisture != moisture && *dword_9B22E0 < 0) {
                    int base_id = base_find_2(x, y, player_id);
                    if (base_id >= 0 && *BaseFindDist <= 2) {
                        *dword_9B22E0 = base_id;
                    }
                }
            }
            climate_set(x, y, moisture);
        }
    }

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if (sq && !sq->is_rainy_or_moist()) {
                for (int i = 0; i < 8; i++) {
                    int nx = wrap(x + BaseOffsetX[i]);
                    int ny = y + BaseOffsetY[i];
                    MAP* nsq = mapsq(nx, ny);
                    if (nsq && !is_ocean(nsq) && nsq->is_rainy()) {
                        climate_set(x, y, 1);
                        break;
                    }
                }
            }
        }
    }
}

void __cdecl mod_world_analysis() {
    for (int i = 0; i < MaxRegionNum; i++) {
        Continents[i].open_terrain = 0;
    }
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* const sq = mapsq(x, y);
            assert(sq->region < MaxRegionNum);
            if (!is_ocean(sq) && sq->is_rainy_or_moist()
            && !sq->is_rocky() && !sq->is_fungus()) {
                Continents[sq->region].open_terrain++;
            }
            sq->items &= ~(BIT_DOUBLE_SEA | BIT_CANAL_COAST);
            if (!is_ocean(sq)) {
                int last_is_ocean = -1;
                int changes_inner = 0;
                int changes_outer = 0;
                for (int i = 1; i <= 20; i++) {
                    int nx = wrap(x + TableOffsetX[i]);
                    int ny = y + TableOffsetY[i];
                    MAP* const nsq = mapsq(nx, ny);
                    if (!nsq) {
                        continue;
                    }
                    int ocean = is_ocean(nsq);
                    if (i <= 8) {
                        if (ocean != last_is_ocean) {
                            last_is_ocean = ocean;
                            changes_inner++;
                            if (changes_inner == 4 && Continents[sq->region].tile_count >= 80) {
                                sq->items |= BIT_DOUBLE_SEA;
                            }
                        }
                    } else {
                        if (ocean != last_is_ocean || i == 9) {
                            last_is_ocean = ocean;
                            changes_outer++;
                        }
                        if (ny > 4 && ny < *MapAreaY - 4) {
                            if (nsq->region < MaxRegionLandNum && nsq->region != sq->region
                            && Continents[nsq->region].tile_count >= 40
                            && Continents[sq->region].tile_count >= 40) {
                                sq->items |= BIT_CANAL_COAST;
                            }
                        }
                    }
                }
                if (changes_outer < 4) {
                    sq->items &= ~BIT_DOUBLE_SEA;
                }
            }
        }
    }
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].val2 &= 0xF;
    }
}

void __cdecl mod_world_rocky() {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            bool flag = sq->landmarks & LM_DISABLE;
            if ((flag || !(sq->landmarks & LM_CRATER)) || sq->code_at() >= 21) {
                if ((flag || !(sq->landmarks & LM_UNITY))
                || (sq->code_at() >= 9
                && sq->code_at() != 10
                && sq->code_at() != 12
                && sq->code_at() != 15
                && sq->code_at() != 16
                && sq->code_at() != 19
                && sq->code_at() != 20)) {
                    int value = mod_minerals_at(x, y);
                    assert(value == minerals_at(x, y));
                    rocky_set(x, y, value);
                    if (!(sq->landmarks & LM_FRESH) || !is_ocean(sq)) {
                        if (is_ocean(sq)) {
                            bit_set(x, y, BIT_ENERGY_RES, 1);
                        } else if (flag || !(sq->landmarks & LM_JUNGLE)) {
                            if (sq->is_rainy() || sq->alt_level() > ALT_ONE_ABOVE_SEA) {
                                bit_set(x, y, BIT_MINERAL_RES, 1);
                            }
                        } else if (!map_rand.get(2)) {
                            bit_set(x, y, BIT_ENERGY_RES, 1);
                        } else if (sq->is_rainy() || sq->alt_level() > ALT_ONE_ABOVE_SEA) {
                            bit_set(x, y, BIT_MINERAL_RES, 1);
                        }
                    }
                }
            }
        }
    }
}

void __cdecl mod_world_riverbeds() {
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].items &= ~BIT_RIVER_SRC;
    }
    uint32_t riverbed_count = 0;
    uint32_t max_riverbeds = (*MapAreaTiles * ((4 - *MapOceanCoverage) * (WorldBuilder->rivers_base
        + *MapCloudCover * WorldBuilder->rivers_rain_mod) / 3)) / 3200;
    for (int i = 0; i < 4000 && riverbed_count < max_riverbeds; i++) {
        int x = map_rand.get(*MapAreaX);
        int y = map_rand.get(*MapAreaY);
        x = ((x ^ y) & 1) ^ x;
        MAP* sq = mapsq(x, y);
        if (!is_ocean(sq) && !(sq->items & BIT_RIVER_SRC)) {
            int search_alt = 0;
            int x_search = -1;
            int y_search = -1;
            for (int j = 0; j < TableRange[4]; j++) {
                int x2 = wrap(x + TableOffsetX[j]);
                int y2 = y + TableOffsetY[j];
                if ((sq = mapsq(x2, y2))) {
                    if (sq->items & BIT_RIVER_SRC) {
                        x_search = -1;
                        break;
                    }
                    if (j < TableRange[3]) {
                        int alt = sq->alt_level();
                        if (alt > search_alt) {
                            search_alt = alt;
                            x_search = x2;
                            y_search = y2;
                        }
                    }
                }
            }
            if (x_search >= 0) {
                bit_set(x_search, y_search, BIT_RIVER_SRC, true);
                riverbed_count++;
            }
        }
    }
}

void __cdecl mod_world_shorelines() {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            int alt = alt_natural(x, y);
            alt_set(x, y, alt);
            if ((*GameState & STATE_OMNISCIENT_VIEW) && *WorldAddTemperature) {
                if (alt >= 3) {
                    temp_set(x, y, clamp(alt - 3, 0, 2));
                }
                draw_tile(x, y, 2);
            }
        }
    }
    for (int alt = conf.altitude_limit; alt >= 2; alt--) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y & 1; x < *MapAreaX; x += 2) {
                if (alt_at(x, y) == alt) {
                    world_alt_set(x, y, alt, 1);
                }
            }
        }
    }
    if ((*GameState & STATE_OMNISCIENT_VIEW) && *WorldAddTemperature) {
        MapWin_clear_terrain(MapWin);
        draw_map(1);
    }
}

void __cdecl mod_world_polar_caps() {
    int yd = *MapAreaY / 2 * 2;
    for (int x = 0; x < *MapAreaX; x += 2) {
        world_alt_put_detail(x, 0);
        world_alt_put_detail(x + 1, yd - 1);
    }
    int limit = *MapAreaX / 16;
    for (int i = 0; i < limit; i++) {
        world_alt_put_detail(map_rand.get(*MapAreaX / 2) * 2, 0);
        world_alt_put_detail(map_rand.get(*MapAreaX / 2) * 2 + 1, 1);
        world_alt_put_detail(map_rand.get(*MapAreaX / 2) * 2 + 1, yd - 1);
        world_alt_put_detail(map_rand.get(*MapAreaX / 2) * 2, yd - 2);
    }
}

void __cdecl mod_world_linearize_contours() {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            int alt = alt_natural(x, y);
            int alt_val = (alt >= conf.altitude_limit ? 10 : alt + 1);
            int div_val = AltDefault[alt + 1] - AltDefault[alt];
            assert(div_val > 0);
            uint8_t detail = ElevDetail[alt]
                + ((alt_detail_at(x, y) - AltDefault[alt])
                * (ElevDetail[alt_val] - ElevDetail[alt]) / div_val);
            alt_put_detail(x, y, detail);
        }
    }
    memcpy(AltNatural, ElevDetail, sizeof(ElevDetail));
}

void __cdecl mod_world_crater(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    world_alt_set(x, y, ALT_OCEAN_TRENCH, true);
    world_alt_set(x, y, ALT_THREE_ABOVE_SEA, true);
    world_alt_set(x, y, ALT_SHORE_LINE, true);
    world_alt_set(x, y, ALT_OCEAN_SHELF, true);
    world_alt_set(x, y, ALT_SHORE_LINE, true);
    uint8_t temperature = temp_at(x, y);
    for (int i = 0; i < TableRange[3]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            bit_set(x2, y2, BIT_RIVER_SRC, false);
            bit2_set(x2, y2, LM_CRATER, true);
            code_set(x2, y2, i);
            if (i < 21) {
                rocky_set(x2, y2, LEVEL_ROLLING);
                temp_set(x2, y2, temperature);
            }
        }
    }
    name_landmark(x, y, LM_CRATER);
}

void __cdecl mod_world_monsoon(int x, int y) {
    MAP* sq;
    world_rainfall();
    if (!conf.modified_landmarks) {
        int attempts = 0;
        int land_count;
        if (!mapsq(x, y)) {
            do {
                y = *MapAreaY / 2 + map_rand.get(4) - 2;
                x = map_rand.get(*MapAreaX);
                x = ((x ^ y) & 1) ^ x;
                land_count = 0;
                for (int i = 0; i < TableRange[5]; i++) {
                    int x2 = wrap(x + TableOffsetX[i]);
                    int y2 = y + TableOffsetY[i];
                    if (!is_ocean(mapsq(x2, y2))) {
                        land_count++;
                    }
                }
                sq = mapsq(x, y);
                if (++attempts >= 1000) {
                    return;
                }
            } while (is_ocean(sq) || !is_coast(x, y, true)
            || land_count < 40 || !sq->is_rainy() || near_landmark(x, y));
        }
        for (int i = 0; i < TableRange[5]; i++) {
            int x2 = wrap(x + TableOffsetX[i]);
            int y2 = y + TableOffsetY[i];
            sq = mapsq(x2, y2);
            if (sq && abs(TableOffsetY[i]) <= 8) {
                if (i < 21 && is_ocean(sq)) {
                    world_alt_set(x2, y2, ALT_SHORE_LINE, true);
                }
                bit2_set(x2, y2, LM_JUNGLE, true);
                code_set(x2, y2, i);
            }
        }
        name_landmark(x, y, LM_JUNGLE);
        return;
    }
    struct TileInfo {
        uint8_t valid;
        uint8_t valid_near;
        uint8_t sea;
        uint8_t sea_near;
    };
    std::unordered_map<Point, TileInfo> tiles;

    int i = 0, j = 0, x2 = 0, y2 = 0, num = 0;
    const int y_a = *MapAreaY * 5/16;
    const int y_b = *MapAreaY * 11/16;
    const int limit = max(1024, *MapAreaTiles) * (2 + *MapCloudCover) / 128;

    for (y = 0; y < *MapAreaY; y++) {
        for (x = y&1; x < *MapAreaX; x+=2) {
            if (!(sq = mapsq(x, y))) {
                continue;
            }
            tiles[{x, y}].sea = sq->alt_level() < ALT_SHORE_LINE
                && Continents[sq->region].tile_count > 15;
            tiles[{x, y}].valid = !sq->lm_items()
                && (sq->alt_level() == ALT_SHORE_LINE || sq->alt_level() == ALT_ONE_ABOVE_SEA);
        }
    }
    for (y = 0; y < *MapAreaY; y++) {
        for (x = y&1; x < *MapAreaX; x+=2) {
            if (tiles[{x, y}].valid) {
                for (auto& p : iterate_tiles(x, y, 0, 45)) {
                    if (tiles[{p.x, p.y}].valid) {
                        tiles[{x, y}].valid_near++;
                    }
                    if (tiles[{p.x, p.y}].sea) {
                        tiles[{x, y}].sea_near++;
                    }
                }
            }
        }
    }
    int loc_all = 0;
    int loc_cur = map_rand.get(4);
    for (i = 0; i < 512 && num < limit; i++) {
        if (loc_all == 15) {
            loc_all = 0;
            loc_cur = map_rand.get(4);
        }
        if (i % 16 == 0 || loc_all & (1 << loc_cur)) {
            while (loc_all & (1 << loc_cur)) {
                loc_cur = map_rand.get(4);
            }
        }
        y = (map_rand.get(y_b - y_a) + y_a);
        x = wrap(((map_rand.get(*MapAreaX / 4) + loc_cur * *MapAreaX / 4) &~1) + (y&1));
        if (!tiles[{x, y}].valid
        || tiles[{x, y}].sea_near < 8 - i/16
        || tiles[{x, y}].valid_near < 16 - i/32) {
            continue;
        }
        int goal = num + max(21, limit/4);
        for (j = 0; j < 32 && num < goal && num < limit; j++) {
            if (j % 4 == 0) {
                y2 = y;
                x2 = x;
            }
            y2 = y2 + map_rand.get(8);
            x2 = wrap(((x2 + map_rand.get(8)) &~1) + (y2&1));
            if (y2 >= y_a && y2 <= y_b
            && tiles[{x2, y2}].valid
            && tiles[{x2, y2}].valid_near > 8 - i/32) {
                for (auto& p : iterate_tiles(x2, y2, 0, 21)) {
                    if (tiles[{p.x, p.y}].valid && !(p.sq->landmarks & LM_JUNGLE)) {
                        assert(!is_ocean(p.sq));
                        bit2_set(p.x, p.y, LM_JUNGLE, 1);
                        code_set(p.x, p.y, num % 121);
                        loc_all |= (1 << loc_cur);
                        num++;
                    }
                }
            }
        }
    }
}

void __cdecl mod_world_sargasso(int x, int y) {
    if (!locate_landmark(&x, &y, true)) {
        return;
    }
    bool has_skipped = false;
    for (int i = 0; i < TableRange[4]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq && (is_ocean(sq) || i < 21)) {
            world_alt_set(x2, y2, ALT_OCEAN_SHELF, true);
            bit2_set(x2, y2, LM_SARGASSO, true);
            code_set(x2, y2, i);
            if (i <= 4) {
                bit_set(x2, y2, BIT_SUPPLY_POD, true);
            }
            if ((has_skipped || i < TableRange[2] || map_rand.get(24))
            && (i < TableRange[3] || map_rand.get(3))) {
                bit_set(x2, y2, BIT_FUNGUS, true);
            } else {
                has_skipped = true;
            }
        }
    }
    name_landmark(x, y, LM_SARGASSO);
}

void __cdecl mod_world_ruin(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            if (is_ocean(sq)) {
                if (i && i < 9) {
                    bit_set(x2, y2, BIT_MONOLITH, true);
                }
            } else {
                bit_set(x2, y2, BIT_FUNGUS | BIT_SUPPLY_REMOVE, true);
                if (!i) {
                    bit_set(x2, y2, BIT_FUNGUS, false);
                    if (sq->is_rocky()) { // Remove rocky tiles from the center
                        rocky_set(x2, y2, LEVEL_FLAT);
                    }
                } else if (i < 9) {
                    bit_set(x2, y2, BIT_MONOLITH, true);
                }
            }
            bit2_set(x2, y2, LM_RUINS, true);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y, LM_RUINS);
}

void __cdecl mod_world_dune(int x, int y) {
    world_rainfall();
    MAP* sq = mapsq(x, y);
    if (!sq) {
        int attempts = 0;
        int half_height = *MapAreaY / 2;
        do {
            y = map_rand.get(half_height) + half_height - *MapAreaY / 4;
            x = map_rand.get(*MapAreaX);
            x = ((x ^ y) & 1) ^ x;
            sq = mapsq(x, y);
            if (++attempts >= 1000) {
                return;
            }
        } while (is_ocean(sq) || sq->is_rainy_or_moist() || near_landmark(x, y));
    }
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        sq = mapsq(x2, y2);
        if (sq && (!is_ocean(sq) || i < 9 || map_rand.get(3))) {
            if (i == 2 || i == 7) {
                world_alt_set(x2, y2, ALT_ONE_ABOVE_SEA, true);
            } else {
                world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            }
            bit_set(x2, y2, BIT_FUNGUS, false);
            bit2_set(x2, y2, LM_DUNES, true);
            rocky_set(x2, y2, LEVEL_FLAT);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y + 2, LM_DUNES);
}

void __cdecl mod_world_diamond(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    for (int i = 0; i < 21; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            if (i < 9 || !is_ocean(sq) || map_rand.get(3)) {
                world_alt_set(x2, y2, ALT_SHORE_LINE, true);
                bit2_set(x2, y2, LM_URANIUM, true);
                rocky_set(x2, y2, LEVEL_FLAT);
                bit_set(x2, y2, BIT_FUNGUS, false);
                code_set(x2, y2, i);
            }
        }
    }
    name_landmark(x, y, LM_URANIUM);
}

void __cdecl mod_world_fresh(int x, int y) {
    int region;
    if (mapsq(x, y)) {
        region = region_at(x, y);
        if (!is_ocean(mapsq(x, y))) {
            return;
        }
    } else {
        int tile_search = 0;
        int region_search = -1;
        for (int i = MaxRegionLandNum; i < MaxRegionNum; i++) {
            int tile_count = Continents[i].tile_count;
            if (tile_count >= 3 && tile_count <= 32 && tile_count >= tile_search) {
                tile_search = tile_count;
                region_search = i;
            }
        }
        if (region_search < 0) {
            return;
        }
        region = region_search;
    }
    int x_search = -1;
    bool has_landmark = false;
    for (int y_it = *MapAreaY - 1; y_it >= 0 ; y_it--) {
        for (int x_it = y_it & 1; x_it < *MapAreaX; x_it += 2) {
            if (region_at(x_it, y_it) == region) {
                bit2_set(x_it, y_it, LM_FRESH, true);
                if (x_search < 0) {
                    x_search = x_it;
                }
            } else if (!has_landmark && x_search >= 0) {
                int x_fresh = (x_search + x_it - 2) / 2;
                name_landmark(((x_fresh ^ y_it) & 1) ^ x_fresh, y_it, LM_FRESH);
                has_landmark = true;
            }
        }
    }
}

void __cdecl mod_world_volcano(int x, int y, int skip_label) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    world_alt_set(x, y, ALT_OCEAN_TRENCH, true);
    world_alt_set(x, y, ALT_THREE_ABOVE_SEA, true);
    for (int i = 0; i < TableRange[1]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            bit2_set(x2, y2, LM_VOLCANO, true);
            code_set(x2, y2, i);
            uint32_t items = bit_at(x2, y2) & ~(BIT_SUPPLY_POD | BIT_THERMAL_BORE
                | BIT_ECH_MIRROR | BIT_CONDENSER | BIT_SOIL_ENRICHER | BIT_FARM | BIT_RIVER_SRC
                | BIT_SOLAR | BIT_FUNGUS | BIT_MINE | BIT_MAGTUBE | BIT_ROAD);
            if (!i) {
                items &= ~(BIT_UNK_4000000 | BIT_UNK_8000000);
            }
            items |= BIT_SUPPLY_REMOVE;
            bit_put(x2, y2, items);
            rocky_set(x2, y2, LEVEL_ROCKY);
        }
    }
    *MountPlanetX = x;
    *MountPlanetY = y;
    if (!skip_label) {
        name_landmark(x, y, LM_VOLCANO);
    }
}

/*
Setup the 'Borehole Cluster' landmark. Added to SMAC in 3.0 patch.
*/
void __cdecl mod_world_borehole(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    // Replace inconsistent timeGetTime() values used for seeding
    uint32_t seed = map_rand.get(256);
    int val0 = (seed / 8) % 4;
    int val1 = 8;
    int val2 = ((seed % 8) / 3) + 5;
    int val3 = 3 - ((seed % 8) % 3);
    int val4 = -1;

    if (val0 & 2) {
        val2--;
        val3--;
        if (val0 & 1) {
            val1 = 8;
            val2++;
            val3++;
        } else {
            val1 = 6;
            val2--;
            val3--;
        }
        val1 = (val1 + 8) % 8 + 1;
        val2 = (val2 + 8) % 8 + 1;
        val3 = (val3 + 8) % 8 + 1;
    }
    if (conf.modified_landmarks) {
        val1 = 2;
        val2 = 4;
        val3 = 6;
        val4 = 8;
    }
    for (int i = 0; i < 9; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            bit_set(x2, y2, BIT_SUPPLY_REMOVE, true);
            if (i == val1 || i == val2 || i == val3 || i == val4) {
                bit_set(x2, y2, BIT_THERMAL_BORE, true);
                bit2_set(x2, y2, LM_BOREHOLE, true);
                code_set(x2, y2, i);
            }
        }
    }
    for (auto offset : {val1, val2, val3, val4}) {
        if (offset > 0) {
            int x2 = wrap(x + TableOffsetX[offset]);
            int y2 = y + TableOffsetY[offset];
            for (int i = 1; i < 9; i++) {
                int x3 = wrap(x2 + TableOffsetX[i]);
                int y3 = y2 + TableOffsetY[i];
                if (mapsq(x3, y3) && is_ocean(mapsq(x3, y3))) {
                    world_alt_set(x3, y3, ALT_SHORE_LINE, true);
                }
            }
        }
    }
    bit2_set(x, y, LM_BOREHOLE, true);
    name_landmark(x, y, LM_BOREHOLE);
}

/*
Setup 'The Manifold Nexus' landmark. Added to SMAC in 4.0 patch.
*/
void __cdecl mod_world_temple(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    for (int i = 0; i < TableRange[1]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            bit_set(x2, y2, BIT_SUPPLY_REMOVE, true);
            bit2_set(x2, y2, LM_NEXUS, true);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y, LM_NEXUS);
}

/*
Setup the 'Unity Wreckage' landmark (SMACX only).
*/
void __cdecl mod_world_unity(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    x--;
    y--;
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_ONE_ABOVE_SEA, true);
        }
    }
    x += 2;
    y += 2;
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_ONE_ABOVE_SEA, true);
        }
    }
    x--;
    y--;
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            rocky_set(x2, y2, LEVEL_FLAT);
            bit_set(x2, y2, BIT_RIVER_SRC, false);
            bit_set(x2, y2, BIT_FUNGUS, false);
            bit2_set(x2, y2, LM_UNITY, true);
            code_set(x2, y2, i);
            if (!i || i == 8 || i == 10 || i == 19) {
                bit_set(x2, y2, BIT_SUPPLY_POD, true);
            } else {
                bit_set(x2, y2, BIT_SUPPLY_REMOVE, true);
            }
        }
    }
    name_landmark(x, y, LM_UNITY);
}

/*
Setup the 'Fossil Ridge' landmark (SMACX only).
*/
void __cdecl mod_world_fossil(int x, int y) {
    if (!locate_landmark(&x, &y, true)) {
        return;
    }
    for (int i = 0; i < 6; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq && is_ocean(sq)) {
            world_alt_set(x2, y2, ALT_OCEAN, true);
            if (conf.modified_landmarks) {
                world_alt_set(x2, y2, ALT_OCEAN_SHELF, true);
            }
            bit2_set(x2, y2, LM_FOSSIL, true);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y, LM_FOSSIL);
}

void __cdecl mod_world_canyon(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    const int WorldCanyonOffset[12] = {26, 27, 9, 1, 2, 0, 3, 4, 17, 23, 36, 35};
    for (int i = 0; i < 12; i++) {
        int x2 = wrap(x + TableOffsetX[WorldCanyonOffset[i]]);
        int y2 = y + TableOffsetY[WorldCanyonOffset[i]];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_ONE_ABOVE_SEA, true);
            world_alt_set(x2, y2, ALT_SHORE_LINE, true);
            bit2_set(x2, y2, LM_CANYON, true); // rolled these two into single loop with
            code_set(x2, y2, WorldCanyonOffset[i]); // the above world_alt_set()
        }
    }
    name_landmark(x, y + 2, LM_CANYON);
}

void __cdecl mod_world_mesa(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    for (int i = 0; i < TableRange[3]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, (i < TableRange[2]) + ALT_SHORE_LINE, true);
            bit2_set(x2, y2, LM_MESA, true);
            code_set(x2, y2, i);
        }
    }
    name_landmark(x, y + 2, LM_MESA);
}

void __cdecl mod_world_ridge(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    const int WorldRidgeOffset[13] = {47, 44, 24, 20, 8, 7, 0, 5, 4, 17, 23, 35, 45};
    for (int i = 0; i < 13; i++) {
        int x2 = wrap(x + TableOffsetX[WorldRidgeOffset[i]]);
        int y2 = y + TableOffsetY[WorldRidgeOffset[i]];
        if (mapsq(x2, y2)) {
            world_alt_set(x2, y2, ALT_ONE_ABOVE_SEA, true);
            bit2_set(x2, y2, LM_RIDGE, true); // rolled these two into single loop with
            code_set(x2, y2, WorldRidgeOffset[i]); // the above world_alt_set()
        }
    }
    name_landmark(x, y + 2, LM_RIDGE);
}

void __cdecl mod_world_geothermal(int x, int y) {
    if (!locate_landmark(&x, &y, true)) {
        return;
    }
    bool has_skipped = false;
    for (int i = 0; i < TableRange[2]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq && (is_ocean(sq) || i < 9)) {
            if ((has_skipped || !i || map_rand.get(25)) && (i < 9 || map_rand.get(3))) {
                world_alt_set(x2, y2, ALT_OCEAN_SHELF, true);
                bit2_set(x2, y2, LM_GEOTHERMAL, true);
                code_set(x2, y2, i);
            } else {
                has_skipped = true;
            }
        }
    }
    name_landmark(x, y, LM_GEOTHERMAL);
}

static float world_fractal(FastNoiseLite& noise, float w, int x, int y) {
    float value = 1.0f * noise.GetNoise(1.5f*w*x, w*y)
        + 0.3f * (2 + *MapErosiveForces) * noise.GetNoise(-6.0f*x, -4.0f*y);
    if (x < 8 && *MapAreaX >= 16 && !map_is_flat()) {
        value = x/8.0f * value + (1.0f - x/8.0f) * world_fractal(noise, w, *MapAreaX + x, y);
    }
    return value;
}

/*
Replace original WorldBuilder fungus placement method. This still produces similar
fungus density given the same parameters but the placement patterns are altered.
*/
static void mod_world_fungus(FastNoiseLite& noise) {
    int WBvalue = WorldBuilder->fungus * (*MapNativeLifeForms - 1);
    int FTvalue = conf.spawn_fungal_towers > 1 ? clamp(conf.spawn_fungal_towers, 8, 1024) : 40;
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x+=2) {
            MAP* sq = mapsq(x, y);
            int value = clamp((int)(50.0f + 50.0f*noise.GetNoise(-6.0f*(256+x), 6.0f*y)), 0, 100);
            if ((sq->landmarks & (LM_VOLCANO|LM_DISABLE)) != LM_VOLCANO || sq->code_at() >= 0x90) {
                if ((value < 20 - WBvalue || value > 2 * WBvalue + 25)
                && (value < 40 - WBvalue || value > 2 * WBvalue + 45)
                && (value < 60 - WBvalue || value > 2 * WBvalue + 65)) {
                    if (y && y != *MapAreaY - 1) {
                        continue;
                    }
                    if (!map_rand.get(2)) {
                        continue;
                    }
                }
                if (!sq->is_rocky()) { // Later world_rocky can change the tile
                    sq->items |= BIT_FUNGUS;
                    sq->items &= ~TerraformRules[FORMER_PLANT_FUNGUS][1];
                    if (!map_rand.get(FTvalue) && !is_ocean(sq)
                    && *ExpansionEnabled && conf.spawn_fungal_towers) {
                        veh_init(BSC_FUNGAL_TOWER, 0, x, y);
                    }
                }
            }
        }
    }
}

void __cdecl mod_world_build() {
    static uint32_t seed = random_state();
    if (!conf.new_world_builder) {
        ThinkerVars->map_random_value = 0;
        world_build();
        return;
    }
    seed += pair_hash(seed, GetTickCount());
    world_generate(seed);
}

void console_world_generate(uint32_t seed) {
    if (*GameState & STATE_SCENARIO_EDITOR && *GameState & STATE_OMNISCIENT_VIEW) {
        *VehCount = 0;
        *BaseCount = 0;
        MapWin->fUnitNotViewMode = 0;
        MapWin->iUnit = -1;
        *GameState |= STATE_UNK_4;
        *GameState &= ~STATE_OMNISCIENT_VIEW;
        world_generate(seed);
        *GameState |= STATE_OMNISCIENT_VIEW;
        draw_map(1);
        GraphicWin_redraw(WorldWin);
    }
}

void world_generate(uint32_t seed) {
    if (DEBUG) {
        *GameState |= STATE_DEBUG_MODE;
    }
    MAP* sq;
    map_wipe();
    ThinkerVars->map_random_value = seed;
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(seed);
    map_rand.reseed(seed);

    memcpy(AltNatural, AltDefault, sizeof(AltDefault));
    *WorldAddTemperature = 1;
    *WorldSkipTerritory = 1;
    /*
    MapSizePlanet (this should not be unset, but checked regardless)
    MapOceanCoverage
    MapLandCoverage
    MapErosiveForces
    MapPlanetaryOrbit
    MapCloudCover
    MapNativeLifeForms
    */
    for (int i = 0; i < 7; i++) {
        if (MapSizePlanet[i] < 0) {
            MapSizePlanet[i] = map_rand.get(3);
        }
        if (i > 0) {
            MapSizePlanet[i] = clamp(MapSizePlanet[i], 0, 2);
        }
    }
    if (*GameState & STATE_OMNISCIENT_VIEW) {
        MapWin_clear_terrain(MapWin);
        draw_map(1);
    }
    *MapRandomSeed = (seed % 0x7fff) + 1; // Must be non-zero, supply pod placement
    *MapLandCoverage = 2 - *MapOceanCoverage;

    int levels[256] = {};
    int x = 0, y = 0, i = 0;

    const float L = AltNatural[ALT_SHORE_LINE]; // Default shoreline altitude = 45
    const float Wmid = (50 - conf.world_sea_levels[*MapOceanCoverage])*0.01f;
    const float Wsea = 0.1f + 0.01f * conf.world_ocean_mod;
    const float Wland = 0.1f + 0.01f * conf.world_hills_mod + 0.25f * (*MapErosiveForces);
    const float Warea = (conf.world_continents && *MapAreaY >= 32 ? 144.0f : 240.0f)
        / clamp(*MapAreaSqRoot, 120, 240);

    for (y = 0; y < *MapAreaY; y++) {
        float Wcaps = 1.0f - min(1.0f, (min(y, *MapAreaY - y) / (max(1.0f, *MapAreaY * 0.2f))));

        for (x = y&1; x < *MapAreaX; x+=2) {
            float value = world_fractal(noise, Warea, x, y) + Wmid - 0.5f*Wcaps;
            if (value > 0) {
                value = value * Wland;
            } else {
                value = value * Wsea;
            }
            sq = mapsq(x, y);
            sq->contour = clamp((int)(L + L*value), 0, 255);
        }
    }
    if (conf.world_mirror_x) {
        const int ky = 2 - (*MapAreaY & 1);
        for (y = 0; y < *MapAreaY/2; y++) {
            for (x = y&1; x < *MapAreaX; x+=2) {
                MAP* src = mapsq(x, y);
                MAP* tgt = mapsq(x, *MapAreaY - y - ky);
                tgt->contour = src->contour;
            }
        }
    }
    if (conf.world_mirror_y) {
        const int kx = 2 - (*MapAreaX & 1);
        for (y = 0; y < *MapAreaY; y++) {
            for (x = y&1; x < *MapAreaX/2; x+=2) {
                MAP* src = mapsq(x, y);
                MAP* tgt = mapsq(*MapAreaX - x - kx, y);
                tgt->contour = src->contour;
            }
        }
    }
    for (y = 0; y < *MapAreaY; y++) {
        for (x = y&1; x < *MapAreaX; x+=2) {
            sq = mapsq(x, y);
            levels[sq->contour]++;
        }
    }
    int level_sum = 0;
    int level_mod = 0;
    for (i = 0; i < 256; i++) {
        level_sum += levels[i];
        if (level_sum >= *MapAreaTiles * conf.world_sea_levels[*MapOceanCoverage] / 100) {
            level_mod = AltNatural[ALT_SHORE_LINE] - i;
            break;
        }
    }
    for (y = 0; y < *MapAreaY; y++) {
        for (x = y&1; x < *MapAreaX; x+=2) {
            sq = mapsq(x, y);
            sq->contour = clamp(sq->contour + level_mod, 0, 255);
        }
    }
    debug("world_build seed: %10u size: %d x: %d y: %d "\
    "ocean: %d hills: %d native: %d cloud: %d sea_level: %d level_mod: %d\n",
    seed, *MapSizePlanet, *MapAreaX, *MapAreaY, *MapOceanCoverage, *MapErosiveForces,
    *MapNativeLifeForms, *MapCloudCover, conf.world_sea_levels[*MapOceanCoverage], level_mod);

    if (conf.world_polar_caps) {
        mod_world_polar_caps();
    }
    mod_world_linearize_contours();
    mod_world_shorelines();
    Path_continents(Paths);
    Points bridges;

    for (y = 3; y < *MapAreaY - 3; y++) {
        for (x = y&1; x < *MapAreaX; x+=2) {
            sq = mapsq(x, y);
            if (is_ocean(sq)) {
                continue;
            }
            uint64_t sea = 0;
            int land_count = Continents[sq->region].tile_count;
            if (conf.world_islands_mod > 0) {
                if (land_count < conf.world_islands_mod && land_count < *MapAreaTiles/8) {
                    bridges.insert({x, y});
                    if (DEBUG) mapdata[{x, y}].overlay = -1;
                    continue;
                }
            }
            for (auto& m : iterate_tiles(x, y, 1, 13)) {
                if (is_ocean(m.sq)) {
                    if (Continents[m.sq->region].tile_count >= 30) {
                        sea |= 1 << (m.sq->region & 0x3f);
                    }
                }
            }
            if (bit_count(sea) > 1) {
                if (land_count > *MapAreaTiles/8 || pair_hash(seed^(x/8), y/8) & 1) {
                    bridges.insert({x, y});
                    if (DEBUG) mapdata[{x, y}].overlay = -2;
                }
            }
        }
    }
    for (auto& p : bridges) {
        world_alt_set(p.x, p.y, ALT_OCEAN, 1);
        world_alt_set(p.x, p.y, ALT_OCEAN_SHELF, 1);
    }
    world_temperature();
    mod_world_riverbeds();
    mod_world_fungus(noise);
    Path_continents(Paths);

    LMConfig lm;
    memcpy(&lm, &conf.landmarks, sizeof(lm));

    for (i = 0; i < 16; i++) {
        if (conf.world_mirror_x || conf.world_mirror_y) {
            break;
        }
        if (lm.jungle > 0) {
            mod_world_monsoon(-1, -1);
            lm.jungle--;
        }
        if (lm.crater > 0) {
            mod_world_crater(-1, -1);
            lm.crater--;
        }
        if (lm.volcano > 0) {
            mod_world_volcano(-1, -1, 0);
            lm.volcano--;
        }
        if (lm.mesa > 0) {
            mod_world_mesa(-1, -1);
            lm.mesa--;
        }
        if (lm.ridge > 0) {
            mod_world_ridge(-1, -1);
            lm.ridge--;
        }
        if (lm.uranium > 0) {
            mod_world_diamond(-1, -1);
            lm.uranium--;
        }
        if (lm.ruins > 0) {
            mod_world_ruin(-1, -1);
            lm.ruins--;
        }
        /*
        Unity Wreckage and Fossil Field Ridge are always expansion only content.
        Manifold Nexus and Borehole Cluster were also added to vanilla SMAC
        in the patches, even though those landmarks are technically expansion content.
        */
        if (*ExpansionEnabled) {
            if (lm.unity > 0) {
                mod_world_unity(-1, -1);
                lm.unity--;
            }
            if (lm.fossil > 0) {
                mod_world_fossil(-1, -1);
                lm.fossil--;
            }
        }
        if (lm.canyon > 0) {
            mod_world_canyon(-1, -1);
            lm.canyon--;
        }
        if (lm.nexus > 0) {
            mod_world_temple(-1, -1);
            lm.nexus--;
        }
        if (lm.borehole > 0) {
            mod_world_borehole(-1, -1);
            lm.borehole--;
        }
        if (lm.sargasso > 0) {
            mod_world_sargasso(-1, -1);
            lm.sargasso--;
        }
        if (lm.dunes > 0) {
            mod_world_dune(-1, -1);
            lm.dunes--;
        }
        if (lm.fresh > 0) {
            mod_world_fresh(-1, -1);
            lm.fresh--;
        }
        if (lm.geothermal > 0) {
            mod_world_geothermal(-1, -1);
            lm.geothermal--;
        }
    }
    fixup_landmarks();
    *WorldAddTemperature = 0;
    *WorldSkipTerritory = 0; // If this flag is false, reset_territory is run in world_climate
    world_climate(); // Run Path::continents
    mod_world_rocky();

    if (!*GameHalted) {
        MapWin_clear_terrain(MapWin);
    }
    flushlog();
}

