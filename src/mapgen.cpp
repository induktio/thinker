
#include "mapgen.h"
#include "lib/FastNoiseLite.h"


bool locate_landmark(int* x, int* y, bool ocean) {
    int attempts = 0;
    if (!mapsq(*x, *y)) {
        do {
            if (*MapAreaY > 17) {
                *y = random(*MapAreaY - 16) + 8;
            } else {
                *y = 0;
            }
            if (*MapAreaX > 1) {
                *x = (random(*MapAreaX) & (~1)) + (*y&1);
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

void __cdecl mod_world_monsoon(int x, int y) {
    MAP* sq;
    world_rainfall();
    if (!conf.modified_landmarks) {
        int attempts = 0;
        int land_count;
        if (!mapsq(x, y)) {
            do {
                y = *MapAreaY / 2 + random(4) - 2;
                x = (random(*MapAreaX) & (~1)) + (y&1);
                land_count = 0;
                for (int i = 0; i < TableRange[5]; i++) {
                    int x2 = wrap(x + TableOffsetX[i]);
                    int y2 = y + TableOffsetY[i];
                    if (!is_ocean(mapsq(x2, y2))) {
                        land_count++;
                    }
                }
                if (++attempts >= 1000) {
                    return;
                }
                sq = mapsq(x, y);
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
        new_landmark(x, y, (int)Natural[__builtin_ctz(LM_JUNGLE)].name);
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
            tiles[{x, y}].valid = !sq->landmarks
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
    int loc_cur = random(4);
    for (i = 0; i < 512 && num < limit; i++) {
        if (loc_all == 15) {
            loc_all = 0;
            loc_cur = random(4);
        }
        if (i % 16 == 0 || loc_all & (1 << loc_cur)) {
            while (loc_all & (1 << loc_cur)) {
                loc_cur = random(4);
            }
        }
        y = (random(y_b - y_a) + y_a);
        x = wrap(((random(*MapAreaX / 4) + loc_cur * *MapAreaX / 4) &~1) + (y&1));
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
            y2 = y2 + random(8);
            x2 = wrap(((x2 + random(8)) &~1) + (y2&1));
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

void __cdecl mod_world_borehole(int x, int y) {
    if (!locate_landmark(&x, &y, false)) {
        return;
    }
    // Replace inconsistent timeGetTime() values used for seeding
    uint32_t seed = random(256);
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
    new_landmark(x, y, (int)Natural[__builtin_ctz(LM_BOREHOLE)].name);
}

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
    new_landmark(x, y, (int)Natural[__builtin_ctz(LM_FOSSIL)].name);
}

static float world_fractal(FastNoiseLite& noise, int x, int y) {
    float val = 1.0f * noise.GetNoise(3.0f*x, 2.0f*y)
        + 0.3f * (2 + *MapErosiveForces) * noise.GetNoise(-6.0f*x, -4.0f*y);
    if (x < 8 && *MapAreaX >= 32 && !map_is_flat()) {
        val = x/8.0f * val + (1.0f - x/8.0f) * world_fractal(noise, *MapAreaX + x, y);
    }
    return val;
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
    random_reseed(seed);

    memcpy(AltNatural, AltNaturalDefault, 0x2Cu);
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
            MapSizePlanet[i] = random(3);
        }
        if (i > 0) {
            MapSizePlanet[i] = clamp(MapSizePlanet[i], 0, 2);
        }
    }
    if (*GameState & STATE_OMNISCIENT_VIEW) {
        MapWin_clear_terrain(MapWin);
        draw_map(1);
    }
    game_srand(seed ^ 0xffff); // For game_rand function, terrain detail and landmark placement
    *MapRandomSeed = (seed % 0x7fff) + 1; // Must be non-zero, supply pod placement
    *MapLandCoverage = 2 - *MapOceanCoverage;

    Points conts;
    uint32_t continents = clamp(*MapAreaSqRoot / 12, 4, 20);
    int levels[256] = {};
    int x = 0, y = 0, i = 0;

    if (conf.world_continents && *MapAreaY >= 32) {
        while (++i <= 200 && conts.size() < continents) {
            y = (random(*MapAreaY - 16) + 8);
            x = (random(*MapAreaX) &~1) + (y&1);
            if (i & 1 && min_vector(conts, x, y) <= *MapAreaSqRoot/6) {
                conts.insert({x, y});
            }
            if (~i & 1 && min_vector(conts, x, y) >= *MapAreaSqRoot/3) {
                conts.insert({x, y});
            }
        }
    }

    const float L = AltNatural[ALT_SHORE_LINE]; // Default shoreline altitude = 45
    const float Wmid = (50 - conf.world_sea_levels[*MapOceanCoverage])*0.01f;
    const float Wsea = 0.1f + 0.01f * conf.world_ocean_mod;
    const float Wland = 0.1f + 0.01f * conf.world_hills_mod + 0.25f * (*MapErosiveForces);
    const float Wdist = 1.0f / clamp(*MapAreaSqRoot * 0.1f, 1.0f, 15.0f);

    for (y = 0; y < *MapAreaY; y++) {
        float Wcaps = 1.0f - min(1.0f, (min(y, *MapAreaY - y) / (max(1.0f, *MapAreaY * 0.2f))));

        for (x = y&1; x < *MapAreaX; x+=2) {
            float Wcont = 1.5f - clamp(min_vector(conts, x, y) * Wdist, 0.75f, 1.5f);
            float value = world_fractal(noise, x, y) + Wmid - 0.5f*Wcaps;
            if (value > 0) {
                value = value * Wland;
            } else {
                value = value * Wsea;
            }
            if (conf.world_continents && value < 0.2f) {
                value += Wcont * Wland;
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
    "ocean: %d erosion: %d cloud: %d sea_level: %d level_mod: %d\n",
    seed, *MapSizePlanet, *MapAreaX, *MapAreaY, *MapOceanCoverage,
    *MapErosiveForces, *MapCloudCover, conf.world_sea_levels[*MapOceanCoverage], level_mod);

    if (conf.world_polar_caps) {
        world_polar_caps();
    }
    world_linearize_contours();
    world_shorelines();
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
            if (__builtin_popcount(sea) > 1) {
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
    world_riverbeds();
    world_fungus();
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
            world_crater(-1, -1);
            lm.crater--;
        }
        if (lm.volcano > 0) {
            world_volcano(-1, -1, 0);
            lm.volcano--;
        }
        if (lm.mesa > 0) {
            world_mesa(-1, -1);
            lm.mesa--;
        }
        if (lm.ridge > 0) {
            world_ridge(-1, -1);
            lm.ridge--;
        }
        if (lm.uranium > 0) {
            world_diamond(-1, -1);
            lm.uranium--;
        }
        if (lm.ruins > 0) {
            world_ruin(-1, -1);
            lm.ruins--;
        }
        /*
        Unity Wreckage and Fossil Field Ridge are always expansion only content.
        Manifold Nexus and Borehole Cluster were also added to vanilla SMAC
        in the patches, even though those landmarks are technically expansion content.
        */
        if (*ExpansionEnabled) {
            if (lm.unity > 0) {
                world_unity(-1, -1);
                lm.unity--;
            }
            if (lm.fossil > 0) {
                mod_world_fossil(-1, -1);
                lm.fossil--;
            }
        }
        if (lm.canyon > 0) {
            world_canyon_nessus(-1, -1);
            lm.canyon--;
        }
        if (lm.nexus > 0) {
            world_temple(-1, -1);
            lm.nexus--;
        }
        if (lm.borehole > 0) {
            mod_world_borehole(-1, -1);
            lm.borehole--;
        }
        if (lm.sargasso > 0) {
            world_sargasso(-1, -1);
            lm.sargasso--;
        }
        if (lm.dunes > 0) {
            world_dune(-1, -1);
            lm.dunes--;
        }
        if (lm.fresh > 0) {
            world_fresh(-1, -1);
            lm.fresh--;
        }
        if (lm.geothermal > 0) {
            world_geothermal(-1, -1);
            lm.geothermal--;
        }
    }
    fixup_landmarks();
    *WorldAddTemperature = 0;
    *WorldSkipTerritory = 0; // If this flag is false, reset_territory is run in world_climate
    world_climate(); // Run Path::continents
    world_rocky();

    if (!*GameHalted) {
        MapWin_clear_terrain(MapWin);
    }
    flushlog();
}

