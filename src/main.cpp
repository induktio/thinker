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
NodeSet mapnodes;
AIPlans plans[MaxPlayerNum];

const char* landmark_params[] = {
    "crater", "volcano", "jungle", "uranium",
    "sargasso", "ruins", "dunes", "fresh",
    "mesa", "canyon", "geothermal", "ridge",
    "borehole", "nexus", "unity", "fossil"
};

int option_handler(void* user, const char* section, const char* name, const char* value) {
    static bool unknown_option = false;
    char buf[INI_MAX_LINE+2] = {};
    Config* cf = (Config*)user;
    strncpy(buf, value, INI_MAX_LINE);
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("thinker", "DirectDraw")) {
        cf->directdraw = atoi(value);
    } else if (MATCH("thinker", "DisableOpeningMovie")) {
        cf->disable_opening_movie = atoi(value);
    } else if (MATCH("thinker", "autosave_interval")) {
        cf->autosave_interval = atoi(value);
    } else if (MATCH("thinker", "smooth_scrolling")) {
        cf->smooth_scrolling = atoi(value);
    } else if (MATCH("thinker", "scroll_area")) {
        cf->scroll_area = max(0, atoi(value));
    } else if (MATCH("thinker", "render_base_info")) {
        cf->render_base_info = atoi(value);
    } else if (MATCH("thinker", "render_high_detail")) {
        cf->render_high_detail = atoi(value);
    } else if (MATCH("thinker", "manage_player_bases")) {
        cf->manage_player_bases = atoi(value);
    } else if (MATCH("thinker", "manage_player_units")) {
        cf->manage_player_units = atoi(value);
    } else if (MATCH("thinker", "warn_on_former_replace")) {
        cf->warn_on_former_replace = atoi(value);
    } else if (MATCH("thinker", "render_probe_labels")) {
        cf->render_probe_labels = atoi(value);
    } else if (MATCH("thinker", "foreign_treaty_popup")) {
        cf->foreign_treaty_popup = atoi(value);
    } else if (MATCH("thinker", "editor_free_units")) {
        cf->editor_free_units = atoi(value);
    } else if (MATCH("thinker", "new_base_names")) {
        cf->new_base_names = atoi(value);
    } else if (MATCH("thinker", "new_unit_names")) {
        cf->new_unit_names = atoi(value);
    } else if (MATCH("thinker", "windowed")) {
        cf->windowed = atoi(value);
    } else if (MATCH("thinker", "window_width")) {
        cf->window_width = max(800, atoi(value));
    } else if (MATCH("thinker", "window_height")) {
        cf->window_height = max(600, atoi(value));
    } else if (MATCH("thinker", "smac_only")) {
        cf->smac_only = atoi(value);
    } else if (MATCH("thinker", "player_colony_pods")) {
        cf->player_colony_pods = atoi(value);
    } else if (MATCH("thinker", "computer_colony_pods")) {
        cf->computer_colony_pods = atoi(value);
    } else if (MATCH("thinker", "player_formers")) {
        cf->player_formers = atoi(value);
    } else if (MATCH("thinker", "computer_formers")) {
        cf->computer_formers = atoi(value);
    } else if (MATCH("thinker", "player_satellites")) {
        opt_list_parse(cf->player_satellites, buf, 3, 0);
    } else if (MATCH("thinker", "computer_satellites")) {
        opt_list_parse(cf->computer_satellites, buf, 3, 0);
    } else if (MATCH("thinker", "design_units")) {
        cf->design_units = atoi(value);
    } else if (MATCH("thinker", "factions_enabled")) {
        cf->factions_enabled = atoi(value);
    } else if (MATCH("thinker", "social_ai")) {
        cf->social_ai = atoi(value);
    } else if (MATCH("thinker", "social_ai_bias")) {
        cf->social_ai = clamp(atoi(value), 0, 1000);
    } else if (MATCH("thinker", "tech_balance")) {
        cf->tech_balance = atoi(value);
    } else if (MATCH("thinker", "base_hurry")) {
        cf->base_hurry = atoi(value);
    } else if (MATCH("thinker", "base_spacing")) {
        cf->base_spacing = clamp(atoi(value), 2, 8);
    } else if (MATCH("thinker", "base_nearby_limit")) {
        cf->base_nearby_limit = atoi(value);
    } else if (MATCH("thinker", "expansion_limit")) {
        cf->expansion_limit = atoi(value);
    } else if (MATCH("thinker", "expansion_autoscale")) {
        cf->expansion_autoscale = atoi(value);
    } else if (MATCH("thinker", "conquer_priority")) {
        cf->conquer_priority = clamp(atoi(value), 1, 10000);
    } else if (MATCH("thinker", "crawler_priority")) {
        cf->crawler_priority = clamp(atoi(value), 1, 10000);
    } else if (MATCH("thinker", "max_satellites")) {
        cf->max_satellites = max(0, atoi(value));
    } else if (MATCH("thinker", "new_world_builder")) {
        cf->new_world_builder = atoi(value);
    } else if (MATCH("thinker", "world_continents")) {
        cf->world_continents = atoi(value);
    } else if (MATCH("thinker", "world_polar_caps")) {
        cf->world_polar_caps = atoi(value);
    } else if (MATCH("thinker", "world_hills_mod")) {
        cf->world_hills_mod = clamp(atoi(value), 0, 100);
    } else if (MATCH("thinker", "world_ocean_mod")) {
        cf->world_ocean_mod = clamp(atoi(value), 0, 100);
    } else if (MATCH("thinker", "world_islands_mod")) {
        cf->world_islands_mod = atoi(value);
    } else if (MATCH("thinker", "modified_landmarks")) {
        cf->modified_landmarks = atoi(value);
    } else if (MATCH("thinker", "map_mirror_x")) {
        cf->map_mirror_x = atoi(value);
    } else if (MATCH("thinker", "map_mirror_y")) {
        cf->map_mirror_y = atoi(value);
    } else if (MATCH("thinker", "world_sea_levels")) {
        opt_list_parse(cf->world_sea_levels, buf, 3, 0);
    } else if (MATCH("thinker", "time_warp_mod")) {
        cf->time_warp_mod = atoi(value);
    } else if (MATCH("thinker", "time_warp_techs")) {
        cf->time_warp_techs = atoi(value);
    } else if (MATCH("thinker", "time_warp_projects")) {
        cf->time_warp_projects = atoi(value);
    } else if (MATCH("thinker", "faction_placement")) {
        cf->faction_placement = atoi(value);
    } else if (MATCH("thinker", "nutrient_bonus")) {
        cf->nutrient_bonus = atoi(value);
    } else if (MATCH("thinker", "rare_supply_pods")) {
        cf->rare_supply_pods = atoi(value);
    } else if (MATCH("thinker", "simple_cost_factor")) {
        cf->simple_cost_factor = atoi(value);
    } else if (MATCH("thinker", "revised_tech_cost")) {
        cf->revised_tech_cost = atoi(value);
    } else if (MATCH("thinker", "cheap_early_tech")) {
        cf->cheap_early_tech = atoi(value);
    } else if (MATCH("thinker", "tech_stagnate_rate")) {
        cf->tech_stagnate_rate = max(1, atoi(value));
    } else if (MATCH("thinker", "fast_fungus_movement")) {
        cf->fast_fungus_movement = atoi(value);
    } else if (MATCH("thinker", "magtube_movement_rate")) {
        cf->magtube_movement_rate = atoi(value);
    } else if (MATCH("thinker", "chopper_attack_rate")) {
        cf->chopper_attack_rate = atoi(value);
    } else if (MATCH("thinker", "nerve_staple")) {
        cf->nerve_staple = atoi(value);
    } else if (MATCH("thinker", "nerve_staple_mod")) {
        cf->nerve_staple_mod = atoi(value);
    } else if (MATCH("thinker", "delay_drone_riots")) {
        cf->delay_drone_riots = atoi(value);
    } else if (MATCH("thinker", "skip_drone_revolts")) {
        cf->skip_drone_revolts = atoi(value);
    } else if (MATCH("thinker", "activate_skipped_units")) {
        cf->activate_skipped_units = atoi(value);
    } else if (MATCH("thinker", "counter_espionage")) {
        cf->counter_espionage = atoi(value);
    } else if (MATCH("thinker", "ignore_reactor_power")) {
        cf->ignore_reactor_power = atoi(value);
    } else if (MATCH("thinker", "modify_unit_morale")) {
        cf->modify_unit_morale = atoi(value);
    } else if (MATCH("thinker", "skip_default_balance")) {
        cf->skip_default_balance = atoi(value);
    } else if (MATCH("thinker", "early_research_start")) {
        cf->early_research_start = atoi(value);
    } else if (MATCH("thinker", "facility_capture_fix")) {
        cf->facility_capture_fix = atoi(value);
    } else if (MATCH("thinker", "territory_border_fix")) {
        cf->territory_border_fix = atoi(value);
    } else if (MATCH("thinker", "auto_relocate_hq")) {
        cf->auto_relocate_hq = atoi(value);
    } else if (MATCH("thinker", "simple_hurry_cost")) {
        cf->simple_hurry_cost = atoi(value);
    } else if (MATCH("thinker", "eco_damage_fix")) {
        cf->eco_damage_fix = atoi(value);
    } else if (MATCH("thinker", "clean_minerals")) {
        cf->clean_minerals = clamp(atoi(value), 0, 127);
    } else if (MATCH("thinker", "spawn_fungal_towers")) {
        cf->spawn_fungal_towers = atoi(value);
    } else if (MATCH("thinker", "spawn_spore_launchers")) {
        cf->spawn_spore_launchers = atoi(value);
    } else if (MATCH("thinker", "spawn_sealurks")) {
        cf->spawn_sealurks = atoi(value);
    } else if (MATCH("thinker", "spawn_battle_ogres")) {
        cf->spawn_battle_ogres = atoi(value);
    } else if (MATCH("thinker", "planetpearls")) {
        cf->planetpearls = atoi(value);
    } else if (MATCH("thinker", "aquatic_bonus_minerals")) {
        cf->aquatic_bonus_minerals = atoi(value);
    } else if (MATCH("thinker", "alien_guaranteed_techs")) {
        cf->alien_guaranteed_techs = atoi(value);
    } else if (MATCH("thinker", "alien_early_start")) {
        cf->alien_early_start = atoi(value);
    } else if (MATCH("thinker", "cult_early_start")) {
        cf->cult_early_start = atoi(value);
    } else if (MATCH("thinker", "natives_weak_until_turn")) {
        cf->natives_weak_until_turn = clamp(atoi(value), 0, 127);
    } else if (MATCH("thinker", "neural_amplifier_bonus")) {
        cf->neural_amplifier_bonus = clamp(atoi(value), 0, 1000);
    } else if (MATCH("thinker", "dream_twister_bonus")) {
        cf->dream_twister_bonus = clamp(atoi(value), 0, 1000);
    } else if (MATCH("thinker", "fungal_tower_bonus")) {
        cf->fungal_tower_bonus = clamp(atoi(value), 0, 1000);
    } else if (MATCH("thinker", "perimeter_defense_bonus")) {
        cf->perimeter_defense_bonus = clamp(atoi(value), 0, 127);
    } else if (MATCH("thinker", "tachyon_field_bonus")) {
        cf->tachyon_field_bonus = clamp(atoi(value), 0, 127);
    } else if (MATCH("thinker", "collateral_damage_value")) {
        cf->collateral_damage_value = clamp(atoi(value), 0, 127);
    } else if (MATCH("thinker", "cost_factor")) {
        opt_list_parse(CostRatios, buf, MaxDiffNum, 1);
    } else if (MATCH("thinker", "tech_cost_factor")) {
        opt_list_parse(cf->tech_cost_factor, buf, MaxDiffNum, 1);
    } else if (MATCH("thinker", "content_pop_player")) {
        opt_list_parse(cf->content_pop_player, buf, MaxDiffNum, 0);
    } else if (MATCH("thinker", "content_pop_computer")) {
        opt_list_parse(cf->content_pop_computer, buf, MaxDiffNum, 0);
    } else if (MATCH("thinker", "repair_minimal")) {
        cf->repair_minimal = clamp(atoi(value), 0, 10);
    } else if (MATCH("thinker", "repair_fungus")) {
        cf->repair_fungus = clamp(atoi(value), 0, 10);
    } else if (MATCH("thinker", "repair_friendly")) {
        cf->repair_friendly = atoi(value);
    } else if (MATCH("thinker", "repair_airbase")) {
        cf->repair_airbase = atoi(value);
    } else if (MATCH("thinker", "repair_bunker")) {
        cf->repair_bunker = atoi(value);
    } else if (MATCH("thinker", "repair_base")) {
        cf->repair_base = clamp(atoi(value), 0, 10);
    } else if (MATCH("thinker", "repair_base_native")) {
        cf->repair_base_native = clamp(atoi(value), 0, 10);
    } else if (MATCH("thinker", "repair_base_facility")) {
        cf->repair_base_facility = clamp(atoi(value), 0, 10);
    } else if (MATCH("thinker", "repair_nano_factory")) {
        cf->repair_nano_factory = clamp(atoi(value), 0, 10);
    } else if (MATCH("thinker", "cpu_idle_fix")) {
        cf->cpu_idle_fix = atoi(value);
    } else if (MATCH("thinker", "minimal_popups")) {
        if (DEBUG) {
            cf->minimal_popups = atoi(value);
            cf->debug_verbose = !atoi(value);
        }
    } else if (MATCH("thinker", "skip_faction")) {
        if (atoi(value) > 0) {
            cf->skip_random_factions |= 1 << (atoi(value) - 1);
        }
    } else {
        for (int i=0; i<16; i++) {
            if (MATCH("thinker", landmark_params[i])) {
                cf->landmarks &= ~((atoi(value) ? 0 : 1) << i);
                return 1;
            }
        }
        if (!unknown_option) {
            char msg[1024] = {};
            snprintf(msg, sizeof(msg),
                "Unknown configuration option detected in thinker.ini.\n"
                "Game might not work as intended.\n"
                "Header: %s\n"
                "Option: %s\n",
                section, name);
            MessageBoxA(0, msg, MOD_VERSION, MB_OK | MB_ICONWARNING);
        }
        unknown_option = true;
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

int opt_list_parse(int* ptr, char* buf, int len, int min_val) {
    const char *d=",";
    char *s, *p;
    p = strtok_r(buf, d, &s);
    for (int i=0; i<len && p != NULL; i++, p = strtok_r(NULL, d, &s)) {
        ptr[i] = max(min_val, atoi(p));
    }
    return 1;
}

int cmd_parse(Config* cf) {
    int argc;
    LPWSTR* argv;
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i=1; i<argc; i++) {
        if (wcscmp(argv[i], L"-smac") == 0) {
            cf->smac_only = 1;
        } else if (wcscmp(argv[i], L"-windowed") == 0) {
            cf->windowed = 1;
        }
    }
    return 1;
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
                exit(EXIT_FAILURE);
            }
            if (ini_parse("thinker.ini", option_handler, &conf) < 0) {
                MessageBoxA(0, "Error while opening thinker.ini file.",
                    MOD_VERSION, MB_OK | MB_ICONSTOP);
                exit(EXIT_FAILURE);
            }
            if (!cmd_parse(&conf) || !patch_setup(&conf)) {
                MessageBoxA(0, "Error while loading the game.",
                    MOD_VERSION, MB_OK | MB_ICONSTOP);
                exit(EXIT_FAILURE);
            }
            random_reseed(GetTickCount());
            *EngineVersion = MOD_VERSION;
            *EngineDate = MOD_DATE;
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



