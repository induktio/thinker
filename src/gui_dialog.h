#pragma once

#include "main.h"

void __thiscall Popup_init_icon(Popup* This, const char* filename, int* px, int* py);
int __thiscall Popup_load_icon(Popup* This, Sprite* image, int a3, void* a4);
int __cdecl X_pop(const char* label, fp_none fn);
int __cdecl X_pop_2(const char* filename, const char* label, fp_none fn);
int __cdecl X_pop_6(const char* label, uint32_t flags, fp_none fn);
int __cdecl X_pops(const char* label, Sprite* image, fp_none fn);
int __cdecl X_pops_11(const char* label, uint32_t flags, Sprite* image, fp_none fn);
int __cdecl X_dialog(const char* label, int faction_id);
int __cdecl X_dialog_alt(const char* filename, const char* label, int faction_id);

