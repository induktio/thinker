#ifndef __GAME_H__
#define __GAME_H__

#include "main.h"

char* prod_name(int prod);
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

class TileSearch {
    int type;
    int head;
    int tail;
    int items;
    MAP* tile;
    int newtiles[QSIZE];
    std::set<int> oldtiles;
    public:
    int cur_x, cur_y;
    void init(int, int, int);
    int visited();
    MAP* get_next();
};


#endif // __GAME_H__
