
#include "game.h"


char* prod_name(int prod) {
    if (prod >= 0)
        return (char*)&(tx_units[prod].name);
    else
        return (char*)(tx_facility[-prod].name);
}

int mineral_cost(int faction, int prod) {
    if (prod >= 0)
        return tx_units[prod].cost * tx_cost_factor(faction, 1, -1);
    else
        return tx_facility[-prod].cost * tx_cost_factor(faction, 1, -1);
}

bool has_tech(int faction, int tech) {
    assert(faction > 0 && faction < 8);
    if (tech == TECH_None)
        return true;
    return tech >= 0 && tx_tech_discovered[tech] & (1 << faction);
}

bool has_ability(int faction, int abl) {
    return has_tech(faction, tx_ability[abl].preq_tech);
}

bool has_chassis(int faction, int chs) {
    return has_tech(faction, tx_chassis[chs].preq_tech);
}

bool has_weapon(int faction, int wpn) {
    return has_tech(faction, tx_weapon[wpn].preq_tech);
}

bool has_terra(int faction, int act, bool ocean) {
    int preq_tech = (ocean ? tx_terraform[act].preq_tech_sea : tx_terraform[act].preq_tech);
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
    int i = tx_secret_projects[id-PROJECT_ID_FIRST];
    return i >= 0 && (faction < 0 || tx_bases[i].faction_id == faction);
}

bool has_facility(int base_id, int id) {
    assert(base_id >= 0 && id > 0 && id <= FAC_EMPTY_SP_64);
    if (id >= PROJECT_ID_FIRST) {
        return tx_secret_projects[id-PROJECT_ID_FIRST] == base_id;
    }
    int faction = tx_bases[base_id].faction_id;
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
    return tx_bases[base_id].facilities_built[id/8] & (1 << (id % 8));
}

bool can_build(int base_id, int id) {
    assert(base_id >= 0 && id > 0 && id <= FAC_EMPTY_SP_64);
    BASE* base = &tx_bases[base_id];
    int faction = base->faction_id;
    Faction* f = &tx_factions[faction];
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
        if (tx_secret_projects[id-PROJECT_ID_FIRST] != PROJECT_UNBUILT
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
        if (base->pop_size < tx_basic->base_size_subspace_gen)
            return false;
        int n = 0;
        for (int i=0; i<*total_num_bases; i++) {
            BASE* b = &tx_bases[i];
            if (b->faction_id == faction && has_facility(i, FAC_SUBSPACE_GENERATOR)
            && b->pop_size >= tx_basic->base_size_subspace_gen
            && ++n >= tx_basic->subspace_gen_req)
                return false;
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
    return has_tech(faction, tx_facility[id].preq_tech) && !has_facility(base_id, id);
}

bool can_build_unit(int faction, int id) {
    assert(faction > 0 && faction < 8 && id >= -1);
    UNIT* u = &tx_units[id];
    if (id >= 0 && id < 64 && u->preq_tech != TECH_None && !has_tech(faction, u->preq_tech)) {
        return false;
    }
    return *total_num_vehicles < 2000;
}

bool is_human(int faction) {
    return *human_players & (1 << faction);
}

bool is_alien(int faction) {
    return tx_metafactions[faction].rule_flags & FACT_ALIEN;
}

bool ai_faction(int faction) {
    /* Exclude native life since Thinker AI routines don't apply to them. */
    return faction > 0 && !(*human_players & (1 << faction));
}

bool ai_enabled(int faction) {
    return ai_faction(faction) && faction <= conf.factions_enabled;
}

bool at_war(int faction1, int faction2) {
    if (faction1 == faction2 || faction1 < 0 || faction2 < 0)
        return false;
    return !faction1 || !faction2
        || tx_factions[faction1].diplo_status[faction2] & DIPLO_VENDETTA;
}

bool un_charter() {
    return *un_charter_repeals <= *un_charter_reinstates;
}

int prod_count(int faction, int id, int skip) {
    int n = 0;
    for (int i=0; i<*total_num_bases; i++) {
        BASE* base = &tx_bases[i];
        if (base->faction_id == faction && base->queue_items[0] == id && i != skip) {
            n++;
        }
    }
    return n;
}

int find_hq(int faction) {
    for(int i=0; i<*total_num_bases; i++) {
        BASE* base = &tx_bases[i];
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
                return (sq->owner >= 0 && sq->visibility & (1 << sq->owner) ? sq->owner : -1);
            }
        }
    }
    return -1;
}

int veh_triad(int id) {
    return unit_triad(tx_vehicles[id].proto_id);
}

int veh_speed(int id) {
    return tx_veh_speed(id, 0);
}

int unit_triad(int id) {
    int triad = tx_chassis[tx_units[id].chassis_type].triad;
    assert(triad == TRIAD_LAND || triad == TRIAD_SEA || triad == TRIAD_AIR);
    return triad;
}

int unit_speed(int id) {
    return tx_chassis[tx_units[id].chassis_type].speed;
}

int best_armor(int faction, bool cheap) {
    int ci = ARM_NO_ARMOR;
    int cv = 4;
    for (int i=ARM_SYNTHMETAL_ARMOR; i<=ARM_RESONANCE_8_ARMOR; i++) {
        if (has_tech(faction, tx_defense[i].preq_tech)) {
            int val = tx_defense[i].defense_value;
            int cost = tx_defense[i].cost;
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
        if (has_tech(faction, tx_weapon[i].preq_tech)) {
            int iv = tx_weapon[i].offense_value *
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
        if (has_tech(faction, tx_reactor[r - 1].preq_tech)) {
            return r;
        }
    }
    return REC_FISSION;
}

int offense_value(UNIT* u) {
    if (u->weapon_type == WPN_CONVENTIONAL_PAYLOAD)
        return tx_factions[*active_faction].best_weapon_value * u->reactor_type;
    return tx_weapon[u->weapon_type].offense_value * u->reactor_type;
}

int defense_value(UNIT* u) {
    return tx_defense[u->armor_type].defense_value * u->reactor_type;
}

int random(int n) {
    return (n != 0 ? rand() % n : 0);
}

int map_hash(int x, int y) {
    return ((*map_random_seed ^ x) * 127) ^ (y * 179);
}

double lerp(double a, double b, double t) {
    return a + t*(b-a);
}

int wrap(int a) {
    if (!*map_toggle_flat)
        return (a < 0 ? a + *map_axis_x : a % *map_axis_x);
    else
        return a;
}

int map_range(int x1, int y1, int x2, int y2) {
    int xd = abs(x1-x2);
    int yd = abs(y1-y2);
    if (!*map_toggle_flat && xd > *map_axis_x/2)
        xd = *map_axis_x - xd;
    return (xd + yd)/2;
}

int point_range(const Points& S, int x, int y) {
    int z = MAPSZ;
    for (auto& p : S) {
        z = min(z, map_range(x, y, p.first, p.second));
    }
    return z;
}

double mean_range(const Points& S, int x, int y) {
    int n = 0;
    int sum = 0;
    for (auto& p : S) {
        sum += map_range(x, y, p.first, p.second);
        n++;
    }
    return (n > 0 ? (double)sum/n : 0);
}

MAP* mapsq(int x, int y) {
    assert((x + y)%2 == 0);
    if (x >= 0 && y >= 0 && x < *map_axis_x && y < *map_axis_y) {
        int i = x/2 + (*map_half_x) * y;
        return &((*tx_map_ptr)[i]);
    } else {
        return NULL;
    }
}

int unit_in_tile(MAP* sq) {
    if (!sq || (sq->flags & 0xf) == 0xf)
        return -1;
    return sq->flags & 0xf;
}

int set_move_to(int id, int x, int y) {
    VEH* veh = &tx_vehicles[id];
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->move_status = ORDER_MOVE_TO;
    veh->status_icon = 'G';
    return SYNC;
}

int set_road_to(int id, int x, int y) {
    VEH* veh = &tx_vehicles[id];
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->move_status = ORDER_ROAD_TO;
    veh->status_icon = 'R';
    return SYNC;
}

int set_action(int id, int act, char icon) {
    VEH* veh = &tx_vehicles[id];
    if (act == ORDER_THERMAL_BOREHOLE)
        boreholes.insert({veh->x, veh->y});
    veh->move_status = act;
    veh->status_icon = icon;
    veh->state &= 0xFFFEFFFF;
    return SYNC;
}

int set_convoy(int id, int res) {
    VEH* veh = &tx_vehicles[id];
    convoys.insert({veh->x, veh->y});
    veh->type_crawling = res-1;
    veh->move_status = ORDER_CONVOY;
    veh->status_icon = 'C';
    return tx_veh_skip(id);
}

int veh_skip(int id) {
    VEH* veh = &tx_vehicles[id];
    veh->waypoint_1_x = veh->x;
    veh->waypoint_1_y = veh->y;
    veh->status_icon = '-';
    if (veh->damage_taken) {
        MAP* sq = mapsq(veh->x, veh->y);
        if (sq && sq->items & TERRA_MONOLITH) {
            tx_monolith(id);
        }
    }
    return tx_veh_skip(id);
}

bool at_target(VEH* veh) {
    return veh->move_status == ORDER_NONE || (veh->waypoint_1_x < 0 && veh->waypoint_1_y < 0)
        || (veh->x == veh->waypoint_1_x && veh->y == veh->waypoint_1_y);
}

bool is_ocean(MAP* sq) {
    return (!sq || (sq->level >> 5) < LEVEL_SHORE_LINE);
}

bool is_ocean_shelf(MAP* sq) {
    return (sq && (sq->level >> 5) == LEVEL_OCEAN_SHELF);
}

bool is_sea_base(int id) {
    MAP* sq = mapsq(tx_bases[id].x, tx_bases[id].y);
    return is_ocean(sq);
}

bool workable_tile(int x, int y, int faction) {
    assert(faction > 0 && faction < 8);
    MAP* sq = mapsq(x, y);
    if (!sq || ~sq->items & TERRA_BASE_RADIUS) {
        return false;
    }
    for (int i=0; i<20; i++) {
        sq = mapsq(wrap(x + offset_tbl[i][0]), y + offset_tbl[i][1]);
        if (sq && sq->owner == faction && sq->items & TERRA_BASE_IN_TILE) {
            return true;
        }
    }
    return false;
}

bool has_defenders(int x, int y, int faction) {
    assert(faction > 0 && faction < 8);
    for (int i=0; i<*total_num_vehicles; i++) {
        VEH* veh = &tx_vehicles[i];
        UNIT* u = &tx_units[veh->proto_id];
        if (veh->faction_id == faction && veh->x == x && veh->y == y
        && u->weapon_type <= WPN_PSI_ATTACK && unit_triad(veh->proto_id) == TRIAD_LAND) {
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
    for (const int* t : offset) {
        sq = mapsq(wrap(x + t[0]), y + t[1]);
        if (sq && is_ocean(sq)) {
            n++;
        }
    }
    return n;
}

int spawn_veh(int unit_id, int faction, int x, int y, int base_id) {
    int id = tx_veh_init(unit_id, faction, x, y);
    if (id >= 0) {
        tx_vehicles[id].home_base_id = base_id;
        // Set these flags to disable any non-Thinker unit automation.
        tx_vehicles[id].state |= VSTATE_UNK_40000;
        tx_vehicles[id].state &= ~VSTATE_UNK_2000;
    }
    return id;
}

char* parse_str(char* buf, int len, const char* s1, const char* s2, const char* s3, const char* s4) {
    buf[0] = '\0';
    strcat_s(buf, len, s1);
    strcat_s(buf, len, s2);
    strcat_s(buf, len, s3);
    strcat_s(buf, len, s4);
    return (strlen(buf) > 0 ? buf : NULL);
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
    debug("MAP %2d %2d | %2d %d | %02x %02x %02x | %02x %02x %02x | %02x %02x %02x | %08x %08x\n",
        x, y, m->owner, tx_bonus_at(x, y), m->level, m->altitude, m->rocks,
        m->flags, m->region, m->visibility, m->unk_1, m->unk_2, m->art_ref_id,
        m->items, m->landmarks);
}

void print_veh(int id) {
    VEH* v = &tx_vehicles[id];
    debug("VEH %22s %3d %3d %d | %08x %04x %02x | %2d %3d | %2d %2d %2d %2d | %2d %2d %d %d %d %d\n",
        tx_units[v->proto_id].name, v->proto_id, id, v->faction_id,
        v->state, v->flags, v->visibility, v->move_status, v->status_icon,
        v->x, v->y, v->waypoint_1_x, v->waypoint_1_y,
        v->morale, v->damage_taken, v->iter_count, v->unk_1, v->probe_action, v->probe_sabotage_id);
}

void print_base(int id) {
    BASE* base = &tx_bases[id];
    int prod = base->queue_items[0];
    debug("[ turn: %d faction: %d base: %2d x: %2d y: %2d "\
        "pop: %d tal: %d dro: %d mins: %2d acc: %2d | %08x | %3d %s | %s ]\n",
        *current_turn, base->faction_id, id, base->x, base->y,
        base->pop_size, base->talent_total, base->drone_total,
        base->mineral_surplus, base->minerals_accumulated,
        base->status_flags, prod, prod_name(prod), (char*)&(base->name));
}

void TileSearch::init(int x, int y, int tp) {
    assert(tp == TRIAD_LAND || tp == TRIAD_SEA || tp == TRIAD_AIR
        || tp == NEAR_ROADS || tp == TERRITORY_LAND);
    head = 0;
    tail = 0;
    items = 0;
    roads = 0;
    owner = 0;
    y_skip = 0;
    type = tp;
    oldtiles.clear();
    sq = mapsq(x, y);
    if (sq) {
        items++;
        tail = 1;
        owner = sq->owner;
        newtiles[0] = {x, y, 0, -1};
        oldtiles.insert({x, y});
        if (!is_ocean(sq)) {
            if (sq->items & (TERRA_ROAD | TERRA_BASE_IN_TILE))
                roads |= TERRA_ROAD;
            if (sq->items & (TERRA_RIVER | TERRA_BASE_IN_TILE))
                roads |= TERRA_RIVER;
        }
    }
}

void TileSearch::init(int x, int y, int tp, int skip) {
    init(x, y, tp);
    y_skip = skip;
}

/*
Traverse current search path and check for zones of control.
*/
bool TileSearch::has_zoc(int faction) {
    int zoc = 0;
    int i = 0;
    int j = cur;
    while (j >= 0 && i++ < 20) {
        auto p = newtiles[j];
        if (tx_zoc_any(p.x, p.y, faction) && ++zoc > 1)
            return true;
        j = p.prev;
    }
    return false;
}

/*
Implement a breadth-first search of adjacent map tiles to iterate possible movement paths.
Pathnodes are also used to keep track of the route to reach the current destination.
*/
MAP* TileSearch::get_next() {
    while (items > 0) {
        bool first = oldtiles.size() == 1;
        cur = head;
        rx = newtiles[head].x;
        ry = newtiles[head].y;
        dist = newtiles[head].dist;
        head = (head + 1) % QSIZE;
        items--;
        if (!(sq = mapsq(rx, ry)))
            continue;
        bool skip = (type == TRIAD_LAND && is_ocean(sq)) ||
                    (type == TRIAD_SEA && !is_ocean(sq)) ||
                    (type == NEAR_ROADS && (is_ocean(sq) || !(sq->items & roads))) ||
                    (type == TERRITORY_LAND && (is_ocean(sq) || sq->owner != owner));
        if (!first && skip) {
            if (type == NEAR_ROADS && !is_ocean(sq))
                return sq;
            continue;
        }
        for (const int* t : offset) {
            int x2 = wrap(rx + t[0]);
            int y2 = ry + t[1];
            if (y2 >= y_skip && y2 < *map_axis_y - y_skip
            && items < QSIZE && !oldtiles.count({x2, y2})) {
                newtiles[tail] = {x2, y2, dist+1, cur};
                tail = (tail + 1) % QSIZE;
                items++;
                oldtiles.insert({x2, y2});
            }
        }
        if (!first) {
            return sq;
        }
    }
    return NULL;
}



