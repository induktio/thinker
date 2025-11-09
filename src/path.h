#pragma once

#include "main.h"

const int NearbyTiles[][2] = {{1,-1}, {2,0}, {1,1}, {0,2}, {-1,1}, {-2,0}, {-1,-1}, {0,-2}};
const int BaseOffsetX[] = { 1, 2, 1, 0, -1, -2, -1,  0, 0}; // Path::find offset
const int BaseOffsetY[] = {-1, 0, 1, 2,  1,  0, -1, -2, 0}; // Path::find offset
const int MaxTableRange = 8;
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

const int PathLimit = 128;
const int QueueSize = 8192;
const int MaxTileSearchType = 6;

enum TSType {
    TS_TRIAD_LAND = 0,
    TS_TRIAD_SEA = 1,
    TS_TRIAD_AIR = 2,
    TS_SEA_AND_SHORE = 3,
    TS_TERRITORY_LAND = 4,
    TS_TERRITORY_SHORE = 5,
    TS_TERRITORY_PACT = 6,
};

enum NodesetType {
    NODE_BOREHOLE, // Borehole being built
    NODE_RAISE_LAND, // Land raise action initiated
    NODE_SENSOR_ARRAY, // Sensor being built
    NODE_NEED_FERRY,
    NODE_BASE_SITE, // Skip from search
    NODE_CONVOY_SITE, // Skip from search
    NODE_GOAL_RAISE_LAND, // Former priority only
    NODE_NAVAL_START,
    NODE_NAVAL_BEACH,
    NODE_NAVAL_END,
    NODE_NAVAL_PICK,
    NODE_SCOUT_SITE,
    NODE_PATROL, // Includes probes/transports
    NODE_COMBAT_PATROL, // Only attack-capable units
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
    MAP* sq;
    void reset();
    void add_start(int x, int y);
    public:
    int rx, ry, dist, current, faction_id, y_skip;
    PathNode paths[QueueSize];
    Points oldtiles;
    void init(int x, int y, int tp);
    void init(int x, int y, int tp, int skip);
    void init(const PointList& points, TSType tp, int skip);
    void get_route(PointList& pp);
    void adjust_roads(PMTable& tbl, int value);
    void connect_roads(PMTable& tbl, int x, int y, int pact_id);
    bool has_zoc(int pact_id);
    PathNode& get_prev();
    PathNode& get_node();
    MAP* get_next();
};

int __cdecl mod_zoc_any(int x, int y, int faction_id);
int __cdecl mod_zoc_veh(int x, int y, int faction_id);
int __cdecl mod_zoc_sea(int x, int y, int faction_id);
int __cdecl mod_zoc_move(int x, int y, int faction_id);
std::vector<MapTile> iterate_tiles(int x, int y, size_t start_index, size_t end_index);
int nearby_items(int x, int y, size_t start_index, size_t end_index, uint32_t item);
int path_get_next(int x1, int y1, int x2, int y2, int unit_id, int faction_id);
int path_distance(int x1, int y1, int x2, int y2, int unit_id, int faction_id);
int path_cost(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int max_cost);
void update_path(PMTable& tbl, int veh_id, int tx, int ty);
bool defend_tile(VEH* veh, MAP* sq);
bool safe_path(TileSearch& ts, int faction_id, bool skip_owner);
bool has_base_sites(TileSearch& ts, int x, int y, int faction_id, int triad);
int route_distance(PMTable& tbl, int x1, int y1, int x2, int y2);
int defender_goal(int x, int y, int faction_id, int triad);
int defender_count(int x, int y, int veh_skip_id);
int garrison_count(int x, int y);
int veh_cargo_loaded(int veh_id);
int cargo_capacity(int x, int y, int faction_id);
bool has_transport(int x, int y, int faction_id);
int move_to_base(int veh_id, bool ally);
int escape_move(int veh_id);
int search_escape(TileSearch& ts, int veh_id, int* tx, int* ty);
int search_base(TileSearch& ts, int veh_id, bool ally, int* tx, int* ty);
int search_route(TileSearch& ts, int veh_id, int* tx, int* ty);



