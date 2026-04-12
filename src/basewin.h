#pragma once

#include "main.h"

void __cdecl action_staple(int base_id);
void __cdecl action_sat_attack(int faction_id, int faction_id_tgt, int target_id, int base_id);
void BaseWin_support_zoom(bool zoom_in);
int __thiscall BaseWin_hurry_popup_start(
    Popup* This, const char* filename, const char* label, int a4, char* a5, int a6, GraphicWin* a7);
int __cdecl BaseWin_hurry_ask_number(const char* label, int value, fp_none fn);
void __thiscall BaseWin_hurry_unlock_base(AlphaNet* This, int base_id);
void __thiscall BaseWin_draw_support(BaseWindow* This);
void __thiscall BaseWin_draw_misc_eco_damage(Buffer* This, char* buf, int x, int y, int len);
void __thiscall BaseWin_draw_farm_set_font(Buffer* This, Font* a2, Font* a3, Font* a4, Font* a5);
void __thiscall BaseWin_draw_energy_set_text_color(Buffer* This, int a2, int a3, int a4, int a5);
void __cdecl BaseWin_draw_psych_strcat(char* buffer, char* source);
void __cdecl mod_base_draw(Buffer* buffer, int base_id, int x, int y, int zoom, int opts);
int __cdecl BaseWin_staple_popp(const char* filename, const char* label, int a3, const char* imagefile, fp_none fn);
void __cdecl BaseWin_action_staple(int base_id);
void __cdecl popb_action_staple(int base_id);
int __thiscall BaseWin_click_staple(BaseWindow* This);
int __thiscall BaseWin_gov_options(BaseWindow* This, int flag);

