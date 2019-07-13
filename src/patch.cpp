
#include "patch.h"
#include "game.h"
#include "move.h"

const char* ac_alpha = "ac_mod\\alphax";
const char* ac_help = "ac_mod\\helpx";
const char* ac_tutor = "ac_mod\\tutor";
const char* ac_concepts = "ac_mod\\conceptsx";
const char* ac_opening = "opening";
const char* ac_movlist = "movlist";
const char* ac_movlist_txt = "movlist.txt";
const char* ac_movlistx_txt = "movlistx.txt";

const char* lm_params[] = {
    "crater", "volcano", "jungle", "uranium",
    "sargasso", "ruins", "dunes", "fresh",
    "mesa", "canyon", "geothermal", "ridge",
    "borehole", "nexus", "unity", "fossil"
};

const byte asm_find_start[] = {
    0x8D,0x45,0xF8,0x50,0x8D,0x45,0xFC,0x50,0x8B,0x45,0x08,0x50,
    0xE8,0x00,0x00,0x00,0x00,0x83,0xC4,0x0C
};

int prev_rnd = -1;
Points spawns;
Points natives;
Points goodtiles;

bool FileExists(const char* path) {
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

HOOK_API int crop_yield(int fac, int base, int x, int y, int tf) {
    int value = tx_crop_yield(fac, base, x, y, tf);
    MAP* sq = mapsq(x, y);
    if (sq && sq->items & TERRA_THERMAL_BORE) {
        value++;
    }
    return value;
}

void process_map(int k) {
    TileSearch ts;
    Points visited;
    Points current;
    natives.clear();
    goodtiles.clear();
    /* Map area square root values: Standard = 56, Huge = 90 */
    int limit = *tx_map_area_sq_root * (*tx_map_area_sq_root < 70 ? 1 : 2);

    for (int y = 0; y < *tx_map_axis_y; y++) {
        for (int x = y&1; x < *tx_map_axis_x; x+=2) {
            MAP* sq = mapsq(x, y);
            if (unit_in_tile(sq) == 0)
                natives.insert({x, y});
            if (y < 4 || y >= *tx_map_axis_x - 4)
                continue;
            if (sq && !is_ocean(sq) && !visited.count({x, y})) {
                int n = 0;
                ts.init(x, y, TRIAD_LAND, k);
                while ((sq = ts.get_next()) != NULL) {
                    auto p = mp(ts.rx, ts.ry);
                    visited.insert(p);
                    if (~sq->items & TERRA_FUNGUS && !(sq->landmarks & ~LM_FRESH)) {
                        current.insert(p);
                    }
                    n++;
                }
                if (n >= limit) {
                    for (auto& p : current) {
                        goodtiles.insert(p);
                    }
                }
                current.clear();
            }
        }
    }
    if ((int)goodtiles.size() < *tx_map_area_sq_root * 8) {
        goodtiles.clear();
    }
    debug("process_map %d %d %d %d %d\n", *tx_map_axis_x, *tx_map_axis_y,
        *tx_map_area_sq_root, visited.size(), goodtiles.size());
}

bool valid_start (int fac, int iter, int x, int y, bool aquatic) {
    Points pods;
    MAP* sq = mapsq(x, y);
    if (!sq || sq->items & BASE_DISALLOWED || (sq->rocks & TILE_ROCKY && !is_ocean(sq)))
        return false;
    if (sq->landmarks & ~LM_FRESH)
        return false;
    if (aquatic != is_ocean(sq) || tx_bonus_at(x, y) != RES_NONE)
        return false;
    if (point_range(natives, x, y) < 4)
        return false;
    int sc = 0;
    int nut = 0;
    for (int i=0; i<44; i++) {
        int x2 = wrap(x + offset_tbl[i][0]);
        int y2 = y + offset_tbl[i][1];
        sq = mapsq(x2, y2);
        if (sq) {
            int bonus = tx_bonus_at(x2, y2);
            if (is_ocean(sq)) {
                if (!is_ocean_shelf(sq))
                    sc--;
                else if (aquatic)
                    sc += 4;
            } else {
                sc += (sq->level & (TILE_RAINY | TILE_MOIST) ? 3 : 1);
                if (sq->items & TERRA_RIVER)
                    sc += (i < 20 ? 4 : 2);
                if (sq->rocks & TILE_ROLLING)
                    sc += 1;
            }
            if (bonus > 0) {
                if (i < 20 && bonus == RES_NUTRIENT && !is_ocean(sq))
                    nut++;
                sc += (i < 20 ? 10 : 5);
            }
            if (tx_goody_at(x2, y2) > 0) {
                sc += (i < 20 ? 25 : 8);
                if (!is_ocean(sq) && (i < 20 || pods.size() < 2))
                    pods.insert({x2, y2});
            }
            if (sq->items & TERRA_FUNGUS) {
                sc -= (i < 20 ? 4 : 1);
            }
        }
    }
    int min_sc = 160 - iter/2;
    bool need_bonus = (!aquatic && conf.nutrient_bonus > is_human(fac));
    debug("find_score %2d | %3d %3d | %d %d %d %d\n", iter, x, y, pods.size(), nut, min_sc, sc);
    if (sc >= min_sc && need_bonus && (int)pods.size() > 1 - iter/50) {
        int n = 0;
        while (!aquatic && pods.size() > 0 && nut + n < 2) {
            Points::const_iterator it(pods.begin());
            std::advance(it, random(pods.size()));
            sq = mapsq(it->first, it->second);
            pods.erase(it);
            sq->items &= ~(TERRA_FUNGUS | TERRA_MINERAL_RES | TERRA_ENERGY_RES);
            sq->items |= (TERRA_SUPPLY_REMOVE | TERRA_BONUS_RES | TERRA_NUTRIENT_RES);
            n++;
        }
        return true;
    }
    return sc >= min_sc && (!need_bonus || nut > 1);
}

HOOK_API void find_start(int fac, int* tx, int* ty) {
    bool aquatic = tx_factions_meta[fac].rule_flags & FACT_AQUATIC;
    int k = (*tx_map_axis_y < 80 ? 8 : 16);
    if (*tx_random_seed != prev_rnd) {
        process_map(k/2);
        prev_rnd = *tx_random_seed;
    }
    spawns.clear();
    for (int i=0; i<*tx_total_num_vehicles; i++) {
        VEH* v = &tx_vehicles[i];
        if (v->faction_id != 0 && v->faction_id != fac) {
            spawns.insert({v->x, v->y});
        }
    }
    int z = 0;
    int x = 0;
    int y = 0;
    int i = 0;
    while (++i < 120) {
        if (!aquatic && goodtiles.size() > 0) {
            Points::const_iterator it(goodtiles.begin());
            std::advance(it, random(goodtiles.size()));
            x = it->first;
            y = it->second;
        } else {
            y = (random(*tx_map_axis_y - k) + k/2);
            x = (random(*tx_map_axis_x) &~1) + (y&1);
        }
        int min_range = max(8, *tx_map_area_sq_root/3 - i/5);
        z = point_range(spawns, x, y);
        debug("find_iter %d %d | %3d %3d | %2d %2d\n", fac, i, x, y, min_range, z);
        if (z >= min_range && valid_start(fac, i, x, y, aquatic)) {
            *tx = x;
            *ty = y;
            break;
        }
    }
    tx_site_set(*tx, *ty, tx_world_site(*tx, *ty, 0));
    debug("find_start %d %d %d %d %d\n", fac, i, *tx, *ty, point_range(spawns, *tx, *ty));
    fflush(debug_log);
}

void write_call_ptr(int addr, int func) {
    if (*(byte*)addr != 0xE8 || abs(*(int*)(addr+1)) > (int)AC_IMAGE_LEN) {
        throw std::exception();
    }
    *(int*)(addr+1) = func - addr - 5;
}

void write_offset(int addr, const void* ofs) {
    if (*(byte*)addr != 0x68 || *(int*)(addr+1) < (int)AC_IMAGE_BASE) {
        throw std::exception();
    }
    *(int*)(addr+1) = (int)ofs;
}

void remove_call(int addr) {
    if (*(byte*)addr != 0xE8) {
        throw std::exception();
    }
    memset((void*)addr, 0x90, 5);
}

bool patch_setup(Config* cf) {
    DWORD attrs;
    int lm = ~cf->landmarks;
    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READWRITE, &attrs))
        return false;

    write_call_ptr(0x52768A, (int)turn_upkeep);
    write_call_ptr(0x52A4AD, (int)turn_upkeep);
    write_call_ptr(0x528214, (int)faction_upkeep);
    write_call_ptr(0x52918F, (int)faction_upkeep);
    write_call_ptr(0x4E61D0, (int)base_production);
    write_call_ptr(0x4E888C, (int)crop_yield);
    write_call_ptr(0x5BDC4C, (int)tech_value);
    write_call_ptr(0x579362, (int)enemy_move);
    write_call_ptr(0x4AED04, (int)social_ai);
    write_call_ptr(0x527304, (int)social_ai);

    if (FileExists(ac_movlist_txt) && !FileExists(ac_movlistx_txt)) {
        CopyFile(ac_movlist_txt, ac_movlistx_txt, TRUE);
    }
    if (cf->smac_only) {
        *(int*)0x45F97A = 0;
        *(const char**)0x691AFC = ac_alpha;
        *(const char**)0x691B00 = ac_help;
        *(const char**)0x691B14 = ac_tutor;
        write_offset(0x42B30E, ac_concepts);
        write_offset(0x42B49E, ac_concepts);
        write_offset(0x42C450, ac_concepts);
        write_offset(0x42C7C2, ac_concepts);
        write_offset(0x403BA8, ac_movlist);
        write_offset(0x4BEF8D, ac_movlist);
        write_offset(0x52AB68, ac_opening);

        /* Enable custom faction selection during the game setup. */
        memset((void*)0x58A5E1, 0x90, 6);
        memset((void*)0x58B76F, 0x90, 2);
        memset((void*)0x58B9F3, 0x90, 2);
    }
    if (cf->faction_placement) {
        remove_call(0x5B1DFF);
        remove_call(0x5B2137);
        memset((void*)0x5B220F, 0x90, 63);
        memset((void*)0x5B2257, 0x90, 11);
        memcpy((void*)0x5B220F, asm_find_start, sizeof(asm_find_start));
        write_call_ptr(0x5B221B, (int)find_start);
    }
    if (lm & LM_JUNGLE) remove_call(0x5C88A0);
    if (lm & LM_CRATER) remove_call(0x5C88A9);
    if (lm & LM_VOLCANO) remove_call(0x5C88B5);
    if (lm & LM_MESA) remove_call(0x5C88BE);
    if (lm & LM_RIDGE) remove_call(0x5C88C7);
    if (lm & LM_URANIUM) remove_call(0x5C88D0);
    if (lm & LM_RUINS) remove_call(0x5C88D9);
    if (lm & LM_UNITY) remove_call(0x5C88EE);
    if (lm & LM_FOSSIL) remove_call(0x5C88F7);
    if (lm & LM_CANYON) remove_call(0x5C8903);
    if (lm & LM_NEXUS) remove_call(0x5C890F);
    if (lm & LM_BOREHOLE) remove_call(0x5C8918);
    if (lm & LM_SARGASSO) remove_call(0x5C8921);
    if (lm & LM_DUNES) remove_call(0x5C892A);
    if (lm & LM_FRESH) remove_call(0x5C8933);
    if (lm & LM_GEOTHERMAL) remove_call(0x5C893C);

    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READ, &attrs))
        return false;

    return true;
}

