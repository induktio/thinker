
#include "plan.h"

int plan_upkeep_turn = 0;


int facility_score(FacilityId item_id, WItem& Wgov) {
    CFacility& p = Facility[item_id];
    return Wgov.AI_fight * p.AI_fight
        + Wgov.AI_growth * p.AI_growth + Wgov.AI_power * p.AI_power
        + Wgov.AI_tech * p.AI_tech + Wgov.AI_wealth * p.AI_wealth;
}

void governor_priorities(BASE& base, WItem& Wgov) {
    Faction& f = Factions[base.faction_id];
    uint32_t gov = base.governor_flags;
    if (is_human(base.faction_id)) {
        Wgov.AI_growth = (gov & GOV_PRIORITY_EXPLORE ? 4 : 1);
        Wgov.AI_tech   = (gov & GOV_PRIORITY_DISCOVER ? 4 : 1);
        Wgov.AI_wealth = (gov & GOV_PRIORITY_BUILD ? 4 : 1);
        Wgov.AI_power  = (gov & GOV_PRIORITY_CONQUER ? 4 : 1);
        Wgov.AI_fight  = clamp(((base.defend_goal/2)-1)*2, -2, 2);
    } else {
        Wgov.AI_growth = (f.AI_growth ? 4 : 1);
        Wgov.AI_tech   = (f.AI_tech ? 4 : 1);
        Wgov.AI_wealth = (f.AI_wealth ? 4 : 1);
        Wgov.AI_power  = (f.AI_power ? 4 : 1);
        Wgov.AI_fight  = clamp(2*f.AI_fight, -2, 2);
    }
}

void reset_state() {
    // Invalidate all previous plans
    plan_upkeep_turn = -1;
    move_upkeep_faction = -1;
}

static bool check_disband(int unit_id, int faction_id) {
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

static int upgrade_value(UNIT* new_unit, UNIT* old_unit) {
    const int priority[][2] = {
        {ABL_AAA, 7},
        {ABL_COMM_JAMMER, 4},
        {ABL_POLICE_2X, 3},
        {ABL_TRANCE, 3},
        {ABL_TRAINED, 2},
    };
    uint32_t abls_new = new_unit->ability_flags;
    uint32_t abls_old = old_unit->ability_flags;
    int atk_new = new_unit->offense_value();
    int atk_old = old_unit->offense_value();
    int def_new = new_unit->defense_value();
    int def_old = old_unit->defense_value();
    bool defend = def_old + (old_unit->speed() == 1) > atk_old;
    int diff_val = (defend ? def_new - def_old : atk_new - atk_old);

    if (old_unit != new_unit
    && old_unit->chassis_id == new_unit->chassis_id
    && old_unit->plan <= PLAN_RECON
    && new_unit->plan <= PLAN_RECON
    && atk_new >= atk_old && def_new >= def_old
    && (defend == (def_new + (new_unit->speed() == 1) > atk_new))
    && diff_val > (abls_new & ~abls_old ? 0 : 1)
    && (max(atk_old, def_old) < 4 || diff_val >= max((atk_old + def_old)/2, 4))
    && !(((abls_old & abls_new) ^ abls_old) & ~ABL_SLOW)
    && (abls_old & ABL_ARTILLERY) == (abls_new & ABL_ARTILLERY)) {
        int value = 4*(atk_new + def_new) - 2*new_unit->cost;
        for (auto& p : priority) {
            if (abls_new & p[0]) value += p[1];
        }
        return value;
    }
    return 0;
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
    bool upgrade = !(*CurrentTurn % 4);
    int wpn_v = Weapon[wpn].offense_value;
    int arm_v = Weapon[arm].offense_value;

    if (upgrade) {
        score_max_queue_t old_units;
        std::vector<int> new_units;
        for (int i = 0; i < MaxProtoNum; i++) {
            UNIT* u = &Units[i];
            if ((i / MaxProtoFactionNum == 0 || i / MaxProtoFactionNum == fc)
            && u->is_active() && u->triad() == TRIAD_LAND
            && u->offense_value() > 0 && u->defense_value() > 0) {
                if (i / MaxProtoFactionNum == fc
                && u->is_prototyped() && !(u->obsolete_factions & (1 << fc))) {
                    new_units.push_back(i);
                }
                if (Factions[fc].units_active[i]) {
                    int atk_val = u->offense_value();
                    int def_val = u->defense_value();
                    if ((u->obsolete_factions & (1 << fc))
                    || (atk_val > def_val ? atk_val < wpn_v : def_val < arm_v)) {
                        int score = (u->ability_flags & ~ABL_SLOW ? 0 : 4)
                            + 4*(atk_val == 1) + 4*(def_val == 1) - atk_val - def_val - u->cost;
                        old_units.push({i, score});
                    }
                }
            }
        }
        while (!old_units.empty()) {
            int old_score = old_units.top().score;
            int old_unit_id = old_units.top().item_id;
            int num = veh_count(fc, old_unit_id);
            UNIT* old_unit = &Units[old_unit_id];
            old_units.pop();
            int best_score = 0;
            int best_id = -1;
            for (int new_unit_id : new_units) {
                UNIT* new_unit = &Units[new_unit_id];
                int score;
                if ((score = upgrade_value(new_unit, old_unit)) > best_score) {
                    best_score = score;
                    best_id = new_unit_id;
                }
            }
            if (best_id >= 0 && num > 0) {
                int cost_limit = clamp(plans[fc].defense_modifier/2 + 1, 1, 3)
                    * max(0, Factions[fc].energy_credits - 20) / 4;
                int cost = 10 * mod_upgrade_cost(fc, best_id, old_unit_id);
                int total_cost = num * cost;
                if (cost < 50 + min(50, Factions[fc].energy_credits/32)) {
                    debug("upgrade_check %d %d count: %d cost: %d %s / %s\n",
                        *CurrentTurn, fc, num, total_cost, old_unit->name, Units[best_id].name);
                    if (total_cost < cost_limit) {
                        full_upgrade(fc, best_id, old_unit_id);
                    } else if (old_score > 0) {
                        part_upgrade(fc, best_id, old_unit_id);
                    }
                }
            }
        }
    }
    score_min_queue_t obsolete;
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
                obsolete.push({i, score});
            }
        }
    }
    if (active >= 60) {
        while (!obsolete.empty()) {
            int unit_id = obsolete.top().item_id;
            int score = obsolete.top().score;
            obsolete.pop();
            if (--active >= (score ? 56 : 48)
            && (!score || (upgrade && check_disband(unit_id, fc)))) {
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
                (rec >= REC_FUSION ? arm : ARM_NO_ARMOR), algo, rec, PLAN_PROBE);
        }
        if (arm != ARM_NO_ARMOR && rec >= REC_FUSION) {
            VehAblFlag algo = has_ability(fc, ABL_ID_ALGO_ENHANCEMENT, chs_land, WPN_PROBE_TEAM)
                ? ABL_ALGO_ENHANCEMENT : ABL_NONE;
            create_proto(fc, chs_land, WPN_PROBE_TEAM, arm, algo, rec, PLAN_PROBE);
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
            VehAblFlag abls = ABL_ARTILLERY
                | (twoabl && has_ability(fc, ABL_ID_AAA, chs_ship, wpn)
                && (rec >= REC_QUANTUM || Weapon[wpn].cost <= 6 * rec
                || !arty.cost_increase_with_speed())
                && arm_ship != ARM_NO_ARMOR ? ABL_AAA : ABL_NONE);
            create_proto(fc, chs_ship, wpn, arm_ship, abls, rec, PLAN_OFFENSE);
        }
        if (!long_range || arty.cost < -1 || arty.cost > 2) {
            VehArmor arm_ship = wpn_v >= 6 || rec >= REC_FUSION
                ? (rec >= REC_QUANTUM ? arm : arm_cheap) : ARM_NO_ARMOR;
            VehAblFlag abls = has_ability(fc, ABL_ID_AAA, chs_ship, wpn)
                && (rec >= REC_FUSION || (aaa.cost >= -1 && aaa.cost <= rec))
                && arm_ship != ARM_NO_ARMOR ? ABL_AAA : ABL_NONE;
            create_proto(fc, chs_ship, wpn, arm_ship, abls, rec, PLAN_OFFENSE);
        }
    }
    if (arm != ARM_NO_ARMOR) {
        VehAblFlag abls = ABL_NONE;
        int num = 0;
        for (VehAbl v : DefendAbls) {
            if (v == ABL_ID_POLICE_2X && !need_police(fc)) {
                continue;
            }
            if (v == ABL_ID_TRAINED && Factions[fc].SE_morale < 0) {
                continue;
            }
            if (has_ability(fc, v, CHS_INFANTRY, WPN_HAND_WEAPONS)) {
                abls |= (VehAblFlag)(1 << v);
                num++;
            }
            if (num > twoabl) {
                break;
            }
        }
        create_proto(fc, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, abls, rec, PLAN_DEFENSE);

        if (!(abls & ABL_POLICE_2X) && need_police(fc)
        && has_ability(fc, ABL_ID_POLICE_2X, CHS_INFANTRY, WPN_HAND_WEAPONS)) {
            create_proto(fc, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, ABL_POLICE_2X, rec, PLAN_DEFENSE);
        }
    }
    if (has_chassis(fc, CHS_NEEDLEJET)) {
        VehAblFlag addon = twoabl && has_ability(fc, ABL_ID_DEEP_RADAR, CHS_NEEDLEJET, wpn)
            ? ABL_DEEP_RADAR : ABL_NONE;
        if (has_ability(fc, ABL_ID_AIR_SUPERIORITY, CHS_NEEDLEJET, wpn)) {
            VehAblFlag abls = ABL_AIR_SUPERIORITY | addon;
            create_proto(fc, CHS_NEEDLEJET, wpn, ARM_NO_ARMOR, abls, rec, PLAN_AIR_SUPERIORITY);
        }
        if (has_ability(fc, ABL_ID_DISSOCIATIVE_WAVE, CHS_NEEDLEJET, wpn)
        && has_ability(fc, ABL_ID_AAA, CHS_INFANTRY, wpn)) {
            VehAblFlag abls = ABL_DISSOCIATIVE_WAVE | addon;
            create_proto(fc, CHS_NEEDLEJET, wpn, ARM_NO_ARMOR, abls, rec, PLAN_OFFENSE);
        }
    }
    if (has_weapon(fc, WPN_TERRAFORMING_UNIT) && rec >= REC_FUSION) {
        bool grav = has_chassis(fc, CHS_GRAVSHIP);
        VehChassis chs = (grav ? CHS_GRAVSHIP : chs_land);
        VehAblFlag abls = (has_ability(fc, ABL_ID_SUPER_TERRAFORMER, chs, WPN_TERRAFORMING_UNIT)
            ? ABL_SUPER_TERRAFORMER : ABL_NONE)
            | (twoabl && !grav && has_ability(fc, ABL_ID_FUNGICIDAL, chs, WPN_TERRAFORMING_UNIT)
            ? ABL_FUNGICIDAL : ABL_NONE);
        create_proto(fc, chs, WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, abls,
            REC_FUSION, PLAN_TERRAFORM);
    }
    if (has_weapon(fc, WPN_SUPPLY_TRANSPORT) && rec >= REC_FUSION && arm_cheap != ARM_NO_ARMOR) {
        create_proto(fc, CHS_INFANTRY, WPN_SUPPLY_TRANSPORT,
            rec >= REC_QUANTUM ? arm : arm_cheap, ABL_NONE, rec, PLAN_SUPPLY);
    }
}

bool need_police(int faction_id) {
    Faction* f = &Factions[faction_id];
    return f->SE_police > -2 && f->SE_police < 3 && !has_project(FAC_TELEPATHIC_MATRIX, faction_id);
}

int psi_score(int faction_id) {
    Faction* f = &Factions[faction_id];
    int weapon_value = Weapon[best_weapon(faction_id)].offense_value;
    int enemy_weapon_value = 0;
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
    if (has_project(FAC_VOICE_OF_PLANET, faction_id)) {
        psi++;
    }
    if (MFactions[faction_id].rule_flags & RFLAG_WORMPOLICE && need_police(faction_id)) {
        psi += 2;
    }
    if (enemy_weapon_value) {
        psi += clamp((enemy_weapon_value - weapon_value)/2, -3, 3);
    }
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
    bool former_fungus = (has_terra(FORMER_PLANT_FUNGUS, TRIAD_LAND, faction_id)
        || has_terra(FORMER_PLANT_FUNGUS, TRIAD_SEA, faction_id));
    bool improv_fungus = (has_tech(Rules->tech_preq_ease_fungus_mov, faction_id)
        || has_tech(Rules->tech_preq_improv_fungus, faction_id)
        || has_project(FAC_XENOEMPATHY_DOME, faction_id));
    int value = fungus_yield(faction_id, RES_NONE)
        - (has_terra(FORMER_FOREST, TRIAD_LAND, faction_id)
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
        AIPlans* p = &plans[fc];
        memset((void*)p, 0, sizeof(AIPlans));
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
                        p->land_combat_units++;
                    } else if (triad == TRIAD_SEA) {
                        p->sea_combat_units++;
                    } else {
                        p->air_combat_units++;
                    }
                    if (veh->is_probe()) {
                        p->probe_units++;
                    }
                    if (veh->is_transport() && triad == TRIAD_SEA) {
                        p->transport_units++;
                    }
                }
                if (veh->is_missile()) {
                    p->missile_units++;
                }
            }
        }
        debug("plans_totals %d %d bases: %3d land: %3d sea: %3d air: %3d "\
            "probe: %3d missile: %3d transport: %3d\n", *CurrentTurn, fc, f->base_count,
            p->land_combat_units, p->sea_combat_units, p->air_combat_units,
            p->probe_units, p->missile_units, p->transport_units);

        for (int i = 1; i < MaxPlayerNum; i++) {
            if (fc != i && Factions[i].base_count > 0) {
                if (has_treaty(fc, i, DIPLO_COMMLINK)) {
                    p->contacted_factions++;
                } else {
                    p->unknown_factions++;
                }
                if (at_war(fc, i)) {
                    p->enemy_factions++;
                    p->enemy_odp += Factions[i].satellites_ODP;
                    p->enemy_sat += Factions[i].satellites_nutrient;
                    p->enemy_sat += Factions[i].satellites_mineral;
                    p->enemy_sat += Factions[i].satellites_energy;
                }
                float factor = clamp((is_human(i) ? 2.0f : 1.0f)
                    * (has_treaty(fc, i, DIPLO_COMMLINK) ? 1.0f : 0.5f)
                    * (at_war(fc, i) ? 1.0f : (has_pact(fc, i) ? 0.1f : 0.25f))
                    * (p->main_region == plans[i].main_region ? 1.5f : 1.0f)
                    * faction_might(i) / max(1, faction_might(fc)), 0.01f, 8.0f);

                p->enemy_mil_factor = max(p->enemy_mil_factor, factor);
                p->diplo_flags |= f->diplo_status[i];
            }
        }
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
                            enemy_dist *= 4;
                        }
                        enemy_range = min(enemy_range, enemy_dist / 4);
                    }
                }
                base->defend_range = enemy_range;
                enemy_sum += enemy_range;
                if (base->faction_id_former != faction_id
                && at_war(faction_id, base->faction_id_former)) {
                    p->captured_bases++;
                }
            } else if (base->faction_id_former == faction_id && at_war(faction_id, base->faction_id)) {
                p->enemy_bases++;
            }
        }
        assert(n == f->base_count);
        p->enemy_base_range = (n > 0 ? (1.0f*enemy_sum)/n : MaxEnemyRange);
        p->psi_score = psi_score(fc);
        if (!p->enemy_factions) {
            p->defense_modifier = clamp(f->AI_fight + f->AI_power + f->base_count/32, 0, 2);
        } else {
            p->defense_modifier = clamp((int)(3.0f - p->enemy_base_range/8.0f)
                + min(2, f->base_count/32)
                + min(4, p->enemy_bases/2)
                + min(2, p->captured_bases/2)
                + min(2, p->enemy_factions/2), 1, 4);
        }
        std::sort(minerals, minerals+n);
        std::sort(population, population+n);
        if (f->base_count >= 32) {
            p->project_limit = max(5, minerals[n*3/4]);
        } else {
            p->project_limit = max(5, minerals[n*2/3]);
        }
        bool full_value = has_tech(Facility[FAC_AEROSPACE_COMPLEX].preq_tech, faction_id)
            || has_project(FAC_CLOUDBASE_ACADEMY, faction_id)
            || has_project(FAC_SPACE_ELEVATOR, faction_id);
        p->median_limit = max(5, minerals[n/2]);
        p->satellite_goal = min(conf.max_satellites,
            population[n*7/8] * (full_value ? 1 : 2));

        debug("plans_upkeep %d %d proj_limit: %2d sat_goal: %2d psi: %2d keep_fungus: %d "\
            "plant_fungus: %d enemy_bases: %2d enemy_mil: %.4f enemy_range: %.4f\n",
            *CurrentTurn, fc, p->project_limit, p->satellite_goal,
            p->psi_score, p->keep_fungus, p->plant_fungus,
            p->enemy_bases, p->enemy_mil_factor, p->enemy_base_range);
    }
}


