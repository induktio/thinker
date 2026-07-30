
#include "game.h"

static uint32_t custom_game_rules = 0;
static uint32_t custom_more_rules = 0;


const char* resource_icon(int res_type, bool ocean, bool add) {
    if (res_type == RES_NUTRIENT) {
        if (ocean) {
            return add ? "rdneuw_sm.pcx" : "rdneuwdp_sm.pcx";
        }
        return add ? "rdneul_sm.pcx" : "rdneuldp_sm.pcx";
    } else if (res_type == RES_MINERAL) {
        if (ocean) {
            return add ? "rdminw_sm.pcx" : "rdminwdp_sm.pcx";
        }
        return "rdminl_sm.pcx"; // No additional image available
    } else if (res_type == RES_ENERGY) {
        if (ocean) {
            return add ? "rdengw_sm.pcx" : "rdengwdp_sm.pcx";
        }
        return add ? "rdengl_sm.pcx" : "rdengldp_sm.pcx";
    }
    assert(0);
    return "";
}

bool un_charter() {
    return ProposalPassCount[PROP_REPEAL_UN_CHARTER]
        <= ProposalPassCount[PROP_REINSTATE_UN_CHARTER];
}

bool global_trade_pact() {
    return ProposalPassCount[PROP_REPEAL_GLOBAL_TRADE_PACT]
        < ProposalPassCount[PROP_GLOBAL_TRADE_PACT];
}

bool victory_done() {
    return (*GameState & STATE_GAME_DONE);
}

bool full_game_turn() {
    return !(*GameState & STATE_GAME_DONE) || (*GameState & STATE_FINAL_SCORE_DONE);
}

bool voice_of_planet() {
    // Even destroyed Voice of Planet allows building Ascent to Transcendence
    return project_base(FAC_VOICE_OF_PLANET) != SP_Unbuilt;
}

bool valid_player(int faction_id) {
    return faction_id > 0 && faction_id < MaxPlayerNum;
}

bool valid_triad(int triad) {
    return (triad == TRIAD_LAND || triad == TRIAD_SEA || triad == TRIAD_AIR);
}

char* label_get(size_t index) {
    assert(index < TextLabels->label_count);
    return (TextLabels->labels)[index];
}

void __cdecl clear() {
    StrBuffer[0] = '\0';
}

char* __cdecl says(const char* buf) {
    size_t len = strnlen(StrBuffer, StrBufLen);
    snprintf(StrBuffer + len, StrBufLen - len, "%s", buf);
    return StrBuffer;
}

char* __cdecl say_num(int value) {
    size_t len = strnlen(StrBuffer, StrBufLen);
    snprintf(StrBuffer + len, StrBufLen - len, "%d", value);
    return StrBuffer;
}

char* __cdecl say_year(char* buf) {
    size_t len = strnlen(buf, StrBufLen);
    snprintf(buf + len, StrBufLen - len, "%d", *CurrentTurn + *StartingMissionYear);
    return buf;
}

char* __cdecl parse_set(int faction_id) {
    *GenderDefault = MFactions[faction_id].noun_gender;
    *PluralDefault = MFactions[faction_id].is_noun_plural;
    return MFactions[faction_id].noun_faction;
}

int __cdecl parse_num(size_t index, int value) {
    if (index > 9) {
        return 3;
    }
    ParseNumTable[index] = value;
    return 0;
}

/*
This function is the preferred, more generic version to be used instead of parse_say.
*/
int __cdecl parse_says(size_t index, const char* src, int gender, int plural) {
   if (!src || index > 9) {
       return 3;
   }
   if (gender < 0) {
       gender = *GenderDefault;
   }
   ParseStrGender[index] = gender;
   if (plural < 0) {
       plural = *PluralDefault;
   }
   ParseStrPlurality[index] = plural;
   strcpy_n(ParseStrBuffer[index].str, StrBufLen, src);
   return 0;
}

int __cdecl game_year(int turn) {
    return Rules->normal_start_year + turn;
}

int __cdecl in_box(int x, int y, RECT* rc) {
    return x >= rc->left && x < rc->right
        && y >= rc->top && y < rc->bottom;
}

/*
Calculate the offset and bitmask for the specified input.
*/
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask) {
    *offset = input / 8;
    *mask = 1 << (input & 7);
}

void show_rules_menu() {
    *GameRules = (*GameRules & GAME_RULES_MASK) | custom_game_rules;
    *GameMoreRules = (*GameMoreRules & GAME_MRULES_MASK) | custom_more_rules;
    Console_editor_scen_rules(MapWin);
    custom_game_rules = *GameRules & ~GAME_RULES_MASK;
    custom_more_rules = *GameMoreRules & ~GAME_MRULES_MASK;
}

void init_world_config() {
    ThinkerVars->game_time_spent = 0;
    /*
    Adjust Special Scenario Rules if any are selected from the mod menu.
    This also overrides settings for any scenarions unless all custom options are left empty.
    */
    if ((custom_game_rules || custom_more_rules) && !*MultiplayerActive) {
        *GameRules = (*GameRules & GAME_RULES_MASK) | custom_game_rules;
        *GameMoreRules = (*GameMoreRules & GAME_MRULES_MASK) | custom_more_rules;
        if (*GameRules & RULES_SCN_NO_NATIVE_LIFE) {
            for (int i = *VehCount - 1; i >= 0; --i) {
                if (!Vehs[i].faction_id && Vehs[i].unit_id == BSC_FUNGAL_TOWER) {
                    kill(i);
                }
            }
        }
    }
}

void init_save_game(int faction_id) {
    MFaction* m = &MFactions[faction_id];
    if (!faction_id) {
        return;
    }
    if (!*CurrentTurn) {
        memset(&m->thinker_probe_lost, 0, 20);
    }
    m->thinker_unused[0] = 0;
    m->thinker_unused[1] = 0;
    m->thinker_unused[2] = 0;
    /*
    Remove invalid prototypes from the savegame.
    This also attempts to repair invalid vehicle stacks to prevent game crashes.
    Stack iterators should never contain infinite loops or vehicles from multiple tiles.
    */
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        int unit_id = faction_id*MaxProtoFactionNum + i;
        UNIT* u = &Units[unit_id];
        if (strlen(u->name) >= MaxProtoNameLen
        || u->chassis_id < CHS_INFANTRY
        || u->chassis_id > CHS_MISSILE) {
            for (int j = *VehCount - 1; j >= 0; j--) {
                if (Vehs[j].unit_id == unit_id) {
                    veh_kill(j);
                }
            }
            for (int j = 0; j < *BaseCount; j++) {
                if (Bases[j].queue_items[0] == unit_id) {
                    Bases[j].queue_items[0] = -FAC_STOCKPILE_ENERGY;
                }
            }
            memset(u, 0, sizeof(UNIT));
        }
        if (u->is_active()) {
            if (u->weapon_mode() == WMODE_SUPPLY && u->plan != PLAN_SUPPLY) {
                print_unit(unit_id);
                u->plan = PLAN_SUPPLY;
            } else if (u->weapon_mode() == WMODE_COLONY && u->plan != PLAN_COLONY) {
                print_unit(unit_id);
                u->plan = PLAN_COLONY;
            } else if (u->weapon_mode() == WMODE_TERRAFORM && u->plan != PLAN_TERRAFORM) {
                print_unit(unit_id);
                u->plan = PLAN_TERRAFORM;
            } else if (u->weapon_mode() == WMODE_PROBE && u->plan != PLAN_PROBE) {
                print_unit(unit_id);
                u->plan = PLAN_PROBE;
            }
        }
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        int prev_id = veh->prev_veh_id_stack;
        int next_id = veh->next_veh_id_stack;
        bool adjust = false;

        if (prev_id == i || (prev_id >= 0 && (Vehs[prev_id].x != veh->x || Vehs[prev_id].y != veh->y))) {
            print_veh_stack(veh->x, veh->y);
            veh->prev_veh_id_stack = -1;
            adjust = true;
        }
        if (next_id == i || (next_id >= 0 && (Vehs[next_id].x != veh->x || Vehs[next_id].y != veh->y))) {
            print_veh_stack(veh->x, veh->y);
            veh->next_veh_id_stack = -1;
            adjust = true;
        }
        if (adjust) {
            stack_fix(i);
        }
    }
}

int* const dword_945820 = (int*)0x945820;
int* const dword_9B2074 = (int*)0x9B2074;
char* const unk_945E78 = (char*)0x945E78;
char* const unk_945E7C = (char*)0x945E7C;

int __cdecl custom_planet(int use_images, int use_defaults) {
    Popup cur_popup;
    Buffer cur_preview[3];
    SetupWin cur_setup;
    Popup_ctor(&cur_popup);
    Buffer_ctor(&cur_preview[0]);
    Buffer_ctor(&cur_preview[1]);
    Buffer_ctor(&cur_preview[2]);
    SetupWin_ctor(&cur_setup);
    auto guard = cleanup_handler([&] {
        SetupWin_dtor(&cur_setup);
        Buffer_dtor(&cur_preview[2]);
        Buffer_dtor(&cur_preview[1]);
        Buffer_dtor(&cur_preview[0]);
        Popup_dtor(&cur_popup);
    });
    if (use_images) {
        cur_setup.field_A1C = &cur_preview[0];
        cur_setup.field_A20 = &cur_preview[1];
        cur_setup.field_A24 = &cur_preview[2];
    }
    memset(MapOceanCoverage, 0, 0x18u);
    // Loads the 3 preview thumbnails "S<ocean>L<life>C<cloud>.PCX", changing only
    // the dimension named by label and holding the other two at their selected values.
    auto load_previews = [&](char label) {
        int32_t* px = &cur_setup.field_A28;
        int32_t* py = &cur_setup.field_A34;
        for (int i = 0; i < 3; ++i) {
            int ocean = (label == 'S') ? i + 1 : clamp(*MapOceanCoverage + 1, 1, 3);
            int life  = (label == 'L') ? i + 1 : clamp(*MapNativeLifeForms + 1, 1, 3);
            int cloud = (label == 'C') ? i + 1 : clamp(*MapCloudCover + 1, 1, 3);
            char filename[32];
            snprintf(filename, sizeof(filename), "S%dL%dC%d.PCX", ocean, life, cloud);
            Buffer_load_pcx(&cur_preview[i], filename, 0, 10, 236);
            if (*ScreenWidth == 800) {
                px[i] = 449;
                py[i] = -168;
            } else {
                px[i] = *ScreenWidth - cur_preview[0].stBitMapInfo.bmiHeader.biWidth;
                py[i] = 0;
            }
        }
    };
    const int FieldOrder[6] = {1, 3, 6, 5, 0, 0};
    for (int field_id : FieldOrder) {
        if (!field_id) {
            continue;
        }
        int32_t* map_val = &MapSizePlanet[field_id];
        int32_t* pref_val = &AlphaIniPrefs->custom_world[field_id];
        *map_val = *pref_val;
        if (use_defaults) {
            continue;
        }
        const char* suffix = "";
        switch (field_id) {
            case 1: suffix = "LAND";   break;
            case 3: suffix = "TIDES";  break;
            case 4: suffix = "ORBIT";  break;
            case 5: suffix = "CLOUDS"; break;
            case 6: suffix = "LIFE";   break;
        }
        snprintf(StrBuffer, StrBufLen, "WORLD%s", suffix);
        if (use_images) {
            switch (field_id) {
                case 1:
                    load_previews('S');
                    break;
                case 3:
                    Buffer_load_pcx(&cur_preview[0], *ScreenWidth == 800 ? "moon1_800.pcx" : "MOON1.PCX", 0, 10, 236);
                    Buffer_load_pcx(&cur_preview[1], *ScreenWidth == 800 ? "moon2_800.pcx" : "MOON2.PCX", 0, 10, 236);
                    Buffer_load_pcx(&cur_preview[2], *ScreenWidth == 800 ? "moon3_800.pcx" : "MOON3.PCX", 0, 10, 236);
                    cur_setup.field_A28 = 0;
                    cur_setup.field_A2C = 0;
                    cur_setup.field_A30 = 0;
                    cur_setup.field_A34 = 0;
                    cur_setup.field_A38 = 0;
                    cur_setup.field_A3C = 0;
                    break;
                case 5:
                    load_previews('C');
                    break;
                case 6:
                    load_previews('L');
                    break;
                default:
                    break;
            }
        }
        Popup_start(&cur_popup, PopupScriptFile, StrBuffer, -1, 0, 0x40, 0);
        cur_popup.field_A44 = *pref_val;
        int result = use_images
            ? SetupWin_do_menu_2(&cur_setup, &cur_popup, 1, 0)
            : BasePop_exec_3(&cur_popup, 0, 0);
        *map_val = result;
        if (result < 0) {
            return 1;
        }
        if (cur_popup.field_3100) {
            *map_val = -1;
        } else {
            *pref_val = result;
        }
    }
    if (use_images) {
        cur_setup.field_A1C = nullptr;
        cur_setup.field_A20 = nullptr;
        cur_setup.field_A24 = nullptr;
    }
    *MapLandCoverage = 2 - *MapOceanCoverage;
    prefs_save(0);
    return 0;
}

int __cdecl size_of_planet(int setup_mode) {
    debug("size_of_planet %d\n", setup_mode);
    Popup cur_popup;
    SetupWin cur_setup;
    Popup_ctor(&cur_popup);
    SetupWin_ctor(&cur_setup);
    auto guard = cleanup_handler([&] {
        SetupWin_dtor(&cur_setup);
        Popup_dtor(&cur_popup);
    });
    Popup_start(&cur_popup, PopupScriptFile, "WORLDSIZE", -1, 0, setup_mode != 2 ? 0x40 : 0, 0);
    *dword_945820 = 1;
    int width, height;

    if (!text_open(alpha_file(), "WORLDSIZE")) {
        text_get();
        // TODO: consider if max count is related to Dialogs limitations
        int entry_count = min(32, text_item_number());
        int widths[32];
        int heights[32];
        for (int i = 0; i < entry_count; ++i) {
            text_get();
            snprintf(StrBuffer, StrBufLen, "%s", text_item());
            Dialogs_item(&cur_popup.dialogs, StrBuffer, i);
            widths[i]  = text_item_number();
            heights[i] = text_item_number();
        }
        snprintf(StrBuffer, StrBufLen, "%s", label_get(TL_CustomSize));
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 99);
        int selection = AlphaIniPrefs->custom_world[0];
        cur_popup.field_A44 = selection;

        if (setup_mode == 1) {
            *MapSizePlanet = selection;
        } else {
            selection = (setup_mode == 2)
                ? BasePop_exec_3(&cur_popup, 0, 0)
                : SetupWin_do_menu_2(&cur_setup, &cur_popup, 1, 0);
            *MapSizePlanet = selection;
            if (selection < 0) {
                return 1;
            }
            if (cur_popup.field_3100) {
                selection = game_rand() % 5;
                *MapSizePlanet = selection;
            } else if (selection != 99) {
                AlphaIniPrefs->custom_world[0] = selection;
            }
        }
        if (selection != 99) {
            width = widths[selection];
            *MapAreaY = heights[selection];
        } else {
            Popup_start(&cur_popup, PopupScriptFile, "CUSTOMMAP", -1, 0, 0x44, 0);
            int custom_x  = prefs_get_2("Custom Map X", 40, 0);
            int custom_y = prefs_get_2("Custom Map Y", 80, 0);
            EditGroup* editgrp = (EditGroup*)(&cur_popup.dialogs.editGroup);
            snprintf(StrBuffer, StrBufLen, "%d", custom_x);
            EditGroup_edit(editgrp, label_get(TL_Horizontal), StrBuffer, 8);
            snprintf(StrBuffer, StrBufLen, "%d", custom_y);
            EditGroup_edit(editgrp, label_get(TL_Vertical), StrBuffer, 8);
            if (BasePop_exec_3(&cur_popup, 0, 0) < 0) {
                return 1;
            }
            // Increase maximum custom map dimensions from 256x256 to 512x512
            // Warning dialog threshold increased from 128x128 to 256x256
            width = clamp(atoi(ParseStrBuffer[0].str), 16, 512);
            height = clamp(atoi(ParseStrBuffer[1].str), 16, 512);
            if ((width <= 256 && height <= 256) || !X_pop("VERYLARGEMAP", 0)) {
                prefs_put_2("Custom Map X", width, 0);
                prefs_put_2("Custom Map Y", height, 0);
                *MapAreaY = height;
            } else {
                return 1;
            }
        }
    } else {
        *MapSizePlanet = 2;
        width = 40;
        *MapAreaY = 80;
    }
    *MapAreaX = 2 * width;
    prefs_save(0);
    if (setup_mode < 2) {
        map_init();
        Path_init(Paths);
    }
    *GameDrawState |= 8;
    return 0;
}

int __cdecl map_menu(int flag) {
    debug("map_menu %d\n", flag);
    SetupWin cur_setup;
    Popup cur_popup;
    SetupWin_ctor(&cur_setup);
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] {
        Popup_dtor(&cur_popup);
        SetupWin_dtor(&cur_setup);
    });
    cur_setup.pMenuWin = *TopMenuWin;

    while (true) {
        *dword_945820 = 0;
        Popup_start(&cur_popup, PopupScriptFile, "MAPMENU", -1, 0, 0, 0);
        cur_popup.field_A44 = DefaultPrefs->map_type;
        int choice = DefaultPrefs->map_type;
        if (!flag) {
            choice = SetupWin_do_menu_2(&cur_setup, &cur_popup, 1, 0);
        }
        if (choice < 0) {
            return 1;
        }
        DefaultPrefs->map_type = choice;
        prefs_save(0);
        bool restart_outer = false;
        bool reset_climate = false;
        bool retry;
        do {
            retry = false;
            switch (choice) {
            case 0:
            case 1:
                if (size_of_planet(flag)) {
                    restart_outer = true;
                    break;
                }
                if (!*dword_945820) {
                    break;
                }
                if (choice == 1) {
                    if (custom_planet(1, flag)) {
                        retry = true;
                    }
                } else {
                    int ocean;
                    if (*MultiplayerActive) {
                        ocean = 0;
                    } else {
                        ocean = 2 - clamp((game_rand() % 7 + 1) / 2, 0, 2);
                    }
                    *MapOceanCoverage = ocean;
                    *MapLandCoverage = 2 - ocean;
                    *MapPlanetaryOrbit = 1;
                    *MapCloudCover = 1;
                    *MapNativeLifeForms = 1;
                    *MapErosiveForces = (game_rand() % 6 + 5) / 5;
                }
                break;

            case 2:
                if (mod_load_map_daemon("maps\\xplanet.mp")) {
                    restart_outer = true;
                } else {
                    reset_climate = true;
                }
                break;

            case 3:
                if (mod_load_map_daemon("maps\\xplanetx.mp")) {
                    restart_outer = true;
                } else {
                    reset_climate = true;
                }
                break;

            case 4:
                if (load_map()) {
                    flag = 0;
                    restart_outer = true;
                } else {
                    reset_climate = true;
                }
                break;

            default:
                break;
            }
        } while (retry);
        if (restart_outer) {
            continue;
        }
        if (reset_climate) {
            world_climate();
        }
        if (config_game(flag)) {
            continue;
        }
        return 0;
    }
}

int __cdecl top_menu(int flag) {
    debug("top_menu %d\n", flag);
    SetupWin cur_setup;
    Popup cur_popup;
    SetupWin_ctor(&cur_setup);
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] {
        Popup_dtor(&cur_popup);
        SetupWin_dtor(&cur_setup);
    });
    auto menu_image = []() {
        return *ExpansionEnabled
            ? (*ScreenWidth != 800 ? "xopeninga.pcx" : "xopeningb.pcx")
            : (*ScreenWidth != 800 ? "openinga.pcx"  : "openingb.pcx");
    };
    if (!flag) {
        *PbemActive = 0;
        clear_all_player_messages();
    }
    prefs_load(0);
    prefs_use();
    if (!flag && (*GamePreferences & PREF_AV_VOLUME_MUSIC_TOGGLE)) {
        Wave_load_3(WaveTopMenu, "fx\\opening menu.wav", 2);
        Wave_play_2(WaveTopMenu);
    }
    if (!(AlphaIniPrefs->preferences & PREF_ADV_RADIO_BTN_NOT_SEL_SING_CLK)) {
        *GameDialogFlags |= 4;
    } else {
        *GameDialogFlags &= ~4;
    }
    init_opening(menu_image());
    cur_setup.pMenuWin = *TopMenuWin;
    *NetUpkeepState = 0;
    const uint32_t ScnFlags = (STATE_EDITOR_ONLY_MODE|STATE_OMNISCIENT_VIEW|\
        STATE_SCENARIO_EDITOR|STATE_SCENARIO_CHEATED_FLAG);

    while (true) {
        bool close_menu = false;
        init_opening(menu_image());
        cur_setup.pMenuWin = *TopMenuWin;
        prefs_load(0);
        prefs_use();
        Popup_start(&cur_popup, flag ? PopupScriptFile : "modmenu",
            flag ? "HOTSEAT" : "TOPMENU", -1, 0, 0, 0);
        if (!flag) {
            cur_popup.field_A44 = DefaultPrefs->top_menu;
        }
        int menu_choice = SetupWin_do_menu_2(&cur_setup, &cur_popup, 1, 0);
        debug("menu_choice %d\n", menu_choice);
        // show_credits() moved from 5 to 6, and exit game becomes 7
        if (menu_choice < 0 || menu_choice == 7) {
            return 1;
        }
        if (flag) {
            if (menu_choice > 0) {
                menu_choice = menu_choice + 1;
            }
        } else {
            DefaultPrefs->top_menu = menu_choice;
            prefs_save(0);
        }
        switch (menu_choice) {
        case 0:
        case 1:
            if (map_menu(menu_choice == 1) == 0) {
                close_menu = true;
            }
            break;
        case 2:
            int scenario_choice;
            if (flag) {
                scenario_choice = 0;
            } else {
                scenario_choice = SetupWin_do_menu(&cur_setup, "SCENARIOMENU", 1, 0);
                if (scenario_choice < 0) {
                    break;
                }
                if (scenario_choice == 1) {
                    size_of_planet(1);
                    *MapOceanCoverage = 1;
                    *MapLandCoverage = 1;
                    *MapErosiveForces = 1;
                    *MapPlanetaryOrbit = 1;
                    *MapCloudCover = 1;
                    *MapNativeLifeForms = 1;
                    map_wipe();
                } else if (scenario_choice == 2) {
                    if (load_map()) {
                        break;
                    }
                }
                if (scenario_choice == 1 || scenario_choice == 2) {
                    config_game(3);
                    *VehCount = 0;
                    setup_game(1);
                    *ControlTurnMove = MapWin->cOwner;
                    *GameState |= ScnFlags;
                    close_menu = true;
                    break;
                }
            }
            int is_pbem, result;
            is_pbem = *PbemActive;
            result = load_game(1, 0);
            *PbemActive = (is_pbem && !*MultiplayerActive);
            if (result) {
                break;
            }
            if (*dword_9B2074) {
                stop_timers();
                labels_shutdown();
                Strings_shutdown(TextTable);
                if (!game_init(1, 0))
                    *GameHalted = 0;
            }
            if (scenario_choice) {
                *ControlTurnMove = MapWin->cOwner;
                *GameState |= ScnFlags;
                close_menu = true;
                break;
            }
            if (config_game(2)) {
                read_rules(1);
            } else {
                close_menu = true;
                *ControlTurnMove = MapWin->cOwner;
                parse_says(1, get_title(MapWin->cOwner), -1, -1);
                parse_says(2, get_name(MapWin->cOwner), -1, -1);
                parse_says(3, get_noun(MapWin->cOwner), -1, -1);
                snprintf(StrBuffer, StrBufLen, "%s ", label_get(TL_MissionYear));
                say_year(StrBuffer);
                parse_says(4, StrBuffer, -1, -1);
                parse_says(0, StrBuffer, -1, -1);
                parse_num(0, *ObjectiveReqVictory);
                parse_num(1, *ObjectivesSuddenDeathVictory);
                parse_num(2, num_objectives(MapWin->cOwner, *GameRules & RULES_VICTORY_COOPERATIVE));
                parse_num(3, *EndingMissionYear);
                X_pop_2("SCENARIO", "INTRO", 0);
                Console_set_move(MapWin, 1);
            }
            break;
        case 3:
            *SaveFileMenu = flag;
            if (load_game(0, 0)) {
                *SaveFileMenu = 0;
                break;
            }
            *SaveFileMenu = 0;
            *ControlTurnMove = MapWin->cOwner;
            if (*dword_9B2074) {
                stop_timers();
                labels_shutdown();
                Strings_shutdown(TextTable);
                if (!game_init(1, 0)) {
                    *GameHalted = 0;
                }
            }
            close_menu = true;
            break;
        case 4:
            if (!multiplayer_init(0)) {
                close_menu = true;
            }
            break;
        case 5:
            show_mod_menu();
            break;
        case 6:
            show_credits();
            break;
        default:
            close_menu = true;
            break;
        }
        flushlog();
        if (close_menu || *ControlTurnA) {
            if ((!flag && (*GamePreferences & PREF_AV_VOLUME_MUSIC_TOGGLE))
            || Wave_is_playing(WaveTopMenu)) {
                Sound_fade_2(WaveTopMenu, 3000);
                Wave_unload(WaveTopMenu);
            }
            return *ControlTurnA;
        }
    }
}

void __cdecl setup_game(int flag) {
    debug("setup_game %d\n", flag);
    LastSavePath[0] = 0;
    prefs_put("Latest Save", unk_945E78);
    prefs_put("Latest Scenario", unk_945E7C);

    // Skip DontResetBeginnerPrefs=0 preferences reset if Thinker is enabled
    if (!*DiffLevel && !flag && !conf.factions_enabled
    && !prefs_get_2("DontResetBeginnerPrefs", 0, 0)) {
        prefs_load(1);
        AlphaIniPrefs->preferences |= PREF_BSC_TUTORIAL_MSGS;
        prefs_put_2("DontResetBeginnerPrefs", 0, 0);
    }
    if (*MultiplayerActive) {
        AlphaIniPrefs->preferences &= ~PREF_BSC_TUTORIAL_MSGS;
    }
    prefs_use();
    *ObjectiveAchievePts = 0;
    *ObjectiveReqVictory = 9999;
    *ObjectivesSuddenDeathVictory = 9999;
    *VictoryAchieveBonusPts = 0;
    *StartingMissionYear = Rules->normal_start_year;

    if (*DiffLevel >= 3) {
        *EndingMissionYear = Rules->normal_end_year_high_three_diff;
    } else {
        *EndingMissionYear = Rules->normal_end_year_low_three_diff;
    }
    clear_monuments();
    wipe_undo();
    *ReplayEventSize = 0;
    *GameInterludeState = 0;
    *GameState &= (STATE_RAND_FAC_LEADER_SOCIAL_AGENDA | STATE_RAND_FAC_LEADER_PERSONALITIES);
    cs->dword_9A64AC = 0;
    cs->dword_9A64B0 = 0;
    *TutWinMapState = 0;
    cs->dword_9A64B8 = 0;
    cs->dword_9A64BC = 0;
    *CurrentTurn = 0;
    FactionStatus[1] = 0xFF;
    *BaseCount = 0;
    cs->dword_9A64D0 = 0;
    clear_all_offers();
    clear_hotseat_chat();
    clear_council_notify_2();
    clear_tamper();
    cs->dword_9A64E4 = 0;
    *GovernorFaction = -1;
    for (int i = 0; i < MaxProposalNum; ++i) {
        ProposalPassCount[i] = 0;
        ProposalCallTurn[i] = -9999;
    }
    *ClimateValueA = 0;
    *ClimateValueB = 0;
    *ClimateValueC = 0;
    *ClimateFutureChange = 0;

    if (!*MultiplayerActive && flag && !*ControlTurnB) {
        *GameState |= (STATE_OMNISCIENT_VIEW|STATE_SCENARIO_EDITOR|STATE_SCENARIO_CHEATED_FLAG);
    }
    memset(TechOwners, 0, 0x59u);
    memset(SecretProjects, 0xFF, 0x100u);
    memset(ReplayEvents, 0, 0x2000u);
    memset(FactionTurnMight, 0, 0x3E80u);
    memset(SunspotDuration, 0, 0x28u);
    memset(TectonicDetonationCount, 0, 0x20u);
    memset(DiploStateB, 0, 0x100u);
    /*
    Fix issue with randomized faction agendas where they might be given agendas that are
    their opposition social models making the choice unusable. Future social models can be
    also selected but much less often. The original version never selected future social models
    but these are used by several default expansion factions. As for soc_priority_effect,
    the game picks the first social effect that is more than zero in the default social model.
    */
    if (*GameState & STATE_RAND_FAC_LEADER_SOCIAL_AGENDA) {
        for (int fc = 1; fc < MaxPlayerNum; ++fc) {
            MFaction* fac = &MFactions[fc];
            fac->soc_priority_category = -1;
            fac->soc_priority_model = 0;
            fac->soc_priority_effect = -1; // Fix: always initialize
            fac->soc_opposition_category = -1;
            fac->soc_opposition_model = 0; // Fix: always initialize
            fac->soc_opposition_effect = -1; // Fix: always initialize
            for (int iter = 0; iter < 1000; ++iter) {
                int val = game_randv(4) ? 3 : 4;
                int sfield = game_randv(val);
                int smodel = game_randv(3) + 1;
                if (SocialField[sfield].soc_preq_tech[smodel] == TECH_Disable
                || (sfield == fac->soc_opposition_category && smodel == fac->soc_opposition_model)) {
                    continue;
                }
                bool valid = true;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (fc != i && is_alive(i)
                    && MFactions[i].soc_priority_category == sfield
                    && MFactions[i].soc_priority_model == smodel) {
                        valid = false;
                    }
                }
                if (valid) {
                    fac->soc_priority_category = sfield;
                    fac->soc_priority_model = smodel;
                    debug("setup_game agenda %s %d %d\n",  fac->filename, sfield, smodel);
                    for (int i = 0; i < MaxSocialEffectNum; ++i) {
                        if (SocialField[sfield].soc_effect[smodel].values[i] > 0) {
                            fac->soc_priority_effect = i;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    for (int i = 0; i < MaxPlayerNum; ++i) {
        if (is_human(i)) {
            mod_setup_player(i, -1 - flag, 0);
        }
    }
    for (int i = 0; i < MaxPlayerNum; ++i) {
        if (!is_human(i)) {
            mod_setup_player(i, -1 - flag, 0);
        }
    }
    strcpy_n(MFactions[0].formal_name_faction, sizeof(MFaction::formal_name_faction), label_get(TL_Aliens));
    strcpy_n(MFactions[0].noun_faction, sizeof(MFaction::noun_faction), label_get(TL_Aliens));
    strcpy_n(MFactions[0].adj_name_faction, sizeof(MFaction::adj_name_faction), label_get(TL_Alien));

    /*
    COMMFREQ = Gets an extra comm frequency (another faction to talk to) at beginning of game.
    Fix: the ability is applied at the start of the game and commlinks are picked
    with equal probability among all possible players.
    */
    if (!flag) {
        for (int a = 1; a < MaxPlayerNum; ++a) {
            if (MFactions[a].rule_flags & RFLAG_COMMFREQ) {
                int plrs[MaxPlayerNum] = {};
                int num = 0;
                for (int b = 1; b < MaxPlayerNum; ++b) {
                    if (a != b && is_alive(b) && !has_treaty(a, b, DIPLO_COMMLINK)) {
                        plrs[num] = b;
                        ++num;
                    }
                }
                if (num > 0) {
                    set_treaty(a, plrs[game_randv(num)], DIPLO_COMMLINK, 1);
                }
            }
        }
        if (!*MultiplayerActive || (*GameRules & RULES_INTENSE_RIVALRY)) {
            if (*GameState & STATE_RAND_FAC_LEADER_PERSONALITIES) {
                for (int a = 1; a < MaxPlayerNum; ++a) {
                    if (is_human(a)) {
                        int diff_val = (Factions[a].diff_level >= 3)
                            + (*GameRules & RULES_INTENSE_RIVALRY ? 1 : 0);
                        for (int i = 0; i < diff_val; ++i) {
                            for (int iter = 0; iter < 100; ++iter) {
                                int b = game_rand() % (MaxPlayerNum-1) + 1;
                                if (!(Factions[b].diplo_agenda[a] & AGENDA_UNK_200)
                                && is_alive(b) && !is_human(b)) {
                                    agenda_on(b, a, AGENDA_UNK_200);
                                    break;
                                }
                            }
                        }
                    }
                }
            } else {
                for (int a = 1; a < MaxPlayerNum; ++a) {
                    for (int b = 1; b < MaxPlayerNum; ++b) {
                        if (a == b) {
                            continue;
                        }
                        const char* key1 = MFactions[a].search_key;
                        const char* key2 = MFactions[b].search_key;
                        if (MFactions[a].is_alien() || MFactions[b].is_alien()
                        || (!_stricmp(key1, "PEACE")   && !_stricmp(key2, "SPARTANS"))
                        || (!_stricmp(key1, "GAIANS")  && !_stricmp(key2, "MORGAN"))
                        || (!_stricmp(key1, "HIVE")    && !_stricmp(key2, "BELIEVE"))
                        || (!_stricmp(key1, "UNIV")    && !_stricmp(key2, "BELIEVE"))
                        || (!_stricmp(key1, "FUNGBOY") && !_stricmp(key2, "BELIEVE"))
                        || (!_stricmp(key1, "PEACE")   && !_stricmp(key2, "PIRATES"))
                        || (!_stricmp(key1, "DRONE")   && !_stricmp(key2, "PIRATES"))
                        || (!_stricmp(key1, "DRONE")   && !_stricmp(key2, "MORGAN"))
                        || (!_stricmp(key1, "DRONE")   && !_stricmp(key2, "HIVE"))
                        || (!_stricmp(key1, "ANGELS")  && !_stricmp(key2, "HIVE"))
                        || (!_stricmp(key1, "ANGELS")  && !_stricmp(key2, "CYBORG"))) {
                            agenda_on(a, b, AGENDA_UNK_200);
                        }
                        if (MFactions[a].is_alien() && MFactions[b].is_alien()) {
                            set_treaty(b, a, DIPLO_UNK_80000000|DIPLO_UNK_40000000|DIPLO_UNK_8000000|\
                                DIPLO_MAJOR_ATROCITY_VICTIM|DIPLO_ATROCITY_VICTIM|DIPLO_UNK_800|DIPLO_SHALL_BETRAY|\
                                DIPLO_WANT_REVENGE|DIPLO_VENDETTA|DIPLO_COMMLINK, 1);
                            set_agenda(b, a, AGENDA_PERMANENT|AGENDA_UNK_800|AGENDA_UNK_400|AGENDA_UNK_20|\
                                AGENDA_FIGHT_TO_DEATH|AGENDA_UNK_4|AGENDA_UNK_1, 1);
                        }
                    }
                }
            }
        }
        if (*GameRules & RULES_TIME_WARP) {
            mod_time_warp();
        } else if (!conf.skip_default_balance) {
            balance();
        }
    }
}

int __cdecl generators(int faction_id, int* pop_size_req) {
    if (pop_size_req) {
        *pop_size_req = 0;
    }
    if (!MFactions[faction_id].is_alien()) {
        return 0;
    }
    int completed = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id && base->has_fac_built(FAC_SUBSPACE_GENERATOR)) {
            if (base->pop_size < Rules->base_size_subspace_gen) {
                if (pop_size_req) {
                    ++(*pop_size_req);
                }
            } else {
                ++completed;
            }
        }
    }
    return completed;
}

int __cdecl end_of_game(int flag) {
    const int player_id = *CurrentPlayerFaction;
    if ((*GameState & STATE_GAME_DONE) && (*GameState & STATE_FINAL_SCORE_DONE)) {
        return 0;
    }
    int winner_id = -1;
    if (!(*GameState & STATE_GAME_DONE) && flag) {
        if (*CurrentMissionYear >= *EndingMissionYear) {
            *GameVictoryType = VIC_TIME_LIMIT;
            *GameState |= STATE_GAME_DONE;
        }
        for (int fc_id = 1; fc_id < MaxPlayerNum; fc_id++) {
            if (!is_alive(fc_id)) { continue; }
            // Alien Victory (Subspace Generators)
            if (MFactions[fc_id].is_alien()) {
                int active_generators = generators(fc_id, 0);
                if (active_generators >= Rules->subspace_gen_req) {
                    Factions[fc_id].player_flags |= PFLAG_UNK_100000;
                    if (fc_id == player_id) {
                        *GameVictoryType = VIC_ALIEN_SOLO;
                    } else if ((Factions[fc_id].diplo_status[player_id] & DIPLO_PACT)
                    && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
                        if (*GameVictoryType != VIC_ALIEN_SOLO) {
                            *GameVictoryType = VIC_ALIEN_COOP;
                        }
                    } else {
                        if (*GameVictoryType != VIC_ALIEN_SOLO && *GameVictoryType != VIC_ALIEN_COOP) {
                            *GameVictoryType = VIC_ALIEN_LOSS;
                        }
                    }
                    *GameState |= STATE_GAME_DONE;
                    if (winner_id < 0 || fc_id == player_id) {
                        winner_id = fc_id;
                    }
                    *GameMoreRules |= MRULES_UNK_80;
                }
            }
            // Economic Victory
            int market_turn = Factions[fc_id].corner_market_turn;
            if (market_turn > 0 && market_turn == *CurrentTurn && Factions[fc_id].corner_market_cost > 0) {
                Factions[fc_id].player_flags |= PFLAG_UNK_40000;
                if (fc_id == player_id) {
                    *GameVictoryType = VIC_ECONOMIC_SOLO;
                } else if ((Factions[fc_id].diplo_status[player_id] & DIPLO_PACT)
                && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
                    if (*GameVictoryType != VIC_ECONOMIC_SOLO) {
                        *GameVictoryType = VIC_ECONOMIC_COOP;
                    }
                } else {
                    if (*GameVictoryType != VIC_ECONOMIC_SOLO && *GameVictoryType != VIC_ECONOMIC_COOP) {
                        *GameVictoryType = VIC_ECONOMIC_LOSS;
                    }
                }
                *GameState |= (STATE_VICTORY_ECONOMIC | STATE_GAME_DONE);
                winner_id = fc_id;
                if (fc_id == player_id) { break; } // skip other checks
            }
            // Sudden Death Scenario Objectives
            else if ((is_human(fc_id) || !(*GameRules & RULES_SCN_VICT_SOLO_MISSION))
            && *ObjectivesSuddenDeathVictory >= 1) {
                if (num_objectives(fc_id, *GameRules & RULES_VICTORY_COOPERATIVE)
                >= *ObjectivesSuddenDeathVictory) {
                    *GameVictoryType = VIC_SUDDEN_DEATH;
                    *GameState |= STATE_GAME_DONE;
                    if (winner_id < 0 || fc_id == player_id) {
                        winner_id = fc_id;
                    }
                }
            }
        }
    }
    if (flag && !(*GameState & STATE_GAME_DONE)) {
        if (!(*GameState & STATE_IS_SCENARIO) && *CurrentMissionYear == *EndingMissionYear - 20) {
            parse_num(0, *EndingMissionYear);
            snprintf(StrBuffer, StrBufLen, "%d", *EndingMissionYear);
            parse_says(0, StrBuffer, -1, -1);
            X_pop("RETIREWARNING", 0);
        } else if ((*GameState & STATE_IS_SCENARIO)) {
            if (*CurrentMissionYear == *EndingMissionYear - 20
            || *CurrentMissionYear == *EndingMissionYear - 10) {
                parse_num(0, *EndingMissionYear);
                snprintf(StrBuffer, StrBufLen, "%d", *EndingMissionYear);
                parse_says(0, StrBuffer, -1, -1);
                X_pop_2("SCENARIO", "SCENARIOWARNING", 0);
            }
        }
    }
    if (!(*GameState & STATE_GAME_DONE) || (*GameState & STATE_FINAL_SCORE_DONE)) {
        return 0;
    }
    int victor_id = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (Factions[i].player_flags & PFLAG_UNK_20000) {
            victor_id = i;
        }
        if ((Factions[i].player_flags & PFLAG_UNK_40000)
        && (!victor_id || victor_id == player_id
        || (Factions[victor_id].diplo_status[player_id] & DIPLO_PACT))) {
            victor_id = i;
        }
    }
    auto setup_parser = [](int id) {
        parse_says(0, get_title(id), -1, -1);
        parse_says(1, get_name(id), -1, -1);
        parse_says(2, get_noun(id), -1, -1);
    };
    bool play_credits = false;

    switch (*GameVictoryType) {
    case VIC_TRANSCEND_PLR:
    case VIC_TRANSCEND_UNK:
    case VIC_TRANSCEND_LOSS:
        play_credits = true;
        break;

    case VIC_DIPLOMATIC_SOLO:
        popp(ScriptFile, "DIPLOMATICVICTORY", 0, "dipvic_sm.pcx", 0);
        break;

    case VIC_LOST_CAPTURE:
        if (*GamePreferences & PREF_AV_SECRET_PROJECT_MOVIES) {
            if (!MFactions[player_id].is_alien()) {
                mod_amovie_project(MFactions[player_id].is_leader_female ? "losewoman" : "loseman");
            } else {
                mod_amovie_project("losealien");
            }
        }
        break;

    case VIC_TIME_LIMIT:
        parse_num(0, *CurrentMissionYear);
        StrBuffer[0] = 0;
        say_year(StrBuffer);
        parse_says(0, StrBuffer, -1, -1);
        if (*GameState & STATE_IS_SCENARIO) {
            if (!*ObjectiveReqVictory || (*GameState & STATE_SCN_VICT_HIGHEST_AC_SCORE_WINS)) {
                int best_fc_id = player_id;
                int best_score = -9999;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (is_alive(i)) {
                        int current_score;
                        if (*GameState & STATE_SCN_VICT_HIGHEST_AC_SCORE_WINS) {
                            int score_val = 0, other_val = 0;
                            compute_score(i, &score_val, &other_val, 1);
                            current_score = score_val;
                        } else {
                            current_score = num_objectives(i, *GameRules & RULES_VICTORY_COOPERATIVE);
                        }
                        if (current_score > best_score || (current_score == best_score && i == player_id)) {
                            best_fc_id = i;
                            best_score = current_score;
                        }
                    }
                }
                setup_parser(best_fc_id);
                if (best_fc_id == player_id) {
                    X_pop_2("SCENARIO", "SCENTIMEWIN", 0);
                } else {
                    X_pop_2("SCENARIO", "SCENTIMELOSS", 0);
                }
            } else {
                X_pop_2("SCENARIO", "SCENTIMELIMIT", 0);
            }
        } else {
            X_pop("TIMELIMIT", 0);
        }
        break;

    case VIC_SUDDEN_DEATH:
        setup_parser(winner_id);
        if (winner_id == player_id) {
            X_pop_2("SCENARIO", "SUDDENDEATH0", 0);
        } else {
            X_pop_2("SCENARIO", "SUDDENDEATH", 0);
        }
        break;

    case VIC_DIPLOMATIC_COOP:
        setup_parser(victor_id);
        popp(ScriptFile, "DIPLOMATICCOOP", 0, "dipvic_sm.pcx", 0);
        break;

    case VIC_DIPLOMATIC_LOSS:
        setup_parser(victor_id);
        {
            const char* popup_str = "DIPLOMATICLOSE2";
            if (!(Factions[victor_id].diplo_status[player_id] & (DIPLO_WANT_REVENGE | DIPLO_ATROCITY_VICTIM))) {
                popup_str = "DIPLOMATICLOSE";
            }
            popp(ScriptFile, popup_str, 0, "dipvic_sm.pcx", 0);
        }
        break;

    case VIC_ECONOMIC_SOLO:
        popp(ScriptFile, "ECONOMICVICTORY", 0, "econwin_sm.pcx", 0);
        break;

    case VIC_ECONOMIC_COOP:
        setup_parser(victor_id);
        popp(ScriptFile, "ECONOMICCOOP", 0, "econwin_sm.pcx", 0);
        break;

    case VIC_ECONOMIC_LOSS:
        setup_parser(victor_id);
        {
            const char* popup_str = "ECONOMICLOSE2";
            if (!(Factions[victor_id].diplo_status[player_id] & (DIPLO_ATROCITY_VICTIM | DIPLO_WANT_REVENGE))) {
                popup_str = "ECONOMICLOSE";
            }
            popp(ScriptFile, popup_str, 0, "econwin_sm.pcx", 0);
        }
        break;

    case VIC_LOST_REMOVE:
        if (*GameLanguage) {
            X_pop("YOULOSE2", 0);
        }
        break;

    case VIC_ALIEN_SOLO:
        interlude(32, 0, 4, 0);
        setup_parser(victor_id);
        popp(ScriptFile, "ENDBEACON", 0, "beacon_sm.pcx", 0);

        if (*GamePreferences & PREF_AV_SECRET_PROJECT_MOVIES) {
            if (!strcmpi("CARETAKE", MFactions[player_id].search_key)) {
                mod_amovie_project("close_ct");
            } else {
                mod_amovie_project("close_us");
            }
        }
        break;

    case VIC_ALIEN_COOP:
        setup_parser(victor_id);
        popp(ScriptFile, "ENDBEACONCOOP", 0, "beacon_sm.pcx", 0);
        break;

    case VIC_ALIEN_LOSS:
        if (!MFactions[player_id].is_alien()) {
            *GenderDefault = MFactions[victor_id].noun_gender;
            *PluralDefault = MFactions[victor_id].is_noun_plural;
            interlude(29, MFactions[victor_id].noun_faction, 1, 0);
        }
        setup_parser(victor_id);
        popp(ScriptFile, "ENDBEACONLOSE", 0, "beaconlose_sm.pcx", 0);
        break;
    }

    switch (*GameVictoryType) {
    case VIC_UNIFY_SOLO:
    case VIC_UNIFY_COOP:
    case VIC_DIPLOMATIC_SOLO:
    case VIC_ECONOMIC_SOLO:
        mon_winning_unify(player_id, -1);
        break;
    case VIC_DIPLOMATIC_COOP:
    case VIC_ECONOMIC_COOP:
        mon_winning_unify(victor_id, -1);
        mon_winning_unify(player_id, victor_id);
        break;
    case VIC_DIPLOMATIC_LOSS:
    case VIC_ECONOMIC_LOSS:
        mon_winning_unify(victor_id, -1);
        break;
    }

    if (!(*GamePreferences & PREF_AV_INTERLUDES_DISABLED)) {
        switch (*GameVictoryType) {
        case VIC_UNIFY_SOLO:
        case VIC_DIPLOMATIC_SOLO:
        case VIC_ECONOMIC_SOLO:
            if (!MFactions[player_id].is_alien()) {
                interlude(19, 0, 4, 0);
            } else {
                interlude(33, 0, 4, 0);
            }
            play_credits = true;
            break;
        case VIC_UNIFY_COOP:
        case VIC_DIPLOMATIC_COOP:
        case VIC_ECONOMIC_COOP:
            interlude(20, 0, 4, 0);
            play_credits = true;
            break;
        }
    }
    if (play_credits
    || *GameVictoryType == VIC_TRANSCEND_PLR
    || *GameVictoryType == VIC_TRANSCEND_UNK
    || *GameVictoryType == VIC_TRANSCEND_LOSS) {
        show_credits();
    }
    report_score(1);
    if (!(*GameState & STATE_IS_SCENARIO)) {
        quayle(player_id);
        hall_of_fame(1);
    }
    show_replay();
    *GameState |= STATE_FINAL_SCORE_DONE;
    bool should_exit = (*MultiplayerActive || *GameVictoryType == VIC_LOST_CAPTURE);
    if (!should_exit) {
        should_exit = !popp(ScriptFile, "GAMEOVERMAN", 0, "stars_sm.pcx", 0);
    }
    if (should_exit) {
        *ControlTurnB = (*MultiplayerActive == 0);
        if (*MultiplayerActive) {
            net_game_close();
        }
        *ControlTurnA = 1;
        return 1;
    }
    return 0;
}

/*
Store base related events for the endgame replay screen.
0 = create base, 1 = change base owner, 2 = kill base.
*/
int __cdecl replay_base(int event, int x, int y, int faction_id) {
    assert(mapsq(x, y) && event >= 0 && event <= 2);
    debug("replay_base %d %d %d %d %d %d\n",
        *ReplayEventSize, *CurrentTurn, event, faction_id, x, y);
    if (*ReplayEventSize >= 0 && *ReplayEventSize < 8192) {
        ReplayEvent* p = &ReplayEvents[*ReplayEventSize/8];
        p->event = event;
        p->faction_id = faction_id;
        p->turn = *CurrentTurn;
        p->x = x;
        p->y = y;
        *ReplayEventSize += 8;
    }
    return *ReplayEventSize;
}

uint32_t offset_next(int32_t faction, uint32_t position, uint32_t amount) {
    if (!position) {
        return 0;
    }
    uint32_t loop = 0;
    uint32_t offset = ((*MapRandomSeed + faction) & 0xFE) | 1;
    do {
        if (offset & 1) {
            offset ^= 0x170;
        }
        offset >>= 1;
    } while (offset >= amount || ++loop != position);
    return offset;
}

/*
Generate a base name. Uses additional base names from basenames folder.
When faction base names are used, creates additional variations from basenames/generic.txt.
First land/sea base always uses the first available name from land/sea names list.
Vanilla name_base chooses sea base names in a sequential non-random order (this version is random).
*/
void __cdecl mod_name_base(int faction_id, char* name, bool save_offset, bool sea_base) {
    if (!conf.new_base_names) {
        return name_base(faction_id, name, save_offset, sea_base);
    }
    Faction& f = Factions[faction_id];
    uint32_t offset = f.base_name_offset;
    uint32_t offset_sea = f.base_sea_name_offset;
    const int buf_size = 256;
    char file_name_1[buf_size];
    char file_name_2[buf_size];
    set_str_t all_names;
    vec_str_t sea_names;
    vec_str_t land_names;
    snprintf(file_name_1, buf_size, "%s.txt", MFactions[faction_id].filename);
    snprintf(file_name_2, buf_size, "basenames\\%s.txt", MFactions[faction_id].filename);
    strlwr(file_name_1);
    strlwr(file_name_2);

    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            all_names.insert(Bases[i].name);
        }
    }
    if (sea_base) {
        reader_path(sea_names, file_name_1, "#WATERBASES", MaxBaseNameLen);
        reader_path(sea_names, file_name_2, "#WATERBASES", MaxBaseNameLen);

        if (sea_names.size() > 0 && offset_sea < sea_names.size()) {
            for (uint32_t i = 0; i < sea_names.size(); i++) {
                uint32_t seed = offset_next(faction_id, offset_sea + i, sea_names.size());
                if (seed < sea_names.size()) {
                    vec_str_t::const_iterator it(sea_names.begin());
                    std::advance(it, seed);
                    if (!has_item(all_names, it->c_str())) {
                        strcpy_n(name, MaxBaseNameLen, it->c_str());
                        if (save_offset) {
                            f.base_sea_name_offset++;
                        }
                        return;
                    }
                }
            }
        }
    }
    land_names.clear();
    reader_path(land_names, file_name_1, "#BASES", MaxBaseNameLen);
    reader_path(land_names, file_name_2, "#BASES", MaxBaseNameLen);

    if (save_offset) {
        f.base_name_offset++;
    }
    if (land_names.size() > 0 && offset < land_names.size()) {
        for (uint32_t i = 0; i < land_names.size(); i++) {
            uint32_t seed = offset_next(faction_id, offset + i, land_names.size());
            if (seed < land_names.size()) {
                vec_str_t::const_iterator it(land_names.begin());
                std::advance(it, seed);
                if (!has_item(all_names, it->c_str())) {
                    strcpy_n(name, MaxBaseNameLen, it->c_str());
                    return;
                }
            }
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        all_names.insert(Bases[i].name);
    }
    uint32_t x = 0;
    uint32_t a = 0;
    uint32_t b = 0;

    land_names.clear();
    sea_names.clear();
    reader_path(land_names, "basenames\\generic.txt", "#BASESA", MaxBaseNameLen);
    if (land_names.size() > 0) {
        reader_path(sea_names, "basenames\\generic.txt", "#BASESB", MaxBaseNameLen);
    } else {
        reader_path(land_names, "basename.txt", "#GENERIC", MaxBaseNameLen);
    }

    for (int i = 0; i < 2*MaxBaseNum && land_names.size() > 0; i++) {
        x = pair_hash(faction_id + MaxPlayerNum*f.base_name_offset, *MapRandomSeed + i);
        a = ((x & 0xffff) * land_names.size()) >> 16;
        name[0] = '\0';

        if (sea_names.size() > 0) {
            b = ((x >> 16) * sea_names.size()) >> 16;
            snprintf(name, MaxBaseNameLen, "%s %s", land_names[a].c_str(), sea_names[b].c_str());
        } else {
            snprintf(name, MaxBaseNameLen, "%s", land_names[a].c_str());
        }
        if (strlen(name) >= 2 && !all_names.count(name)) {
            return;
        }
    }
    for (int i = *BaseCount + 1; i <= 2*MaxBaseNum; i++) {
        name[0] = '\0';
        snprintf(name, MaxBaseNameLen, "Sector %d", i);
        if (!all_names.count(name)) {
            return;
        }
    }
}

/*
Game engine uses these defaults for ambient music tracks.
This patches load_music comparison function such that it maps
the user config to any available music tracks.
If none of the config values match current faction, aset1.amb is used.

DEFAULT, aset1.amb
GAIANS, gset1.amb
HIVE, mset1.amb
UNIV, uset2.amb
MORGAN, mset1.amb
SPARTANS, sset1.amb
BELIEVE, bset1.amb
PEACE, gset1.amb
*/

int __cdecl load_music_strcmpi(const char* active, const char* label)
{
    bool fallback = true;
    char lookup[StrBufLen] = {};

    for (const auto& pair : musiclabels) {
        if (!strcmpi(pair.first.c_str(), active)) {
            if (!strcmpi(pair.second.c_str(), "gset1.amb")) {
                strncpy(lookup, "gaians", StrBufLen);
            } else if (!strcmpi(pair.second.c_str(), "mset1.amb")) {
                strncpy(lookup, "hive", StrBufLen);
            } else if (!strcmpi(pair.second.c_str(), "uset2.amb")) {
                strncpy(lookup, "univ", StrBufLen);
            } else if (!strcmpi(pair.second.c_str(), "sset1.amb")) {
                strncpy(lookup, "spartans", StrBufLen);
            } else if (!strcmpi(pair.second.c_str(), "bset1.amb")) {
                strncpy(lookup, "believe", StrBufLen);
            } else {
                break; // aset1.amb or incorrect filename is used
            }
            fallback = false;
            break;
        }
    }
    debug("load_music %d %s %s %s\n", fallback, active, label, lookup);
    flushlog();
    if (fallback) {
        return 1; // Skip all other choices and select original fallback default
    }
    return strcmpi(label, lookup);
}


