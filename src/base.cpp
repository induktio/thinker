
#include "base.h"


bool governor_enabled(int base_id) {
    return conf.manage_player_bases && Bases[base_id].governor_flags & GOV_MANAGES_PRODUCTION;
}

void __cdecl mod_base_reset(int base_id, bool has_gov) {
    BASE& base = Bases[base_id];
    assert(base_id >= 0 && base_id < *total_num_bases);
    assert(base.defend_goal >= 0 && base.defend_goal <= 5);
    print_base(base_id);
    if (is_human(base.faction_id) && !governor_enabled(base_id)) {
        debug("skipping human base\n");
        base_reset(base_id, has_gov);
    } else if (!is_human(base.faction_id) && !thinker_enabled(base.faction_id)) {
        debug("skipping computer base\n");
        base_reset(base_id, has_gov);
    } else {
        int choice = mod_base_build(base_id, has_gov);
        base_change(base_id, choice);
    }
}

/*
Performs nearly the same thing as vanilla base_build but the last 3 parameters
have been replaced with the governor force recalculate flag.
*/
int __cdecl mod_base_build(int base_id, bool has_gov) {
    BASE& base = Bases[base_id];
    int choice = 0;
    set_base(base_id);
    base_compute(1);

    if (is_human(base.faction_id)) {
        // Unimplemented settings
        base.governor_flags &= ~(GOV_MAY_PROD_SP|GOV_MAY_HURRY_PRODUCTION
            |GOV_PRIORITY_BUILD|GOV_PRIORITY_CONQUER|GOV_PRIORITY_DISCOVER|GOV_PRIORITY_EXPLORE);
        base.governor_flags |= GOV_MULTI_PRIORITIES;
    } else {
        consider_staple(base_id);
    }
    if (base.state_flags & BSTATE_PRODUCTION_DONE || has_gov) {
        debug("BUILD NEW\n");
        choice = select_build(base_id);
        base.state_flags &= ~BSTATE_PRODUCTION_DONE;
    } else if (base.item() >= 0 && !can_build_unit(base_id, base.item())) {
        debug("BUILD FACILITY\n");
        choice = select_build(base_id);
    } else if (base.item() < 0 && !can_build(base_id, abs(base.item()))) {
        debug("BUILD CHANGE\n");
        choice = select_build(base_id);
    } else if ((base.item() < 0 || !Units[base.item()].is_garrison_unit())
    && !has_defenders(base.x, base.y, base.faction_id)) {
        debug("BUILD DEFENSE\n");
        choice = find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, DEF);
    } else {
        debug("BUILD OLD\n");
        choice = base.item();
    }
    debug("choice: %d %s\n", choice, prod_name(choice));
    flushlog();
    return choice;
}

void __cdecl base_first(int base_id) {
    BASE& base = Bases[base_id];
    Faction& f = Factions[base.faction_id];
    base.queue_items[0] = find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, DEF);

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

void __cdecl set_base(int base_id) {
    *current_base_id = base_id;
    *current_base_ptr = &Bases[base_id];
}

void __cdecl base_compute(bool update_prev) {
    if (update_prev || *compute_base_id != *current_base_id) {
        *compute_base_id = *current_base_id;
        base_support();
        base_yield();
        base_nutrient();
        base_minerals();
        base_energy();
    }
}

bool valid_relocate_base(int base_id) {
    int faction = Bases[base_id].faction_id;
    int new_id = -1;

    if (!has_tech(Facility[FAC_HEADQUARTERS].preq_tech, faction)) {
        return false;
    }
    for (int i = 0; i < *total_num_bases; i++) {
        BASE* b = &Bases[i];
        if (b->faction_id == faction) {
            if ((i != base_id && b->item() == -FAC_HEADQUARTERS)
            || has_fac_built(FAC_HEADQUARTERS, i)) {
                return false;
            }
            if (new_id < 0 && b->mineral_surplus > 3) {
                new_id = i;
            }
        }
    }
    return new_id == base_id;
}

void find_relocate_base(int faction) {
    if (find_hq(faction) < 0) {
        int best_score = INT_MIN;
        int best_id = -1;
        Points bases;
        for (int i = 0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction) {
                bases.insert({b->x, b->y});
            }
        }
        for (int i = 0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction) {
                int score = 4 * b->pop_size - (int)(10 * avg_range(bases, b->x, b->y))
                    - clamp(2 * b->assimilation_turns_left, 0, 80);
                debug("relocate_base %s %4d %s\n",
                    MFactions[faction].filename, score, b->name);
                if (score > best_score) {
                    best_id = i;
                    best_score = score;
                }
            }
        }
        if (best_id >= 0) {
            BASE* b = &Bases[best_id];
            set_fac(FAC_HEADQUARTERS, best_id, true);
            draw_tile(b->x, b->y, 0);
        }
    }
}

int __cdecl mod_base_kill(int base_id) {
    int old_faction = Bases[base_id].faction_id;
    assert(base_id >= 0 && base_id < *total_num_bases);
    base_kill(base_id);
    find_relocate_base(old_faction);
    return 0;
}

int __cdecl mod_capture_base(int base_id, int faction, int is_probe) {
    BASE* base = &Bases[base_id];
    int old_faction = base->faction_id;
    int prev_owner = base->faction_id;
    int last_spoke = *current_turn - Factions[faction].diplo_spoke[old_faction];
    bool vendetta = at_war(faction, old_faction);
    bool alien_fight = is_alien(faction) != is_alien(old_faction);
    bool destroy_base = base->pop_size < 2;
    assert(base_id >= 0 && base_id < *total_num_bases);
    assert(valid_player(faction) && faction != old_faction);

    if (is_probe) {
        MFactions[old_faction].thinker_last_mc_turn = *current_turn;
    }
    if (!destroy_base && alien_fight) {
        base->pop_size = max(2, base->pop_size / 2);
    }
    if (!destroy_base && base->faction_id_former >= 0
    && is_alive(base->faction_id_former)
    && base->assimilation_turns_left > 10) {
        prev_owner = base->faction_id_former;
    }
    base->defend_goal = 0;
    capture_base(base_id, faction, is_probe);
    find_relocate_base(old_faction);
    /*
    Modify captured base extra drone effect to take into account the base size.
    */
    if (!destroy_base) {
        int num = 0;
        for (int i = Fac_ID_First; i <= Fac_ID_Last; i++) {
            if (has_fac_built((FacilityId)i, base_id)) {
                num++;
            }
        }
        base->assimilation_turns_left = clamp((num + base->pop_size) * 5 + 15, 20, 50);
        base->faction_id_former = prev_owner;
    }
    /*
    Prevent AIs from initiating diplomacy once every turn after losing a base.
    Allow dialog if surrender is possible given the diplomacy check values.
    */
    if (vendetta && is_human(faction) && !is_human(old_faction)
    && last_spoke < 10 && !*diplo_value_93FA98 && !*diplo_value_93FA24) {
        int lost = 0;
        for (int i = 0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && b->faction_id_former == old_faction) {
                lost++;
            }
        }
        int div = max(2, 6 - last_spoke/2) + max(0, 4 - lost/2)
            + (want_revenge(old_faction, faction) ? 4 : 0);
        if (random(div) > 0) {
            set_treaty(faction, old_faction, DIPLO_WANT_TO_TALK, 0);
            set_treaty(old_faction, faction, DIPLO_WANT_TO_TALK, 0);
        }
    }
    debug("capture_base %3d %3d old_owner: %d new_owner: %d last_spoke: %d v1: %d v2: %d\n",
        *current_turn, base_id, old_faction, faction, last_spoke, *diplo_value_93FA98, *diplo_value_93FA24);
    return 0;
}

char* prod_name(int item_id) {
    if (item_id >= 0) {
        return Units[item_id].name;
    } else {
        return Facility[-item_id].name;
    }
}

int prod_turns(int base_id, int item_id) {
    BASE* b = &Bases[base_id];
    assert(base_id >= 0 && base_id < *total_num_bases);
    if (item_id >= 0) {
        assert(strlen(Units[item_id].name) > 0);
    } else {
        assert(item_id >= -SP_ID_Last);
    }
    int minerals = max(0, mineral_cost(base_id, item_id) - b->minerals_accumulated);
    int surplus = max(1, 10 * b->mineral_surplus);
    return 10 * minerals / surplus + ((10 * minerals) % surplus != 0);
}

int mineral_cost(int base_id, int item_id) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    // Take possible prototype costs into account in veh_cost
    if (item_id >= 0) {
        return mod_veh_cost(item_id, base_id, 0) * cost_factor(Bases[base_id].faction_id, 1, -1);
    } else {
        return Facility[-item_id].cost * cost_factor(Bases[base_id].faction_id, 1, -1);
    }
}

int hurry_cost(int base_id, int item_id, int hurry_mins) {
    BASE* b = &Bases[base_id];
    MFaction* m = &MFactions[b->faction_id];
    assert(base_id >= 0 && base_id < *total_num_bases);

    bool cheap = conf.simple_hurry_cost || b->minerals_accumulated >= Rules->retool_exemption;
    int project_factor = (item_id > -SP_ID_First ? 1 :
        2 * (b->minerals_accumulated < 4*cost_factor(b->faction_id, 1, -1) ? 2 : 1));
    int mins = max(0, mineral_cost(base_id, item_id) - b->minerals_accumulated);
    int cost = (item_id < 0 ? 2*mins : mins*mins/20 + 2*mins)
        * project_factor
        * (cheap ? 1 : 2)
        * (has_project(FAC_VOICE_OF_PLANET, -1) ? 2 : 1)
        * m->rule_hurry / 100;
    if (hurry_mins > 0 && mins > 0) {
        return hurry_mins * cost / mins + (((hurry_mins * cost) % mins) != 0);
    }
    return 0;
}

int unused_space(int base_id) {
    BASE* base = &Bases[base_id];
    int limit_mod = (has_project(FAC_ASCETIC_VIRTUES, base->faction_id) ? 2 : 0)
        - MFactions[base->faction_id].rule_population;

    if (!has_facility(FAC_HAB_COMPLEX, base_id)) {
        return max(0, Rules->pop_limit_wo_hab_complex + limit_mod - base->pop_size);
    }
    if (!has_facility(FAC_HABITATION_DOME, base_id)) {
        return max(0, Rules->pop_limit_wo_hab_dome + limit_mod - base->pop_size);
    }
    return max(0, 24 - base->pop_size);
}

bool can_build(int base_id, int fac_id) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    assert(fac_id > 0 && fac_id <= FAC_EMPTY_SP_64);
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];

    if (fac_id >= SP_ID_First && fac_id <= FAC_EMPTY_SP_64) {
        if (SecretProjects[fac_id-SP_ID_First] != SP_Unbuilt
        || *game_rules & RULES_SCN_NO_BUILDING_SP) {
            return false;
        }
    }
    if (fac_id == FAC_ASCENT_TO_TRANSCENDENCE || fac_id == FAC_VOICE_OF_PLANET) {
        if (is_alien(faction) || ~*game_rules & RULES_VICTORY_TRANSCENDENCE) {
            return false;
        }
    }
    if (fac_id == FAC_ASCENT_TO_TRANSCENDENCE) {
        return has_project(FAC_VOICE_OF_PLANET, -1)
            && !has_project(FAC_ASCENT_TO_TRANSCENDENCE, -1);
    }
    if (fac_id == FAC_HEADQUARTERS && find_hq(faction) >= 0) {
        return false;
    }
    if (fac_id == FAC_RECYCLING_TANKS && has_facility(FAC_PRESSURE_DOME, base_id)) {
        return false;
    }
    if (fac_id == FAC_HOLOGRAM_THEATRE && (has_project(FAC_VIRTUAL_WORLD, faction)
    || !has_facility(FAC_RECREATION_COMMONS, base_id))) {
        return false;
    }
    if ((fac_id == FAC_RECREATION_COMMONS || fac_id == FAC_HOLOGRAM_THEATRE
    || fac_id == FAC_PUNISHMENT_SPHERE || fac_id == FAC_PARADISE_GARDEN)
    && !base_can_riot(base_id, false)) {
        return false;
    }
    if (fac_id == FAC_GENEJACK_FACTORY && base_can_riot(base_id, false)
    && base->talent_total + 1 < base->drone_total + Rules->drones_induced_genejack_factory) {
        return false;
    }
    if ((fac_id == FAC_HAB_COMPLEX || fac_id == FAC_HABITATION_DOME) && base->nutrient_surplus < 2) {
        return false;
    }
    if (fac_id == FAC_SUBSPACE_GENERATOR) {
        if (!is_alien(faction) || base->pop_size < Rules->base_size_subspace_gen) {
            return false;
        }
        int n = 0;
        for (int i=0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && has_facility(FAC_SUBSPACE_GENERATOR, i)
            && b->pop_size >= Rules->base_size_subspace_gen
            && ++n >= Rules->subspace_gen_req) {
                return false;
            }
        }
    }
    if (fac_id >= FAC_SKY_HYDRO_LAB && fac_id <= FAC_ORBITAL_DEFENSE_POD) {
        if (!has_facility(FAC_AEROSPACE_COMPLEX, base_id)
        && !has_project(FAC_SPACE_ELEVATOR, faction)) {
            return false;
        }
        int prod_num = prod_count(-fac_id, faction, base_id);
        int goal_num = satellite_goal(faction, fac_id);
        int built_num = satellite_count(faction, fac_id);
        if (built_num + prod_num >= goal_num) {
            return false;
        }
    }
    if (fac_id == FAC_FLECHETTE_DEFENSE_SYS && f->satellites_ODP > 0) {
        return false;
    }
    if (fac_id == FAC_GEOSYNC_SURVEY_POD || fac_id == FAC_FLECHETTE_DEFENSE_SYS) {
        for (int i=0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && i != base_id
            && map_range(base->x, base->y, b->x, b->y) <= 3
            && (has_facility(FAC_GEOSYNC_SURVEY_POD, i)
            || has_facility(FAC_FLECHETTE_DEFENSE_SYS, i)
            || b->item() == -FAC_GEOSYNC_SURVEY_POD
            || b->item() == -FAC_FLECHETTE_DEFENSE_SYS)) {
                return false;
            }
        }
    }
    /* Rare special case if the game engine reaches the global unit limit. */
    if (fac_id == FAC_STOCKPILE_ENERGY) {
        return (*current_turn + base_id) % 4 > 0 || !can_build_unit(base_id, -1);
    }
    return has_tech(Facility[fac_id].preq_tech, faction) && !has_facility((FacilityId)fac_id, base_id);
}

bool can_build_unit(int base_id, int unit_id) {
    assert(base_id >= 0 && base_id < *total_num_bases && unit_id >= -1);
    UNIT* u = &Units[unit_id];
    BASE* b = &Bases[base_id];
    if (unit_id >= 0) {
        if (unit_id < MaxProtoFactionNum && !has_tech(u->preq_tech, b->faction_id)) {
            return false;
        }
        if (u->triad() == TRIAD_SEA && !nearby_tiles(b->x, b->y, TS_TRIAD_SEA, 10)) {
            return false;
        }
    }
    return *total_num_vehicles < MaxVehNum * 15 / 16;
}

bool can_build_ships(int base_id) {
    BASE* b = &Bases[base_id];
    int k = *map_area_sq_root + 20;
    return has_ships(b->faction_id) && nearby_tiles(b->x, b->y, TS_TRIAD_SEA, k);
}

bool can_staple(int base_id) {
    int faction_id = Bases[base_id].faction_id;
    return base_id >= 0 && conf.nerve_staple > is_human(faction_id)
        && Factions[faction_id].SE_police >= 0;
}

/*
Check if the base might riot on the next turn, usually after a population increase.
Used only for rendering additional details about base psych status.
*/
bool maybe_riot(int base_id) {
    BASE* b = &Bases[base_id];

    if (!base_can_riot(base_id, true)) {
        return false;
    }
    if (!conf.delay_drone_riots && unused_space(base_id) > 0 && b->drone_total <= b->talent_total) {
        int cost = (b->pop_size + 1) * cost_factor(b->faction_id, 0, base_id);
        return b->drone_total + 1 > b->talent_total && (base_pop_boom(base_id)
            || (b->nutrients_accumulated + b->nutrient_surplus >= cost));
    }
    return b->drone_total > b->talent_total;
}

bool base_can_riot(int base_id, bool allow_staple) {
    BASE* b = &Bases[base_id];
    return (!allow_staple || !b->nerve_staple_turns_left)
        && !has_project(FAC_TELEPATHIC_MATRIX, b->faction_id)
        && !has_facility(FAC_PUNISHMENT_SPHERE, base_id);
}

bool base_pop_boom(int base_id) {
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    if (b->nutrient_surplus < Rules->nutrient_intake_req_citizen) {
        return false;
    }
    return has_project(FAC_CLONING_VATS, b->faction_id)
        || f->SE_growth_pending
        + (has_facility(FAC_CHILDREN_CRECHE, base_id) ? 2 : 0)
        + (b->state_flags & BSTATE_GOLDEN_AGE_ACTIVE ? 2 : 0) > 5;
}

bool can_use_teleport(int base_id) {
    return has_fac_built(FAC_PSI_GATE, base_id)
        && ~Bases[base_id].state_flags & BSTATE_PSI_GATE_USED;
}

bool has_facility(FacilityId item_id, int base_id) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    if (item_id <= 0) { // zero slot unused
        return false;
    }
    if (item_id >= SP_ID_First) {
        return SecretProjects[item_id - SP_ID_First] == base_id;
    }
    const int freebies[][2] = {
        {FAC_COMMAND_CENTER, FAC_COMMAND_NEXUS},
        {FAC_NAVAL_YARD, FAC_MARITIME_CONTROL_CENTER},
        {FAC_ENERGY_BANK, FAC_PLANETARY_ENERGY_GRID},
        {FAC_PERIMETER_DEFENSE, FAC_CITIZENS_DEFENSE_FORCE},
        {FAC_AEROSPACE_COMPLEX, FAC_CLOUDBASE_ACADEMY},
        {FAC_BIOENHANCEMENT_CENTER, FAC_CYBORG_FACTORY},
        {FAC_QUANTUM_CONVERTER, FAC_SINGULARITY_INDUCTOR},
    };
    for (const int* p : freebies) {
        if (p[0] == item_id && has_project((FacilityId)p[1], Bases[base_id].faction_id)) {
            return true;
        }
    }
    return has_fac_built(item_id, base_id);
}

bool __cdecl has_fac_built(FacilityId item_id, int base_id) {
    return base_id >= 0 && item_id >= 0 && item_id <= Fac_ID_Last
        && Bases[base_id].facilities_built[item_id/8] & (1 << (item_id % 8));
}

/*
Original set_fac does not check variable bounds.
*/
void __cdecl set_fac(FacilityId item_id, int base_id, bool add) {
    if(base_id >= 0 && item_id >= 0 && item_id <= Fac_ID_Last) {
        if (add) {
            Bases[base_id].facilities_built[item_id/8] |= (1 << (item_id % 8));
        } else {
            Bases[base_id].facilities_built[item_id/8] &= ~(1 << (item_id % 8));
        }
    }
}

/*
Calculate facility maintenance cost taking into account faction bonus facilities.
Vanilla game calculates Command Center maintenance based on the current highest
reactor level which is unnecessary to implement here.
*/
int __cdecl fac_maint(int facility_id, int faction_id) {
    CFacility& facility = Facility[facility_id];
    MFaction& meta = MFactions[faction_id];

    for (int i=0; i < meta.faction_bonus_count; i++) {
        if (meta.faction_bonus_val1[i] == facility_id
        && (meta.faction_bonus_id[i] == FCB_FREEFAC
        || (meta.faction_bonus_id[i] == FCB_FREEFAC_PREQ
        && has_tech(facility.preq_tech, faction_id)))) {
            return 0;
        }
    }
    return facility.maint;
}

int satellite_output(int satellites, int pop_size, bool full_value) {
    if (full_value) {
        return max(0, min(pop_size, satellites));
    }
    return max(0, min(pop_size, (satellites + 1) / 2));
}

/*
Calculate satellite bonuses and return true if any satellite is active.
*/
bool satellite_bonus(int base_id, int* nutrient, int* mineral, int* energy) {
    BASE& base = Bases[base_id];
    Faction& f = Factions[base.faction_id];

    if (f.satellites_nutrient > 0 || f.satellites_mineral > 0 || f.satellites_mineral > 0) {
        bool full_value = has_facility(FAC_AEROSPACE_COMPLEX, base_id)
            || has_project(FAC_SPACE_ELEVATOR, base.faction_id);

        *nutrient += satellite_output(f.satellites_nutrient, base.pop_size, full_value);
        *mineral += satellite_output(f.satellites_mineral, base.pop_size, full_value);
        *energy += satellite_output(f.satellites_energy, base.pop_size, full_value);
        return true;
    }
    return f.satellites_ODP > 0;
}

int consider_staple(int base_id) {
    BASE* b = &Bases[base_id];
    if (can_staple(base_id)
    && (!un_charter() || (*SunspotDuration > 1 && *diff_level >= DIFF_LIBRARIAN))
    && b->nerve_staple_count < 3
    && base_can_riot(base_id, true)
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
        && can_build(base_id, FAC_PUNISHMENT_SPHERE) && prod_turns(base_id, -FAC_PUNISHMENT_SPHERE) < 12) {
            return -FAC_PUNISHMENT_SPHERE;
        }
        if (can_build(base_id, FAC_RECREATION_COMMONS)) {
            return -FAC_RECREATION_COMMONS;
        }
        if (has_project(FAC_VIRTUAL_WORLD, b->faction_id) && can_build(base_id, FAC_NETWORK_NODE)) {
            return -FAC_NETWORK_NODE;
        }
        if (can_build(base_id, FAC_HOLOGRAM_THEATRE)) {
            return -FAC_HOLOGRAM_THEATRE;
        }
        if (can_build(base_id, FAC_PARADISE_GARDEN) && prod_turns(base_id, -FAC_PARADISE_GARDEN) < 8) {
            return -FAC_PARADISE_GARDEN;
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
    debug("need_scouts %2d %2d value: %d score: %d scouts: %d goodies: %d native: %d\n",
        x, y, val, score, scouts, Continents[sq->region].pods, *MapNativeLifeForms);
    return val;
}

int project_score(int faction, FacilityId item_id, bool shuffle) {
    CFacility* p = &Facility[item_id];
    Faction* f = &Factions[faction];
    return (shuffle ? random(4) : 0) + f->AI_fight * p->AI_fight
        + (f->AI_growth+1) * p->AI_growth + (f->AI_power+1) * p->AI_power
        + (f->AI_tech+1) * p->AI_tech + (f->AI_wealth+1) * p->AI_wealth;
}

int hurry_item(BASE* b, int mins, int cost) {
    Faction* f = &Factions[b->faction_id];
    debug("hurry_item %d %d mins: %2d cost: %2d credits: %d %s %s\n",
        *current_turn, b->faction_id, mins, cost, f->energy_credits, b->name, prod_name(b->item()));
    f->energy_credits -= cost;
    b->minerals_accumulated += mins;
    b->state_flags |= BSTATE_HURRY_PRODUCTION;
    return 1;
}

int consider_hurry() {
    const int base_id = *current_base_id;
    BASE* b = &Bases[base_id];
    const int t = b->item();
    Faction* f = &Factions[b->faction_id];
    AIPlans* p = &plans[b->faction_id];
    assert(b == *current_base_ptr);
    bool is_cheap = conf.simple_hurry_cost || b->minerals_accumulated >= Rules->retool_exemption;
    bool is_project = t <= -SP_ID_First && t >= -SP_ID_Last;

    if (!thinker_enabled(b->faction_id)) {
        return base_hurry();
    }
    if (conf.base_hurry < (is_project ? 2 : 1)
    || t == -FAC_STOCKPILE_ENERGY || t < -SP_ID_Last
    || b->state_flags & (BSTATE_PRODUCTION_DONE | BSTATE_HURRY_PRODUCTION)) {
        return 0;
    }
    int mins = mineral_cost(base_id, t) - b->minerals_accumulated;
    int cost = hurry_cost(base_id, t, mins);
    int turns = prod_turns(base_id, t);
    int reserve = clamp(*current_turn * 2 + 10 * f->base_count, 20, 600)
        * (is_project ? 1 : 4)
        * (has_fac_built(FAC_HEADQUARTERS, base_id) ? 1 : 2)
        * (b->defend_goal > 3 && p->enemy_factions > 0 ? 1 : 2) / 16;

    if (!is_cheap || mins < 1 || cost < 1 || f->energy_credits - cost < reserve) {
        return 0;
    }
    if (is_project) {
        int delay = clamp(DIFF_TRANSCEND - *diff_level + (f->ranking > 5 + random(4)), 0, 3);
        int threshold = max(Facility[-t].cost/8, 4*cost_factor(b->faction_id, 1, -1));

        if (has_project((FacilityId)-t, -1) || turns < 2 + delay
        || (b->defend_goal > 3 && p->enemy_factions > 0)
        || b->mineral_surplus < 4
        || b->minerals_accumulated < threshold) { // Avoid double cost threshold
            return 0;
        }
        for (int i = 0; i < *total_num_bases; i++) {
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
                values[num] = project_score(b->faction_id, (FacilityId)i, true);
                if (i == -t) {
                    proj_score = values[num];
                }
                num++;
            }
        }
        std::sort(values, values+num);
        debug("hurry_proj %d %d score: %d limit: %d %s\n",
            *current_turn, b->faction_id, proj_score, values[num/2], Facility[-t].name);
        if (proj_score < max(4, values[num/2])) {
            return 0;
        }
        mins = mineral_cost(base_id, t) - b->minerals_accumulated
            - b->mineral_surplus/2 - delay*b->mineral_surplus;
        cost = hurry_cost(base_id, t, mins);

        if (cost > 0 && cost < f->energy_credits && mins > 0 && mins > b->mineral_surplus) {
            hurry_item(b, mins, cost);
            if (!(*game_state & STATE_GAME_DONE)
            && has_treaty(b->faction_id, MapWin->cOwner, DIPLO_COMMLINK)) {
                parse_says(0, MFactions[b->faction_id].adj_name_faction, -1, -1);
                parse_says(1, Facility[-t].name, -1, -1);
                popp(ScriptFile, "DONEPROJECT", 0, "secproj_sm.pcx", 0);
            }
            return 1;
        }
        return 0;
    }
    if (b->drone_total + b->specialist_total > b->talent_total
    && t < 0 && t == need_psych(base_id)) {
        return hurry_item(b, mins, cost);
    }
    if (t < 0 && turns > 1 && cost < f->energy_credits/8) {
        if (t == -FAC_RECYCLING_TANKS || t == -FAC_PRESSURE_DOME
        || t == -FAC_RECREATION_COMMONS || t == -FAC_TREE_FARM
        || t == -FAC_PUNISHMENT_SPHERE || t == -FAC_HEADQUARTERS) {
            return hurry_item(b, mins, cost);
        }
        if (t == -FAC_CHILDREN_CRECHE && unused_space(base_id) > 2) {
            return hurry_item(b, mins, cost);
        }
        if (t == -FAC_HAB_COMPLEX && unused_space(base_id) == 0) {
            return hurry_item(b, mins, cost);
        }
        if (t == -FAC_PERIMETER_DEFENSE && b->defend_goal > 2 && p->enemy_factions > 0) {
            return hurry_item(b, mins, cost);
        }
        if (t == -FAC_AEROSPACE_COMPLEX && has_tech(TECH_Orbital, b->faction_id)) {
            return hurry_item(b, mins, cost);
        }
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
        if (Units[t].is_former() && cost < f->energy_credits/16
        && (*current_turn < 80 || b->mineral_surplus < p->median_limit)) {
            return hurry_item(b, mins, cost);
        }
        if (Units[t].is_colony() && cost < f->energy_credits/16
        && (b->state_flags & BSTATE_DRONE_RIOTS_ACTIVE
        || (!unused_space(base_id) && turns > 3))) {
            return hurry_item(b, mins, cost);
        }
    }
    return 0;
}

bool redundant_project(int faction, int fac_id) {
    Faction* f = &Factions[faction];
    if (fac_id == FAC_PLANETARY_DATALINKS) {
        int n = 0;
        for (int i=0; i < MaxPlayerNum; i++) {
            if (Factions[i].base_count > 0) {
                n++;
            }
        }
        return n < 4;
    }
    if (fac_id == FAC_CITIZENS_DEFENSE_FORCE) {
        return Facility[FAC_PERIMETER_DEFENSE].maint == 0
            && facility_count(FAC_PERIMETER_DEFENSE, faction) > f->base_count/2 + 2;
    }
    if (fac_id == FAC_MARITIME_CONTROL_CENTER) {
        int n = 0;
        for (int i=0; i<*total_num_vehicles; i++) {
            VEH* veh = &Vehicles[i];
            if (veh->faction_id == faction && veh->triad() == TRIAD_SEA) {
                n++;
            }
        }
        return n < 8 && n < f->base_count/3;
    }
    if (fac_id == FAC_HUNTER_SEEKER_ALGORITHM) {
        return f->SE_probe >= 3;
    }
    if (fac_id == FAC_LIVING_REFINERY) {
        return f->SE_support >= 3;
    }
    return false;
}

int find_satellite(int base_id, int defenders) {
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

    if (p.enemy_factions > 0 && base.defend_goal > 2 && defenders < base.defend_goal) {
        return 0;
    }
    if (!has_complex && ((base_id + *current_turn)/8) & 1) {
        return 0;
    }
    if (!has_complex && !can_build(base_id, FAC_AEROSPACE_COMPLEX)) {
        return 0;
    }
    for (const int fac_id : satellites) {
        if (!has_tech(Facility[fac_id].preq_tech, base.faction_id)) {
            continue;
        }
        int prod_num = prod_count(-fac_id, base.faction_id, base_id);
        int built_num = satellite_count(base.faction_id, fac_id);
        int goal_num = satellite_goal(base.faction_id, fac_id);

        if (fac_id != FAC_ORBITAL_DEFENSE_POD) {
            if (clamp(p.enemy_odp - f.satellites_ODP + 7, 0, 14) > random(16)) {
                continue;
            }
        }
        if (built_num + prod_num >= goal_num) {
            continue;
        }
        if (!has_complex && can_build(base_id, FAC_AEROSPACE_COMPLEX)) {
            if (min(12, prod_turns(base_id, -FAC_AEROSPACE_COMPLEX)) < 2+random(16)) {
                return -FAC_AEROSPACE_COMPLEX;
            }
            return 0;
        }
        if (has_complex && min(12, prod_turns(base_id, -fac_id)) < 2+random(16)) {
            return -fac_id;
        }
    }
    return 0;
}

int find_planet_buster(int faction) {
    int best_id = -1;
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        int unit_id = faction*MaxProtoFactionNum + i;
        UNIT* u = &Units[unit_id];
        if (u->is_planet_buster() && strlen(u->name) > 0
        && u->std_offense_value() > Units[best_id].std_offense_value()) {
            best_id = unit_id;
        }
    }
    return best_id;
}

int find_project(int base_id) {
    BASE& base = Bases[base_id];
    int faction = base.faction_id;
    Faction* f = &Factions[faction];
    int projs = 0;
    int nukes = 0;
    int works = 0;
    int bases = f->base_count;
    int diplo = plans[faction].diplo_flags;
    int similar_limit = (base.minerals_accumulated >= 80 ? 2 : 1);
    int nuke_score = (un_charter() ? 0 : 3) + 2*f->AI_power + f->AI_fight
        + max(-1, plans[faction].enemy_nukes - f->satellites_ODP)
        + (diplo & DIPLO_ATROCITY_VICTIM ? 5 : 0)
        + (diplo & DIPLO_WANT_REVENGE ? 4 : 0);
    int unit_id = find_planet_buster(faction);
    int nuke_limit = (unit_id >= 0 && nuke_score > 2 &&
        f->planet_busters < 2 + bases/40 + f->AI_fight ? 1 + bases/40 : 0);

    for (int i=0; i < *total_num_bases; i++) {
        if (Bases[i].faction_id == faction) {
            int t = Bases[i].queue_items[0];
            if (t <= -SP_ID_First || t == -FAC_SUBSPACE_GENERATOR) {
                projs++;
            } else if (t == -FAC_SKUNKWORKS) {
                works++;
            } else if (t >= 0 && Units[t].is_planet_buster()) {
                nukes++;
            }
        }
    }
    if (unit_id >= 0 && nukes < nuke_limit && nukes < bases/8
    && (!((base_id + *current_turn) & 3) || has_facility(FAC_SKUNKWORKS, base_id))) {
        debug("find_project %d %d %s\n", faction, unit_id, Units[unit_id].name);
        if (!Units[unit_id].is_prototyped()
        && Rules->extra_cost_prototype_air >= 50
        && can_build(base_id, FAC_SKUNKWORKS)) {
            if (works < 2) {
                return -FAC_SKUNKWORKS;
            }
        } else {
            return unit_id;
        }
    }
    if (projs+nukes < 3 + nuke_limit && projs+nukes < bases/4) {
        if (can_build(base_id, FAC_SUBSPACE_GENERATOR)) {
            return -FAC_SUBSPACE_GENERATOR;
        }
        int best_score = INT_MIN;
        int choice = 0;
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            if (can_build(base_id, i) && prod_count(-i, faction, base_id) < similar_limit
            && (similar_limit > 1 || !redundant_project(faction, i))) {
                int score = project_score(faction, (FacilityId)i, true);
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
    UNIT* u1 = &Units[unit_id1];
    UNIT* u2 = &Units[unit_id2];
    bool val = (u1->cost >= u2->cost
        && offense_value(unit_id1) >= 0
        && offense_value(unit_id1) <= offense_value(unit_id2)
        && defense_value(unit_id1) <= defense_value(unit_id2)
        && Chassis[u1->chassis_id].speed <= Chassis[u2->chassis_id].speed
        && (Chassis[u2->chassis_id].triad != TRIAD_AIR || u1->chassis_id == u2->chassis_id)
        && !((u1->ability_flags & u2->ability_flags) ^ u1->ability_flags))
        && (u1->ability_flags & ABL_SLOW) <= (u2->ability_flags & ABL_SLOW);
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
        {ABL_AMPHIBIOUS, -2},
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
    int v = 18 * (defend ? defense_value(id) : offense_value(id));
    if (v < 0) {
        v = (defend ? Armor[best_armor(faction, false)].defense_value
            : Weapon[best_weapon(faction)].offense_value)
            * (conf.ignore_reactor_power ? REC_FISSION : best_reactor(faction))
            + plans[faction].psi_score * 12;
    }
    if (u->triad() != TRIAD_AIR) {
        v += (defend ? 12 : 32) * u->speed();
        if (u->triad() == TRIAD_SEA && u->is_combat_unit()
        && defense_value(id) > offense_value(id)) {
            v -= 20;
        }
    }
    if (u->ability_flags & ABL_POLICE_2X && need_police(faction)) {
        v += (u->speed() > 1 ? 16 : 32);
    }
    if (u->is_missile()) {
        v -= 8 * plans[faction].missile_units;
    }
    for (const int* s : specials) {
        if (u->ability_flags & s[0]) {
            v += 8 * s[1];
        }
    }
    int turns = max(0, u->cost * cfactor - accumulated) / max(2, minerals);
    int score = v - turns * (u->is_colony() ? 6 : 3)
        * (max(2, 7 - *current_turn/16) + max(0, 2 - minerals/4));
    debug("unit_score %s cfactor: %d minerals: %d cost: %d turns: %d score: %d\n",
        u->name, cfactor, minerals, u->cost, turns, score);
    return score;
}

/*
Find the best prototype for base production when weighted against cost given the triad
and type constraints. For any combat-capable unit, mode is set to WMODE_COMBAT.
*/
int find_proto(int base_id, Triad triad, VehWeaponMode mode, bool defend) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    BASE* b = &Bases[base_id];
    int faction = b->faction_id;
    int basic = BSC_SCOUT_PATROL;
    bool combat = (mode == WMODE_COMBAT);
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
            if (id < MaxProtoFactionNum && !has_tech(u->preq_tech, faction)) {
                continue;
            }
            if ((!combat && Weapon[u->weapon_id].mode != mode)
            || (combat && Weapon[u->weapon_id].offense_value == 0)
            || (combat && defend && u->chassis_id != CHS_INFANTRY)
            || (u->is_psi_unit() && plans[faction].psi_score < 1)
            || (is_human(faction) && u->obsolete_factions & (1 << faction))
            || (is_human(faction) && !u->is_prototyped() && ~b->governor_flags & GOV_MAY_PROD_PROTOTYPE)
            || u->is_planet_buster()) {
                continue;
            }
            if (mode == WMODE_COMBAT && best_id != basic
            && ((defend && offense_value(id) > defense_value(id))
            || (!defend && offense_value(id) < defense_value(id)))) {
                continue;
            }
            int val = unit_score(
                id, faction, cfactor, b->mineral_surplus, b->minerals_accumulated, defend);
            if (unit_is_better(best_id, id) || random(100) > 50 + best_val - val) {
                best_id = id;
                best_val = val;
                debug("===> %s\n", Units[best_id].name);
            }
        }
    }
    return best_id;
}

Triad select_colony(int base_id, int pods, bool build_ships) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    bool land = has_base_sites(base->x, base->y, faction, TRIAD_LAND);
    bool sea = has_base_sites(base->x, base->y, faction, TRIAD_SEA);
    int limit = (*current_turn < 60 || (!random(3) && (land || (build_ships && sea))) ? 2 : 1);

    if (Factions[faction].base_count >= 8
    && base_id < *total_num_bases/4
    && base->mineral_surplus >= plans[faction].project_limit
    && find_project(base_id) != 0) {
        limit = (base_id > *total_num_bases/8);
    }
    if (pods >= limit) {
        return TRIAD_NONE;
    }
    if (is_ocean(base)) {
        for (auto& m : iterate_tiles(base->x, base->y, 1, 9)) {
            if (land && (!m.sq->is_owned() || (m.sq->owner == faction && !random(6)))
            && (m.sq->veh_owner() < 0 || m.sq->veh_owner() == faction)) {
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
    // AI bases are not limited by governor settings
    int gov = (is_human(faction) ? base->governor_flags : ~0);
    int w1 = 4*plans[faction].air_combat_units < f->base_count ? 2 : 5;
    int w2 = (plans[faction].transport_units < 2
        || 5*plans[faction].transport_units < f->base_count ? 2 : 5);
    bool need_ships = 6*plans[faction].sea_combat_units < plans[faction].land_combat_units;
    bool reserve = base->mineral_surplus >= base->mineral_intake_2/2;
    bool probes = has_wmode(faction, WMODE_INFOWAR) && gov & GOV_MAY_PROD_PROBES;
    bool transports = has_wmode(faction, WMODE_TRANSPORT) && gov & GOV_MAY_PROD_TRANSPORT;
    bool aircraft = has_aircraft(faction) && gov & (GOV_MAY_PROD_AIR_COMBAT | GOV_MAY_PROD_AIR_DEFENS);

    if (probes && (!random(num_probes*2 + 4) || !reserve)) {
        return find_proto(base_id, (build_ships ? TRIAD_SEA : TRIAD_LAND), WMODE_INFOWAR, DEF);
    }
    if (aircraft && (f->SE_police >= -3 || !base_can_riot(base_id, true)) && !random(w1)) {
        return find_proto(base_id, TRIAD_AIR, WMODE_COMBAT, ATT);
    }
    if (build_ships && gov & (GOV_MAY_PROD_TRANSPORT | GOV_MAY_PROD_NAVAL_COMBAT)) {
        int min_dist = INT_MAX;
        bool sea_enemy = false;

        for (int i=0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (faction != b->faction_id && !has_pact(faction, b->faction_id)) {
                int dist = map_range(base->x, base->y, b->x, b->y)
                    * (at_war(faction, b->faction_id) ? 1 : 4);
                if (dist < min_dist) {
                    sea_enemy = is_ocean(b);
                    min_dist = dist;
                }
            }
        }
        if (random(4) < (sea_base ? 3 : 1 + (need_ships || sea_enemy))) {
            VehWeaponMode mode;
            if (!transports) {
                mode = WMODE_COMBAT;
            } else if (~gov & GOV_MAY_PROD_NAVAL_COMBAT) {
                mode = WMODE_TRANSPORT;
            } else {
                mode = (!random(w2) ? WMODE_TRANSPORT : WMODE_COMBAT);
            }
            return find_proto(base_id, TRIAD_SEA, mode, ATT);
        }
    }
    return find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, (sea_base || !random(5) ? DEF : ATT));
}

int select_build(int base_id) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    AIPlans* p = &plans[faction];
    int gov = (is_human(faction) ? base->governor_flags : ~0);
    int minerals = base->mineral_surplus + base->minerals_accumulated/10;
    int cfactor = cost_factor(faction, 1, -1);
    int reserve = max(2, base->mineral_intake_2 / 2);
    int content_pop_value = (is_human(faction) ? conf.content_pop_player[*diff_level]
        : conf.content_pop_computer[*diff_level]);
    bool sea_base = is_ocean(base);
    bool core_base = minerals >= plans[faction].project_limit;
    bool project_change = base->item_is_project()
        && !can_build(base_id, -base->item())
        && ~base->state_flags & BSTATE_PRODUCTION_DONE
        && base->minerals_accumulated > Rules->retool_exemption;
    bool allow_units = can_build_unit(base_id, -1) && !project_change;
    bool allow_supply = !sea_base;
    int all_crawlers = 0;
    int near_formers = 0;
    int need_ferry = 0;
    int transports = 0;
    int landprobes = 0;
    int seaprobes = 0;
    int defenders = 0;
    int formers = 0;
    int scouts = 0;
    int pods = 0;

    for (int i=0; i < *total_num_vehicles; i++) {
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
            if (map_range(base->x, base->y, veh->x, veh->y) <= 1) {
                defenders++;
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
        }
        if (sea_base && veh->x == base->x && veh->y == base->y && veh->triad() == TRIAD_LAND) {
            if (veh->is_colony() || veh->is_former() || veh->is_supply()) {
                need_ferry++;
            }
        }
    }
    if (is_human(faction)) {
        p->target_land_region = 0;
        plans_upkeep(faction);
    }
    bool build_ships = can_build_ships(base_id);
    bool build_pods = allow_expand(faction) && pods < 2
        && (base->pop_size > 1 || base->nutrient_surplus > 1);
    double w1 = min(1.0, max(0.4, 1.0 * minerals / p->project_limit))
        * (conf.conquer_priority / 100.0)
        * (region_at(base->x, base->y) == p->target_land_region
        && p->main_region != p->target_land_region ? 4 : 1);
    double w2 = 2.0 * p->enemy_mil_factor / (p->enemy_base_range * 0.1 + 0.1)
        + 0.8 * p->enemy_bases + min(0.4, *current_turn/400.0)
        + min(1.0, 1.5 * f->base_count / *map_area_sq_root) * (f->AI_fight * 0.2 + 0.8);
    double threat = 1 - (1 / (1 + max(0.0, w1 * w2)));

    debug("select_build %d %d %2d %2d def: %d frm: %d prb: %d crw: %d pods: %d expand: %d "\
        "scouts: %d min: %2d res: %2d limit: %2d mil: %.4f threat: %.4f\n",
        *current_turn, faction, base->x, base->y,
        defenders, formers, landprobes+seaprobes, all_crawlers, pods, build_pods,
        scouts, minerals, reserve, p->project_limit, p->enemy_mil_factor, threat);

    const int SecretProject = -1;
    const int MorePsych = -2;
    const int DefendUnit = -3;
    const int CombatUnit = -4;
    const int ColonyUnit = -5;
    const int FormerUnit = -6;
    const int FerryUnit = -7;
    const int CrawlerUnit = -8;
    const int SeaProbeUnit = -9;
    const int Satellites = -10;

    const int F_Mineral = 1;
    const int F_Energy = 2;
    const int F_Repair = 4;
    const int F_Combat = 8;
    const int F_Trees = 16;
    const int F_Surplus = 32;
    const int F_Default = 64;

    const int build_order[][2] = {
        {DefendUnit,                0},
        {MorePsych,                 0},
        {FAC_PRESSURE_DOME,         0},
        {FAC_HEADQUARTERS,          0},
        {Satellites,                0},
        {CombatUnit,                0},
        {FormerUnit,                0},
        {FAC_RECYCLING_TANKS,       0},
        {SeaProbeUnit,              0},
        {CrawlerUnit,               0},
        {FerryUnit,                 0},
        {ColonyUnit,                0},
        {FAC_RECREATION_COMMONS,    F_Default},
        {FAC_TREE_FARM,             F_Trees},
        {SecretProject,             0},
        {FAC_CHILDREN_CRECHE,       0},
        {FAC_HAB_COMPLEX,           0},
        {FAC_HOLOGRAM_THEATRE,      F_Default},
        {FAC_PERIMETER_DEFENSE,     F_Combat},
        {FAC_GENEJACK_FACTORY,      F_Mineral},
        {FAC_AEROSPACE_COMPLEX,     0},
        {FAC_ROBOTIC_ASSEMBLY_PLANT,F_Mineral},
        {FAC_NANOREPLICATOR,        F_Mineral},
        {FAC_TACHYON_FIELD,         F_Combat},
        {FAC_NETWORK_NODE,          F_Energy|F_Default},
        {FAC_COMMAND_CENTER,        F_Repair|F_Combat},
        {FAC_NAVAL_YARD,            F_Repair|F_Combat},
        {FAC_HABITATION_DOME,       0},
        {FAC_GEOSYNC_SURVEY_POD,    F_Surplus},
        {FAC_PSI_GATE,              F_Surplus},
        {FAC_ORBITAL_DEFENSE_POD,   F_Surplus},
        {FAC_NESSUS_MINING_STATION, F_Surplus},
        {FAC_ORBITAL_POWER_TRANS,   F_Surplus},
        {FAC_SKY_HYDRO_LAB,         F_Surplus},
        {FAC_FUSION_LAB,            F_Energy},
        {FAC_ENERGY_BANK,           F_Energy},
        {FAC_RESEARCH_HOSPITAL,     F_Energy},
        {FAC_FLECHETTE_DEFENSE_SYS, F_Combat|F_Surplus},
        {FAC_QUANTUM_LAB,           F_Energy|F_Surplus},
        {FAC_NANOHOSPITAL,          F_Energy|F_Surplus},
        {FAC_COVERT_OPS_CENTER,     F_Combat|F_Surplus},
        {FAC_HYBRID_FOREST,         F_Energy|F_Trees|F_Surplus},
    };
    int default_choice = 0;

    for (const int* item : build_order) {
        const CFacility& facility = Facility[item[0]];
        const int t = item[0];
        const int flag = item[1];
        int choice = 0;

        if (t == Satellites && minerals >= p->median_limit) {
            if ((choice = find_satellite(base_id, defenders)) != 0) {
                return choice;
            }
        }
        if (t == SecretProject && core_base && gov & GOV_MAY_PROD_SP) {
            if ((choice = find_project(base_id)) != 0) {
                return choice;
            }
        }
        if (t == MorePsych && (choice = need_psych(base_id)) != 0) {
            return choice;
        }
        if (t < 0 && !allow_units) {
            continue;
        }
        if (t == DefendUnit && gov & GOV_ALLOW_COMBAT && gov & GOV_MAY_PROD_LAND_DEFENS) {
            if (minerals > 2 && (defenders < 1 || need_scouts(base->x, base->y, faction, scouts))) {
                return find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, DEF);
            }
        }
        if (t == CombatUnit && gov & GOV_ALLOW_COMBAT) {
            if (minerals > reserve && random(100) < (int)(100 * threat)) {
                return select_combat(base_id, landprobes+seaprobes, sea_base, build_ships);
            }
        }
        if (t == SeaProbeUnit && gov & GOV_MAY_PROD_PROBES) {
            if (build_ships && has_wmode(faction, WMODE_INFOWAR)
            && ocean_coast_tiles(base->x, base->y) && !random(seaprobes > 0 ? 6 : 3)
            && p->unknown_factions > 1 && p->contacted_factions < 2) {
                return find_proto(base_id, TRIAD_SEA, WMODE_INFOWAR, DEF);
            }
        }
        if (t == FormerUnit && gov & GOV_MAY_PROD_TERRAFORMS) {
            if (has_wmode(faction, WMODE_TERRAFORMER)
            && formers + near_formers/2 < (base->pop_size < 4 ? 1 : 2)) {
                int num = 0;
                int sea = 0;
                for (auto& m : iterate_tiles(base->x, base->y, 1, 21)) {
                    if (m.sq->owner == faction && select_item(m.x, m.y, faction, m.sq) >= 0) {
                        num++;
                        sea += is_ocean(m.sq);
                    }
                }
                if (num > 3) {
                    if (has_chassis(faction, CHS_GRAVSHIP)) {
                        int unit_id = find_proto(base_id, TRIAD_AIR, WMODE_TERRAFORMER, DEF);
                        if (unit_id >= 0 && Units[unit_id].triad() == TRIAD_AIR
                        && prod_turns(base_id, unit_id) < 8) {
                            return unit_id;
                        }
                    }
                    if ((sea*2 >= num || sea_base) && has_ships(faction)) {
                        return find_proto(base_id, TRIAD_SEA, WMODE_TERRAFORMER, DEF);
                    }
                    if (!sea_base) {
                        return find_proto(base_id, TRIAD_LAND, WMODE_TERRAFORMER, DEF);
                    }
                }
            }
        }
        if (t == FerryUnit && gov & GOV_MAY_PROD_TRANSPORT) {
            if (build_ships && !transports && need_ferry) {
                for (auto& m : iterate_tiles(base->x, base->y, 1, 9)) {
                    if (!is_ocean(m.sq) && (!m.sq->is_owned() || m.sq->owner == faction)) {
                        return find_proto(base_id, TRIAD_SEA, WMODE_TRANSPORT, DEF);
                    }
                }
            }
        }
        if (t == ColonyUnit && gov & GOV_MAY_PROD_COLONY_POD) {
            if (build_pods) {
                Triad type = select_colony(base_id, pods, build_ships);
                if (type == TRIAD_LAND || type == TRIAD_SEA) {
                    return find_proto(base_id, type, WMODE_COLONIST, DEF);
                }
            }
        }
        if (t == CrawlerUnit) {
            if (allow_supply && has_wmode(faction, WMODE_CONVOY)
            && 100 * all_crawlers < conf.crawler_priority * f->base_count) {
                default_choice = find_proto(base_id, TRIAD_LAND, WMODE_CONVOY, DEF);
                if (prod_turns(base_id, default_choice) < 10 && random(2)) {
                    return default_choice;
                }
            }
        }
        if (t < 0 || !can_build(base_id, t) || (t >= 0 && ~gov & GOV_MAY_PROD_FACILITIES)) {
            continue; // Skip facility checks
        }
        if (flag & F_Default && !default_choice) {
            default_choice = -t;
        }
        int turns = max(0, facility.cost * min(12, cfactor) - base->minerals_accumulated)
            / max(2, base->mineral_surplus);
        /* Check if we have sufficient base energy for multiplier facilities. */
        if (flag & F_Energy && (turns > 10 || base->energy_surplus < 8))
            continue;
        /* Avoid building combat-related facilities in peacetime. */
        if (flag & F_Combat && p->enemy_base_range > MaxEnemyRange - 5*min(5, facility.maint))
            continue;
        if (flag & F_Repair && (!conf.repair_base_facility || f->SE_morale < 0))
            continue;
        if (flag & F_Surplus && turns > random(4) + (allow_units ? 3 : 5))
            continue;
        if (flag & F_Mineral && (base->mineral_intake_2 > (core_base ? 80 : 50)
        || 3*base->mineral_intake_2 > 2*(conf.clean_minerals + f->clean_minerals_modifier)
        || turns > 5 + f->AI_power + f->AI_wealth))
            continue;
        if (flag & F_Trees && (base->eco_damage < random(16)
        || (!base->eco_damage && (turns > random(16) || nearby_items(base->x, base->y, 1, BIT_FOREST) < 3))))
            continue;
        if (t == FAC_RECYCLING_TANKS && base->drone_total > 0 && can_build(base_id, FAC_RECREATION_COMMONS))
            continue;
        if ((t == FAC_RECREATION_COMMONS || t == FAC_HOLOGRAM_THEATRE)
        && base->pop_size < content_pop_value + 1
        && !base->drone_total && base->specialist_total < 2)
            continue;
        if (t == FAC_PRESSURE_DOME && (*climate_future_change <= 0 || !is_shore_level(mapsq(base->x, base->y))))
            continue;
        if (t == FAC_HEADQUARTERS && (conf.auto_relocate_hq || !valid_relocate_base(base_id)))
            continue;
        if (t == FAC_COMMAND_CENTER && (sea_base || turns > 5))
            continue;
        if (t == FAC_NAVAL_YARD && (!sea_base || turns > 5))
            continue;
        if (t == FAC_TACHYON_FIELD && base->defend_goal < 4 && turns > (allow_units ? 2 : 4))
            continue;
        if (t == FAC_HAB_COMPLEX && unused_space(base_id) > 0)
            continue;
        if (t == FAC_HABITATION_DOME && unused_space(base_id) > 0)
            continue;
        if (t == FAC_PSI_GATE && facility_count(FAC_PSI_GATE, faction) >= f->base_count/2)
            continue;
        if (t > 0) {
            return -t;
        }
    }
    if (default_choice) {
        return default_choice;
    }
    if (!allow_units) {
        return -FAC_STOCKPILE_ENERGY;
    }
    debug("BUILD OFFENSE\n");
    return select_combat(base_id, landprobes+seaprobes, sea_base, build_ships);
}






