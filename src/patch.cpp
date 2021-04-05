
#include "patch.h"

const char* ac_alpha = "ac_mod\\alphax";
const char* ac_help = "ac_mod\\helpx";
const char* ac_tutor = "ac_mod\\tutor";
const char* ac_concepts = "ac_mod\\conceptsx";
const char* ac_opening = "opening";
const char* ac_movlist = "movlist";
const char* ac_movlist_txt = "movlist.txt";
const char* ac_movlistx_txt = "movlistx.txt";

const char* landmark_params[] = {
    "crater", "volcano", "jungle", "uranium",
    "sargasso", "ruins", "dunes", "fresh",
    "mesa", "canyon", "geothermal", "ridge",
    "borehole", "nexus", "unity", "fossil"
};

typedef int(__cdecl *Fexcept_handler3)(EXCEPTION_RECORD*, PVOID, CONTEXT*);
Fexcept_handler3 _except_handler3 = (Fexcept_handler3)0x646DF8;

int* top_menu_handle = (int*)0x945824;
int* peek_msg_status = (int*)0x9B7B9C;

UNIT fission_reactor[MaxProtoNum];
Points natives;
Points goodtiles;
bool has_supply_pods = true;

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
    int color = -1;
    int width = 1;
    BASE* b = &Bases[base_id];
    base_draw(ptr, base_id, x, y, zoom, v1);

    if (conf.world_map_labels > 0 && zoom >= -8) {
        if (has_facility(base_id, FAC_HEADQUARTERS)) {
            color = 255;
            width = 2;
        }
        if (b->status_flags & BASE_GOLDEN_AGE_ACTIVE) {
            color = 251;
        }
        if (color < 0) {
            return 0;
        }
        // Game engine uses this way to determine the population label width
        const char* s1 = "8";
        const char* s2 = "88";
        int w = font_width(*(int*)0x93FC24, (int)(b->pop_size >= 10 ? s2 : s1)) + 5;
        int h = *(int*)((*(int*)0x93FC24)+16) + 4;

        for (int i=1; i <= width; i++) {
            RECT rr = {x-i, y-i, x+w+i, y+h+i};
            buffer_box(ptr, (int)(&rr), color, color);
        }
    }
    return 0;
}

void check_relocate_hq(int faction) {
    if (find_hq(faction) < 0) {
        int best_score = INT_MIN;
        int best_id = -1;
        Points bases;
        for (int i=0; i<*total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction) {
                bases.insert({b->x, b->y});
            }
        }
        for (int i=0; i<*total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction) {
                int score = b->pop_size - (int)(10 * avg_range(bases, b->x, b->y))
                    + (has_facility(i, FAC_PERIMETER_DEFENSE) ? 4 : 0);
                debug("relocate_hq %4d %s\n", score, b->name);
                if (score > best_score) {
                    best_id = i;
                    best_score = score;
                }
            }
        }
        if (best_id >= 0) {
            BASE* b = &Bases[best_id];
            set_base_facility(best_id, FAC_HEADQUARTERS, true);
            draw_tile(b->x, b->y, 0);
        }
    }
}

HOOK_API int mod_capture_base(int base_id, int faction, int probe) {
    int old_faction = Bases[base_id].faction_id;
    assert(valid_player(faction) && faction != old_faction);
    capture_base(base_id, faction, probe);
    check_relocate_hq(old_faction);
    return 0;
}

HOOK_API int mod_base_kill(int base_id) {
    int old_faction = Bases[base_id].faction_id;
    assert(base_id >= 0 && base_id < *total_num_bases);
    base_kill(base_id);
    check_relocate_hq(old_faction);
    return 0;
}

HOOK_API int content_pop() {
    int faction = (*current_base_ptr)->faction_id;
    assert(valid_player(faction));
    if (is_human(faction)) {
        return conf.content_pop_player[*diff_level];
    }
    return conf.content_pop_computer[*diff_level];
}

HOOK_API int mod_setup_player(int faction, int v1, int v2) {
    setup_player(faction, v1, v2);
    if (faction > 0 && (!is_human(faction) || conf.player_free_units > 0)) {
        for (int i=0; i < *total_num_vehicles; i++) {
            VEH* veh = &Vehicles[i];
            if (veh->faction_id == faction) {
                MAP* sq = mapsq(veh->x, veh->y);
                int former = (is_ocean(sq) ? BSC_SEA_FORMERS : BSC_FORMERS);
                int colony = (is_ocean(sq) ? BSC_SEA_ESCAPE_POD : BSC_COLONY_POD);
                for (int j=0; j < conf.free_formers; j++) {
                    spawn_veh(former, faction, veh->x, veh->y, -1);
                }
                for (int j=0; j < conf.free_colony_pods; j++) {
                    spawn_veh(colony, faction, veh->x, veh->y, -1);
                }
                break;
            }
        }
        Factions[faction].satellites_nutrient = conf.satellites_nutrient;
        Factions[faction].satellites_mineral = conf.satellites_mineral;
        Factions[faction].satellites_energy = conf.satellites_energy;
    }
    return 0;
}

HOOK_API int skip_action_destroy(int id) {
    veh_skip(id);
    *veh_attack_flags = 0;
    return 0;
}

HOOK_API int render_ocean_fungus(int x, int y) {
    MAP* sq;
    int k = 0;
    for (int i=0; i<8; i++) {
        int z = (i - 1) & 7;
        sq = mapsq(wrap(x + NearbyTiles[z][0]), y + NearbyTiles[z][1]);
        if (sq && sq->items & TERRA_FUNGUS && is_ocean_shelf(sq)) {
            k |= (1 << i);
        }
    }
    return k;
}

void process_map(int k) {
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
    Map area square root values: Standard = 56, Huge = 90
    */
    int limit = *map_area_sq_root * (*map_area_sq_root < 70 ? 1 : 2);
    /*
    Some user created maps might have only manually assigned supply pods.
    We need to account for the fact that the pods might not be where they're
    supposed to be as implied by the default placement algorithm.
    If there are insufficient pods present, has_supply_pods is set to false.
    */
    int pods_removed = 0;

    for (int y = 0; y < *map_axis_y; y++) {
        for (int x = y&1; x < *map_axis_x; x+=2) {
            MAP* sq = mapsq(x, y);
            if (sq) {
                if (unit_in_tile(sq) == 0) {
                    natives.insert({x, y});
                }
                assert(sq->region >= 0 && sq->region < MaxRegionNum);
                region_count[sq->region]++;
                if (!is_ocean(sq) && sq->items & TERRA_SUPPLY_REMOVE) {
                    pods_removed++;
                }
            }
        }
    }
    for (int y = 0; y < *map_axis_y; y++) {
        for (int x = y&1; x < *map_axis_x; x+=2) {
            if (y < k || y >= *map_axis_y - k) {
                continue;
            }
            MAP* sq = mapsq(x, y);
            if (sq && !is_ocean(sq) && ~sq->items & TERRA_FUNGUS && !(sq->landmarks & ~LM_FRESH)
            && region_count[sq->region] >= limit) {
                goodtiles.insert({x, y});
            }
        }
    }
    if ((int)goodtiles.size() < *map_area_sq_root * 8) {
        goodtiles.clear();
    }
    has_supply_pods = (*map_area_tiles > pods_removed * 20);
    debug("process_map x: %d y: %d sqrt: %d goodtiles: %d removed: %d has_pods: %d\n",
        *map_axis_x, *map_axis_y, *map_area_sq_root, goodtiles.size(), pods_removed, has_supply_pods);
}

bool valid_start (int faction, int iter, int x, int y, bool aquatic) {
    Points pods;
    MAP* sq = mapsq(x, y);
    if (!sq || sq->items & BASE_DISALLOWED || (sq->is_rocky() && !is_ocean(sq))) {
        return false;
    }
    if (sq->landmarks & ~LM_FRESH) {
        return false;
    }
    if (aquatic != is_ocean(sq) || bonus_at(x, y) != RES_NONE) {
        return false;
    }
    if (min_range(natives, x, y) < max(4, 8 - iter/16)) {
        return false;
    }
    int sc = 0;
    int nut = 0;
    for (int i=1; i<45; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        sq = mapsq(x2, y2);
        if (sq) {
            int bonus = bonus_at(x2, y2);
            if (is_ocean(sq)) {
                if (!is_ocean_shelf(sq)) {
                    sc--;
                } else if (aquatic) {
                    sc += 4;
                }
            } else {
                sc += (sq->is_rainy_or_moist() ? 3 : 1);
                if (sq->items & TERRA_RIVER) {
                    sc += (i < 20 ? 4 : 2);
                }
                if (sq->is_rolling()) {
                    sc += 1;
                }
            }
            if (bonus > 0) {
                if (i < 20 && bonus == RES_NUTRIENT && !is_ocean(sq)) {
                    nut++;
                }
                sc += (i < 20 ? 10 : 5);
            }
            if (goody_at(x2, y2) > 0 || bonus == RES_MINERAL || bonus == RES_ENERGY) {
                sc += (i < 20 ? 25 : 8);
                if (!is_ocean(sq) && (i < 20 || pods.size() < 2)) {
                    pods.insert({x2, y2});
                }
            }
            if (sq->items & TERRA_FUNGUS) {
                sc -= (i < 20 ? 4 : 1);
            }
        }
    }
    int min_sc = (has_supply_pods ? 160 : 100) - iter/2;
    bool need_bonus = (has_supply_pods && !aquatic && conf.nutrient_bonus > is_human(faction));
    debug("find_score %d %d x: %3d y: %3d pods: %d nuts: %d min: %d score: %d\n",
        faction, iter, x, y, pods.size(), nut, min_sc, sc);
    if (sc >= min_sc && need_bonus && (int)pods.size() > 1 - iter/50) {
        int n = 0;
        while (!aquatic && pods.size() > 0 && nut + n < 2) {
            Points::const_iterator it(pods.begin());
            std::advance(it, random(pods.size()));
            sq = mapsq(it->x, it->y);
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
    bool aquatic = MFactions[faction].rule_flags & FACT_AQUATIC;
    int k = (*map_axis_y < 80 ? 4 : 8);
    process_map(k/2);
    for (int i=0; i<*total_num_vehicles; i++) {
        VEH* v = &Vehicles[i];
        if (v->faction_id != 0 && v->faction_id != faction) {
            spawns.insert({v->x, v->y});
        }
    }
    int z = 0;
    int x = 0;
    int y = 0;
    int i = 0;
    while (++i < 150) {
        if (!aquatic && goodtiles.size() > 0 && i < 120) {
            Points::const_iterator it(goodtiles.begin());
            std::advance(it, random(goodtiles.size()));
            x = it->x;
            y = it->y;
        } else {
            y = (random(*map_axis_y - k*2) + k);
            x = (random(*map_axis_x) &~1) + (y&1);
        }
        int limit = max(8, *map_area_sq_root/3 - i/6);
        z = min_range(spawns, x, y);
        debug("find_iter  %d %d x: %3d y: %3d limit: %2d range: %2d\n", faction, i, x, y, limit, z);
        if (z >= limit && valid_start(faction, i, x, y, aquatic)) {
            *tx = x;
            *ty = y;
            break;
        }
    }
    site_set(*tx, *ty, world_site(*tx, *ty, 0));
    debug("find_start %d %d x: %3d y: %3d range: %d\n", faction, i, *tx, *ty, min_range(spawns, *tx, *ty));
    fflush(debug_log);
}

/*
Read more about the idea behind idle loop patch: https://github.com/vinceho/smac-cpu-fix/
*/
BOOL WINAPI ModPeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
    static bool wait_next = false;
    int wait_time = (wait_next && (*top_menu_handle != 0 || *peek_msg_status == 0) ? 8 : 0);
    int wait_result = MsgWaitForMultipleObjectsEx(0, 0, wait_time, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

    if (wait_result == WAIT_TIMEOUT) {
        wait_next = true;
    } else {
        wait_next = false;
    }
    return PeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

DWORD WINAPI ModGetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName,
LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
    debug("GET %s %s %s\n", lpAppName, lpKeyName, lpDefault);
    if (!strcmp(lpAppName, "Alpha Centauri")) {
        if (conf.directdraw >= 0 && !strcmp(lpKeyName, "DirectDraw")) {
            strncpy(lpReturnedString, (conf.directdraw ? "1" : "0"), 2);
            return 1;
        }
        if (conf.disable_opening_movie >= 0 && !strcmp(lpKeyName, "DisableOpeningMovie")) {
            strncpy(lpReturnedString, (conf.disable_opening_movie ? "1" : "0"), 2);
            return 1;
        }
    }
    return GetPrivateProfileStringA(lpAppName, lpKeyName,
        lpDefault, lpReturnedString, nSize, lpFileName);
}

void exit_fail() {
    MessageBoxA(0, "Error while patching game binary. Game will now exit.",
        MOD_VERSION, MB_OK | MB_ICONSTOP);
    exit(EXIT_FAILURE);
}

/*
Replace the default C++ exception handler in SMACX binary with a custom version.
This allows us to get more information about crash locations.
https://en.wikipedia.org/wiki/Microsoft-specific_exception_handling_mechanisms
*/
int __cdecl mod_except_handler3(EXCEPTION_RECORD *rec, PVOID *frame, CONTEXT *ctx)
{
    if (!debug_log && !(debug_log = fopen("debug.txt", "w"))) {
        return _except_handler3(rec, frame, ctx);
    }
    time_t rawtime = time(&rawtime);
    struct tm *now = localtime(&rawtime);
    char datetime[70];
    char filepath[1024];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", now);
    HANDLE hProcess = GetCurrentProcess();
    int ret = GetMappedFileNameA(hProcess, (LPVOID)ctx->Eip, filepath, sizeof(filepath));

    fprintf(debug_log,
        "**************************************************\n"
        "ModVersion %s (%s)\n"
        "CrashTime  %s\n"
        "SavedGame  %s\n"
        "ModuleName %s\n"
        "**************************************************\n"
        "ExceptionCode    %08x\n"
        "ExceptionFlags   %08x\n"
        "ExceptionRecord  %08x\n"
        "ExceptionAddress %08x\n",
        MOD_VERSION,
        MOD_DATE,
        datetime,
        last_save_path,
        (ret != 0 ? filepath : ""),
        (int)rec->ExceptionCode,
        (int)rec->ExceptionFlags,
        (int)rec->ExceptionRecord,
        (int)rec->ExceptionAddress);

     fprintf(debug_log,
        "CFlags %08lx\n"
        "EFlags %08lx\n"
        "EAX %08lx\n"
        "EBX %08lx\n"
        "ECX %08lx\n"
        "EDX %08lx\n"
        "ESI %08lx\n"
        "EDI %08lx\n"
        "EBP %08lx\n"
        "ESP %08lx\n"
        "EIP %08lx\n",
        ctx->ContextFlags, ctx->EFlags,
        ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx,
        ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp, ctx->Eip);

    int32_t* p = (int32_t*)&conf;
    for (int32_t i = 0; i < (int32_t)sizeof(conf)/4; i++) {
        fprintf(debug_log, "Config %d: %d\n", i, p[i]);
    }

    fflush(debug_log);
    return _except_handler3(rec, frame, ctx);
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
Verify that the location contains a standard function prologue (55 8B EC)
or a near call with absolute indirect address (FF 25) before patching.
*/
void write_jump(int addr, int func) {
    if ((*(int*)addr & 0xFFFFFF) != 0xEC8B55 && *(int16_t*)addr != 0x25FF) {
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
    DWORD oldattrs;
    int lm = ~cf->landmarks;
    bool pracx = strcmp((const char*)0x668165, "prax") == 0;

    if (conf.smooth_scrolling && pracx) {
        MessageBoxA(0, "Smooth scrolling feature will be disabled while PRACX is also running.",
            MOD_VERSION, MB_OK | MB_ICONWARNING);
        conf.smooth_scrolling = 0;
    }
    if (!VirtualProtect(AC_IMPORT_BASE, AC_IMPORT_LEN, PAGE_EXECUTE_READWRITE, &oldattrs)) {
        return false;
    }
    *(int*)GetPrivateProfileStringAImport = (int)ModGetPrivateProfileStringA;
    if (!pracx) {
        *(int*)RegisterClassImport = (int)ModRegisterClassA;
    }
    if (cf->windowed && !pracx) {
        *(int*)GetSystemMetricsImport = (int)ModGetSystemMetrics;
    }
    if (cf->cpu_idle_fix) {
        *(int*)PeekMessageImport = (int)ModPeekMessage;
    }
    if (!VirtualProtect(AC_IMPORT_BASE, AC_IMPORT_LEN, oldattrs, &attrs)) {
        return false;
    }

    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READWRITE, &attrs)) {
        return false;
    }
    extra_setup(cf);

    write_jump(0x527290, (int)mod_faction_upkeep);
    write_jump(0x579D80, (int)wipe_goals);
    write_jump(0x579A30, (int)add_goal);
    write_jump(0x4688E0, (int)MapWin_gen_overlays);
    write_call(0x52768A, (int)mod_turn_upkeep);
    write_call(0x52A4AD, (int)mod_turn_upkeep);
    write_call(0x4E61D0, (int)mod_base_production);
    write_call(0x5BDC4C, (int)mod_tech_value);
    write_call(0x579362, (int)mod_enemy_move);
    write_call(0x4E888C, (int)mod_crop_yield);
    write_call(0x4672A7, (int)mod_base_draw);
    write_call(0x40F45A, (int)mod_base_draw);
    write_call(0x525CC7, (int)mod_setup_player);
    write_call(0x5A3C9B, (int)mod_setup_player);
    write_call(0x5B341C, (int)mod_setup_player);
    write_call(0x5B3C03, (int)mod_setup_player);
    write_call(0x5B3C4C, (int)mod_setup_player);
    write_call(0x5C0908, (int)log_veh_kill);
    write_offset(0x6456EE, (void*)mod_except_handler3);
    write_offset(0x64576E, (void*)mod_except_handler3);
    write_offset(0x6457CC, (void*)mod_except_handler3);
    write_offset(0x645F98, (void*)mod_except_handler3);
    write_offset(0x646CA7, (void*)mod_except_handler3);
    write_offset(0x646FDE, (void*)mod_except_handler3);
    write_offset(0x648C54, (void*)mod_except_handler3);
    write_offset(0x648D83, (void*)mod_except_handler3);
    write_offset(0x648EC8, (void*)mod_except_handler3);
    write_offset(0x64908C, (void*)mod_except_handler3);
    write_offset(0x6492D4, (void*)mod_except_handler3);
    write_offset(0x649335, (void*)mod_except_handler3);
    write_offset(0x64A3C0, (void*)mod_except_handler3);
    write_offset(0x64D947, (void*)mod_except_handler3);

    if (cf->smooth_scrolling) {
        write_offset(0x50F3DC, (void*)blink_timer);
        write_call(0x46AB81, (int)draw_map);
        write_call(0x46ABD1, (int)draw_map);
        write_call(0x46AC56, (int)draw_map);
        write_call(0x46ACC5, (int)draw_map);
        write_call(0x46A5A9, (int)zoom_process);
        write_call(0x48B6C2, (int)zoom_process);
        write_call(0x48B893, (int)zoom_process);
        write_call(0x48B91F, (int)zoom_process);
        write_call(0x48BA15, (int)zoom_process);
    }

    /*
    Hide unnecessary region_base_plan display next to base names in debug mode.
    */
    remove_call(0x468175);
    remove_call(0x468186);

    /*
    Fix a bug that occurs after the player does an artillery attack on unoccupied
    tile and then selects another unit to view combat odds and cancels the attack.
    After this veh_attack_flags will not get properly cleared and the next bombardment
    on an unoccupied tile always results in the destruction of the bombarding unit.
    */
    write_call(0x4CAA7C, (int)skip_action_destroy);

    /*
    Fix engine rendering issue where ocean fungus tiles displayed
    inconsistent graphics compared to the adjacent land fungus tiles.
    */
    {
        const byte old_bytes[] = {
            0x8B,0x1D,0x8C,0x98,0x94,0x00,0x83,0xE3,0x01,0x8B,0x75,0x20,
            0x8D,0x79,0xFF,0x83,0xE7,0x07,0xC1,0xE7,0x02,0x8B,0x87,0x50,
        };
        const byte new_bytes[] = {
            0x8B,0x45,0x24,0x50,0x8B,0x45,0x20,0x50,0xE8,0x00,0x00,0x00,
            0x00,0x83,0xC4,0x08,0x89,0x45,0xC4,0xE9,0x85,0x07,0x00,0x00,
        };
        write_bytes(0x463651, old_bytes, new_bytes, sizeof(new_bytes));
        write_call(0x463659, (int)render_ocean_fungus);
    }

    /*
    Fixes issue where attacking other satellites doesn't work in
    Orbital Attack View when smac_only is activated.
    */
    {
        const byte old_bytes[] = {0xD2,0x01,0x00,0x00};
        const byte new_bytes[] = {0x75,0x01,0x00,0x00};
        write_bytes(0x4AB327, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Patch AI vehicle home base reassignment bug.
    This change inverts the old condition where an apparent oversight made
    the engine to reassign vehicles to bases with mineral_surplus < 2.
    */
    {
        const byte old_bytes[] = {0x0F,0x8D,0x83,0x00,0x00,0x00};
        const byte new_bytes[] = {0x0F,0x8C,0x83,0x00,0x00,0x00};
        write_bytes(0x562094, old_bytes, new_bytes, sizeof(new_bytes));
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
    if (cf->ignore_reactor_power) {
        const byte old_bytes[] = {0x7C};
        const byte new_bytes[] = {0xEB};
        write_bytes(0x506F90, old_bytes, new_bytes, sizeof(new_bytes));
        write_bytes(0x506FF6, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->territory_border_fix || DEBUG) {
        write_call(0x523ED7, (int)mod_base_find3);
        write_call(0x52417F, (int)mod_base_find3);
    }
    if (cf->revised_tech_cost) {
        write_call(0x4452D5, (int)mod_tech_rate);
        write_call(0x498D26, (int)mod_tech_rate);
        write_call(0x4A77DA, (int)mod_tech_rate);
        write_call(0x521872, (int)mod_tech_rate);
        write_call(0x5218BE, (int)mod_tech_rate);
        write_call(0x5581E9, (int)mod_tech_rate);
        write_call(0x5BEA4D, (int)mod_tech_rate);
        write_call(0x5BEAC7, (int)mod_tech_rate);

        write_call(0x52935F, (int)mod_tech_selection);
        write_call(0x5BE5E1, (int)mod_tech_selection);
        write_call(0x5BE690, (int)mod_tech_selection);
        write_call(0x5BEB5D, (int)mod_tech_selection);

        /* Remove start delay from factions with negative SOCIAL, RESEARCH value. */
        const byte old_bytes[] = {0x33, 0xFF};
        const byte new_bytes[] = {0x90, 0x90};
        write_bytes(0x4F4F17, old_bytes, new_bytes, sizeof(new_bytes));
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
        write_bytes(0x4EA0B9, old_bytes, NULL, sizeof(old_bytes));

        /* Patch Tree Farms to always increase the clean minerals limit. */
        const byte old_bytes_2[] = {0x85,0xC0,0x74,0x07};
        write_bytes(0x4F2AC6, old_bytes_2, NULL, sizeof(old_bytes_2));
    }
    if (cf->clean_minerals != 16) {
        const byte old_bytes[] = {0x83, 0xC6, 0x10};
        const byte new_bytes[] = {0x83, 0xC6, (byte)cf->clean_minerals};
        write_bytes(0x4E9E41, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->spawn_fungal_towers) {
        /* Spawn nothing in this case. */
        remove_call(0x4F7143);
        remove_call(0x5C363F);
        remove_call(0x57BBC9);
    }
    if (!cf->spawn_spore_launchers) {
        /* Patch the game to spawn Mind Worms instead. */
        *(byte*)0x4F73DB = 0xEB;
        *(byte*)0x4F74D3 = 0xEB;
        *(byte*)0x522808 = 0xEB;
        *(byte*)0x522A9C = 0xEB;
        *(byte*)0x522B8B = 0xEB;
        *(byte*)0x522D8B = 0xEB;
        *(byte*)0x57C804 = 0xEB;
        *(byte*)0x57C99A = 0xEB;
        *(byte*)0x5956FC = 0xEB;
    }
    if (!cf->spawn_sealurks) {
        *(byte*)0x522777 = 0xEB;
    }
    if (!cf->spawn_battle_ogres) {
        *(byte*)0x57BC90 = 0xEB;
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
    if (cf->patch_content_pop) {
        const byte old_bytes[] = {
            0x74,0x19,0x8B,0xD3,0xC1,0xE2,0x06,0x03,0xD3,0x8D,
            0x04,0x53,0x8D,0x0C,0xC3,0x8D,0x14,0x4B,0x8B,0x04,
            0x95,0xE8,0xC9,0x96,0x00,0xEB,0x05,0xB8,0x03,0x00,
            0x00,0x00,0xBE,0x06,0x00,0x00,0x00,0x2B,0xF0
        };
        const byte new_bytes[] = {
            0xE8,0x00,0x00,0x00,0x00,0x89,0xC6,0x90,0x90,0x90,
            0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
            0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
            0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90
        };
        write_bytes(0x4EA56D, old_bytes, new_bytes, sizeof(new_bytes));
        write_call(0x4EA56D, (int)content_pop);
    }
    if (cf->rare_supply_pods) {
        *(byte*)0x592085 = 0xEB; // bonus_at
        *(byte*)0x5920E9 = 0xEB; // bonus_at
        *(byte*)0x5921C8 = 0xEB; // goody_at
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

    if (cf->repair_minimal != 1) {
        const byte old_bytes[] = {0xC7,0x45,0xFC,0x01,0x00,0x00,0x00};
        const byte new_bytes[] = {0xC7,0x45,0xFC,(byte)cf->repair_minimal,0x00,0x00,0x00};
        write_bytes(0x526200, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->repair_fungus != 2) {
        const byte old_bytes[] = {0xC7,0x45,0xFC,0x02,0x00,0x00,0x00};
        const byte new_bytes[] = {0xC7,0x45,0xFC,(byte)cf->repair_fungus,0x00,0x00,0x00};
        write_bytes(0x526261, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->repair_friendly) {
        const byte old_bytes[] = {0xFF,0x45,0xFC};
        const byte new_bytes[] = {0x90,0x90,0x90};
        write_bytes(0x526284, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->repair_airbase) {
        const byte old_bytes[] = {0xFF,0x45,0xFC};
        const byte new_bytes[] = {0x90,0x90,0x90};
        write_bytes(0x5262D4, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->repair_bunker) {
        const byte old_bytes[] = {0xFF,0x45,0xFC};
        const byte new_bytes[] = {0x90,0x90,0x90};
        write_bytes(0x5262F5, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->repair_base != 1) {
        const byte old_bytes[] = {
           0x8B,0x45,0xFC,0x66,0x8B,0x8E,0x32,0x28,0x95,0x00,0x40,0x33,0xD2,0x89,0x45,0xFC};
        const byte new_bytes[] = {
           0x66,0x8B,0x8E,0x32,0x28,0x95,0x00,0x31,0xD2,0x83,0x45,0xFC,(byte)cf->repair_base,0x90,0x90,0x90};
        write_bytes(0x52632E, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->repair_base_native != 10) {
        const byte old_bytes[] = {
            0x33,0xC9,0x8A,0x8E,0x38,0x28,0x95,0x00,0x89,0x4D,0xFC};
        const byte new_bytes[] = {
            0x83,0x45,0xFC,(byte)cf->repair_base_native,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        write_bytes(0x526376, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->repair_base_facility != 10) {
        const byte old_bytes[] = {
            0x33,0xD2,0x8A,0x96,0x38,0x28,0x95,0x00,0x89,0x55,0xFC};
        const byte new_bytes[] = {
            0x83,0x45,0xFC,(byte)cf->repair_base_facility,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        write_bytes(0x526422, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->repair_nano_factory != 10) {
        const byte old_bytes[] = {
            0x33,0xC9,0x8A,0x8E,0x38,0x28,0x95,0x00,0x89,0x4D,0xFC};
        const byte new_bytes[] = {
            0x83,0x45,0xFC,(byte)cf->repair_nano_factory,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        write_bytes(0x526540, old_bytes, new_bytes, sizeof(new_bytes));
    }

    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READ, &attrs)) {
        return false;
    }
    return true;
}

