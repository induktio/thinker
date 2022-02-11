
#include "game.h"

static uint32_t random_state = 0;

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
    int minerals = max(0, mineral_cost(base_id, item_id) - b->minerals_accumulated);
    int surplus = max(1, 10 * b->mineral_surplus);
    return 10 * minerals / surplus + ((10 * minerals) % surplus != 0);
}

int mineral_cost(int base_id, int item_id) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    // Take possible prototype costs into account in veh_cost
    if (item_id >= 0) {
        return veh_cost(item_id, base_id, 0) * cost_factor(Bases[base_id].faction_id, 1, -1);
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
        * (has_project(-1, FAC_VOICE_OF_PLANET) ? 2 : 1)
        * m->rule_hurry / 100;
    if (hurry_mins > 0 && mins > 0) {
        return hurry_mins * cost / mins + (((hurry_mins * cost) % mins) != 0);
    }
    return 0;
}

bool has_tech(int faction, int tech) {
    assert(valid_player(faction));
    if (tech == TECH_None) {
        return true;
    }
    return tech >= 0 && TechOwners[tech] & (1 << faction);
}

bool has_ability(int faction, VehAbility abl) {
    return has_tech(faction, Ability[abl].preq_tech);
}

bool has_ability(int faction, VehAbility abl, VehChassis chs, VehWeapon wpn) {
    int F = Ability[abl].flags;
    int triad = Chassis[chs].triad;

    if ((triad == TRIAD_LAND && ~F & AFLAG_ALLOWED_LAND_UNIT)
    || (triad == TRIAD_SEA && ~F & AFLAG_ALLOWED_SEA_UNIT)
    || (triad == TRIAD_AIR && ~F & AFLAG_ALLOWED_AIR_UNIT)) {
        return false;
    }
    if ((~F & AFLAG_ALLOWED_COMBAT_UNIT && wpn <= WPN_PSI_ATTACK)
    || (~F & AFLAG_ALLOWED_TERRAFORM_UNIT && wpn == WPN_TERRAFORMING_UNIT)
    || (~F & AFLAG_ALLOWED_NONCOMBAT_UNIT
    && wpn > WPN_PSI_ATTACK && wpn != WPN_TERRAFORMING_UNIT)) {
        return false;
    }
    if ((F & AFLAG_NOT_ALLOWED_PROBE_TEAM && wpn == WPN_PROBE_TEAM)
    || (F & AFLAG_ONLY_PROBE_TEAM && wpn != WPN_PROBE_TEAM)) {
        return false;
    }
    if (F & AFLAG_TRANSPORT_ONLY_UNIT && wpn != WPN_TROOP_TRANSPORT) {
        return false;
    }
    if (F & AFLAG_NOT_ALLOWED_FAST_UNIT && Chassis[chs].speed > 1) {
        return false;
    }
    if (F & AFLAG_NOT_ALLOWED_PSI_UNIT && wpn == WPN_PSI_ATTACK) {
        return false;
    }
    return has_tech(faction, Ability[abl].preq_tech);
}


bool has_chassis(int faction, VehChassis chs) {
    return has_tech(faction, Chassis[chs].preq_tech);
}

bool has_weapon(int faction, VehWeapon wpn) {
    return has_tech(faction, Weapon[wpn].preq_tech);
}

bool has_aircraft(int faction) {
    return has_chassis(faction, CHS_NEEDLEJET) || has_chassis(faction, CHS_COPTER)
        || has_chassis(faction, CHS_GRAVSHIP);
}

bool has_ships(int faction) {
    return has_chassis(faction, CHS_FOIL) || has_chassis(faction, CHS_CRUISER);
}

bool has_terra(int faction, FormerItem act, bool ocean) {
    int preq_tech = (ocean ? Terraform[act].preq_tech_sea : Terraform[act].preq_tech);
    if ((act == FORMER_RAISE_LAND || act == FORMER_LOWER_LAND)
    && *game_rules & RULES_SCN_NO_TERRAFORMING) {
        return false;
    }
    if (act >= FORMER_CONDENSER && act <= FORMER_LOWER_LAND
    && has_project(faction, FAC_WEATHER_PARADIGM)) {
        return preq_tech != TECH_Disable;
    }
    return has_tech(faction, preq_tech);
}

bool has_project(int faction, int id) {
    /* If faction_id is negative, check if anyone has built the project. */
    assert(faction < MaxPlayerNum && id >= SP_ID_First && id <= FAC_EMPTY_SP_64);
    int i = SecretProjects[id - SP_ID_First];
    return i >= 0 && (faction < 0 || Bases[i].faction_id == faction);
}

bool has_facility(int base_id, int id) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    assert(id > 0 && id <= FAC_EMPTY_SP_64);
    if (id >= SP_ID_First) {
        return SecretProjects[id - SP_ID_First] == base_id;
    }
    int faction = Bases[base_id].faction_id;
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
        if (p[0] == id && has_project(faction, p[1])) {
            return true;
        }
    }
    return Bases[base_id].facilities_built[id/8] & (1 << (id % 8));
}

bool has_fac_built(int base_id, int facility_id) {
    if (facility_id < Fac_ID_First || facility_id > Fac_ID_Last) {
        return false;
    }
    return Bases[base_id].facilities_built[facility_id/8] & (1 << (facility_id % 8));
}

bool can_build(int base_id, int id) {
    assert(base_id >= 0 && base_id < *total_num_bases);
    assert(id > 0 && id <= FAC_EMPTY_SP_64);
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    if (id >= SP_ID_First && id <= FAC_EMPTY_SP_64) {
        if (SecretProjects[id-SP_ID_First] != SP_Unbuilt
        || *game_rules & RULES_SCN_NO_BUILDING_SP) {
            return false;
        }
    }
    if (id == FAC_ASCENT_TO_TRANSCENDENCE || id == FAC_VOICE_OF_PLANET) {
        if (is_alien(faction) || ~*game_rules & RULES_VICTORY_TRANSCENDENCE) {
            return false;
        }
    }
    if (id == FAC_ASCENT_TO_TRANSCENDENCE) {
        return has_project(-1, FAC_VOICE_OF_PLANET)
            && !has_project(-1, FAC_ASCENT_TO_TRANSCENDENCE);
    }
    if (id == FAC_HEADQUARTERS && find_hq(faction) >= 0) {
        return false;
    }
    if (id == FAC_RECYCLING_TANKS && has_facility(base_id, FAC_PRESSURE_DOME)) {
        return false;
    }
    if (id == FAC_HOLOGRAM_THEATRE && (has_project(faction, FAC_VIRTUAL_WORLD)
    || !has_facility(base_id, FAC_RECREATION_COMMONS))) {
        return false;
    }
    if ((id == FAC_RECREATION_COMMONS || id == FAC_HOLOGRAM_THEATRE
    || id == FAC_PUNISHMENT_SPHERE || id == FAC_PARADISE_GARDEN)
    && !base_can_riot(base_id, false)) {
        return false;
    }
    if (id == FAC_GENEJACK_FACTORY && base_can_riot(base_id, false)
    && base->talent_total + 1 < base->drone_total + Rules->drones_induced_genejack_factory) {
        return false;
    }
    if ((id == FAC_HAB_COMPLEX || id == FAC_HABITATION_DOME) && base->nutrient_surplus < 2) {
        return false;
    }
    if (id == FAC_SUBSPACE_GENERATOR) {
        if (!is_alien(faction) || base->pop_size < Rules->base_size_subspace_gen) {
            return false;
        }
        int n = 0;
        for (int i=0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && has_facility(i, FAC_SUBSPACE_GENERATOR)
            && b->pop_size >= Rules->base_size_subspace_gen
            && ++n >= Rules->subspace_gen_req) {
                return false;
            }
        }
    }
    if (id >= FAC_SKY_HYDRO_LAB && id <= FAC_ORBITAL_DEFENSE_POD) {
        if (!has_facility(base_id, FAC_AEROSPACE_COMPLEX)) {
            return false;
        }
        int n = prod_count(faction, -id, base_id);
        int goal = plans[faction].satellite_goal;
        if (id == FAC_ORBITAL_DEFENSE_POD) {
            goal = min(conf.max_satellites,
                goal/4 + clamp(f->base_count/8, 2, 8)
                + (plans[faction].diplo_flags & DIPLO_VENDETTA ? 4 : 0));
        }
        if ((id == FAC_SKY_HYDRO_LAB && f->satellites_nutrient + n >= goal)
        || (id == FAC_ORBITAL_POWER_TRANS && f->satellites_energy + n >= goal)
        || (id == FAC_NESSUS_MINING_STATION && f->satellites_mineral + n >= goal)
        || (id == FAC_ORBITAL_DEFENSE_POD && f->satellites_ODP + n >= goal)) {
            return false;
        }
    }
    if (id == FAC_FLECHETTE_DEFENSE_SYS && f->satellites_ODP > 0) {
        return false;
    }
    if (id == FAC_GEOSYNC_SURVEY_POD || id == FAC_FLECHETTE_DEFENSE_SYS) {
        for (int i=0; i < *total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && i != base_id
            && map_range(base->x, base->y, b->x, b->y) <= 3
            && (has_facility(i, FAC_GEOSYNC_SURVEY_POD)
            || has_facility(i, FAC_FLECHETTE_DEFENSE_SYS)
            || b->item() == -FAC_GEOSYNC_SURVEY_POD
            || b->item() == -FAC_FLECHETTE_DEFENSE_SYS)) {
                return false;
            }
        }
    }
    /* Rare special case if the game engine reaches the global unit limit. */
    if (id == FAC_STOCKPILE_ENERGY) {
        return (*current_turn + base_id) % 4 > 0 || !can_build_unit(faction, -1);
    }
    return has_tech(faction, Facility[id].preq_tech) && !has_facility(base_id, id);
}

bool can_build_unit(int faction, int id) {
    assert(valid_player(faction) && id >= -1);
    UNIT* u = &Units[id];
    if (id >= 0 && id < MaxProtoFactionNum && u->preq_tech != TECH_None
    && !has_tech(faction, u->preq_tech)) {
        return false;
    }
    return *total_num_vehicles < MaxVehNum * 15 / 16;
}

bool can_build_ships(int base_id) {
    BASE* b = &Bases[base_id];
    int k = *map_area_sq_root + 20;
    return has_ships(b->faction_id) && nearby_tiles(b->x, b->y, TRIAD_SEA, k) >= k;
}

bool base_can_riot(int base_id, bool allow_staple) {
    BASE* b = &Bases[base_id];
    return (!allow_staple || !b->nerve_staple_turns_left)
        && !has_project(b->faction_id, FAC_TELEPATHIC_MATRIX)
        && !has_facility(base_id, FAC_PUNISHMENT_SPHERE);
}

bool base_pop_boom(int base_id) {
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    if (b->nutrient_surplus < Rules->nutrient_intake_req_citizen) {
        return false;
    }
    return has_project(b->faction_id, FAC_CLONING_VATS)
        || f->SE_growth_pending
        + (has_facility(base_id, FAC_CHILDREN_CRECHE) ? 2 : 0)
        + (b->state_flags & BSTATE_GOLDEN_AGE_ACTIVE ? 2 : 0) > 5;
}

bool can_use_teleport(int base_id) {
    return has_facility(base_id, FAC_PSI_GATE)
        && ~Bases[base_id].state_flags & BSTATE_PSI_GATE_USED;
}

/*
Determine if the specified faction is controlled by a human player or computer AI.
*/
bool is_human(int faction) {
    return FactionStatus[0] & (1 << faction);
}

/*
Determine if the specified faction is alive or whether they've been eliminated.
*/
bool is_alive(int faction) {
    return FactionStatus[1] & (1 << faction);
}

/*
Determine if the specified faction is a Progenitor faction (Caretakers / Usurpers).
*/
bool is_alien(int faction) {
    return *expansion_enabled && MFactions[faction].rule_flags & RFLAG_ALIEN;
}

/*
Exclude native life since Thinker AI routines don't apply to them.
*/
bool thinker_enabled(int faction) {
    return faction > 0 && !is_human(faction) && faction <= conf.factions_enabled;
}

bool at_war(int faction1, int faction2) {
    return faction1 != faction2 && faction1 >= 0 && faction2 >= 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_VENDETTA;
}

bool has_pact(int faction1, int faction2) {
    return faction1 > 0 && faction2 > 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_PACT;
}

bool has_treaty(int faction1, int faction2, uint32_t treaty) {
    return faction1 > 0 && faction2 > 0
        && Factions[faction1].diplo_status[faction2] & treaty;
}

bool has_agenda(int faction1, int faction2, uint32_t agenda) {
    return faction1 > 0 && faction2 > 0
        && Factions[faction1].diplo_agenda[faction2] & agenda;
}

void set_treaty(int faction1, int faction2, uint32_t treaty, bool add) {
    if (add) {
        Factions[faction1].diplo_status[faction2] |= treaty;
        if (treaty & DIPLO_UNK_40) {
            Factions[faction1].diplo_merc[faction2] = 50;
        }
    } else {
        Factions[faction1].diplo_status[faction2] &= ~treaty;
    }
}

void set_agenda(int faction1, int faction2, uint32_t agenda, bool add) {
    if (add) {
        Factions[faction1].diplo_agenda[faction2] |= agenda;
    } else {
        Factions[faction1].diplo_agenda[faction2] &= ~agenda;
    }
}

bool want_revenge(int faction1, int faction2) {
    return Factions[faction1].diplo_status[faction2] & (DIPLO_ATROCITY_VICTIM | DIPLO_WANT_REVENGE);
}

bool un_charter() {
    return *un_charter_repeals <= *un_charter_reinstates;
}

bool victory_done() {
    // TODO: Check for scenario victory conditions
    return *game_state & (STATE_VICTORY_CONQUER | STATE_VICTORY_DIPLOMATIC | STATE_VICTORY_ECONOMIC)
        || has_project(-1, FAC_ASCENT_TO_TRANSCENDENCE);
}

bool valid_player(int faction) {
    return faction > 0 && faction < MaxPlayerNum;
}

bool valid_triad(int triad) {
    return (triad == TRIAD_LAND || triad == TRIAD_SEA || triad == TRIAD_AIR);
}

int unused_space(int base_id) {
    BASE* base = &Bases[base_id];
    int limit_mod = (has_project(base->faction_id, FAC_ASCETIC_VIRTUES) ? 2 : 0)
        - MFactions[base->faction_id].rule_population;

    if (!has_facility(base_id, FAC_HAB_COMPLEX)) {
        return max(0, Rules->pop_limit_wo_hab_complex + limit_mod - base->pop_size);
    }
    if (!has_facility(base_id, FAC_HABITATION_DOME)) {
        return max(0, Rules->pop_limit_wo_hab_dome + limit_mod - base->pop_size);
    }
    return max(0, 24 - base->pop_size);
}

int prod_count(int faction, int prod_id, int base_skip_id) {
    assert(valid_player(faction));
    int n = 0;
    for (int i=0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction
        && base->item() == prod_id
        && i != base_skip_id) {
            n++;
        }
    }
    return n;
}

int facility_count(int faction, int facility_id) {
    assert(valid_player(faction) && facility_id >= 0);
    int n = 0;
    for (int i=0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction && has_facility(i, facility_id)) {
            n++;
        }
    }
    return n;
}

int find_hq(int faction) {
    for(int i=0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction && has_facility(i, FAC_HEADQUARTERS)) {
            return i;
        }
    }
    return -1;
}

int manifold_nexus_owner() {
    for (int y=0; y < *map_axis_y; y++) {
        for (int x=y&1; x < *map_axis_x; x += 2) {
            MAP* sq = mapsq(x, y);
            /* First Manifold Nexus tile must also be visible to the owner. */
            if (sq && sq->landmarks & LM_NEXUS && sq->art_ref_id == 0) {
                return (sq->owner >= 0 && sq->is_visible(sq->owner) ? sq->owner : -1);
            }
        }
    }
    return -1;
}

int mod_veh_speed(int veh_id) {
    return veh_speed(veh_id, 0);
}

VehArmor best_armor(int faction, bool cheap) {
    int ci = ARM_NO_ARMOR;
    int cv = 4;
    for (int i=ARM_SYNTHMETAL_ARMOR; i<=ARM_RESONANCE_8_ARMOR; i++) {
        if (has_tech(faction, Armor[i].preq_tech)) {
            int val = Armor[i].defense_value;
            int cost = Armor[i].cost;
            if (cheap && (cost > 5 || cost > val))
                continue;
            int iv = val * (i >= ARM_PULSE_3_ARMOR ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return (VehArmor)ci;
}

VehWeapon best_weapon(int faction) {
    int ci = WPN_HAND_WEAPONS;
    int cv = 4;
    for (int i=WPN_LASER; i<=WPN_STRING_DISRUPTOR; i++) {
        if (has_tech(faction, Weapon[i].preq_tech)) {
            int iv = Weapon[i].offense_value *
                (i == WPN_RESONANCE_LASER || i == WPN_RESONANCE_BOLT ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return (VehWeapon)ci;
}

VehReactor best_reactor(int faction) {
    for (VehReactor r : {REC_SINGULARITY, REC_QUANTUM, REC_FUSION}) {
        if (has_tech(faction, Reactor[r - 1].preq_tech)) {
            return r;
        }
    }
    return REC_FISSION;
}

int offense_value(UNIT* u) {
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_type);
    if (u->weapon_type == WPN_CONVENTIONAL_PAYLOAD) {
        int wpn = best_weapon(*active_faction);
        return max(1, Weapon[wpn].offense_value * w);
    }
    return Weapon[u->weapon_type].offense_value * w;
}

int defense_value(UNIT* u) {
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_type);
    return Armor[u->armor_type].defense_value * w;
}

int faction_might(int faction) {
    Faction* f = &Factions[faction];
    return plans[faction].mil_strength + 4*f->pop_total;
}

bool allow_expand(int faction) {
    int bases = 0;
    if (*game_rules & RULES_SCN_NO_COLONY_PODS || !has_weapon(faction, WPN_COLONY_MODULE)
    || *total_num_bases >= MaxBaseNum * 19 / 20) {
        return false;
    }
    for (int i=1; i < MaxPlayerNum && conf.expansion_autoscale > 0; i++) {
        if (is_human(i)) {
            bases = Factions[i].base_count;
            break;
        }
    }
    if (conf.expansion_limit > 0) {
        return Factions[faction].base_count < max(bases, conf.expansion_limit);
    }
    return true;
}

uint32_t map_hash(uint32_t a, uint32_t b) {
    uint32_t h = (*map_random_seed ^ (a << 8) ^ (a << 24) ^ b ^ (b << 16)) * 2654435761;
    return (h ^ (h>>16));
}

uint32_t pair_hash(uint32_t a, uint32_t b) {
    uint32_t h = (a ^ b ^ (b << 16)) * 2654435761;
    return (h ^ (h>>16));
}

void random_seed(uint32_t value) {
    random_state = value;
}

int random(int n) {
    // Produces same values than the game engine function random(0, n)
    random_state = 1664525 * random_state + 1013904223;
    return ((random_state & 0xffff) * n) >> 16;
}

int wrap(int a) {
    if (!(*map_toggle_flat & 1)) {
        return (a < 0 ? a + *map_axis_x : a % *map_axis_x);
    } else {
        return a;
    }
}

int map_range(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!(*map_toggle_flat & 1) && dx > *map_half_x) {
        dx = *map_axis_x - dx;
    }
    return (dx + dy)/2;
}

int vector_dist(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!(*map_toggle_flat & 1) && dx > *map_half_x) {
        dx = *map_axis_x - dx;
    }
    return max(dx, dy) - ((((dx + dy) / 2) - min(dx, dy) + 1) / 2);
}

int min_range(const Points& S, int x, int y) {
    int z = MaxMapW;
    for (auto& p : S) {
        z = min(z, map_range(x, y, p.x, p.y));
    }
    return z;
}

int min_vector(const Points& S, int x, int y) {
    int z = MaxMapW;
    for (auto& p : S) {
        z = min(z, vector_dist(x, y, p.x, p.y));
    }
    return z;
}

double avg_range(const Points& S, int x, int y) {
    int n = 0;
    int sum = 0;
    for (auto& p : S) {
        sum += map_range(x, y, p.x, p.y);
        n++;
    }
    return (n > 0 ? (double)sum/n : 0);
}

MAP* mapsq(int x, int y) {
    if (x >= 0 && y >= 0 && x < *map_axis_x && y < *map_axis_y && !((x + y)&1)) {
        return &((*MapPtr)[ x/2 + (*map_half_x) * y ]);
    } else {
        return NULL;
    }
}

int unit_in_tile(MAP* sq) {
    if (!sq || (sq->val2 & 0xf) == 0xf) {
        return -1;
    }
    return sq->val2 & 0xf;
}

int region_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    return sq ? sq->region : 0;
}

int set_move_to(int veh_id, int x, int y) {
    VEH* veh = &Vehicles[veh_id];
    debug("set_move_to %2d %2d -> %2d %2d %s\n", veh->x, veh->y, x, y, veh->name());
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->move_status = ORDER_MOVE_TO;
    veh->status_icon = 'G';
    veh->terraforming_turns = 0;
    mapnodes.erase({x, y, NODE_PATROL});
    mapnodes.erase({x, y, NODE_COMBAT_PATROL});
    if (veh->x == x && veh->y == y) {
        return mod_veh_skip(veh_id);
    }
    pm_target[x][y]++;
    return SYNC;
}

int set_move_next(int veh_id, int offset) {
    VEH* veh = &Vehicles[veh_id];
    int x = wrap(veh->x + BaseOffsetX[offset]);
    int y = veh->y + BaseOffsetY[offset];
    return set_move_to(veh_id, x, y);
}

int set_road_to(int veh_id, int x, int y) {
    VEH* veh = &Vehicles[veh_id];
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->move_status = ORDER_ROAD_TO;
    veh->status_icon = 'R';
    return SYNC;
}

int set_action(int veh_id, int act, char icon) {
    VEH* veh = &Vehicles[veh_id];
    if (act == ORDER_THERMAL_BOREHOLE) {
        mapnodes.insert({veh->x, veh->y, NODE_BOREHOLE});
    } else if (act == ORDER_TERRAFORM_UP) {
        mapnodes.insert({veh->x, veh->y, NODE_RAISE_LAND});
    } else if (act == ORDER_SENSOR_ARRAY) {
        mapnodes.insert({veh->x, veh->y, NODE_SENSOR_ARRAY});
    }
    veh->move_status = act;
    veh->status_icon = icon;
    veh->state &= ~VSTATE_UNK_10000;
    return SYNC;
}

int set_convoy(int veh_id, int res) {
    VEH* veh = &Vehicles[veh_id];
    mapnodes.insert({veh->x, veh->y, NODE_CONVOY});
    veh->type_crawling = res-1;
    veh->move_status = ORDER_CONVOY;
    veh->status_icon = 'C';
    return veh_skip(veh_id);
}

int set_board_to(int veh_id, int trans_veh_id) {
    VEH* veh = &Vehicles[veh_id];
    VEH* v2 = &Vehicles[trans_veh_id];
    assert(veh_id != trans_veh_id);
    assert(veh->x == v2->x && veh->y == v2->y);
    assert(cargo_capacity(trans_veh_id) > 0);
    veh->move_status = ORDER_SENTRY_BOARD;
    veh->waypoint_1_x = trans_veh_id;
    veh->waypoint_1_y = 0;
    veh->status_icon = 'L';
    debug("set_board_to %2d %2d %s -> %s\n", veh->x, veh->y, veh->name(), v2->name());
    return SYNC;
}

int cargo_loaded(int veh_id) {
    int n=0;
    for (int i=0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->move_status == ORDER_SENTRY_BOARD && veh->waypoint_1_x == veh_id) {
            n++;
        }
    }
    return n;
}

int mod_veh_skip(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    veh->waypoint_1_x = veh->x;
    veh->waypoint_1_y = veh->y;
    veh->status_icon = '-';
    if (veh->need_monolith()) {
        MAP* sq = mapsq(veh->x, veh->y);
        if (sq && sq->items & BIT_MONOLITH) {
            monolith(veh_id);
        }
    }
    return veh_skip(veh_id);
}

int mod_veh_kill(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    debug("disband %d %d %s\n", veh->x, veh->y, veh->name());
    return veh_kill(veh_id);
}

bool is_ocean(MAP* sq) {
    return (!sq || (sq->climate >> 5) < ALT_SHORE_LINE);
}

bool is_ocean(BASE* base) {
    return is_ocean(mapsq(base->x, base->y));
}

bool is_ocean_shelf(MAP* sq) {
    return (sq && (sq->climate >> 5) == ALT_OCEAN_SHELF);
}

bool is_shore_level(MAP* sq) {
    return (sq && (sq->climate >> 5) == ALT_SHORE_LINE);
}

bool has_transport(int x, int y, int faction) {
    assert(valid_player(faction));
    for (int i = 0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction && veh->x == x && veh->y == y
        && veh->weapon_type() == WPN_TROOP_TRANSPORT) {
            return true;
        }
    }
    return false;
}

bool has_defenders(int x, int y, int faction) {
    assert(valid_player(faction));
    for (int i = 0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction && veh->x == x && veh->y == y
        && (veh->is_combat_unit() || veh->is_armored())
        && veh->triad() == TRIAD_LAND) {
            return true;
        }
    }
    return false;
}

bool has_colony_pods(int faction) {
    for (int i = 0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction && veh->is_colony()) {
            return true;
        }
    }
    return false;
}

int nearby_items(int x, int y, int range, uint32_t item) {
    assert(range >= 0 && range <= MaxTableRange);
    MAP* sq;
    int n = 0;
    for (int i = 0; i < TableRange[range]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if ((sq = mapsq(x2, y2)) && sq->items & item) {
            n++;
        }
    }
    return n;
}

int nearby_tiles(int x, int y, int type, int limit) {
    MAP* sq;
    int n = 0;
    TileSearch ts;
    ts.init(x, y, type, 2);
    while (n < limit && (sq = ts.get_next()) != NULL) {
        n++;
    }
    return n;
}

int set_base_facility(int base_id, int facility_id, bool add) {
    assert(base_id >= 0 && facility_id > 0 && facility_id <= FAC_EMPTY_FACILITY_64);
    if (add) {
        Bases[base_id].facilities_built[facility_id/8] |= (1 << (facility_id % 8));
    } else {
        Bases[base_id].facilities_built[facility_id/8] &= ~(1 << (facility_id % 8));
    }
    return 0;
}

char* parse_str(char* buf, int len, const char* s1, const char* s2, const char* s3, const char* s4) {
    buf[0] = '\0';
    strncat(buf, s1, len);
    if (s2) {
        strncat(buf, " ", 2);
        strncat(buf, s2, len);
    }
    if (s3) {
        strncat(buf, " ", 2);
        strncat(buf, s3, len);
    }
    if (s4) {
        strncat(buf, " ", 2);
        strncat(buf, s4, len);
    }
    // strlen count does not include the first null character.
    if (strlen(buf) > 0 && strlen(buf) < (size_t)len) {
        return buf;
    }
    return NULL;
}

char* strstrip(char* s) {
    size_t size;
    char* end;
    size = strlen(s);
    if (!size) {
        return s;
    }
    end = s + size - 1;
    while (end >= s && isspace(*end)) {
        end--;
    }
    *(end + 1) = '\0';
    while (*s && isspace(*s)) {
        s++;
    }
    return s;
}

/*
For debugging purposes only, check if the address range is unused.
*/
void check_zeros(int* ptr, int len) {
    char buf[100];
    if (DEBUG && !(*(byte*)ptr == 0 && memcmp((byte*)ptr, (byte*)ptr + 1, len - 1) == 0)) {
        snprintf(buf, sizeof(buf), "Non-zero values detected at: 0x%06x", (int)ptr);
        MessageBoxA(0, buf, "Debug notice", MB_OK | MB_ICONINFORMATION);
        int* p = ptr;
        for (int i=0; i*sizeof(int) < (unsigned)len; i++, p++) {
            debug("LOC %08x %d: %08x\n", (int)p, i, *p);
        }
    }
}

void print_map(int x, int y) {
    MAP* m = mapsq(x, y);
    debug("MAP %3d %3d owner: %2d bonus: %d reg: %3d cont: %3d clim: %02x val2: %02x val3: %02x "\
        "vis: %02x unk1: %02x unk2: %02x art: %02x items: %08x lm: %04x\n",
        x, y, m->owner, bonus_at(x, y), m->region, m->contour, m->climate, m->val2, m->val3,
        m->visibility, m->unk_1, m->unk_2, m->art_ref_id,
        m->items, m->landmarks);
}

void print_veh(int id) {
    VEH* v = &Vehicles[id];
    int speed = veh_speed(id, 0);
    debug("VEH %24s %3d %3d %d order: %2d %c %3d %3d -> %3d %3d moves: %2d speed: %2d damage: %d "
        "state: %08x flags: %04x vis: %02x mor: %d iter: %d angle: %d\n",
        Units[v->unit_id].name, v->unit_id, id, v->faction_id,
        v->move_status, (v->status_icon ? v->status_icon : ' '),
        v->x, v->y, v->waypoint_1_x, v->waypoint_1_y, speed - v->road_moves_spent, speed,
        v->damage_taken, v->state, v->flags, v->visibility, v->morale,
        v->iter_count, v->rotate_angle);
}

void print_base(int id) {
    BASE* base = &Bases[id];
    int prod = base->item();
    debug("[ turn: %d faction: %d base: %2d x: %2d y: %2d "\
        "pop: %d tal: %d dro: %d spe: %d min: %2d acc: %2d | %08x | %3d %s | %s ]\n",
        *current_turn, base->faction_id, id, base->x, base->y,
        base->pop_size, base->talent_total, base->drone_total, base->specialist_total,
        base->mineral_surplus, base->minerals_accumulated,
        base->state_flags, prod, prod_name(prod), (char*)&(base->name));
}

int __cdecl spawn_veh(int unit_id, int faction, int x, int y) {
    int id = veh_init(unit_id, faction, x, y);
    if (id >= 0) {
        Vehicles[id].home_base_id = -1;
        // Set these flags to disable any non-Thinker unit automation.
        Vehicles[id].state |= VSTATE_UNK_40000;
        Vehicles[id].state &= ~VSTATE_UNK_2000;
    }
    return id;
}

/*
Original Offset: 005C89A0
*/
int __cdecl game_year(int n) {
    return Rules->normal_start_year + n;
}

/*
Purpose: Calculate the offset and bitmask for the specified input.
Original Offset: 0050BA00
Return Value: n/a
*/
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask) {
    *offset = input / 8;
    *mask = 1 << (input & 7);
}

int __cdecl neural_amplifier_bonus(int value) {
    return value * conf.neural_amplifier_bonus / 100;
}

int __cdecl dream_twister_bonus(int value) {
    return value * conf.dream_twister_bonus / 100;
}

int __cdecl fungal_tower_bonus(int value) {
    return value * conf.fungal_tower_bonus / 100;
}

/*
Purpose: Calculate the psi combat factor for an attacking or defending unit.
Original Offset: 00501500
*/
int __cdecl psi_factor(int value, int faction_id, bool is_attack, bool is_fungal_twr) {
    int rule_psi = MFactions[faction_id].rule_psi;
    if (rule_psi) {
        value = ((rule_psi + 100) * value) / 100;
    }
    if (is_attack) {
        if (has_project(faction_id, FAC_DREAM_TWISTER)) {
            value += value * conf.dream_twister_bonus / 100;
        }
    } else {
        if (has_project(faction_id, FAC_NEURAL_AMPLIFIER)) {
            value += value * conf.neural_amplifier_bonus / 100;
        }
        if (is_fungal_twr) {
            value += value * conf.fungal_tower_bonus / 100;
        }
    }
    return value;
}

/*
Purpose: Calculate facility maintenance cost taking into account faction bonus facilities.
Vanilla game calculates Command Center maintenance based on the current highest reactor level
which is unnecessary to implement here.
Original Offset: 004F6510
*/
int __cdecl fac_maint(int facility_id, int faction_id) {
    CFacility& facility = Facility[facility_id];
    MFaction& meta = MFactions[faction_id];

    for (int i=0; i < meta.faction_bonus_count; i++) {
        if (meta.faction_bonus_val1[i] == facility_id
        && (meta.faction_bonus_id[i] == FCB_FREEFAC
        || (meta.faction_bonus_id[i] == FCB_FREEFAC_PREQ
        && has_tech(faction_id, facility.preq_tech)))) {
            return 0;
        }
    }
    return facility.maint;
}

/*
Purpose: Calculate nutrient/mineral cost factors for base production.
In vanilla game mechanics, if the player faction is ranked first, then the AIs will get
additional growth/industry bonuses. This modified version removes them.
Original Offset: 004E4430
*/
int __cdecl mod_cost_factor(int faction_id, int is_mineral, int base_id) {
    int value;
    int multiplier = (is_mineral ? Rules->mineral_cost_multi : Rules->nutrient_cost_multi);

    if (is_human(faction_id)) {
        value = multiplier;
    } else {
        value = multiplier * CostRatios[*diff_level] / 10;
    }

    if (*MapSizePlanet == 0) {
        value = 8 * value / 10;
    } else if (*MapSizePlanet == 1) {
        value = 9 * value / 10;
    }
    if (is_mineral) {
        if (is_mineral == 1) {
            switch (Factions[faction_id].SE_industry_pending) {
                case -7:
                case -6:
                case -5:
                case -4:
                case -3:
                    return (13 * value + 9) / 10;
                case -2:
                    return (6 * value + 4) / 5;
                case -1:
                    return (11 * value + 9) / 10;
                case 0:
                    break;
                case 1:
                    return (9 * value + 9) / 10;
                case 2:
                    return (4 * value + 4) / 5;
                case 3:
                    return (7 * value + 9) / 10;
                case 4:
                    return (3 * value + 4) / 5;
                default:
                    return (value + 1) / 2;
            }
        }
        return value;
    } else {
        int growth = Factions[faction_id].SE_growth_pending;
        if (base_id >= 0) {
            if (has_facility(base_id, FAC_CHILDREN_CRECHE)) {
                growth += 2;
            }
            if (Bases[base_id].state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
                growth += 2;
            }
        }
        return (value * (10 - clamp(growth, -2, 5)) + 9) / 10;
    }
}

/*
Calculate unit upgrade cost in mineral rows. This version modifies crawler upgrade costs.
*/
int __cdecl mod_upgrade_cost(int faction, int new_unit_id, int old_unit_id) {
    UNIT* old_unit = &Units[old_unit_id];
    UNIT* new_unit = &Units[new_unit_id];
    int modifier = new_unit->cost;

    if (old_unit->is_supply()) {
        return 4*max(1, new_unit->cost - old_unit->cost);
    }
    if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
        modifier = ((new_unit->cost + 1) / 2 + new_unit->cost)
            * ((new_unit->cost + 1) / 2 + new_unit->cost + 1);
    }
    int cost = max(0, new_unit->speed() - old_unit->speed())
        + max(0, new_unit->armor_cost() - old_unit->armor_cost())
        + max(0, new_unit->weapon_cost() - old_unit->weapon_cost())
        + modifier;
    if (has_project(faction, FAC_NANO_FACTORY)) {
        cost /= 2;
    }
    return cost;
}

/*
Purpose: Determine if the tile has a resource bonus.
Original Offset: 00592030
Return Value: 0 (no bonus), 1 (nutrient), 2 (mineral), 3 (energy)
*/
int __cdecl mod_bonus_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    uint32_t bit = sq->items;
    uint32_t alt = sq->climate >> 5;
    bool has_rsc_bonus = bit & BIT_BONUS_RES;
    if (!has_rsc_bonus && (!*map_random_seed || (alt >= ALT_SHORE_LINE
    && !conf.rare_supply_pods && !(*game_rules & RULES_NO_UNITY_SCATTERING)))) {
        return 0;
    }
    int avg = (x + y) >> 1;
    x -= avg;
    int chk = (avg & 3) + 4 * (x & 3);
    if (!has_rsc_bonus && chk != ((*map_random_seed + (-5 * (avg >> 2)) - 3 * (x >> 2)) & 0xF)) {
        return 0;
    }
    if (alt < ALT_OCEAN_SHELF) {
        return 0;
    }
    int ret = (alt < ALT_SHORE_LINE && !conf.rare_supply_pods) ? chk % 3 + 1 : (chk % 5) & 3;
    if (!ret || bit & BIT_NUTRIENT_RES) {
        if (bit & BIT_ENERGY_RES) {
            return 3; // energy
        }
        return ((bit & BIT_MINERAL_RES) != 0) + 1; // nutrient or mineral
    }
    return ret;
}

/*
Purpose: Determine if the tile has a supply pod and if so what type.
Original Offset: 00592140
Return Value: 0 (no supply pod), 1 (standard supply pod), 2 (unity pod?)
*/
int __cdecl mod_goody_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    uint32_t bit = sq->items;
    if (bit & (BIT_SUPPLY_REMOVE | BIT_MONOLITH)) {
        return 0; // nothing, supply pod already opened or monolith
    }
    if (*game_rules & RULES_NO_UNITY_SCATTERING) {
        return (bit & (BIT_UNK_4000000 | BIT_UNK_8000000)) ? 2 : 0; // ?
    }
    if (bit & BIT_SUPPLY_POD) {
        return 1; // supply pod
    }
    if (!*map_random_seed) {
        return 0; // nothing
    }
    int avg = (x + y) >> 1;
    int x_diff = x - avg;
    int cmp = (avg & 3) + 4 * (x_diff & 3);
    if (!conf.rare_supply_pods && !is_ocean(sq)) {
        if (cmp == ((-5 * (avg >> 2) - 3 * (x_diff >> 2) + *map_random_seed) & 0xF)) {
            return 2;
        }
    }
    return cmp == ((11 * (avg / 4) + 61 * (x_diff / 4) + *map_random_seed + 8) & 0x1F); // 0 or 1
}

/*
Original Offset: 005C1760
*/
int __cdecl cargo_capacity(int veh_id) {
    VEH* v = &Vehicles[veh_id];
    UNIT* u = &Units[v->unit_id];
    if (u->carry_capacity > 0 && veh_id < MaxProtoFactionNum
    && Weapon[u->weapon_type].offense_value < 0) {
        return v->morale + 1;
    }
    return u->carry_capacity;
}

/*
Original Offset: 005C0DB0
*/
bool __cdecl can_arty(int unit_id, bool allow_sea_arty) {
    UNIT& u = Units[unit_id];
    if ((Weapon[u.weapon_type].offense_value <= 0 // PSI + non-combat
    || Armor[u.armor_type].defense_value < 0) // PSI
    && unit_id != BSC_SPORE_LAUNCHER) { // Spore Launcher exception
        return false;
    }
    if (u.triad() == TRIAD_SEA) {
        return allow_sea_arty;
    }
    if (u.triad() == TRIAD_AIR) {
        return false;
    }
    return has_abil(unit_id, ABL_ARTILLERY); // TRIAD_LAND
}

bool ignore_goal(int type) {
    return type == AI_GOAL_COLONIZE || type == AI_GOAL_TERRAFORM_LAND
        || type == AI_GOAL_TERRAFORM_WATER || type == AI_GOAL_CONDENSER
        || type == AI_GOAL_THERMAL_BOREHOLE || type == AI_GOAL_ECHELON_MIRROR
        || type == AI_GOAL_SENSOR_ARRAY || type == AI_GOAL_DEFEND;
}

/*
Original Offset: 00579A30
*/
void __cdecl add_goal(int faction, int type, int priority, int x, int y, int base_id) {
    if (!mapsq(x, y)) {
        return;
    }
    if (thinker_enabled(faction) && type < Thinker_Goal_ID_First) {
        return;
    }
    if (conf.debug_verbose) {
        debug("add_goal %d type: %3d pr: %2d x: %3d y: %3d base: %d\n",
            faction, type, priority, x, y, base_id);
    }
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.x == x && goal.y == y && goal.type == type) {
            if (goal.priority <= priority) {
                goal.priority = (int16_t)priority;
            }
            return;
        }
    }
    int goal_score = 0, goal_id = -1;
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];

        if (goal.type < 0 || goal.priority < priority) {
            int cmp = goal.type >= 0 ? 0 : 1000;
            if (!cmp) {
                cmp = goal.priority > 0 ? 20 - goal.priority : goal.priority + 100;
            }
            if (cmp > goal_score) {
                goal_score = cmp;
                goal_id = i;
            }
        }
    }
    if (goal_id >= 0) {
        Goal& goal = Factions[faction].goals[goal_id];
        goal.type = (int16_t)type;
        goal.priority = (int16_t)priority;
        goal.x = x;
        goal.y = y;
        goal.base_id = base_id;
    }
}

/*
Original Offset: 00579B70
*/
void __cdecl add_site(int faction, int type, int priority, int x, int y) {
    if ((x ^ y) & 1 && *game_state & STATE_DEBUG_MODE) {
        debug("Bad SITE %d %d %d\n", x, y, type);
    }
    if (thinker_enabled(faction)) {
        return;
    }
    if (conf.debug_verbose) {
        debug("add_site %d type: %3d pr: %2d x: %3d y: %3d\n",
            faction, type, priority, x, y);
    }
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& sites = Factions[faction].sites[i];
        if (sites.x == x && sites.y == y && sites.type == type) {
            if (sites.priority <= priority) {
                sites.priority = (int16_t)priority;
            }
            return;
        }
    }
    int priority_search = 0;
    int site_id = -1;
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& sites = Factions[faction].sites[i];
        int type_cmp = sites.type;
        int priority_cmp = sites.priority;
        if (type_cmp < 0 || priority_cmp < priority) {
            int cmp = type_cmp >= 0 ? 0 : 1000;
            if (!cmp) {
                cmp = 20 - priority_cmp;
            }
            if (cmp > priority_search) {
                priority_search = cmp;
                site_id = i;
            }
        }
    }
    if (site_id >= 0) {
        Goal &sites = Factions[faction].sites[site_id];
        sites.type = (int16_t)type;
        sites.priority = (int16_t)priority;
        sites.x = x;
        sites.y = y;
        add_goal(faction, type, priority, x, y, -1);
    }
}

/*
Original Offset: 0x579D80
*/
void __cdecl wipe_goals(int faction) {
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        // Mod: instead of always deleting decrement priority each turn
        if (goal.priority > 0) {
            goal.priority--;
        }
        if (goal.priority <= 0) {
            goal.type = AI_GOAL_UNUSED;
        }
    }
    for (int i = 0; i < MaxSitesNum; i++) {
        Goal& site = Factions[faction].sites[i];
        int16_t type = site.type;
        if (type >= 0) {
            add_goal(faction, type, site.priority, site.x, site.y, -1);
        }
    }
}

int has_goal(int faction, int type, int x, int y) {
    assert(valid_player(faction) && mapsq(x, y));
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.priority > 0 && goal.x == x && goal.y == y && goal.type == type) {
            return goal.priority;
        }
    }
    return 0;
}

Goal* find_priority_goal(int faction, int type, int* px, int* py) {
    int pp = 0;
    *px = -1;
    *py = -1;
    Goal* value = NULL;
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.type == type && goal.priority > pp && mapsq(goal.x, goal.y)) {
            value = &goal;
            pp = goal.priority;
            *px = goal.x;
            *py = goal.y;
        }
    }
    return value;
}

std::vector<MapTile> iterate_tiles(int x, int y, size_t start_index, size_t end_index) {
    std::vector<MapTile> tiles;
    assert(start_index < end_index && end_index <= sizeof(TableOffsetX)/sizeof(int));

    for (size_t i = start_index; i < end_index; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            tiles.push_back({x2, y2, sq});
        }
    }
    return tiles;
}

void TileSearch::reset() {
    type = 0;
    head = 0;
    tail = 0;
    roads = 0;
    y_skip = 0;
    faction = -1;
    oldtiles.clear();
}

void TileSearch::add_start(int x, int y) {
    assert(!((x + y)&1));
    assert(type >= TS_TRIAD_LAND && type <= MaxTileSearchType);
    if (tail < QueueSize/2 && (sq = mapsq(x, y)) && !oldtiles.count({x, y})) {
        paths[tail] = {x, y, 0, -1};
        oldtiles.insert({x, y});
        if (!is_ocean(sq)) {
            if (sq->items & (BIT_ROAD | BIT_BASE_IN_TILE)) {
                roads |= BIT_ROAD | BIT_BASE_IN_TILE;
            }
            if (sq->items & (BIT_RIVER | BIT_BASE_IN_TILE)) {
                roads |= BIT_RIVER | BIT_BASE_IN_TILE;
            }
        }
        tail++;
    }
}

void TileSearch::init(int x, int y, int tp) {
    reset();
    type = tp;
    add_start(x, y);
}

void TileSearch::init(int x, int y, int tp, int skip) {
    reset();
    type = tp;
    y_skip = skip;
    add_start(x, y);
}

void TileSearch::init(const PointList& points, TSType tp, int skip) {
    reset();
    type = tp;
    y_skip = skip;
    for (auto& p : points) {
        add_start(p.x, p.y);
    }
}

void TileSearch::get_route(PointList& pp) {
    pp.clear();
    int i = 0;
    int j = current;
    while (j >= 0 && ++i < PathLimit) {
        auto& p = paths[j];
        j = p.prev;
        pp.push_front({p.x, p.y});
    }
}

/*
Traverse current search path and check for zones of control.
*/
bool TileSearch::has_zoc(int faction_id) {
    bool prev_zoc = false;
    int i = 0;
    int j = current;
    while (j >= 0 && ++i < PathLimit) {
        auto& p = paths[j];
        j = p.prev;
        if (zoc_any(p.x, p.y, faction_id)) {
            if (prev_zoc) return true;
            prev_zoc = true;
        } else {
            prev_zoc = false;
        }
    }
    return false;
}

/*
Implement a breadth-first search of adjacent map tiles to iterate possible movement paths.
Pathnodes are also used to keep track of the route to reach the current destination.
*/
MAP* TileSearch::get_next() {
    while (head < tail) {
        bool first = !head;
        rx = paths[head].x;
        ry = paths[head].y;
        dist = paths[head].dist;
        current = head;
        head++;
        if (!(sq = mapsq(rx, ry))) {
            continue;
        }
        if (first) {
            faction = unit_in_tile(sq);
            if (faction < 0) {
                faction = sq->owner;
            }
        }
        bool our_base = sq->is_base() && (faction == sq->owner || has_pact(faction, sq->owner));
        bool skip = (sq->is_base() && !our_base) ||
                    (type == TS_TRIAD_LAND && is_ocean(sq)) ||
                    (type == TS_TRIAD_SEA && !is_ocean(sq) && !our_base) ||
                    (type == TS_SEA_AND_SHORE && !is_ocean(sq) && !our_base) ||
                    (type == TS_NEAR_ROADS && (is_ocean(sq) || !(sq->items & roads))) ||
                    (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction)) ||
                    (type == TS_TERRITORY_BORDERS && (is_ocean(sq) || sq->owner != faction));
        if (dist > 0 && skip) {
            if ((type == TS_NEAR_ROADS && is_ocean(sq))
            || (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction))
            || (type == TS_TRIAD_LAND && is_ocean(sq))
            || (type == TS_TRIAD_SEA && !is_ocean(sq))) {
                continue;
            }
            return sq;
        }
        for (const int* t : NearbyTiles) {
            int x2 = wrap(rx + t[0]);
            int y2 = ry + t[1];
            if (y2 >= y_skip && y2 < *map_axis_y - y_skip
            && x2 >= 0 && x2 < *map_axis_x
            && tail < QueueSize && dist < PathLimit
            && !oldtiles.count({x2, y2})) {
                paths[tail] = {x2, y2, dist+1, current};
                tail++;
                oldtiles.insert({x2, y2});
            }
        }
        if (dist > 0) {
            return sq;
        }
    }
    return NULL;
}



