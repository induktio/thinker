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
    } else if (MATCH("thinker", "world_map_labels")) {
        cf->world_map_labels = atoi(value);
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
    } else if (MATCH("thinker", "new_base_names")) {
        cf->new_base_names = atoi(value);
    } else if (MATCH("thinker", "windowed")) {
        cf->windowed = atoi(value);
    } else if (MATCH("thinker", "window_width")) {
        cf->window_width = max(800, atoi(value));
    } else if (MATCH("thinker", "window_height")) {
        cf->window_height = max(600, atoi(value));
    } else if (MATCH("thinker", "smac_only")) {
        cf->smac_only = atoi(value);
    } else if (MATCH("thinker", "player_free_units")) {
        cf->player_free_units = atoi(value);
    } else if (MATCH("thinker", "free_formers")) {
        cf->free_formers = max(0, atoi(value));
    } else if (MATCH("thinker", "free_colony_pods")) {
        cf->free_colony_pods = max(0, atoi(value));
    } else if (MATCH("thinker", "satellites_nutrient")) {
        cf->satellites_nutrient = max(0, atoi(value));
    } else if (MATCH("thinker", "satellites_mineral")) {
        cf->satellites_mineral = max(0, atoi(value));
    } else if (MATCH("thinker", "satellites_energy")) {
        cf->satellites_energy = max(0, atoi(value));
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
        cf->crawler_priority = atoi(value);
    } else if (MATCH("thinker", "max_satellites")) {
        cf->max_satellites = atoi(value);
    } else if (MATCH("thinker", "new_world_builder")) {
        cf->new_world_builder = atoi(value);
    } else if (MATCH("thinker", "world_continents")) {
        cf->world_continents = atoi(value);
    } else if (MATCH("thinker", "modified_landmarks")) {
        cf->modified_landmarks = atoi(value);
    } else if (MATCH("thinker", "map_mirror_x")) {
        cf->map_mirror_x = atoi(value);
    } else if (MATCH("thinker", "map_mirror_y")) {
        cf->map_mirror_y = atoi(value);
    } else if (MATCH("thinker", "world_sea_levels")) {
        opt_list_parse(cf->world_sea_levels, buf, 3, 0);
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
    } else if (MATCH("thinker", "magtube_movement_rate")) {
        cf->magtube_movement_rate = atoi(value);
    } else if (MATCH("thinker", "counter_espionage")) {
        cf->counter_espionage = atoi(value);
    } else if (MATCH("thinker", "ignore_reactor_power")) {
        cf->ignore_reactor_power = atoi(value);
    } else if (MATCH("thinker", "former_rebase")) {
        cf->former_rebase = atoi(value);
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
    } else if (MATCH("thinker", "collateral_damage_value")) {
        cf->collateral_damage_value = clamp(atoi(value), 0, 127);
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
            char msg[500] = {};
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

DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE UNUSED(hinstDLL), DWORD fdwReason, LPVOID UNUSED(lpvReserved)) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            if (DEBUG && !(debug_log = fopen("debug.txt", "w"))) {
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

DLL_EXPORT int ThinkerDecide() {
    return 0;
}

/*
Improved Social Engineering choices feature.
*/
int social_score(int faction, int sf, int sm, int range, bool pop_boom, bool has_nexus,
int robust, int immunity, int impunity, int penalty) {
    enum {ECO, EFF, SUP, TAL, MOR, POL, GRW, PLA, PRO, IND, RES};
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    double base_ratio = min(1.0, f->base_count / min(40.0, *map_area_sq_root * 0.5));
    int w_morale = (has_project(faction, FAC_COMMAND_NEXUS) ? 2 : 0)
        + (has_project(faction, FAC_CYBORG_FACTORY) ? 2 : 0);
    int w_probe = (range < 25 && *current_turn - m->thinker_last_mc_turn < 8 ? 5 : 0);
    int sc = 0;
    int vals[11];

    if ((&f->SE_Politics)[sf] == sm) {
        /* Evaluate the current active social model. */
        memcpy(vals, &f->SE_economy, sizeof(vals));
    } else {
        /* Take the faction base social values and apply all modifiers. */
        memcpy(vals, &f->SE_economy_base, sizeof(vals));

        for (int i=0; i<4; i++) {
            int j = (sf == i ? sm : (&f->SE_Politics)[i]);
            for (int k=0; k<11; k++) {
                int val = Social[i].effects[j][k];
                if ((1 << (i*4 + j)) & impunity) {
                    val = max(0, val);
                } else if ((1 << (i*4 + j)) & penalty) {
                    val = val * (val < 0 ? 2 : 1);
                }
                vals[k] += val;
            }
        }
        if (has_project(faction, FAC_ASCETIC_VIRTUES)) {
            vals[POL] += 1;
        }
        if (has_project(faction, FAC_LIVING_REFINERY)) {
            vals[SUP] += 2;
        }
        if (has_nexus) {
            if (is_alien(faction)) {
                vals[RES]++;
            }
            vals[PLA]++;
        }
        for (int k=0; k<11; k++) {
            if ((1 << k) & immunity) {
                vals[k] = max(0, vals[k]);
            } else if ((1 << k) & robust && vals[k] < 0) {
                vals[k] /= 2;
            }
        }
    }
    if (m->soc_priority_category >= 0 && m->soc_priority_model >= 0) {
        if (sf == m->soc_priority_category) {
            if (sm == m->soc_priority_model) {
                sc += conf.social_ai_bias;
            } else if (sm != SOCIAL_M_FRONTIER) {
                sc -= conf.social_ai_bias;
            }
        } else {
            if ((&f->SE_Politics)[m->soc_priority_category] == m->soc_priority_model) {
                sc += conf.social_ai_bias;
            } else if ((&f->SE_Politics)[m->soc_priority_category] != SOCIAL_M_FRONTIER) {
                sc -= conf.social_ai_bias;
            }
        }
    }
    if (vals[ECO] >= 2) {
        sc += (vals[ECO] >= 4 ? 16 : 12);
    }
    if (vals[EFF] < -2) {
        sc -= (vals[EFF] < -3 ? 20 : 14);
    }
    if (vals[SUP] < -3) {
        sc -= 10;
    }
    if (vals[SUP] < 0) {
        sc -= (int)((1.0 - base_ratio) * 10.0);
    }
    if (vals[MOR] >= 1 && vals[MOR] + w_morale >= 4) {
        sc += 10;
    }
    if (vals[PRO] >= 3 && !has_project(faction, FAC_HUNTER_SEEKER_ALGO)) {
        sc += 4 * max(0, 4 - range/8);
    }
    sc += max(2, 2 + 4*f->AI_wealth + 3*f->AI_tech - f->AI_fight)
        * clamp(vals[ECO], -3, 5);
    sc += max(2, 2*(f->AI_wealth + f->AI_tech) - f->AI_fight + (int)(4*base_ratio))
        * (min(6, vals[EFF]) + (vals[EFF] >= 3 ? 2 : 0));
    sc += max(3, 3 + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals[SUP], -4, 3);
    sc += max(2, 4 - range/8 + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals[MOR], -4, 4);

    if (!has_project(faction, FAC_TELEPATHIC_MATRIX)) {
        sc += (vals[POL] < 0 ? 2 : 4) * clamp(vals[POL], -5, 3);
        if (vals[POL] < -2) {
            sc -= (vals[POL] < -3 ? 4 : 2) * max(0, 4 - range/8)
                * (has_aircraft(faction) ? 2 : 1);
        }
        if (has_project(faction, FAC_LONGEVITY_VACCINE) && sf == SOCIAL_C_ECONOMICS) {
            sc += (sm == SOCIAL_M_PLANNED ? 10 : 0);
            sc += (sm == SOCIAL_M_SIMPLE || sm == SOCIAL_M_GREEN ? 5 : 0);
        }
        sc += 3*clamp(vals[TAL], -5, 5);

        int drone_score = 3 + (m->rule_drone > 0) - (m->rule_talent > 0)
            - (has_tech(faction, Facility[FAC_PUNISHMENT_SPHERE].preq_tech) ? 1 : 0);
        if (*SunspotDuration > 5 && *diff_level >= DIFF_LIBRARIAN
        && un_charter() && vals[POL] >= 0) {
            sc += 3*drone_score;
        }
        if (!un_charter() && vals[POL] >= 0) {
            sc += 2*drone_score;
        }
    }
    if (!has_project(faction, FAC_CLONING_VATS)) {
        if (pop_boom && vals[GRW] >= 4) {
            sc += 20;
        }
        if (vals[GRW] < -2) {
            sc -= 10;
        }
        sc += (pop_boom ? 6 : 3) * clamp(vals[GRW], -3, 6);
    }
    // Negative planet values reduce fungus yield
    if (plans[faction].keep_fungus) {
        sc += 3*clamp(vals[PLA], -3, 0);
    }
    sc += max(2, (f->SE_planet_base > 0 ? 5 : 2) + m->rule_psi/10
        + (has_project(faction, FAC_MANIFOLD_HARMONICS) ? 6 : 0)) * clamp(vals[PLA], -3, 3);
    sc += max(2, 5 - range/8 + w_probe + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals[PRO], -2, 3);
    sc += 8 * clamp(vals[IND], -3, 8 - *diff_level);

    if (~*game_rules & RULES_SCN_NO_TECH_ADVANCES) {
        sc += max(2, 3 + 4*f->AI_tech + 2*(f->AI_wealth - f->AI_fight))
            * clamp(vals[RES], -5, 5);
    }

    debug("social_score %d %d %d %d %s\n", faction, sf, sm, sc, Social[sf].soc_name[sm]);
    return sc;
}

int __cdecl SocialWin_social_ai(int faction,
int UNUSED(v1), int UNUSED(v2), int UNUSED(v3), int UNUSED(v4), int UNUSED(v5))
{
    Faction* f = &Factions[faction];
    if (f->SE_upheaval_cost_paid > 0) {
        social_set(faction);
    }
    return 0;
}

int __cdecl mod_social_ai(int faction, int v1, int v2, int v3, int v4, int v5) {
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    bool pop_boom = 0;
    int want_pop = 0;
    int pop_total = 0;
    int robust = 0;
    int immunity = 0;
    int impunity = 0;
    int penalty = 0;

    if (is_human(faction) || !is_alive(faction)) {
        return 0;
    }
    if (!thinker_enabled(faction)) {
        return social_ai(faction, v1, v2, v3, v4, v5);
    }
    if (f->SE_upheaval_cost_paid > 0) {
        social_set(faction);
        return 0;
    }
    int range = (int)(plans[faction].enemy_base_range / (1.0 + 0.1 * min(4, plans[faction].enemy_bases)));
    bool has_nexus = (manifold_nexus_owner() == faction);
    assert(!memcmp(&f->SE_Politics, &f->SE_Politics_pending, 16));

    for (int i=0; i<m->faction_bonus_count; i++) {
        if (m->faction_bonus_id[i] == FCB_ROBUST) {
            robust |= (1 << m->faction_bonus_val1[i]);
        }
        if (m->faction_bonus_id[i] == FCB_IMMUNITY) {
            immunity |= (1 << m->faction_bonus_val1[i]);
        }
        if (m->faction_bonus_id[i] == FCB_IMPUNITY) {
            impunity |= (1 << (m->faction_bonus_val1[i] * 4 + m->faction_bonus_val2[i]));
        }
        if (m->faction_bonus_id[i] == FCB_PENALTY) {
            penalty |= (1 << (m->faction_bonus_val1[i] * 4 + m->faction_bonus_val2[i]));
        }
    }
    if (has_project(faction, FAC_NETWORK_BACKBONE)) {
        /* Cybernetic */
        impunity |= (1 << (4*SOCIAL_C_FUTURE + SOCIAL_M_CYBERNETIC));
    }
    if (has_project(faction, FAC_CLONING_VATS)) {
        /* Power & Thought Control */
        impunity |= (1 << (4*SOCIAL_C_VALUES + SOCIAL_M_POWER))
            | (1 << (4*SOCIAL_C_FUTURE + SOCIAL_M_THOUGHT_CONTROL));

    } else if (has_tech(faction, Facility[FAC_CHILDREN_CRECHE].preq_tech)) {
        for (int i=0; i<*total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction) {
                want_pop += (pop_goal(i) - b->pop_size)
                    * (b->nutrient_surplus > 1 && has_facility(i, FAC_CHILDREN_CRECHE) ? 4 : 1);
                pop_total += b->pop_size;
            }
        }
        if (pop_total > 0) {
            pop_boom = ((f->SE_growth < 4 ? 1 : 2) * want_pop) >= pop_total;
        }
    }
    debug("social_params %d %d %8s range: %2d has_nexus: %d pop_boom: %d want_pop: %3d pop_total: %3d "\
        "robust: %04x immunity: %04x impunity: %04x penalty: %04x\n", *current_turn, faction, m->filename,
        range, has_nexus, pop_boom, want_pop, pop_total, robust, immunity, impunity, penalty);
    int score_diff = 1 + (*current_turn + 11*faction) % 6;
    int sf = -1;
    int sm2 = -1;

    for (int i=0; i<4; i++) {
        int sm1 = (&f->SE_Politics)[i];
        int sc1 = social_score(faction, i, sm1, range, pop_boom, has_nexus, robust, immunity, impunity, penalty);

        for (int j=0; j<4; j++) {
            if (j == sm1 || !has_tech(faction, Social[i].soc_preq_tech[j]) ||
            (i == m->soc_opposition_category && j == m->soc_opposition_model)) {
                continue;
            }
            int sc2 = social_score(faction, i, j, range, pop_boom, has_nexus, robust, immunity, impunity, penalty);
            if (sc2 - sc1 > score_diff) {
                sf = i;
                sm2 = j;
                score_diff = sc2 - sc1;
            }
        }
    }
    int cost = (is_alien(faction) ? 36 : 24);
    if (sf >= 0 && f->energy_credits > cost) {
        int sm1 = (&f->SE_Politics)[sf];
        (&f->SE_Politics_pending)[sf] = sm2;
        f->energy_credits -= cost;
        f->SE_upheaval_cost_paid += cost;
        debug("social_change %d %d %8s cost: %2d score_diff: %2d %s -> %s\n",
            *current_turn, faction, m->filename,
            cost, score_diff, Social[sf].soc_name[sm1], Social[sf].soc_name[sm2]);
    }
    social_set(faction);
    consider_designs(faction);
    return 0;
}




