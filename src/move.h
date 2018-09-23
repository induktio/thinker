#ifndef __MOVE_H__
#define __MOVE_H__

#include "main.h"
#include "game.h"

#define IMP_SIMPLE (TERRA_FARM | TERRA_MINE | TERRA_FOREST)
#define IMP_ADVANCED (TERRA_CONDENSER | TERRA_THERMAL_BORE)

void move_upkeep();
int former_move(int id);

#endif // __MOVE_H__
