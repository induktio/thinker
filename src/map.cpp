
#include "map.h"


static Points spawns;
static Points natives;
static Points goodtiles;


static int mapgen_rand(int value) {
    // Replaces game_rand function, used by alt_set / alt_set_both
    return (value > 1 ? map_rand.get(value) : 0);
}

static MAP* next_tile(int x, int y, size_t offset, int* tx, int* ty) {
    *tx = wrap(x + TableOffsetX[offset]);
    *ty = y + TableOffsetY[offset];
    return mapsq(*tx, *ty);
}

MAP* mapsq(int x, int y) {
    if (x >= 0 && y >= 0 && x < *MapAreaX && y < *MapAreaY && !((x + y)&1)) {
        return &((*MapTiles)[ (x + *MapAreaX * y)/2 ]);
    } else {
        return NULL;
    }
}

int wrap(int x) {
    if (!map_is_flat()) {
        if (x >= 0) {
            if (x >= *MapAreaX) {
                x -= *MapAreaX;
            }
        } else {
            x += *MapAreaX;
        }
    }
    return x;
}

int map_range(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!map_is_flat() && dx > *MapHalfX) {
        dx = *MapAreaX - dx;
    }
    return (dx + dy)/2;
}

int vector_dist(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!map_is_flat() && dx > *MapHalfX) {
        dx = *MapAreaX - dx;
    }
    return max(dx, dy) - ((((dx + dy) / 2) - min(dx, dy) + 1) / 2);
}

int min_range(const Points& S, int x, int y) {
    int z = MaxMapDist;
    for (auto& p : S) {
        z = min(z, map_range(x, y, p.x, p.y));
    }
    return z;
}

int min_vector(const Points& S, int x, int y) {
    int z = MaxMapDist;
    for (auto& p : S) {
        z = min(z, vector_dist(x, y, p.x, p.y));
    }
    return z;
}

double avg_range(const Points& S, int x, int y) {
    int n = 0;
    int sum = 0;
    for (auto& p : S) {
        sum += map_range(x, y, p.x, p.y);
        n++;
    }
    return (n > 0 ? (1.0*sum)/n : 0);
}

bool is_ocean(MAP* sq) {
    return (!sq || (sq->climate >> 5) < ALT_SHORE_LINE);
}

bool is_ocean(BASE* base) {
    return is_ocean(mapsq(base->x, base->y));
}

bool is_ocean_shelf(MAP* sq) {
    return (sq && (sq->climate >> 5) == ALT_OCEAN_SHELF);
}

bool is_shore_level(MAP* sq) {
    return (sq && (sq->climate >> 5) == ALT_SHORE_LINE);
}

bool map_is_flat() {
    return *MapToggleFlat & 1;
}

bool adjacent_region(int x, int y, int owner, int threshold, bool ocean) {
    assert(mapsq(x, y));
    for (auto& m : iterate_tiles(x, y, 1, 9)) {
        if (is_ocean(m.sq) != ocean) {
            continue;
        }
        if ((owner < 0 || m.sq->owner < 0 || m.sq->owner == owner) && !bad_reg(m.sq->region)
        && Continents[m.sq->region].tile_count >= threshold) {
            return true;
        }
    }
    return false;
}

int clear_overlay(int UNUSED(x), int UNUSED(y)) {
    return 0;
}

void refresh_overlay(std::function<int(int, int)> tile_value) {
    if (*GameState & STATE_OMNISCIENT_VIEW && MapWin->iWhatToDrawFlags & MAPWIN_DRAW_GOALS) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y&1; x < *MapAreaX; x+=2) {
                mapdata[{x, y}].overlay = tile_value(x, y);
            }
        }
        draw_map(1);
    }
}

int __cdecl is_coast(int x, int y, bool is_base_radius) {
    int radius = is_base_radius ? 21 : 9;
    for (int i = 1; i < radius; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq && is_ocean(sq)) {
            return i; // Must return index to the first ocean tile
        }
    }
    return 0;
}

int __cdecl is_port(int base_id, bool is_base_radius) {
    return is_coast(Bases[base_id].x, Bases[base_id].y, is_base_radius);
}

int __cdecl on_map(int x, int y) {
    assert(!((x + y)&1)); // Original version does not check this condition
    return x >= 0 && x < *MapAreaX && y >= 0 && y < *MapAreaY;
}

/*
Validate region bounds. Bad regions include: 0, 63, 64, 127, 128.
*/
int __cdecl bad_reg(int region) {
    return (region & RegionBounds) == RegionBounds || !(region & RegionBounds);
}

void __cdecl owner_set(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->val2 &= 0xF0;
        sq->val2 |= faction_id & 0xF;
    }
}

void __cdecl site_set(int x, int y, uint8_t site) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->val2 &= 0x0F;
        sq->val2 |= site << 4;
    }
}

void __cdecl region_set(int x, int y, uint8_t region) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->region = region;
    }
}

int __cdecl region_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return sq ? sq->region : 0;
}

int __cdecl base_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq && sq->is_base()) {
        if (*BaseCount <= 0) {
            // Removed unnecessary debug code from here
            rebuild_base_bits();
            return -1;
        }
        for (int i = 0; i < *BaseCount; ++i) {
            if (Bases[i].x == x && Bases[i].y == y) {
                return i;
            }
        }
        assert(0);
    }
    return -1;
}

int __cdecl x_dist(int x1, int x2) {
    int dist = abs(x1 - x2);
    if (!map_is_flat() && dist > *MapHalfX) {
        dist = *MapAreaX - dist;
    }
    return dist;
}

int __cdecl alt_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        return sq->alt_level();
    }
    assert(sq);
    return 0;
}

int __cdecl alt_detail_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        return sq->contour;
    }
    return 0;
}

void __cdecl alt_put_detail(int x, int y, uint8_t detail) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        mapsq(x, y)->contour = detail;
    }
}

void __cdecl alt_set(int x, int y, int altitude) {
    MAP* sq = mapsq(x, y);
    assert(sq && altitude >= 0 && altitude <= 7);
    int alt = sq->alt_level();
    int is_sea = altitude < ALT_SHORE_LINE;
    auto set_alt = [&]() {
        sq->climate = (altitude * 32) | (sq->climate & 0x1F);
        *GameDrawState |= 4;
    };
    if (!*CurrentTurn) {
        return set_alt();
    }
    if (altitude != alt) {
        delete_landmark(x, y);
    }
    if ((alt < ALT_SHORE_LINE) != is_sea) {
        sq->items &= ~(BIT_SENSOR|BIT_THERMAL_BORE|BIT_ECH_MIRROR|BIT_CONDENSER|BIT_FOREST|\
            BIT_SOIL_ENRICHER|BIT_AIRBASE|BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD);
        for (int i = 1; i < MaxPlayerNum; i++) {
            sq->visible_items[i - 1] = sq->items;
        }
        if (*WorldSkipTerritory) {
            return set_alt();
        }
        int base_id = base_at(x, y);
        if (base_id >= 0) {
            BASE* base = &Bases[base_id];
            const bool is_player = base->faction_id == *CurrentPlayerFaction;
            const bool is_visible = base->visibility & (1 << *CurrentPlayerFaction);
            if (!is_sea || has_fac_built(FAC_PRESSURE_DOME, base_id)) {
                return set_alt();
            }
            // Fix: skip rendering pop_size as negative, zero pop_size deletes the base
            base->pop_size = max(0, base->pop_size
                - clamp(base->pop_size / 2, mapgen_rand(3) + 2, 10));
            set_fac(FAC_PRESSURE_DOME, base_id, 1);
            Factions[base->faction_id].player_flags |= PFLAG_UNK_80000;
            MapBaseSubmergedCount[base->faction_id]++;
            if (is_objective(base_id) && base->pop_size < 1) {
                base->pop_size = 1;
            }
            if (base->pop_size > 0) {
                return set_alt();
            }
            if (Factions[base->faction_id].base_count <= 1 || (*MultiplayerActive && !*ControlTurnC)) {
                base->pop_size = 1;
                if (!*VehBattleState && (is_player || is_visible)) {
                    Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                    parse_says(0, &base->name[0], -1, -1);
                    if (is_player) {
                        X_pop3("BASESUBMERGED", "subbase_sm.pcx", 0);
                    } else {
                        parse_says(1, &MFactions[base->faction_id].adj_name_faction[0], -1, -1);
                        X_pop3("BASESUBMERGED2", "subbase_sm.pcx", 0);
                    }
                }
                return set_alt();
            }
            if (!*VehBattleState && (is_player || is_visible)) {
                Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                parse_says(0, &base->name[0], -1, -1);
                if (is_player) {
                    popp(ScriptFile, "BASESUBMERGED1", 0, "subbase_sm.pcx", 0);
                } else {
                    parse_says(1, &MFactions[base->faction_id].adj_name_faction[0], -1, -1);
                    popp(ScriptFile, "BASESUBMERGED3", 0, "subbase_sm.pcx", 0);
                }
            }
            mod_base_kill(base_id);
        }
        int veh_id = stack_fix(veh_at(x, y));
        while (veh_id >= 0) {
            VEH* veh = &Vehs[veh_id];
            int next_veh_id = veh->next_veh_id_stack;
            if (veh->triad() != TRIAD_AIR && (veh->triad() == TRIAD_SEA) != is_sea
            && !(veh->flags & VFLAG_IS_OBJECTIVE)) {
                MapBaseIdClosestSubmergedVeh[veh->faction_id] = base_find2(x, y, veh->faction_id);
                veh_kill(veh_id);
                if (next_veh_id > veh_id) {
                    --next_veh_id;
                }
            }
            veh_id = next_veh_id;
        }
    } else if (altitude >= ALT_SHORE_LINE) {
        int val = sq->val3 >> 6;
        if (altitude <= alt) {
            if (altitude < alt && val > altitude - 3 && mapgen_rand(val + 1)) {
                sq->val3 = (sq->val3 & 0x3F) | ((val + 3) << 6);
                sq->landmarks |= LM_UNK_400000;
                *GameDrawState |= 1;
            }
        } else if (val < 2 && val + 1 <= altitude - 3) {
            if (!mapgen_rand(val + 4)) {
                sq->val3 = (sq->val3 & 0x3F) | ((val + 1) << 6);
                sq->landmarks |= LM_UNK_400000;
                *GameDrawState |= 1;
            }
        }
        assert((sq->val3 & 0xC0) != 0xC0); // Both rocky bits must not be set
    }
    set_alt(); // Always set altitude level
}

/*
Set both the altitude level and altitude detail for the specified tile.
*/
void __cdecl alt_set_both(int x, int y, int altitude) {
    alt_set(x, y, altitude);
    if (alt_natural(x, y) != altitude) {
        int offset = AltNatural[altitude + 1] - AltNatural[altitude];
        alt_put_detail(x, y,
            (uint8_t)(*MapSeaLevel + AltNatural[altitude] + mapgen_rand(offset)));
    }
}

/*
Calculate the altitude of the specified tile.
Return Value: altitude level on a scale from 0 (ocean trench) to 6 (mountain tops)
*/
int __cdecl alt_natural(int x, int y) {
    int detail = alt_detail_at(x, y) - *MapSeaLevel;
    int alt = conf.altitude_limit;
    while (alt > 0 && detail < AltNatural[alt]) {
        alt--;
    }
    return alt;
}

/*
Calculate the elevation of the specified tile.
Return Value: elevation meters (bounded to: -3000 to 3500)
*/
int __cdecl elev_at(int x, int y) {
    int contour = alt_detail_at(x, y);
    int elev = 50 * (contour - ElevDetail[3] - *MapSeaLevel);
    elev += (contour <= ElevDetail[alt_at(x, y)]) ? 10
        : (x * 113 + y * 217 + *MapSeaLevel * 301) % 50;
    return clamp(elev, -3000, conf.altitude_limit > ALT_THREE_ABOVE_SEA ? 4500 : 3500);
}

void __cdecl world_alt_set(int x, int y, int altitude, int toggle) {
    assert(altitude >= 0 && altitude <= 7);
    memset(MapBaseSubmergedCount, 0, MaxPlayerNum * sizeof(int));
    memset(MapBaseIdClosestSubmergedVeh, 0xFF, MaxPlayerNum * sizeof(int));
    if (on_map(x, y)) {
        if (toggle) {
            alt_set_both(x, y, altitude);
        } else {
            alt_set(x, y, altitude);
        }
    }
    MAP* sq;
    int tx = -1;
    int ty = -1;
    if (altitude > 1) {
        int alt = altitude - 1;
        for (int i = 0; i < altitude - 1; ++i, --alt) {
            bool found = false;
            for (int j = TableRange[i]; j < TableRange[i+1]; ++j) {
                if ((sq = next_tile(x, y, j, &tx, &ty)) != NULL && sq->alt_level() < alt) {
                    if (toggle) {
                        alt_set_both(tx, ty, alt);
                        if (sq->anything_at() < 0 && !bonus_at(tx, ty)) {
                            owner_set(tx, ty, -1);
                        }
                    } else {
                        alt_set(tx, ty, alt);
                    }
                    found = true;
                }
            }
            if (!found) {
                break;
            }
        }
    }
    if (altitude <= conf.altitude_limit) {
        int alt = altitude + 1;
        for (int i = 0; i <= conf.altitude_limit - altitude; ++i, ++alt) {
            bool found = false;
            for (int j = TableRange[i]; j < TableRange[i+1]; ++j) {
                if ((sq = next_tile(x, y, j, &tx, &ty)) != NULL && sq->alt_level() > alt) {
                    if (toggle) {
                        alt_set_both(tx, ty, alt);
                        if (sq->anything_at() < 0 && !bonus_at(tx, ty)) {
                            owner_set(tx, ty, -1);
                        }
                    } else {
                        alt_set(tx, ty, alt);
                    }
                    found = true;
                }
            }
            if (!found) {
                break;
            }
        }
    }
}

void __cdecl world_raise_alt(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        int alt = sq->alt_level();
        if (alt < conf.altitude_limit) {
            world_alt_set(x, y, alt + 1, 1);
        }
    } else {
        assert(sq);
    }
}

void __cdecl world_lower_alt(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        int alt = sq->alt_level();
        if (alt > 0) {
            world_alt_set(x, y, alt - 1, 1);
        }
    } else {
        assert(sq);
    }
}

void __cdecl world_alt_put_detail(int x, int y) {
    alt_put_detail(x, y, (uint8_t)AltNatural[3]);
}

int __cdecl temp_at(int x, int y) {
    return mapsq(x, y)->climate & 7;
}

void __cdecl temp_set(int x, int y, uint8_t value) {
    MAP* sq = mapsq(x, y);
    sq->climate &= 0xF8;
    sq->climate |= value & 7;
}

void __cdecl climate_set(int x, int y, uint8_t rainfall) {
    MAP* sq = mapsq(x, y);
    sq->climate &= 0xE7;
    sq->climate |= (rainfall & 3) << 3;
    sq->landmarks |= LM_UNK_400000;
    *GameDrawState |= 1;
}

void __cdecl rocky_set(int x, int y, uint8_t rocky) {
    MAP* sq = mapsq(x, y);
    sq->val3 &= 0x3F;
    sq->val3 |= rocky << 6;
    sq->landmarks |= LM_UNK_400000;
    *GameDrawState |= 1;
}

uint32_t __cdecl bit_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return sq ? sq->items : 0;
}

void __cdecl bit_put(int x, int y, uint32_t items) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->items = items;
    }
    assert(sq);
}

void __cdecl bit_set(int x, int y, uint32_t items, bool add) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        if (add) {
            sq->items |= items;
        } else {
            sq->items &= ~items;
        }
    }
    assert(sq);
}

uint32_t __cdecl bit2_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return (sq ? sq->landmarks : 0);
}

void __cdecl bit2_set(int x, int y, uint32_t items, bool add) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        if (add) {
            sq->landmarks |= items;
        } else {
            sq->landmarks &= ~items;
        }
    }
    assert(sq);
}

uint32_t __cdecl code_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return (sq ? sq->code_at() : 0);
}

void __cdecl code_set(int x, int y, int code) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        sq->landmarks &= 0xFFFFFF;
        sq->landmarks |= (code << 24);
        *GameDrawState |= 4;
    }
}

void __cdecl synch_bit(int x, int y, int faction_id) {
    if (faction_id > 0 && faction_id < MaxPlayerNum) {
        MAP* sq = mapsq(x, y);
        if (sq) {
            sq->visible_items[faction_id - 1] = sq->items;
        }
        assert(sq);
    }
}

/*
First Manifold Nexus tile must also be visible to the owner for the effect to take place.
*/
int __cdecl has_temple(int faction_id) {
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if (sq && sq->landmarks & LM_NEXUS
            && sq->code_at() == 0 && sq->owner == faction_id
            && sq->visibility & (1 << faction_id)) {
                return true;
            }
        }
    }
    return false;
}

/*
Search for the first landmark found within the radius range of the specified tile.
Return Value: Landmark offset or -1 if none found
*/
int __cdecl find_landmark(int x, int y, size_t range_offset) {
    int offset = TableRange[range_offset];
    for (int i = 0; i < offset; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (mapsq(x2, y2)) {
            for (int lm = 0; lm < *MapLandmarkCount; lm++) {
                if (x2 == Landmarks[lm].x && y2 == Landmarks[lm].y) {
                    return lm;
                }
            }
        }
    }
    return -1;
}

/*
Set up a new landmark with the provided name at the specified tile.
Return Value: Landmark offset or -1 if max landmark count is reached
*/
int __cdecl new_landmark(int x, int y, const char* name) {
    int offset = *MapLandmarkCount;
    if (offset >= MaxLandmarkNum) {
        return -1;
    }
    *MapLandmarkCount += 1;
    Landmarks[offset].x = x;
    Landmarks[offset].y = y;
    strcpy_n(&Landmarks[offset].name[0], 32, name);
    return offset;
}

/*
Check whether the specified faction has permission to name a landmark on the provided tile.
Return Value: Does the faction have control of the tile to set a landmark? true/false
*/
int __cdecl valid_landmark(int x, int y, int faction_id) {
    MAP* sq;
    int owner = *MultiplayerActive ? (sq = mapsq(x, y), sq ? sq->owner : -1)
        : mod_whose_territory(faction_id, x, y, NULL, false);
    if (owner == faction_id) {
        return true;
    }
    if (owner > 0) {
        return false;
    }
    int base_id = base_find(x, y);
    return base_id < 0 || Bases[base_id].faction_id == faction_id;
}

/*
Remove the landmark at the specified tile.
*/
void __cdecl kill_landmark(int x, int y) {
    int remove_id = find_landmark(x, y, 1);
    if (remove_id >= 0) {
        if (remove_id < (*MapLandmarkCount - 1)) {
            memmove_s(&Landmarks[remove_id], sizeof(Landmark) * MaxLandmarkNum,
                &Landmarks[remove_id + 1],
                sizeof(Landmark) * (*MapLandmarkCount - remove_id - 1));
        }
        *MapLandmarkCount -= 1;
    }
}

/*
Reset the map to a blank state. Original doesn't wipe unk_1 and owner fields.
This is simplified by zeroing all fields first and then setting specific fields.
*/
void __cdecl map_wipe() {
    *MapSeaLevel = 0;
    *MapSeaLevelCouncil = 0;
    *MapLandmarkCount = 0;
    *MapRandomSeed = random(0x7FFF) + 1;
    memset(*MapTiles, 0, *MapAreaTiles * sizeof(MAP));
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].climate = 0x20;
        (*MapTiles)[i].contour = 20;
        (*MapTiles)[i].val2 = 0xF;
        (*MapTiles)[i].owner = -1;
    }
    reset_state();
    mapdata.clear();
    mapnodes.clear();
}

int __cdecl resource_yield(BaseResType type, int faction_id, int base_id, int x, int y) {
    if (type == RSC_NUTRIENT) {
        return mod_crop_yield(faction_id, base_id, x, y, 0);
    } else if (type == RSC_MINERAL) {
        return mod_mine_yield(faction_id, base_id, x, y, 0);
    } else if (type == RSC_ENERGY) {
        return mod_energy_yield(faction_id, base_id, x, y, 0);
    } else if (type == RSC_UNUSED) {
        *BaseTerraformReduce = 0;
    }
    return 0;
}

int __cdecl mod_crop_yield(int faction_id, int base_id, int x, int y, int flag) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return 0;
    }
    int value = 0;
    int alt = sq->alt_level();
    bool has_limit = false;
    bool bonus_landmark = false;
    bool bonus_nutrient = bonus_at(x, y) == RES_NUTRIENT;
    bool is_base = sq->is_base();
    int planet = Factions[faction_id].SE_planet_pending;

    if ((alt >= ALT_SHORE_LINE && sq->landmarks & LM_JUNGLE && !(sq->landmarks & LM_DISABLE))
    || (alt < ALT_SHORE_LINE && sq->landmarks & LM_FRESH)) {
        bonus_landmark = true;
    }
    if (is_base) {
        value = ResInfo->base_sq.nutrient;
        if (bonus_nutrient) {
            value = 2 * ResInfo->base_sq.nutrient;
        }
        if (bonus_landmark) {
            value++;
        }
        if (has_fac_built(FAC_PRESSURE_DOME, base_id)
        || has_fac_built(FAC_RECYCLING_TANKS, base_id)) {
            value += ResInfo->recycling_tanks.energy;
        }
    }
    else if (sq->items & BIT_THERMAL_BORE) {
        value = ResInfo->borehole_sq.nutrient;
        if (bonus_nutrient) {
            value += ResInfo->bonus_sq.nutrient;
        }
    }
    else if (sq->items & BIT_MONOLITH) {
        value = ResInfo->monolith_sq.nutrient;
        if (bonus_nutrient) {
            value += ResInfo->bonus_sq.nutrient;
        }
        if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
            value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][0];
        }
    }
    else if (sq->items & BIT_FUNGUS && !flag && alt >= ALT_OCEAN_SHELF) {
        int fungus_val = clamp(planet, -3, 0) + Factions[faction_id].tech_fungus_nutrient;
        value = clamp(fungus_val, 0, 99);
        if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
            value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][0];
        }
    } else {
        has_limit = true;
        if (alt < ALT_SHORE_LINE) {
            value = ResInfo->ocean_sq.nutrient;
            if (bonus_nutrient) {
                value = ResInfo->bonus_sq.nutrient + ResInfo->ocean_sq.nutrient;
            }
            if (bonus_landmark)
                value++;
            if (alt == ALT_OCEAN_SHELF || *ExpansionEnabled) {
                if (sq->items & BIT_FARM) {
                    value += ResInfo->improved_sea.nutrient;
                    if (has_fac_built(FAC_AQUAFARM, base_id)) {
                        value++;
                    }
                }
                if (sq->items & BIT_MINE && value > 1) {
                    value += Rules->nutrient_effect_mine_sq;
                }
            }
        } else {
            if (sq->items & BIT_FOREST) {
                value = ResInfo->forest_sq.nutrient
                    + (bonus_nutrient ? ResInfo->bonus_sq.nutrient : 0);
                if (has_fac_built(FAC_TREE_FARM, base_id)) {
                    value++;
                }
                if (has_fac_built(FAC_HYBRID_FOREST, base_id)) {
                    value++;
                }
                if (bonus_landmark) {
                    value++;
                }
            } else {
                if (sq->is_rocky()) {
                    value = 0;
                } else {
                    value = (sq->is_rainy() ? 2 : (sq->is_moist() ? 1 : 0));
                }
                if (bonus_nutrient) {
                    value += ResInfo->bonus_sq.nutrient;
                }
                if (bonus_landmark) {
                    value++;
                }
                if (value < 0) {
                    value = 0;
                }
                if (sq->items & BIT_FARM && !sq->is_rocky()) {
                    value += ResInfo->improved_land.nutrient;
                }
                if (sq->items & BIT_MINE && value > 1) {
                    value += Rules->nutrient_effect_mine_sq;
                }
            }
        }
        if (sq->items & BIT_SOIL_ENRICHER) {
            value += (conf.soil_improve_value ? conf.soil_improve_value : value / 2);
        }
        if (sq->items & BIT_CONDENSER) {
            value += (conf.soil_improve_value ? conf.soil_improve_value : value / 2);
        }
    }
    if (has_limit && value > conf.tile_output_limit[0] && !bonus_nutrient && !(sq->items & BIT_CONDENSER)
    && (faction_id < 0 || !has_tech(Rules->tech_preq_allow_3_nutrients_sq, faction_id))) {
        *BaseTerraformReduce += (value - conf.tile_output_limit[0]);
        value = conf.tile_output_limit[0];
    }
    if (base_id >= 0) {
        if (Bases[base_id].event_flags & BEVENT_BUMPER) {
            value++;
        }
        if (Bases[base_id].event_flags & BEVENT_FAMINE && value) {
            value--;
        }
    }
    assert((conf.soil_improve_value && sq->items & (BIT_CONDENSER|BIT_SOIL_ENRICHER))
        || (has_limit && !bonus_nutrient && conf.tile_output_limit[0] != 2)
        || value == crop_yield(faction_id, base_id, x, y, flag));
    return value;
}

int __cdecl mod_mine_yield(int faction_id, int base_id, int x, int y, int flag) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return 0;
    }
    bool has_limit = true;
    bool bonus_landmark = false;
    bool bonus_mineral = bonus_at(x, y) == RES_MINERAL;
    bool is_base = sq->is_base();
    int alt = sq->alt_level();
    int planet = Factions[faction_id].SE_planet_pending;

    if ((sq->landmarks & LM_CRATER && sq->code_at() < 9)
    || (sq->landmarks & LM_VOLCANO && sq->code_at() < 9)
    || (sq->landmarks & LM_FOSSIL && sq->code_at() < 6)
    || (sq->landmarks & LM_CANYON)) {
        bonus_landmark = !(sq->landmarks & LM_DISABLE);
    }
    int value = bonus_landmark + (bonus_mineral ? ResInfo->bonus_sq.mineral : 0);

    if (is_base) {
        value += ResInfo->base_sq.mineral;
        if (has_fac_built(FAC_PRESSURE_DOME, base_id)
        || has_fac_built(FAC_RECYCLING_TANKS, base_id)) {
            value += ResInfo->recycling_tanks.mineral;
        }
        has_limit = false;
    } else {
        if (sq->items & BIT_MONOLITH) {
            value = ResInfo->monolith_sq.mineral;
            if (bonus_mineral) {
                value += ResInfo->bonus_sq.mineral;
            }
            if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
                value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][1];
            }
            has_limit = false;
        }
        else if (sq->items & BIT_THERMAL_BORE) {
            value = ResInfo->borehole_sq.mineral;
            if (bonus_mineral) {
                value += ResInfo->bonus_sq.mineral;
            }
        }
        else if (sq->items & BIT_FUNGUS && alt >= ALT_OCEAN_SHELF) {
            int fungus_val = clamp(planet, -3, 0) + Factions[faction_id].tech_fungus_mineral;
            value = clamp(fungus_val, 0, 99);
            if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
                value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][1];
            }
            has_limit = false;
        }
        else if (alt >= ALT_SHORE_LINE) {
            int modifier = sq->val3 >> 6;
            if (sq->items & BIT_FOREST) {
                value += ResInfo->forest_sq.mineral;
            } else if (!(sq->items & BIT_MINE) && !flag) {
                value += (modifier > 0);
            } else {
                value += modifier;
                if (!modifier) {
                    modifier = 1;
                }
                if (bonus_mineral || bonus_landmark) {
                    modifier++;
                }
                if (!(sq->items & BIT_ROAD) && !flag
                && modifier > Rules->limit_mineral_inc_for_mine_wo_road) {
                    *BaseTerraformReduce += (modifier - Rules->limit_mineral_inc_for_mine_wo_road);
                    modifier = Rules->limit_mineral_inc_for_mine_wo_road;
                }
                value += modifier;
            }
        }
        else {
            value += ResInfo->ocean_sq.mineral;
            if (alt == ALT_OCEAN_SHELF && MFactions[faction_id].is_aquatic()
            && conf.aquatic_bonus_minerals) {
                value++;
            }
            if (alt == ALT_OCEAN_SHELF || *ExpansionEnabled) {
                if (sq->items & BIT_MINE || flag) {
                    value += ResInfo->improved_sea.mineral;
                    if (has_tech(Rules->tech_preq_mining_platform_bonus, faction_id) ) {
                        value++;
                    }
                    if (has_fac_built(FAC_SUBSEA_TRUNKLINE, base_id)) {
                        value++;
                    }
                }
            }
        }
    }
    if (has_limit && value > conf.tile_output_limit[1] && !bonus_mineral
    && (faction_id < 0 || !has_tech(Rules->tech_preq_allow_3_minerals_sq, faction_id))) {
        *BaseTerraformReduce += (value - conf.tile_output_limit[1]);
        value = conf.tile_output_limit[1];
    }
    if (base_id >= 0) {
        if (Bases[base_id].event_flags & BEVENT_INDUSTRY) {
            value++;
        }
        if (Bases[base_id].event_flags & BEVENT_BUST && value) {
            value--;
        }
    }
    // Original function can return inconsistent sea mineral output when base_id is not set
    assert((base_id < 0 && alt == ALT_OCEAN_SHELF && MFactions[faction_id].is_aquatic())
        || (has_limit && !bonus_mineral && conf.tile_output_limit[1] != 2)
        || (value == mine_yield(faction_id, base_id, x, y, flag)));
    return value;
}

int __cdecl mod_energy_yield(int faction_id, int base_id, int x, int y, int flag) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return 0;
    }
    bool is_fungus = false;
    bool has_limit = true;
    bool bonus_energy = bonus_at(x, y) == RES_ENERGY;
    bool is_base = sq->is_base();
    int economy = Factions[faction_id].SE_economy_pending
        + (base_id >= 0 && Bases[base_id].golden_age_active() ? 1 : 0);
    int planet = Factions[faction_id].SE_planet_pending;
    int alt = sq->alt_level();
    int value = 0;

    if (is_base) {
        bool is_hq = has_fac_built(FAC_HEADQUARTERS, base_id);
        if (has_fac_built(FAC_PRESSURE_DOME, base_id)
        || has_fac_built(FAC_RECYCLING_TANKS, base_id)) {
            value += ResInfo->recycling_tanks.energy;
        }
        if (economy > 4) {
            value += 4;
        } else if (economy > 3) {
            value += 2;
        } else if (economy >= 0) {
            value += (economy == 1); // +1 each square is added later
        } else {
            value += economy;
            if (!is_hq || economy < -1 || Factions[faction_id].base_count == 1) {
                value++;
            }
        }
        value += 1 + is_hq;
        if (*GovernorFaction == faction_id) {
            value++;
        }
        has_limit = false;
    }
    // Monolith takes priority over any fungus values on the same tile.
    // Modify the game to not apply 2 resource yield restrictions on monolith energy.
    // This limit does not apply on nutrients/minerals produced by monoliths.
    else if (sq->items & BIT_MONOLITH) {
        value = ResInfo->monolith_sq.energy;
        if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
            value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][2];
        }
        has_limit = false;
    }
    else if (sq->items & BIT_THERMAL_BORE) {
        value = ResInfo->borehole_sq.energy;
    }
    else if (sq->items & BIT_FUNGUS && alt >= ALT_OCEAN_SHELF) {
        int fungus_val = clamp(planet, -3, 0) + Factions[faction_id].tech_fungus_energy;
        value = clamp(fungus_val, 0, 99);
        if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
            value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][2];
        }
        is_fungus = true;
        has_limit = false;
    }
    else if (alt < ALT_SHORE_LINE) {
        if (alt == ALT_OCEAN_SHELF || *ExpansionEnabled) {
            value = ResInfo->ocean_sq.energy;
            if (sq->items & BIT_SOLAR || flag) {
                value += ResInfo->improved_sea.energy;
                if (has_fac_built(FAC_THERMOCLINE_TRANSDUCER, base_id)) {
                    value++;
                }
            }
        }
        if (sq->items & BIT_FOREST) {
            value--;
        }
    }
    else if (sq->items & BIT_FOREST) {
        value = ResInfo->forest_sq.energy;
        if (has_fac_built(FAC_HYBRID_FOREST, base_id)) {
            value++;
        }
    }
    else {
        if (sq->items & (BIT_ECH_MIRROR|BIT_SOLAR) || flag) {
            value += 1 + max(0, alt - ALT_SHORE_LINE);
        }
        // Fix crash issue by adding null pointer check for CurrentBase
        if (sq->items & BIT_SOLAR && *CurrentBase) {
            for (auto& m : iterate_tiles(x, y, 1, 9)) {
                if (m.sq->items & BIT_ECH_MIRROR
                && m.sq->owner == (*CurrentBase)->faction_id) {
                    value++;
                }
            }
        }
    }
    if (!is_fungus) {
        if (sq->items & BIT_RIVER && alt >= ALT_SHORE_LINE) {
            value++;
        }
        if (bonus_energy) {
            value += ResInfo->bonus_sq.energy;
        }
        if ((sq->landmarks & LM_VOLCANO && sq->code_at() < 9)
        || sq->landmarks & (LM_URANIUM|LM_GEOTHERMAL|LM_RIDGE)) {
            value += !(sq->landmarks & LM_DISABLE);
        }
        if (base_id >= 0 && project_base(FAC_MERCHANT_EXCHANGE) == base_id) {
            value++;
        }
        if (economy >= 2) {
            value++;
        }
        if (is_base) {
            value = clamp(value, 0, 99);
            *BaseTerraformEnergy = value;
        }
    }
    if (has_limit && value > conf.tile_output_limit[2] && !bonus_energy
    && (faction_id < 0 || !has_tech(Rules->tech_preq_allow_3_energy_sq, faction_id))) {
        *BaseTerraformReduce += (value - conf.tile_output_limit[2]);
        value = conf.tile_output_limit[2];
    }
    if (base_id >= 0) {
        if (Bases[base_id].event_flags & BEVENT_HEAT_WAVE) {
            value++;
        }
        if (Bases[base_id].event_flags & BEVENT_CLOUD_COVER && value) {
            value--;
        }
    }
    if (*DustCloudDuration && value) {
        value--;
    }
    bool solar_flares;
    if (*ControlUpkeepA) {
        solar_flares = *SolarFlaresEvent & 2;
    } else {
        solar_flares = *SolarFlaresEvent & 1;
    }
    value = (solar_flares ? 3 * value : value);

    // Original function can return inconsistent base output when economy is between 3 and 4
    assert((is_base && economy >= 3 && economy <= 4)
        || (!is_base && sq->items & BIT_MONOLITH && !has_tech(Rules->tech_preq_allow_3_energy_sq, faction_id))
        || (has_limit && !bonus_energy && conf.tile_output_limit[2] != 2)
        || (value == energy_yield(faction_id, base_id, x, y, flag)));
    return value;
}

static int __cdecl base_hex_cost(int unit_id, int faction_id, int x1, int y1, int x2, int y2, int toggle) {
    MAP* sq_dst = mapsq(x2, y2);
    uint32_t bit_dst = (sq_dst ? sq_dst->items : 0);
    if (is_ocean(sq_dst)) {
        if (bit_dst & BIT_FUNGUS
        && sq_dst->alt_level() == ALT_OCEAN_SHELF
        && Units[unit_id].triad() == TRIAD_SEA
        && unit_id != BSC_SEALURK // Bug fix
        && unit_id != BSC_ISLE_OF_THE_DEEP
        && !has_project(FAC_XENOEMPATHY_DOME, faction_id)) {
            return Rules->move_rate_roads * 3;
        }
        return Rules->move_rate_roads;
    }
    MAP* sq_src = mapsq(x1, y1);
    uint32_t bit_src = (sq_src ? sq_src->items : 0);
    if (is_ocean(sq_src)) {
        return Rules->move_rate_roads;
    }
    if (unit_id >= 0 && Units[unit_id].triad() != TRIAD_LAND) {
        return Rules->move_rate_roads;
    }
    // Land only conditions
    if (bit_src & (BIT_MAGTUBE | BIT_BASE_IN_TILE) && bit_dst & (BIT_MAGTUBE | BIT_BASE_IN_TILE)
    && faction_id) {
        return 0;
    }
    if ((bit_src & (BIT_ROAD | BIT_BASE_IN_TILE) || (bit_src & BIT_FUNGUS && faction_id > 0
    && has_project(FAC_XENOEMPATHY_DOME, faction_id))) && bit_dst & (BIT_ROAD | BIT_BASE_IN_TILE)
    && faction_id) {
        return 1;
    }
    if (faction_id >= 0 && (has_project(FAC_XENOEMPATHY_DOME, faction_id) || !faction_id
    || unit_id == BSC_MIND_WORMS || unit_id == BSC_SPORE_LAUNCHER) && bit_dst & BIT_FUNGUS) {
        return 1;
    }
    if (bit_src & BIT_RIVER && bit_dst & BIT_RIVER && x_dist(x1, x2) == 1
    && abs(y1 - y2) == 1 && faction_id) {
        return 1;
    }
    if (Units[unit_id].chassis_id == CHS_HOVERTANK
    || has_abil(unit_id, ABL_ANTIGRAV_STRUTS)) {
        return Rules->move_rate_roads;
    }
    int cost = Rules->move_rate_roads;
    if (sq_dst->is_rocky() && !toggle) {
        cost += Rules->move_rate_roads;
    }
    if (bit_dst & BIT_FOREST && !toggle) {
        cost += Rules->move_rate_roads;
    }
    if (faction_id && bit_dst & BIT_FUNGUS && (unit_id >= MaxProtoFactionNum
    || Units[unit_id].offense_value() >= 0)) {
        int plan = Units[unit_id].plan;
        if (plan != PLAN_TERRAFORM && plan != PLAN_ARTIFACT
        && Factions[faction_id].SE_planet <= 0) {
            return cost + Rules->move_rate_roads * 2;
        }
        int value = proto_speed(unit_id);
        if (cost <= value) {
            return value;
        }
    }
    return cost;
}

int __cdecl mod_hex_cost(int unit_id, int faction_id, int x1, int y1, int x2, int y2, int toggle) {
    int value = base_hex_cost(unit_id, faction_id, x1, y1, x2, y2, toggle);
    MAP* sq_a = mapsq(x1, y1);
    MAP* sq_b = mapsq(x2, y2);

    if (DEBUG && sq_a && sq_b) {
        assert(value == hex_cost(unit_id, faction_id, x1, y1, x2, y2, toggle));
    }
    if (conf.magtube_movement_rate > 0 && Units[unit_id].triad() == TRIAD_LAND) {
        if (!is_ocean(sq_a) && !is_ocean(sq_b)) {
            if (sq_a->items & (BIT_BASE_IN_TILE | BIT_MAGTUBE)
            && sq_b->items & (BIT_BASE_IN_TILE | BIT_MAGTUBE)) {
                value = 1;
            } else if (value == 1) { // Moving along a road
                value = conf.road_movement_rate;
            }
        }
    }
    if (conf.fast_fungus_movement > 0 && Units[unit_id].triad() != TRIAD_AIR) {
        if (!is_ocean(sq_b) && sq_b->is_fungus()) {
            value = min(max(proto_speed(unit_id), Rules->move_rate_roads), value);
        }
    }
    return value;
}

/*
Determine the tile mineral count that translates to rockiness.
Return Value: 0 (Flat), 1 (Rolling), 2 (Rocky)
*/
int __cdecl mod_minerals_at(int x, int y) {
    if (!y || y == (*MapAreaY - 1)) {
        return 2;
    }
    MAP* sq = mapsq(x, y);
    int alt = sq->alt_level();
    int avg = (x + y) / 2;
    int val1 = (x - avg) / 2 + 2 * ((x - avg) / 2) + 2 * (avg / 2) + *MapRandomSeed;
    int val2 = ((byte)val1 - 2 * (((byte)x - (byte)avg) & 1) - (avg & 1)) & 3;
    int val3 = (alt - 3 >= 0 ? alt - 3 : 3 - alt) - (alt < 3);
    if (val3 == 0) {
        if (val2 == 0) {
            return 1;
        }
        if (val2 == 1 || val2 == 3) {
            return 0;
        }
        if (val2 == 2) {
            return (val1 & 4 ? 2 : 1);
        }
        return ~val2 & 1;
    } else if (val3 == 1) {
        if (!val2) {
            return 0;
        }
        if (!(val2 - 1)) {
            return 1;
        }
        if ((val2 - 1) != 1) {
            return 2;
        }
        return 1;
    } else if (val3 == 2) {
        if (!val2) {
            return 0;
        }
        if (val2 == 1) {
            return 1;
        }
        return 2;
    } else if (val3 == 3) {
        if (val2 < 0 || val2 > 1) {
            return 2;
        }
        return 1;
    } else {
        return ~val2 & 1;
    }
}

/*
Determine if the tile has a resource bonus.
Return Value: 0 (no bonus), 1 (nutrient), 2 (mineral), 3 (energy)
*/
int __cdecl mod_bonus_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    uint32_t bit = sq->items;
    uint32_t alt = sq->alt_level();
    bool has_rsc_bonus = bit & BIT_BONUS_RES;
    if (!has_rsc_bonus && (!*MapRandomSeed || (alt >= ALT_SHORE_LINE
    && !conf.rare_supply_pods && !(*GameRules & RULES_NO_UNITY_SCATTERING)))) {
        return 0;
    }
    int avg = (x + y) >> 1;
    x -= avg;
    int chk = (avg & 3) + 4 * (x & 3);
    if (!has_rsc_bonus && chk != ((*MapRandomSeed + (-5 * (avg >> 2)) - 3 * (x >> 2)) & 0xF)) {
        return 0;
    }
    if (alt < ALT_OCEAN_SHELF) {
        return 0;
    }
    int ret = (alt < ALT_SHORE_LINE && !conf.rare_supply_pods) ? chk % 3 + 1 : (chk % 5) & 3;
    if (!ret || bit & BIT_NUTRIENT_RES) {
        if (bit & BIT_ENERGY_RES) {
            return 3; // energy
        }
        return ((bit & BIT_MINERAL_RES) != 0) + 1; // nutrient or mineral
    }
    return ret;
}

/*
Determine if the tile has a supply pod and if so what type.
Return Value: 0 (no supply pod), 1 (standard supply pod), 2 (unity pod?)
*/
int __cdecl mod_goody_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    uint32_t bit = sq->items;
    uint32_t alt = sq->alt_level();
    if (bit & (BIT_SUPPLY_REMOVE | BIT_MONOLITH)) {
        return 0; // nothing, supply pod already opened or monolith
    }
    if (*GameRules & RULES_NO_UNITY_SCATTERING) {
        return (bit & (BIT_UNK_4000000 | BIT_UNK_8000000)) ? 2 : 0; // ?
    }
    if (bit & BIT_SUPPLY_POD) {
        return 1; // supply pod
    }
    if (!*MapRandomSeed) {
        return 0; // nothing
    }
    int avg = (x + y) >> 1;
    int x_diff = x - avg;
    int cmp = (avg & 3) + 4 * (x_diff & 3);
    if (!conf.rare_supply_pods && alt >= ALT_SHORE_LINE) {
        if (cmp == ((-5 * (avg >> 2) - 3 * (x_diff >> 2) + *MapRandomSeed) & 0xF)) {
            return 2;
        }
    }
    return cmp == ((11 * (avg / 4) + 61 * (x_diff / 4) + *MapRandomSeed + 8) & 0x1F); // 0 or 1
}

/*
This version adds support for modified territory borders (earlier bases claim tiles first).
*/
int __cdecl mod_base_find3(int x, int y, int faction_id, int region, int faction_id_2, int faction_id_3) {
    int base_dist = 9999;
    int base_id = -1;
    bool border_fix = conf.territory_border_fix && region >= MaxRegionLandNum;

    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (border_fix || region < 0 || region_at(base->x, base->y) == region) {
            if (faction_id < 0 ? (faction_id_2 < 0 || base->faction_id != faction_id_2)
            : (faction_id == base->faction_id
            || (faction_id_2 == -2 ? has_treaty(faction_id, base->faction_id, DIPLO_PACT)
            : (faction_id_2 >= 0 && faction_id_2 == base->faction_id)))) {
                if (faction_id_3 < 0 || base->faction_id == faction_id_3
                || base->visibility & (1 << faction_id_3)) {
                    int dist = vector_dist(x, y, base->x, base->y);
                    if (conf.territory_border_fix ? dist < base_dist : dist <= base_dist) {
                        base_dist = dist;
                        base_id = i;
                    }
                }
            }
        }
    }
    if (DEBUG && !conf.territory_border_fix) {
        int value = base_find3(x, y, faction_id, region, faction_id_2, faction_id_3);
        assert(base_id == value);
        assert(base_dist == *BaseFindDist);
    }
    *BaseFindDist = 9999;
    if (base_id >= 0) {
        *BaseFindDist = base_dist;
    }
    return base_id;
}

int __cdecl mod_whose_territory(int faction_id, int x, int y, int* base_id, int ignore_comm) {
    MAP* sq = mapsq(x, y);
    if (!sq || sq->owner < 0) { // Fix: invalid coordinates return -1 (no owner)
        return -1;
    }
    if (faction_id != sq->owner) {
        if (!ignore_comm
        && !(*GameState & STATE_OMNISCIENT_VIEW)
        && (!has_treaty(faction_id, sq->owner, DIPLO_COMMLINK)
        || !has_treaty(faction_id, sq->owner, DIPLO_UNK_8000000))) {
            return -1;
        }
        if (base_id) {
            *base_id = mod_base_find3(x, y, -1, sq->region, -1, -1);
        }
    }
    return sq->owner;
}

int bonus_yield(int res_type) {
    switch (res_type) {
    case RES_NUTRIENT: return ResInfo->bonus_sq.nutrient;
    case RES_MINERAL: return ResInfo->bonus_sq.mineral;
    case RES_ENERGY: return ResInfo->bonus_sq.energy;
    default: return 0;
    }
}

int total_yield(int x, int y, int faction_id) {
    return mod_crop_yield(faction_id, -1, x, y, 0)
        + mod_mine_yield(faction_id, -1, x, y, 0)
        + mod_energy_yield(faction_id, -1, x, y, 0);
}

int fungus_yield(int faction_id, ResType res_type) {
    Faction* f = &Factions[faction_id];
    int p = clamp(f->SE_planet_pending, -3, 0);
    int N = clamp(f->tech_fungus_nutrient + p, 0, 99);
    int M = clamp(f->tech_fungus_mineral + p, 0, 99);
    int E = clamp(f->tech_fungus_energy + p + (f->SE_economy_pending >= 2), 0, 99);

    if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
        int m = clamp(f->SE_planet_pending + 1, 0, 4);
        N += ManifoldHarmonicsBonus[m][0];
        M += ManifoldHarmonicsBonus[m][1];
        E += ManifoldHarmonicsBonus[m][2];
    }
    if (res_type == RES_NUTRIENT) {
        return N;
    }
    if (res_type == RES_MINERAL) {
        return M;
    }
    if (res_type == RES_ENERGY) {
        return E;
    }
    return N+M+E;
}

int item_yield(int x, int y, int faction_id, int bonus, MapItem item) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return 0;
    }
    int N = 0, M = 0, E = 0;
    if (item != BIT_FUNGUS) {
        if (bonus == RES_NUTRIENT) {
            N += ResInfo->bonus_sq.nutrient;
        }
        if (bonus == RES_MINERAL) {
            M += ResInfo->bonus_sq.mineral;
        }
        if (bonus == RES_ENERGY) {
            E += ResInfo->bonus_sq.energy;
        }
        if (sq->items & BIT_RIVER && !is_ocean(sq)) {
            E++;
        }
        if (sq->landmarks & LM_CRATER && sq->code_at() < 9) { M++; }
        if (sq->landmarks & LM_VOLCANO && sq->code_at() < 9) { M++; E++; }
        if (sq->landmarks & LM_JUNGLE) { N++; }
        if (sq->landmarks & LM_URANIUM) { E++; }
        if (sq->landmarks & LM_FRESH && is_ocean(sq)) { N++; }
        if (sq->landmarks & LM_GEOTHERMAL) { E++; }
        if (sq->landmarks & LM_RIDGE) { E++; }
        if (sq->landmarks & LM_CANYON) { M++; }
        if (sq->landmarks & LM_FOSSIL && sq->code_at() < 6) { M++; }
        if (Factions[faction_id].SE_economy_pending >= 2) {
            E++;
        }
    }
    if (item == BIT_FOREST) {
        N += ResInfo->forest_sq.nutrient;
        M += ResInfo->forest_sq.mineral;
        E += ResInfo->forest_sq.energy;
    }
    else if (item == BIT_THERMAL_BORE) {
        N += ResInfo->borehole_sq.nutrient;
        M += ResInfo->borehole_sq.mineral;
        E += ResInfo->borehole_sq.energy;
        N -= (N > 0);
    }
    else if (item == BIT_FUNGUS) {
        N = fungus_yield(faction_id, RES_NUTRIENT);
        M = fungus_yield(faction_id, RES_MINERAL);
        E = fungus_yield(faction_id, RES_ENERGY);
    }
    else if (item == BIT_FARM && is_ocean(sq)) {
        N += ResInfo->ocean_sq.nutrient + ResInfo->improved_sea.nutrient;
        M += ResInfo->ocean_sq.mineral;
        E += ResInfo->ocean_sq.energy;
        if (sq->items & BIT_MINE) {
            M += ResInfo->improved_sea.mineral
                + has_tech(Rules->tech_preq_mining_platform_bonus, faction_id);
            N -= (N > 0);
        }
        if (sq->items & BIT_SOLAR) {
            E += ResInfo->improved_sea.energy;
        }
    }
    else {
        assert(0);
    }
    if (N > 2 && bonus != RES_NUTRIENT
    && !has_tech(Rules->tech_preq_allow_3_nutrients_sq, faction_id)) {
        N = 2;
    }
    if (M > 2 && bonus != RES_MINERAL
    && !has_tech(Rules->tech_preq_allow_3_minerals_sq, faction_id)) {
        M = 2;
    }
    if (E > 2 && bonus != RES_ENERGY
    && !has_tech(Rules->tech_preq_allow_3_energy_sq, faction_id)) {
        E = 2;
    }
    if (conf.debug_verbose) {
        debug("item_yield %2d %2d %08X faction: %d bonus: %d planet: %d N: %d M: %d E: %d prev: %d\n",
        x, y, item, faction_id, bonus, Factions[faction_id].SE_planet_pending,
        N, M, E, total_yield(x, y, faction_id));
    }
    return N+M+E;
}

static void process_map(int faction_id, int k) {
    spawns.clear();
    natives.clear();
    goodtiles.clear();
    /*
    This value defines how many tiles an area needs to have before it's
    considered sufficiently large to be a faction starting location.
    Map area square root values: Tiny = 33, Standard = 56, Huge = 90
    */
    int limit = clamp(*MapAreaTiles, 1024, 8192) / 40;
    size_t land_area = 0;
    MAP* sq;

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x+=2) {
            if (y < k || y >= *MapAreaY - k || !(sq = mapsq(x, y))) {
                continue;
            }
            // LM_FRESH landmark is ignored for this check
            if (!is_ocean(sq) && !sq->is_rocky() && !sq->is_fungus()
            && sq->is_land_region() && !(sq->lm_items() & ~LM_FRESH)
            && Continents[sq->region].tile_count >= limit) {
                goodtiles.insert({x, y});
            }
            if (sq->is_land_region()) {
                land_area++;
            }
        }
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehicles[i];
        if (v->faction_id == 0) {
            natives.insert({v->x, v->y});
        }
        if (v->faction_id != 0 && v->faction_id != faction_id) {
            spawns.insert({v->x, v->y});
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        spawns.insert({Bases[i].x, Bases[i].y});
    }
    if (goodtiles.size() * 3 < land_area) {
        goodtiles.clear();
    }
    debug("process_map x: %d y: %d sqrt: %d tiles: %d good: %d\n",
        *MapAreaX, *MapAreaY, *MapAreaSqRoot, *MapAreaTiles, goodtiles.size());
}

static bool valid_start(int faction_id, int iter, int x, int y) {
    MAP* sq = mapsq(x, y);
    bool aquatic = MFactions[faction_id].is_aquatic();
    int native_limit = (goodtiles.size() > 0 ? 3 : 2) + ((int)natives.size() < *MapAreaTiles/80);
    int spawn_limit = max((*MapAreaTiles < 1600 ? 5 : 7), 8 - iter/100);

    if (!sq || !sq->allow_spawn()) { // Select only tiles where bases can be built
        return false;
    }
    // LM_FRESH landmark is incorrectly used on some maps
    if (aquatic != is_ocean(sq) || ((sq->lm_items() & ~LM_FRESH) && iter < 160)) {
        return false;
    }
    if ((goody_at(x, y) > 0 || bonus_at(x, y) > 0) && iter < 160) {
        return false;
    }
    if (min_range(natives, x, y) < max(native_limit, 8 - iter/16)) {
        return false;
    }
    int spawn_range = min_range(spawns, x, y);
    int min_sc = 80 - iter/4 + 20 * max(0, 12 - spawn_range);
    int sea = 0;
    int sc = 0;
    int xd = 0;
    int yd = 0;
    if (spawn_range < clamp(*MapAreaSqRoot/4 + 8 - iter/8, spawn_limit, 32)) {
        return false;
    }
    if (aquatic) {
        if (Continents[sq->region].tile_count < *MapAreaTiles / (16 << (iter/32))) {
            return false;
        }
    } else {
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
    for (auto& m : iterate_tiles(x, y, 0, 45)) {
        int bonus = bonus_at(m.x, m.y);
        if (is_ocean(m.sq)) {
            if (!is_ocean_shelf(m.sq)) {
                sc--;
            } else if (aquatic) {
                sc += 4;
            }
            sea++;
        } else {
            sc += (m.sq->is_rainy_or_moist() ? 3 : 1);
            if (m.sq->items & BIT_RIVER) {
                sc += 5;
            }
            if (m.sq->items & BIT_FOREST) {
                sc += 4;
            }
            if (m.sq->is_rolling()) {
                sc += 1;
            }
        }
        if (bonus != RES_NONE) {
            sc += (m.i < 25 ? 15 : 10);
        }
        if (goody_at(m.x, m.y) > 0) {
            sc += 15;
        }
        if (m.sq->items & BIT_FUNGUS) {
            sc -= (m.i <= 20 ? 4 : 2) * (is_ocean(m.sq) ? 1 : 2);
        }
        if (m.sq->items & BIT_MONOLITH) {
            sc += 8;
        }
    }
    debug("find_score %d %3d x: %3d y: %3d xd: %d yd: %d min: %d score: %d\n",
        faction_id, iter, x, y, xd, yd, min_sc, sc);

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

static void apply_nutrient_bonus(int faction_id, int* x, int* y) {
    MAP* sq;
    Points addon;
    Points places;
    Points allpods;
    Points rivers;
    bool aquatic = MFactions[faction_id].is_aquatic();
    int adjust = (aquatic ? 0 : 8);
    int nutrient = 0;
    int num = 0;

    // Bonus resources are placed adjacent to each other only on diagonals
    for (auto& m : iterate_tiles(*x, *y, 0, 80)) {
        int bonus = bonus_at(m.x, m.y);
        int goody = goody_at(m.x, m.y);
        if (aquatic == is_ocean(m.sq) && m.sq->alt_level() >= ALT_OCEAN_SHELF
        && (m.i < 25 || (m.i < 45 && nutrient + (int)places.size()/4 < conf.nutrient_bonus))) {
            if (bonus == RES_NUTRIENT) {
                if (nutrient < conf.nutrient_bonus && m.sq->items & BIT_FUNGUS) {
                    m.sq->items &= ~BIT_FUNGUS;
                }
                if (m.sq->is_rocky()) {
                    rocky_set(m.x, m.y, LEVEL_ROLLING);
                }
                synch_bit(m.x, m.y, faction_id);
                nutrient++;
            } else if (!goody && bonus == RES_NONE
            && adjust > 0 && m.sq->items & BIT_RIVER) {
                if (m.i == 0) {
                    adjust = 0;
                }
                if (m.i > 0 && m.i <= adjust
                && m.sq->region == region_at(*x, *y)
                && can_build_base(m.x, m.y, faction_id, TRIAD_LAND)
                && min_range(spawns, m.x, m.y) >= 8) {
                    rivers.insert({m.x, m.y});
                }
            }
        }
        if (bonus || goody) {
            allpods.insert({m.x, m.y});
        }
        else if (m.i < 45 && (int)places.size()/4 < conf.nutrient_bonus
        && aquatic == is_ocean(m.sq) && m.sq->alt_level() >= ALT_OCEAN_SHELF
        && !(m.sq->items & (BIT_BASE_IN_TILE|BIT_VEH_IN_TILE|BIT_MONOLITH))) {
            places.insert({m.x, m.y});
        }
    }
    while (places.size() > 0 && nutrient + (int)addon.size() < conf.nutrient_bonus) {
        auto t = pick_random(places);
        bool found = false;
        for (auto& p : allpods) {
            if (wrap(abs(p.x - t.x)) == 1 && abs(p.y - t.y) == 1) {
                found = true; break;
            }
        }
        if (!found) {
            addon.insert({t.x, t.y});
            allpods.insert({t.x, t.y});
        }
        places.erase(t);
    }
    for (auto& t : addon) {
        sq = mapsq(t.x, t.y);
        sq->items &= ~(BIT_FUNGUS | BIT_MINERAL_RES | BIT_ENERGY_RES);
        sq->items |= (BIT_SUPPLY_REMOVE | BIT_BONUS_RES | BIT_NUTRIENT_RES);
        if (sq->is_rocky()) {
            rocky_set(t.x, t.y, LEVEL_ROLLING);
        }
        synch_bit(t.x, t.y, faction_id);
        num++;
    }
    // Adjust position to adjacent river if currently not on river
    if (adjust > 0 && rivers.size() > 0) {
        auto t = pick_random(rivers);
        *x = t.x;
        *y = t.y;
    }
}

static void apply_supply_pods(int faction_id, int x, int y) {
    MAP* sq;
    Points places;
    int bonus = 0;
    int pods = 0;
    for (auto& m : iterate_tiles(x, y, 0, 25)) {
        if (conf.rare_supply_pods > 1) {
            m.sq->items |= BIT_SUPPLY_REMOVE;
            synch_bit(m.x, m.y, faction_id);
        } else if (goody_at(m.x, m.y)) {
            pods++;
        } else if (bonus_at(m.x, m.y)) {
            bonus++;
        } else if (m.i > 0 && m.sq->allow_supply()) {
            places.insert({m.x, m.y});
        }
    }
    if (conf.rare_supply_pods <= 1) {
        int adjust = clamp(random(10) - bonus, 2, 4) - pods;
        while (adjust > 0 && places.size() > 0) {
            auto t = pick_random(places);
            sq = mapsq(t.x, t.y);
            sq->items |= BIT_UNK_8000000;
            synch_bit(t.x, t.y, faction_id);
            adjust--;
            places.erase(t);
        }
    }
}

void __cdecl find_start(int faction_id, int* tx, int* ty) {
    bool aquatic = MFactions[faction_id].is_aquatic();
    int x = 0;
    int y = 0;
    int i = 0;
    int k = (*MapAreaY < 80 ? 4 : 8);
    process_map(faction_id, k/2);

    while (++i <= 800) {
        if (!aquatic && goodtiles.size() > 0 && i <= 200) {
            auto t = pick_random(goodtiles);
            y = t.y;
            x = t.x;
        } else {
            y = (random(*MapAreaY - k*2) + k);
            x = (random(*MapAreaX) & (~1)) + (y&1);
        }
        if (valid_start(faction_id, i, x, y)) {
            if (conf.nutrient_bonus > 0) {
                apply_nutrient_bonus(faction_id, &x, &y);
            }
            // No unity scattering can normally spawn pods at two tile range from the start
            if (*GameRules & RULES_NO_UNITY_SCATTERING) {
                apply_supply_pods(faction_id, x, y);
            }
            *tx = x;
            *ty = y;
            site_set(x, y, world_site(x, y, 0));
            break;
        }
    }
    debug("find_start %d %3d x: %3d y: %3d range: %d\n",
        faction_id, i, *tx, *ty, min_range(spawns, *tx, *ty));
    flushlog();
}

void set_project_owner(FacilityId item_id, int faction_id) {
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            SecretProjects[item_id - SP_ID_First] = i;
            break;
        }
    }
}

void __cdecl mod_time_warp() {
    const uint32_t BIT_SKIP_BONUS = (BIT_BASE_IN_TILE | BIT_VEH_IN_TILE
        | BIT_THERMAL_BORE | BIT_MONOLITH | BIT_MINE | BIT_SOLAR);
    const FacilityId projects[] = {
        FAC_CITIZENS_DEFENSE_FORCE,
        FAC_COMMAND_NEXUS,
        FAC_HUMAN_GENOME_PROJECT,
        FAC_MERCHANT_EXCHANGE,
        FAC_PLANETARY_TRANSIT_SYSTEM,
        FAC_VIRTUAL_WORLD,
        FAC_WEATHER_PARADIGM,
    };
    std::set<int32_t> selected = {};
    std::set<int32_t> choices = {};
    const int num = clamp(*MapAreaTiles / 3200 + 2, 2, 5);
    /*
    Aquatic factions always spawn with an extra Unity Gunship.
    Alien factions spawn with an extra Colony Pod and Battle Ogre Mk1.
    These units are added in setup_player regardless of other conditions.
    */
    if (conf.time_warp_mod) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            bool ocean = MFactions[i].is_aquatic();
            bool alien = MFactions[i].is_alien();
            *SkipTechScreenB = 1;
            for (int j = 0; j < conf.time_warp_techs; j++) {
                Factions[i].tech_research_id = mod_tech_ai(i);
                tech_advance(i);
            }
            *SkipTechScreenB = 0;
            Factions[i].tech_ranking = 2 * Factions[i].tech_achieved;
            Factions[i].tech_research_id = -1;
            Factions[i].energy_credits += num * 50;
            consider_designs(i);

            int base_id = find_hq(i);
            if (base_id >= 0) {
                BASE* base = &Bases[base_id];
                base->pop_size = 4;
                if (!has_fac_built(FAC_PRESSURE_DOME, base_id)) {
                    set_fac(FAC_RECYCLING_TANKS, base_id, 1);
                }
                set_fac(FAC_RECREATION_COMMONS, base_id, 1);
                set_fac(FAC_PERIMETER_DEFENSE, base_id, 1);
                set_fac(FAC_NETWORK_NODE, base_id, 1);

                for (int j = 0; j < num; j++) {
                    if (ocean) {
                        mod_veh_init(j&1 ? BSC_UNITY_GUNSHIP : BSC_TRANSPORT_FOIL, i, base->x, base->y);
                    } else {
                        mod_veh_init(j&1 ? BSC_UNITY_ROVER : BSC_SCOUT_PATROL, i, base->x, base->y);
                    }
                }
                for (int j = 0; j < num - ocean - alien; j++) {
                    mod_veh_init(ocean ? BSC_SEA_ESCAPE_POD : BSC_COLONY_POD, i, base->x, base->y);
                }
                for (int j = 0; j < num; j++) {
                    mod_veh_init(ocean ? BSC_SEA_FORMERS : BSC_FORMERS, i, base->x, base->y);
                }
                int fixed = 0;
                int added = 0;
                for (auto& m : iterate_tiles(base->x, base->y, 0, 81)) {
                    if ((base->x != m.x || base->y != m.y)
                    && (m.sq->owner < 0 || m.sq->owner == i)
                    && !(m.sq->landmarks & LM_VOLCANO)
                    && !(m.sq->items & BIT_SKIP_BONUS)) {
                        // Improve resources/remove fungus near start
                        int bonus = bonus_at(m.x, m.y);
                        if (!goody_at(m.x, m.y) && !m.sq->is_rocky() && added < 5 && !random(4)) {
                            if (!is_ocean(m.sq)) {
                                m.sq->items &= ~TerraformRules[FORMER_FOREST][1];
                                m.sq->items &= ~BIT_FUNGUS;
                                m.sq->items |= BIT_FOREST;
                                added++;
                            } else if (is_ocean_shelf(m.sq)) {
                                m.sq->items &= ~TerraformRules[FORMER_FARM][1];
                                m.sq->items &= ~BIT_FUNGUS;
                                m.sq->items |= BIT_FARM;
                                added++;
                            }
                        }
                        if (m.sq->items & BIT_FUNGUS && bonus != RES_NONE && fixed < 4) {
                            m.sq->items &= ~BIT_FUNGUS;
                            fixed++;
                        }
                        if (bonus == RES_NUTRIENT && m.sq->is_rocky()) {
                            rocky_set(m.x, m.y, LEVEL_ROLLING);
                        }
                    }
                    m.sq->visibility |= (1 << i);
                    synch_bit(m.x, m.y, i);
                }
                // Set initial production, turn advances once before player turn begins
                if (!base->nerve_staple_turns_left) {
                    ++base->nerve_staple_turns_left;
                }
                set_base(base_id);
                base_compute(1);
                base_change(base_id, select_build(base_id));
            }
        }
        *CurrentTurn = conf.time_warp_start_turn;
    } else {
        time_warp();
    }
    init_world_config();
    if (!conf.time_warp_projects) {
        return;
    }
    for (const FacilityId item_id : projects) {
        choices.clear();
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (selected.count(i)
            || (item_id == FAC_COMMAND_NEXUS && has_free_facility(FAC_COMMAND_CENTER, i))
            || (item_id == FAC_CITIZENS_DEFENSE_FORCE && has_free_facility(FAC_PERIMETER_DEFENSE, i))) {
                // skip faction
            } else {
                choices.insert(i);
            }
        }
        if (!choices.size()) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (!selected.count(i)) {
                    choices.insert(i);
                }
            }
        }
        if (choices.size()) {
            int32_t choice = pick_random(choices);
            set_project_owner(item_id, choice);
            selected.insert(choice);
            debug("time_warp %s %s\n", MFactions[choice].filename, Facility[item_id].name);
        } else {
            assert(0);
        }
    }
}

