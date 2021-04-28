
#include "game.h"


char* prod_name(int item_id) {
    if (item_id >= 0) {
        return Units[item_id].name;
    } else {
        return Facility[-item_id].name;
    }
}

int mineral_cost(int faction, int item_id) {
    if (item_id >= 0) {
        return Units[item_id].cost * cost_factor(faction, 1, -1);
    } else {
        return Facility[-item_id].cost * cost_factor(faction, 1, -1);
    }
}

bool has_tech(int faction, int tech) {
    assert(valid_player(faction));
    if (tech == TECH_None) {
        return true;
    }
    return tech >= 0 && TechOwners[tech] & (1 << faction);
}

bool has_ability(int faction, int abl) {
    assert(abl >= 0 && abl <= ABL_ID_ALGO_ENHANCEMENT);
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
    assert(faction != 0 && id >= SP_ID_First && id <= FAC_EMPTY_SP_64);
    int i = SecretProjects[id - SP_ID_First];
    return i >= 0 && (faction < 0 || Bases[i].faction_id == faction);
}

bool has_facility(int base_id, int id) {
    assert(base_id >= 0 && id > 0 && id <= FAC_EMPTY_SP_64);
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
    if ((id == FAC_RECREATION_COMMONS || id == FAC_HOLOGRAM_THEATRE || id == FAC_PUNISHMENT_SPHERE)
    && !base_can_riot(base_id)) {
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
        if (!has_facility(base_id, FAC_AEROSPACE_COMPLEX)) {
            return false;
        }
        int n = prod_count(faction, -id, base_id);
        int goal = plans[faction].satellites_goal;
        if (id == FAC_ORBITAL_DEFENSE_POD) {
            goal = min(conf.max_satellites,
                goal/3 + (plans[faction].diplo_flags & DIPLO_VENDETTA ? 4 : 0));
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
        for (int i=0; i<*total_num_bases; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && i != base_id
            && map_range(base->x, base->y, b->x, b->y) <= 3
            && (has_facility(i, FAC_GEOSYNC_SURVEY_POD)
            || has_facility(i, FAC_FLECHETTE_DEFENSE_SYS)
            || b->queue_items[0] == -FAC_GEOSYNC_SURVEY_POD
            || b->queue_items[0] == -FAC_FLECHETTE_DEFENSE_SYS)) {
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

bool base_can_riot(int base_id) {
    BASE* b = &Bases[base_id];
    return !b->nerve_staple_turns_left
        && !has_project(b->faction_id, FAC_TELEPATHIC_MATRIX)
        && !has_facility(base_id, FAC_PUNISHMENT_SPHERE);
}

bool is_human(int faction) {
    return *human_players & (1 << faction);
}

bool is_alien(int faction) {
    return MFactions[faction].rule_flags & RFLAG_ALIEN;
}

bool ai_enabled(int faction) {
    /* Exclude native life since Thinker AI routines don't apply to them. */
    return faction > 0 && !(*human_players & (1 << faction))
        && faction <= conf.factions_enabled;
}

bool at_war(int faction1, int faction2) {
    return faction1 != faction2 && faction1 >= 0 && faction2 >= 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_VENDETTA;
}

bool has_pact(int faction1, int faction2) {
    return faction1 > 0 && faction2 > 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_PACT;
}

bool has_treaty(int faction1, int faction2, int treaty) {
    return faction1 > 0 && faction2 > 0
        && Factions[faction1].diplo_status[faction2] & treaty;
}

bool un_charter() {
    return *un_charter_repeals <= *un_charter_reinstates;
}

bool valid_player(int faction) {
    return faction > 0 && faction < MaxPlayerNum;
}

bool valid_triad(int triad) {
    return (triad == TRIAD_LAND || triad == TRIAD_SEA || triad == TRIAD_AIR);
}

int prod_count(int faction, int id, int base_skip_id) {
    assert(valid_player(faction));
    int n = 0;
    for (int i=0; i<*total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction
        && base->queue_items[0] == id
        && i != base_skip_id) {
            n++;
        }
    }
    return n;
}

int facility_count(int faction, int facility) {
    assert(valid_player(faction) && facility >= 0);
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
        int wpn = best_weapon(*active_faction);
        return max(1, Weapon[wpn].offense_value * w * 4/5);
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
            bases = Factions[i].current_num_bases;
            break;
        }
    }
    if (conf.expansion_limit > 0) {
        return Factions[faction].current_num_bases < max(bases, conf.expansion_limit);
    }
    return true;
}

uint32_t map_hash(uint32_t a, uint32_t b) {
    uint32_t h = (*map_random_seed ^ (a << 8) ^ (b << 16) ^ b) * 2654435761;
    return (h ^ (h>>16)) & 0x7fffffff;
}

int random(int n) {
    return (n > 0 ? rand() % n : 0);
}

int wrap(int a) {
    if (!*map_toggle_flat) {
        return (a < 0 ? a + *map_axis_x : a % *map_axis_x);
    } else {
        return a;
    }
}

int map_range(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!*map_toggle_flat & 1 && dx > *map_half_x) {
        dx = *map_axis_x - dx;
    }
    return (dx + dy)/2;
}

int vector_dist(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    if (!*map_toggle_flat & 1 && dx > *map_half_x) {
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

bool has_defenders(int x, int y, int faction) {
    assert(valid_player(faction));
    for (int i=0; i<*total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction && veh->x == x && veh->y == y
        && (veh->is_combat_unit() || veh->is_armored())
        && veh->triad() == TRIAD_LAND) {
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
    debug("MAP %2d %2d owner: %2d bonus: %d clim: %02x cont: %02x val3: %02x "\
          "val2: %02x reg: %02x vis: %02x unk1: %02x unk2: %02x art: %02x items: %08x lm: %08x\n",
        x, y, m->owner, bonus_at(x, y), m->climate, m->contour, m->val3,
        m->val2, m->region, m->visibility, m->unk_1, m->unk_2, m->art_ref_id,
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
    int prod = base->queue_items[0];
    debug("[ turn: %d faction: %d base: %2d x: %2d y: %2d "\
        "pop: %d tal: %d dro: %d min: %2d acc: %2d | %08x | %3d %s | %s ]\n",
        *current_turn, base->faction_id, id, base->x, base->y,
        base->pop_size, base->talent_total, base->drone_total,
        base->mineral_surplus, base->minerals_accumulated,
        base->state_flags, prod, prod_name(prod), (char*)&(base->name));
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
    if (ai_enabled(faction) && type < 200) { // Discard all non-Thinker goals
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
Original Offset: 00579B70
*/
void __cdecl add_site(int faction, int type, int priority, int x, int y) {
    if ((x ^ y) & 1 && *game_state & STATE_DEBUG_MODE) {
        debug("Bad SITE %d %d %d\n", x, y, type);
    }
    if (ai_enabled(faction)) {
        return;
    }
    debug("add_site %d type: %3d pr: %2d x: %3d y: %3d\n",
        faction, type, priority, x, y);

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
        int priroty_cmp = sites.priority;
        if (type_cmp < 0 || priroty_cmp < priority) {
            int cmp = type_cmp >= 0 ? 0 : 1000;
            if (!cmp) {
                cmp = 20 - priroty_cmp;
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
    init(x, y, tp);
    y_skip = skip;
}

void TileSearch::init(const PointList& points, int tp, int skip) {
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
        if (!first && skip) {
            if (!((type == TS_NEAR_ROADS && is_ocean(sq))
            || (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction))
            || (type == TS_TRIAD_LAND && is_ocean(sq))
            || (type == TS_TRIAD_SEA && !is_ocean(sq)))) {
                return sq;
            }
            continue;
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
        if (!first) {
            return sq;
        }
    }
    return NULL;
}



