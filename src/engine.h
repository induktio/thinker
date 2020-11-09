#pragma once

#include "main.h"

#define THINKER_HEADER (short)0xACAC

void init_save_game(int faction);
HOOK_API int mod_faction_upkeep(int faction);
HOOK_API int mod_base_find3(int x, int y, int faction1, int region, int faction2, int faction3);

