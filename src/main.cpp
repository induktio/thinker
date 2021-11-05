/*
 * Thinker - AI improvement mod for Sid Meier's Alpha Centauri.
 * https://github.com/induktio/thinker/
 *
 * Thinker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
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


int handler(void* user, const char* section, const char* name, const char* value) {
    static bool unknown_option = false;
    char buf[INI_MAX_LINE+2] = {};
    Config* cf = (Config*)user;
    strncpy(buf, value, INI_MAX_LINE);
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("thinker", "DirectDraw")) {
        cf->directdraw = atoi(value);
    } else if (MATCH("thinker", "DisableOpeningMovie")) {
        cf->disable_opening_movie = atoi(value);
    } else if (MATCH("thinker", "cpu_idle_fix")) {
        cf->cpu_idle_fix = atoi(value);
    } else if (MATCH("thinker", "minimal_popups")) {
        cf->minimal_popups = atoi(value);
    } else if (MATCH("thinker", "autosave_interval")) {
        cf->autosave_interval = atoi(value);
    } else if (MATCH("thinker", "smooth_scrolling")) {
        cf->smooth_scrolling = atoi(value);
    } else if (MATCH("thinker", "scroll_area")) {
        cf->scroll_area = max(0, atoi(value));
    } else if (MATCH("thinker", "world_map_labels")) {
        cf->world_map_labels = atoi(value);
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
        cf->social_ai = min(1000, max(0, atoi(value)));
    } else if (MATCH("thinker", "tech_balance")) {
        cf->tech_balance = atoi(value);
    } else if (MATCH("thinker", "hurry_items")) {
        cf->hurry_items = atoi(value);
    } else if (MATCH("thinker", "base_spacing")) {
        cf->base_spacing = min(8, max(2, atoi(value)));
    } else if (MATCH("thinker", "base_nearby_limit")) {
        cf->base_nearby_limit = atoi(value);
    } else if (MATCH("thinker", "expansion_limit")) {
        cf->expansion_limit = atoi(value);
    } else if (MATCH("thinker", "expansion_autoscale")) {
        cf->expansion_autoscale = atoi(value);
    } else if (MATCH("thinker", "conquer_priority")) {
        cf->conquer_priority = min(10000, max(1, atoi(value)));
    } else if (MATCH("thinker", "crawler_priority")) {
        cf->crawler_priority = min(10000, max(1, atoi(value)));
    } else if (MATCH("thinker", "limit_project_start")) {
        cf->limit_project_start = atoi(value);
    } else if (MATCH("thinker", "max_satellites")) {
        cf->max_satellites = atoi(value);
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
    } else if (MATCH("thinker", "auto_relocate_hq")) {
        cf->auto_relocate_hq = atoi(value);
    } else if (MATCH("thinker", "counter_espionage")) {
        cf->counter_espionage = atoi(value);
    } else if (MATCH("thinker", "ignore_reactor_power")) {
        cf->ignore_reactor_power = atoi(value);
    } else if (MATCH("thinker", "early_research_start")) {
        cf->early_research_start = atoi(value);
    } else if (MATCH("thinker", "facility_capture_fix")) {
        cf->facility_capture_fix = atoi(value);
    } else if (MATCH("thinker", "territory_border_fix")) {
        cf->territory_border_fix = atoi(value);
    } else if (MATCH("thinker", "eco_damage_fix")) {
        cf->eco_damage_fix = atoi(value);
    } else if (MATCH("thinker", "clean_minerals")) {
        cf->clean_minerals = min(127, max(0, atoi(value)));
    } else if (MATCH("thinker", "spawn_fungal_towers")) {
        cf->spawn_fungal_towers = atoi(value);
    } else if (MATCH("thinker", "spawn_spore_launchers")) {
        cf->spawn_spore_launchers = atoi(value);
    } else if (MATCH("thinker", "spawn_sealurks")) {
        cf->spawn_sealurks = atoi(value);
    } else if (MATCH("thinker", "spawn_battle_ogres")) {
        cf->spawn_battle_ogres = atoi(value);
    } else if (MATCH("thinker", "collateral_damage_value")) {
        cf->collateral_damage_value = min(127, max(0, atoi(value)));
    } else if (MATCH("thinker", "disable_planetpearls")) {
        cf->disable_planetpearls = atoi(value);
    } else if (MATCH("thinker", "disable_aquatic_bonus_minerals")) {
        cf->disable_aquatic_bonus_minerals = atoi(value);
    } else if (MATCH("thinker", "disable_alien_guaranteed_techs")) {
        cf->disable_alien_guaranteed_techs = atoi(value);
    } else if (MATCH("thinker", "cost_factor")) {
        opt_list_parse(CostRatios, buf, MaxDiffNum, 1);
    } else if (MATCH("thinker", "tech_cost_factor")) {
        opt_list_parse(TechCostRatios, buf, MaxDiffNum, 1);
    } else if (MATCH("thinker", "content_pop_player")) {
        opt_list_parse(cf->content_pop_player, buf, MaxDiffNum, 0);
        cf->patch_content_pop = 1;
    } else if (MATCH("thinker", "content_pop_computer")) {
        opt_list_parse(cf->content_pop_computer, buf, MaxDiffNum, 0);
        cf->patch_content_pop = 1;
    } else if (MATCH("thinker", "repair_minimal")) {
        cf->repair_minimal = min(10, max(0, atoi(value)));
    } else if (MATCH("thinker", "repair_fungus")) {
        cf->repair_fungus = min(10, max(0, atoi(value)));
    } else if (MATCH("thinker", "repair_friendly")) {
        cf->repair_friendly = !!atoi(value);
    } else if (MATCH("thinker", "repair_airbase")) {
        cf->repair_airbase = !!atoi(value);
    } else if (MATCH("thinker", "repair_bunker")) {
        cf->repair_bunker = !!atoi(value);
    } else if (MATCH("thinker", "repair_base")) {
        cf->repair_base = min(10, max(0, atoi(value)));
    } else if (MATCH("thinker", "repair_base_native")) {
        cf->repair_base_native = min(10, max(0, atoi(value)));
    } else if (MATCH("thinker", "repair_base_facility")) {
        cf->repair_base_facility = min(10, max(0, atoi(value)));
    } else if (MATCH("thinker", "repair_nano_factory")) {
        cf->repair_nano_factory = min(10, max(0, atoi(value)));
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
                return FALSE;
            }
            if (ini_parse("thinker.ini", handler, &conf) < 0) {
                MessageBoxA(0, "Error while opening thinker.ini file.",
                    MOD_VERSION, MB_OK | MB_ICONSTOP);
                return FALSE;
            }
            if (!cmd_parse(&conf) || !patch_setup(&conf)) {
                return FALSE;
            }
            srand(time(0));
            *engine_version = MOD_VERSION;
            *engine_date = MOD_DATE;
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

int plans_upkeep(int faction) {
    if (!faction || is_human(faction)) {
        return 0;
    }
    /*
    Remove bugged prototypes from the savegame.
    */
    for (int j=0; j < MaxProtoFactionNum; j++) {
        int id = faction*MaxProtoFactionNum + j;
        UNIT* u = &Units[id];
        if (strlen(u->name) >= MaxProtoNameLen
        || u->chassis_type < CHS_INFANTRY
        || u->chassis_type > CHS_MISSILE) {
            for (int k = *total_num_vehicles-1; k >= 0; k--) {
                if (Vehicles[k].unit_id == id) {
                    veh_kill(k);
                }
            }
            for (int k=0; k < *total_num_bases; k++) {
                if (Bases[k].queue_items[0] == id) {
                    Bases[k].queue_items[0] = -FAC_STOCKPILE_ENERGY;
                }
            }
            memset(u, 0, sizeof(UNIT));
        }
    }

    if (!Factions[faction].current_num_bases) {
        return 0;
    }
    if (conf.design_units) {
        const int i = faction;
        int rec = best_reactor(i);
        int wpn = best_weapon(i);
        int arm = best_armor(i, false);
        int arm2 = best_armor(i, true);
        int chs = has_chassis(i, CHS_HOVERTANK) ? CHS_HOVERTANK : 
            (has_chassis(i, CHS_SPEEDER) ? CHS_SPEEDER : CHS_INFANTRY);
        bool twoabl = has_tech(i, Rules->tech_preq_allow_2_spec_abil);
        char buf[256];

        if (has_weapon(i, WPN_PROBE_TEAM)) {
            int algo = has_ability(i, ABL_ID_ALGO_ENHANCEMENT) ? ABL_ALGO_ENHANCEMENT : 0;
            if (has_chassis(i, CHS_FOIL)) {
                int ship = (rec >= REC_FUSION && has_chassis(i, CHS_CRUISER) ? CHS_CRUISER : CHS_FOIL);
                char* name = parse_str(buf, MaxProtoNameLen,
                    Armor[(rec >= REC_FUSION ? arm : ARM_NO_ARMOR)].name_short,
                    Chassis[ship].offsv1_name, "Probe", NULL);
                propose_proto(i, ship, WPN_PROBE_TEAM,
                    (rec >= REC_FUSION ? arm : ARM_NO_ARMOR), algo, rec, PLAN_INFO_WARFARE, name);
            }
            if (arm != ARM_NO_ARMOR && rec >= REC_FUSION) {
                char* name = parse_str(buf, MaxProtoNameLen, Armor[arm].name_short,
                    Chassis[chs].offsv1_name, "Probe", NULL);
                propose_proto(i, chs, WPN_PROBE_TEAM, arm, algo, rec, PLAN_INFO_WARFARE, name);
            }
        }
        if (has_ability(i, ABL_ID_AAA) && arm != ARM_NO_ARMOR) {
            int abls = ABL_AAA;
            if (twoabl) {
                abls |= (has_ability(i, ABL_ID_COMM_JAMMER) ? ABL_COMM_JAMMER :
                    (has_ability(i, ABL_ID_TRANCE) ? ABL_TRANCE : 0));
            }
            propose_proto(i, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, abls, rec, PLAN_DEFENSIVE, NULL);
        }
        if (has_ability(i, ABL_ID_TRANCE) && !has_ability(i, ABL_ID_AAA)) {
            propose_proto(i, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, ABL_TRANCE, rec, PLAN_DEFENSIVE, NULL);
        }
        if (has_chassis(i, CHS_NEEDLEJET) && has_ability(i, ABL_ID_AIR_SUPERIORITY)) {
            propose_proto(i, CHS_NEEDLEJET, wpn, ARM_NO_ARMOR, ABL_AIR_SUPERIORITY | ABL_DEEP_RADAR,
                rec, PLAN_AIR_SUPERIORITY, NULL);
        }
        if (has_weapon(i, WPN_TERRAFORMING_UNIT) && rec >= REC_FUSION) {
            bool grav = has_chassis(i, CHS_GRAVSHIP);
            int abls = has_ability(i, ABL_ID_SUPER_TERRAFORMER ? ABL_SUPER_TERRAFORMER : 0) |
                (twoabl && !grav && has_ability(i, ABL_ID_FUNGICIDAL) ? ABL_FUNGICIDAL : 0);
            propose_proto(i, (grav ? CHS_GRAVSHIP : chs), WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, abls,
                REC_FUSION, PLAN_TERRAFORMING, NULL);
        }
        if (has_weapon(i, WPN_SUPPLY_TRANSPORT) && rec >= REC_FUSION && arm2 != ARM_NO_ARMOR) {
            char* name = parse_str(buf, MaxProtoNameLen, Reactor[REC_FUSION-1].name_short,
                Armor[arm2].name_short, "Supply", NULL);
            propose_proto(i, CHS_INFANTRY, WPN_SUPPLY_TRANSPORT, arm2, 0, REC_FUSION, PLAN_DEFENSIVE, name);
        }
    }

    if (ai_enabled(faction)) {
        const int i = faction;
        int minerals[MaxBaseNum];
        int population[MaxBaseNum];
        Faction* f = &Factions[i];
        MFaction* m = &MFactions[i];

        plans[i].unknown_factions = 0;
        plans[i].contacted_factions = 0;
        plans[i].enemy_factions = 0;
        plans[i].diplo_flags = 0;
        plans[i].enemy_nukes = 0;
        plans[i].enemy_bases = 0;
        plans[i].enemy_mil_factor = 0;
        update_main_region(faction);

        for (int j=1; j < MaxPlayerNum; j++) {
            plans[j].mil_strength = 0;
        }
        for (int j=0; j < *total_num_vehicles; j++) {
            VEH* veh = &Vehicles[j];
            if (veh->faction_id > 0) {
                plans[veh->faction_id].mil_strength += (abs(veh->offense_value())
                    + abs(veh->defense_value())) * (veh->speed() > 1 ? 3 : 2);
            }
        }
        for (int j=1; j < MaxPlayerNum; j++) {
            if (i != j && Factions[j].current_num_bases > 0) {
                if (has_treaty(i, j, DIPLO_COMMLINK)) {
                    plans[i].contacted_factions++;
                } else {
                    plans[i].unknown_factions++;
                }
                if (at_war(i, j)) {
                    plans[i].enemy_factions++;
                    plans[i].enemy_nukes += Factions[j].planet_busters;
                }
                float factor = min(8.0f, (is_human(j) ? 2.0f : 1.0f)
                    * (has_treaty(i, j, DIPLO_COMMLINK) ? 1.0f : 0.5f)
                    * (at_war(i, j) ? 1.0f : (has_pact(i, j) ? 0.1f : 0.25f))
                    * (plans[i].main_region == plans[j].main_region ? 1.5f : 1.0f)
                    * faction_might(j) / max(1, faction_might(i)));

                plans[i].enemy_mil_factor = max(plans[i].enemy_mil_factor, factor);
                plans[i].diplo_flags |= f->diplo_status[j];
            }
        }
        float enemy_sum = 0;
        int n = 0;
        for (int j=0; j < *total_num_bases; j++) {
            BASE* base = &Bases[j];
            MAP* sq = mapsq(base->x, base->y);
            if (base->faction_id == i) {
                population[n] = base->pop_size;
                minerals[n] = base->mineral_surplus;
                n++;
                // Update enemy base threat distances
                int base_region = (sq ? sq->region : 0);
                float enemy_range = MaxEnemyRange;
                for (int k=0; k < *total_num_bases; k++) {
                    BASE* b = &Bases[k];
                    if (at_war(i, b->faction_id) && (sq = mapsq(b->x, b->y))) {
                        float range = map_range(base->x, base->y, b->x, b->y)
                            * (sq->region == base_region ? 1.0f : 1.5f);
                        enemy_range = min(enemy_range, range);
                    }
                }
                enemy_sum += enemy_range;
            } else if (base->faction_id_former == i && at_war(i, base->faction_id)) {
                plans[i].enemy_bases += (is_human(base->faction_id) ? 2 : 1);
            }
        }
        // Exponentially weighted moving average of distances to nearest enemy bases.
        if (n > 0) {
            m->thinker_enemy_range = (enemy_sum/n + 3 * m->thinker_enemy_range)/4;
        }
        std::sort(minerals, minerals+n);
        std::sort(population, population+n);
        if (f->current_num_bases >= 32) {
            plans[i].proj_limit = max(5, minerals[n*3/4]);
        } else {
            plans[i].proj_limit = max(5, minerals[n*2/3]);
        }
        plans[i].satellites_goal = min(conf.max_satellites, population[n*3/4]);
        plans[i].need_police = (f->SE_police > -2 && f->SE_police < 3
            && !has_project(i, FAC_TELEPATHIC_MATRIX));

        int psi = max(-4, 2 - f->best_weapon_value);
        if (has_project(i, FAC_NEURAL_AMPLIFIER)) {
            psi += 2;
        }
        if (has_project(i, FAC_DREAM_TWISTER)) {
            psi += 2;
        }
        if (has_project(i, FAC_PHOLUS_MUTAGEN)) {
            psi++;
        }
        if (has_project(i, FAC_XENOEMPATHY_DOME)) {
            psi++;
        }
        if (MFactions[i].rule_flags & RFLAG_WORMPOLICE && plans[i].need_police) {
            psi += 2;
        }
        psi += min(2, max(-2, (f->enemy_best_weapon_value - f->best_weapon_value)/2));
        psi += MFactions[i].rule_psi/10 + 2*f->SE_planet;
        plans[i].psi_score = psi;

        const int manifold[][3] = {{0,0,0}, {0,1,0}, {1,1,0}, {1,1,1}, {1,2,1}};
        int p = (has_project(i, FAC_MANIFOLD_HARMONICS) ? min(4, max(0, f->SE_planet + 1)) : 0);
        int dn = manifold[p][0] + f->tech_fungus_nutrient - Resource->forest_sq_nutrient;
        int dm = manifold[p][1] + f->tech_fungus_mineral - Resource->forest_sq_mineral;
        int de = manifold[p][2] + f->tech_fungus_energy - Resource->forest_sq_energy;
        bool former_fungus = has_terra(i, FORMER_PLANT_FUNGUS, LAND)
            && (has_tech(i, Rules->tech_preq_ease_fungus_mov) || has_project(i, FAC_XENOEMPATHY_DOME));

        plans[i].keep_fungus = (dn+dm+de > 0);
        plans[i].plant_fungus = (dn+dm+de > 0 && former_fungus);

        debug("plans_upkeep %d %d proj_limit: %2d sat_goal: %2d psi: %2d keep_fungus: %d "\
              "plant_fungus: %d enemy_bases: %2d enemy_mil: %.4f enemy_range: %.4f\n",
              *current_turn, i, plans[i].proj_limit, plans[i].satellites_goal,
              psi, plans[i].keep_fungus, plans[i].plant_fungus,
              plans[i].enemy_bases, plans[i].enemy_mil_factor, m->thinker_enemy_range);
    }
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
    double base_ratio = min(1.0, f->current_num_bases / min(40.0, *map_area_sq_root * 0.5));
    int morale = (has_project(faction, FAC_COMMAND_NEXUS) ? 2 : 0)
        + (has_project(faction, FAC_CYBORG_FACTORY) ? 2 : 0);
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
    if (vals[MOR] >= 1 && vals[MOR] + morale >= 4) {
        sc += 10;
    }
    if (vals[PRO] >= 3 && !has_project(faction, FAC_HUNTER_SEEKER_ALGO)) {
        sc += 4 * max(0, 4 - range/8);
    }
    sc += max(2, 2 + 4*f->AI_wealth + 3*f->AI_tech - f->AI_fight)
        * min(5, max(-3, vals[ECO]));
    sc += max(2, 2*(f->AI_wealth + f->AI_tech) - f->AI_fight + (int)(4*base_ratio))
        * (min(6, vals[EFF]) + (vals[EFF] >= 3 ? 2 : 0));
    sc += max(3, 3 + 2*f->AI_power + 2*f->AI_fight)
        * min(3, max(-4, vals[SUP]));
    sc += max(2, 4 - range/8 + 2*f->AI_power + 2*f->AI_fight)
        * min(4, max(-4, vals[MOR]));

    if (!has_project(faction, FAC_TELEPATHIC_MATRIX)) {
        sc += (vals[POL] < 0 ? 2 : 4) * min(3, max(-5, vals[POL]));
        if (vals[POL] < -2) {
            sc -= (vals[POL] < -3 ? 4 : 2) * max(0, 4 - range/8)
                * (has_chassis(faction, CHS_NEEDLEJET) ? 2 : 1);
        }
        if (has_project(faction, FAC_LONGEVITY_VACCINE) && sf == SOCIAL_C_ECONOMICS) {
            sc += (sm == SOCIAL_M_PLANNED ? 10 : 0);
            sc += (sm == SOCIAL_M_SIMPLE || sm == SOCIAL_M_GREEN ? 5 : 0);
        }
        sc += min(5, max(-5, vals[TAL])) * 3;
    }
    if (!has_project(faction, FAC_CLONING_VATS)) {
        if (pop_boom && vals[GRW] >= 4) {
            sc += 20;
        }
        if (vals[GRW] < -2) {
            sc -= 10;
        }
        sc += (pop_boom ? 6 : 3) * min(6, max(-3, vals[GRW]));
    }
    sc += max(2, (f->SE_planet_base > 0 ? 5 : 2) + m->rule_psi/10
        + (has_project(faction, FAC_MANIFOLD_HARMONICS) ? 6 : 0)) * min(3, max(-3, vals[PLA]));
    sc += max(2, 4 - range/8 + 2*f->AI_power + 2*f->AI_fight) * min(3, max(-2, vals[PRO]));
    sc += 8 * min(8 - *diff_level, max(-3, vals[IND]));

    if (~*game_rules & RULES_SCN_NO_TECH_ADVANCES) {
        sc += max(2, 3 + 4*f->AI_tech + 2*(f->AI_wealth - f->AI_fight))
            * min(5, max(-5, vals[RES]));
    }

    debug("social_score %d %d %d %d %s\n", faction, sf, sm, sc, Social[sf].soc_name[sm]);
    return sc;
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

    if (!conf.social_ai || !ai_enabled(faction)) {
        return social_ai(faction, v1, v2, v3, v4, v5);
    }
    if (f->SE_upheaval_cost_paid > 0) {
        social_set(faction);
        return 0;
    }
    int range = (int)(m->thinker_enemy_range / (1.0 + 0.1 * min(4, plans[faction].enemy_bases)));
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
    int score_diff = 1 + map_hash(*current_turn, faction) % 6;
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




