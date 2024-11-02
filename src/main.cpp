/*
 * Thinker - AI improvement mod for Sid Meier's Alpha Centauri.
 * https://github.com/induktio/thinker/
 *
 * Thinker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) version 3 of the GPL.
 *
 * Thinker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Thinker.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "main.h"
#include "lib/ini.h"

FILE* debug_log = NULL;
Config conf;
AIPlans plans[MaxPlayerNum];
set_str_t movedlabels;
map_str_t musiclabels;


int option_handler(void* user, const char* section, const char* name, const char* value) {
    #define MATCH(n) strcmp(name, n) == 0
    char buf[INI_MAX_LINE] = {};
    Config* cf = (Config*)user;
    strncpy(buf, value, INI_MAX_LINE);
    buf[INI_MAX_LINE-1] = '\0';

    if (strcmp(section, "thinker") != 0) {
        return opt_handle_error(section, name);
    } else if (MATCH("DirectDraw")) {
        cf->directdraw = atoi(value);
    } else if (MATCH("DisableOpeningMovie")) {
        cf->disable_opening_movie = atoi(value);
    } else if (MATCH("video_mode")) {
        cf->video_mode = clamp(atoi(value), 0, 2);
    } else if (MATCH("window_width")) {
        cf->window_width = atoi(value);
    } else if (MATCH("window_height")) {
        cf->window_height = atoi(value);
    } else if (MATCH("smac_only")) {
        cf->smac_only = atoi(value);
    } else if (MATCH("smooth_scrolling")) {
        cf->smooth_scrolling = atoi(value);
    } else if (MATCH("scroll_area")) {
        cf->scroll_area = max(0, atoi(value));
    } else if (MATCH("auto_minimise")) {
        cf->auto_minimise = atoi(value);
    } else if (MATCH("render_base_info")) {
        cf->render_base_info = atoi(value);
    } else if (MATCH("render_high_detail")) {
        cf->render_high_detail = atoi(value);
    } else if (MATCH("autosave_interval")) {
        cf->autosave_interval = atoi(value);
    } else if (MATCH("warn_on_former_replace")) {
        cf->warn_on_former_replace = atoi(value);
    } else if (MATCH("manage_player_bases")) {
        cf->manage_player_bases = atoi(value);
    } else if (MATCH("manage_player_units")) {
        cf->manage_player_units = atoi(value);
    } else if (MATCH("render_probe_labels")) {
        cf->render_probe_labels = atoi(value);
    } else if (MATCH("foreign_treaty_popup")) {
        cf->foreign_treaty_popup = atoi(value);
    } else if (MATCH("editor_free_units")) {
        cf->editor_free_units = atoi(value);
    } else if (MATCH("new_base_names")) {
        cf->new_base_names = atoi(value);
    } else if (MATCH("new_unit_names")) {
        cf->new_unit_names = atoi(value);
    } else if (MATCH("player_colony_pods")) {
        cf->player_colony_pods = atoi(value);
    } else if (MATCH("computer_colony_pods")) {
        cf->computer_colony_pods = atoi(value);
    } else if (MATCH("player_formers")) {
        cf->player_formers = atoi(value);
    } else if (MATCH("computer_formers")) {
        cf->computer_formers = atoi(value);
    } else if (MATCH("player_satellites")) {
        opt_list_parse(cf->player_satellites, buf, 3, 0);
    } else if (MATCH("computer_satellites")) {
        opt_list_parse(cf->computer_satellites, buf, 3, 0);
    } else if (MATCH("design_units")) {
        cf->design_units = atoi(value);
    } else if (MATCH("factions_enabled")) {
        cf->factions_enabled = atoi(value);
    } else if (MATCH("social_ai")) {
        cf->social_ai = atoi(value);
    } else if (MATCH("social_ai_bias")) {
        cf->social_ai_bias = clamp(atoi(value), 0, 1000);
    } else if (MATCH("tech_balance")) {
        cf->tech_balance = atoi(value);
    } else if (MATCH("base_hurry")) {
        cf->base_hurry = atoi(value);
    } else if (MATCH("base_spacing")) {
        cf->base_spacing = clamp(atoi(value), 2, 8);
    } else if (MATCH("base_nearby_limit")) {
        cf->base_nearby_limit = atoi(value);
    } else if (MATCH("expansion_limit")) {
        cf->expansion_limit = atoi(value);
    } else if (MATCH("expansion_autoscale")) {
        cf->expansion_autoscale = atoi(value);
    } else if (MATCH("limit_project_start")) {
        cf->limit_project_start = atoi(value);
    } else if (MATCH("max_satellites")) {
        cf->max_satellites = max(0, atoi(value));
    } else if (MATCH("new_world_builder")) {
        cf->new_world_builder = atoi(value);
    } else if (MATCH("world_continents")) {
        cf->world_continents = atoi(value);
    } else if (MATCH("world_polar_caps")) {
        cf->world_polar_caps = atoi(value);
    } else if (MATCH("world_hills_mod")) {
        cf->world_hills_mod = clamp(atoi(value), 0, 100);
    } else if (MATCH("world_ocean_mod")) {
        cf->world_ocean_mod = clamp(atoi(value), 0, 100);
    } else if (MATCH("world_islands_mod")) {
        cf->world_islands_mod = atoi(value);
    } else if (MATCH("world_mirror_x")) {
        cf->world_mirror_x = atoi(value);
    } else if (MATCH("world_mirror_y")) {
        cf->world_mirror_y = atoi(value);
    } else if (MATCH("modified_landmarks")) {
        cf->modified_landmarks = atoi(value);
    } else if (MATCH("world_sea_levels")) {
        opt_list_parse(cf->world_sea_levels, buf, 3, 0);
    } else if (MATCH("time_warp_mod")) {
        cf->time_warp_mod = atoi(value);
    } else if (MATCH("time_warp_techs")) {
        cf->time_warp_techs = atoi(value);
    } else if (MATCH("time_warp_projects")) {
        cf->time_warp_projects = atoi(value);
    } else if (MATCH("time_warp_start_turn")) {
        cf->time_warp_start_turn = clamp(atoi(value), 0, 500);
    } else if (MATCH("faction_placement")) {
        cf->faction_placement = atoi(value);
    } else if (MATCH("nutrient_bonus")) {
        cf->nutrient_bonus = atoi(value);
    } else if (MATCH("rare_supply_pods")) {
        cf->rare_supply_pods = atoi(value);
    } else if (MATCH("simple_cost_factor")) {
        cf->simple_cost_factor = atoi(value);
    } else if (MATCH("revised_tech_cost")) {
        cf->revised_tech_cost = atoi(value);
    } else if (MATCH("tech_stagnate_rate")) {
        cf->tech_stagnate_rate = max(1, atoi(value));
    } else if (MATCH("fast_fungus_movement")) {
        cf->fast_fungus_movement = atoi(value);
    } else if (MATCH("magtube_movement_rate")) {
        cf->magtube_movement_rate = atoi(value);
    } else if (MATCH("chopper_attack_rate")) {
        cf->chopper_attack_rate = atoi(value);
    } else if (MATCH("base_psych")) {
        cf->base_psych = atoi(value);
    } else if (MATCH("nerve_staple")) {
        cf->nerve_staple = atoi(value);
    } else if (MATCH("nerve_staple_mod")) {
        cf->nerve_staple_mod = atoi(value);
    } else if (MATCH("delay_drone_riots")) {
        cf->delay_drone_riots = atoi(value);
    } else if (MATCH("skip_drone_revolts")) {
        cf->skip_drone_revolts = atoi(value);
    } else if (MATCH("activate_skipped_units")) {
        cf->activate_skipped_units = atoi(value);
    } else if (MATCH("counter_espionage")) {
        cf->counter_espionage = atoi(value);
    } else if (MATCH("ignore_reactor_power")) {
        cf->ignore_reactor_power = atoi(value);
    } else if (MATCH("long_range_artillery")) {
        cf->long_range_artillery = atoi(value);
    } else if (MATCH("modify_upgrade_cost")) {
        cf->modify_upgrade_cost = atoi(value);
    } else if (MATCH("modify_unit_support")) {
        cf->modify_unit_support = atoi(value);
    } else if (MATCH("skip_default_balance")) {
        cf->skip_default_balance = atoi(value);
    } else if (MATCH("early_research_start")) {
        cf->early_research_start = atoi(value);
    } else if (MATCH("facility_capture_fix")) {
        cf->facility_capture_fix = atoi(value);
    } else if (MATCH("territory_border_fix")) {
        cf->territory_border_fix = atoi(value);
    } else if (MATCH("facility_free_tech")) {
        cf->facility_free_tech = atoi(value);
    } else if (MATCH("auto_relocate_hq")) {
        cf->auto_relocate_hq = atoi(value);
    } else if (MATCH("simple_hurry_cost")) {
        cf->simple_hurry_cost = atoi(value);
    } else if (MATCH("eco_damage_fix")) {
        cf->eco_damage_fix = atoi(value);
    } else if (MATCH("clean_minerals")) {
        cf->clean_minerals = clamp(atoi(value), 0, 127);
    } else if (MATCH("biology_lab_bonus")) {
        cf->biology_lab_bonus = clamp(atoi(value), 0, 127);
    } else if (MATCH("spawn_fungal_towers")) {
        cf->spawn_fungal_towers = atoi(value);
    } else if (MATCH("spawn_spore_launchers")) {
        cf->spawn_spore_launchers = atoi(value);
    } else if (MATCH("spawn_sealurks")) {
        cf->spawn_sealurks = atoi(value);
    } else if (MATCH("spawn_battle_ogres")) {
        cf->spawn_battle_ogres = atoi(value);
    } else if (MATCH("planetpearls")) {
        cf->planetpearls = atoi(value);
    } else if (MATCH("event_perihelion")) {
        cf->event_perihelion = atoi(value);
    } else if (MATCH("event_sunspots")) {
        cf->event_sunspots = clamp(atoi(value), 0, 100);
    } else if (MATCH("event_market_crash")) {
        cf->event_market_crash = atoi(value);
    } else if (MATCH("soil_improve_value")) {
        cf->soil_improve_value = clamp(atoi(value), 0, 10);
    } else if (MATCH("aquatic_bonus_minerals")) {
        cf->aquatic_bonus_minerals = atoi(value);
    } else if (MATCH("alien_guaranteed_techs")) {
        cf->alien_guaranteed_techs = atoi(value);
    } else if (MATCH("alien_early_start")) {
        cf->alien_early_start = atoi(value);
    } else if (MATCH("cult_early_start")) {
        cf->cult_early_start = atoi(value);
    } else if (MATCH("normal_elite_moves")) {
        cf->normal_elite_moves = atoi(value);
    } else if (MATCH("native_elite_moves")) {
        cf->native_elite_moves = atoi(value);
    } else if (MATCH("native_weak_until_turn")) {
        cf->native_weak_until_turn = clamp(atoi(value), 0, 127);
    } else if (MATCH("native_lifecycle_levels")) {
        opt_list_parse(cf->native_lifecycle_levels, buf, 6, 0);
    } else if (MATCH("facility_defense_bonus")) {
        opt_list_parse(cf->facility_defense_bonus, buf, 4, 0);
    } else if (MATCH("neural_amplifier_bonus")) {
        cf->neural_amplifier_bonus = clamp(atoi(value), 0, 1000);
    } else if (MATCH("dream_twister_bonus")) {
        cf->dream_twister_bonus = clamp(atoi(value), 0, 1000);
    } else if (MATCH("fungal_tower_bonus")) {
        cf->fungal_tower_bonus = clamp(atoi(value), 0, 1000);
    } else if (MATCH("planet_defense_bonus")) {
        cf->planet_defense_bonus = atoi(value);
    } else if (MATCH("sensor_defense_ocean")) {
        cf->sensor_defense_ocean = atoi(value);
    } else if (MATCH("collateral_damage_value")) {
        cf->collateral_damage_value = clamp(atoi(value), 0, 127);
    } else if (MATCH("cost_factor")) {
        opt_list_parse(CostRatios, buf, MaxDiffNum, 1);
    } else if (MATCH("tech_cost_factor")) {
        opt_list_parse(cf->tech_cost_factor, buf, MaxDiffNum, 1);
    } else if (MATCH("content_pop_player")) {
        opt_list_parse(cf->content_pop_player, buf, MaxDiffNum, 0);
    } else if (MATCH("content_pop_computer")) {
        opt_list_parse(cf->content_pop_computer, buf, MaxDiffNum, 0);
    } else if (MATCH("repair_minimal")) {
        cf->repair_minimal = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_fungus")) {
        cf->repair_fungus = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_friendly")) {
        cf->repair_friendly = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_airbase")) {
        cf->repair_airbase = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_bunker")) {
        cf->repair_bunker = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_base")) {
        cf->repair_base = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_base_native")) {
        cf->repair_base_native = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_base_facility")) {
        cf->repair_base_facility = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_nano_factory")) {
        cf->repair_nano_factory = clamp(atoi(value), 0, 10);
    } else if (MATCH("repair_battle_ogre")) {
        cf->repair_battle_ogre = clamp(atoi(value), 0, 10);
    } else if (MATCH("minimal_popups")) {
        if (DEBUG) {
            cf->minimal_popups = atoi(value);
            cf->debug_verbose = !atoi(value);
        }
    } else if (MATCH("skip_faction")) {
        if (atoi(value) > 0) {
            cf->skip_random_factions |= 1 << (atoi(value) - 1);
        }
    } else if (MATCH("crater")) {
        cf->landmarks.crater = max(0, atoi(value));
    } else if (MATCH("volcano")) {
        cf->landmarks.volcano = max(0, atoi(value));
    } else if (MATCH("jungle")) {
        cf->landmarks.jungle = max(0, atoi(value));
    } else if (MATCH("uranium")) {
        cf->landmarks.uranium = max(0, atoi(value));
    } else if (MATCH("sargasso")) {
        cf->landmarks.sargasso = max(0, atoi(value));
    } else if (MATCH("ruins")) {
        cf->landmarks.ruins = max(0, atoi(value));
    } else if (MATCH("dunes")) {
        cf->landmarks.dunes = max(0, atoi(value));
    } else if (MATCH("fresh")) {
        cf->landmarks.fresh = max(0, atoi(value));
    } else if (MATCH("mesa")) {
        cf->landmarks.mesa = max(0, atoi(value));
    } else if (MATCH("canyon")) {
        cf->landmarks.canyon = max(0, atoi(value));
    } else if (MATCH("geothermal")) {
        cf->landmarks.geothermal = max(0, atoi(value));
    } else if (MATCH("ridge")) {
        cf->landmarks.ridge = max(0, atoi(value));
    } else if (MATCH("borehole")) {
        cf->landmarks.borehole = max(0, atoi(value));
    } else if (MATCH("nexus")) {
        cf->landmarks.nexus = max(0, atoi(value));
    } else if (MATCH("unity")) {
        cf->landmarks.unity = max(0, atoi(value));
    } else if (MATCH("fossil")) {
        cf->landmarks.fossil = max(0, atoi(value));
    } else if (MATCH("label_pop_size")) {
        parse_format_args(label_pop_size, value, 4, StrBufLen);
    } else if (MATCH("label_pop_boom")) {
        parse_format_args(label_pop_boom, value, 0, StrBufLen);
    } else if (MATCH("label_nerve_staple")) {
        parse_format_args(label_nerve_staple, value, 1, StrBufLen);
    } else if (MATCH("label_captured_base")) {
        parse_format_args(label_captured_base, value, 1, StrBufLen);
    } else if (MATCH("label_stockpile_energy")) {
        parse_format_args(label_stockpile_energy, value, 1, StrBufLen);
    } else if (MATCH("label_sat_nutrient")) {
        parse_format_args(label_sat_nutrient, value, 1, StrBufLen);
    } else if (MATCH("label_sat_mineral")) {
        parse_format_args(label_sat_mineral, value, 1, StrBufLen);
    } else if (MATCH("label_sat_energy")) {
        parse_format_args(label_sat_energy, value, 1, StrBufLen);
    } else if (MATCH("label_eco_damage")) {
        parse_format_args(label_eco_damage, value, 2, StrBufLen);
    } else if (MATCH("label_base_surplus")) {
        parse_format_args(label_base_surplus, value, 3, StrBufLen);
    } else if (MATCH("label_unit_reactor")) {
        int len = strlen(buf);
        int j = 0;
        int k = 0;
        for (int i = 0; i < len && i < StrBufLen && k < 4; i++) {
            bool last = i == len - 1;
            if (buf[i] == ',' || last) {
                strncpy(label_unit_reactor[k], buf+j, i-j+last);
                label_unit_reactor[k][i-j+last] = '\0';
                j = i + 1;
                k++;
            }
        }
    } else if (MATCH("script_label")) {
        char* p = strupr(strstrip(buf));
        debug("script_label %s\n", p);
        movedlabels.insert(p);
    } else if (MATCH("music_label")) {
        char *p, *s, *k, *v;
        if ((p = strtok_r(buf, ",", &s)) != NULL) {
            k = strstrip(p);
            if ((p = strtok_r(NULL, ",", &s)) != NULL) {
                v = strstrip(p);
                if (strlen(k) && strlen(v)) {
                    debug("music_label %s = %s\n", k, v);
                    musiclabels[k] = v;
                }
            }
        }
    } else {
        return opt_handle_error(section, name);
    }
    return 1;
}

int opt_handle_error(const char* section, const char* name) {
    static bool unknown_option = false;
    char msg[1024] = {};
    if (!unknown_option) {
        snprintf(msg, sizeof(msg),
            "Unknown configuration option detected in thinker.ini.\n"
            "Game might not work as intended.\n"
            "Header: %s\n"
            "Option: %s\n",
            section, name);
        MessageBoxA(0, msg, MOD_VERSION, MB_OK | MB_ICONWARNING);
    }
    unknown_option = true;
    return 0;
}

int opt_list_parse(int32_t* dst, char* src, int num, int min_val) {
    const char *d=",";
    char *s, *p;
    p = strtok_r(src, d, &s);
    for (int i = 0; i < num && p != NULL; i++, p = strtok_r(NULL, d, &s)) {
        dst[i] = max(min_val, atoi(p));
    }
    return 1;
}

int cmd_parse(Config* cf) {
    int argc;
    LPWSTR* argv;
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-smac") == 0) {
            cf->smac_only = 1;
        } else if (wcscmp(argv[i], L"-native") == 0) {
            cf->video_mode = VM_Native;
        } else if (wcscmp(argv[i], L"-screen") == 0) {
            cf->video_mode = VM_Custom;
        } else if (wcscmp(argv[i], L"-windowed") == 0) {
            cf->video_mode = VM_Window;
        }
    }
    return 1;
}

void exit_fail(int32_t addr) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Error while patching address %08X in the game binary.\n"
        "This mod requires Alien Crossfire v2.0 terranx.exe in the same folder.", addr);
    MessageBoxA(0, buf, MOD_VERSION, MB_OK | MB_ICONSTOP);
    exit(EXIT_FAILURE);
}

void exit_fail() {
    exit(EXIT_FAILURE);
}

DLL_EXPORT DWORD ThinkerModule() {
    return 0;
}

DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE UNUSED(hinstDLL), DWORD fdwReason, LPVOID UNUSED(lpvReserved)) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            if (DEBUG && !(debug_log = fopen("debug.txt", "w"))) {
                MessageBoxA(0, "Error while opening debug.txt file.",
                    MOD_VERSION, MB_OK | MB_ICONSTOP);
                exit_fail();
            }
            if (ini_parse("thinker.ini", option_handler, &conf) < 0) {
                MessageBoxA(0, "Error while opening thinker.ini file.",
                    MOD_VERSION, MB_OK | MB_ICONSTOP);
                exit_fail();
            }
            if (!cmd_parse(&conf) || !patch_setup(&conf)) {
                MessageBoxA(0, "Error while loading the game.",
                    MOD_VERSION, MB_OK | MB_ICONSTOP);
                exit_fail();
            }
            *EngineVersion = MOD_VERSION;
            *EngineDate = MOD_DATE;
            random_reseed(GetTickCount());
            debug("random_reseed %u\n", random_state());
            flushlog();
            break;

        case DLL_PROCESS_DETACH:
            fclose(debug_log);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}



