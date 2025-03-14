
#include "patch.h"
#include "patchdata.h"

const char* ac_genwarning_sm_pcx = "genwarning_sm.pcx";


bool FileExists(const char* path) {
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

int __cdecl BaseWin_random_seed() {
    return *CurrentBaseID ^ *MapRandomSeed;
}

int __cdecl ProdPicker_calculate_itoa(int UNUSED(value), char* buf, int UNUSED(base)) {
    snprintf(buf, 16, "%d", stockpile_energy(*CurrentBaseID));
    return 0;
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
    mod_veh_skip(id);
    *VehAttackFlags = 0;
    return 0;
}

/*
Change FORESTGROWS / KELPGROWS / PRODUCE popups into delayed notification items on the message log.
*/
int __cdecl alien_fauna_pop2(const char* label, const char* imagefile, int UNUSED(a3)) {
    return NetMsg_pop(NetMsg, label, 5000, 0, imagefile);
}

int __cdecl base_production_popp(const char* textfile, const char* label, int a3, const char* imagefile, int a5) {
    int item_id = (*CurrentBase ? (*CurrentBase)->item() : 0);
    if (item_id == -FAC_SKY_HYDRO_LAB
    || item_id == -FAC_ORBITAL_POWER_TRANS
    || item_id == -FAC_NESSUS_MINING_STATION
    || item_id == -FAC_ORBITAL_DEFENSE_POD
    || item_id == -FAC_GEOSYNC_SURVEY_POD) {
        return NetMsg_pop(NetMsg, label, 5000, 0, imagefile);
    }
    return popp(textfile, label, a3, imagefile, a5);
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

/*
Fix possible crash when say_orders is called without CurrentBase pointer being set.
*/
int mod_say_orders(char* buf, int veh_id) {
    VEH* veh = &Vehs[veh_id];
    if (!*CurrentBase) {
        if (veh->home_base_id >= 0) {
            set_base(veh->home_base_id);
        } else {
            int base_id = mod_base_find3(veh->x, veh->y, veh->faction_id, -1, -1, -1);
            if (base_id >= 0) {
                set_base(base_id);
            }
        }
    }
    return say_orders((int)buf, veh_id);
}

/*
This is called from enemy_strategy to upgrade or remove units marked with obsolete_factions flag.
Returning non-zero makes the function skip all additional actions on veh_id.
*/
int __thiscall enemy_strategy_upgrade(Console* This, int veh_id) {
    if (conf.factions_enabled >= Vehs[veh_id].faction_id) {
        return 1;
    }
    return Console_upgrade(This, veh_id);
}

/*
Draw Hive faction labels with a more visible high contrast text color.
*/
int __cdecl map_draw_strcmp(const char* s1, const char* UNUSED(s2))
{
    return strcmp(s1, "SPARTANS") && (!conf.render_base_info || strcmp(s1, "HIVE"));
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

/*
Replaces old byte sequence with a new byte sequence at address.
Checks that the address contains old values before replacing.
If new_bytes is a null pointer, replace old_bytes with NOP code instead.
*/
void write_bytes(int32_t addr, const byte* old_bytes, const byte* new_bytes, int32_t len) {
    if (addr < (int32_t)AC_IMAGE_BASE || addr + len > (int32_t)AC_IMAGE_BASE + (int32_t)AC_IMAGE_LEN) {
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

void write_byte(int32_t addr, byte old_byte, byte new_byte) {
    if (addr < (int32_t)AC_IMAGE_BASE) {
        exit_fail(addr);
    }
    if (*(byte*)addr != old_byte && *(byte*)addr != new_byte) {
        exit_fail(addr);
    }
    *(byte*)addr = new_byte;
}

void write_word(int32_t addr, int32_t old_word, int32_t new_word) {
    if (addr < (int32_t)AC_IMAGE_BASE) {
        exit_fail(addr);
    }
    if (*(int32_t*)addr != old_word && *(int32_t*)addr != new_word) {
        exit_fail(addr);
    }
    *(int32_t*)addr = new_word;
}

/*
Check before patching that the locations contain expected data.
*/
void write_jump(int32_t addr, int32_t func) {
    if ((addr & 0xF) != 0 && *(byte*)(addr) != 0xE9) {
        exit_fail(addr);
    }
    *(byte*)addr = 0xE9;
    *(int32_t*)(addr + 1) = func - addr - 5;
}

/*
Modify conditional short jump such that it is always taken.
*/
void short_jump(int32_t addr) {
    if (*(byte*)addr == 0x74 || *(byte*)addr == 0x75 || *(byte*)addr == 0x7C
    || *(byte*)addr == 0x7D || *(byte*)addr == 0x7E) {
        *(byte*)addr = 0xEB;
    } else if (*(byte*)addr != 0xEB) {
        exit_fail(addr);
    }
}

/*
Modify conditional long jump such that it is always taken.
*/
void long_jump(int32_t addr) {
    if (*(uint16_t*)addr == 0x840F || *(uint16_t*)addr == 0x850F || *(uint16_t*)addr == 0x8C0F
    || *(uint16_t*)addr == 0x8D0F || (*(uint16_t*)addr == 0x8E0F)) {
        *(uint16_t*)addr = 0xE990;
    } else if (*(uint16_t*)addr != 0xE990) {
        exit_fail(addr);
    }
}

void write_call(int32_t addr, int32_t func) {
    if (*(uint16_t*)addr == 0x15FF && *(int32_t*)(addr+2) + addr >= (int32_t)AC_IMAGE_BASE) {
        *(uint16_t*)addr = 0xE890;
        *(int32_t*)(addr+2) = func - addr - 6; // offset decreased by one
    } else if (*(byte*)addr == 0xE8 && *(int32_t*)(addr+1) + addr >= (int32_t)AC_IMAGE_BASE) {
        *(int32_t*)(addr+1) = func - addr - 5;
    } else {
        exit_fail(addr);
    }
}

void write_offset(int32_t addr, const void* offset) {
    if (*(byte*)addr != 0x68 || *(int32_t*)(addr+1) < (int32_t)AC_IMAGE_BASE || offset < AC_IMAGE_BASE) {
        exit_fail(addr);
    }
    *(int32_t*)(addr+1) = (int32_t)offset;
}

void remove_call(int32_t addr) {
    if (*(byte*)addr != 0xE8 || *(int32_t*)(addr+1) + addr < (int32_t)AC_IMAGE_BASE) {
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
    const char* DefaultPaths[] = {
        "C:\\Program Files\\VideoLAN\\VLC\\vlc.exe",
        "C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe",
    };
    char buf_path[4096] = {};
    char buf_args[4096] = {};
    GetPrivateProfileStringA(GameAppName, "MoviePlayerPath", "<DEFAULT>", buf_path, 4096, GameIniFile);
    GetPrivateProfileStringA(GameAppName, "MoviePlayerArgs",
        "--fullscreen --video-on-top --play-and-exit --no-repeat --swscale-mode=2", buf_args, 4096, GameIniFile);

    char* path = strtrim(&buf_path[0]);
    char* args = strtrim(&buf_args[0]);
    if (!strlen(path)) {
        conf.video_player = 1;
    } else {
        bool found = false;
        if (!strcmp(path, "<DEFAULT>")) {
            for (auto& cur_path : DefaultPaths) {
                if (FileExists(cur_path)) {
                    conf.video_player = 2;
                    video_player_path = std::string(cur_path);
                    video_player_args = std::string(args);
                    prefs_put2("MoviePlayerPath", cur_path);
                    prefs_put2("MoviePlayerArgs", args);
                    found = true;
                    break;
                }
            }
        } else if (FileExists(path)) {
            conf.video_player = 2;
            video_player_path = std::string(path);
            video_player_args = std::string(args);
            prefs_put2("MoviePlayerPath", path);
            prefs_put2("MoviePlayerArgs", args);
            found = true;
        }
        if (!found) {
            int value = MessageBoxA(0,
                "Video player not found from MoviePlayerPath in Alpha Centauri.ini.\n"\
                "Select YES to reset game to the default video player.\n"\
                "Select NO to skip video playback temporarily.",
                MOD_VERSION, MB_YESNO | MB_ICONWARNING);
            if (value == IDYES) {
                conf.video_player = 1;
                prefs_put2("MoviePlayerPath", "");
                prefs_put2("MoviePlayerArgs", "");
            } else {
                conf.video_player = 0;
                prefs_put2("MoviePlayerPath", "<DEFAULT>");
                prefs_put2("MoviePlayerArgs", args);
            }
        }
    }
}

bool patch_setup(Config* cf) {
    DWORD attrs;
    DWORD oldattrs;
    cf->reduced_mode = strcmp((const char*)0x668165, "prax") == 0;

    if (cf->smooth_scrolling && cf->reduced_mode) {
        cf->smooth_scrolling = 0; // Feature not supported
    }
    init_video_config(cf);
    debug("patch_setup screen: %dx%d window: %dx%d\n",
        cf->screen_width, cf->screen_height, cf->window_width, cf->window_height);

    if (!VirtualProtect(AC_IMPORT_BASE, AC_IMPORT_LEN, PAGE_EXECUTE_READWRITE, &oldattrs)) {
        return false;
    }
    *(int32_t*)PeekMessageImport = (int32_t)ModPeekMessage;
    *(int32_t*)RegisterClassImport = (int32_t)ModRegisterClassA;
    *(int32_t*)GetSystemMetricsImport = (int32_t)ModGetSystemMetrics;

    if (!VirtualProtect(AC_IMPORT_BASE, AC_IMPORT_LEN, oldattrs, &attrs)) {
        return false;
    }

    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READWRITE, &attrs)) {
        return false;
    }
    // Apply Scient's Patch changes first
    for (auto& item : AXPatchData) {
        uint8_t* addr = (uint8_t*)(item.address);
        if(*addr != item.old_byte && *addr != item.new_byte
        && (addr < AXSkipVerifyStart || addr > AXSkipVerifyEnd)) {
            exit_fail((int)addr);
        }
        *addr = item.new_byte;
    }
    extra_setup(cf);

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

    write_jump(0x4688E0, (int)MapWin_gen_overlays);
    write_jump(0x4E3EF0, (int)mod_whose_territory);
    write_jump(0x4E4020, (int)mod_best_specialist);
    write_jump(0x4E4430, (int)mod_cost_factor);
    write_jump(0x4E4AA0, (int)base_first);
    write_jump(0x4E6400, (int)morale_mod);
    write_jump(0x4E65C0, (int)breed_mod);
    write_jump(0x4E6740, (int)worm_mod);
    write_jump(0x4E80B0, (int)mod_base_yield);
    write_jump(0x4E9550, (int)mod_base_support);
    write_jump(0x4E9B70, (int)mod_base_nutrient);
    write_jump(0x4E9CB0, (int)mod_base_minerals);
    write_jump(0x4EB560, (int)mod_base_energy);
    write_jump(0x4EC3B0, (int)base_compute);
    write_jump(0x4F6510, (int)fac_maint);
    write_jump(0x500320, (int)drop_range);
    write_jump(0x501350, (int)mod_morale_alien);
    write_jump(0x501500, (int)psi_factor);
    write_jump(0x50C4B0, (int)steal_energy);
    write_jump(0x527290, (int)mod_faction_upkeep);
    write_jump(0x52AD30, (int)council_votes);
    write_jump(0x52AE20, (int)eligible);
    write_jump(0x5391C0, (int)net_treaty_on);
    write_jump(0x539230, (int)net_treaty_off);
    write_jump(0x5392A0, (int)net_set_treaty);
    write_jump(0x539380, (int)net_agenda_off);
    write_jump(0x5393F0, (int)net_set_agenda);
    write_jump(0x539460, (int)net_energy);
    write_jump(0x539510, (int)net_loan);
    write_jump(0x539580, (int)net_maps);
    write_jump(0x5395F0, (int)net_tech);
    write_jump(0x539660, (int)net_pact_ends);
    write_jump(0x539740, (int)net_cede_base);
    write_jump(0x5397B0, (int)net_double_cross);
    write_jump(0x539B70, (int)great_beelzebub);
    write_jump(0x539C00, (int)great_satan);
    write_jump(0x539D40, (int)aah_ooga);
    write_jump(0x539E40, (int)climactic_battle);
    write_jump(0x539EF0, (int)at_climax);
    write_jump(0x53A030, (int)cause_friction);
    write_jump(0x53A090, (int)get_mood);
    write_jump(0x53A100, (int)reputation);
    write_jump(0x53A150, (int)get_patience);
    write_jump(0x53A1C0, (int)energy_value);
    write_jump(0x55BB30, (int)set_treaty);
    write_jump(0x55BBA0, (int)set_agenda);
    write_jump(0x579A30, (int)add_goal);
    write_jump(0x579B70, (int)add_site);
    write_jump(0x579D80, (int)wipe_goals);
    write_jump(0x579F80, (int)want_monolith);
    write_jump(0x584D60, (int)tech_name);
    write_jump(0x584E40, (int)chas_name);
    write_jump(0x584F40, (int)weap_name);
    write_jump(0x585030, (int)arm_name);
    write_jump(0x585170, (int)read_basic_rules);
    write_jump(0x585E30, (int)read_tech);
    write_jump(0x586050, (int)read_faction2);
    write_jump(0x586090, (int)read_faction);
    write_jump(0x586F30, (int)read_factions);
    write_jump(0x587240, (int)read_units);
    write_jump(0x5873C0, (int)read_rules);
    write_jump(0x591040, (int)map_wipe);
    write_jump(0x591E50, (int)synch_bit);
    write_jump(0x592250, (int)say_loc);
    write_jump(0x592550, (int)find_landmark);
    write_jump(0x592600, (int)new_landmark);
    write_jump(0x592650, (int)valid_landmark);
    write_jump(0x5926F0, (int)kill_landmark);
    write_jump(0x59D980, (int)prefs_get2);
    write_jump(0x59DA20, (int)default_prefs);
    write_jump(0x59DAA0, (int)default_prefs2);
    write_jump(0x59DB20, (int)default_warn);
    write_jump(0x59DB30, (int)default_rules);
    write_jump(0x59DB40, (int)prefs_get);
    write_jump(0x59DBD0, (int)prefs_fac_load);
    write_jump(0x59DCF0, (int)prefs_load);
    write_jump(0x59E510, (int)prefs_put2);
    write_jump(0x59E530, (int)prefs_put);
    write_jump(0x59E5D0, (int)prefs_save);
    write_jump(0x59E980, (int)vulnerable);
    write_jump(0x59EE50, (int)corner_market);
    write_jump(0x59E950, (int)prefs_use);
    write_jump(0x5AC060, (int)is_objective);
    write_jump(0x5B4210, (int)social_calc);
    write_jump(0x5B44D0, (int)social_upkeep);
    write_jump(0x5B4550, (int)social_upheaval);
    write_jump(0x5B4730, (int)society_avail);
    write_jump(0x5B9C40, (int)say_tech);
    write_jump(0x5B9FE0, (int)tech_category);
    write_jump(0x5BF1F0, (int)has_abil);
    write_jump(0x5BF310, (int)X_pop2);
    write_jump(0x5C0DB0, (int)can_arty);
    write_jump(0x5C0E40, (int)mod_morale_veh);
    write_jump(0x5C1540, (int)veh_speed);
    write_jump(0x5C1C40, (int)mod_veh_jail);
    write_jump(0x5C1D20, (int)mod_veh_skip);
    write_jump(0x5C1D70, (int)mod_veh_wake);
    write_jump(0x626250, (int)log_say2);
    write_jump(0x6262F0, (int)log_say);
    write_jump(0x626350, (int)log_say_hex2);
    write_jump(0x6263F0, (int)log_say_hex);
    write_jump(0x645460, (int)limit_strcpy);

    remove_call(0x415F69); // base_doctors
    remove_call(0x41608E); // base_doctors
    remove_call(0x419FB2); // base_doctors
    remove_call(0x4F7B42); // base_doctors

    write_call(0x58D84C, (int)mod_load_map_daemon); // map_menu
    write_call(0x5AB891, (int)mod_load_map_daemon); // load_map
    write_call(0x5AAD7D, (int)mod_load_daemon); // load_game
    write_call(0x5ABEB3, (int)mod_load_daemon); // load_undo
    write_call(0x5ADCD7, (int)mod_load_daemon); // show_replay
    write_call(0x4E1061, (int)mod_world_build); // Console::editor_generate
    write_call(0x4E113B, (int)mod_world_build); // Console::editor_fast
    write_call(0x58B9BF, (int)mod_world_build); // config_game
    write_call(0x58DDD8, (int)mod_world_build); // multiplayer_init
    write_call(0x525CC7, (int)mod_setup_player); // turn_upkeep
    write_call(0x5A3C9B, (int)mod_setup_player); // probe
    write_call(0x5B341C, (int)mod_setup_player); // eliminate_player
    write_call(0x5B3C03, (int)mod_setup_player); // setup_game
    write_call(0x5B3C4C, (int)mod_setup_player); // setup_game
    write_call(0x5B41E9, (int)mod_time_warp);  // setup_game
    write_call(0x52768A, (int)mod_turn_upkeep); // control_turn
    write_call(0x52A4AD, (int)mod_turn_upkeep); // net_control_turn
    write_call(0x527039, (int)mod_base_upkeep);
    write_call(0x4F7A38, (int)mod_base_hurry);
    write_call(0x579362, (int)mod_enemy_move);
    write_call(0x40F45A, (int)mod_base_draw);
    write_call(0x4672A7, (int)mod_base_draw);
    write_call(0x4F2A4C, (int)base_production_popp); // #PRODUCE
    write_call(0x522544, (int)alien_fauna_pop2); // #KELPGROWS
    write_call(0x522555, (int)alien_fauna_pop2); // #FORESTGROWS
    write_call(0x559E21, (int)map_draw_strcmp); // veh_draw
    write_call(0x55B5E1, (int)map_draw_strcmp); // base_draw
    write_call(0x5C0984, (int)veh_kill_lift); // veh_kill
    write_call(0x43FE47, (int)DiploPop_spying); // DiploPop::draw_info
    write_call(0x43FEA8, (int)DiploPop_spying); // DiploPop::draw_info
    write_call(0x4B72C0, (int)elev_at); // StatusWin::draw_status
    write_call(0x57C419, (int)alt_set_both); // goody_box
    write_call(0x5C2073, (int)alt_set_both); // world_alt_set
    write_call(0x5C2148, (int)alt_set_both); // world_alt_set
    write_call(0x5C22C3, (int)alt_set_both); // world_alt_set
    write_call(0x403BD4, (int)mod_amovie_project); // amovie_project2
    write_call(0x4F2B4B, (int)mod_amovie_project); // base_production
    write_call(0x524D06, (int)mod_amovie_project); // end_of_game
    write_call(0x524D28, (int)mod_amovie_project); // end_of_game
    write_call(0x5253F5, (int)mod_amovie_project); // end_of_game
    write_call(0x525407, (int)mod_amovie_project); // end_of_game
    write_call(0x52AB6D, (int)mod_amovie_project); // control_game
    write_call(0x5B3681, (int)mod_amovie_project); // eliminate_player
    write_call(0x445846, (int)load_music_strcmpi);
    write_call(0x445898, (int)load_music_strcmpi);
    write_call(0x4458EE, (int)load_music_strcmpi);
    write_call(0x445956, (int)load_music_strcmpi);
    write_call(0x4459C1, (int)load_music_strcmpi);
    write_call(0x445A2F, (int)load_music_strcmpi);
    write_call(0x445AB2, (int)load_music_strcmpi);
    write_call(0x498720, (int)ReportWin_close_handler);
    write_call(0x408DBD, (int)BaseWin_draw_psych_strcat);
    write_call(0x40F8F8, (int)BaseWin_draw_farm_set_font);
    write_call(0x4129E5, (int)BaseWin_draw_energy_set_text_color);
    write_call(0x415AD8, (int)BaseWin_draw_misc_eco_damage);
    write_call(0x41916B, (int)BaseWin_popup_start);
    write_call(0x4195A6, (int)BaseWin_ask_number);
    write_call(0x41B6E5, (int)BaseWin_staple_popp);
    write_call(0x41B719, (int)BaseWin_staple_popp);
    write_call(0x41B771, (int)BaseWin_action_staple);
    write_call(0x41D99D, (int)BaseWin_click_staple);
    write_call(0x41D75C, (int)BaseWin_gov_options);
    write_call(0x497E05, (int)BaseWin_gov_options);
    write_call(0x4A664E, (int)BaseWin_gov_options);
    write_call(0x516E50, (int)BaseWin_gov_options);
    write_call(0x48CDA4, (int)popb_action_staple);
    write_call(0x4936F4, (int)ProdPicker_calculate_itoa);
    write_call(0x4AED04, (int)SocialWin_social_ai);
    write_call(0x51D1C2, (int)Console_editor_fungus);
    write_call(0x54814D, (int)mod_diplomacy_caption);
    write_call(0x54F7D7, (int)mod_energy_trade);
    write_call(0x54F77E, (int)mod_base_swap);
    write_call(0x542278, (int)mod_buy_tech);
    write_call(0x542425, (int)mod_buy_tech);
    write_call(0x5425CF, (int)mod_buy_tech);
    write_call(0x543843, (int)mod_buy_tech);
    write_call(0x54F4E2, (int)mod_threaten);
    write_call(0x54F532, (int)mod_threaten);
    write_call(0x54F702, (int)mod_threaten);
    write_call(0x5A3F7D, (int)probe_veh_health);
    write_call(0x5A3F98, (int)probe_veh_health);
    write_call(0x5A4972, (int)probe_mind_control_range);
    write_call(0x5A4B8C, (int)probe_thought_control);
    write_call(0x561948, (int)enemy_strategy_upgrade);
    write_call(0x5BBEB0, (int)tech_achieved_pop3);
    write_call(0x4868B2, (int)mod_tech_avail); // PickTech::pick
    write_call(0x4DFC41, (int)mod_tech_avail); // Console::editor_tech
    write_call(0x558246, (int)mod_tech_avail); // communicate
    write_call(0x57C0CC, (int)mod_tech_avail); // goody_box
    write_call(0x57D130, (int)mod_tech_avail); // study_artifact
    write_call(0x5BDC38, (int)mod_tech_avail); // tech_ai
    write_call(0x486A1B, (int)mod_tech_val); // PickTech::pick
    write_call(0x53E9B4, (int)mod_tech_val); // tech_analysis
    write_call(0x53EA56, (int)mod_tech_val); // tech_analysis
    write_call(0x53F34B, (int)mod_tech_val); // buy_council_vote
    write_call(0x53F394, (int)mod_tech_val); // buy_council_vote
    write_call(0x53F3C9, (int)mod_tech_val); // buy_council_vote
    write_call(0x53F3FE, (int)mod_tech_val); // buy_council_vote
    write_call(0x540250, (int)mod_tech_val); // buy_tech
    write_call(0x54099D, (int)mod_tech_val); // energy_trade
    write_call(0x5409C9, (int)mod_tech_val); // energy_trade
    write_call(0x541D58, (int)mod_tech_val); // tech_trade
    write_call(0x541D69, (int)mod_tech_val); // tech_trade
    write_call(0x544861, (int)mod_tech_val); // propose_pact
    write_call(0x54547F, (int)mod_tech_val); // propose_treaty
    write_call(0x54561B, (int)mod_tech_val); // propose_treaty
    write_call(0x5457B7, (int)mod_tech_val); // propose_treaty
    write_call(0x546A35, (int)mod_tech_val); // propose_attack
    write_call(0x546A63, (int)mod_tech_val); // propose_attack
    write_call(0x546A94, (int)mod_tech_val); // propose_attack
    write_call(0x546AC5, (int)mod_tech_val); // propose_attack
    write_call(0x5489A4, (int)mod_tech_val); // make_gift
    write_call(0x548AFD, (int)mod_tech_val); // make_gift
    write_call(0x54D912, (int)mod_tech_val); // base_swap
    write_call(0x552DE4, (int)mod_tech_val); // communicate
    write_call(0x5540A5, (int)mod_tech_val); // communicate
    write_call(0x554644, (int)mod_tech_val); // communicate
    write_call(0x5548C0, (int)mod_tech_val); // communicate
    write_call(0x554950, (int)mod_tech_val); // communicate
    write_call(0x554974, (int)mod_tech_val); // communicate
    write_call(0x55515E, (int)mod_tech_val); // communicate
    write_call(0x556565, (int)mod_tech_val); // communicate
    write_call(0x5582AC, (int)mod_tech_val); // communicate
    write_call(0x55D521, (int)mod_tech_val); // enemies_trade_tech
    write_call(0x55D622, (int)mod_tech_val); // enemies_trade_tech
    write_call(0x5BDC4C, (int)mod_tech_val); // tech_ai
    write_call(0x486655, (int)mod_tech_ai); // PickTech::pick
    write_call(0x5AEFA1, (int)mod_tech_ai); // time_warp
    write_call(0x5B2A6F, (int)mod_tech_ai); // setup_player
    write_call(0x50C398, (int)mod_tech_pick); // steal_tech
    write_call(0x515080, (int)mod_tech_pick); // Console::set_research
    write_call(0x5BE4AC, (int)mod_tech_pick); // tech_selection
    write_call(0x4452D5, (int)mod_tech_rate); // energy_compute
    write_call(0x498D26, (int)mod_tech_rate); // ReportWin::draw_labs
    write_call(0x4A77DA, (int)mod_tech_rate); // ReportIF::draw_labs
    write_call(0x521872, (int)mod_tech_rate); // random_events
    write_call(0x5218BE, (int)mod_tech_rate); // random_events
    write_call(0x5581E9, (int)mod_tech_rate); // communicate
    write_call(0x5BEA4D, (int)mod_tech_rate); // tech_research
    write_call(0x5BEAC7, (int)mod_tech_rate); // tech_research
    write_call(0x4B497C, (int)mod_say_orders); // say_orders2
    write_call(0x4B5C27, (int)mod_say_orders); // StatusWin::draw_active
    write_call(0x4F7B82, (int)mod_base_research); // base_upkeep
    write_call(0x4CCF13, (int)mod_capture_base); // action_airdrop
    write_call(0x598778, (int)mod_capture_base); // order_veh
    write_call(0x5A4AB0, (int)mod_capture_base); // probe
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
    write_call(0x415F35, (int)mod_base_reset); // BaseWin::unk4
    write_call(0x41605A, (int)mod_base_reset); // BaseWin::gov_on
    write_call(0x417F83, (int)mod_base_reset); // BaseWin::production
    write_call(0x419F7C, (int)mod_base_reset); // BaseWin::gov_options
    write_call(0x4E63CD, (int)mod_base_reset); // bases_reset
    write_call(0x4F098F, (int)mod_base_reset); // base_production
    write_call(0x4F154C, (int)mod_base_reset); // base_production
    write_call(0x4F184C, (int)mod_base_reset); // base_production
    write_call(0x4F1BA7, (int)mod_base_reset); // base_production
    write_call(0x4F1CBB, (int)mod_base_reset); // base_production
    write_call(0x4F3404, (int)mod_base_reset); // base_production
    write_call(0x4F3922, (int)mod_base_reset); // base_production
    write_call(0x4F5109, (int)mod_base_reset); // drone_riot
    write_call(0x4F5F04, (int)mod_base_reset); // base_drones
    write_call(0x508B3F, (int)mod_base_reset); // battle_fight_2
    write_call(0x50AE18, (int)mod_base_reset); // battle_fight_2
    write_call(0x50CCA8, (int)mod_base_reset); // capture_base
    write_call(0x561607, (int)mod_base_reset); // enemy_strategy
    write_call(0x564850, (int)mod_base_reset); // enemy_strategy
    write_call(0x5B01C7, (int)mod_base_reset); // time_warp
    write_call(0x4E4C2F, (int)mod_replay_base); // base_init
    write_call(0x4E5378, (int)mod_replay_base); // base_kill
    write_call(0x4F579E, (int)mod_replay_base); // drone_riot
    write_call(0x50CF02, (int)mod_replay_base); // capture_base
    write_call(0x54D313, (int)mod_replay_base); // give_a_base
    write_call(0x41B8BF, (int)mod_facility_avail); // BaseWin::base_editor_fac
    write_call(0x41CC07, (int)mod_facility_avail); // BaseWin::base_editor
    write_call(0x49357A, (int)mod_facility_avail); // ProdPicker::calculate
    write_call(0x4F077E, (int)mod_facility_avail); // base_queue
    write_call(0x4FD920, (int)mod_facility_avail); // base_build
    write_call(0x4FF2D4, (int)mod_facility_avail); // base_build
    write_call(0x4FD672, (int)mod_base_lose_minerals); // base_build
    write_call(0x4FEF55, (int)mod_base_lose_minerals); // base_build
    write_call(0x4FF909, (int)mod_base_lose_minerals); // base_build
    write_call(0x4179A8, (int)mod_base_making); // BaseWin::production
    write_call(0x4179BC, (int)mod_base_making); // BaseWin::production
    write_call(0x417A2E, (int)mod_base_making); // BaseWin::production
    write_call(0x417A3F, (int)mod_base_making); // BaseWin::production
    write_call(0x4932D3, (int)mod_base_making); // ProdPicker::calculate
    write_call(0x4932E5, (int)mod_base_making); // ProdPicker::calculate
    write_call(0x49365A, (int)mod_base_making); // ProdPicker::calculate
    write_call(0x49366B, (int)mod_base_making); // ProdPicker::calculate
    write_call(0x4E5B06, (int)mod_base_making); // base_change
    write_call(0x4E5B1B, (int)mod_base_making); // base_change
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

    write_call(0x465DD6, (int)mod_crop_yield); // MapWin::gen_terrain_poly
    write_call(0x4B6C44, (int)mod_crop_yield); // StatusWin::draw_status
    write_call(0x4BCEEB, (int)mod_crop_yield); // TutWin::tour
    write_call(0x4E7DE4, (int)mod_crop_yield); // resource_yield
    write_call(0x4E888C, (int)mod_crop_yield); // base_yield
    write_call(0x4E96F4, (int)mod_crop_yield); // base_support
    write_call(0x4ED7F1, (int)mod_crop_yield); // base_terraform
    write_call(0x565878, (int)mod_crop_yield); // can_terraform
    write_call(0x4B6E4F, (int)mod_mine_yield); // StatusWin::draw_status
    write_call(0x4B6EF9, (int)mod_mine_yield); // StatusWin::draw_status
    write_call(0x4B6F84, (int)mod_mine_yield); // StatusWin::draw_status
    write_call(0x4E7E00, (int)mod_mine_yield); // resource_yield
    write_call(0x4E88AC, (int)mod_mine_yield); // base_yield
    write_call(0x4E970A, (int)mod_mine_yield); // base_support
    write_call(0x4B7028, (int)mod_energy_yield); // StatusWin::draw_status
    write_call(0x4B7136, (int)mod_energy_yield); // StatusWin::draw_status
    write_call(0x4D3E5C, (int)mod_energy_yield); // Console::terraform
    write_call(0x4E7E1C, (int)mod_energy_yield); // resource_yield
    write_call(0x4E88CA, (int)mod_energy_yield); // base_yield
    write_call(0x4E971F, (int)mod_energy_yield); // base_support
    write_call(0x56C856, (int)mod_energy_yield); // enemy_move
    write_call(0x46DB16, (int)mod_base_find3); // MapWin::click
    write_call(0x4C94B8, (int)mod_base_find3); // terraform_cost
    write_call(0x4CB104, (int)mod_base_find3); // action_destroy
    write_call(0x4CB1F4, (int)mod_base_find3); // action_destroy
    write_call(0x4CCC56, (int)mod_base_find3); // action_airdrop
    write_call(0x4E3F8C, (int)mod_base_find3); // whose_territory
    write_call(0x5224A0, (int)mod_base_find3); // alien_fauna
    write_call(0x52293A, (int)mod_base_find3); // alien_fauna
    write_call(0x523ED7, (int)mod_base_find3); // reset_territory
    write_call(0x52417F, (int)mod_base_find3); // reset_territory
    write_call(0x54AEA1, (int)mod_base_find3); // suggest_plan
    write_call(0x54AF20, (int)mod_base_find3); // suggest_plan
    write_call(0x563745, (int)mod_base_find3); // enemy_strategy
    write_call(0x56B8F1, (int)mod_base_find3); // enemy_move
    write_call(0x56B924, (int)mod_base_find3); // enemy_move
    write_call(0x56E460, (int)mod_base_find3); // enemy_move
    write_call(0x570C4B, (int)mod_base_find3); // enemy_move
    write_call(0x5B19B3, (int)mod_base_find3); // setup_player
    write_call(0x5C0609, (int)mod_base_find3); // veh_init
    write_call(0x467711, (int)mod_hex_cost); // MapWin::dest_line
    write_call(0x572518, (int)mod_hex_cost); // enemy_move
    write_call(0x5772D7, (int)mod_hex_cost); // enemy_move
    write_call(0x5776F4, (int)mod_hex_cost); // enemy_move
    write_call(0x577E2C, (int)mod_hex_cost); // enemy_move
    write_call(0x577F0E, (int)mod_hex_cost); // enemy_move
    write_call(0x597695, (int)mod_hex_cost); // order_veh
    write_call(0x59ACA4, (int)mod_hex_cost); // Path::find
    write_call(0x59B61A, (int)mod_hex_cost); // Path::find
    write_call(0x59C105, (int)mod_hex_cost); // Path::move

    // Prototypes and combat game mechanics
    write_call(0x4B43D0, (int)mod_say_morale2); // say_morale
    write_call(0x4B51BE, (int)mod_say_morale2); // StatusWin::draw_active
    write_call(0x50211F, (int)mod_get_basic_offense); // battle_compute
    write_call(0x50274A, (int)mod_get_basic_offense); // battle_compute
    write_call(0x5044EB, (int)mod_get_basic_offense); // battle_compute
    write_call(0x502A69, (int)mod_get_basic_defense); // battle_compute
    write_call(0x506D07, (int)mod_best_defender); // battle_fight_2
    write_call(0x50474C, (int)mod_battle_compute); // best_defender
    write_call(0x506EA6, (int)mod_battle_compute); // battle_fight_2
    write_call(0x5085E0, (int)mod_battle_compute); // battle_fight_2
    write_call(0x506ADE, (int)mod_battle_fight_2); // battle_fight_1
    write_call(0x568B1C, (int)mod_battle_fight_2); // air_power
    write_call(0x5697AC, (int)mod_battle_fight_2); // air_power
    write_call(0x56A2E2, (int)mod_battle_fight_2); // air_power
    write_call(0x4CACFD, (int)mod_battle_fight); // action_destroy
    write_call(0x567234, (int)mod_battle_fight); // alien_move
    write_call(0x436796, (int)transport_val); // DesignWin::draw_unit_preview
    write_call(0x5A5F3D, (int)transport_val); // make_proto
    write_call(0x4930F8, (int)mod_veh_avail); // ProdPicker::calculate
    write_call(0x4DED97, (int)mod_veh_avail); // Console::editor_veh
    write_call(0x4E4AE4, (int)mod_veh_avail); // base_first
    write_call(0x4F0769, (int)mod_veh_avail); // base_queue
    write_call(0x4FA04C, (int)mod_veh_avail); // base_build
    write_call(0x4FFD37, (int)mod_veh_avail); // base_build
    write_call(0x4FFF5D, (int)mod_veh_avail); // base_build
    write_call(0x560DDF, (int)mod_veh_avail); // enemy_capabilities
    write_call(0x560E86, (int)mod_veh_avail); // enemy_capabilities
    write_call(0x5B30E1, (int)mod_veh_avail); // setup_player
    write_call(0x5808B3, (int)mod_is_bunged); // propose_proto
    write_call(0x438354, (int)mod_make_proto); // DesignWin::design_it
    write_call(0x580F6C, (int)mod_make_proto); // propose_proto
    write_call(0x587364, (int)mod_make_proto); // read_units
    write_call(0x5B2FED, (int)mod_make_proto); // setup_player
    write_call(0x4364FB, (int)mod_name_proto); // DesignWin::draw_unit_preview
    write_call(0x4383B2, (int)mod_name_proto); // DesignWin::design_it
    write_call(0x4383DE, (int)mod_name_proto); // DesignWin::design_it
    write_call(0x43BE8F, (int)mod_name_proto); // DesignWin::select_name
    write_call(0x43E06E, (int)mod_name_proto); // DesignWin::draw_flash
    write_call(0x43E663, (int)mod_name_proto); // DesignWin::setup_veh
    write_call(0x581044, (int)mod_name_proto); // propose_proto
    write_call(0x5B301E, (int)mod_name_proto); // setup_player
    write_call(0x438DEF, (int)mod_upgrade_prototype); // DesignWin::on_button_clicked
    write_call(0x4F061B, (int)mod_upgrade_prototype); // upgrade_prototypes
    write_call(0x5843F5, (int)mod_upgrade_prototype); // design_workshop
    write_call(0x4F0447, (int)full_upgrade); // upgrade_prototype
    write_call(0x536C34, (int)full_upgrade); // NetDaemon::process_message
    write_call(0x4D0ECF, (int)mod_upgrade_cost); // Console::upgrade
    write_call(0x4D16D9, (int)mod_upgrade_cost); // Console::upgrade
    write_call(0x4EFB76, (int)mod_upgrade_cost); // do_upgrade
    write_call(0x4EFEB9, (int)mod_upgrade_cost); // upgrade_prototype
    write_call(0x42275A, (int)mod_offense_proto); // BattleWin::battle_report
    write_call(0x5018AE, (int)mod_offense_proto); // get_basic_offense
    write_call(0x569FF3, (int)mod_offense_proto); // air_power
    write_call(0x57D5AD, (int)mod_offense_proto); // say_offense
    write_call(0x4226D5, (int)mod_armor_proto); // BattleWin::battle_report
    write_call(0x501C95, (int)mod_armor_proto); // get_basic_defense
    write_call(0x57D703, (int)mod_armor_proto); // say_defense
    write_call(0x436ADD, (int)mod_proto_cost); // DesignWin::draw_unit_preview
    write_call(0x43704C, (int)mod_proto_cost); // DesignWin::draw_unit_preview
    write_call(0x5817C9, (int)mod_proto_cost); // consider_designs
    write_call(0x581833, (int)mod_proto_cost); // consider_designs
    write_call(0x581BB3, (int)mod_proto_cost); // consider_designs
    write_call(0x581BCB, (int)mod_proto_cost); // consider_designs
    write_call(0x582339, (int)mod_proto_cost); // consider_designs
    write_call(0x582359, (int)mod_proto_cost); // consider_designs
    write_call(0x582378, (int)mod_proto_cost); // consider_designs
    write_call(0x582398, (int)mod_proto_cost); // consider_designs
    write_call(0x5823B0, (int)mod_proto_cost); // consider_designs
    write_call(0x582482, (int)mod_proto_cost); // consider_designs
    write_call(0x58249A, (int)mod_proto_cost); // consider_designs
    write_call(0x58254A, (int)mod_proto_cost); // consider_designs
    write_call(0x5827E4, (int)mod_proto_cost); // consider_designs
    write_call(0x582EC5, (int)mod_proto_cost); // consider_designs
    write_call(0x582FEC, (int)mod_proto_cost); // consider_designs
    write_call(0x5A5D35, (int)mod_proto_cost); // base_cost
    write_call(0x5A5F15, (int)mod_proto_cost); // make_proto
    write_call(0x40E666, (int)mod_veh_cost); // BaseWin::draw_production
    write_call(0x40E9C6, (int)mod_veh_cost); // BaseWin::draw_production
    write_call(0x418F75, (int)mod_veh_cost); // BaseWin::hurry
    write_call(0x492D90, (int)mod_veh_cost); // ProdPicker::unk3
    write_call(0x493250, (int)mod_veh_cost); // ProdPicker::calculate
    write_call(0x49E097, (int)mod_veh_cost); // ReportWin::draw_ops
    write_call(0x4F0B5E, (int)mod_veh_cost); // base_production
    write_call(0x4F15DF, (int)mod_veh_cost); // base_production
    write_call(0x4F4061, (int)mod_veh_cost); // base_hurry
    write_call(0x4FA2AB, (int)mod_veh_cost); // base_build
    write_call(0x4FA510, (int)mod_veh_cost); // base_build
    write_call(0x4FD6B5, (int)mod_veh_cost); // base_build
    write_call(0x56D38D, (int)mod_veh_cost); // enemy_move
    write_call(0x57AB53, (int)mod_veh_cost); // goody_box
    write_call(0x593D35, (int)mod_veh_cost); // supply_options
    write_call(0x593F72, (int)mod_veh_cost); // supply_options
    write_call(0x594B8E, (int)mod_veh_cost); // order_veh
    write_call(0x59EEA1, (int)mod_mind_control); // corner_market
    write_call(0x5A20E8, (int)mod_mind_control); // probe
    write_call(0x59FE9D, (int)mod_success_rates); // probe
    write_call(0x59FEBB, (int)mod_success_rates); // probe
    write_call(0x5A00EE, (int)mod_success_rates); // probe
    write_call(0x5A010C, (int)mod_success_rates); // probe
    write_call(0x5A03AD, (int)mod_success_rates); // probe
    write_call(0x5A03C8, (int)mod_success_rates); // probe
    write_call(0x5A077D, (int)mod_success_rates); // probe
    write_call(0x5A07C5, (int)mod_success_rates); // probe
    write_call(0x5A0BB1, (int)mod_success_rates); // probe
    write_call(0x5A0CE8, (int)mod_success_rates); // probe
    write_call(0x5A0F08, (int)mod_success_rates); // probe
    write_call(0x5A10BE, (int)mod_success_rates); // probe
    write_call(0x5A1EAC, (int)mod_success_rates); // probe
    write_call(0x5A1EC8, (int)mod_success_rates); // probe
    write_call(0x5A2B45, (int)mod_success_rates); // probe
    write_call(0x5A2B64, (int)mod_success_rates); // probe
    write_call(0x5A2F4B, (int)mod_success_rates); // probe
    write_call(0x5A2F6C, (int)mod_success_rates); // probe

    // Redirect functions for foreign_treaty_popup option
    write_call(0x55DC00, (int)mod_NetMsg_pop); // enemies_treaty
    write_call(0x55DF04, (int)mod_NetMsg_pop); // enemies_treaty
    write_call(0x55E364, (int)mod_NetMsg_pop); // enemies_treaty
    write_call(0x55CB33, (int)mod_NetMsg_pop); // enemies_war
    write_call(0x597141, (int)mod_NetMsg_pop); // order_veh

    // Redirect popup dialog entries
    write_call(0x4063CB, (int)mod_BasePop_start); // Popup::start
    write_call(0x40649A, (int)mod_BasePop_start); // Popup::start
    write_call(0x4064BE, (int)mod_BasePop_start); // Popup::start
    write_call(0x4C90EB, (int)mod_BasePop_start); // sub_4C9080
    write_call(0x4E27F6, (int)mod_BasePop_start); // AlphaNet::pick_service
    write_call(0x60053A, (int)mod_BasePop_start); // filefind_init
    write_call(0x6276DB, (int)mod_BasePop_start); // pops
    write_call(0x62794B, (int)mod_BasePop_start); // pop_ask
    write_call(0x627C90, (int)mod_BasePop_start); // pop_ask_number

    if (!cf->reduced_mode) {
        write_call(0x62D3EC, (int)mod_Win_init_class);
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
        memset((void*)0x58B63C, 0x90, 10); // config_game
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
        }
    }
    /*
    Provide better defaults for multiplayer settings.
    */
    write_byte(0x481EE0, 2, 1); // MapOceanCoverage
    write_byte(0x481EE7, 2, 1); // MapErosiveForces
    write_byte(0x481EEE, 2, 1); // MapNativeLifeForms
    write_byte(0x481EF5, 2, 1); // MapCloudCover
    write_byte(0x4E3222, 9, NetVersion); // AlphaNet::setup
    write_byte(0x52AA5C, 9, NetVersion); // control_game
    write_byte(0x627C8B, 9, 11); // pop_ask_number maximum length
    /*
    Allow custom map sizes up to 512x512. Warning dialog will be displayed if size exceeds 256x256.
    */
    write_word(0x58D3A2, 256, 512); // size_of_planet
    write_word(0x58D3A9, 256, 512); // size_of_planet
    write_word(0x58D3BB, 256, 512); // size_of_planet
    write_word(0x58D3C2, 256, 512); // size_of_planet
    write_word(0x58D3CF, 128, 256); // size_of_planet
    write_word(0x58D3D7, 128, 256); // size_of_planet

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
    if (FileExists(MovlistTxtFile) && !FileExists(ExpMovlistTxtFile)) {
        CopyFile(MovlistTxtFile, ExpMovlistTxtFile, TRUE);
    }

    /*
    Patch base window bug where the population row would display superdrones
    even though they would be suppressed by psych-related effects.
    Also initialize Random::Random with base-specific constant instead of timeGetTime.
    */
    {
        write_call(0x414B81, (int)BaseWin_random_seed); // BaseWin_draw_pop
        write_call(0x408786, (int)BaseWin_random_seed); // BaseWin_psych_row
        write_call(0x49D57B, (int)BaseWin_random_seed); // ReportWin_draw_ops
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
    Fix visual issues where the game sometimes did not update the map properly
    when recentering it offscreen on native resolution mode.
    */
    {
        write_call(0x4BE697, (int)mod_MapWin_focus); // TutWin::tut_map
        write_call(0x51094C, (int)mod_MapWin_focus); // Console::focus
        write_call(0x51096D, (int)mod_MapWin_focus); // Console::focus
        write_call(0x510C81, (int)mod_MapWin_focus); // sub_510B70
        write_call(0x51A7B4, (int)mod_MapWin_focus); // Console::on_key_click
        write_call(0x51B594, (int)mod_MapWin_focus); // Console::on_key_click
        write_call(0x51C7BC, (int)mod_MapWin_focus); // Console::iface_click
        write_call(0x522504, (int)mod_MapWin_focus); // alien_fauna
        write_call(0x5583ED, (int)mod_MapWin_focus); // communicate
        write_call(0x46E4A1, (int)mod_MapWin_set_center); // MapWin::click
        write_call(0x46E7CF, (int)mod_MapWin_set_center); // MapWin::click
        write_call(0x46E7FA, (int)mod_MapWin_set_center); // MapWin::click
        write_call(0x46E8CA, (int)mod_MapWin_set_center); // MapWin::click
        write_call(0x518285, (int)mod_MapWin_set_center); // Console::on_key_click
        write_call(0x51B19F, (int)mod_MapWin_set_center); // Console::on_key_click
        write_call(0x51C6B4, (int)mod_MapWin_set_center); // Console::iface_click
    }

    /*
    Fix rendering bug that caused half of the bottom row map tiles to shift
    to the wrong side of screen when zoomed out in MapWin_tile_to_pixel.
    */
    {
        if (*(uint16_t*)0x462F2E == 0x657D) {
            *(uint16_t*)0x462F2E = 0x657F;
        } else {
            assert(0);
        }
    }
    if (cf->render_high_detail) {
        // Patch map window to render more detailed tiles when zoomed out.
        // 83 7D 10 F8      cmp     [ebp+zoom], 0FFFFFFF8h
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
        // Restore the planet preview background on game setup screen for all resolutions
        short_jump(0x4AE66C); // SetupWin_draw_item resolution checks
        write_call(0x4AE6D5, (int)SetupWin_buffer_draw);
        write_call(0x4AE710, (int)SetupWin_buffer_copy);
        write_call(0x4AE73B, (int)SetupWin_soft_update3);
        // Draw zoomable support view on the base window
        write_call(0x40F1C3, (int)BaseWin_draw_support); // BaseWin::draw_farm
        write_call(0x40F1EB, (int)BaseWin_draw_support); // BaseWin::draw_farm
        // Update interlude and credits backgrounds
        write_call(0x45F3C2, (int)window_scale_load_pcx); // Interlude::exec
        write_call(0x5D552E, (int)window_scale_load_pcx); // GraphicWin::load_pcx
        write_call(0x428A40, (int)Credits_GraphicWin_init); // Credits::exec
        write_call(0x428AB9, (int)Credits_GraphicWin_init); // Credits::exec
    }
    if (cf->render_base_info) {
        short_jump(0x49DE33);
        write_call(0x49DE83, (int)ReportWin_draw_ops_color);
        write_call(0x49DEA5, (int)ReportWin_draw_ops_strcat);
    }
    if (cf->render_probe_labels) {
        memset((void*)0x559590, 0x90, 2);
        memset((void*)0x5599DE, 0x90, 6);
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
    Fix diplomacy dialog appearing multiple times when both human and alien factions are
    involved in a base capture by removing the event that spawns additional colony pods.
    */
    {
        long_jump(0x50D67A);
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
        long_jump(0x5414AA);
    }

    /*
    Patch Stockpile Energy when it enabled double production if the base produces one item
    and then switches to Stockpile Energy on the same turn gaining additional credits.
    */
    {
        short_jump(0x4EC33D); // base_energy skip stockpile energy
        write_call(0x4F7A2F, (int)mod_base_production); // base_upkeep
    }

    /*
    Fix datalinks window not showing the first character for Sea Formers units.
    */
    {
        short_jump(0x42A651); // Datalink::set_cat_unit
        short_jump(0x42CB01); // Datalink::draw_unit
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
        write_call(0x4CFEA4, (int)mod_psych_check);
    }

    /*
    Modify planetpearls income after wiping out any planet-owned units.
    */
    {
        const byte old_bytes[] = {0x40,0x8D,0x0C,0x80,0x8B,0x06,0xD1,0xE1,0x03,0xC1,0x89,0x06};
        const byte new_bytes[] = {0x01,0x06,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        write_bytes(0x5060FD, old_bytes, new_bytes, sizeof(old_bytes));
        write_call(0x5060F5, (int)battle_kill_credits);
    }

    if (cf->smac_only) {
        if (!FileExists(ModAlphaTxtFile)
        || !FileExists(ModHelpTxtFile)
        || !FileExists(ModTutorTxtFile)
        || !FileExists(ModConceptsTxtFile)) {
            MessageBoxA(0, "Error while opening smac_mod folder. Unable to start smac_only mode.",
                MOD_VERSION, MB_OK | MB_ICONSTOP);
            exit_fail();
        }
        *(int32_t*)0x45F97A = 0;
        *(const char**)0x691AFC = ModAlphaFile;
        *(const char**)0x691B00 = ModHelpFile;
        *(const char**)0x691B14 = ModTutorFile;
        write_offset(0x42B30E, ModConceptsFile);
        write_offset(0x42B49E, ModConceptsFile);
        write_offset(0x42C450, ModConceptsFile);
        write_offset(0x42C7C2, ModConceptsFile);
        write_offset(0x403BA8, MovlistFile);
        write_offset(0x4BEF8D, MovlistFile);
        write_offset(0x52AB68, OpeningFile);
        // Enable custom faction selection during the game setup / config_game
        memset((void*)0x58A5E1, 0x90, 6);
        memset((void*)0x58B76F, 0x90, 2);
        memset((void*)0x58B9F3, 0x90, 2);
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
        long_jump(0x589097);
    }
    if (cf->alien_early_start || cf->cult_early_start) {
        // Use default starting year instead of advancing by 5 turns
        const byte old_bytes[] = {0xC7,0x05,0xD4,0x64,0x9A,0x00,0x05,0x00,0x00,0x00};
        write_bytes(0x5AE3C3, old_bytes, NULL, sizeof(old_bytes));
        write_bytes(0x5AE3E9, old_bytes, NULL, sizeof(old_bytes));
    }
    if (cf->ignore_reactor_power) {
        short_jump(0x506F90); // battle_fight_2
        short_jump(0x506FF6); // battle_fight_2
        // Adjust combat odds confirmation dialog to match new odds
        write_call(0x5082AF, (int)battle_fight_parse_num);
        write_call(0x5082B7, (int)battle_fight_parse_num);
    }
    if (cf->long_range_artillery > 0) {
        write_call(0x46D42F, (int)mod_action_move); // MapWin::right_menu
        write_call(0x46E1FF, (int)mod_action_move); // MapWin::click

        const byte old_cursor[] = {0x8B,0x0D,0x44,0x97,0x94,0x00,0x51};
        const byte new_cursor[] = {0x57,0x90,0x90,0x90,0x90,0x90,0x90};
        write_bytes(0x4D927F, old_cursor, new_cursor, sizeof(new_cursor));
        write_call(0x4D928D, (int)Console_arty_cursor_on);

        const byte old_menu[] = {
            0x8B,0x45,0xF0,0x6A,0x01,0x8D,0x0C,0x40,0x8D,0x14,
            0x88,0x0F,0xBF,0x04,0x95,0x32,0x28,0x95,0x00,0x50,
            0xE8,0x82,0x43,0x15,0x00,0x83,0xC4,0x08,0x85,0xC0
        };
        const byte new_menu[] = {
            0x8B,0x45,0x0C,0x50,0x8B,0x45,0x08,0x50,0x8B,0x45,
            0xF0,0x50,0xE8,0x00,0x00,0x00,0x00,0x83,0xC4,0x0C,
            0x85,0xC0,0x75,0x6E,0xE9,0x8E,0x00,0x00,0x00,0x90
        };
        write_bytes(0x46CA15, old_menu, new_menu, sizeof(new_menu));
        write_call(0x46CA21, (int)MapWin_right_menu_arty);
    }
    if (cf->skip_default_balance) {
        remove_call(0x5B41F5);
    }
    if (cf->facility_capture_fix) {
        remove_call(0x50D06A);
        remove_call(0x50D074);
    }
    if (cf->auto_relocate_hq) {
        long_jump(0x50C99D); // Disable #ESCAPED event
    }
    if (cf->simple_hurry_cost) {
        short_jump(0x41900D);
    }
    if (cf->eco_damage_fix) {
        const byte old_bytes[] = {0x84,0x05,0xE8,0x64,0x9A,0x00,0x74,0x24};
        write_bytes(0x4EA0B9, old_bytes, NULL, sizeof(old_bytes));
        // Patch Tree Farms to always increase the clean minerals limit.
        const byte old_bytes_2[] = {0x85,0xC0,0x74,0x07};
        write_bytes(0x4F2AC6, old_bytes_2, NULL, sizeof(old_bytes_2));
    }
    if (!cf->spawn_fungal_towers) {
        // Spawn nothing in this case.
        remove_call(0x4F7143);
        remove_call(0x5C363F);
        remove_call(0x57BBC9);
    }
    if (!cf->spawn_spore_launchers) {
        // Patch the game to spawn Mind Worms instead.
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
    if (!cf->event_perihelion) {
        short_jump(0x51F481);
    }
    if (cf->event_sunspots > 0) {
        const byte old_bytes[] = {0x83,0xC0,0x0A,0x6A,0x14};
        const byte new_bytes[] = {0x83,0xC0,
            (byte)cf->event_sunspots,0x6A,(byte)(cf->event_sunspots)};
        write_call(0x52064C, (int)zero_value);
        write_bytes(0x520651, old_bytes, new_bytes, sizeof(new_bytes));
    } else if (!cf->event_sunspots) { // Remove event
        long_jump(0x520615);
    }
    if (cf->event_market_crash > 0) { // Reduce reserves only by 1/2 instead of 3/4
        const byte old_bytes[] = {0x99,0x83,0xE2,0x03,0x03,0xC2,0xC1,0xF8,0x02};
        const byte new_bytes[] = {0xD1,0xF8,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        write_bytes(0x520725, old_bytes, new_bytes, sizeof(new_bytes));
        write_offset(0x520751, ac_genwarning_sm_pcx);
        write_offset(0x520786, ac_genwarning_sm_pcx);
    } else if (!cf->event_market_crash) { // Remove event
        const byte old_bytes[] = {0x75,0x0C,0x81,0xFE,0xD0};
        const byte new_bytes[] = {0xE9,0x02,0x1A,0x00,0x00};
        write_bytes(0x52070F, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->alien_guaranteed_techs) {
        short_jump(0x5B29F8);
    }
    if (cf->native_weak_until_turn >= 0) {
        const byte old_bytes[] = {0x83, 0x3D, 0xD4, 0x64, 0x9A, 0x00, 0x0F};
        const byte new_bytes[] = {0x83, 0x3D, 0xD4, 0x64, 0x9A, 0x00,
            (byte)cf->native_weak_until_turn};
        write_bytes(0x507C22, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->rare_supply_pods) {
        short_jump(0x592085); // bonus_at
        short_jump(0x5920E9); // bonus_at
        short_jump(0x5921C8); // goody_at
    }

    if (!cf->landmarks.jungle    ) remove_call(0x5C88A0);
    if (!cf->landmarks.crater    ) remove_call(0x5C88A9);
    if (!cf->landmarks.volcano   ) remove_call(0x5C88B5);
    if (!cf->landmarks.mesa      ) remove_call(0x5C88BE);
    if (!cf->landmarks.ridge     ) remove_call(0x5C88C7);
    if (!cf->landmarks.uranium   ) remove_call(0x5C88D0);
    if (!cf->landmarks.ruins     ) remove_call(0x5C88D9);
    if (!cf->landmarks.unity     ) remove_call(0x5C88EE);
    if (!cf->landmarks.fossil    ) remove_call(0x5C88F7);
    if (!cf->landmarks.canyon    ) remove_call(0x5C8903);
    if (!cf->landmarks.nexus     ) remove_call(0x5C890F);
    if (!cf->landmarks.borehole  ) remove_call(0x5C8918);
    if (!cf->landmarks.sargasso  ) remove_call(0x5C8921);
    if (!cf->landmarks.dunes     ) remove_call(0x5C892A);
    if (!cf->landmarks.fresh     ) remove_call(0x5C8933);
    if (!cf->landmarks.geothermal) remove_call(0x5C893C);

    /*
    {
        const byte old_bytes[] = {0x8B,0xC7,0x99,0x2B,0xC2,0xD1,0xF8};
        const byte new_bytes[] = {0x57,0xE8,0x00,0x00,0x00,0x00,0x5F};

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
    {
        const byte old_perimeter[] = {0xBE,0x04,0x00,0x00,0x00};
        const byte new_perimeter[] = {0xBE,(byte)(2 + cf->perimeter_defense_bonus),0x00,0x00,0x00};
        write_bytes(0x5034B9, old_perimeter, new_perimeter, sizeof(new_perimeter));

        const byte old_tachyon[] = {0x83,0xC6,0x02};
        const byte new_tachyon[] = {0x83,0xC6,(byte)cf->tachyon_field_bonus};
        write_bytes(0x503506, old_tachyon, new_tachyon, sizeof(new_tachyon));
    }
    {
        const byte old_bytes[] = {0x83,0x80,0x08,0x01,0x00,0x00,0x02};
        const byte new_bytes[] = {0x83,0x80,0x08,0x01,0x00,0x00,(byte)cf->biology_lab_bonus};
        write_bytes(0x4EBC85, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->clean_minerals != 16) {
        const byte old_bytes[] = {0x83,0xC6,0x10};
        const byte new_bytes[] = {0x83,0xC6,(byte)cf->clean_minerals};
        write_bytes(0x4E9E41, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (!cf->aquatic_bonus_minerals) {
        const byte old_bytes[] = {0x46};
        const byte new_bytes[] = {0x90};
        write_bytes(0x4E7604, old_bytes, new_bytes, sizeof(new_bytes));
    }
    if (cf->collateral_damage_value != 3) {
        const byte old_bytes[] = {0xB2,0x03};
        const byte new_bytes[] = {0xB2,(byte)cf->collateral_damage_value};
        write_bytes(0x50AAA5, old_bytes, new_bytes, sizeof(new_bytes));
    }
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
    */

    if (!VirtualProtect(AC_IMAGE_BASE, AC_IMAGE_LEN, PAGE_EXECUTE_READ, &attrs)) {
        return false;
    }
    return true;
}

