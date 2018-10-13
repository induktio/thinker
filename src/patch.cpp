
#include "patch.h"

int prev_fac = 0;
std::set<std::pair<int,int>> spawns;

const char* lm_params[] = {
    "crater", "volcano", "jungle", "uranium",
    "sargasso", "ruins", "dunes", "fresh",
    "mesa", "canyon", "geothermal", "ridge",
    "borehole", "nexus", "unity", "fossil"
};

const byte asm_find_start[] = {
    0x8D,0x45,0xF8,0x50,0x8D,0x45,0xFC,0x50,0x8B,0x45,0x08,0x50,
    0xE8,0x00,0x00,0x00,0x00,0x83,0xC4,0x0C,0xE9,0x6E,0x01,0x00,0x00
};

HOOK_API int crop_yield(int fac, int base, int x, int y, int tf) {
    int value = tx_crop_yield(fac, base, x, y, tf);
    MAP* sq = mapsq(x, y);
    if (sq && sq->built_items & TERRA_THERMAL_BORE) {
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

HOOK_API void find_start(int fac, int* tx, int* ty) {
    if (fac != prev_fac) {
        spawns.clear();
        for (int i=0; i<*tx_total_num_vehicles; i++) {
            VEH* v = &tx_vehicles[i];
            if (v->faction_id != 0 && v->faction_id != fac) {
                spawns.insert(mp(v->x_coord, v->y_coord));
            }
        }
    }
    prev_fac = fac;
    int z = 0;
    int x = 0;
    int y = 0;
    int i = 0;
    int k = (*tx_map_axis_y < 80 ? 8 : 16);
    while (i++ < 10) {
        x = random(*tx_map_axis_x) & ~1;
        y = (random(*tx_map_axis_y - k) + k/2) & ~1;
        z = spawn_range(x, y);
        if (z >= max(10, *tx_map_area_sq_root/3 - i*2)) {
            break;
        }
    }
    *tx = x;
    *ty = y;
}

void write_call_ptr(int addr, int func) {
    if (*(byte*)addr != 0xE8 || *(int*)(addr+1) > (int)AC_IMAGE_LEN) {
        throw std::exception();
    }
    *(int*)(addr+1) = func - addr - 5;
}

void remove_call(int addr) {
    if (*(byte*)addr != 0xE8) {
        throw std::exception();
    }
    memset((void*)addr, 0x90, 5);
}

bool patch_setup(Config* conf) {
    DWORD attrs;
    int lm = ~conf->landmarks;
    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READWRITE, &attrs))
        return false;

    write_call_ptr(0x52768A, (int)turn_upkeep);
    write_call_ptr(0x52A4AD, (int)turn_upkeep);
    write_call_ptr(0x4E61D0, (int)base_production);
    write_call_ptr(0x4E888C, (int)crop_yield);
    write_call_ptr(0x5BDC4C, (int)tech_value);
    write_call_ptr(0x579362, (int)enemy_move);

    if (!conf->load_expansion) {
        *(int*)0x45F97A = 0;
    }
    if (conf->faction_placement) {
        memcpy((void*)0x5B1688, asm_find_start, sizeof(asm_find_start));
        write_call_ptr(0x5B1694, (int)find_start);
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

