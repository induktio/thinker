#pragma once

#include "main.h"

// Bottom middle UI console size in pixels
const int ConsoleHeight = 219;
const int ConsoleWidth = 1024;

LRESULT WINAPI ModWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int show_mod_menu();
int __cdecl mod_blink_timer();
void __cdecl mod_turn_timer();
int __thiscall mod_calc_dim(Console* This);
int __thiscall mod_gen_map(Console* This, int iOwner, int fUnitsOnly);
void __thiscall FileBox_init(void* This);
void __thiscall FileBox_close(void* This);
void __thiscall MapWin_gen_overlays(Console* This, int x, int y);
int __thiscall BaseWin_popup_start(
    Win* This, const char* filename, const char* label, int a4, int a5, int a6, int a7);
int __cdecl BaseWin_ask_number(const char* label, int value, int a3);
void __thiscall Basewin_draw_farm_set_font(Buffer* This, Font* font, int a3, int a4, int a5);
void __cdecl mod_base_draw(Buffer* buffer, int base_id, int x, int y, int zoom, int a6);
void __cdecl BaseWin_draw_psych_strcat(char* buffer, char* source);
void __cdecl BaseWin_action_staple(int base_id);
void __cdecl popb_action_staple(int base_id);
int __thiscall BaseWin_staple(void* This);
int __thiscall mod_MapWin_focus(Console* This, int x, int y);
int __thiscall ReportWin_close_handler(void* This);
void __thiscall Console_editor_fungus(Console* UNUSED(This));
void __cdecl mod_say_loc(char* dest, int x, int y, int a4, int a5, int a6);
void __cdecl mod_diplomacy_caption(int faction1, int faction2);
void __cdecl reset_netmsg_status();
int __thiscall mod_NetMsg_pop(void* This, char* label, int delay, int a4, void* a5);

