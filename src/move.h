#ifndef __MOVE_H__
#define __MOVE_H__

#include "main.h"
#include "game.h"

#define BASE_DISALLOWED (TERRA_BASE_IN_TILE | TERRA_MONOLITH | TERRA_FUNGUS | TERRA_THERMAL_BORE)

#define IMP_SIMPLE (TERRA_FARM | TERRA_MINE | TERRA_FOREST)
#define IMP_ADVANCED (TERRA_CONDENSER | TERRA_THERMAL_BORE)

#define PM_SAFE -20
#define PM_NEAR_SAFE -40

bool has_transport(int x, int y, int fac);
bool non_combat_move(int x, int y, int fac, int triad);
bool can_build_base(int x, int y, int fac, int triad);
bool has_base_sites(int x, int y, int fac, int type);
void move_upkeep(int fac);
int crawler_move(int id);
int colony_move(int id);
int former_move(int id);
int trans_move(int id);
int combat_move(int id);

#endif // __MOVE_H__
