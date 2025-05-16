#pragma once

#include "main.h"

// Last three values on AltDefault are increased to enable higher altitude limit
const int AltDefault[] = {0, 15, 32, 45, 60, 75, 80, 100, 120, 140, 160};
const int ElevDetail[] = {0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200};
static_assert(sizeof(AltDefault) == 44, "");
static_assert(sizeof(ElevDetail) == 44, "");

int __cdecl near_landmark(int x, int y);
bool locate_landmark(int* x, int* y, bool ocean);
void __cdecl mod_world_rocky();
void __cdecl mod_world_riverbeds();
void __cdecl mod_world_shorelines();
void __cdecl mod_world_polar_caps();
void __cdecl mod_world_linearize_contours();
void __cdecl mod_world_crater(int x, int y);
void __cdecl mod_world_monsoon(int x, int y);
void __cdecl mod_world_sargasso(int x, int y);
void __cdecl mod_world_ruin(int x, int y);
void __cdecl mod_world_dune(int x, int y);
void __cdecl mod_world_diamond(int x, int y);
void __cdecl mod_world_fresh(int x, int y);
void __cdecl mod_world_volcano(int x, int y, int skip_label);
void __cdecl mod_world_borehole(int x, int y);
void __cdecl mod_world_temple(int x, int y);
void __cdecl mod_world_unity(int x, int y);
void __cdecl mod_world_fossil(int x, int y);
void __cdecl mod_world_canyon(int x, int y);
void __cdecl mod_world_mesa(int x, int y);
void __cdecl mod_world_ridge(int x, int y);
void __cdecl mod_world_geothermal(int x, int y);
void __cdecl mod_world_build();
void console_world_generate(uint32_t seed);
void world_generate(uint32_t seed);

