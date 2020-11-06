#pragma once

#include "main.h"
#include "game.h"

HOOK_API int tech_rate(int faction);
HOOK_API int tech_selection(int faction);
HOOK_API int tech_value(int tech, int faction, int flag);

