
#include "patch.h"

const char* lm_params[] = {
    "lm_crater", "lm_volcano", "lm_jungle", "lm_uranium",
    "lm_sargasso", "lm_ruins", "lm_dunes", "lm_freshwater",
    "lm_mesa", "lm_canyon", "lm_geothermal", "lm_ridge",
    "lm_borehole", "lm_nexus", "lm_unity", "lm_fossil"
};

HOOK_API int crop_yield(int fac, int base, int x, int y, int tf) {
    int value = tx_crop_yield(fac, base, x, y, tf);
    MAP* sq = mapsq(x, y);
    if (sq && sq->built_items & TERRA_THERMAL_BORE) {
        value++;
    }
    return value;
}

void write_call_ptr(int addr, int func) {
    if (*(byte*)(addr-1) != 0xE8 || *(int*)addr > (int)AC_IMAGE_LEN) {
        throw std::exception();
    }
    *(int*)addr = func - addr - sizeof(int);
}

void remove_call(int addr) {
    if (*(byte*)(addr) != 0xE8) {
        throw std::exception();
    }
    memset((void*)addr, 0x90, 5);
}

bool patch_setup(Config* conf) {
    DWORD attrs;
    int lm = ~conf->landmarks;
    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READWRITE, &attrs))
        return false;

    write_call_ptr(0x52768B, (int)turn_upkeep);
    write_call_ptr(0x52A4AE, (int)turn_upkeep);
    write_call_ptr(0x4E61D1, (int)base_production);
    write_call_ptr(0x4E888D, (int)crop_yield);
    write_call_ptr(0x5BDC4D, (int)tech_value);
    write_call_ptr(0x579363, (int)enemy_move);

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

