#pragma once

#include "main.h"

bool locate_landmark(int* x, int* y, bool ocean);
void __cdecl mod_world_monsoon(int x, int y);
void __cdecl mod_world_borehole(int x, int y);
void __cdecl mod_world_fossil(int x, int y);
void __cdecl mod_world_build();
void console_world_generate(uint32_t seed);
void world_generate(uint32_t seed);

