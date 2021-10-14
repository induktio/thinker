
#include "base.h"


int __cdecl mod_base_prod_choices(int id, int v1, int v2, int v3) {
    assert(id >= 0 && id < *total_num_bases);
    BASE* base = &Bases[id];
    int prod = base->queue_items[0];
    int faction = base->faction_id;
    int choice = 0;
    print_base(id);

    if (is_human(faction)) {
        debug("skipping human base\n");
        choice = base_prod_choices(id, v1, v2, v3);
    } else if (!ai_enabled(faction)) {
        debug("skipping computer base\n");
        choice = base_prod_choices(id, v1, v2, v3);
    } else {
        set_base(id);
        base_compute(1);
        if ((choice = need_psych(id)) != 0 && choice != prod) {
            debug("BUILD PSYCH\n");
        } else if (base->state_flags & BSTATE_PRODUCTION_DONE) {
            choice = select_production(id);
            base->state_flags &= ~BSTATE_PRODUCTION_DONE;
        } else if (prod >= 0 && !can_build_unit(faction, prod)) {
            debug("BUILD FACILITY\n");
            choice = find_facility(id);
        } else if (prod < 0 && !can_build(id, abs(prod))) {
            debug("BUILD CHANGE\n");
            if (base->minerals_accumulated > Rules->retool_exemption) {
                choice = find_facility(id);
            } else {
                choice = select_production(id);
            }
        } else if (need_defense(id)) {
            debug("BUILD DEFENSE\n");
            choice = find_proto(id, TRIAD_LAND, COMBAT, DEF);
        } else {
            debug("BUILD OLD\n");
            choice = prod;
        }
        debug("choice: %d %s\n", choice, prod_name(choice));
    }
    fflush(debug_log);
    return choice;
}

int need_psych(int id) {
    BASE* b = &Bases[id];
    int faction = b->faction_id;
    Faction* f = &Factions[faction];
    if (unit_in_tile(mapsq(b->x, b->y)) != faction || !base_can_riot(id)) {
        return 0;
    }
    if (b->drone_total > b->talent_total) {
        if (b->nerve_staple_count < 3 && !un_charter() && f->SE_police >= 0 && b->pop_size >= 4
        && (b->faction_id_former == faction || Factions[faction].diplo_status[b->faction_id_former]
        & (DIPLO_ATROCITY_VICTIM | DIPLO_WANT_REVENGE))) {
            action_staple(id);
            return 0;
        }
        if (b->faction_id_former != faction && b->assimilation_turns_left > 5
        && b->pop_size >= 6 && b->mineral_surplus >= 8 && can_build(id, FAC_PUNISHMENT_SPHERE))
            return -FAC_PUNISHMENT_SPHERE;
        if (can_build(id, FAC_RECREATION_COMMONS))
            return -FAC_RECREATION_COMMONS;
        if (has_project(faction, FAC_VIRTUAL_WORLD) && can_build(id, FAC_NETWORK_NODE))
            return -FAC_NETWORK_NODE;
        if (!b->assimilation_turns_left && b->pop_size >= 6
        && b->energy_surplus >= 4*Facility[FAC_HOLOGRAM_THEATRE].maint
        && can_build(id, FAC_HOLOGRAM_THEATRE))
            return -FAC_HOLOGRAM_THEATRE;
    }
    return 0;
}

int need_defense(int id) {
    BASE* b = &Bases[id];
    int prod = b->queue_items[0];
    /* Do not interrupt secret project building. */
    return !has_defenders(b->x, b->y, b->faction_id)
        && (Rules->retool_strictness == 0 || (prod < 0 && prod > -SP_ID_First));
}

bool need_scouts(int x, int y, int faction, int scouts) {
    Faction* f = &Factions[faction];
    MAP* sq = mapsq(x, y);
    if (is_ocean(sq)) {
        return false;
    }
    int nearby_pods = Continents[sq->region].pods
        * min(2*f->region_territory_tiles[sq->region], Continents[sq->region].tile_count)
        / max(1, Continents[sq->region].tile_count);
    int score = max(-2, 5 - *current_turn/10)
        + 2*(*MapNativeLifeForms) - 3*scouts
        + min(4, nearby_pods / 6);

    bool val = random(16) < score;
    debug("need_scouts %2d %2d score: %2d value: %d scouts: %d goodies: %d native: %d\n",
        x, y, score, val, scouts, Continents[sq->region].pods, *MapNativeLifeForms);
    return val;
}

bool can_build_ships(int base_id) {
    BASE* b = &Bases[base_id];
    int k = *map_area_sq_root + 20;
    return (has_chassis(b->faction_id, CHS_FOIL) || has_chassis(b->faction_id, CHS_CRUISER))
        && nearby_tiles(b->x, b->y, TRIAD_SEA, k) >= k;
}

int project_score(int faction, int proj) {
    CFacility* p = &Facility[proj];
    Faction* f = &Factions[faction];
    return random(5) + f->AI_fight * p->AI_fight
        + (f->AI_growth+1) * p->AI_growth + (f->AI_power+1) * p->AI_power
        + (f->AI_tech+1) * p->AI_tech + (f->AI_wealth+1) * p->AI_wealth;
}

bool redundant_project(int faction, int proj) {
    Faction* f = &Factions[faction];
    if (proj == FAC_PLANETARY_DATALINKS) {
        int n = 0;
        for (int i=0; i < MaxPlayerNum; i++) {
            if (Factions[i].current_num_bases > 0) {
                n++;
            }
        }
        return n < 4;
    }
    if (proj == FAC_CITIZENS_DEFENSE_FORCE) {
        return Facility[FAC_CITIZENS_DEFENSE_FORCE].maint == 0
            && facility_count(faction, FAC_CITIZENS_DEFENSE_FORCE) > f->current_num_bases/2 + 2;
    }
    if (proj == FAC_MARITIME_CONTROL_CENTER) {
        int n = 0;
        for (int i=0; i<*total_num_vehicles; i++) {
            VEH* veh = &Vehicles[i];
            if (veh->faction_id == faction && veh->triad() == TRIAD_SEA) {
                n++;
            }
        }
        return n < f->current_num_bases/4 && facility_count(faction, FAC_NAVAL_YARD) < 4;
    }
    if (proj == FAC_HUNTER_SEEKER_ALGO) {
        return f->SE_probe >= 3;
    }
    if (proj == FAC_LIVING_REFINERY) {
        return f->SE_support >= 3;
    }
    return false;
}

int find_project(int id) {
    BASE* base = &Bases[id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    int projs = 0;
    int nukes = 0;
    int works = 0;
    int bases = f->current_num_bases;
    int diplo = plans[faction].diplo_flags;
    int similar_limit = (base->minerals_accumulated > 80 ? 2 : 1);
    int nuke_score = (un_charter() ? 0 : 3) + 2*f->AI_power + f->AI_fight
        + max(-1, plans[faction].enemy_nukes - f->satellites_ODP)
        + (diplo & DIPLO_ATROCITY_VICTIM ? 5 : 0)
        + (diplo & DIPLO_WANT_REVENGE ? 4 : 0);
    bool build_nukes = has_weapon(faction, WPN_PLANET_BUSTER) && nuke_score > 2;
    int nuke_limit = (build_nukes &&
        f->planet_busters < 2 + bases/40 + f->AI_fight ? 1 + bases/40 : 0);

    for (int i=0; i < *total_num_bases; i++) {
        if (Bases[i].faction_id == faction) {
            int t = Bases[i].queue_items[0];
            if (t <= -SP_ID_First || t == -FAC_SUBSPACE_GENERATOR) {
                projs++;
            } else if (t == -FAC_SKUNKWORKS) {
                works++;
            } else if (t >= 0 && Units[t].weapon_type == WPN_PLANET_BUSTER) {
                nukes++;
            }
        }
    }
    if (build_nukes && nukes < nuke_limit && nukes < bases/8) {
        int best = 0;
        for(int i=0; i < MaxProtoFactionNum; i++) {
            int unit_id = faction*MaxProtoFactionNum + i;
            UNIT* u = &Units[unit_id];
            if (u->weapon_type == WPN_PLANET_BUSTER && strlen(u->name) > 0
            && u->std_offense_value() > Units[best].std_offense_value()) {
                debug("find_project %d %d %s\n", faction, unit_id, (char*)Units[unit_id].name);
                best = unit_id;
            }
        }
        if (best) {
            if (~Units[best].unit_flags & UNIT_PROTOTYPED && can_build(id, FAC_SKUNKWORKS)) {
                if (works < 2)
                    return -FAC_SKUNKWORKS;
            } else {
                return best;
            }
        }
    }
    if (projs+nukes < 3 + nuke_limit && projs+nukes < bases/4) {
        if (can_build(id, FAC_SUBSPACE_GENERATOR)) {
            return -FAC_SUBSPACE_GENERATOR;
        }
        int score = INT_MIN;
        int choice = 0;
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            int tech = Facility[i].preq_tech;
            if (conf.limit_project_start > *diff_level && i != FAC_ASCENT_TO_TRANSCENDENCE
            && tech >= 0 && !(TechOwners[tech] & *human_players)) {
                continue;
            }
            if (can_build(id, i) && prod_count(faction, -i, id) < similar_limit
            && (similar_limit > 1 || !redundant_project(faction, i))) {
                int sc = project_score(faction, i);
                choice = (sc > score ? i : choice);
                score = max(score, sc);
                debug("find_project %d %d %d %s\n", faction, i, sc, Facility[i].name);
            }
        }
        return (projs > 0 || score > 3 ? -choice : 0);
    }
    return 0;
}

bool relocate_hq(int base_id) {
    int faction = Bases[base_id].faction_id;
    int new_id = -1;

    if (!has_tech(faction, Facility[FAC_HEADQUARTERS].preq_tech)) {
        return false;
    }
    for (int i=0; i < *total_num_bases; i++) {
        BASE* b = &Bases[i];
        int t = b->queue_items[0];
        if (b->faction_id == faction) {
            if ((i != base_id && t == -FAC_HEADQUARTERS) || has_facility(i, FAC_HEADQUARTERS)) {
                return false;
            }
            if (new_id < 0 && b->mineral_surplus > 3) {
                new_id = i;
            }
        }
    }
    return new_id == base_id;
}

/*
Return true if unit2 is strictly better than unit1 in all circumstances (non PSI).
Disable random chance in prototype choices in these instances.
*/
bool unit_is_better(UNIT* u1, UNIT* u2) {
    bool val = (u1->cost >= u2->cost
        && offense_value(u1) >= 0
        && offense_value(u1) <= offense_value(u2)
        && defense_value(u1) <= defense_value(u2)
        && Chassis[u1->chassis_type].speed <= Chassis[u2->chassis_type].speed
        && (Chassis[u2->chassis_type].triad != TRIAD_AIR || u1->chassis_type == u2->chassis_type)
        && !((u1->ability_flags & u2->ability_flags) ^ u1->ability_flags));
    if (val) {
        debug("unit_is_better %s -> %s\n", u1->name, u2->name);
    }
    return val;
}

int unit_score(int id, int faction, int cfactor, int minerals, int accumulated, bool defend) {
    assert(valid_player(faction) && id >= 0 && cfactor > 0);
    const int abls[][2] = {
        {ABL_AAA, 4},
        {ABL_AIR_SUPERIORITY, 2},
        {ABL_ALGO_ENHANCEMENT, 5},
        {ABL_AMPHIBIOUS, -3},
        {ABL_ARTILLERY, -1},
        {ABL_DROP_POD, 3},
        {ABL_EMPATH, 2},
        {ABL_TRANCE, 3},
        {ABL_SLOW, -4},
        {ABL_TRAINED, 2},
        {ABL_COMM_JAMMER, 3},
        {ABL_CLEAN_REACTOR, 2},
        {ABL_ANTIGRAV_STRUTS, 3},
        {ABL_BLINK_DISPLACER, 3},
        {ABL_DEEP_PRESSURE_HULL, 2},
        {ABL_SUPER_TERRAFORMER, 8},
    };
    UNIT* u = &Units[id];
    int v = 18 * (defend ? defense_value(u) : offense_value(u));
    if (v < 0) {
        v = (defend ? Armor[best_armor(faction, false)].defense_value
            : Weapon[best_weapon(faction)].offense_value)
            * (conf.ignore_reactor_power ? REC_FISSION : best_reactor(faction))
            + plans[faction].psi_score * 12;
    }
    if (u->triad() != TRIAD_AIR) {
        v += (defend ? 12 : 32) * u->speed();
        if (u->triad() == TRIAD_SEA && u->weapon_type <= WPN_PSI_ATTACK
        && defense_value(u) > offense_value(u)) {
            v -= 20;
        }
    }
    if (u->ability_flags & ABL_POLICE_2X && plans[faction].need_police) {
        v += 20;
    }
    for (const int* i : abls) {
        if (u->ability_flags & i[0]) {
            v += 8 * i[1];
        }
    }
    int turns = max(0, u->cost * cfactor - accumulated) / max(2, minerals);
    int score = v - turns * (u->weapon_type == WPN_COLONY_MODULE ? 6 : 3)
        * (max(3, 9 - *current_turn/10) + (minerals < 6 ? 2 : 0));
    debug("unit_score %s cfactor: %d minerals: %d cost: %d turns: %d score: %d\n",
        u->name, cfactor, minerals, u->cost, turns, score);
    return score;
}

/*
Find the best prototype for base production when weighted against cost given the triad
and type constraints. For any combat-capable unit, mode is set to COMBAT (= 0).
*/
int find_proto(int base_id, int triad, int mode, bool defend) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    assert(mode >= COMBAT && mode <= WMODE_INFOWAR);
    assert(valid_triad(triad));
    BASE* b = &Bases[base_id];
    int faction = b->faction_id;
    int basic = BSC_SCOUT_PATROL;
    debug("find_proto faction: %d triad: %d mode: %d defend: %d\n", faction, triad, mode, defend);

    if (mode == WMODE_COLONIST) {
        basic = (triad == TRIAD_SEA ? BSC_SEA_ESCAPE_POD : BSC_COLONY_POD);
    } else if (mode == WMODE_TERRAFORMER) {
        basic = (triad == TRIAD_SEA ? BSC_SEA_FORMERS : BSC_FORMERS);
    } else if (mode == WMODE_CONVOY) {
        basic = BSC_SUPPLY_CRAWLER;
    } else if (mode == WMODE_TRANSPORT) {
        basic = BSC_TRANSPORT_FOIL;
    } else if (mode == WMODE_INFOWAR) {
        basic = BSC_PROBE_TEAM;
    }
    int best = basic;
    int cfactor = cost_factor(faction, 1, -1);
    int best_val = unit_score(
        best, faction, cfactor, b->mineral_surplus, b->minerals_accumulated, defend);
    if (Units[best].triad() != triad) {
        best_val -= 40;
    }
    for (int i=0; i < 2*MaxProtoFactionNum; i++) {
        int id = (i < MaxProtoFactionNum ? i : (faction-1)*MaxProtoFactionNum + i);
        UNIT* u = &Units[id];
        if (u->unit_flags & UNIT_ACTIVE && strlen(u->name) > 0 && u->triad() == triad && id != best) {
            if (id < MaxProtoFactionNum && !has_tech(faction, u->preq_tech)) {
                continue;
            }
            if ((mode && Weapon[u->weapon_type].mode != mode)
            || (!mode && Weapon[u->weapon_type].offense_value == 0)
            || (!mode && defend && u->chassis_type != CHS_INFANTRY)
            || (u->weapon_type == WPN_PSI_ATTACK && plans[faction].psi_score < 1)
            || u->weapon_type == WPN_PLANET_BUSTER
            || !(mode != COMBAT || best == basic || (defend == (offense_value(u) < defense_value(u))))) {
                continue;
            }
            int val = unit_score(
                id, faction, cfactor, b->mineral_surplus, b->minerals_accumulated, defend);
            if (unit_is_better(&Units[best], u) || random(100) > 50 + best_val - val) {
                best = id;
                best_val = val;
                debug("===> %s\n", Units[best].name);
            }
        }
    }
    return best;
}

int hurry_item(BASE* b, int mins, int cost) {
    Faction* f = &Factions[b->faction_id];
    f->energy_credits -= cost;
    b->minerals_accumulated += mins;
    b->state_flags |= BSTATE_HURRY_PRODUCTION;
    debug("hurry_item %d %d %d %d %d %s %s\n", *current_turn, b->faction_id,
        mins, cost, f->energy_credits, b->name, prod_name(b->queue_items[0]));
    return 1;
}

int consider_hurry() {
    const int id = *current_base_id;
    BASE* b = &Bases[id];
    int t = b->queue_items[0];
    int faction = b->faction_id;
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    assert(b == *current_base_ptr);

    if (!ai_enabled(faction)) {
        return base_hurry();
    }
    if (conf.hurry_items < 1 || t <= -FAC_STOCKPILE_ENERGY
    || b->state_flags & (BSTATE_PRODUCTION_DONE | BSTATE_HURRY_PRODUCTION)) {
        return 0;
    }
    bool cheap = b->minerals_accumulated >= Rules->retool_exemption;
    int reserve = 20 + min(900, max(0, f->current_num_bases * min(30, (*current_turn - 20)/5)));
    // Fix: veh_cost takes possible prototype costs into account.
    int mins = (t >= 0 ? (veh_cost(t, id, 0) * cost_factor(faction, 1, -1)) : mineral_cost(faction, t))
        - b->minerals_accumulated;
    int cost = (t < 0 ? 2*mins : mins*mins/20 + 2*mins) * (cheap ? 1 : 2) * m->rule_hurry / 100;
    int turns = (int)ceil(mins / max(0.01, 1.0 * b->mineral_surplus));

    if (!(cheap && mins > 0 && cost > 0 && f->energy_credits - cost > reserve)) {
        return 0;
    }
    if (b->drone_total > b->talent_total && t < 0 && t == need_psych(id)) {
        return hurry_item(b, mins, cost);
    }
    if (t < 0 && turns > 1) {
        if (t == -FAC_RECYCLING_TANKS || t == -FAC_PRESSURE_DOME
        || t == -FAC_RECREATION_COMMONS || t == -FAC_TREE_FARM
        || t == -FAC_PUNISHMENT_SPHERE || t == -FAC_HEADQUARTERS)
            return hurry_item(b, mins, cost);
        if (t == -FAC_CHILDREN_CRECHE && f->SE_growth >= 2)
            return hurry_item(b, mins, cost);
        if (t == -FAC_PERIMETER_DEFENSE && m->thinker_enemy_range < 20)
            return hurry_item(b, mins, cost);
        if (t == -FAC_AEROSPACE_COMPLEX && has_tech(faction, TECH_Orbital))
            return hurry_item(b, mins, cost);
    }
    if (t >= 0 && turns > 1) {
        if (m->thinker_enemy_range < 20 && Units[t].weapon_type <= WPN_PSI_ATTACK
        && !has_defenders(b->x, b->y, faction)) {
            return hurry_item(b, mins, cost);
        }
        if (Units[t].weapon_type == WPN_TERRAFORMING_UNIT && b->pop_size < 3) {
            return hurry_item(b, mins, cost);
        }
    }
    return 0;
}

int find_facility(int id) {
    const int SecretProject = -1;
    const int F_Mineral = 1;
    const int F_Energy = 2;
    const int F_Repair = 4;
    const int F_Combat = 8;
    const int F_Trees = 16;
    const int F_Space = 32;
    const int F_Surplus = 64;

    const int build_order[][2] = {
        {FAC_RECYCLING_TANKS,       0},
        {FAC_RECREATION_COMMONS,    0},
        {SecretProject,             0},
        {FAC_CHILDREN_CRECHE,       0},
        {FAC_HAB_COMPLEX,           0},
        {FAC_ORBITAL_DEFENSE_POD,   F_Space},
        {FAC_NESSUS_MINING_STATION, F_Space},
        {FAC_ORBITAL_POWER_TRANS,   F_Space},
        {FAC_SKY_HYDRO_LAB,         F_Space},
        {FAC_PERIMETER_DEFENSE,     F_Combat},
        {FAC_GENEJACK_FACTORY,      F_Mineral},
        {FAC_ROBOTIC_ASSEMBLY_PLANT,F_Mineral},
        {FAC_NANOREPLICATOR,        F_Mineral},
        {FAC_AEROSPACE_COMPLEX,     0},
        {FAC_TREE_FARM,             F_Energy|F_Trees},
        {FAC_NETWORK_NODE,          F_Energy},
        {FAC_COMMAND_CENTER,        F_Repair|F_Combat},
        {FAC_NAVAL_YARD,            F_Repair|F_Combat},
        {FAC_GEOSYNC_SURVEY_POD,    F_Surplus},
        {FAC_HABITATION_DOME,       0},
        {FAC_FUSION_LAB,            F_Energy},
        {FAC_ENERGY_BANK,           F_Energy},
        {FAC_RESEARCH_HOSPITAL,     F_Energy},
        {FAC_TACHYON_FIELD,         F_Surplus|F_Combat},
        {FAC_FLECHETTE_DEFENSE_SYS, F_Surplus|F_Combat},
        {FAC_QUANTUM_LAB,           F_Energy|F_Surplus},
        {FAC_NANOHOSPITAL,          F_Energy|F_Surplus},
        {FAC_HYBRID_FOREST,         F_Energy|F_Trees|F_Surplus},
    };
    BASE* base = &Bases[id];
    int faction = base->faction_id;
    int minerals = base->mineral_surplus + base->minerals_accumulated/10;
    int pop_rule = MFactions[faction].rule_population;
    int hab_complex_limit = Rules->pop_limit_wo_hab_complex - pop_rule;
    int hab_dome_limit = Rules->pop_limit_wo_hab_dome - pop_rule;
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    bool sea_base = is_ocean(base);
    bool core_base = minerals >= plans[faction].proj_limit;
    bool can_build_units = can_build_unit(faction, -1);
    int cfactor = cost_factor(faction, 1, -1);
    int forest = nearby_items(base->x, base->y, 1, BIT_FOREST);

    for (const int* item : build_order) {
        int t = item[0];
        if (t == SecretProject) {
            if (core_base && (t = find_project(id)) != 0) {
                return t;
            }
            continue;
        }
        CFacility* fc = &Facility[t];
        int turns = max(0, fc->cost * min(12, cfactor) - base->minerals_accumulated)
            / max(2, base->mineral_surplus);
        if (t == FAC_RECYCLING_TANKS && base->drone_total > 0 && can_build(id, FAC_RECREATION_COMMONS))
            continue;
        /* Check if we have sufficient base energy for multiplier facilities. */
        if (item[1] & F_Energy && base->energy_surplus < 2*fc->maint + fc->cost)
            continue;
        if (item[1] & F_Energy && turns > 5 + f->AI_tech + f->AI_wealth - f->AI_power - f->AI_fight)
            continue;
        /* Avoid building combat-related facilities in peacetime. */
        if (item[1] & F_Combat && m->thinker_enemy_range > MaxEnemyRange - 5*min(5, fc->maint))
            continue;
        if (item[1] & F_Repair && (!conf.repair_base_facility || f->SE_morale < 0))
            continue;
        if (item[1] & F_Surplus && turns > (can_build_units ? 5 : 8))
            continue;
        if (item[1] & F_Mineral && (base->mineral_intake_2 > (core_base ? 80 : 50)
        || 3*base->mineral_intake_2 > 2*(conf.clean_minerals + f->clean_minerals_modifier)
        || turns > 5 + f->AI_power + f->AI_wealth))
            continue;
        if (item[1] & F_Space && (!core_base || !has_facility(id, FAC_AEROSPACE_COMPLEX)))
            continue;
        if (item[1] & F_Trees && base->eco_damage < 2 && forest < 3)
            continue;
        if (t == FAC_COMMAND_CENTER && (sea_base || turns > 5))
            continue;
        if (t == FAC_NAVAL_YARD && (!sea_base || turns > 5))
            continue;
        if (t == FAC_HAB_COMPLEX && base->pop_size < hab_complex_limit)
            continue;
        if (t == FAC_HABITATION_DOME && base->pop_size < hab_dome_limit)
            continue;
        if (can_build(id, t)) {
            return -t;
        }
    }
    if (!can_build_units) {
        return -FAC_STOCKPILE_ENERGY;
    }
    debug("BUILD OFFENSE\n");
    return select_combat(id, sea_base, can_build_ships(id));
}

int select_colony(int id, int pods, bool build_ships) {
    BASE* base = &Bases[id];
    int faction = base->faction_id;
    bool land = has_base_sites(base->x, base->y, faction, TRIAD_LAND);
    bool sea = has_base_sites(base->x, base->y, faction, TRIAD_SEA);
    int limit = (*current_turn < 60 || (!random(3) && (land || (build_ships && sea))) ? 2 : 1);

    if (pods >= limit) {
        return -1;
    }
    if (is_ocean(base)) {
        for (const auto& m : iterate_tiles(base->x, base->y, 1, 9)) {
            if (land && non_combat_move(m.x, m.y, faction, TRIAD_LAND)) {
                if (m.sq->owner < 1 || !random(6)) {
                    return TRIAD_LAND;
                }
            }
        }
        if (sea) {
            return TRIAD_SEA;
        }
    } else {
        if (build_ships && sea && (!land || (*current_turn > 50 && !random(3)))) {
            return TRIAD_SEA;
        } else if (land) {
            return TRIAD_LAND;
        }
    }
    return -1;
}

int select_combat(int base_id, bool sea_base, bool build_ships) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    bool reserve = base->mineral_surplus >= base->mineral_intake_2/2;
    int probes = 0;

    int w1 = 4*plans[faction].air_combat_units < f->current_num_bases ? 2 : 5;
    int w2 = 4*plans[faction].transport_units < f->current_num_bases ? 2 : 5;

    for (int i=0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction && veh->home_base_id == base_id && veh->is_probe()) {
            probes++;
        }
    }
    if (has_weapon(faction, WPN_PROBE_TEAM) && (!random(probes*2 + 4) || !reserve)) {
        return find_proto(base_id, (build_ships ? TRIAD_SEA : TRIAD_LAND), WMODE_INFOWAR, DEF);
    }
    if (has_chassis(faction, CHS_NEEDLEJET)
    && (f->SE_police >= -3 || !base_can_riot(base_id)) && !random(w1)) {
        return find_proto(base_id, TRIAD_AIR, COMBAT, ATT);
    }
    if (build_ships) {
        MAP* sq = mapsq(base->x, base->y);
        int base_region = (sq ? sq->region : 0);
        int score = 90;

        for (int i=0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (at_war(faction, b->faction_id) && (sq = mapsq(b->x, b->y))) {
                int range = (sea_base ? 2 : 1)
                    * (sq->region != base_region ? 2 : 1)
                    * (plans[faction].prioritize_naval ? 2 : 1)
                    * map_range(base->x, base->y, b->x, b->y);
                score = min(score, range);
            }
        }
        if (random(100) < score) {
            return find_proto(base_id, TRIAD_SEA, (!random(w2) ? WMODE_TRANSPORT : COMBAT), ATT);
        }
    }
    return find_proto(base_id, TRIAD_LAND, COMBAT, (sea_base || !random(5) ? DEF : ATT));
}

int select_production(const int id) {
    BASE* base = &Bases[id];
    MAP* sq = mapsq(base->x, base->y);
    int faction = base->faction_id;
    int minerals = base->mineral_surplus;
    int base_region = (sq ? sq->region : 0);
    Faction* f = &Factions[faction];
    MFaction* m = &MFactions[faction];
    AIPlans* p = &plans[faction];

    int reserve = max(2, base->mineral_intake_2 / 2);
    bool has_formers = has_weapon(faction, WPN_TERRAFORMING_UNIT);
    bool has_supply = has_weapon(faction, WPN_SUPPLY_TRANSPORT);
    bool build_ships = can_build_ships(id);
    bool sea_base = is_ocean(base);
    int transports = 0;
    int landprobes = 0;
    int seaprobes = 0;
    int defenders = 0;
    int crawlers = 0;
    int formers = 0;
    int scouts = 0;
    int pods = 0;
    int enemy_range = MaxEnemyRange;

    for (int i=0; i < *total_num_bases; i++) {
        BASE* b = &Bases[i];
        if (at_war(faction, b->faction_id) && (sq = mapsq(b->x, b->y))) {
            int range = map_range(base->x, base->y, b->x, b->y);
            if (sq->region != base_region) {
                range = range*3/2;
            }
            enemy_range = min(enemy_range, range);
        }
    }
    for (int i=0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id != faction) {
            continue;
        }
        if (veh->home_base_id == id) {
            if (veh->is_former()) {
                formers++;
            } else if (veh->is_colony()) {
                pods++;
            } else if (veh->is_probe()) {
                if (veh->triad() == TRIAD_LAND) {
                    landprobes++;
                } else {
                    seaprobes++;
                }
            } else if (veh->is_supply()) {
                crawlers++;
                if (veh->move_status != ORDER_CONVOY) {
                    has_supply = false;
                }
            } else if (veh->is_transport()) {
                transports++;
            }
        }
        if (veh->is_combat_unit() && veh->triad() == TRIAD_LAND) {
            if (map_range(base->x, base->y, veh->x, veh->y) <= 1) {
                defenders++;
            }
            if (veh->home_base_id == id) {
                scouts++;
            }
        }
    }
    bool build_pods = allow_expand(faction) && pods < 2
        && (base->pop_size > 1 || base->nutrient_surplus > 1);
    m->thinker_enemy_range = (enemy_range + 15 * m->thinker_enemy_range)/16;

    double w1 = min(1.0, max(0.4, 1.0 * minerals / p->proj_limit))
        * (conf.conquer_priority / 100.0)
        * (base_region == p->target_land_region && p->main_region != p->target_land_region ? 3 : 1);
    double w2 = 2.0 * p->enemy_mil_factor / (m->thinker_enemy_range * 0.1 + 0.1)
        + 0.8 * p->enemy_bases + min(0.4, *current_turn/400.0)
        + min(1.0, 1.5 * f->current_num_bases / *map_area_sq_root) * (f->AI_fight * 0.2 + 0.8);
    double threat = 1 - (1 / (1 + max(0.0, w1 * w2)));

    debug("select_prod %d %d %2d %2d def: %d frm: %d prb: %d crw: %d pods: %d expand: %d "\
        "scouts: %d min: %2d res: %2d limit: %2d range: %2d mil: %.4f threat: %.4f\n",
        *current_turn, faction, base->x, base->y,
        defenders, formers, landprobes+seaprobes, crawlers, pods, build_pods,
        scouts, minerals, reserve, p->proj_limit, enemy_range, p->enemy_mil_factor, threat);

    if (minerals > 2 && (defenders < 1 || need_scouts(base->x, base->y, faction, scouts))) {
        return find_proto(id, TRIAD_LAND, COMBAT, DEF);
    }
    if (*climate_future_change > 0 && is_shore_level(mapsq(base->x, base->y))
    && can_build(id, FAC_PRESSURE_DOME)) {
            return -FAC_PRESSURE_DOME;
    }
    if (!conf.auto_relocate_hq && relocate_hq(id)) {
        return -FAC_HEADQUARTERS;
    }
    if (minerals > reserve && random(100) < (int)(100 * threat)) {
        return select_combat(id, sea_base, build_ships);
    }
    if (has_formers && formers < (base->pop_size < (sea_base ? 4 : 3) ? 1 : 2)) {
        int num = 0;
        int sea = 0;
        for (auto& t : iterate_tiles(base->x, base->y, 1, 21)) {
            if (t.sq->owner == faction && select_item(t.x, t.y, faction, t.sq) >= 0) {
                num++;
                sea += is_ocean(t.sq);
            }
        }
        if (num > 3) {
            if (has_chassis(faction, CHS_GRAVSHIP) && minerals >= Chassis[CHS_GRAVSHIP].cost) {
                int unit = find_proto(id, TRIAD_AIR, WMODE_TERRAFORMER, DEF);
                if (unit >= 0 && Units[unit].triad() == TRIAD_AIR) {
                    return unit;
                }
            }
            if (build_ships && sea*2 >= num) {
                return find_proto(id, TRIAD_SEA, WMODE_TERRAFORMER, DEF);
            }
            return find_proto(id, TRIAD_LAND, WMODE_TERRAFORMER, DEF);
        }
    }
    if (build_ships && has_weapon(faction, WPN_PROBE_TEAM)
    && ocean_coast_tiles(base->x, base->y) && !random(seaprobes > 0 ? 6 : 3)
    && p->unknown_factions > 1 && p->contacted_factions < 2) {
        return find_proto(id, TRIAD_SEA, WMODE_INFOWAR, DEF);
    }
    if (base->eco_damage > 0 && can_build(id, FAC_TREE_FARM)
    && minerals >= Facility[FAC_TREE_FARM].cost
    && Factions[faction].SE_planet_base >= 0) {
        return -FAC_TREE_FARM;
    }
    int nearby = 1;
    for (auto& t : iterate_tiles(base->x, base->y, 1, 49)) {
        if (t.sq->is_base()) {
            nearby++;
        }
    }
    int crawl_target = min(1 + base->pop_size/4,
        (minerals >= p->proj_limit ? (nearby < 3 ? 2 : 1) : 0) + max(0, 3 - nearby/2));
    if (has_supply && !sea_base && crawlers < crawl_target
    && Weapon[WPN_SUPPLY_TRANSPORT].cost/2 <= minerals) {
        return find_proto(id, TRIAD_LAND, WMODE_CONVOY, DEF);
    }
    if (build_ships && !transports && mapnodes.count({base->x, base->y, NODE_NEED_FERRY})) {
        return find_proto(id, TRIAD_SEA, WMODE_TRANSPORT, DEF);
    }
    if (build_pods && !can_build(id, FAC_RECYCLING_TANKS)) {
        int tr = select_colony(id, pods, build_ships);
        if (tr == TRIAD_LAND || tr == TRIAD_SEA) {
            return find_proto(id, tr, WMODE_COLONIST, DEF);
        }
    }
    return find_facility(id);
}






