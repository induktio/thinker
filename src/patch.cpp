
#include "patch.h"
#include "game.h"
#include "move.h"

const char* ac_alpha = "ac_mod\\alphax";
const char* ac_help = "ac_mod\\helpx";
const char* ac_tutor = "ac_mod\\tutor";
const char* ac_labels = "ac_mod\\labels";
const char* ac_concepts = "ac_mod\\conceptsx";
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
std::set<std::pair<int,int>> spawns;
std::set<std::pair<int,int>> goodtiles;

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

int spawn_range(int x, int y) {
    int z = MAPSZ;
    for (auto p : spawns) {
        z = min(z, map_range(x, y, p.first, p.second));
    }
    return z;
}

void process_map(int k) {
    TileSearch ts;
    std::set<std::pair<int,int>> visited;
    std::set<std::pair<int,int>> current;
    goodtiles.clear();
    int limit = *tx_map_area_sq_root * 2;

    for (int y = 4; y < *tx_map_axis_y - 4; y++) {
        for (int x = y&1; x < *tx_map_axis_x; x+=2) {
            MAP* sq = mapsq(x, y);
            if (sq && !is_ocean(sq) && !visited.count(mp(x, y))) {
                ts.init(x, y, LAND_ONLY, k);
                while ((sq = ts.get_next()) != NULL) {
                    auto xy = mp(ts.rx, ts.ry);
                    visited.insert(xy);
                    current.insert(xy);
                }
                if ((int)current.size() >= limit) {
                    for (auto t : current) {
                        goodtiles.insert(t);
                    }
                }
                current.clear();
            }
        }
    }
    if ((int)goodtiles.size() < *tx_map_area_sq_root * 8) {
        goodtiles.clear();
    }
    debuglog("process_map %d %d %d %d %d\n", *tx_map_axis_x, *tx_map_axis_y,
        *tx_map_area_sq_root, visited.size(), goodtiles.size());
}

bool valid_start (int x, int y, int iter, bool aquatic) {
    const int range = 4;
    MAP* sq = mapsq(x, y);
    if (!sq || sq->items & BASE_DISALLOWED || sq->rocks & TILE_ROCKY || sq->landmarks)
        return false;
    if (aquatic != is_ocean(sq) || tx_bonus_at(x, y) != RES_NONE)
        return false;
    int sc = 0;
    for (int i=-range*2; i<=range*2; i++) {
        for (int j=-range*2 + abs(i); j<=range*2 - abs(i); j+=2) {
            int x2 = wrap(x + i);
            int y2 = y + j;
            sq = mapsq(x2, y2);
            if (sq) {
                if (!is_ocean(sq)) {
                    sc += (sq->level & (TILE_RAINY | TILE_MOIST) ? 2 : 1);
                    if (sq->items & TERRA_RIVER)
                        sc += 1;
                    if (sq->rocks & TILE_ROLLING)
                        sc += 1;
                }
                if (tx_bonus_at(x2, y2))
                    sc += 5;
                if (sq->items & TERRA_FUNGUS)
                    sc -= 1;
                if (unit_in_tile(sq) == 0)
                    return false;
            }
        }
    }
    int min_sc = (aquatic ? 15 : 120 - iter/3);
    debuglog("score %3d %3d %d %d %d\n", x, y, iter, min_sc, sc);
    return sc >= min_sc;
}

HOOK_API void find_start(int fac, int* tx, int* ty) {
    int k = (*tx_map_axis_y < 80 ? 8 : 16);
    bool aquatic = tx_factions_meta[fac].rule_flags & FACT_AQUATIC;
    bool use_set = !aquatic && goodtiles.size();
    if (*tx_random_seed != prev_rnd) {
        process_map(k/2);
        prev_rnd = *tx_random_seed;
    }
    spawns.clear();
    for (int i=0; i<*tx_total_num_vehicles; i++) {
        VEH* v = &tx_vehicles[i];
        if (v->faction_id != 0 && v->faction_id != fac) {
            spawns.insert(mp(v->x, v->y));
        }
    }
    int z = 0;
    int x = 0;
    int y = 0;
    int i = 0;
    while (++i < 120) {
        if (use_set) {
            std::set<std::pair<int,int>>::const_iterator it(goodtiles.begin());
            std::advance(it, random(goodtiles.size()));
            x = it->first;
            y = it->second;
        } else {
            y = (random(*tx_map_axis_y - k) + k/2);
            x = (random(*tx_map_axis_x) &~1) + (y&1);
        }
        z = spawn_range(x, y);
        if (z >= 8 && valid_start(x, y, i, aquatic)) {
            *tx = x;
            *ty = y;
            if (z >= max(10, *tx_map_area_sq_root/3 - i/4)) {
                break;
            }
        }
    }
    tx_site_set(*tx, *ty, tx_world_site(*tx, *ty, 0));
    debuglog("find_start %d %d %d %d %d\n", fac, i, spawn_range(*tx, *ty), *tx, *ty);
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
        write_offset(0x616A20, ac_labels);
        write_offset(0x42B30E, ac_concepts);
        write_offset(0x42B49E, ac_concepts);
        write_offset(0x42C450, ac_concepts);
        write_offset(0x42C7C2, ac_concepts);
        write_offset(0x403BA8, ac_movlist);
        write_offset(0x4BEF8D, ac_movlist);

        // Enable custom faction selection
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
    if (lm & LM_FRESHWATER) remove_call(0x5C8933);
    if (lm & LM_GEOTHERMAL) remove_call(0x5C893C);

    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READ, &attrs))
        return false;

    return true;
}

