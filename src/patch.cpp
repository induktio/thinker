
#include "patch.h"
#include "patchdata.h"

const char* ac_mod_alpha = "smac_mod\\alphax";
const char* ac_mod_help = "smac_mod\\helpx";
const char* ac_mod_tutor = "smac_mod\\tutor";
const char* ac_mod_concepts = "smac_mod\\conceptsx";
const char* ac_alpha = "alphax";
const char* ac_opening = "opening";
const char* ac_movlist = "movlist";
const char* ac_movlist_txt = "movlist.txt";
const char* ac_movlistx_txt = "movlistx.txt";
const int8_t NetVersion = (DEBUG ? 64 : 10);


bool FileExists(const char* path) {
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

int __cdecl base_governor_crop_yield(int faction, int base_id, int x, int y, int flags) {
    int value = crop_yield(faction, base_id, x, y, flags);
    MAP* sq = mapsq(x, y);
    if (sq && sq->items & BIT_THERMAL_BORE && value + mine_yield(faction, base_id, x, y, 0)
    + energy_yield(faction, base_id, x, y, 0) >= 6) {
        value++;
    }
    return value;
}

/*
Calculate current vehicle health only for the purposes of
possible damage from genetic warfare probe team action.
*/
int __cdecl probe_veh_health(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    int level = clamp(veh->reactor_type(), 1, 100);
    if (veh->is_artifact()) {
        return 1;
    }
    if (veh->damage_taken > 5*level) {
        return 1;
    }
    return clamp(min(5*level, 10*level - veh->damage_taken), 0, 9999);
}

int __cdecl base_psych_content_pop() {
    int faction = (*CurrentBase)->faction_id;
    assert(valid_player(faction));
    if (is_human(faction)) {
        return conf.content_pop_player[*DiffLevel];
    }
    return conf.content_pop_computer[*DiffLevel];
}

int __cdecl basewin_random_seed() {
    return *CurrentBaseID ^ *MapRandomSeed;
}

int __cdecl zero_value() {
    return 0;
}

int __cdecl config_game_rand() {
    int val = 0;
    for (int i = 0; i < 1000; i++) {
        val = random(conf.faction_file_count);
        if ((1 << val) & ~conf.skip_random_factions) {
            break;
        }
    }
    return val;
}

int __cdecl skip_action_destroy(int id) {
    veh_skip(id);
    *veh_attack_flags = 0;
    return 0;
}

int __cdecl MapWin_gen_terrain_nearby_fungus(int x, int y) {
    MAP* sq;
    int k = 0;
    for (int i=0; i<8; i++) {
        int z = (i - 1) & 7;
        sq = mapsq(wrap(x + NearbyTiles[z][0]), y + NearbyTiles[z][1]);
        if (sq && sq->items & BIT_FUNGUS && is_ocean_shelf(sq)) {
            k |= (1 << i);
        }
    }
    return k;
}

int __cdecl mod_read_basic_rules() {
    // read_basic_rules may be called multiple times
    int value = read_basic_rules();
    if (DEBUG && value == 0 && conf.magtube_movement_rate > 0) {
        int reactor_value = REC_FISSION;
        for (VehReactor r : {REC_FUSION, REC_QUANTUM, REC_SINGULARITY}) {
            if (Reactor[r - 1].preq_tech != TECH_Disable) {
                reactor_value = r;
            }
        }
        for (int i = 0; i < MaxChassisNum; i++) {
            if (Chassis[i].speed < 1 || Chassis[i].preq_tech == TECH_Disable) {
                continue;
            }
            int multiplier = Rules->move_rate_roads * conf.magtube_movement_rate;
            int base_speed = Chassis[i].speed + 1; // elite bonus
            if (Chassis[i].triad == TRIAD_AIR) {
                base_speed += 2*reactor_value; // triad bonus
                if (!Chassis[i].missile) {
                    if (Facility[FAC_CLOUDBASE_ACADEMY].preq_tech != TECH_Disable) {
                        base_speed += 2;
                    }
                    if (Ability[ABL_ID_FUEL_NANOCELLS].preq_tech != TECH_Disable) {
                        base_speed += 2;
                    }
                    if (Ability[ABL_ID_ANTIGRAV_STRUTS].preq_tech != TECH_Disable) {
                        base_speed += 2*reactor_value;
                    }
                }
            } else {
                base_speed += 1; // antigrav struts
            }
            debug("chassis_speed %d %d %d %s\n",
                i, base_speed, base_speed * multiplier, Chassis[i].offsv1_name);
        }
    }
    if (value == 0 && conf.magtube_movement_rate > 0) {
        conf.road_movement_rate = conf.magtube_movement_rate;
        Rules->move_rate_roads *= conf.magtube_movement_rate;
    }
    return value;
}

/*
This is called from enemy_strategy to upgrade prototypes marked with obsolete_factions flag.
Early upgrades are disabled to prevent unnecessary costs for any starting units.
*/
int __stdcall enemy_strategy_upgrade(int veh_id) {
    typedef int (__stdcall *std_int)(int);
    std_int Console_upgrade = (std_int)0x4D06C0;
    if (*CurrentTurn <= 20 + game_start_turn()) {
        return 1; // skip upgrade
    }
    return Console_upgrade(veh_id);
}

/*
Draw Hive faction labels with a more visible high contrast text color.
*/
int __cdecl map_draw_strcmp(const char* s1, const char* UNUSED(s2))
{
    return strcmp(s1, "SPARTANS") && (!conf.render_base_info || strcmp(s1, "HIVE"));
}

char* __cdecl limit_strcpy(char* dst, const char* src)
{
    strcpy_s(dst, StrBufLen, src);
    return dst;
}

char* __cdecl limit_strcat(char* dst, const char* src)
{
    strcat_s(dst, StrBufLen, src);
    return dst;
}

/*
Patch the game engine to use significantly less CPU time by modifying the idle loop.
*/
BOOL WINAPI ModPeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
    int* top_menu_handle = (int*)0x945824;
    int* peek_msg_status = (int*)0x9B7B9C;
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
//    debug("GET %s\t%s\t%s\n", lpAppName, lpKeyName, lpDefault);
    if (!strcmp(lpAppName, GameAppName)) {
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

/*
Override Windows API call to give fake screensize values to SMACX to set the game resolution.
*/
int WINAPI ModGetSystemMetrics(int nIndex) {
    if (nIndex == SM_CXSCREEN) {
        return conf.window_width;
    }
    if (nIndex == SM_CYSCREEN) {
        return conf.window_height;
    }
    return GetSystemMetrics(nIndex);
}

ATOM WINAPI ModRegisterClassA(WNDCLASS* pstWndClass) {
    if (conf.reduced_mode) {
        WinProc = (FWinProc)pstWndClass->lpfnWndProc;
    }
    pstWndClass->lpfnWndProc = ModWinProc;
    return RegisterClassA(pstWndClass);
}

void exit_fail(int addr) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Error while patching address %08X in the game binary.\n"
        "This mod requires Alien Crossfire v2.0 terranx.exe in the same folder.", addr);
    MessageBoxA(0, buf, MOD_VERSION, MB_OK | MB_ICONSTOP);
    exit(EXIT_FAILURE);
}

/*
Replaces old byte sequence with a new byte sequence at address.
Checks that the address contains old values before replacing.
If new_bytes is a null pointer, replace old_bytes with NOP code instead.
*/
void write_bytes(int addr, const byte* old_bytes, const byte* new_bytes, int len) {
    if (addr < (int)AC_IMAGE_BASE || addr + len > (int)AC_IMAGE_BASE + (int)AC_IMAGE_LEN) {
        exit_fail(addr);
    }
    for (int i = 0; i < len; i++) {
        if (*((byte*)addr + i) != old_bytes[i]) {
            debug("write_bytes: address: %08X actual value: %02X expected value: %02X\n",
                addr + i, *((byte*)addr + i), old_bytes[i]);
            exit_fail(addr + i);
        }
        if (new_bytes != NULL) {
            *((byte*)addr + i) = new_bytes[i];
        } else {
            *((byte*)addr + i) = 0x90;
        }
    }
}

/*
Check before patching that the locations contain expected data.
*/
void write_jump(int addr, int func) {
    if ((addr & 0xF) != 0 && *(uint8_t*)(addr) != 0xE9) {
        exit_fail(addr);
    }
    *(byte*)addr = 0xE9;
    *(int*)(addr + 1) = func - addr - 5;
}

/*
Modify conditional short jump such that it is always taken.
*/
void short_jump(int addr) {
    if (*(byte*)addr == 0x74 || *(byte*)addr == 0x75 || *(byte*)addr == 0x7C
    || *(byte*)addr == 0x7D || *(byte*)addr == 0x7E) {
        *(byte*)addr = 0xEB;
    } else if (*(byte*)addr != 0xEB) {
        exit_fail(addr);
    }
}

void write_call(int addr, int func) {
    if (*(byte*)addr != 0xE8 || *(int*)(addr+1) + addr < (int)AC_IMAGE_BASE) {
        exit_fail(addr);
    }
    *(int*)(addr+1) = func - addr - 5;
}

void write_offset(int addr, const void* offset) {
    if (*(byte*)addr != 0x68 || *(int*)(addr+1) < (int)AC_IMAGE_BASE || offset < AC_IMAGE_BASE) {
        exit_fail(addr);
    }
    *(int*)(addr+1) = (int)offset;
}


void remove_call(int addr) {
    if (*(byte*)addr != 0xE8 || *(int*)(addr+1) + addr < (int)AC_IMAGE_BASE) {
        exit_fail(addr);
    }
    memset((void*)addr, 0x90, 5);
}

/*
Screen dimensions must be divisible by 8 to avoid crashes in Buffer_copy.
*/
bool valid_resolution(Config* cf) {
    DEVMODE dm = {};
    dm.dmSize = sizeof(dm);
    bool check = !(cf->window_width & 7) && !(cf->window_height & 7)
        && cf->screen_width >= cf->window_width && cf->screen_height >= cf->window_height;

    if (cf->video_mode == VM_Window) {
        return check;
    }
    for (int i = 0; EnumDisplaySettings(NULL, i, &dm); i++) {
        if (dm.dmBitsPerPel == 32
        && (int)dm.dmPelsWidth == cf->window_width
        && (int)dm.dmPelsHeight == cf->window_height) {
            return check;
        }
    }
    return false;
}

void init_video_config(Config* cf) {
    cf->screen_width = GetSystemMetrics(SM_CXSCREEN);
    cf->screen_height = GetSystemMetrics(SM_CYSCREEN);

    if (cf->video_mode == VM_Native) {
        cf->window_width = cf->screen_width;
        cf->window_height = cf->screen_height;
    } else {
        cf->window_width = max(800, cf->window_width);
        cf->window_height = max(600, cf->window_height);
    }
    if (!valid_resolution(cf)) {
        DEVMODE dm = {};
        dm.dmSize = sizeof(dm);
        int w_width = 800;
        int w_height = 600;

        for (int i = 0; EnumDisplaySettings(NULL, i, &dm); i++) {
            if (dm.dmBitsPerPel == 32 && dm.dmPelsWidth >= 1024 && dm.dmPelsHeight >= 768
            && !(dm.dmPelsWidth & 7) && !(dm.dmPelsHeight & 7)
            && ((int)dm.dmPelsWidth >= w_width || (int)dm.dmPelsHeight >= w_height)) {
                w_width = dm.dmPelsWidth;
                w_height = dm.dmPelsHeight;
                if (cf->video_mode != VM_Native && w_width >= cf->window_width
                && w_height >= cf->window_height) {
                    break;
                }
            }
        }
        cf->window_width = w_width;
        cf->window_height = w_height;
    }
}

bool patch_setup(Config* cf) {
    DWORD attrs;
    DWORD oldattrs;
    int lm = ~cf->landmarks;
    cf->reduced_mode = strcmp((const char*)0x668165, "prax") == 0;

    if (cf->smooth_scrolling && cf->reduced_mode) {
        MessageBoxA(0, "Smooth scrolling feature will be disabled while PRACX is also running.",
            MOD_VERSION, MB_OK | MB_ICONWARNING);
        cf->smooth_scrolling = 0;
    }
    init_video_config(cf);
    debug("patch_setup screen: %dx%d window: %dx%d\n",
        cf->screen_width, cf->screen_height, cf->window_width, cf->window_height);

    if (!VirtualProtect(AC_IMPORT_BASE, AC_IMPORT_LEN, PAGE_EXECUTE_READWRITE, &oldattrs)) {
        return false;
    }
    *(int*)RegisterClassImport = (int)ModRegisterClassA;
    *(int*)GetSystemMetricsImport = (int)ModGetSystemMetrics;
    *(int*)GetPrivateProfileStringAImport = (int)ModGetPrivateProfileStringA;

    if (cf->cpu_idle_fix) {
        *(int*)PeekMessageImport = (int)ModPeekMessage;
    }
    if (!VirtualProtect(AC_IMPORT_BASE, AC_IMPORT_LEN, oldattrs, &attrs)) {
        return false;
    }

    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READWRITE, &attrs)) {
        return false;
    }
    // Apply Scient's Patch changes first
    for(auto& item : AXPatchData) {
        uint8_t* addr = (uint8_t*)(item.address);
        if(*addr != item.old_byte && *addr != item.new_byte
        && (addr < AXSkipVerifyStart || addr > AXSkipVerifyEnd)) {
            exit_fail((int)addr);
        }
        *addr = item.new_byte;
    }
    extra_setup(cf);

    write_jump(0x4688E0, (int)MapWin_gen_overlays);
    write_jump(0x4E3EF0, (int)mod_whose_territory);
    write_jump(0x4F6510, (int)fac_maint);
    write_jump(0x579A30, (int)add_goal);
    write_jump(0x579B70, (int)add_site);
    write_jump(0x579D80, (int)wipe_goals);
    write_jump(0x527290, (int)mod_faction_upkeep);
    write_jump(0x5BF310, (int)X_pop2);
    write_jump(0x4E4AA0, (int)base_first);
    write_jump(0x592250, (int)mod_say_loc);
    write_jump(0x5C1D20, (int)mod_veh_skip);
    write_jump(0x5C1D70, (int)mod_veh_wake);
    write_jump(0x5C1540, (int)veh_speed);
    write_jump(0x6262F0, (int)log_say);
    write_jump(0x626250, (int)log_say2);
    write_jump(0x6263F0, (int)log_say_hex);
    write_jump(0x626350, (int)log_say_hex2);
    write_jump(0x634BE0, (int)FileBox_init);
    write_jump(0x634C20, (int)FileBox_close);
    write_jump(0x645460, (int)limit_strcpy);
    write_jump(0x645470, (int)limit_strcat);
    write_call(0x52768A, (int)mod_turn_upkeep);
    write_call(0x52A4AD, (int)mod_turn_upkeep);
    write_call(0x415F35, (int)mod_base_reset);
    write_call(0x41605A, (int)mod_base_reset);
    write_call(0x417F83, (int)mod_base_reset);
    write_call(0x419F7C, (int)mod_base_reset);
    write_call(0x4E63CD, (int)mod_base_reset);
    write_call(0x4F098F, (int)mod_base_reset);
    write_call(0x4F154C, (int)mod_base_reset);
    write_call(0x4F184C, (int)mod_base_reset);
    write_call(0x4F1BA7, (int)mod_base_reset);
    write_call(0x4F1CBB, (int)mod_base_reset);
    write_call(0x4F3404, (int)mod_base_reset);
    write_call(0x4F3922, (int)mod_base_reset);
    write_call(0x4F5109, (int)mod_base_reset);
    write_call(0x4F5F04, (int)mod_base_reset);
    write_call(0x508B3F, (int)mod_base_reset);
    write_call(0x50AE18, (int)mod_base_reset);
    write_call(0x50CCA8, (int)mod_base_reset);
    write_call(0x561607, (int)mod_base_reset);
    write_call(0x564850, (int)mod_base_reset);
    write_call(0x5B01C7, (int)mod_base_reset);
    write_call(0x4F7A38, (int)consider_hurry);
    write_call(0x5BDC4C, (int)mod_tech_value);
    write_call(0x579362, (int)mod_enemy_move);
    write_call(0x4E888C, (int)base_governor_crop_yield);
    write_call(0x4672A7, (int)mod_base_draw);
    write_call(0x40F45A, (int)mod_base_draw);
    write_call(0x525CC7, (int)mod_setup_player);
    write_call(0x5A3C9B, (int)mod_setup_player);
    write_call(0x5B341C, (int)mod_setup_player);
    write_call(0x5B3C03, (int)mod_setup_player);
    write_call(0x5B3C4C, (int)mod_setup_player);
    write_call(0x5C0984, (int)veh_kill_lift);
    write_call(0x498720, (int)ReportWin_close_handler);
    write_call(0x5A3F7D, (int)probe_veh_health);
    write_call(0x5A3F98, (int)probe_veh_health);
    write_call(0x4E1061, (int)mod_world_build);
    write_call(0x4E113B, (int)mod_world_build);
    write_call(0x58B9BF, (int)mod_world_build);
    write_call(0x58DDD8, (int)mod_world_build);
    write_call(0x408DBD, (int)BaseWin_draw_psych_strcat);
    write_call(0x40F8F8, (int)Basewin_draw_farm_set_font);
    write_call(0x4129E5, (int)BaseWin_draw_energy_set_text_color);
    write_call(0x41B771, (int)BaseWin_action_staple);
    write_call(0x41916B, (int)BaseWin_popup_start);
    write_call(0x4195A6, (int)BaseWin_ask_number);
    write_call(0x41D99D, (int)BaseWin_click_staple);
    write_call(0x48CDA4, (int)popb_action_staple);
    write_call(0x51D1C2, (int)Console_editor_fungus);
    write_call(0x4AED04, (int)SocialWin_social_ai);
    write_call(0x54814D, (int)mod_diplomacy_caption);
    write_call(0x4D0ECF, (int)mod_upgrade_cost);
    write_call(0x4D16D9, (int)mod_upgrade_cost);
    write_call(0x4EFB76, (int)mod_upgrade_cost);
    write_call(0x4EFEB9, (int)mod_upgrade_cost);
    write_call(0x54F7D7, (int)mod_energy_trade);
    write_call(0x54F77E, (int)mod_base_swap);
    write_call(0x542278, (int)mod_buy_tech);
    write_call(0x542425, (int)mod_buy_tech);
    write_call(0x5425CF, (int)mod_buy_tech);
    write_call(0x543843, (int)mod_buy_tech);
    write_call(0x59FBA7, (int)set_treaty);
    write_call(0x559E21, (int)map_draw_strcmp); // veh_draw
    write_call(0x55B5E1, (int)map_draw_strcmp); // base_draw
    write_call(0x4364FB, (int)mod_name_proto);
    write_call(0x4383B2, (int)mod_name_proto);
    write_call(0x4383DE, (int)mod_name_proto);
    write_call(0x43BE8F, (int)mod_name_proto);
    write_call(0x43E06E, (int)mod_name_proto);
    write_call(0x43E663, (int)mod_name_proto);
    write_call(0x581044, (int)mod_name_proto);
    write_call(0x5B301E, (int)mod_name_proto);
    write_call(0x506ADE, (int)mod_battle_fight_2);
    write_call(0x4F7D13, (int)base_upkeep_rand);
    write_call(0x527039, (int)mod_base_upkeep);
    write_call(0x5B41E9, (int)mod_time_warp);
    write_call(0x561948, (int)enemy_strategy_upgrade);
    write_call(0x50474C, (int)mod_battle_compute); // best_defender
    write_call(0x506EA6, (int)mod_battle_compute); // battle_fight_2
    write_call(0x5085E0, (int)mod_battle_compute); // battle_fight_2
    write_call(0x4F7B82, (int)mod_base_research); // base_upkeep

    write_offset(0x50F421, (void*)mod_turn_timer);
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

    write_call(0x52B0E1, (int)mod_wants_to_attack); // wants_prop
    write_call(0x52B0F4, (int)mod_wants_to_attack); // wants_prop
    write_call(0x52B21A, (int)mod_wants_to_attack); // wants_prop
    write_call(0x52B229, (int)mod_wants_to_attack); // wants_prop
    write_call(0x53BED6, (int)mod_wants_to_attack); // diplomacy_check
    write_call(0x54648E, (int)mod_wants_to_attack); // propose_attack
    write_call(0x54649D, (int)mod_wants_to_attack); // propose_attack
    write_call(0x54685A, (int)mod_wants_to_attack); // propose_attack
    write_call(0x546881, (int)mod_wants_to_attack); // propose_attack
    write_call(0x549DF1, (int)mod_wants_to_attack); // threaten
    write_call(0x549E0A, (int)mod_wants_to_attack); // threaten
    write_call(0x54C352, (int)mod_wants_to_attack); // call_off_vendetta
    write_call(0x552566, (int)mod_wants_to_attack); // communicate
    write_call(0x5527E8, (int)mod_wants_to_attack); // communicate
    write_call(0x553325, (int)mod_wants_to_attack); // communicate
    write_call(0x553334, (int)mod_wants_to_attack); // communicate
    write_call(0x55EB3F, (int)mod_wants_to_attack); // encounter
    write_call(0x55EB4E, (int)mod_wants_to_attack); // encounter
    write_call(0x562D0A, (int)mod_wants_to_attack); // enemy_strategy
    write_call(0x562D19, (int)mod_wants_to_attack); // enemy_strategy
    write_call(0x5BCC70, (int)mod_wants_to_attack); // tech_val
    write_call(0x5BCC85, (int)mod_wants_to_attack); // tech_val

    write_call(0x46D368, (int)Console_go_to_init); // MapWin::right_menu
    write_call(0x516833, (int)Console_go_to_init); // Console::cursor_key
    write_call(0x517397, (int)Console_go_to_init); // Console::veh_key
    write_call(0x5173FE, (int)Console_go_to_init); // Console::veh_key
    write_call(0x51840D, (int)Console_go_to_init); // Console::on_key_click
    write_call(0x51ABF6, (int)Console_go_to_init); // Console::on_key_click
    write_call(0x51C68D, (int)Console_go_to_init); // Console::iface_click
    write_call(0x51C8DC, (int)Console_go_to_init); // Console::iface_click
    write_call(0x51CA49, (int)Console_go_to_init); // Console::iface_click
    write_call(0x51CA73, (int)Console_go_to_init); // Console::iface_click

    // Magtube and fungus movement speed patches
    write_call(0x587424, (int)mod_read_basic_rules);
    write_call(0x467711, (int)mod_hex_cost);
    write_call(0x572518, (int)mod_hex_cost);
    write_call(0x5772D7, (int)mod_hex_cost);
    write_call(0x5776F4, (int)mod_hex_cost);
    write_call(0x577E2C, (int)mod_hex_cost);
    write_call(0x577F0E, (int)mod_hex_cost);
    write_call(0x597695, (int)mod_hex_cost);
    write_call(0x59ACA4, (int)mod_hex_cost);
    write_call(0x59B61A, (int)mod_hex_cost);
    write_call(0x59C105, (int)mod_hex_cost);

    // Redirected but not modified from vanilla game logic
    write_call(0x436ADD, (int)mod_proto_cost);
    write_call(0x43704C, (int)mod_proto_cost);
    write_call(0x5817C9, (int)mod_proto_cost);
    write_call(0x581833, (int)mod_proto_cost);
    write_call(0x581BB3, (int)mod_proto_cost);
    write_call(0x581BCB, (int)mod_proto_cost);
    write_call(0x582339, (int)mod_proto_cost);
    write_call(0x582359, (int)mod_proto_cost);
    write_call(0x582378, (int)mod_proto_cost);
    write_call(0x582398, (int)mod_proto_cost);
    write_call(0x5823B0, (int)mod_proto_cost);
    write_call(0x582482, (int)mod_proto_cost);
    write_call(0x58249A, (int)mod_proto_cost);
    write_call(0x58254A, (int)mod_proto_cost);
    write_call(0x5827E4, (int)mod_proto_cost);
    write_call(0x582EC5, (int)mod_proto_cost);
    write_call(0x582FEC, (int)mod_proto_cost);
    write_call(0x5A5D35, (int)mod_proto_cost);
    write_call(0x5A5F15, (int)mod_proto_cost);
    write_call(0x40E666, (int)mod_veh_cost);
    write_call(0x40E9C6, (int)mod_veh_cost);
    write_call(0x418F75, (int)mod_veh_cost);
    write_call(0x492D90, (int)mod_veh_cost);
    write_call(0x493250, (int)mod_veh_cost);
    write_call(0x49E097, (int)mod_veh_cost);
    write_call(0x4F0B5E, (int)mod_veh_cost);
    write_call(0x4F15DF, (int)mod_veh_cost);
    write_call(0x4F4061, (int)mod_veh_cost);
    write_call(0x4FA2AB, (int)mod_veh_cost);
    write_call(0x4FA510, (int)mod_veh_cost);
    write_call(0x4FD6B5, (int)mod_veh_cost);
    write_call(0x56D38D, (int)mod_veh_cost);
    write_call(0x57AB53, (int)mod_veh_cost);
    write_call(0x593D35, (int)mod_veh_cost);
    write_call(0x593F72, (int)mod_veh_cost);
    write_call(0x594B8E, (int)mod_veh_cost);
    write_call(0x4C1DC0, (int)mod_morale_alien);
    write_call(0x4DDF29, (int)mod_morale_alien);
    write_call(0x4DDF6A, (int)mod_morale_alien);
    write_call(0x501609, (int)mod_morale_alien);
    write_call(0x501994, (int)mod_morale_alien);
    write_call(0x5230AC, (int)mod_morale_alien);
    write_call(0x5597BD, (int)mod_morale_alien);
    write_call(0x5672C7, (int)mod_morale_alien);
    write_call(0x595E61, (int)mod_morale_alien);
    write_call(0x5C0E6D, (int)mod_morale_alien);

    // Redirect functions for foreign_treaty_popup option
    write_call(0x55DC00, (int)mod_NetMsg_pop); // enemies_treaty
    write_call(0x55DF04, (int)mod_NetMsg_pop); // enemies_treaty
    write_call(0x55E364, (int)mod_NetMsg_pop); // enemies_treaty
    write_call(0x55CB33, (int)mod_NetMsg_pop); // enemies_war
    write_call(0x597141, (int)mod_NetMsg_pop); // order_veh

    // Custom ambient music option
    write_call(0x445846, (int)load_music_strcmpi);
    write_call(0x445898, (int)load_music_strcmpi);
    write_call(0x4458EE, (int)load_music_strcmpi);
    write_call(0x445956, (int)load_music_strcmpi);
    write_call(0x4459C1, (int)load_music_strcmpi);
    write_call(0x445A2F, (int)load_music_strcmpi);
    write_call(0x445AB2, (int)load_music_strcmpi);

    if (cf->directdraw) {
        *(int32_t*)0x45F9EF = cf->window_width;
        *(int32_t*)0x45F9F4 = cf->window_height;
    }
    if (!cf->reduced_mode) {
        write_call(0x62D3EC, (int)mod_Win_init_class);
        write_call(0x403BD4, (int)mod_amovie_project); // amovie_project2
        write_call(0x4F2B4B, (int)mod_amovie_project); // base_production
        write_call(0x524D06, (int)mod_amovie_project); // end_of_game
        write_call(0x524D28, (int)mod_amovie_project); // end_of_game
        write_call(0x5253F5, (int)mod_amovie_project); // end_of_game
        write_call(0x525407, (int)mod_amovie_project); // end_of_game
        write_call(0x52AB6D, (int)mod_amovie_project); // control_game
        write_call(0x5B3681, (int)mod_amovie_project); // eliminate_player
    }
    if (cf->smooth_scrolling) {
        write_offset(0x50F3DC, (void*)mod_blink_timer);
        write_call(0x46AB81, (int)mod_gen_map);
        write_call(0x46ABD1, (int)mod_gen_map);
        write_call(0x46AC56, (int)mod_gen_map);
        write_call(0x46ACC5, (int)mod_gen_map);
        write_call(0x46A5A9, (int)mod_calc_dim);
        write_call(0x48B6C2, (int)mod_calc_dim);
        write_call(0x48B893, (int)mod_calc_dim);
        write_call(0x48B91F, (int)mod_calc_dim);
        write_call(0x48BA15, (int)mod_calc_dim);
    }
    if (cf->skip_random_factions) {
        const char* filename = (cf->smac_only ? ac_mod_alpha : ac_alpha);
        vec_str_t lines;
        reader_file(lines, filename, "#FACTIONS", 100);
        reader_file(lines, filename, "#NEWFACTIONS", 100);
        reader_file(lines, filename, "#CUSTOMFACTIONS", 100);
        cf->faction_file_count = lines.size();
        debug("patch_setup factions: %d\n", cf->faction_file_count);

        memset((void*)0x58B63C, 0x90, 10);
        write_call(0x58B526, (int)zero_value); // config_game
        write_call(0x58B539, (int)zero_value); // config_game
        write_call(0x58B632, (int)config_game_rand); // config_game
        write_call(0x587066, (int)config_game_rand); // read_factions
    }
    if (cf->autosave_interval > 0) {
        write_jump(0x5ABD20, (int)mod_auto_save);
    }
    if (cf->editor_free_units) {
        write_call(0x4DF19B, (int)mod_veh_init); // Console_editor_veh
    }
    if (DEBUG) {
        if (cf->minimal_popups) {
            remove_call(0x4E5F96); // #BEGINPROJECT
            remove_call(0x4E5E0D); // #CHANGEPROJECT
            remove_call(0x4F4817); // #DONEPROJECT
            remove_call(0x4F2A4C); // #PRODUCE (project/satellite)
        }
    }
    /*
    Provide better defaults for multiplayer settings.
    */
    *(int8_t*)0x481EE0 = 1; // MapOceanCoverage
    *(int8_t*)0x481EE7 = 1; // MapErosiveForces
    *(int8_t*)0x481EEE = 1; // MapNativeLifeForms
    *(int8_t*)0x481EF5 = 1; // MapCloudCover
    *(int8_t*)0x4E3222 = NetVersion; // AlphaNet::setup
    *(int8_t*)0x52AA5C = NetVersion; // control_game
    *(int8_t*)0x627C8B = 11; // pop_ask_number maximum length

    /*
    Hide unnecessary region_base_plan display next to base names in debug mode.
    */
    remove_call(0x468175);
    remove_call(0x468186);

    /*
    Hide "<other faction> have altered the rainfall patterns" messages from status display.
    */
    {
        const byte old_bytes[] = {0x75,0x07};
        const byte new_bytes[] = {0x75,0x16};
        write_bytes(0x4CA44E, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Remove old code for selecting secret projects in time_warp.
    */
    if (*(uint16_t*)0x5AF56D == 0xCB83 && *(uint16_t*)0x5AF5B5 == 0x4D8B) {
        *(uint16_t*)0x5AF56D = 0x43EB; // jmp short loc_5AF5B2
        *(uint16_t*)0x5AF5B5 = 0x08EB; // jmp short loc_5AF5BF
    } else {
        assert(0);
    }

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
        write_call(0x463659, (int)MapWin_gen_terrain_nearby_fungus);
    }

    /*
    Fix issue where attacking other satellites doesn't work in
    Orbital Attack View when smac_only is activated.
    */
    {
        const byte old_bytes[] = {0xD2,0x01,0x00,0x00};
        const byte new_bytes[] = {0x75,0x01,0x00,0x00};
        write_bytes(0x4AB327, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Disable legacy upkeep code in the game engine that might cause AI formers
    to be reassigned to nearby bases that are owned by other factions.
    */
    {
        const byte old_bytes[] = {0x0F,0x84,0x92,0x01,0x00};
        const byte new_bytes[] = {0xE9,0x93,0x01,0x00,0x00};
        write_bytes(0x561FF2, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Make sure the game engine can read movie settings from "movlistx.txt".
    */
    if (FileExists(ac_movlist_txt) && !FileExists(ac_movlistx_txt)) {
        CopyFile(ac_movlist_txt, ac_movlistx_txt, TRUE);
    }

    /*
    Patch base window bug where the population row would display superdrones
    even though they would be suppressed by psych-related effects.
    Also initialize Random::Random with base-specific constant instead of timeGetTime.
    */
    {
        const byte old_bytes[] = {0xFF,0x15,0x68,0x93,0x66,0x00};
        const byte new_bytes[] = {0xE8,0x00,0x00,0x00,0x00,0x90};
        write_bytes(0x414B81, old_bytes, new_bytes, sizeof(new_bytes));
        write_call(0x414B81, (int)basewin_random_seed); // BaseWin_draw_pop
        write_bytes(0x49D57B, old_bytes, new_bytes, sizeof(new_bytes));
        write_call(0x49D57B, (int)basewin_random_seed); // ReportWin_draw_ops
        memset((void*)0x414D52, 0x90, 5); // Superdrone icons, aliens
        memset((void*)0x414EE2, 0x90, 7); // Superdrone icons, humans
    }

    /*
    Fix faction graphics bug that appears when Alpha Centauri.ini
    has a different set of faction filenames than the loaded scenario file.
    Normally when a scenario file is being loaded, it would skip load_faction_art calls
    which could result in an incorrect faction graphics set being displayed.
    */
    {
        const byte old_bytes[] = {0x0F,0x85,0x99,0x04,0x00,0x00};
        write_bytes(0x5AAEE0, old_bytes, NULL, sizeof(old_bytes));
    }

    /*
    Patch map window to render more detailed tiles when zoomed out.
    83 7D 10 F8      cmp     [ebp+zoom], 0FFFFFFF8h
    */
    if (cf->render_high_detail) {
        int locations[] = {
            0x4636AE,
            0x465050,
            0x465345,
            0x465DB8,
            0x465E08,
            0x465E91,
            0x465EBF,
            0x465EEF,
            0x465F2E,
            0x465FB1,
            0x466005,
            0x46605C,
            0x466085,
            0x4660AE,
            0x466113,
            0x466444,
        };
        for (int p : locations) {
            if (*(uint32_t*)p == 0xF8107D83 || *(uint32_t*)p == 0xF6107D83) {
                *(uint32_t*)p = 0xF2107D83;
            } else {
                assert(0);
            }
        }
        if (cf->window_width >= 1024) {
            short_jump(0x4AE66C); // SetupWin_draw_item resolution checks
            write_call(0x4AE6D5, (int)SetupWin_buffer_draw);
            write_call(0x4AE710, (int)SetupWin_buffer_copy);
            write_call(0x4AE73B, (int)SetupWin_soft_update3);
        }
    }
    /*
    Fix visual issues where the game sometimes did not update the map properly
    when recentering it offscreen on native resolution mode.
    */
    if (cf->render_high_detail && !cf->directdraw) {
        write_call(0x4BE697, (int)mod_MapWin_focus); // TutWin::tut_map
        write_call(0x51094C, (int)mod_MapWin_focus); // Console::focus
        write_call(0x51096D, (int)mod_MapWin_focus); // Console::focus
        write_call(0x510C81, (int)mod_MapWin_focus); // sub_510B70
        write_call(0x51A7B4, (int)mod_MapWin_focus); // Console::on_key_click
        write_call(0x51B594, (int)mod_MapWin_focus); // Console::on_key_click
        write_call(0x51C7BC, (int)mod_MapWin_focus); // Console::iface_click
        write_call(0x522504, (int)mod_MapWin_focus); // alien_fauna
        write_call(0x5583ED, (int)mod_MapWin_focus); // communicate
    }

    /*
    Find nearest base for returned probes in order_veh and probe functions.
    */
    {
        const byte old_order_veh[] = {
            0xBA,0x46,0xD0,0x97,0x00,0x33,0xC9,0x8A,0x4A,0xFE,
            0x3B,0x4D,0xE4,0x75,0x0B,0x0F,0xBE,0x0A,0x3B,0xCF,
            0x7E,0x04,0x8B,0xF9,0x8B,0xF0,0x40,0x81,0xC2,0x34,
            0x01,0x00,0x00,0x3B,0xC3,0x7C,0xE0
        };
        const byte new_order_veh[] = {
            0x8B,0x7D,0xE0,0x57,0xE8,0x00,0x00,0x00,0x00,0x8B,
            0xF0,0x83,0xC4,0x04,0x90,0x90,0x90,0x90,0x90,0x90,
            0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
            0x90,0x90,0x90,0x90,0x90,0x90,0x90
        };
        const byte old_probe[] = {0x53};
        const byte new_probe[] = {0x50};
        write_bytes(0x597021, old_order_veh, new_order_veh, sizeof(new_order_veh));
        write_call(0x597025, (int)find_return_base);
        write_bytes(0x5A430F, old_probe, new_probe, sizeof(new_probe));
        write_call(0x5A432C, (int)probe_return_base);
    }

    /*
    Fix bug that prevents the turn from advancing after force-ending the turn
    while any player-owned needlejet in flight has moves left.
    */
    {
        const byte old_bytes[] = {0x3B,0xC8,0x0F,0x8C,0x65,0x01,0x00,0x00};
        const byte new_bytes[] = {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        write_bytes(0x5259CE, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Patch Energy Market Crash event to reduce energy reserves only by 1/4 instead of 3/4.
    */
    {
        const byte old_bytes[] = {0x99,0x83,0xE2,0x03,0x03,0xC2,0xC1,0xF8,0x02};
        const byte new_bytes[] = {0xC1,0xF8,0x02,0x6B,0xC0,0x03,0x90,0x90,0x90};
        write_bytes(0x520725, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Fix diplomacy dialog issues when both human and alien factions are involved
    in a base capture by removing the event that spawns additional colony pods.
    */
    {
        const byte old_bytes[] = {0x0F,0x84};
        const byte new_bytes[] = {0x90,0xE9};
        write_bytes(0x50D67A, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Fix issue where TECHSHARE faction ability always skips the checks
    for infiltration conditions while smac_only mode is activated.
    Spying by probe team, pact, governor or Empath Guild is required.
    */
    {
        const byte old_bytes[] = {0x74,0x51};
        const byte new_bytes[] = {0x90,0x90};
        write_bytes(0x5BC386, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Prevent the AI from making unnecessary trades where it sells their techs for maps
    owned by the player. The patch removes TRADETECH4 / TRADETECH5 dialogue paths
    making the AI usually demand a credit payment for any techs.
    */
    {
        const byte old_bytes[] = {0x0F,0x8D};
        const byte new_bytes[] = {0x90,0xE9};
        write_bytes(0x5414AA, old_bytes, new_bytes, sizeof(new_bytes));
    }

    /*
    Fix Stockpile Energy when it enabled double production if the base produces one item
    and then switches to Stockpile Energy on the same turn gaining additional credits.
    */
    {
        short_jump(0x4EC33D); // base_energy skip stockpile energy
        write_call(0x4F7A2F, (int)mod_base_production); // base_upkeep
    }

    /*
    Additional combat bonus options for PSI effects.
    */
    {
        const byte old_bytes[] = {0x8B,0xC7,0x99,0x2B,0xC2,0xD1,0xF8};
        const byte new_bytes[] = {0x57,0xE8,0x00,0x00,0x00,0x00,0x5F};
        write_jump(0x501500, (int)psi_factor);

        write_bytes(0x501CFB, old_bytes, new_bytes, sizeof(new_bytes));
        write_call(0x501CFC, (int)neural_amplifier_bonus); // get_basic_defense
        *(int*)0x50209F = cf->neural_amplifier_bonus; // battle_compute

        write_bytes(0x501D11, old_bytes, new_bytes, sizeof(new_bytes));
        write_call(0x501D12, (int)fungal_tower_bonus); // get_basic_defense
        *(int*)0x5020EC = cf->fungal_tower_bonus; // battle_compute

        write_bytes(0x501914, old_bytes, new_bytes, sizeof(new_bytes));
        write_call(0x501915, (int)dream_twister_bonus); // get_basic_offense
        *(int*)0x501F9C = cf->dream_twister_bonus; // battle_compute
    }

    /*
    Modify Perimeter Defense and Tachyon Field defense values.
    */
    {
        const byte old_perimeter[] = {0xBE,0x04,0x00,0x00,0x00};
        const byte new_perimeter[] = {0xBE,(byte)(2 + cf->perimeter_defense_bonus),0x00,0x00,0x00};
        write_bytes(0x5034B9, old_perimeter, new_perimeter, sizeof(new_perimeter));

        const byte old_tachyon[] = {0x83,0xC6,0x02};
        const byte new_tachyon[] = {0x83,0xC6,(byte)cf->tachyon_field_bonus};
        write_bytes(0x503506, old_tachyon, new_tachyon, sizeof(new_tachyon));
    }

    /*
    Initial content base population before psych modifiers.
    */
    {
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
        write_call(0x4EA56D, (int)base_psych_content_pop);
    }

    if (cf->smac_only) {
        if (!FileExists("smac_mod/alphax.txt")
        || !FileExists("smac_mod/conceptsx.txt")
        || !FileExists("smac_mod/helpx.txt")
        || !FileExists("smac_mod/tutor.txt")) {
            MessageBoxA(0, "Error while opening smac_mod folder. Unable to start smac_only mode.",
                MOD_VERSION, MB_OK | MB_ICONSTOP);
            exit(EXIT_FAILURE);
        }
        *(int*)0x45F97A = 0;
        *(const char**)0x691AFC = ac_mod_alpha;
        *(const char**)0x691B00 = ac_mod_help;
        *(const char**)0x691B14 = ac_mod_tutor;
        write_offset(0x42B30E, ac_mod_concepts);
        write_offset(0x42B49E, ac_mod_concepts);
        write_offset(0x42C450, ac_mod_concepts);
        write_offset(0x42C7C2, ac_mod_concepts);
        write_offset(0x403BA8, ac_movlist);
        write_offset(0x4BEF8D, ac_movlist);
        write_offset(0x52AB68, ac_opening);
        // Enable custom faction selection during the game setup.
        memset((void*)0x58A5E1, 0x90, 6);
        memset((void*)0x58B76F, 0x90, 2);
        memset((void*)0x58B9F3, 0x90, 2);
    }
    if (cf->delay_drone_riots) {
        write_call(0x4F7A52, (int)mod_base_growth);
        write_jump(0x4F5E08, (int)mod_drone_riot);
    }
    if (cf->skip_drone_revolts) {
        const byte old_bytes[] = {0x0F, 0x8E};
        const byte new_bytes[] = {0x90, 0xE9};
        write_bytes(0x4F5663, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->counter_espionage) {
        // Check for flag DIPLO_RENEW_INFILTRATOR when choosing the menu entries
        const byte old_bytes[] = {0xF6, 0xC5, 0x10};
        const byte new_bytes[] = {0xF6, 0xC5, 0x80};
        write_bytes(0x59F90C, old_bytes, new_bytes, sizeof(new_bytes)); // probe
        write_bytes(0x59FB99, old_bytes, new_bytes, sizeof(new_bytes)); // probe
        // Modify popup_start function argument order
        const byte old_bytes_2[] = {0x68,0x38,0x03,0x69,0x00,0x68,0xA8,0x8A,0x9B,0x00};
        const byte new_bytes_2[] = {0x8B,0x45,0x0C,0x50,0x8B,0x45,0x08,0x50,0x90,0x90};
        write_bytes(0x59F834, old_bytes_2, new_bytes_2, sizeof(new_bytes_2)); // probe
        write_call(0x59F83E, (int)probe_popup_start);
    }
    if (cf->render_probe_labels) {
        memset((void*)0x559590, 0x90, 2);
        memset((void*)0x5599DE, 0x90, 6);
    }
    if (cf->new_base_names) {
        write_call(0x4CFF47, (int)mod_name_base);
        write_call(0x4E4CFC, (int)mod_name_base);
        write_call(0x4F7E18, (int)mod_name_base);
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
    {
        // Skip default randomize faction leader personalities code
        short_jump(0x5B1254); // setup_player

        // Remove aquatic faction extra sea colony pod in setup_player
        const byte old_bytes[] = {0x66,0x89,0xB1,0x56,0x28,0x95,0x00};
        write_bytes(0x5B3060, old_bytes, NULL, sizeof(old_bytes));
        remove_call(0x5B3052);
        remove_call(0x5B3067);
    }
    if (cf->alien_early_start) {
        const byte old_bytes[] = {0x75,0x1A};
        const byte new_bytes[] = {0x90,0x90};
        write_bytes(0x589081, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->cult_early_start) {
        const byte old_bytes[] = {0x0F,0x85};
        const byte new_bytes[] = {0x90,0xE9};
        write_bytes(0x589097, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->alien_early_start || cf->cult_early_start) {
        // Use default starting year instead of advancing by 5 turns
        const byte old_bytes[] = {0xC7,0x05,0xD4,0x64,0x9A,0x00,0x05,0x00,0x00,0x00};
        write_bytes(0x5AE3C3, old_bytes, NULL, sizeof(old_bytes));
        write_bytes(0x5AE3E9, old_bytes, NULL, sizeof(old_bytes));
    }
    if (cf->ignore_reactor_power) {
        short_jump(0x506F90);
        short_jump(0x506FF6);
        // Adjust combat odds confirmation dialog to match new odds
        write_call(0x506D07, (int)mod_best_defender);
        write_call(0x5082AF, (int)battle_fight_parse_num);
        write_call(0x5082B7, (int)battle_fight_parse_num);
    }
    if (cf->modify_unit_morale) {
        write_jump(0x5C0E40, (int)mod_morale_veh);
        write_call(0x50211F, (int)mod_get_basic_offense);
        write_call(0x50274A, (int)mod_get_basic_offense);
        write_call(0x5044EB, (int)mod_get_basic_offense);
        write_call(0x502A69, (int)mod_get_basic_defense);
    }
    if (cf->skip_default_balance) {
        remove_call(0x5B41F5);
    }
    if (cf->facility_capture_fix) {
        remove_call(0x50D06A);
        remove_call(0x50D074);
    }
    if (cf->territory_border_fix || DEBUG) {
        write_call(0x523ED7, (int)mod_base_find3);
        write_call(0x52417F, (int)mod_base_find3);
    }
    if (cf->simple_cost_factor) {
        write_jump(0x4E4430, (int)mod_cost_factor);
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
    if (cf->simple_hurry_cost) {
        short_jump(0x41900D);
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
        short_jump(0x4F73DB);
        short_jump(0x4F74D3);
        short_jump(0x522808);
        short_jump(0x522A9C);
        short_jump(0x522B8B);
        short_jump(0x522D8B);
        short_jump(0x57C804);
        short_jump(0x57C99A);
        short_jump(0x5956FC);
    }
    if (!cf->spawn_sealurks) {
        short_jump(0x522777);
    }
    if (!cf->spawn_battle_ogres) {
        short_jump(0x57BC90);
    }
    if (cf->collateral_damage_value != 3) {
        const byte old_bytes[] = {0xB2, 0x03};
        const byte new_bytes[] = {0xB2, (byte)cf->collateral_damage_value};
        write_bytes(0x50AAA5, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->aquatic_bonus_minerals) {
        const byte old_bytes[] = {0x46};
        const byte new_bytes[] = {0x90};
        write_bytes(0x4E7604, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->planetpearls) {
        const byte old_bytes[] = {
            0x8B,0x45,0x08,0x6A,0x00,0x6A,0x01,0x50,0xE8,0x46,
            0xAD,0x0B,0x00,0x83,0xC4,0x0C,0x40,0x8D,0x0C,0x80,
            0x8B,0x06,0xD1,0xE1,0x03,0xC1,0x89,0x06
        };
        write_bytes(0x5060ED, old_bytes, NULL, sizeof(old_bytes));
    }
    if (!cf->event_perihelion) {
        short_jump(0x51F481);
    }
    if (cf->event_sunspots > 0) {
        const byte old_bytes[] = {0x83,0xC0,0x0A,0x6A,0x14};
        const byte new_bytes[] = {0x83,0xC0,
            (byte)cf->event_sunspots,0x6A,(byte)(cf->event_sunspots)};
        write_call(0x52064C, (int)zero_value);
        write_bytes(0x520651, old_bytes, new_bytes, sizeof(new_bytes));
    } else if (!cf->event_sunspots) {
        const byte old_bytes[] = {0x0F, 0x8C};
        const byte new_bytes[] = {0x90, 0xE9};
        write_bytes(0x520615, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->alien_guaranteed_techs) {
        short_jump(0x5B29F8);
    }
    if (cf->natives_weak_until_turn >= 0) {
        const byte old_bytes[] = {0x83, 0x3D, 0xD4, 0x64, 0x9A, 0x00, 0x0F};
        const byte new_bytes[] = {0x83, 0x3D, 0xD4, 0x64, 0x9A, 0x00,
            (byte)cf->natives_weak_until_turn};
        write_bytes(0x507C22, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->rare_supply_pods) {
        short_jump(0x592085); // bonus_at
        short_jump(0x5920E9); // bonus_at
        short_jump(0x5921C8); // goody_at
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
    flushlog();
    return true;
}

