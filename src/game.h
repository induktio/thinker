#ifndef __GAME_H__
#define __GAME_H__

#include "main.h"

const int offset[][2] = {
    {1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2}
};

const int offset_tbl[][2] = {
    {1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2},
    {2,-2},{2,2},{-2,2},{-2,-2},{1,-3},{3,-1},{3,1},{1,3},
    {-1,3},{-3,1},{-3,-1},{-1,-3},{4,0},{-4,0},{0,4},{0,-4},
    {1,-5},{2,-4},{3,-3},{4,-2},{5,-1},{5,1},{4,2},{3,3},
    {2,4},{1,5},{-1,5},{-2,4},{-3,3},{-4,2},{-5,1},{-5,-1},
    {-4,-2},{-3,-3},{-2,-4},{-1,-5},{0,6},{6,0},{0,-6},{-6,0},
    {0,-8},{1,-7},{2,-6},{3,-5},{4,-4},{5,-3},{6,-2},{7,-1},
    {8,0},{7,1},{6,2},{5,3},{4,4},{3,5},{2,6},{1,7},
    {0,8},{-1,7},{-2,6},{-3,5},{-4,4},{-5,3},{-6,2},{-7,1},
    {-8,0},{-7,-1},{-6,-2},{-5,-3},{-4,-4},{-3,-5},{-2,-6},{-1,-7},
};

char* prod_name(int prod);
int mineral_cost(int fac, int prod);
bool has_tech(int fac, int tech);
bool has_ability(int fac, int abl);
bool has_chassis(int fac, int chs);
bool has_weapon(int fac, int wpn);
bool has_terra(int fac, int act, bool ocean);
bool has_project(int fac, int id);
bool has_facility(int base_id, int id);
bool can_build(int base_id, int id);
bool can_build_unit(int fac, int id);
bool is_human(int fac);
bool ai_faction(int fac);
bool ai_enabled(int fac);
bool at_war(int a, int b);
bool un_charter();
int prod_count(int fac, int id, int skip);
int find_hq(int fac);
int veh_triad(int id);
int veh_speed(int id);
int unit_triad(int id);
int unit_speed(int id);
int best_armor(int fac, bool cheap);
int best_weapon(int fac);
int best_reactor(int fac);
int offense_value(UNIT* u);
int defense_value(UNIT* u);
int random(int n);
int map_hash(int x, int y);
int wrap(int a);
int map_range(int x1, int y1, int x2, int y2);
int point_range(const Points& S, int x, int y);
MAP* mapsq(int x, int y);
int unit_in_tile(MAP* sq);
int set_move_to(int id, int x, int y);
int set_road_to(int id, int x, int y);
int set_action(int id, int act, char icon);
int set_convoy(int id, int res);
int veh_skip(int id);
bool at_target(VEH* veh);
bool is_ocean(MAP* sq);
bool is_ocean_shelf(MAP* sq);
bool is_sea_base(int id);
bool workable_tile(int x, int y, int fac);
bool has_defenders(int x, int y, int fac);
int nearby_items(int x, int y, int range, uint32_t item);
int bases_in_range(int x, int y, int range);
int nearby_tiles(int x, int y, int type, int limit);
int coast_tiles(int x, int y);
char* parse_str(char* buf, int len, const char* s1, const char* s2, const char* s3, const char* s4);
void check_zeros(int* ptr, int len);
void print_map(int x, int y);
void print_veh(int id);
void print_base(int id);

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
    int items;
    int y_skip;
    MAP* sq;
    public:
    int rx, ry, dist, cur;
    PathNode newtiles[QSIZE];
    Points oldtiles;
    void init(int, int, int);
    void init(int, int, int, int);
    bool has_zoc(int);
    MAP* get_next();
};


#endif // __GAME_H__
