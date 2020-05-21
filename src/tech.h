#pragma once

#include "main.h"
#include "game.h"

#define THINKER_HEADER (short)0xACAC

void init_save_game(int faction);
HOOK_API int tech_rate(int faction);
HOOK_API int tech_selection(int faction);
HOOK_API int tech_value(int tech, int faction, int flag);

