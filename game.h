#ifndef __GAME_H__
#define __GAME_H__

#include "main.h"

char* prod_name(int prod);
bool knows_tech(int fac, int tech);
int unit_triad(int id);
int random(int n);
int wrap(int a, int b);
int map_range(int x1, int y1, int x2, int y2);
MAP* mapsq(int x, int y);


#endif // __GAME_H__
