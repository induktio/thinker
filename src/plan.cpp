
#include "plan.h"


void create_proto(int faction, VehChassis chassis, VehWeapon weapon, VehArmor armor,
int abilities, VehReactor reactor, VehPlan ai_plan, const char* name)
{
    debug("propose_proto %d %d chs: %d rec: %d wpn: %2d arm: %2d plan: %2d %08X %s\n",
    *current_turn, faction, chassis, reactor, weapon, armor, ai_plan, abilities, name);
    propose_proto(faction, chassis, weapon, armor, abilities, reactor, ai_plan, name);
}

void design_units(int faction) {
    const int i = faction;
    VehReactor rec = best_reactor(i);
    VehWeapon wpn = best_weapon(i);
    VehArmor arm = best_armor(i, false);
    VehArmor arm_cheap = best_armor(i, true);
    VehChassis chs_land = has_chassis(i, CHS_HOVERTANK) ? CHS_HOVERTANK :
        (has_chassis(i, CHS_SPEEDER) ? CHS_SPEEDER : CHS_INFANTRY);
    VehChassis chs_ship = has_chassis(i, CHS_CRUISER) ? CHS_CRUISER :
        (has_chassis(i, CHS_FOIL) ? CHS_FOIL : CHS_INFANTRY);
    VehAbility DefendAbls[] =
        {ABL_ID_AAA, ABL_ID_COMM_JAMMER, ABL_ID_POLICE_2X, ABL_ID_TRANCE, ABL_ID_TRAINED};
    bool twoabl = has_tech(i, Rules->tech_preq_allow_2_spec_abil);
    char buf[256];

    if (has_weapon(i, WPN_PROBE_TEAM)) {
        if (chs_ship != CHS_INFANTRY) {
            VehChassis ship = (rec == REC_FISSION && has_chassis(i, CHS_FOIL) ? CHS_FOIL : chs_ship);
            int algo = has_ability(i, ABL_ID_ALGO_ENHANCEMENT, chs_ship, WPN_PROBE_TEAM)
                ? ABL_ALGO_ENHANCEMENT : ABL_NONE;
            char* name = parse_str(buf, MaxProtoNameLen,
                Armor[(rec >= REC_FUSION ? arm : ARM_NO_ARMOR)].name_short,
                Chassis[ship].offsv1_name, "Probe", NULL);
            create_proto(i, ship, WPN_PROBE_TEAM,
                (rec >= REC_FUSION ? arm : ARM_NO_ARMOR), algo, rec, PLAN_INFO_WARFARE, name);
        }
        if (arm != ARM_NO_ARMOR && rec >= REC_FUSION) {
            int algo = has_ability(i, ABL_ID_ALGO_ENHANCEMENT, chs_land, WPN_PROBE_TEAM)
                ? ABL_ALGO_ENHANCEMENT : ABL_NONE;
            char* name = parse_str(buf, MaxProtoNameLen, Armor[arm].name_short,
                Chassis[chs_land].offsv1_name, "Probe", NULL);
            create_proto(i, chs_land, WPN_PROBE_TEAM, arm, algo, rec, PLAN_INFO_WARFARE, name);
        }
    }
    if (chs_ship != CHS_INFANTRY && Weapon[wpn].offense_value >= 4) {
        VehArmor arm_ship = Weapon[wpn].offense_value >= 10 || rec >= REC_FUSION ? arm_cheap : ARM_NO_ARMOR;
        create_proto(i, chs_ship, wpn, arm_ship, ABL_NONE, rec, PLAN_OFFENSIVE, NULL);
    }
    if (arm != ARM_NO_ARMOR) {
        int abls = 0;
        int num = 0;
        for (VehAbility v : DefendAbls) {
            if (v == ABL_ID_POLICE_2X && !need_police(i)) {
                continue;
            }
            if (v == ABL_ID_TRAINED && Factions[i].SE_morale < 0) {
                continue;
            }
            if (has_ability(i, v, CHS_INFANTRY, WPN_HAND_WEAPONS)) {
                abls |= (1 << v);
                num++;
            }
            if (num > twoabl) {
                break;
            }
        }
        create_proto(i, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, abls, rec, PLAN_DEFENSIVE, NULL);

        if (~abls & ABL_POLICE_2X && need_police(i)
        && has_ability(i, ABL_ID_POLICE_2X, CHS_INFANTRY, WPN_HAND_WEAPONS)) {
            create_proto(i, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, ABL_POLICE_2X, rec, PLAN_DEFENSIVE, NULL);
        }
    }
    if (has_chassis(i, CHS_NEEDLEJET) && has_ability(i, ABL_ID_AIR_SUPERIORITY, CHS_NEEDLEJET, wpn)) {
        int abls = ABL_AIR_SUPERIORITY
            | (twoabl && has_ability(i, ABL_ID_DEEP_RADAR, CHS_NEEDLEJET, wpn) ? ABL_DEEP_RADAR : 0);
        create_proto(i, CHS_NEEDLEJET, wpn, ARM_NO_ARMOR, abls, rec, PLAN_AIR_SUPERIORITY, NULL);
    }
    if (has_weapon(i, WPN_TERRAFORMING_UNIT) && rec >= REC_FUSION) {
        bool grav = has_chassis(i, CHS_GRAVSHIP);
        VehChassis chs = (grav ? CHS_GRAVSHIP : chs_land);
        int abls = (has_ability(i, ABL_ID_SUPER_TERRAFORMER, chs, WPN_TERRAFORMING_UNIT)
            ? ABL_SUPER_TERRAFORMER : ABL_NONE)
            | (twoabl && !grav && has_ability(i, ABL_ID_FUNGICIDAL, chs, WPN_TERRAFORMING_UNIT)
            ? ABL_FUNGICIDAL : ABL_NONE);
        create_proto(i, chs, WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, abls,
            REC_FUSION, PLAN_TERRAFORMING, NULL);
    }
    if (has_weapon(i, WPN_SUPPLY_TRANSPORT) && rec >= REC_FUSION && arm_cheap != ARM_NO_ARMOR) {
        char* name = parse_str(buf, MaxProtoNameLen, Reactor[REC_FUSION-1].name_short,
            Armor[arm_cheap].name_short, "Supply", NULL);
        create_proto(i, CHS_INFANTRY, WPN_SUPPLY_TRANSPORT, arm_cheap, ABL_NONE, REC_FUSION, PLAN_DEFENSIVE, name);
    }
}

bool need_police(int faction) {
    Faction* f = &Factions[faction];
    return f->SE_police > -2 && f->SE_police < 3 && !has_project(faction, FAC_TELEPATHIC_MATRIX);
}

int psi_score(int faction) {
    Faction* f = &Factions[faction];
    int weapon_value = Weapon[best_weapon(faction)].offense_value;
    int enemy_weapon_value = 1;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction != i && is_alive(i) && !has_pact(faction, i)) {
            enemy_weapon_value = max(
                enemy_weapon_value, (int)Weapon[best_weapon(i)].offense_value);
        }
    }
    int psi = max(-6, 2 - weapon_value);
    if (has_project(faction, FAC_NEURAL_AMPLIFIER)) {
        psi += min(5, conf.neural_amplifier_bonus/20);
    }
    if (has_project(faction, FAC_DREAM_TWISTER)) {
        psi += min(5, conf.dream_twister_bonus/20);
    }
    if (has_project(faction, FAC_PHOLUS_MUTAGEN)) {
        psi++;
    }
    if (has_project(faction, FAC_XENOEMPATHY_DOME)) {
        psi++;
    }
    if (MFactions[faction].rule_flags & RFLAG_WORMPOLICE && need_police(faction)) {
        psi += 2;
    }
    psi += clamp((enemy_weapon_value - weapon_value)/2, -2, 2);
    psi += MFactions[faction].rule_psi/10 + 2*f->SE_planet;
    return psi;
}

int satellite_count(int faction, int fac_id) {
    Faction& f = Factions[faction];
    int value;
    switch (fac_id) {
        case FAC_SKY_HYDRO_LAB: value = f.satellites_nutrient; break;
        case FAC_ORBITAL_POWER_TRANS: value = f.satellites_energy; break;
        case FAC_NESSUS_MINING_STATION: value = f.satellites_mineral; break;
        case FAC_ORBITAL_DEFENSE_POD: default: value = f.satellites_ODP; break;
    }
    return value;
}

int satellite_goal(int faction, int fac_id) {
    Faction& f = Factions[faction];
    int goal = plans[faction].satellite_goal;
    if (fac_id == FAC_ORBITAL_DEFENSE_POD) {
        if (plans[faction].enemy_factions > 0) {
            goal = clamp(goal/4, 0, 4) + clamp(f.base_count/8 + 2, 2, 8);
        } else {
            goal = clamp(goal/4, 0, 4) + clamp(f.base_count/8, 2, 4);
        }
    } else {
        if (f.base_count <= 5) {
            goal /= 2;
        }
    }
    if (f.base_count <= 10) {
        goal /= 2;
    }
    return clamp(goal, 0, conf.max_satellites);
}

void former_plans(int faction) {
    bool former_fungus = (has_terra(faction, FORMER_PLANT_FUNGUS, LAND)
        || has_terra(faction, FORMER_PLANT_FUNGUS, SEA));
    bool improv_fungus = (has_tech(faction, Rules->tech_preq_ease_fungus_mov)
        || has_tech(faction, Rules->tech_preq_improv_fungus)
        || has_project(faction, FAC_XENOEMPATHY_DOME));
    int value = fungus_yield(faction, RES_NONE)
        - (has_terra(faction, FORMER_FOREST, LAND)
        ? Resource->forest_sq_nutrient
        + Resource->forest_sq_mineral
        + Resource->forest_sq_energy : 2);
    plans[faction].keep_fungus = (value > 0 ? min((improv_fungus ? 8 : 4), 2*value) : 0);
    plans[faction].plant_fungus = value > 0 && former_fungus && (improv_fungus || value > 2);
}

void plans_upkeep(int faction) {
    bool governor = is_human(faction) && conf.manage_player_bases;
    if (!faction || !is_alive(faction)) {
        return;
    }
    if (!is_human(faction)) {
        /*
        Remove bugged prototypes from the savegame.
        */
        for (int j = 0; j < MaxProtoFactionNum; j++) {
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
        if (conf.design_units) {
            design_units(faction);
        }
    }
    if (governor) {
        static int last_turn = 0;
        if (last_turn == *current_turn) {
            return;
        }
        last_turn = *current_turn;
    }
    if (thinker_enabled(faction) || governor) {
        const int i = faction;
        int minerals[MaxBaseNum];
        int population[MaxBaseNum];
        Faction* f = &Factions[i];

        plans[i].unknown_factions = 0;
        plans[i].contacted_factions = 0;
        plans[i].enemy_factions = 0;
        plans[i].diplo_flags = 0;
        plans[i].enemy_nukes = 0;
        plans[i].enemy_bases = 0;
        plans[i].enemy_odp = 0;
        plans[i].enemy_mil_factor = 0;
        plans[i].land_combat_units = 0;
        plans[i].sea_combat_units = 0;
        plans[i].air_combat_units = 0;
        plans[i].probe_units = 0;
        plans[i].missile_units = 0;
        plans[i].transport_units = 0;
        update_main_region(faction);
        former_plans(faction);

        for (int j=1; j < MaxPlayerNum; j++) {
            plans[j].mil_strength = 0;
        }
        for (int j=0; j < *total_num_vehicles; j++) {
            VEH* veh = &Vehicles[j];
            int triad = veh->triad();
            if (veh->faction_id > 0) {
                int offense = (veh->offense_value() < 0 ? 2 : 1) * abs(veh->offense_value());
                plans[veh->faction_id].mil_strength +=
                    (offense + abs(veh->defense_value())) * (veh->speed() > 1 ? 3 : 2);
            }
            if (veh->faction_id == faction) {
                if (veh->is_combat_unit() || veh->is_probe() || veh->is_transport()) {
                    if (triad == TRIAD_LAND) {
                        plans[i].land_combat_units++;
                    } else if (triad == TRIAD_SEA) {
                        plans[i].sea_combat_units++;
                    } else {
                        plans[i].air_combat_units++;
                    }
                    if (veh->is_probe()) {
                        plans[i].probe_units++;
                    }
                    if (veh->is_transport() && triad == TRIAD_SEA) {
                        plans[i].transport_units++;
                    }
                }
                if (veh->chassis_type() == CHS_MISSILE) { // not included in is_combat_unit
                    plans[i].missile_units++;
                }
            }
        }
        debug("plans_totals %d %d bases: %3d land: %3d sea: %3d air: %3d "\
            "probe: %3d missile: %3d transport: %3d\n", *current_turn, i, f->base_count,
            plans[i].land_combat_units, plans[i].sea_combat_units, plans[i].air_combat_units,
            plans[i].probe_units, plans[i].missile_units, plans[i].transport_units);

        for (int j=1; j < MaxPlayerNum; j++) {
            if (i != j && Factions[j].base_count > 0) {
                if (has_treaty(i, j, DIPLO_COMMLINK)) {
                    plans[i].contacted_factions++;
                } else {
                    plans[i].unknown_factions++;
                }
                if (at_war(i, j)) {
                    plans[i].enemy_factions++;
                    plans[i].enemy_nukes += Factions[j].planet_busters;
                    plans[i].enemy_odp += Factions[j].satellites_ODP;
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
        memset(base_enemy_range, 0, sizeof(base_enemy_range));
        float enemy_sum = 0;
        int n = 0;
        for (int j=0; j < *total_num_bases; j++) {
            BASE* base = &Bases[j];
            MAP* sq = mapsq(base->x, base->y);
            if (base->faction_id == faction) {
                population[n] = base->pop_size;
                minerals[n] = base->mineral_surplus;
                n++;
                // Update enemy base threat distances
                int base_region = (sq ? sq->region : 0);
                float enemy_range = 10*MaxEnemyRange;
                for (int k=0; k < *total_num_bases; k++) {
                    BASE* b = &Bases[k];
                    if (faction != b->faction_id
                    && !has_pact(faction, b->faction_id)
                    && (sq = mapsq(b->x, b->y))) {
                        float range = map_range(base->x, base->y, b->x, b->y)
                            * (sq->region == base_region ? 1.0f : 1.5f)
                            * (at_war(faction, b->faction_id) ? 1.0f : 5.0f)
                            * (b->faction_id_former == faction ? 1.0f : 2.0f)
                            * (is_human(b->faction_id) ? 1.0f : 1.5f);
                        enemy_range = min(enemy_range, range);
                    }
                }
                base_enemy_range[j] = (int)enemy_range;
                enemy_sum += min((float)MaxEnemyRange, enemy_range);

            } else if (base->faction_id_former == faction && at_war(faction, base->faction_id)) {
                plans[i].enemy_bases += (is_human(base->faction_id) ? 2 : 1);
            }
        }
        plans[i].enemy_base_range = (n > 0 ? enemy_sum/n : MaxEnemyRange);
        plans[i].psi_score = psi_score(i);
        std::sort(minerals, minerals+n);
        std::sort(population, population+n);
        if (f->base_count >= 32) {
            plans[i].project_limit = max(5, minerals[n*3/4]);
        } else {
            plans[i].project_limit = max(5, minerals[n*2/3]);
        }
        plans[i].median_limit = max(5, minerals[n/2]);

        if (has_project(faction, FAC_CLOUDBASE_ACADEMY)
        || has_project(faction, FAC_SPACE_ELEVATOR)
        || facility_count(faction, FAC_AEROSPACE_COMPLEX) >= f->base_count/2) {
            plans[i].satellite_goal = min(conf.max_satellites,
                population[n*7/8]);
        } else {
            plans[i].satellite_goal = min(conf.max_satellites,
                (population[n*3/4] + 3) / 2 * 2);
        }

        debug("plans_upkeep %d %d proj_limit: %2d sat_goal: %2d psi: %2d keep_fungus: %d "\
            "plant_fungus: %d enemy_bases: %2d enemy_mil: %.4f enemy_range: %.4f\n",
            *current_turn, i, plans[i].project_limit, plans[i].satellite_goal,
            plans[i].psi_score, plans[i].keep_fungus, plans[i].plant_fungus,
            plans[i].enemy_bases, plans[i].enemy_mil_factor, plans[i].enemy_base_range);
    }
}


