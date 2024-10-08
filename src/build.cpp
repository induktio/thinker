
#include "build.h"


int __cdecl mod_base_hurry() {
    const int base_id = *CurrentBaseID;
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    AIPlans* p = &plans[b->faction_id];
    assert(b == *CurrentBase);
    const int t = b->item();
    bool is_cheap = conf.simple_hurry_cost || b->minerals_accumulated >= Rules->retool_exemption;
    bool is_project = t <= -SP_ID_First && t >= -SP_ID_Last;
    bool player_gov = is_human(b->faction_id);
    int hurry_option;

    if (player_gov) {
        if (!conf.manage_player_bases) {
            return base_hurry();
        }
        hurry_option = (b->governor_flags & GOV_ACTIVE) &&
            (b->governor_flags & GOV_MAY_HURRY_PRODUCTION) ?
            ((b->governor_flags & GOV_MAY_PROD_SP) ? 2 : 1) : 0;
    } else {
        if (!thinker_enabled(b->faction_id)) {
            return base_hurry();
        }
        hurry_option = conf.base_hurry;
    }
    if (hurry_option < (is_project ? 2 : 1)
    || t == -FAC_STOCKPILE_ENERGY || t < -SP_ID_Last
    || b->state_flags & (BSTATE_PRODUCTION_DONE | BSTATE_HURRY_PRODUCTION)) {
        return 0;
    }
    int mins = mineral_cost(base_id, t) - b->minerals_accumulated;
    int cost = hurry_cost(base_id, t, mins);
    int turns = prod_turns(base_id, t);
    int reserve = clamp(*CurrentTurn * f->base_count / 8, 20, 500)
        * (!p->contacted_factions || is_project ? 1 : 4)
        * (b->defend_goal > 3 && p->enemy_factions > 0 ? 1 : 2)
        * (has_fac_built(FAC_HEADQUARTERS, base_id) ? 1 : 2) / 16;

    if (!is_cheap || mins < 1 || cost < 1 || f->energy_credits - cost < reserve) {
        return 0;
    }
    if (is_project) {
        int delay = player_gov ? 0 : clamp((*DiffLevel < DIFF_THINKER) + random(4), 0, 3);
        int threshold = 4*mod_cost_factor(b->faction_id, RSC_MINERAL, -1);
        WItem Wgov;
        governor_priorities(Bases[base_id], Wgov);

        if (has_project((FacilityId)-t) || turns < 2 + delay
        || (b->defend_goal > 3 && p->enemy_factions > 0)
        || (delay > 0 && b->mineral_surplus < 4)
        || b->minerals_accumulated < threshold) { // Avoid double cost threshold
            return 0;
        }
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == b->faction_id
            && Bases[i].item() == t
            && Bases[i].minerals_accumulated > b->minerals_accumulated) {
                return 0;
            }
        }
        int num = 0;
        int proj_score = 0;
        int values[256];
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            if (Facility[i].preq_tech != TECH_Disable) {
                values[num] = facility_score((FacilityId)i, Wgov) + random(8);
                if (i == -t) {
                    proj_score = values[num];
                }
                num++;
            }
        }
        std::sort(values, values+num);
        if (proj_score < max(4, values[num/2])) {
            return 0;
        }
        mins = mineral_cost(base_id, t) - b->minerals_accumulated
            - b->mineral_surplus/2 - delay*b->mineral_surplus;
        cost = hurry_cost(base_id, t, mins);

        if (cost > 0 && cost < f->energy_credits && mins > 0 && mins > b->mineral_surplus) {
            hurry_item(base_id, mins, cost);
            if (DEBUG && conf.minimal_popups) {
                return 1;
            }
            if (!(*GameState & STATE_GAME_DONE) && b->faction_id != MapWin->cOwner
            && has_treaty(b->faction_id, MapWin->cOwner, DIPLO_COMMLINK)) {
                parse_says(0, MFactions[b->faction_id].adj_name_faction, -1, -1);
                parse_says(1, Facility[-t].name, -1, -1);
                popp(ScriptFile, "DONEPROJECT", 0, "secproj_sm.pcx", 0);
            }
            return 1;
        }
        return 0;
    }
    if (t < 0 && turns > 1 && cost < f->energy_credits/8) {
        if (t == -FAC_RECYCLING_TANKS || t == -FAC_PRESSURE_DOME
        || t == -FAC_TREE_FARM || t == -FAC_HEADQUARTERS) {
            return hurry_item(base_id, mins, cost);
        }
        if ((t == -FAC_RECREATION_COMMONS || t == -FAC_PUNISHMENT_SPHERE
        || (t == -FAC_NETWORK_NODE && has_project(FAC_VIRTUAL_WORLD, b->faction_id)))
        && b->drone_total + b->specialist_adjust > b->talent_total) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_CHILDREN_CRECHE && base_unused_space(base_id) > 2) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_HAB_COMPLEX && base_unused_space(base_id) == 0) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_PERIMETER_DEFENSE && b->defend_goal > 2 && p->enemy_factions > 0) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_AEROSPACE_COMPLEX && has_tech(TECH_Orbital, b->faction_id)) {
            return hurry_item(base_id, mins, cost);
        }
    }
    if (t >= 0 && turns > 1 && cost < f->energy_credits/4) {
        if (Units[t].is_combat_unit()) {
            int val = (b->defend_goal > 2)
                + (b->defend_goal > 3)
                + (p->enemy_bases > 0)
                + (p->enemy_factions > 0)
                + (at_war(b->faction_id, MapWin->cOwner) > 0)
                + (cost < f->energy_credits/16)
                + 2*(!has_defenders(b->x, b->y, b->faction_id));
            if (val > 3) {
                return hurry_item(base_id, mins, cost);
            }
        }
        if (Units[t].is_former() && turns > (*CurrentTurn + base_id) % 16
        && (cost < f->energy_credits/16 || b->mineral_surplus < p->median_limit)) {
            return hurry_item(base_id, mins, cost);
        }
        if (Units[t].is_colony() && b->pop_size > 1
        && (cost < f->energy_credits/16 || turns > (*CurrentTurn + base_id) % 16)
        && (b->drone_riots_active()
        || (!base_unused_space(base_id) && b->nutrient_surplus > 1)
        || (base_can_riot(base_id, true)
        && b->drone_total + b->specialist_adjust > b->talent_total))) {
            return hurry_item(base_id, mins, cost);
        }
    }
    return 0;
}

int hurry_item(int base_id, int mins, int cost) {
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    debug("hurry_item %d %d mins: %d cost: %d credits: %d %s / %s\n",
        *CurrentTurn, base_id, mins, cost, f->energy_credits, b->name, prod_name(b->item()));
    f->energy_credits -= cost;
    b->minerals_accumulated += mins;
    b->state_flags |= BSTATE_HURRY_PRODUCTION;

    // This also enables "zoom to base" feature in the message window
    if (b->faction_id == MapWin->cOwner) {
        parse_says(0, b->name, -1, -1);
        parse_says(1, prod_name(b->item()), -1, -1);
        popb("GOVHURRY", 0, -1, "facblt_sm.pcx", 0);
    }
    return 1;
}

int consider_staple(int base_id) {
    BASE* b = &Bases[base_id];
    if (can_staple(base_id)
    && (!un_charter() || (*SunspotDuration > 1 && *DiffLevel >= DIFF_LIBRARIAN))
    && base_can_riot(base_id, true)
    && b->nerve_staple_count < 4
    && b->pop_size / 4 >= b->nerve_staple_count
    && b->drone_total + b->specialist_adjust > 0
    && (b->faction_id == b->faction_id_former || !is_alive(b->faction_id_former)
    || want_revenge(b->faction_id, b->faction_id_former))) {
        if (b->drone_riots_active() || b->drone_total + b->specialist_adjust
        > b->talent_total + (b->nutrient_surplus > 0 && b->mineral_surplus > 0)) {
            action_staple(base_id);
        }
    }
    return 0;
}

bool need_scouts(int base_id, int scouts) {
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    MAP* sq = mapsq(b->x, b->y);
    if (is_ocean(sq)) {
        return false;
    }
    int nearby_pods = Continents[sq->region].pods
        * min(2*f->region_territory_tiles[sq->region], Continents[sq->region].tile_count)
        / max(1, Continents[sq->region].tile_count);
    int score = max(-2, 5 - *CurrentTurn/10)
        + 2*(*MapNativeLifeForms) - 3*scouts
        + min(4, nearby_pods / 6);

    bool val = random(16) < score;
    debug("need_scouts %d %d value: %d score: %d scouts: %d goodies: %d native: %d\n",
        *CurrentTurn, base_id, val, score, scouts, Continents[sq->region].pods, *MapNativeLifeForms);
    return val;
}

bool redundant_project(int faction_id, int item_id) {
    Faction* f = &Factions[faction_id];
    if (item_id == FAC_PLANETARY_DATALINKS) {
        int n = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (Factions[i].base_count > 0) {
                n++;
            }
        }
        return n < 4;
    }
    if (item_id == FAC_CITIZENS_DEFENSE_FORCE) {
        return Facility[FAC_PERIMETER_DEFENSE].maint == 0
            && facility_count(FAC_PERIMETER_DEFENSE, faction_id) > f->base_count/2 + 2;
    }
    if (item_id == FAC_MARITIME_CONTROL_CENTER) {
        int n = 0;
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehicles[i];
            if (veh->faction_id == faction_id && veh->triad() == TRIAD_SEA) {
                n++;
            }
        }
        return n < 8 && n < f->base_count/3;
    }
    if (item_id == FAC_HUNTER_SEEKER_ALGORITHM) {
        return f->SE_probe >= 3;
    }
    if (item_id == FAC_LIVING_REFINERY) {
        return f->SE_support >= 3;
    }
    return false;
}

int find_satellite(int base_id) {
    const int satellites[] = {
        FAC_ORBITAL_DEFENSE_POD,
        FAC_NESSUS_MINING_STATION,
        FAC_ORBITAL_POWER_TRANS,
        FAC_SKY_HYDRO_LAB,
    };
    BASE& base = Bases[base_id];
    Faction& f = Factions[base.faction_id];
    AIPlans& p = plans[base.faction_id];
    bool has_complex = has_facility(FAC_AEROSPACE_COMPLEX, base_id)
        || has_project(FAC_SPACE_ELEVATOR, base.faction_id);

    if (!has_complex && ((base_id + *CurrentTurn)/8) & 1) {
        return 0;
    }
    if (!has_complex && !can_build(base_id, FAC_AEROSPACE_COMPLEX)) {
        return 0;
    }
    bool defense_only = clamp(p.enemy_odp - f.satellites_ODP + 7, 0, 14) > random(16);
    for (const int item_id : satellites) {
        if (!has_tech(Facility[item_id].preq_tech, base.faction_id)) {
            continue;
        }
        if (item_id != FAC_ORBITAL_DEFENSE_POD && defense_only) {
            continue;
        }
        int prod_num = prod_count(-item_id, base.faction_id, base_id);
        int built_num = satellite_count(base.faction_id, item_id);
        int goal_num = satellite_goal(base.faction_id, item_id);

        if (built_num + prod_num >= goal_num) {
            continue;
        }
        if (!has_complex && can_build(base_id, FAC_AEROSPACE_COMPLEX)) {
            if (min(12, prod_turns(base_id, -FAC_AEROSPACE_COMPLEX)) < 2+random(16)) {
                return -FAC_AEROSPACE_COMPLEX;
            }
            return 0;
        }
        if (has_complex && min(12, prod_turns(base_id, -item_id)) < 2+random(16)) {
            return -item_id;
        }
    }
    return 0;
}

int find_planet_buster(int faction_id) {
    int best_id = -1;
    for (int unit_id = 0; unit_id < MaxProtoNum; unit_id++) {
        UNIT* u = &Units[unit_id];
        if ((unit_id < MaxProtoFactionNum || unit_id / MaxProtoFactionNum == faction_id)
        && u->is_planet_buster() && mod_veh_avail(unit_id, faction_id, -1)
        && (best_id < 0 || u->std_offense_value() > Units[best_id].std_offense_value())) {
            best_id = unit_id;
        }
    }
    return best_id;
}

int find_project(int base_id, WItem& Wgov) {
    BASE* base = &Bases[base_id];
    Faction* f = &Factions[base->faction_id];
    int gov = base->gov_config();
    int faction = base->faction_id;
    int projs = 0;
    int nukes = 0;
    int works = 0;
    int bases = f->base_count;
    int diplo = plans[faction].diplo_flags;
    int unit_id = (gov & (GOV_MAY_PROD_AIR_COMBAT | GOV_MAY_PROD_AIR_DEFENSE)
        ? find_planet_buster(faction) : -1);
    int built_nukes = 0;
    int enemy_nukes = 0;
    if (unit_id >= 0) {
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->is_planet_buster()) {
                if (faction == veh->faction_id) {
                    built_nukes++;
                } else if (at_war(faction, veh->faction_id)) {
                    enemy_nukes++;
                }
            }
        }
    }
    int nuke_score = (un_charter() ? 0 : 4) + 2*f->AI_power + 2*f->AI_fight
        + clamp(enemy_nukes - f->satellites_ODP, -2, 4)
        + (diplo & DIPLO_ATROCITY_VICTIM ? 4 : 0)
        + (diplo & DIPLO_WANT_REVENGE ? 4 : 0)
        + min(4, base->mineral_surplus / 32);
    int nuke_limit = (unit_id >= 0 && bases >= 8 && nuke_score > 3 &&
        built_nukes < clamp(1 + (f->AI_fight > 0) + nuke_score/8 + bases/32, 1, 3)
        ? (bases < 32 ? 1 : 2) : 0);
    debug("find_project %d score: %d limit: %d built: %d enemy: %d\n",
        faction, nuke_score, nuke_limit, built_nukes, enemy_nukes);

    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction) {
            int t = Bases[i].item();
            if (t <= -SP_ID_First || t == -FAC_SUBSPACE_GENERATOR) {
                projs++;
            } else if (t == -FAC_SKUNKWORKS) {
                works++;
            } else if (t >= 0 && Units[t].is_planet_buster()) {
                nukes++;
            }
        }
    }
    if (unit_id >= 0 && nukes < nuke_limit
    && (!un_charter() || !random(4) || has_fac_built(FAC_SKUNKWORKS, base_id))) {
        debug("find_project %d %d %s\n", faction, unit_id, Units[unit_id].name);
        int extra_cost = !Units[unit_id].is_prototyped()
            && unit_id >= MaxProtoFactionNum ? prototype_factor(unit_id) : 0;
        bool free_protos = !extra_cost || has_fac_built(FAC_SKUNKWORKS, base_id);
        if (gov & GOV_MAY_PROD_PROTOTYPE || free_protos) {
            if (extra_cost >= 50 && gov & GOV_MAY_PROD_FACILITIES
            && can_build(base_id, FAC_SKUNKWORKS) && works < 2) {
                return -FAC_SKUNKWORKS;
            }
            if ((extra_cost <= (works ? 0 : 50)
            || has_fac_built(FAC_SKUNKWORKS, base_id))) {
                return unit_id;
            }
        }
    }
    int similar_limit = (base->minerals_accumulated >= 50 ? 2 : 1);
    if (projs + nukes < min(3 + nuke_limit, bases/4)) {
        if (can_build(base_id, FAC_SUBSPACE_GENERATOR)) {
            return -FAC_SUBSPACE_GENERATOR;
        }
        int best_score = INT_MIN;
        int choice = 0;
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            if (can_build(base_id, i) && prod_count(-i, faction, base_id) < similar_limit
            && (similar_limit > 1 || !redundant_project(faction, i))) {
                int score = facility_score((FacilityId)i, Wgov);
                if (score > best_score) {
                    choice = i;
                    best_score = score;
                }
                debug("find_project %d %d %d %s\n", faction, i, score, Facility[i].name);
            }
        }
        return (projs > 0 || best_score > 3 ? -choice : 0);
    }
    return 0;
}

/*
Return true if unit2 is strictly better than unit1 in all circumstances (non PSI).
Disable random chance in prototype choices in these instances.
*/
bool unit_is_better(int unit_id1, int unit_id2) {
    assert(unit_id1 >= 0 && unit_id2 >= 0);
    UNIT* u1 = &Units[unit_id1];
    UNIT* u2 = &Units[unit_id2];
    bool val = (u1->cost >= u2->cost
        && proto_offense(unit_id1) >= 0
        && proto_offense(unit_id1) <= proto_offense(unit_id2)
        && proto_defense(unit_id1) <= proto_defense(unit_id2)
        && Chassis[u1->chassis_id].speed <= Chassis[u2->chassis_id].speed
        && (Chassis[u2->chassis_id].triad != TRIAD_AIR || u1->chassis_id == u2->chassis_id)
        && !((u1->ability_flags & u2->ability_flags) ^ u1->ability_flags))
        && (u1->ability_flags & ABL_SLOW) <= (u2->ability_flags & ABL_SLOW);
    if (val) {
        debug("unit_is_better %s -> %s\n", u1->name, u2->name);
    }
    return val;
}

int unit_score(BASE* base, int unit_id, int psi_score, bool defend) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    const int specials[][2] = {
        {ABL_AAA, 4},
        {ABL_AIR_SUPERIORITY, 2},
        {ABL_ALGO_ENHANCEMENT, 5},
        {ABL_AMPHIBIOUS, -2},
        {ABL_DROP_POD, 4},
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
    UNIT* u = &Units[unit_id];
    int v = 16 * (defend ? proto_defense(unit_id) : proto_offense(unit_id));
    if (v < 0) {
        v = 12 * (defend ? Armor[best_armor(base->faction_id, -1)].defense_value
            : Weapon[best_weapon(base->faction_id)].offense_value)
            * (conf.ignore_reactor_power ? REC_FISSION : best_reactor(base->faction_id))
            + psi_score * 16;
    }
    if (u->triad() != TRIAD_AIR) {
        v += (defend ? 12 : 32) * u->speed();
        if (u->triad() == TRIAD_SEA && u->is_combat_unit()
        && proto_defense(unit_id) > proto_offense(unit_id)) {
            v -= 24;
        }
    }
    if (u->ability_flags & ABL_ARTILLERY) {
        if (conf.long_range_artillery > 0 && Rules->artillery_max_rng <= 4
        && !*MultiplayerActive && u->triad() == TRIAD_SEA && u->offense_value() > 0) {
            v += (conf.long_range_artillery > 1 ? 24 : 16);
        } else {
            v -= 8;
        }
    }
    if (u->ability_flags & ABL_POLICE_2X && need_police(base->faction_id)) {
        v += (u->speed() > 1 ? 16 : 32);
        v += 8*min(4, base->specialist_adjust);
    }
    if (u->is_missile()) {
        v -= 8 * plans[base->faction_id].missile_units;
    }
    for (const int* s : specials) {
        if (u->ability_flags & s[0]) {
            v += 8 * s[1];
        }
    }
    // More detailed prototype cost calculations are skipped here
    int turns = max(0, u->cost*10 - base->minerals_accumulated) / max(2, base->mineral_surplus);
    int score = v - turns * (u->is_colony() ? 6 : 3)
        * (max(2, 7 - *CurrentTurn/16) + max(0, 2 - base->mineral_surplus/4));
    debug("unit_score %3d psi: %d cost: %d turns: %d score: %d %s\n",
        unit_id, psi_score, u->cost, turns, score, u->name);
    return score;
}

/*
Find the best prototype for base production when weighted against cost given the triad
and type constraints. For any combat-capable unit, mode is set to WMODE_COMBAT.
This assumes only Scout Patrol as the fallback choice for land combat units.
For all other unit types requirements for building the prototype are always checked.
*/
int find_proto(int base_id, Triad triad, VehWeaponMode mode, bool defend) {
    assert(base_id >= 0 && base_id < *BaseCount);
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    debug("find_proto faction: %d triad: %d mode: %d defend: %d\n", faction, triad, mode, defend);

    int gov = base->gov_config();
    int psi_score = (gov & GOV_MAY_PROD_NATIVE ? plans[faction].psi_score
        + has_fac_built(FAC_BROOD_PIT, base_id)
        + has_fac_built(FAC_BIOLOGY_LAB, base_id)
        + has_fac_built(FAC_CENTAURI_PRESERVE, base_id)
        + has_fac_built(FAC_TEMPLE_OF_PLANET, base_id) : 0);
    bool prototypes = (gov & GOV_MAY_PROD_PROTOTYPE) || has_fac_built(FAC_SKUNKWORKS, base_id);
    bool combat = (mode == WMODE_COMBAT);
    bool pacifism = combat && triad == TRIAD_AIR
        && base_can_riot(base_id, true)
        && base->SE_police(SE_Pending) <= -3
        && base->drone_total + base->specialist_adjust >= base->talent_total;
    int best_id;
    int best_val;

    if (combat && triad == TRIAD_LAND) {
        best_id = BSC_SCOUT_PATROL;
        best_val = (mod_veh_avail(best_id, faction, base_id) ? 0 : -50)
            + unit_score(base, best_id, psi_score, defend);
    } else {
        best_id = -FAC_STOCKPILE_ENERGY;
        best_val = -10000;
    }
    for (int id = 0; id < MaxProtoNum; id++) {
        UNIT* u = &Units[id];
        if ((id < MaxProtoFactionNum || id / MaxProtoFactionNum == faction)
        && mod_veh_avail(id, faction, base_id) && u->triad() == triad && id != best_id) {
            if (!prototypes && id >= MaxProtoFactionNum && !u->is_prototyped()
            && prototype_factor(id) > 0) {
                continue;
            }
            if ((!combat && Weapon[u->weapon_id].mode != mode)
            || (combat && Weapon[u->weapon_id].offense_value == 0)
            || (combat && defend && u->chassis_id != CHS_INFANTRY)
            || (u->is_psi_unit() && psi_score <= 0)
            || u->is_planet_buster()) {
                continue;
            }
            if (combat && best_id >= 0 && best_id != BSC_SCOUT_PATROL
            && ((defend && proto_offense(id) > proto_defense(id))
            || (!defend && proto_offense(id) < proto_defense(id)))) {
                continue;
            }
            if (combat && triad == TRIAD_AIR) {
                bool intercept = has_abil(id, ABL_AIR_SUPERIORITY);
                if ((!intercept && pacifism)
                || (!intercept && !(gov & GOV_MAY_PROD_AIR_COMBAT))
                || (intercept && !(gov & GOV_MAY_PROD_AIR_DEFENSE))) {
                    continue;
                }
            }
            int val = unit_score(base, id, psi_score, defend);
            if (best_id < 0 || unit_is_better(best_id, id) || random(128) > 64 + best_val - val) {
                best_id = id;
                best_val = val;
            }
        }
    }
    return best_id;
}

int select_colony(int base_id, int pods, bool build_ships) {
    TileSearch ts;
    BASE* base = &Bases[base_id];
    bool start = Factions[base->faction_id].base_count < min(16, 2 + *MapAreaSqRoot/4);
    bool land = has_base_sites(ts, base->x, base->y, base->faction_id, TRIAD_LAND);
    bool sea = has_base_sites(ts, base->x, base->y, base->faction_id, TRIAD_SEA);
    int limit = (start || (!random(3) && (land || (build_ships && sea))) ? 2 : 1);

    if (pods >= limit) {
        return -1;
    }
    if (is_ocean(base)) {
        for (const auto& m : iterate_tiles(base->x, base->y, 1, 9)) {
            if (land && (!m.sq->is_owned() || (m.sq->owner == base->faction_id && !random(6)))
            && (m.sq->veh_owner() < 0 || m.sq->veh_owner() == base->faction_id)) {
                return find_proto(base_id, TRIAD_LAND, WMODE_COLONY, DEF);
            }
        }
        if (sea) {
            return find_proto(base_id, TRIAD_SEA, WMODE_COLONY, DEF);
        }
    } else {
        bool cheap = build_ships && (best_reactor(base->faction_id) >= REC_FUSION);
        if (build_ships && sea && (!land || !start || cheap)
        && random(16) > 10 + 2*(land + start - cheap)) {
            return find_proto(base_id, TRIAD_SEA, WMODE_COLONY, DEF);
        }
        if (land) {
            return find_proto(base_id, TRIAD_LAND, WMODE_COLONY, DEF);
        }
    }
    return -1;
}

int select_combat(int base_id, int num_probes, bool sea_base, bool build_ships) {
    BASE* base = &Bases[base_id];
    Faction* f = &Factions[base->faction_id];
    AIPlans* p = &plans[base->faction_id];
    int gov = base->gov_config();
    int choice;
    int w_air = 4*p->air_combat_units < f->base_count ? 2 : 5;
    int w_sea = (p->transport_units < 2 || 5*p->transport_units < f->base_count
        ? 2 : (3*p->transport_units < f->base_count ? 5 : 8));
    bool need_ships = 6*p->sea_combat_units < p->land_combat_units;
    bool reserve = base->mineral_surplus >= base->mineral_intake_2/2;
    bool probes = has_wmode(base->faction_id, WMODE_PROBE) && gov & GOV_MAY_PROD_PROBES;
    bool transports = has_wmode(base->faction_id, WMODE_TRANSPORT) && gov & GOV_MAY_PROD_TRANSPORT;
    bool land = gov & (GOV_MAY_PROD_LAND_COMBAT | GOV_MAY_PROD_LAND_DEFENSE);
    bool sea = gov & (GOV_MAY_PROD_NAVAL_COMBAT | GOV_MAY_PROD_TRANSPORT);
    bool air = gov & (GOV_MAY_PROD_AIR_COMBAT | GOV_MAY_PROD_AIR_DEFENSE);

    if (probes && (!(land || sea || air) || !random(num_probes*2 + 4) || !reserve)
    && (choice = find_proto(base_id, (build_ships ? TRIAD_SEA : TRIAD_LAND), WMODE_PROBE, DEF)) >= 0) {
        return choice;
    }
    if (air && (!(land || sea) || !random(w_air))
    && (choice = find_proto(base_id, TRIAD_AIR, WMODE_COMBAT, ATT)) >= 0) {
        return choice;
    }
    if (build_ships && sea) {
        int min_dist = INT_MAX;
        bool sea_enemy = false;

        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (base->faction_id != b->faction_id && !has_pact(base->faction_id, b->faction_id)) {
                int dist = map_range(base->x, base->y, b->x, b->y)
                    * (at_war(base->faction_id, b->faction_id) ? 1 : 4);
                if (dist < min_dist) {
                    sea_enemy = is_ocean(b);
                    min_dist = dist;
                }
            }
        }
        if (!land || (random(4) < (sea_base ? 3 : 1 + (need_ships || sea_enemy)))) {
            VehWeaponMode mode;
            if (!transports) {
                mode = WMODE_COMBAT;
            } else if (!(gov & GOV_MAY_PROD_NAVAL_COMBAT)) {
                mode = WMODE_TRANSPORT;
            } else {
                mode = (!random(w_sea) ? WMODE_TRANSPORT : WMODE_COMBAT);
            }
            if ((choice = find_proto(base_id, TRIAD_SEA, mode, ATT)) >= 0) {
                return choice;
            }
        }
    }
    return find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, (sea_base || !random(5) ? DEF : ATT));
}

static void push_item(BASE* base, score_max_queue_t& builds, int item_id, int score, int modifier) {
    if (item_id >= 0) {
        score -= 2*Units[item_id].cost;
    } else if (item_id >= -FAC_ORBITAL_DEFENSE_POD) {
        int turns = max(0, 10*Facility[-item_id].cost - base->minerals_accumulated)
            / max(2, base->mineral_surplus);
        score -= turns*turns/4;
        score -= 2*Facility[-item_id].cost;
        score -= 8*Facility[-item_id].maint;
    }
    if (modifier > 0) {
        score += 20*modifier;
    }
    builds.push({item_id, score});
    debug("push_item %3d %3d %s\n", item_id, score, prod_name(item_id));
}

int select_build(int base_id) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    AIPlans* p = &plans[faction];
    if (base->plr_owner()) {
        plans_upkeep(faction);
    }
    int gov = base->gov_config();
    int minerals = base->mineral_surplus + base->minerals_accumulated/10;
    int reserve = max(2, base->mineral_intake_2 / 2);
    int content_pop_value = (base->plr_owner() ? conf.content_pop_player[*DiffLevel]
        : conf.content_pop_computer[*DiffLevel]);
    int base_reg = region_at(base->x, base->y);
    int defend_range = (base->defend_range > 0 ?  base->defend_range : MaxEnemyRange/2);
    bool sea_base = base_reg >= MaxRegionLandNum;
    bool core_base = minerals >= p->project_limit;
    bool project_change = base->item_is_project()
        && !can_build(base_id, -base->item())
        && !(base->state_flags & BSTATE_PRODUCTION_DONE)
        && base->minerals_accumulated > Rules->retool_exemption;
    bool allow_units = can_build_unit(base_id, -1) && !project_change;
    bool allow_supply = !sea_base && gov & GOV_MAY_PROD_TERRAFORMERS;
    bool allow_ships = has_ships(faction)
        && adjacent_region(base->x, base->y, -1, *MapAreaSqRoot + 16, TRIAD_SEA);
    bool allow_pods = allow_expand(faction) && (base->pop_size > 1 || base->nutrient_surplus > 1);
    bool drone_riots = base->drone_riots() || base->drone_riots_active();
    int drones = base->drone_total + base->specialist_adjust;
    int all_crawlers = 0;
    int near_formers = 0;
    int need_ferry = 0;
    int transports = 0;
    int landprobes = 0;
    int seaprobes = 0;
    int defenders = 0;
    int artifacts = 0;
    int formers = 0;
    int scouts = 0;
    int pods = 0;

    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id != faction) {
            continue;
        }
        if (veh->home_base_id == base_id) {
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
            } else if (veh->is_transport()) {
                transports++;
            } else if (veh->is_supply() && veh->order != ORDER_CONVOY) {
                allow_supply = false;
            }
        }
        if (veh->is_combat_unit() && veh->triad() == TRIAD_LAND) {
            if (base->x == veh->x && base->y == veh->y) {
                defenders += 2;
            } else if (map_range(base->x, base->y, veh->x, veh->y) <= 1) {
                defenders += 1;
            }
            if (veh->home_base_id == base_id) {
                scouts++;
            }
        } else if (veh->is_former() && veh->home_base_id != base_id) {
            if (map_range(base->x, base->y, veh->x, veh->y) <= 1) {
                near_formers++;
            }
        } else if (veh->is_supply()) {
            all_crawlers++;
        } else if (veh->is_artifact() && map_range(base->x, base->y, veh->x, veh->y) <= 3) {
            artifacts++;
        }
        if (sea_base && veh->x == base->x && veh->y == base->y && veh->triad() == TRIAD_LAND) {
            if (veh->is_colony() || veh->is_former() || veh->is_supply()) {
                need_ferry++;
            }
        }
    }
    WItem Wgov;
    governor_priorities(Bases[base_id], Wgov);
    need_ferry = need_ferry && !transports
        && adjacent_region(base->x, base->y, faction, 16, TRIAD_LAND);
    allow_supply = allow_supply && all_crawlers < min(f->base_count, *MapAreaTiles/20);

    float Wbase = clamp(1.0f * minerals / p->project_limit, 0.4f, 1.0f)
        * (defend_range > 0 && defend_range < 8 ? 4 : 1)
        * max(0.05f, 2.0f * p->enemy_mil_factor / (p->enemy_base_range * 0.1f + 0.1f)
        + min(1.0f, 1.5f * f->base_count / *MapAreaSqRoot)
        + 0.8f * p->enemy_bases + 0.2f * Wgov.AI_fight);

    if (base->plr_owner()) {
        Wbase *= (gov & GOV_PRIORITY_CONQUER ? 4 : 1);
    } else {
        Wbase *= (base_reg != p->main_region && base_reg == p->target_land_region ? 4 : 1);
    }
    float Wthreat = 1.0f - (1.0f / (1.0f + Wbase));

    debug("select_build %3d %3d %3d %3d def: %d frm: %d prb: %d crw: %d pods: %d expand: %d "\
        "scouts: %d min: %2d res: %2d limit: %2d mil: %.4f threat: %.4f\n",
        *CurrentTurn, base_id, base->x, base->y,
        defenders, formers, landprobes+seaprobes, all_crawlers, pods, allow_pods,
        scouts, minerals, reserve, p->project_limit, p->enemy_mil_factor, Wthreat);

    const int SecretProject = -1;
    const int Satellites = -2;
    const int DefendUnit = -3; // Unit types start here
    const int CombatUnit = -4;
    const int ColonyUnit = -5;
    const int FormerUnit = -6;
    const int FerryUnit = -7;
    const int CrawlerUnit = -8;
    const int SeaProbeUnit = -9;

    const PItem build_order[] = { // E  D  B  C  W
        {DefendUnit,                 0, 0, 0, 0, 0},
        {FAC_PRESSURE_DOME,          4, 4, 4, 4, 0},
        {FAC_HEADQUARTERS,           4, 4, 4, 4, 0},
        {FAC_PUNISHMENT_SPHERE,      0, 0, 4, 4, 0},
        {FAC_RECREATION_COMMONS,     0, 4, 4, 0, 0},
        {CombatUnit,                 0, 0, 0, 4, 0},
        {Satellites,                 2, 2, 2, 2, 0},
        {FormerUnit,                 3, 0, 3, 0, 0},
        {FAC_RECYCLING_TANKS,        4, 4, 4, 0, 0},
        {SeaProbeUnit,               2, 0, 0, 3, 0},
        {CrawlerUnit,                3, 0, 3, 0, 0},
        {FerryUnit,                  2, 0, 0, 2, 0},
        {ColonyUnit,                 4, 1, 1, 0, 0},
        {SecretProject,              3, 3, 3, 3, 0},
        {FAC_CHILDREN_CRECHE,        2, 2, 2, 0, 0},
        {FAC_HAB_COMPLEX,            4, 4, 4, 0, 0},
        {FAC_NETWORK_NODE,           2, 4, 4, 0, 2},
        {FAC_HOLOGRAM_THEATRE,       2, 4, 4, 0, 1},
        {FAC_PERIMETER_DEFENSE,      2, 2, 2, 4, 0},
        {FAC_AEROSPACE_COMPLEX,      0, 0, 3, 3, 0},
        {FAC_TREE_FARM,              2, 2, 2, 0, 3},
        {FAC_GENEJACK_FACTORY,       0, 1, 3, 1, 0},
        {FAC_ROBOTIC_ASSEMBLY_PLANT, 0, 1, 3, 1, 0},
        {FAC_NANOREPLICATOR,         0, 1, 3, 1, 0},
        {FAC_QUANTUM_CONVERTER,      0, 1, 3, 1, 0},
        {FAC_HABITATION_DOME,        4, 4, 4, 0, 0},
        {FAC_TACHYON_FIELD,          0, 0, 3, 4, 0},
        {FAC_GEOSYNC_SURVEY_POD,     0, 0, 3, 4, 0},
        {FAC_FLECHETTE_DEFENSE_SYS,  0, 0, 3, 4, 0},
        {FAC_BIOENHANCEMENT_CENTER,  0, 0, 0, 3, 0},
        {FAC_COMMAND_CENTER,         0, 0, 0, 3, 0},
        {FAC_NAVAL_YARD,             0, 0, 0, 3, 0},
        {FAC_PSI_GATE,               0, 0, 3, 3, 0},
        {FAC_FUSION_LAB,             2, 4, 2, 0, 4},
        {FAC_QUANTUM_LAB,            2, 4, 2, 0, 4},
        {FAC_ENERGY_BANK,            0, 2, 2, 0, 2},
        {FAC_PARADISE_GARDEN,        0, 2, 4, 0, 0},
        {FAC_RESEARCH_HOSPITAL,      0, 4, 2, 0, 3},
        {FAC_NANOHOSPITAL,           0, 4, 2, 0, 3},
        {FAC_HYBRID_FOREST,          2, 2, 2, 0, 3},
        {FAC_BIOLOGY_LAB,            3, 2, 0, 0, 0},
        {FAC_CENTAURI_PRESERVE,      3, 0, 0, 0, 0},
        {FAC_COVERT_OPS_CENTER,      0, 0, 0, 3, 0},
        {FAC_EMPTY_FACILITY_42,      0, 2, 2, 0, 0},
        {FAC_EMPTY_FACILITY_43,      0, 2, 2, 0, 0},
        {FAC_EMPTY_FACILITY_44,      0, 2, 2, 0, 0},
        {FAC_EMPTY_FACILITY_45,      0, 2, 2, 0, 0},
    };
    score_max_queue_t builds;
    int Wenergy = has_fac_built(FAC_PUNISHMENT_SPHERE, base_id) ? 1 : 2;
    int Wt = 8;

    for (const auto& item : build_order) {
        const int t = item.item_id;
        int choice = 0;
        int score = random(32)
            + 4*(Wgov.AI_growth * item.explore + Wgov.AI_tech * item.discover
            + Wgov.AI_wealth * item.build + Wgov.AI_power * item.conquer);

        if (t >= 0 && !(gov & GOV_MAY_PROD_FACILITIES && can_build(base_id, t))) {
            continue;
        }
        if (t <= DefendUnit && !allow_units) {
            continue;
        }
        if (t == Satellites && gov & GOV_MAY_PROD_FACILITIES && minerals >= p->median_limit) {
            if ((choice = find_satellite(base_id)) != 0) {
                score += 2*min(40, f->base_count - 10);
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == SecretProject && core_base && gov & GOV_MAY_PROD_SP) {
            if ((choice = find_project(base_id, Wgov)) != 0) {
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == DefendUnit && gov & GOV_ALLOW_COMBAT && minerals > 1 + (defenders > 0)) {
            if (((gov & GOV_MAY_PROD_LAND_DEFENSE && defenders < 2)
            || (gov & GOV_MAY_PROD_EXPLORE_VEH && need_scouts(base_id, scouts)))
            && (choice = find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, DEF)) >= 0) {
                return choice;
            }
        }
        if (t == CombatUnit && gov & GOV_ALLOW_COMBAT && minerals > reserve) {
            if ((choice = select_combat(base_id, landprobes+seaprobes, sea_base, allow_ships)) >= 0) {
                if (random(256) < (int)(256 * Wthreat)) {
                    return choice;
                }
                score -= defend_range;
                push_item(base, builds, choice, score, 0);
                continue;
            }
        }
        if (t == FormerUnit && gov & GOV_MAY_PROD_TERRAFORMERS) {
            if (has_wmode(faction, WMODE_TERRAFORM)
            && formers + near_formers/2 < (base->pop_size < 4 ? 1 : 2)) {
                int num = 0;
                int sea = 0;
                for (const auto& m : iterate_tiles(base->x, base->y, 1, 21)) {
                    if (m.sq->owner == faction
                    && select_item(m.x, m.y, faction, FM_Auto_Full, m.sq) >= 0) {
                        num += (base->worked_tiles & (1 << m.i)
                            && !(m.sq->items & (BIT_SIMPLE|BIT_ADVANCED)) ? 2 : 1);
                        sea += is_ocean(m.sq);
                    }
                }
                if (num < 4) {
                    continue;
                }
                score += 8 * (num-6);
                if (has_chassis(faction, CHS_GRAVSHIP)
                && (choice = find_proto(base_id, TRIAD_AIR, WMODE_TERRAFORM, DEF)) >= 0
                && Units[choice].triad() == TRIAD_AIR) {
                    push_item(base, builds, choice, score, --Wt);
                }
                if ((sea*2 >= num || sea_base)
                && (choice = find_proto(base_id, TRIAD_SEA, WMODE_TERRAFORM, DEF)) >= 0
                && Units[choice].triad() == TRIAD_SEA) {
                    push_item(base, builds, choice, score, --Wt);
                    continue;
                }
                if (!sea_base
                && (choice = find_proto(base_id, TRIAD_LAND, WMODE_TERRAFORM, DEF)) >= 0) {
                    push_item(base, builds, choice, score, --Wt);
                    continue;
                }
            }
        }
        if (t == SeaProbeUnit && gov & GOV_MAY_PROD_PROBES) {
            if (allow_ships && has_wmode(faction, WMODE_PROBE)
            && p->unknown_factions > 1 && p->contacted_factions < 2
            && adjacent_region(base->x, base->y, -1, *MapAreaTiles/16, TRIAD_SEA)
            && (choice = find_proto(base_id, TRIAD_SEA, WMODE_PROBE, DEF)) >= 0) {
                score += 32*(p->unknown_factions - seaprobes) - 2*p->probe_units;
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == CrawlerUnit && allow_supply && has_wmode(faction, WMODE_SUPPLY)) {
            if ((choice = find_proto(base_id, TRIAD_LAND, WMODE_SUPPLY, DEF)) >= 0) {
                score += max(0, 40 - base->mineral_surplus - base->nutrient_surplus);
                score += 40*(all_crawlers < 4 + f->base_count/4);
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == FerryUnit && gov & GOV_MAY_PROD_TRANSPORT && allow_ships && need_ferry) {
            if ((choice = find_proto(base_id, TRIAD_SEA, WMODE_TRANSPORT, DEF)) >= 0) {
                score += (p->target_land_region > 0 || p->transport_units < 4 ? 40 : 0);
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == ColonyUnit && allow_pods && pods < 2 && gov & GOV_MAY_PROD_COLONY_POD) {
            if ((choice = select_colony(base_id, pods, allow_ships)) >= 0) {
                score += clamp(*MapAreaSqRoot*2 - f->base_count, 0, 80);
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t < 0) { // Skip facility evaluation
            continue;
        }
        if (item.energy > 0) {
            if (base->energy_surplus < 4) {
                continue;
            }
            score += Wenergy * item.energy * base->energy_surplus / 4;
            score -= 2*base->energy_inefficiency;
        }
        if (t == FAC_RECYCLING_TANKS) {
            score += 16*(ResInfo->recycling_tanks_energy
                + (1 + (base->nutrient_surplus < 4)) * ResInfo->recycling_tanks_nutrient
                + (1 + (base->mineral_surplus < 4)) * ResInfo->recycling_tanks_mineral);
        }
        if (t == FAC_CHILDREN_CRECHE) {
            score += 4*base->energy_inefficiency + 16*min(4, base_unused_space(base_id));
            score += (f->SE_growth_pending < -1 || f->SE_growth_pending == 4 ? 40 : 0);
            score += (f->SE_growth_pending > 5 || base->nutrient_surplus < 2
                || has_project(FAC_CLONING_VATS, faction) ? -40 : 0);
        }
        if (t == FAC_PUNISHMENT_SPHERE) {
            int turns = base->assimilation_turns_left;
            if (!(gov & GOV_MAY_FORCE_PSYCH) || (!drone_riots && !turns && drones < base->pop_size/2)) {
                continue;
            }
            if (clamp((turns - 5) / 10, 0, 3)
            + clamp((drones - base->talent_total) / 2, 0, 3)
            + (base->energy_surplus < 4 + 2*base->pop_size)
            + (base->energy_inefficiency > base->energy_surplus)
            + (base->energy_inefficiency > 2*base->energy_surplus)
            - 2*has_fac_built(FAC_RECREATION_COMMONS, base_id)
            - 2*has_fac_built(FAC_HOLOGRAM_THEATRE, base_id)
            - 2*has_fac_built(FAC_NETWORK_NODE, base_id) < 3) {
                continue;
            }
            score += 80*drone_riots + 16*drones + 4*turns;
            score -= 2*(f->SE_alloc_labs > 0 ? base->energy_surplus - base->energy_inefficiency : 0);
        }
        if (t == FAC_RECREATION_COMMONS || t == FAC_HOLOGRAM_THEATRE
        || t == FAC_RESEARCH_HOSPITAL || t == FAC_PARADISE_GARDEN) {
            if ((base->pop_size <= content_pop_value && !base->drone_total && !base->specialist_total)
            || (base->talent_total > 0 && !base->drone_total && !base->specialist_total)) {
                continue;
            }
            score += 80*drone_riots + max(16, 8*(5 - Facility[t].maint))*drones;
            score += 8*clamp(drones - base->talent_total, -4, 4);
        }
        if (t == FAC_NETWORK_NODE) {
            if (has_project(FAC_VIRTUAL_WORLD, faction) && base_can_riot(base_id, false)) {
                score += 80*drone_riots + 16*drones;
            }
            if (facility_count(FAC_NETWORK_NODE, faction) < f->base_count/8) {
                score += 40;
            }
            if (artifacts) {
                score += 40;
            }
        }
        if (t == FAC_TREE_FARM || t == FAC_HYBRID_FOREST) {
            if (!base->eco_damage) {
                score -= (sea_base || Wgov.AI_fight > 0 ? 8 : 4)*Facility[t].cost;
            }
            score += (Wgov.AI_fight > 0 || Wgov.AI_power > 1 ? 2 : 4)*min(40, base->eco_damage);
            score += (f->SE_alloc_psych > 0 ? 8*base->specialist_adjust : 0);
            score += ((t == FAC_TREE_FARM ? 16 : 4)
                + 8*(base->nutrient_surplus < 2) - 8*(base->nutrient_surplus > 8))
                * nearby_items(base->x, base->y, 1, 21, BIT_FOREST);
        }
        if (t == FAC_BIOLOGY_LAB) {
            score += 2*Wenergy*conf.biology_lab_bonus;
            score -= 2*base->energy_surplus;
        }
        if (t == FAC_BIOLOGY_LAB || t == FAC_CENTAURI_PRESERVE) {
            score += 8*min(4, p->psi_score);
            score -= 40*(!base->eco_damage + (Wgov.AI_fight > 0) + (Wgov.AI_power > 1));
        }
        if (t == FAC_GENEJACK_FACTORY || t == FAC_ROBOTIC_ASSEMBLY_PLANT
        || t == FAC_NANOREPLICATOR || t == FAC_QUANTUM_CONVERTER) {
            if (3*base->mineral_intake_2 > 2*(conf.clean_minerals + f->clean_minerals_modifier)) {
                score -= 100;
            }
            score += 2*max(-80, (core_base ? 80 : 60) - base->mineral_intake_2);
            score += 8*min(0, base->mineral_intake_2 - 16);
            score -= (Wgov.AI_fight > 0 || Wgov.AI_power > 1 ? 4 : 8)*base->eco_damage;
        }
        if ((t == FAC_COMMAND_CENTER && sea_base) || (t == FAC_NAVAL_YARD && !allow_ships)) {
            continue;
        }
        if (t == FAC_COMMAND_CENTER || t == FAC_NAVAL_YARD || t == FAC_BIOENHANCEMENT_CENTER) {
            if (minerals < reserve || (base->defend_goal < 3 && Facility[t].maint)) {
                continue;
            }
            score -= 4*(Facility[t].cost + Facility[t].maint);
            score -= defend_range;
        }
        if ((t == FAC_PERIMETER_DEFENSE && sea_base) || (t == FAC_NAVAL_YARD && !sea_base)) {
            score -= 40;
        }
        if (t == FAC_PERIMETER_DEFENSE || t == FAC_TACHYON_FIELD
        || t == FAC_GEOSYNC_SURVEY_POD || t == FAC_FLECHETTE_DEFENSE_SYS) {
            score += 4*(MaxEnemyRange - 4*Facility[t].maint - defend_range);
        }
        if (t == FAC_TACHYON_FIELD || t == FAC_GEOSYNC_SURVEY_POD || t == FAC_FLECHETTE_DEFENSE_SYS) {
            if (allow_units && base->defend_goal < 3 && defend_range > MaxEnemyRange/2) {
                continue;
            }
            score += 16*clamp(base->defend_goal-3, -2, 2);
        }
        assert(t > 0 && t <= SP_ID_Last);
        push_item(base, builds, -t, score, --Wt);
    }
    if (builds.size()) {
        return builds.top().item_id;
    }
    if (!allow_units || !(gov & GOV_ALLOW_COMBAT)) {
        return -FAC_STOCKPILE_ENERGY;
    }
    debug("BUILD OFFENSE\n");
    return select_combat(base_id, landprobes+seaprobes, sea_base, allow_ships);
}



