
#include "base.h"


int __cdecl mod_base_prod_choices(int base_id, int v1, int v2, int v3) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    BASE& base = Bases[base_id];
    int choice = 0;
    print_base(base_id);
    assert(base.defend_goal >= 0 && base.defend_goal <= 4);

    if (is_human(base.faction_id)) {
        debug("skipping human base\n");
        choice = base_prod_choices(base_id, v1, v2, v3);
    } else if (!thinker_enabled(base.faction_id)) {
        debug("skipping computer base\n");
        choice = base_prod_choices(base_id, v1, v2, v3);
    } else {
        set_base(base_id);
        base_compute(1);
        consider_staple(base_id);
        if (base.state_flags & BSTATE_PRODUCTION_DONE) {
            debug("BUILD NEW\n");
            choice = select_production(base_id);
            base.state_flags &= ~BSTATE_PRODUCTION_DONE;
        } else if (base.item_is_unit() && !can_build_unit(base.faction_id, base.item())) {
            debug("BUILD FACILITY\n");
            choice = select_production(base_id);
        } else if (!base.item_is_unit() && !can_build(base_id, abs(base.item()))) {
            debug("BUILD CHANGE\n");
            choice = select_production(base_id);
        } else if ((!base.item_is_unit() || !Units[base.item()].is_defend_unit())
        && !has_defenders(base.x, base.y, base.faction_id)) {
            debug("BUILD DEFENSE\n");
            choice = find_proto(base_id, TRIAD_LAND, COMBAT, DEF);
        } else {
            debug("BUILD OLD\n");
            choice = base.item();
        }
        debug("choice: %d %s\n", choice, prod_name(choice));
    }
    fflush(debug_log);
    return choice;
}

void __cdecl base_first(int base_id) {
    BASE& base = Bases[base_id];
    Faction& f = Factions[base.faction_id];
    base.queue_items[0] = find_proto(base_id, TRIAD_LAND, COMBAT, DEF);

    if (is_human(base.faction_id)) {
        int num = f.saved_queue_size[0];
        if (num > 0 && num < 10) {
            for (int i = 0; i < num; i++) {
                // Get only the lower 16 bits *with* sign extension
                base.queue_items[i] = (int16_t)f.saved_queue_items[0][i];
            }
            base.queue_size = num - 1;
        }
    }
}

int consider_staple(int base_id) {
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    if (b->nerve_staple_count < 3 && !un_charter() && base_can_riot(base_id, true) && f->SE_police >= 0
    && (b->drone_total > b->talent_total || b->state_flags & BSTATE_DRONE_RIOTS_ACTIVE)
    && b->pop_size / 4 >= b->nerve_staple_count
    && (b->faction_id == b->faction_id_former || want_revenge(b->faction_id, b->faction_id_former))) {
        action_staple(base_id);
    }
    return 0;
}

int need_psych(int base_id) {
    BASE* b = &Bases[base_id];
    if (!base_can_riot(base_id, false)) {
        return 0;
    }
    if (b->drone_total > b->talent_total || b->state_flags & BSTATE_DRONE_RIOTS_ACTIVE) {
        if (((b->faction_id_former != b->faction_id && b->assimilation_turns_left > 5)
        || (b->drone_total > 2 + b->talent_total && 2*b->energy_inefficiency > b->energy_surplus))
        && prod_turns(base_id, FAC_PUNISHMENT_SPHERE) < 12 && can_build(base_id, FAC_PUNISHMENT_SPHERE)) {
            return -FAC_PUNISHMENT_SPHERE;
        }
        if (can_build(base_id, FAC_RECREATION_COMMONS)) {
            return -FAC_RECREATION_COMMONS;
        }
        if (has_project(b->faction_id, FAC_VIRTUAL_WORLD) && can_build(base_id, FAC_NETWORK_NODE)) {
            return -FAC_NETWORK_NODE;
        }
        if (can_build(base_id, FAC_HOLOGRAM_THEATRE)) {
            return -FAC_HOLOGRAM_THEATRE;
        }
    }
    return 0;
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

int hurry_item(BASE* b, int mins, int cost) {
    Faction* f = &Factions[b->faction_id];
    f->energy_credits -= cost;
    b->minerals_accumulated += mins;
    b->state_flags |= BSTATE_HURRY_PRODUCTION;
    debug("hurry_item %d %d mins: %2d cost: %2d credits: %d %s %s\n",
        *current_turn, b->faction_id, mins, cost, f->energy_credits, b->name, prod_name(b->item()));
    return 1;
}

int consider_hurry() {
    const int base_id = *current_base_id;
    BASE* b = &Bases[base_id];
    const int t = b->item();
    Faction* f = &Factions[b->faction_id];
    AIPlans* p = &plans[b->faction_id];
    assert(b == *current_base_ptr);

    if (!thinker_enabled(b->faction_id)) {
        return base_hurry();
    }
    if (!conf.hurry_items || t == -FAC_STOCKPILE_ENERGY || b->item_is_project()
    || b->state_flags & (BSTATE_PRODUCTION_DONE | BSTATE_HURRY_PRODUCTION)) {
        return 0;
    }
    bool cheap = conf.simple_hurry_cost || b->minerals_accumulated >= Rules->retool_exemption;
    int mins = mineral_cost(base_id, t) - b->minerals_accumulated;
    int cost = hurry_cost(base_id, t, mins);
    int turns = prod_turns(base_id, t);
    int reserve = clamp(*current_turn * f->base_count / 5, 20, 800)
        * (has_facility(base_id, FAC_HEADQUARTERS) ? 1 : 2)
        * (b->defend_goal > 3 && p->enemy_factions > 0 ? 1 : 2) / 4;

    if (!cheap || mins < 1 || cost < 1 || f->energy_credits - cost < reserve) {
        return 0;
    }
    if (b->drone_total > b->talent_total && t < 0 && t == need_psych(base_id)) {
        return hurry_item(b, mins, cost);
    }
    if (t < 0 && turns > 1 && cost < f->energy_credits/8) {
        if (t == -FAC_RECYCLING_TANKS || t == -FAC_PRESSURE_DOME
        || t == -FAC_RECREATION_COMMONS || t == -FAC_TREE_FARM
        || t == -FAC_PUNISHMENT_SPHERE || t == -FAC_HEADQUARTERS)
            return hurry_item(b, mins, cost);
        if (t == -FAC_CHILDREN_CRECHE && unused_space(base_id) > 2)
            return hurry_item(b, mins, cost);
        if (t == -FAC_HAB_COMPLEX && unused_space(base_id) == 0)
            return hurry_item(b, mins, cost);
        if (t == -FAC_PERIMETER_DEFENSE && b->defend_goal > 2 && p->enemy_factions > 0)
            return hurry_item(b, mins, cost);
        if (t == -FAC_AEROSPACE_COMPLEX && has_tech(b->faction_id, TECH_Orbital))
            return hurry_item(b, mins, cost);
    }
    if (t >= 0 && turns > 1 && cost < f->energy_credits/4) {
        if (Units[t].is_combat_unit()) {
            int val = (b->defend_goal > 3)
                + (p->enemy_bases > 0)
                + (p->enemy_bases > 2 && b->defend_goal > 2)
                + (p->enemy_factions > 0)
                + (at_war(b->faction_id, MapWin->cOwner) > 0)
                + (cost < f->energy_credits/16)
                + 2*(!has_defenders(b->x, b->y, b->faction_id));
            if (val > 3) {
                return hurry_item(b, mins, cost);
            }
        }
        if (Units[t].is_former() && b->mineral_surplus < p->proj_limit/2
        && cost < f->energy_credits/16) {
            return hurry_item(b, mins, cost);
        }
        if (Units[t].is_colony() && b->state_flags & BSTATE_DRONE_RIOTS_ACTIVE
        && cost < f->energy_credits/16) {
            return hurry_item(b, mins, cost);
        }
    }
    return 0;
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
            if (Factions[i].base_count > 0) {
                n++;
            }
        }
        return n < 4;
    }
    if (proj == FAC_CITIZENS_DEFENSE_FORCE) {
        return Facility[FAC_PERIMETER_DEFENSE].maint == 0
            && facility_count(faction, FAC_PERIMETER_DEFENSE) > f->base_count/2 + 2;
    }
    if (proj == FAC_MARITIME_CONTROL_CENTER) {
        int n = 0;
        for (int i=0; i<*total_num_vehicles; i++) {
            VEH* veh = &Vehicles[i];
            if (veh->faction_id == faction && veh->triad() == TRIAD_SEA) {
                n++;
            }
        }
        return n < 8 && n < f->base_count/3;
    }
    if (proj == FAC_HUNTER_SEEKER_ALGO) {
        return f->SE_probe >= 3;
    }
    if (proj == FAC_LIVING_REFINERY) {
        return f->SE_support >= 3;
    }
    return false;
}

int find_project(int base_id) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    int projs = 0;
    int nukes = 0;
    int works = 0;
    int bases = f->base_count;
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
            if (!Units[best].is_prototyped() && Rules->extra_cost_prototype_air >= 50
            && can_build(base_id, FAC_SKUNKWORKS)) {
                if (works < 2) {
                    return -FAC_SKUNKWORKS;
                }
            } else {
                return best;
            }
        }
    }
    if (projs+nukes < 3 + nuke_limit && projs+nukes < bases/4) {
        if (can_build(base_id, FAC_SUBSPACE_GENERATOR)) {
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
            if (can_build(base_id, i) && prod_count(faction, -i, base_id) < similar_limit
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

bool need_police(int faction) {
    Faction* f = &Factions[faction];
    return f->SE_police > -2 && f->SE_police < 3 && !has_project(faction, FAC_TELEPATHIC_MATRIX);
}

int psi_score(int faction) {
    Faction* f = &Factions[faction];
    int psi = max(-4, 2 - f->best_weapon_value);
    if (has_project(faction, FAC_NEURAL_AMPLIFIER)) {
        psi += 2;
    }
    if (has_project(faction, FAC_DREAM_TWISTER)) {
        psi += 2;
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
    psi += min(2, max(-2, (f->enemy_best_weapon_value - f->best_weapon_value)/2));
    psi += MFactions[faction].rule_psi/10 + 2*f->SE_planet;
    return psi;
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
    const int specials[][2] = {
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
    if (u->ability_flags & ABL_POLICE_2X && need_police(faction)) {
        v += 20;
    }
    for (const int* s : specials) {
        if (u->ability_flags & s[0]) {
            v += 8 * s[1];
        }
    }
    int turns = max(0, u->cost * cfactor - accumulated) / max(2, minerals);
    int score = v - turns * (u->is_colony() ? 6 : 3)
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
    int best_id = basic;
    int cfactor = cost_factor(faction, 1, -1);
    int best_val = (Units[best_id].is_active() ? 0 : -40)
        + (Units[best_id].triad() == triad ? 0 : -40)
        + unit_score(best_id, faction, cfactor, b->mineral_surplus, b->minerals_accumulated, defend);

    for (int i=0; i < 2*MaxProtoFactionNum; i++) {
        int id = (i < MaxProtoFactionNum ? i : (faction-1)*MaxProtoFactionNum + i);
        UNIT* u = &Units[id];
        if (u->is_active() && strlen(u->name) > 0 && u->triad() == triad && id != best_id) {
            if (id < MaxProtoFactionNum && !has_tech(faction, u->preq_tech)) {
                continue;
            }
            if ((mode && Weapon[u->weapon_type].mode != mode)
            || (!mode && Weapon[u->weapon_type].offense_value == 0)
            || (!mode && defend && u->chassis_type != CHS_INFANTRY)
            || (u->weapon_type == WPN_PSI_ATTACK && plans[faction].psi_score < 1 && !is_human(faction))
            || (u->obsolete_factions & (1 << faction) && is_human(faction))
            || u->weapon_type == WPN_PLANET_BUSTER
            || !(mode != COMBAT || best_id == basic || (defend == (offense_value(u) < defense_value(u))))) {
                continue;
            }
            int val = unit_score(
                id, faction, cfactor, b->mineral_surplus, b->minerals_accumulated, defend);
            if (unit_is_better(&Units[best_id], u) || random(100) > 50 + best_val - val) {
                best_id = id;
                best_val = val;
                debug("===> %s\n", Units[best_id].name);
            }
        }
    }
    return best_id;
}

int select_colony(int base_id, int pods, bool build_ships) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    bool land = has_base_sites(base->x, base->y, faction, TRIAD_LAND);
    bool sea = has_base_sites(base->x, base->y, faction, TRIAD_SEA);
    int limit = (*current_turn < 60 || (!random(3) && (land || (build_ships && sea))) ? 2 : 1);

    if (pods >= limit) {
        return TRIAD_NONE;
    }
    if (is_ocean(base)) {
        for (auto& m : iterate_tiles(base->x, base->y, 1, 9)) {
            if (land && (!m.sq->is_owned() || (m.sq->owner == faction && !random(6)))
            && (unit_in_tile(m.sq) < 0 || unit_in_tile(m.sq) == faction)) {
                return TRIAD_LAND;
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
    return TRIAD_NONE;
}

int select_combat(int base_id, int num_probes, bool sea_base, bool build_ships) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    int w1 = 4*plans[faction].air_combat_units < f->base_count ? 2 : 5;
    int w2 = 4*plans[faction].transport_units < f->base_count ? 2 : 5;
    bool reserve = base->mineral_surplus >= base->mineral_intake_2/2;
    bool probes = has_weapon(faction, WPN_PROBE_TEAM);
    bool aircraft = has_aircraft(faction);
    bool transports = has_weapon(faction, WPN_TROOP_TRANSPORT);

    if (probes && (!random(num_probes*2 + 4) || !reserve)) {
        return find_proto(base_id, (build_ships ? TRIAD_SEA : TRIAD_LAND), WMODE_INFOWAR, DEF);
    }
    if (aircraft && (f->SE_police >= -3 || !base_can_riot(base_id, true)) && !random(w1)) {
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
            int mode = (transports && !random(w2) ? WMODE_TRANSPORT : COMBAT);
            return find_proto(base_id, TRIAD_SEA, mode, ATT);
        }
    }
    return find_proto(base_id, TRIAD_LAND, COMBAT, (sea_base || !random(5) ? DEF : ATT));
}

int select_production(int base_id) {
    const int id = base_id;
    BASE* base = &Bases[id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    AIPlans* p = &plans[faction];

    int minerals = base->mineral_surplus + base->minerals_accumulated/10;
    int cfactor = cost_factor(faction, 1, -1);
    int reserve = max(2, base->mineral_intake_2 / 2);
    bool sea_base = is_ocean(base);
    bool core_base = minerals >= plans[faction].proj_limit;
    bool project_change = base->item_is_project()
        && !can_build(id, -base->item())
        && ~base->state_flags & BSTATE_PRODUCTION_DONE
        && base->minerals_accumulated > Rules->retool_exemption;
    bool allow_units = can_build_unit(faction, -1) && !project_change;
    bool allow_supply = !sea_base;
    int need_ferry = 0;
    int transports = 0;
    int landprobes = 0;
    int seaprobes = 0;
    int defenders = 0;
    int crawlers = 0;
    int formers = 0;
    int scouts = 0;
    int pods = 0;

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
            } else if (veh->is_transport()) {
                transports++;
            } else if (veh->is_supply() && veh->move_status != ORDER_CONVOY) {
                allow_supply = false;
            }
        }
        if (veh->is_combat_unit() && veh->triad() == TRIAD_LAND) {
            if (map_range(base->x, base->y, veh->x, veh->y) <= 1) {
                defenders++;
            }
            if (veh->home_base_id == id) {
                scouts++;
            }
        } else if (veh->is_supply()) {
            crawlers++;
        }
        if (sea_base && veh->x == base->x && veh->y == base->y && veh->triad() == TRIAD_LAND) {
            if (veh->is_colony() || veh->is_former() || veh->is_supply()) {
                need_ferry++;
            }
        }
    }
    bool build_ships = can_build_ships(id);
    bool build_pods = allow_expand(faction) && pods < 2
        && (base->pop_size > 1 || base->nutrient_surplus > 1);
    double w1 = min(1.0, max(0.4, 1.0 * minerals / p->proj_limit))
        * (conf.conquer_priority / 100.0)
        * (region_at(base->x, base->y) == p->target_land_region
        && p->main_region != p->target_land_region ? 4 : 1);
    double w2 = 2.0 * p->enemy_mil_factor / (p->enemy_base_range * 0.1 + 0.1)
        + 0.8 * p->enemy_bases + min(0.4, *current_turn/400.0)
        + min(1.0, 1.5 * f->base_count / *map_area_sq_root) * (f->AI_fight * 0.2 + 0.8);
    double threat = 1 - (1 / (1 + max(0.0, w1 * w2)));

    debug("select_prod %d %d %2d %2d def: %d frm: %d prb: %d crw: %d pods: %d expand: %d "\
        "scouts: %d min: %2d res: %2d limit: %2d mil: %.4f threat: %.4f\n",
        *current_turn, faction, base->x, base->y,
        defenders, formers, landprobes+seaprobes, crawlers, pods, build_pods,
        scouts, minerals, reserve, p->proj_limit, p->enemy_mil_factor, threat);

    const int SecretProject = -1;
    const int MorePsych = -2;
    const int DefendUnit = -3;
    const int CombatUnit = -4;
    const int ColonyUnit = -5;
    const int FormerUnit = -6;
    const int FerryUnit = -7;
    const int CrawlerUnit = -8;
    const int SeaProbeUnit = -9;

    const int F_Mineral = 1;
    const int F_Energy = 2;
    const int F_Repair = 4;
    const int F_Combat = 8;
    const int F_Trees = 16;
    const int F_Space = 32;
    const int F_Surplus = 64;

    const int build_order[][2] = {
        {DefendUnit,                0},
        {MorePsych,                 0},
        {FAC_PRESSURE_DOME,         0},
        {FAC_HEADQUARTERS,          0},
        {CombatUnit,                0},
        {FormerUnit,                0},
        {SeaProbeUnit,              0},
        {CrawlerUnit,               0},
        {FAC_RECYCLING_TANKS,       0},
        {FerryUnit,                 0},
        {ColonyUnit,                0},
        {FAC_RECREATION_COMMONS,    0},
        {FAC_TREE_FARM,             F_Energy|F_Trees},
        {SecretProject,             0},
        {FAC_CHILDREN_CRECHE,       0},
        {FAC_HAB_COMPLEX,           0},
        {FAC_HOLOGRAM_THEATRE,      0},
        {FAC_ORBITAL_DEFENSE_POD,   F_Space},
        {FAC_NESSUS_MINING_STATION, F_Space},
        {FAC_ORBITAL_POWER_TRANS,   F_Space},
        {FAC_SKY_HYDRO_LAB,         F_Space},
        {FAC_PERIMETER_DEFENSE,     F_Combat},
        {FAC_GENEJACK_FACTORY,      F_Mineral},
        {FAC_ROBOTIC_ASSEMBLY_PLANT,F_Mineral},
        {FAC_NANOREPLICATOR,        F_Mineral},
        {FAC_AEROSPACE_COMPLEX,     0},
        {FAC_NETWORK_NODE,          F_Energy},
        {FAC_COMMAND_CENTER,        F_Repair|F_Combat},
        {FAC_NAVAL_YARD,            F_Repair|F_Combat},
        {FAC_HABITATION_DOME,       0},
        {FAC_GEOSYNC_SURVEY_POD,    F_Surplus},
        {FAC_PSI_GATE,              F_Surplus},
        {FAC_FUSION_LAB,            F_Energy},
        {FAC_ENERGY_BANK,           F_Energy},
        {FAC_RESEARCH_HOSPITAL,     F_Energy},
        {FAC_TACHYON_FIELD,         F_Surplus|F_Combat},
        {FAC_FLECHETTE_DEFENSE_SYS, F_Surplus|F_Combat},
        {FAC_QUANTUM_LAB,           F_Energy|F_Surplus},
        {FAC_NANOHOSPITAL,          F_Energy|F_Surplus},
        {FAC_HYBRID_FOREST,         F_Energy|F_Trees|F_Surplus},
    };

    for (const int* item : build_order) {
        const CFacility& facility = Facility[item[0]];
        const int t = item[0];
        const int flag = item[1];
        int choice = 0;

        if (t == SecretProject && core_base && (choice = find_project(id)) != 0) {
            return choice;
        }
        if (t == MorePsych && (choice = need_psych(id)) != 0) {
            return choice;
        }
        if (t < 0 && !allow_units) {
            continue;
        }
        if (t == DefendUnit) {
            if (minerals > 2 && (defenders < 1 || need_scouts(base->x, base->y, faction, scouts))) {
                return find_proto(id, TRIAD_LAND, COMBAT, DEF);
            }
        }
        if (t == CombatUnit) {
            if (minerals > reserve && random(100) < (int)(100 * threat)) {
                return select_combat(id, landprobes+seaprobes, sea_base, build_ships);
            }
        }
        if (t == SeaProbeUnit) {
            if (build_ships && has_weapon(faction, WPN_PROBE_TEAM)
            && ocean_coast_tiles(base->x, base->y) && !random(seaprobes > 0 ? 6 : 3)
            && p->unknown_factions > 1 && p->contacted_factions < 2) {
                return find_proto(id, TRIAD_SEA, WMODE_INFOWAR, DEF);
            }
        }
        if (t == FormerUnit) {
            if (has_weapon(faction, WPN_TERRAFORMING_UNIT)
            && formers < (base->pop_size < (sea_base ? 4 : 3) ? 1 : 2)) {
                int num = 0;
                int sea = 0;
                for (auto& mp : iterate_tiles(base->x, base->y, 1, 21)) {
                    if (mp.sq->owner == faction && select_item(mp.x, mp.y, faction, mp.sq) >= 0) {
                        num++;
                        sea += is_ocean(mp.sq);
                    }
                }
                if (num > 3) {
                    if (has_chassis(faction, CHS_GRAVSHIP) && minerals >= Chassis[CHS_GRAVSHIP].cost) {
                        int unit = find_proto(id, TRIAD_AIR, WMODE_TERRAFORMER, DEF);
                        if (unit >= 0 && Units[unit].triad() == TRIAD_AIR) {
                            return unit;
                        }
                    }
                    if ((sea*2 >= num || sea_base) && has_ships(faction)) {
                        return find_proto(id, TRIAD_SEA, WMODE_TERRAFORMER, DEF);
                    }
                    if (!sea_base) {
                        return find_proto(id, TRIAD_LAND, WMODE_TERRAFORMER, DEF);
                    }
                }
            }
        }
        if (t == FerryUnit) {
            if (build_ships && !transports && need_ferry) {
                for (auto& mp : iterate_tiles(base->x, base->y, 1, 9)) {
                    if (!is_ocean(mp.sq) && (!mp.sq->is_owned() || mp.sq->owner == faction)) {
                        return find_proto(id, TRIAD_SEA, WMODE_TRANSPORT, DEF);
                    }
                }
            }
        }
        if (t == ColonyUnit) {
            if (build_pods) {
                int type = select_colony(id, pods, build_ships);
                if (type == TRIAD_LAND || type == TRIAD_SEA) {
                    return find_proto(id, type, WMODE_COLONIST, DEF);
                }
            }
        }
        if (t == CrawlerUnit) {
            if (allow_supply && has_weapon(faction, WPN_SUPPLY_TRANSPORT)
            && 100 * crawlers < conf.crawler_priority * f->base_count
            && Weapon[WPN_SUPPLY_TRANSPORT].cost < 4*minerals && !random(2)) {
                return find_proto(id, TRIAD_LAND, WMODE_CONVOY, DEF);
            }
        }
        if (t < 0 || !can_build(id, t)) { // Skip facility checks
            continue;
        }
        int turns = max(0, facility.cost * min(12, cfactor) - base->minerals_accumulated)
            / max(2, base->mineral_surplus);
        /* Check if we have sufficient base energy for multiplier facilities. */
        if (flag & F_Energy && base->energy_surplus < 2*facility.maint + facility.cost)
            continue;
        if (flag & F_Energy && turns > 5 + f->AI_tech + f->AI_wealth - f->AI_power - f->AI_fight)
            continue;
        /* Avoid building combat-related facilities in peacetime. */
        if (flag & F_Combat && p->enemy_base_range > MaxEnemyRange - 5*min(5, facility.maint))
            continue;
        if (flag & F_Repair && (!conf.repair_base_facility || f->SE_morale < 0))
            continue;
        if (flag & F_Surplus && turns > (allow_units ? 5 : 8))
            continue;
        if (flag & F_Mineral && (base->mineral_intake_2 > (core_base ? 80 : 50)
        || 3*base->mineral_intake_2 > 2*(conf.clean_minerals + f->clean_minerals_modifier)
        || turns > 5 + f->AI_power + f->AI_wealth))
            continue;
        if (flag & F_Space && (!core_base || !has_facility(id, FAC_AEROSPACE_COMPLEX)))
            continue;
        if (flag & F_Trees && (base->eco_damage < random(16)
        || (!base->eco_damage && nearby_items(base->x, base->y, 1, BIT_FOREST) < 4)))
            continue;
        if (t == FAC_RECYCLING_TANKS && base->drone_total > 0 && can_build(id, FAC_RECREATION_COMMONS))
            continue;
        if (t == FAC_PRESSURE_DOME && (*climate_future_change <= 0 || !is_shore_level(mapsq(base->x, base->y))))
            continue;
        if (t == FAC_HEADQUARTERS && (conf.auto_relocate_hq || !relocate_hq(id)))
            continue;
        if (t == FAC_COMMAND_CENTER && (sea_base || turns > 5))
            continue;
        if (t == FAC_NAVAL_YARD && (!sea_base || turns > 5))
            continue;
        if (t == FAC_HAB_COMPLEX && unused_space(id) > 0)
            continue;
        if (t == FAC_HABITATION_DOME && unused_space(id) > 0)
            continue;
        if ((t == FAC_RECREATION_COMMONS || t == FAC_HOLOGRAM_THEATRE)
        && base->pop_size < 8 && base->drone_total + base->specialist_total < 2)
            continue;
        if (t == FAC_PSI_GATE && facility_count(faction, FAC_PSI_GATE) > f->base_count / 3)
            continue;
        if (t > 0) {
            return -t;
        }
    }
    if (!allow_units) {
        return -FAC_STOCKPILE_ENERGY;
    }
    debug("BUILD OFFENSE\n");
    return select_combat(id, landprobes+seaprobes, sea_base, build_ships);
}






