#ifndef __GAME_H__
#define __GAME_H__

#include "main.h"

#define mp(x, y) std::make_pair(x, y)

const int offset[][2] = {
    {2,0},{-2,0},{0,2},{0,-2},{1,1},{1,-1},{-1,1},{-1,-1}
};

const int offset_20[][2] = {
    {2,0},{-2,0},{0,2},{0,-2},{1,1},{1,-1},{-1,1},{-1,-1},
    {1,-3},{2,-2},{3,-1},{3,1},{2,2},{1,3},
    {-1,3},{-2,2},{-3,1},{-3,-1},{-2,-2},{-1,-3},
};

char* prod_name(int prod);
int mineral_cost(int fac, int prod);
bool knows_tech(int fac, int tech);
int unit_triad(int id);
int offense_value(UNIT* u);
int defense_value(UNIT* u);
int random(int n);
int wrap(int a, int b);
int map_range(int x1, int y1, int x2, int y2);
MAP* mapsq(int x, int y);
int unit_in_tile(MAP* sq);
bool water_base(int id);
bool convoy_not_used(int x, int y);
bool workable_tile(int x, int y, int fac);
int coast_tiles(int x, int y);

class TileSearch {
    int type;
    int head;
    int tail;
    int items;
    MAP* tile;
    int newtiles[QSIZE];
    public:
    int cur_x, cur_y;
    void init(int, int, int);
    std::set<std::pair <int, int>> oldtiles;
    int visited();
    MAP* get_next();
};


#endif // __GAME_H__
