
#include "plan.h"

int base_enemy_range[MaxBaseNum] = {};
int base_border_range[MaxBaseNum] = {};
int plan_upkeep_turn = 0;

typedef std::pair<int32_t, int32_t> ipair_t;
auto ipair_cmp_t = [](ipair_t& a, ipair_t& b)->bool {
    return a.first > b.first || (a.first == b.first && a.second > b.second); };


void reset_state() {
    // Invalidate all previous plans
    plan_upkeep_turn = 0;
    move_upkeep_faction = -1;
}

bool check_disband(int unit_id, int faction_id) {
    Points bases;
    Points units;
    Points other;

    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            bases.insert({Bases[i].x, Bases[i].y});
        }
    }
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].faction_id == faction_id) {
            if (Vehs[i].unit_id == unit_id) {
                units.insert({Vehs[i].x, Vehs[i].y});
            } else if (Vehs[i].is_combat_unit() || Vehs[i].is_probe()) {
                other.insert({Vehs[i].x, Vehs[i].y});
            }
        }
    }
    assert(units.size() < 8 && other.size() > 0 && bases.size() > 0);
    for (auto& p : bases) {
        if (units.count({p.x, p.y}) && !other.count({p.x, p.y})) {
            return false;
        }
    }
    return true;
}

void design_units(int faction_id) {
    if (!conf.design_units || !faction_id || is_human(faction_id)) {
        return;
    }
    const int fc = faction_id;
    CAbility& aaa = Ability[ABL_ID_AAA];
    CAbility& arty = Ability[ABL_ID_ARTILLERY];
    VehReactor rec = best_reactor(fc);
    VehWeapon wpn = best_weapon(fc);
    VehArmor arm = best_armor(fc, -1);
    VehArmor arm_cheap = best_armor(fc, max(3, Weapon[wpn].cost/2));
    VehChassis chs_land = has_chassis(fc, CHS_HOVERTANK) ? CHS_HOVERTANK :
        (has_chassis(fc, CHS_SPEEDER) ? CHS_SPEEDER : CHS_INFANTRY);
    VehChassis chs_ship = has_chassis(fc, CHS_CRUISER) ? CHS_CRUISER :
        (has_chassis(fc, CHS_FOIL) ? CHS_FOIL : CHS_INFANTRY);
    VehAbl DefendAbls[] =
        {ABL_ID_AAA, ABL_ID_COMM_JAMMER, ABL_ID_POLICE_2X, ABL_ID_TRANCE, ABL_ID_TRAINED};
    bool twoabl = has_tech(Rules->tech_preq_allow_2_spec_abil, fc);
    int wpn_v = Weapon[wpn].offense_value;

    std::priority_queue<ipair_t, std::vector<ipair_t>, decltype(ipair_cmp_t)> obsolete(ipair_cmp_t);
    int active = 0;
    for (int i = 0; i < MaxProtoNum; i++) {
        UNIT* u = &Units[i];
        if (i / MaxProtoFactionNum == fc && u->is_active()) {
            active++;
            if (u->is_prototyped()
            && u->obsolete_factions & (1 << fc)
            && Factions[fc].units_active[i] < 8
            && (!Factions[fc].units_active[i]
            || (!u->is_colony() && !u->is_supply()
            && !u->is_transport() && !u->is_missile()))) {
                int score = (u->cost + 1)*(u->triad() == TRIAD_AIR ? 4 : 1)
                    * Factions[fc].units_active[i];
                obsolete.push({score, i});
            }
        }
    }
    if (active >= 60) {
        bool disband = !(*CurrentTurn % 5);
        while (!obsolete.empty()) {
            int unit_id = obsolete.top().second;
            int score = obsolete.top().first;
            obsolete.pop();
            if (--active >= (score ? 56 : 48)
            && (!score || (disband && check_disband(unit_id, fc)))) {
                debug("retire_proto %d %d %3d %3d %3d %s\n", *CurrentTurn, fc,
                    unit_id, score, Factions[fc].units_active[unit_id], Units[unit_id].name);
                retire_proto(unit_id, fc);
            }
        }
    }

    if (has_weapon(fc, WPN_PROBE_TEAM)) {
        if (chs_ship != CHS_INFANTRY) {
            VehChassis ship = (rec == REC_FISSION && has_chassis(fc, CHS_FOIL) ? CHS_FOIL : chs_ship);
            VehAblFlag algo = has_ability(fc, ABL_ID_ALGO_ENHANCEMENT, chs_ship, WPN_PROBE_TEAM)
                ? ABL_ALGO_ENHANCEMENT : ABL_NONE;
            create_proto(fc, ship, WPN_PROBE_TEAM,
                (rec >= REC_FUSION ? arm : ARM_NO_ARMOR), algo, rec, PLAN_INFO_WARFARE);
        }
        if (arm != ARM_NO_ARMOR && rec >= REC_FUSION) {
            VehAblFlag algo = has_ability(fc, ABL_ID_ALGO_ENHANCEMENT, chs_land, WPN_PROBE_TEAM)
                ? ABL_ALGO_ENHANCEMENT : ABL_NONE;
            create_proto(fc, chs_land, WPN_PROBE_TEAM, arm, algo, rec, PLAN_INFO_WARFARE);
        }
    }
    if (chs_ship != CHS_INFANTRY && wpn_v >= 4) {
        bool long_range = conf.long_range_artillery > 0 && !*MultiplayerActive
            && (Rules->artillery_max_rng <= 4 || arty.cost == 0 || arty.cost == 1)
            && has_ability(fc, ABL_ID_ARTILLERY, chs_ship, wpn);
        if (long_range) {
            VehArmor arm_ship = (rec >= REC_FUSION || !arty.cost_increase_with_speed())
                && !arty.cost_increase_with_armor()
                ? (rec >= REC_QUANTUM ? arm : arm_cheap) : ARM_NO_ARMOR;
            uint32_t abls = ABL_ARTILLERY
                | (twoabl && has_ability(fc, ABL_ID_AAA, chs_ship, wpn)
                && (rec >= REC_QUANTUM || Weapon[wpn].cost <= 6 * rec
                || !arty.cost_increase_with_speed())
                && arm_ship != ARM_NO_ARMOR ? ABL_AAA : ABL_NONE);
            create_proto(fc, chs_ship, wpn, arm_ship, (VehAblFlag)abls, rec, PLAN_OFFENSIVE);
        }
        if (!long_range || arty.cost < -1 || arty.cost > 2) {
            VehArmor arm_ship = wpn_v >= 6 || rec >= REC_FUSION
                ? (rec >= REC_QUANTUM ? arm : arm_cheap) : ARM_NO_ARMOR;
            VehAblFlag abls = has_ability(fc, ABL_ID_AAA, chs_ship, wpn)
                && (rec >= REC_FUSION || (aaa.cost >= -1 && aaa.cost <= rec))
                && arm_ship != ARM_NO_ARMOR ? ABL_AAA : ABL_NONE;
            create_proto(fc, chs_ship, wpn, arm_ship, abls, rec, PLAN_OFFENSIVE);
        }
    }
    if (arm != ARM_NO_ARMOR) {
        uint32_t abls = 0;
        int num = 0;
        for (VehAbl v : DefendAbls) {
            if (v == ABL_ID_POLICE_2X && !need_police(fc)) {
                continue;
            }
            if (v == ABL_ID_TRAINED && Factions[fc].SE_morale < 0) {
                continue;
            }
            if (has_ability(fc, v, CHS_INFANTRY, WPN_HAND_WEAPONS)) {
                abls |= (1 << v);
                num++;
            }
            if (num > twoabl) {
                break;
            }
        }
        create_proto(fc, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, (VehAblFlag)abls, rec, PLAN_DEFENSIVE);

        if (!(abls & ABL_POLICE_2X) && need_police(fc)
        && has_ability(fc, ABL_ID_POLICE_2X, CHS_INFANTRY, WPN_HAND_WEAPONS)) {
            create_proto(fc, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, ABL_POLICE_2X, rec, PLAN_DEFENSIVE);
        }
    }
    if (has_chassis(fc, CHS_NEEDLEJET)) {
        uint32_t addon = twoabl && has_ability(fc, ABL_ID_DEEP_RADAR, CHS_NEEDLEJET, wpn)
            ? ABL_DEEP_RADAR : ABL_NONE;
        if (has_ability(fc, ABL_ID_AIR_SUPERIORITY, CHS_NEEDLEJET, wpn)) {
            uint32_t abls = ABL_AIR_SUPERIORITY | addon;
            create_proto(fc, CHS_NEEDLEJET, wpn, ARM_NO_ARMOR, (VehAblFlag)abls, rec, PLAN_AIR_SUPERIORITY);
        }
        if (has_ability(fc, ABL_ID_DISSOCIATIVE_WAVE, CHS_NEEDLEJET, wpn)
        && has_ability(fc, ABL_ID_AAA, CHS_INFANTRY, wpn)) {
            uint32_t abls = ABL_DISSOCIATIVE_WAVE | addon;
            create_proto(fc, CHS_NEEDLEJET, wpn, ARM_NO_ARMOR, (VehAblFlag)abls, rec, PLAN_OFFENSIVE);
        }
    }
    if (has_weapon(fc, WPN_TERRAFORMING_UNIT) && rec >= REC_FUSION) {
        bool grav = has_chassis(fc, CHS_GRAVSHIP);
        VehChassis chs = (grav ? CHS_GRAVSHIP : chs_land);
        uint32_t abls = (has_ability(fc, ABL_ID_SUPER_TERRAFORMER, chs, WPN_TERRAFORMING_UNIT)
            ? ABL_SUPER_TERRAFORMER : ABL_NONE)
            | (twoabl && !grav && has_ability(fc, ABL_ID_FUNGICIDAL, chs, WPN_TERRAFORMING_UNIT)
            ? ABL_FUNGICIDAL : ABL_NONE);
        create_proto(fc, chs, WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, (VehAblFlag)abls,
            REC_FUSION, PLAN_TERRAFORMING);
    }
    if (has_weapon(fc, WPN_SUPPLY_TRANSPORT) && rec >= REC_FUSION && arm_cheap != ARM_NO_ARMOR) {
        create_proto(fc, CHS_INFANTRY, WPN_SUPPLY_TRANSPORT,
            rec >= REC_QUANTUM ? arm : arm_cheap, ABL_NONE, rec, PLAN_DEFENSIVE);
    }
}

bool need_police(int faction_id) {
    Faction* f = &Factions[faction_id];
    return f->SE_police > -2 && f->SE_police < 3 && !has_project(FAC_TELEPATHIC_MATRIX, faction_id);
}

int psi_score(int faction_id) {
    Faction* f = &Factions[faction_id];
    int weapon_value = Weapon[best_weapon(faction_id)].offense_value;
    int enemy_weapon_value = 1;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id != i && is_alive(i) && !has_pact(faction_id, i)) {
            enemy_weapon_value = max(
                enemy_weapon_value, (int)Weapon[best_weapon(i)].offense_value);
        }
    }
    int psi = max(-6, 2 - weapon_value);
    if (has_project(FAC_NEURAL_AMPLIFIER, faction_id)) {
        psi += min(5, conf.neural_amplifier_bonus/20);
    }
    if (has_project(FAC_DREAM_TWISTER, faction_id)) {
        psi += min(5, conf.dream_twister_bonus/20);
    }
    if (has_project(FAC_PHOLUS_MUTAGEN, faction_id)) {
        psi++;
    }
    if (has_project(FAC_XENOEMPATHY_DOME, faction_id)) {
        psi++;
    }
    if (MFactions[faction_id].rule_flags & RFLAG_WORMPOLICE && need_police(faction_id)) {
        psi += 2;
    }
    psi += clamp((enemy_weapon_value - weapon_value)/2, -2, 2);
    psi += MFactions[faction_id].rule_psi/10 + 2*f->SE_planet;
    return psi;
}

int satellite_count(int faction_id, int item_id) {
    Faction& f = Factions[faction_id];
    int value;
    switch (item_id) {
        case FAC_SKY_HYDRO_LAB: value = f.satellites_nutrient; break;
        case FAC_ORBITAL_POWER_TRANS: value = f.satellites_energy; break;
        case FAC_NESSUS_MINING_STATION: value = f.satellites_mineral; break;
        case FAC_ORBITAL_DEFENSE_POD: default: value = f.satellites_ODP; break;
    }
    return value;
}

int satellite_goal(int faction_id, int item_id) {
    Faction& f = Factions[faction_id];
    int goal = plans[faction_id].satellite_goal;
    if (item_id == FAC_ORBITAL_DEFENSE_POD) {
        if (plans[faction_id].enemy_odp > 0 || plans[faction_id].enemy_sat > 0) {
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

void former_plans(int faction_id) {
    bool former_fungus = (has_terra(faction_id, FORMER_PLANT_FUNGUS, LAND)
        || has_terra(faction_id, FORMER_PLANT_FUNGUS, SEA));
    bool improv_fungus = (has_tech(Rules->tech_preq_ease_fungus_mov, faction_id)
        || has_tech(Rules->tech_preq_improv_fungus, faction_id)
        || has_project(FAC_XENOEMPATHY_DOME, faction_id));
    int value = fungus_yield(faction_id, RES_NONE)
        - (has_terra(faction_id, FORMER_FOREST, LAND)
        ? ResInfo->forest_sq_nutrient
        + ResInfo->forest_sq_mineral
        + ResInfo->forest_sq_energy : 2);
    plans[faction_id].keep_fungus = (value > 0 ? min((improv_fungus ? 8 : 4), 2*value) : 0);
    plans[faction_id].plant_fungus = value > 0 && former_fungus && (improv_fungus || value > 2);
}

void plans_upkeep(int faction_id) {
    const bool governor = is_human(faction_id) && conf.manage_player_bases;
    const int fc = faction_id;
    if (!faction_id || !is_alive(faction_id)) {
        return;
    }
    if (governor) {
        if (plan_upkeep_turn == *CurrentTurn) {
            return;
        }
        plan_upkeep_turn = *CurrentTurn;
    }
    if (thinker_enabled(faction_id) || governor) {
        int minerals[MaxBaseNum] = {};
        int population[MaxBaseNum] = {};
        Faction* f = &Factions[fc];

        memset((void*)&plans[fc], 0, sizeof(AIPlans));
        assert(!plans[fc].main_region);
        assert(plans[(fc+1)&7].main_region);
        update_main_region(faction_id);
        former_plans(faction_id);

        for (int i = 1; i < MaxPlayerNum; i++) {
            plans[i].mil_strength = 0;
        }
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            int triad = veh->triad();
            if (veh->faction_id > 0) {
                int offense = (veh->offense_value() < 0 ? 2 : 1) * abs(veh->offense_value());
                plans[veh->faction_id].mil_strength +=
                    (offense + abs(veh->defense_value())) * (veh->speed() > 1 ? 3 : 2);
            }
            if (veh->faction_id == faction_id) {
                if (veh->is_combat_unit() || veh->is_probe() || veh->is_transport()) {
                    if (triad == TRIAD_LAND) {
                        plans[fc].land_combat_units++;
                    } else if (triad == TRIAD_SEA) {
                        plans[fc].sea_combat_units++;
                    } else {
                        plans[fc].air_combat_units++;
                    }
                    if (veh->is_probe()) {
                        plans[fc].probe_units++;
                    }
                    if (veh->is_transport() && triad == TRIAD_SEA) {
                        plans[fc].transport_units++;
                    }
                }
                if (veh->is_missile()) {
                    plans[fc].missile_units++;
                }
            }
        }
        debug("plans_totals %d %d bases: %3d land: %3d sea: %3d air: %3d "\
            "probe: %3d missile: %3d transport: %3d\n", *CurrentTurn, fc, f->base_count,
            plans[fc].land_combat_units, plans[fc].sea_combat_units, plans[fc].air_combat_units,
            plans[fc].probe_units, plans[fc].missile_units, plans[fc].transport_units);

        for (int i = 1; i < MaxPlayerNum; i++) {
            if (fc != i && Factions[i].base_count > 0) {
                if (has_treaty(fc, i, DIPLO_COMMLINK)) {
                    plans[fc].contacted_factions++;
                } else {
                    plans[fc].unknown_factions++;
                }
                if (at_war(fc, i)) {
                    plans[fc].enemy_factions++;
                    plans[fc].enemy_odp += Factions[i].satellites_ODP;
                    plans[fc].enemy_sat += Factions[i].satellites_nutrient;
                    plans[fc].enemy_sat += Factions[i].satellites_mineral;
                    plans[fc].enemy_sat += Factions[i].satellites_energy;
                }
                float factor = clamp((is_human(i) ? 2.0f : 1.0f)
                    * (has_treaty(fc, i, DIPLO_COMMLINK) ? 1.0f : 0.5f)
                    * (at_war(fc, i) ? 1.0f : (has_pact(fc, i) ? 0.1f : 0.25f))
                    * (plans[fc].main_region == plans[i].main_region ? 1.5f : 1.0f)
                    * faction_might(i) / max(1, faction_might(fc)), 0.01f, 8.0f);

                plans[fc].enemy_mil_factor = max(plans[fc].enemy_mil_factor, factor);
                plans[fc].diplo_flags |= f->diplo_status[i];
            }
        }
        memset(base_enemy_range, 0, sizeof(base_enemy_range));
        int enemy_sum = 0;
        int n = 0;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* base = &Bases[i];
            MAP* sq;
            if (base->faction_id == faction_id) {
                population[n] = base->pop_size;
                minerals[n] = base->mineral_surplus;
                n++;
                // Update enemy base threat distances
                int base_region = region_at(base->x, base->y);
                int enemy_range = MaxEnemyRange;
                int border_range = MaxEnemyRange;
                for (int j = 0; j < *BaseCount; j++) {
                    BASE* b = &Bases[j];
                    if (faction_id != b->faction_id
                    && !has_pact(faction_id, b->faction_id)
                    && (sq = mapsq(b->x, b->y))) {
                        int border_dist = map_range(base->x, base->y, b->x, b->y);
                        int enemy_dist = border_dist
                            * (sq->region == base_region ? 2 : 3)
                            * (b->faction_id_former == faction_id ? 2 : 3);
                        if (!at_war(faction_id, b->faction_id)) {
                            border_dist *= 2;
                            enemy_dist *= 4;
                        }
                        enemy_range = min(enemy_range, enemy_dist / 4);
                        border_range = min(border_range, border_dist);
                    }
                }
                base_enemy_range[i] = enemy_range;
                base_border_range[i] = border_range;
                enemy_sum += enemy_range;
                if (base->faction_id_former != faction_id
                && at_war(faction_id, base->faction_id_former)) {
                    plans[fc].captured_bases++;
                }
            } else if (base->faction_id_former == faction_id && at_war(faction_id, base->faction_id)) {
                plans[fc].enemy_bases += (is_human(base->faction_id) ? 2 : 1);
            }
        }
        assert(n == f->base_count);
        plans[fc].enemy_base_range = (n > 0 ? (1.0f*enemy_sum)/n : MaxEnemyRange);
        plans[fc].psi_score = psi_score(fc);
        std::sort(minerals, minerals+n);
        std::sort(population, population+n);
        if (f->base_count >= 32) {
            plans[fc].project_limit = max(5, minerals[n*3/4]);
        } else {
            plans[fc].project_limit = max(5, minerals[n*2/3]);
        }
        plans[fc].median_limit = max(5, minerals[n/2]);

        if (has_project(FAC_CLOUDBASE_ACADEMY, faction_id)
        || has_project(FAC_SPACE_ELEVATOR, faction_id)
        || facility_count(FAC_AEROSPACE_COMPLEX, faction_id) >= f->base_count/2) {
            plans[fc].satellite_goal = min(conf.max_satellites,
                population[n*7/8]);
        } else {
            plans[fc].satellite_goal = min(conf.max_satellites,
                (population[n*3/4] + 3) / 2 * 2);
        }

        debug("plans_upkeep %d %d proj_limit: %2d sat_goal: %2d psi: %2d keep_fungus: %d "\
            "plant_fungus: %d enemy_bases: %2d enemy_mil: %.4f enemy_range: %.4f\n",
            *CurrentTurn, fc, plans[fc].project_limit, plans[fc].satellite_goal,
            plans[fc].psi_score, plans[fc].keep_fungus, plans[fc].plant_fungus,
            plans[fc].enemy_bases, plans[fc].enemy_mil_factor, plans[fc].enemy_base_range);
    }
}


