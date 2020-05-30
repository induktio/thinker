
#include "patch.h"
#include "game.h"
#include "move.h"
#include "tech.h"
#include "engine.h"

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

int prev_rnd = -1;
Points natives;
Points goodtiles;

bool FileExists(const char* path) {
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

HOOK_API int mod_crop_yield(int faction, int base, int x, int y, int tf) {
    int value = crop_yield(faction, base, x, y, tf);
    MAP* sq = mapsq(x, y);
    if (sq && sq->items & TERRA_THERMAL_BORE) {
        value++;
    }
    return value;
}

HOOK_API int mod_base_draw(int ptr, int base_id, int x, int y, int zoom, int v1) {
    BASE* b = &tx_bases[base_id];
    base_draw(ptr, base_id, x, y, zoom, v1);

    if (zoom >= -8 && has_facility(base_id, FAC_HEADQUARTERS)) {
        // Game engine uses this way to determine the population label width
        const char* s1 = "8";
        const char* s2 = "88";
        int w = font_width(*(int*)0x93FC24, (int)(b->pop_size >= 10 ? s2 : s1)) + 5;
        int h = *(int*)((*(int*)0x93FC24)+16) + 4;

        for (int i=1; i<3; i++) {
            RECT rr = {x-i, y-i, x+w+i, y+h+i};
            buffer_box(ptr, (int)(&rr), 255, 255);
        }
    }
    return 0;
}

void check_relocate_hq(int faction) {
    if (find_hq(faction) < 0) {
        double best_score = INT_MIN;
        int best_id = -1;
        Points bases;
        for (int i=0; i<*total_num_bases; i++) {
            BASE* b = &tx_bases[i];
            if (b->faction_id == faction) {
                bases.insert({b->x, b->y});
            }
        }
        for (int i=0; i<*total_num_bases; i++) {
            BASE* b = &tx_bases[i];
            if (b->faction_id == faction) {
                double score = b->pop_size*0.2 - mean_range(bases, b->x, b->y);
                debug("relocate_hq %.4f %s\n", score, b->name);
                if (score > best_score) {
                    best_id = i;
                    best_score = score;
                }
            }
        }
        if (best_id >= 0) {
            BASE* b = &tx_bases[best_id];
            b->facilities_built[FAC_HEADQUARTERS/8] |= (1 << (FAC_HEADQUARTERS % 8));
            draw_tile(b->x, b->y, 0);
        }
    }
}

HOOK_API int mod_capture_base(int base_id, int faction, int probe) {
    int old_faction = tx_bases[base_id].faction_id;
    assert(faction > 0 && faction < 8 && faction != old_faction);
    capture_base(base_id, faction, probe);
    check_relocate_hq(old_faction);
    return 0;
}

HOOK_API int mod_base_kill(int base_id) {
    int old_faction = tx_bases[base_id].faction_id;
    assert(base_id >= 0 && base_id < *total_num_bases);
    base_kill(base_id);
    check_relocate_hq(old_faction);
    return 0;
}

void process_map(int k) {
    TileSearch ts;
    Points visited;
    Points current;
    natives.clear();
    goodtiles.clear();
    // Map area square root values: Standard = 56, Huge = 90
    int limit = *map_area_sq_root * (*map_area_sq_root < 70 ? 1 : 2);

    for (int y = 0; y < *map_axis_y; y++) {
        for (int x = y&1; x < *map_axis_x; x+=2) {
            MAP* sq = mapsq(x, y);
            if (unit_in_tile(sq) == 0)
                natives.insert({x, y});
            if (y < 4 || y >= *map_axis_x - 4)
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
    if ((int)goodtiles.size() < *map_area_sq_root * 8) {
        goodtiles.clear();
    }
    debug("process_map %d %d %d %d %d\n", *map_axis_x, *map_axis_y,
        *map_area_sq_root, visited.size(), goodtiles.size());
}

bool valid_start (int faction, int iter, int x, int y, bool aquatic) {
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
    bool need_bonus = (!aquatic && conf.nutrient_bonus > is_human(faction));
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

HOOK_API void find_start(int faction, int* tx, int* ty) {
    Points spawns;
    bool aquatic = tx_metafactions[faction].rule_flags & FACT_AQUATIC;
    int k = (*map_axis_y < 80 ? 8 : 16);
    if (*map_random_seed != prev_rnd) {
        process_map(k/2);
        prev_rnd = *map_random_seed;
    }
    for (int i=0; i<*total_num_vehicles; i++) {
        VEH* v = &tx_vehicles[i];
        if (v->faction_id != 0 && v->faction_id != faction) {
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
            y = (random(*map_axis_y - k) + k/2);
            x = (random(*map_axis_x) &~1) + (y&1);
        }
        int min_range = max(8, *map_area_sq_root/3 - i/5);
        z = point_range(spawns, x, y);
        debug("find_iter %d %d | %3d %3d | %2d %2d\n", faction, i, x, y, min_range, z);
        if (z >= min_range && valid_start(faction, i, x, y, aquatic)) {
            *tx = x;
            *ty = y;
            break;
        }
    }
    tx_site_set(*tx, *ty, tx_world_site(*tx, *ty, 0));
    debug("find_start %d %d %d %d %d\n", faction, i, *tx, *ty, point_range(spawns, *tx, *ty));
    fflush(debug_log);
}

void exit_fail() {
    MessageBoxA(0, "Error while patching game binary. Game will now exit.",
        MOD_VERSION, MB_OK | MB_ICONSTOP);
    exit(EXIT_FAILURE);
}

/*
Replaces old byte sequence with a new byte sequence at address.
Verifies that address contains old values before replacing.
If new_bytes is a null pointer, replace old_bytes with NOP code instead.
*/
void write_bytes(int addr, const byte* old_bytes, const byte* new_bytes, int len) {
    if ((int*)addr < (int*)AC_IMAGE_BASE || (int*)addr + len > (int*)AC_IMAGE_BASE + AC_IMAGE_LEN) {
        exit_fail();
    }
    for (int i=0; i<len; i++) {
        if (*((byte*)addr + i) != old_bytes[i]) {
            debug("write_bytes: old byte doesn't match at address: %08x, value: %02x, expected: %02x",
                addr + i, *((byte*)addr + i), old_bytes[i]);
            exit_fail();
        }
        if (new_bytes != NULL) {
            *((byte*)addr + i) = new_bytes[i];
        } else {
            *((byte*)addr + i) = 0x90;
        }
    }
}

/*
Verify that the location contains standard function prologue before patching.

55          push    ebp
8B EC       mov     ebp, esp
*/
void write_jump(int addr, int func) {
    if ((*(int*)addr & 0xFFFFFF) != 0xEC8B55) {
        exit_fail();
    }
    *(byte*)addr = 0xE9;
    *(int*)(addr + 1) = func - addr - 5;
}

void write_call(int addr, int func) {
    if (*(byte*)addr != 0xE8 || abs(*(int*)(addr+1)) > (int)AC_IMAGE_LEN) {
        exit_fail();
    }
    *(int*)(addr+1) = func - addr - 5;
}

void write_offset(int addr, const void* ofs) {
    if (*(byte*)addr != 0x68 || *(int*)(addr+1) < (int)AC_IMAGE_BASE) {
        exit_fail();
    }
    *(int*)(addr+1) = (int)ofs;
}

void remove_call(int addr) {
    if (*(byte*)addr != 0xE8 || *(int*)(addr+1) + addr < (int)AC_IMAGE_BASE) {
        exit_fail();
    }
    memset((void*)addr, 0x90, 5);
}

bool patch_setup(Config* cf) {
    DWORD attrs;
    int lm = ~cf->landmarks;
    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READWRITE, &attrs))
        return false;

    write_jump(0x527290, (int)faction_upkeep);
    write_call(0x52768A, (int)turn_upkeep);
    write_call(0x52A4AD, (int)turn_upkeep);
    write_call(0x4E61D0, (int)base_production);
    write_call(0x5BDC4C, (int)tech_value);
    write_call(0x579362, (int)enemy_move);
    write_call(0x5C0908, (int)log_veh_kill);
    write_call(0x4E888C, (int)mod_crop_yield);
    write_call(0x4672A7, (int)mod_base_draw);
    write_call(0x40F45A, (int)mod_base_draw);

    /*
    Fixes issue where attacking other satellites doesn't work in
    Orbital Attack View when smac_only is activated.
    */
    if (*(int*)0x4AB327 == 0x1D2) {
        *(byte*)0x4AB327 = 0x75;
    }
    /*
    Make sure the game engine can read movie settings from "movlistx.txt".
    */
    if (FileExists(ac_movlist_txt) && !FileExists(ac_movlistx_txt)) {
        CopyFile(ac_movlist_txt, ac_movlistx_txt, TRUE);
    }

    if (cf->smac_only) {
        if (!FileExists("ac_mod/alphax.txt") || !FileExists("ac_mod/conceptsx.txt")
        || !FileExists("ac_mod/tutor.txt") || !FileExists("ac_mod/helpx.txt")) {
            MessageBoxA(0, "Error while opening ac_mod folder. Unable to start smac_only mode.",
                MOD_VERSION, MB_OK | MB_ICONSTOP);
            exit(EXIT_FAILURE);
        }
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
        const byte asm_find_start[] = {
            0x8D,0x45,0xF8,0x50,0x8D,0x45,0xFC,0x50,0x8B,0x45,
            0x08,0x50,0xE8,0x00,0x00,0x00,0x00,0x83,0xC4,0x0C
        };
        remove_call(0x5B1DFF);
        remove_call(0x5B2137);
        memset((void*)0x5B220F, 0x90, 63);
        memset((void*)0x5B2257, 0x90, 11);
        memcpy((void*)0x5B220F, asm_find_start, sizeof(asm_find_start));
        write_call(0x5B221B, (int)find_start);
    }
    if (cf->revised_tech_cost) {
        write_call(0x4452D5, (int)tech_rate);
        write_call(0x498D26, (int)tech_rate);
        write_call(0x4A77DA, (int)tech_rate);
        write_call(0x521872, (int)tech_rate);
        write_call(0x5218BE, (int)tech_rate);
        write_call(0x5581E9, (int)tech_rate);
        write_call(0x5BEA4D, (int)tech_rate);
        write_call(0x5BEAC7, (int)tech_rate);

        write_call(0x52935F, (int)tech_selection);
        write_call(0x5BE5E1, (int)tech_selection);
        write_call(0x5BE690, (int)tech_selection);
        write_call(0x5BEB5D, (int)tech_selection);
    }
    if (cf->auto_relocate_hq) {
        write_call(0x4CCF13, (int)mod_capture_base);
        write_call(0x598778, (int)mod_capture_base);
        write_call(0x5A4AB0, (int)mod_capture_base);
        write_call(0x4CD629, (int)mod_base_kill); // action_oblit
        write_call(0x4F1466, (int)mod_base_kill); // base_production
        write_call(0x4EF319, (int)mod_base_kill); // base_growth
        write_call(0x500FD7, (int)mod_base_kill); // planet_busting
        write_call(0x50ADA8, (int)mod_base_kill); // battle_fight_2
        write_call(0x50AE77, (int)mod_base_kill); // battle_fight_2
        write_call(0x520E1A, (int)mod_base_kill); // random_events
        write_call(0x521121, (int)mod_base_kill); // random_events
        write_call(0x5915A6, (int)mod_base_kill); // alt_set
        write_call(0x598673, (int)mod_base_kill); // order_veh
    }
    if (cf->eco_damage_fix) {
        const byte old_bytes[] = {0x84,0x05,0xE8,0x64,0x9A,0x00,0x74,0x24};
        const byte new_bytes[] = {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        write_bytes(0x4EA0B9, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->collateral_damage_value != 3) {
        const byte old_bytes[] = {0xB2, 0x03};
        const byte new_bytes[] = {0xB2, (byte)cf->collateral_damage_value};
        write_bytes(0x50AAA5, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->disable_aquatic_bonus_minerals) {
        const byte old_bytes[] = {0x46};
        const byte new_bytes[] = {0x90};
        write_bytes(0x4E7604, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->disable_planetpearls) {
        const byte old_bytes[] = {
            0x8B,0x45,0x08,0x6A,0x00,0x6A,0x01,0x50,0xE8,0x46,
            0xAD,0x0B,0x00,0x83,0xC4,0x0C,0x40,0x8D,0x0C,0x80,
            0x8B,0x06,0xD1,0xE1,0x03,0xC1,0x89,0x06
        };
        write_bytes(0x5060ED, old_bytes, NULL, sizeof(old_bytes));
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

