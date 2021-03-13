
#include "game.h"


char* prod_name(int prod) {
    if (prod >= 0) {
        return (char*)&(Units[prod].name);
    } else {
        return (char*)(Facility[-prod].name);
    }
}

int mineral_cost(int faction, int prod) {
    if (prod >= 0) {
        return Units[prod].cost * cost_factor(faction, 1, -1);
    } else {
        return Facility[-prod].cost * cost_factor(faction, 1, -1);
    }
}

bool has_tech(int faction, int tech) {
    assert(faction > 0 && faction < 8);
    if (tech == TECH_None) {
        return true;
    }
    return tech >= 0 && TechOwners[tech] & (1 << faction);
}

bool has_ability(int faction, int abl) {
    return has_tech(faction, Ability[abl].preq_tech);
}

bool has_chassis(int faction, int chs) {
    return has_tech(faction, Chassis[chs].preq_tech);
}

bool has_weapon(int faction, int wpn) {
    return has_tech(faction, Weapon[wpn].preq_tech);
}

bool has_terra(int faction, int act, bool ocean) {
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
    assert(faction != 0 && id >= PROJECT_ID_FIRST);
    int i = SecretProjects[id-PROJECT_ID_FIRST];
    return i >= 0 && (faction < 0 || Bases[i].faction_id == faction);
}

bool has_facility(int base_id, int id) {
    assert(base_id >= 0 && id > 0 && id <= FAC_EMPTY_SP_64);
    if (id >= PROJECT_ID_FIRST) {
        return SecretProjects[id-PROJECT_ID_FIRST] == base_id;
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

bool can_build(int base_id, int id) {
    assert(base_id >= 0 && id > 0 && id <= FAC_EMPTY_SP_64);
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
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
    if ((id == FAC_RECREATION_COMMONS || id == FAC_HOLOGRAM_THEATRE)
    && has_project(faction, FAC_TELEPATHIC_MATRIX)) {
        return false;
    }
    if ((id == FAC_HAB_COMPLEX || id == FAC_HABITATION_DOME) && base->nutrient_surplus < 2) {
        return false;
    }
    if (id >= PROJECT_ID_FIRST && id <= PROJECT_ID_LAST) {
        if (SecretProjects[id-PROJECT_ID_FIRST] != PROJECT_UNBUILT
        || *game_rules & RULES_SCN_NO_BUILDING_SP) {
            return false;
        }
    }
    if (id == FAC_VOICE_OF_PLANET && ~*game_rules & RULES_VICTORY_TRANSCENDENCE) {
        return false;
    }
    if (id == FAC_ASCENT_TO_TRANSCENDENCE) {
        return has_project(-1, FAC_VOICE_OF_PLANET)
            && !has_project(-1, FAC_ASCENT_TO_TRANSCENDENCE)
            && *game_rules & RULES_VICTORY_TRANSCENDENCE;
    }
    if (id == FAC_SUBSPACE_GENERATOR) {
        if (base->pop_size < Rules->base_size_subspace_gen) {
            return false;
        }
        int n = 0;
        for (int i=0; i<*total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && has_facility(i, FAC_SUBSPACE_GENERATOR)
            && b->pop_size >= Rules->base_size_subspace_gen
            && ++n >= Rules->subspace_gen_req) {
                return false;
            }
        }
    }
    if (id >= FAC_SKY_HYDRO_LAB && id <= FAC_ORBITAL_DEFENSE_POD) {
        int n = prod_count(faction, -id, base_id);
        if ((id == FAC_SKY_HYDRO_LAB && f->satellites_nutrient + n >= conf.max_sat)
        || (id == FAC_ORBITAL_POWER_TRANS && f->satellites_energy + n >= conf.max_sat)
        || (id == FAC_NESSUS_MINING_STATION && f->satellites_mineral + n >= conf.max_sat)
        || (id == FAC_ORBITAL_DEFENSE_POD && f->satellites_ODP + n >= conf.max_sat)) {
            return false;
        }
    }
    /* Rare special case if the game engine reaches the global unit limit. */
    if (id == FAC_STOCKPILE_ENERGY) {
        return (*current_turn + base_id) % 4 > 0 || !can_build_unit(faction, -1);
    }
    return has_tech(faction, Facility[id].preq_tech) && !has_facility(base_id, id);
}

bool can_build_unit(int faction, int id) {
    assert(faction > 0 && faction < 8 && id >= -1);
    UNIT* u = &Units[id];
    if (id >= 0 && id < 64 && u->preq_tech != TECH_None && !has_tech(faction, u->preq_tech)) {
        return false;
    }
    return *total_num_vehicles < 15 * MaxVehNum / 16;
}

bool is_human(int faction) {
    return *human_players & (1 << faction);
}

bool is_alien(int faction) {
    return MFactions[faction].rule_flags & FACT_ALIEN;
}

bool ai_faction(int faction) {
    /* Exclude native life since Thinker AI routines don't apply to them. */
    return faction > 0 && !(*human_players & (1 << faction));
}

bool ai_enabled(int faction) {
    return ai_faction(faction) && faction <= conf.factions_enabled;
}

bool at_war(int faction1, int faction2) {
    return faction1 != faction2 && faction1 >= 0 && faction2 >= 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_VENDETTA;
}

bool has_pact(int faction1, int faction2) {
    return faction1 > 0 && faction2 > 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_PACT;
}

bool un_charter() {
    return *un_charter_repeals <= *un_charter_reinstates;
}

int prod_count(int faction, int id, int skip) {
    assert(faction > 0 && faction < 8);
    int n = 0;
    for (int i=0; i<*total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction && base->queue_items[0] == id && i != skip) {
            n++;
        }
    }
    return n;
}

int facility_count(int faction, int facility) {
    assert(faction > 0 && faction < 8 && facility >= 0);
    int n = 0;
    for (int i=0; i<*total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction && has_facility(i, facility)) {
            n++;
        }
    }
    return n;
}

int find_hq(int faction) {
    for(int i=0; i<*total_num_bases; i++) {
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

int unit_triad(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    int triad = Chassis[Units[unit_id].chassis_type].triad;
    assert(triad == TRIAD_LAND || triad == TRIAD_SEA || triad == TRIAD_AIR);
    return triad;
}

int unit_speed(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    return Chassis[Units[unit_id].chassis_type].speed;
}

int best_armor(int faction, bool cheap) {
    int ci = ARM_NO_ARMOR;
    int cv = 4;
    for (int i=ARM_SYNTHMETAL_ARMOR; i<=ARM_RESONANCE_8_ARMOR; i++) {
        if (has_tech(faction, Armor[i].preq_tech)) {
            int val = Armor[i].defense_value;
            int cost = Armor[i].cost;
            if (cheap && (cost > 6 || cost > val))
                continue;
            int iv = val * (i >= ARM_PULSE_3_ARMOR ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return ci;
}

int best_weapon(int faction) {
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
    return ci;
}

int best_reactor(int faction) {
    for (const int r : {REC_SINGULARITY, REC_QUANTUM, REC_FUSION}) {
        if (has_tech(faction, Reactor[r - 1].preq_tech)) {
            return r;
        }
    }
    return REC_FISSION;
}

int offense_value(UNIT* u) {
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_type);
    if (u->weapon_type == WPN_CONVENTIONAL_PAYLOAD) {
        return Factions[*active_faction].best_weapon_value * w * 4/5;
    }
    return Weapon[u->weapon_type].offense_value * w;
}

int defense_value(UNIT* u) {
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_type);
    return Armor[u->armor_type].defense_value * w;
}

int faction_might(int faction) {
    Faction* f = &Factions[faction];
    return max(1, f->mil_strength_1 + f->mil_strength_2 + f->pop_total);
}

int random(int n) {
    return (n > 0 ? rand() % n : 0);
}

int map_hash(int x, int y) {
    return ((*map_random_seed ^ x) * 127) ^ (y * 179);
}

int wrap(int a) {
    if (!*map_toggle_flat) {
        return (a < 0 ? a + *map_axis_x : a % *map_axis_x);
    } else {
        return a;
    }
}

int map_range(int x1, int y1, int x2, int y2) {
    int xd = abs(x1-x2);
    int yd = abs(y1-y2);
    if (!*map_toggle_flat & 1 && xd > *map_axis_x/2) {
        xd = *map_axis_x - xd;
    }
    return (xd + yd)/2;
}

int vector_dist(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!(*map_toggle_flat & 1 || dx <= *map_half_x)) {
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
    assert(!((x + y)&1));
    if (x >= 0 && y >= 0 && x < *map_axis_x && y < *map_axis_y && !((x + y)&1)) {
        return &((*MapPtr)[ x/2 + (*map_half_x) * y ]);
    } else {
        return NULL;
    }
}

int unit_in_tile(MAP* sq) {
    if (!sq || (sq->flags & 0xf) == 0xf) {
        return -1;
    }
    return sq->flags & 0xf;
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
    if (veh->x == x && veh->y == y) {
        return mod_veh_skip(veh_id);
    }
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
    for (int i=0; i<*total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->move_status == ORDER_SENTRY_BOARD && veh->waypoint_1_x == veh_id) {
            n++;
        }
    }
    return n;
}

/*
Original Offset: 005C1760
*/
int cargo_capacity(int veh_id) {
    VEH* v = &Vehicles[veh_id];
    UNIT* u = &Units[v->unit_id];
    if (u->carry_capacity > 0 && veh_id < MaxProtoFactionNum 
    && Weapon[u->weapon_type].offense_value < 0) {
        return v->morale + 1;
    }
    return u->carry_capacity;
}

int mod_veh_skip(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    veh->waypoint_1_x = veh->x;
    veh->waypoint_1_y = veh->y;
    veh->status_icon = '-';
    if (veh->damage_taken) {
        MAP* sq = mapsq(veh->x, veh->y);
        if (sq && sq->items & TERRA_MONOLITH) {
            monolith(veh_id);
        }
    }
    return veh_skip(veh_id);
}

bool at_target(VEH* veh) {
    return veh->move_status == ORDER_NONE || (veh->waypoint_1_x < 0 && veh->waypoint_1_y < 0)
        || (veh->x == veh->waypoint_1_x && veh->y == veh->waypoint_1_y);
}

bool is_ocean(MAP* sq) {
    return (!sq || (sq->level >> 5) < LEVEL_SHORE_LINE);
}

bool is_ocean(BASE* base) {
    return is_ocean(mapsq(base->x, base->y));
}

bool is_ocean_shelf(MAP* sq) {
    return (sq && (sq->level >> 5) == LEVEL_OCEAN_SHELF);
}

bool is_shore_level(MAP* sq) {
    return (sq && (sq->level >> 5) == LEVEL_SHORE_LINE);
}

bool has_defenders(int x, int y, int faction) {
    assert(faction > 0 && faction < 8);
    for (int i=0; i<*total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        UNIT* u = &Units[veh->unit_id];
        if (veh->faction_id == faction && veh->x == x && veh->y == y
        && u->weapon_type <= WPN_PSI_ATTACK && unit_triad(veh->unit_id) == TRIAD_LAND) {
            return true;
        }
    }
    return false;
}

int nearby_items(int x, int y, int range, uint32_t item) {
    MAP* sq;
    int n = 0;
    for (int i=-range*2; i<=range*2; i++) {
        for (int j=-range*2 + abs(i); j<=range*2 - abs(i); j+=2) {
            sq = mapsq(wrap(x + i), y + j);
            if (sq && sq->items & item) {
                n++;
            }
        }
    }
    return n;
}

int bases_in_range(int x, int y, int range) {
    return nearby_items(x, y, range, TERRA_BASE_IN_TILE);
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

int coast_tiles(int x, int y) {
    MAP* sq;
    int n = 0;
    for (const int* t : NearbyTiles) {
        sq = mapsq(wrap(x + t[0]), y + t[1]);
        if (sq && is_ocean(sq)) {
            n++;
        }
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

int spawn_veh(int unit_id, int faction, int x, int y, int base_id) {
    int id = veh_init(unit_id, faction, x, y);
    if (id >= 0) {
        Vehicles[id].home_base_id = base_id;
        // Set these flags to disable any non-Thinker unit automation.
        Vehicles[id].state |= VSTATE_UNK_40000;
        Vehicles[id].state &= ~VSTATE_UNK_2000;
    }
    return id;
}

char* parse_str(char* buf, int len, const char* s1, const char* s2, const char* s3, const char* s4) {
    buf[0] = '\0';
    strncat(buf, s1, len);
    strncat(buf, s2, len);
    strncat(buf, s3, len);
    strncat(buf, s4, len);
    // strlen count does not include the first null character.
    if (strlen(buf) > 0 && strlen(buf) < (size_t)len) {
        return buf;
    }
    return NULL;
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
    debug("MAP %2d %2d owner: %2d bonus: %d level: %02x alt: %02x rocks: %02x "\
          "flags: %02x reg: %02x vis: %02x unk1: %02x unk2: %02x art: %02x items: %08x lm: %08x\n",
        x, y, m->owner, bonus_at(x, y), m->level, m->altitude, m->rocks,
        m->flags, m->region, m->visibility, m->unk_1, m->unk_2, m->art_ref_id,
        m->items, m->landmarks);
}

void print_veh(int id) {
    VEH* v = &Vehicles[id];
    int speed = veh_speed(id, 0);
    debug("VEH %22s %3d %3d %d order: %2d %c %3d %3d -> %3d %3d moves: %2d speed: %2d damage: %d "
          "state: %08x flags: %04x vis: %02x mor: %d iter: %d prb: %d sbt: %d\n",
        Units[v->unit_id].name, v->unit_id, id, v->faction_id,
        v->move_status, (v->status_icon ? v->status_icon : ' '),
        v->x, v->y, v->waypoint_1_x, v->waypoint_1_y, speed - v->road_moves_spent, speed,
        v->damage_taken, v->state, v->flags, v->visibility, v->morale,
        v->iter_count, v->probe_action, v->probe_sabotage_id);
}

void print_base(int id) {
    BASE* base = &Bases[id];
    int prod = base->queue_items[0];
    debug("[ turn: %d faction: %d base: %2d x: %2d y: %2d "\
        "pop: %d tal: %d dro: %d min: %2d acc: %2d | %08x | %3d %s | %s ]\n",
        *current_turn, base->faction_id, id, base->x, base->y,
        base->pop_size, base->talent_total, base->drone_total,
        base->mineral_surplus, base->minerals_accumulated,
        base->status_flags, prod, prod_name(prod), (char*)&(base->name));
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
    int triad = unit_triad(unit_id);
    if (triad == TRIAD_SEA) {
        return allow_sea_arty;
    }
    if (triad == TRIAD_AIR) {
        return false;
    }
    return has_abil(unit_id, ABL_ARTILLERY); // TRIAD_LAND
}

/*
Goal types that are entirely redundant for Thinker AIs.
*/
bool ignore_goal(int type) {
    return type == AI_GOAL_COLONIZE || type == AI_GOAL_TERRAFORM_LAND
        || type == AI_GOAL_TERRAFORM_WATER || type == AI_GOAL_CONDENSER
        || type == AI_GOAL_THERMAL_BOREHOLE || type == AI_GOAL_ECHELON_MIRROR
        || type == AI_GOAL_SENSOR_ARRAY || type == AI_GOAL_DEFEND;
}

/*
Original Offset: 0x579A30
*/
void __cdecl add_goal(int faction, int type, int priority, int x, int y, int base_id) {
    if (!mapsq(x, y)) {
        return;
    }
    if (ai_enabled(faction) && type < 200) {
        return;
    }
    debug("add_goal %d type: %3d pr: %2d x: %3d y: %3d base: %d\n",
        faction, type, priority, x, y, base_id);

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
    assert(faction < MaxPlayerNum && mapsq(x, y));
    for (int i = 0; i < MaxGoalsNum; i++) {
        Goal& goal = Factions[faction].goals[i];
        if (goal.priority > 0 && goal.x == x && goal.y == y && goal.type == type) {
            return goal.priority;
        }
    }
    return 0;
}

std::vector<MapTile> iterate_tiles(int x, int y, int start_index, int end_index) {
    std::vector<MapTile> tiles;
    assert(start_index < end_index && (unsigned)end_index <= sizeof(TableOffsetX)/sizeof(int));

    for (int i = start_index; i < end_index; i++) {
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
    owner = 0;
    roads = 0;
    y_skip = 0;
    oldtiles.clear();
}

void TileSearch::add_start(int x, int y) {
    if (tail < QueueSize/2 && (sq = mapsq(x, y)) && !oldtiles.count({x, y})) {
        owner = sq->owner;
        paths[tail] = {x, y, 0, -1};
        oldtiles.insert({x, y});
        if (!is_ocean(sq)) {
            if (sq->items & (TERRA_ROAD | TERRA_BASE_IN_TILE)) {
                roads |= TERRA_ROAD | TERRA_BASE_IN_TILE;
            }
            if (sq->items & (TERRA_RIVER | TERRA_BASE_IN_TILE)) {
                roads |= TERRA_RIVER | TERRA_BASE_IN_TILE;
            }
        }
        tail++;
    }
}

void TileSearch::init(int x, int y, int tp) {
    assert(tp >= TS_TRIAD_LAND && tp <= MaxTileSearchType);
    reset();
    type = tp;
    add_start(x, y);
}

void TileSearch::init(int x, int y, int tp, int skip) {
    init(x, y, tp);
    y_skip = skip;
}

void TileSearch::init(const PointList& points, int tp, int skip) {
    reset();
    type = tp;
    y_skip = skip;
    for (auto p : points) {
        add_start(p.x, p.y);
    }
}

PointList& TileSearch::get_route(PointList& pp) {
    pp.clear();
    int i = 0;
    int j = cur;
    while (j >= 0 && ++i < PathLimit) {
        auto p = paths[j];
        j = p.prev;
        pp.push_front({p.x, p.y});
    }
    return pp;
}

/*
Traverse current search path and check for zones of control.
*/
bool TileSearch::has_zoc(int faction) {
    int zoc = 0;
    int i = 0;
    int j = cur;
    while (j >= 0 && ++i < PathLimit) {
        auto p = paths[j];
        if (zoc_any(p.x, p.y, faction) && ++zoc > 1) {
            return true;
        }
        j = p.prev;
    }
    return false;
}

/*
Implement a breadth-first search of adjacent map tiles to iterate possible movement paths.
Pathnodes are also used to keep track of the route to reach the current destination.
*/
MAP* TileSearch::get_next() {
    while (head < tail) {
        bool first = paths[head].dist == 0;
        rx = paths[head].x;
        ry = paths[head].y;
        dist = paths[head].dist;
        cur = head;
        head++;
        if (!(sq = mapsq(rx, ry))) {
            continue;
        }
        bool skip = (type == TS_TRIAD_LAND && is_ocean(sq)) ||
                    (type == TS_TRIAD_SEA && !is_ocean(sq)) ||
                    (type == TS_NEAR_ROADS && (is_ocean(sq) || !(sq->items & roads))) ||
                    (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != owner)) ||
                    (type == TS_TERRITORY_BORDERS && (is_ocean(sq) || sq->owner != owner)) ||
                    (type == TS_SEA_AND_SHORE && !is_ocean(sq));
        if (!first && skip) {
            if ((type == TS_NEAR_ROADS && !is_ocean(sq))
            || type == TS_TERRITORY_BORDERS
            || type == TS_SEA_AND_SHORE) {
                return sq;
            }
            continue;
        }
        for (const int* t : NearbyTiles) {
            int x2 = wrap(rx + t[0]);
            int y2 = ry + t[1];
            if (y2 >= y_skip && y2 < *map_axis_y - y_skip
            && tail < QueueSize && dist < PathLimit
            && !oldtiles.count({x2, y2})) {
                paths[tail] = {x2, y2, dist+1, cur};
                tail++;
                oldtiles.insert({x2, y2});
            }
        }
        if (!first) {
            return sq;
        }
    }
    return NULL;
}



