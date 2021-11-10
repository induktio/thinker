#pragma once

#include "main.h"

/*
Function parameter naming conventions

Faction ID is always written as "faction". It is also written with a number suffix
(faction1, faction2, ...) if there are multiple factions handled in the same function.

Generic "id" is used to denote any parameter for base, unit, or vehicle IDs.
It is assumed the meaning of the parameter is clear from the function context.
*/

const int NearbyTiles[][2] = {
    {1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2}
};
const int BaseOffsetX[] = { 1, 2, 1, 0, -1, -2, -1,  0, 0};
const int BaseOffsetY[] = {-1, 0, 1, 2,  1,  0, -1, -2, 0};
const int TableRange[] = {1, 9, 25, 49, 81, 121, 169, 225, 289};
const int TableOffsetX[] = {
     0,  1,  2,  1,  0, -1, -2,  -1,   0,   2,   2,  -2,  -2,   1,   3,   3,   1,  -1,  -3,  -3,
    -1,  4, -4,  0,  0,  1,  2,   3,   4,   5,   5,   4,   3,   2,   1,  -1,  -2,  -3,  -4,  -5,
    -5, -4, -3, -2, -1,  0,  6,   0,  -6,   0,   1,   2,   3,   4,   5,   6,   7,   8,   7,   6,
     5,  4,  3,  2,  1,  0, -1,  -2,  -3,  -4,  -5,  -6,  -7,  -8,  -7,  -6,  -5,  -4,  -3,  -2,
    -1,  0,  1,  2,  3,  4,  5,   6,   7,   8,   9,  10,   9,   8,   7,   6,   5,   4,   3,   2,
     1,  0, -1, -2, -3, -4, -5,  -6,  -7,  -8,  -9, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,
    -1,  0,  1,  2,  3,  4,  5,   6,   7,   8,   9,  10,  11,  12,  11,  10,   9,   8,   7,   6,
     5,  4,  3,  2,  1,  0, -1,  -2,  -3,  -4,  -5,  -6,  -7,  -8,  -9, -10, -11, -12, -11, -10,
    -9, -8, -7, -6, -5, -4, -3,  -2,  -1,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,
    11, 12, 13, 14, 13, 12, 11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,  -1,  -2,
    -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -13, -12, -11, -10,  -9,  -8,  -7,  -6,
    -5, -4, -3, -2, -1,  0,  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
    15, 16, 15, 14, 13, 12, 11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,  -1,  -2,
    -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16, -15, -14, -13, -12, -11, -10,
    -9, -8, -7, -6, -5, -4, -3,  -2,  -1,
};
const int TableOffsetY[] = {
     0,  -1,   0,   1,   2,   1,   0,  -1,  -2,  -2,   2,   2,  -2,  -3, -1,  1,  3,  3,  1, -1,
    -3,   0,   0,   4,  -4,  -5,  -4,  -3,  -2,  -1,   1,   2,   3,   4,  5,  5,  4,  3,  2,  1,
    -1,  -2,  -3,  -4,  -5,   6,   0,  -6,   0,  -8,  -7,  -6,  -5,  -4, -3, -2, -1,  0,  1,  2,
     3,   4,   5,   6,   7,   8,   7,   6,   5,   4,   3,   2,   1,   0, -1, -2, -3, -4, -5, -6,
    -7, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,   2,  3,  4,  5,  6,  7,  8,
     9,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,  -1,  -2, -3, -4, -5, -6, -7, -8,
    -9, -12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,   0,  1,  2,  3,  4,  5,  6,
     7,   8,   9,  10,  11,  12,  11,  10,   9,   8,   7,   6,   5,   4,  3,  2,  1,  0, -1, -2,
    -3,  -4,  -5,  -6,  -7,  -8,  -9, -10, -11, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4,
    -3,  -2,  -1,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10, 11, 12, 13, 14, 13, 12,
    11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,  -1,  -2, -3, -4, -5, -6, -7, -8,
    -9, -10, -11, -12, -13, -16, -15, -14, -13, -12, -11, -10,  -9,  -8, -7, -6, -5, -4, -3, -2,
    -1,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12, 13, 14, 15, 16, 15, 14,
    13,  12,  11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0, -1, -2, -3, -4, -5, -6,
    -7,  -8,  -9, -10, -11, -12, -13, -14, -15,
};

char* prod_name(int item_id);
int mineral_cost(int faction, int item_id);
bool has_tech(int faction, int tech);
bool has_ability(int faction, int abl);
bool has_chassis(int faction, int chs);
bool has_weapon(int faction, int wpn);
bool has_terra(int faction, int act, bool ocean);
bool has_project(int faction, int id);
bool has_facility(int base_id, int id);
bool can_build(int base_id, int id);
bool can_build_unit(int faction, int id);
bool base_can_riot(int base_id);
bool base_pop_boom(int base_id);
bool can_use_teleport(int base_id);
bool is_human(int faction);
bool is_alive(int faction);
bool is_alien(int faction);
bool thinker_enabled(int faction);
bool at_war(int faction1, int faction2);
bool has_pact(int faction1, int faction2);
bool has_treaty(int faction1, int faction2, uint32_t treaty);
bool has_agenda(int faction1, int faction2, uint32_t agenda);
void set_treaty(int faction1, int faction2, uint32_t treaty, bool add);
void set_agenda(int faction1, int faction2, uint32_t agenda, bool add);
bool un_charter();
bool victory_done();
bool valid_player(int faction);
bool valid_triad(int triad);
int unused_space(int base_id);
int prod_count(int faction, int prod_id, int base_skip_id);
int facility_count(int faction, int facility_id);
int find_hq(int faction);
int manifold_nexus_owner();
int mod_veh_speed(int veh_id);
int best_armor(int faction, bool cheap);
int best_weapon(int faction);
int best_reactor(int faction);
int offense_value(UNIT* u);
int defense_value(UNIT* u);
int faction_might(int faction);
bool allow_expand(int faction);
uint32_t map_hash(uint32_t a, uint32_t b);
int random(int n);
int wrap(int a);
int map_range(int x1, int y1, int x2, int y2);
int vector_dist(int x1, int y1, int x2, int y2);
int min_range(const Points& S, int x, int y);
double avg_range(const Points& S, int x, int y);
MAP* mapsq(int x, int y);
int unit_in_tile(MAP* sq);
int set_move_to(int veh_id, int x, int y);
int set_move_next(int veh_id, int offset);
int set_road_to(int veh_id, int x, int y);
int set_action(int veh_id, int act, char icon);
int set_convoy(int veh_id, int res);
int set_board_to(int veh_id, int trans_veh_id);
int cargo_loaded(int veh_id);
int mod_veh_skip(int veh_id);
int mod_veh_kill(int veh_id);
bool is_ocean(MAP* sq);
bool is_ocean(BASE* base);
bool is_ocean_shelf(MAP* sq);
bool is_shore_level(MAP* sq);
bool has_defenders(int x, int y, int faction);
bool has_colony_pods(int faction);
int nearby_items(int x, int y, int range, uint32_t item);
int nearby_tiles(int x, int y, int type, int limit);
int set_base_facility(int base_id, int facility_id, bool add);
int __cdecl spawn_veh(int unit_id, int faction, int x, int y);
char* parse_str(char* buf, int len, const char* s1, const char* s2, const char* s3, const char* s4);
char* strstrip(char* s);
void check_zeros(int* ptr, int len);
void print_map(int x, int y);
void print_veh(int id);
void print_base(int id);

int __cdecl game_year(int n);
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask);
int __cdecl fac_maint(int facility_id, int faction_id);
int __cdecl mod_cost_factor(int faction_id, int is_mineral, int base_id);
int __cdecl mod_bonus_at(int x, int y);
int __cdecl mod_goody_at(int x, int y);
int __cdecl cargo_capacity(int veh_id);
bool __cdecl can_arty(int unit_id, bool allow_sea_arty);
void __cdecl add_goal(int faction, int type, int priority, int x, int y, int base_id);
void __cdecl add_site(int faction, int type, int priority, int x, int y);
void __cdecl wipe_goals(int faction);

int has_goal(int faction, int type, int x, int y);
Goal* find_priority_goal(int faction, int type, int* px, int* py);
std::vector<MapTile> iterate_tiles(int x, int y, int start_index, int end_index);

const int PathLimit = 80;
const int QueueSize = 6400;
const int MaxTileSearchType = 6;

enum tilesearch_types {
    TS_TRIAD_LAND = 0,
    TS_TRIAD_SEA = 1,
    TS_TRIAD_AIR = 2,
    TS_NEAR_ROADS = 3,
    TS_TERRITORY_LAND = 4,
    TS_TERRITORY_BORDERS = 5,
    TS_SEA_AND_SHORE = 6,
};

struct PathNode {
    int x;
    int y;
    int dist;
    int prev;
};

class TileSearch {
    int type;
    int head;
    int tail;
    int roads;
    MAP* sq;
    void reset();
    void add_start(int x, int y);
    public:
    int rx, ry, dist, current, faction, y_skip;
    PathNode paths[QueueSize];
    Points oldtiles;
    void init(int x, int y, int tp);
    void init(int x, int y, int tp, int skip);
    void init(const PointList& points, int tp, int skip);
    bool has_zoc(int faction);
    void get_route(PointList& pp);
    MAP* get_next();
};

