#pragma once

#include "main.h"

enum WinFlag {
    WIN_VISIBLE = 1,
};

enum GameWinState {GW_None, GW_World, GW_Base, GW_Design};

const uint32_t WM_WINDOWED = (WM_USER + 3);
const uint32_t WM_MOVIEOVER = (WM_USER + 6);
const uint32_t AC_WS_WINDOWED = (WS_OVERLAPPED | WS_CLIPCHILDREN);
const uint32_t AC_WS_FULLSCREEN = (WS_POPUP | WS_CLIPCHILDREN);

// Translation labels for the user interface
extern char label_pop_size[StrBufLen];
extern char label_pop_boom[StrBufLen];
extern char label_nerve_staple[StrBufLen];
extern char label_captured_base[StrBufLen];
extern char label_stockpile_energy[StrBufLen];
extern char label_sat_nutrient[StrBufLen];
extern char label_sat_mineral[StrBufLen];
extern char label_sat_energy[StrBufLen];
extern char label_eco_damage[StrBufLen];
extern char label_unit_reactor[4][StrBufLen];

// Bottom middle UI console size in pixels
const int ConsoleHeight = 219;
const int ConsoleWidth = 1024;

const int ColorNutrient = 152;
const int ColorMineral = 223;
const int ColorEnergy = 98;
const int ColorNutrientLight = 150;
const int ColorMineralLight = 222;
const int ColorEnergyLight = 95;
const int ColorIntakeSurplus = 169;
const int ColorProdName = 154;
const int ColorLabsAlloc = 155;
const int ColorPsychAlloc = 204;
const int ColorRed = 249;
const int ColorGreen = 250;
const int ColorYellow = 251;
const int ColorBlue = 252;
const int ColorMagenta = 253;
const int ColorCyan = 254;
const int ColorWhite = 255;

LRESULT WINAPI ModWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int __cdecl mod_Win_init_class(const char* lpWindowName);
void __cdecl mod_amovie_project(const char* name);
void restore_video_mode();
void set_video_mode(bool reset_window);
void set_minimised(bool minimise);
void set_windowed(bool windowed);
int show_mod_menu();
int __cdecl mod_blink_timer();
void __cdecl mod_turn_timer();
int __thiscall mod_calc_dim(Console* This);
int __thiscall mod_gen_map(Console* This, int iOwner, int fUnitsOnly);
int __thiscall Win_is_visible(Win* This);
void __thiscall MapWin_gen_overlays(Console* This, int x, int y);
void refresh_overlay(std::function<int32_t(int32_t, int32_t)> tile_value);
int __thiscall SetupWin_buffer_draw(Buffer* src, Buffer* dst, int a3, int a4, int a5, int a6, int a7);
int __thiscall SetupWin_buffer_copy(
    Buffer* src, Buffer* dst, int xSrc, int ySrc, int xDst, int yDst, int wSrc, int hSrc);
int __thiscall SetupWin_soft_update3(Win* This, int a2, int a3, int a4, int a5);
int __thiscall window_scale_load_pcx(Buffer* This, char* filename, int a3, int a4, int a5);
int __thiscall Credits_GraphicWin_init(
    Win* This, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10);
int __thiscall BaseWin_popup_start(
    Win* This, const char* filename, const char* label, int a4, int a5, int a6, int a7);
int __cdecl BaseWin_ask_number(const char* label, int value, int a3);
void __thiscall BaseWin_draw_misc_eco_damage(Buffer* This, char* buf, int x, int y, int len);
void __thiscall BaseWin_draw_support(BaseWindow* This);
void __thiscall Basewin_draw_farm_set_font(Buffer* This, Font* font, int a3, int a4, int a5);
void __thiscall BaseWin_draw_energy_set_text_color(Buffer* This, int a2, int a3, int a4, int a5);
void __cdecl mod_base_draw(Buffer* buffer, int base_id, int x, int y, int zoom, int opts);
void __cdecl BaseWin_draw_psych_strcat(char* buffer, char* source);
void __cdecl BaseWin_action_staple(int base_id);
void __cdecl popb_action_staple(int base_id);
int __thiscall BaseWin_click_staple(Win* This);
int __thiscall mod_MapWin_focus(Console* This, int x, int y);
int __thiscall mod_MapWin_set_center(Console* This, int x, int y, int flag);
int __thiscall ReportWin_close_handler(void* This);
void __thiscall Console_editor_fungus(Console* UNUSED(This));
void __cdecl mod_say_loc(char* dest, int x, int y, int a4, int a5, int a6);
void __cdecl mod_diplomacy_caption(int faction1, int faction2);
void __cdecl reset_netmsg_status();
int __thiscall mod_NetMsg_pop(void* This, const char* label, int delay, int a4, const char* a5);
int __thiscall mod_BasePop_start(
    void* This, const char* filename, const char* label, int a4, int a5, int a6, int a7);
int __cdecl mod_action_move(int veh_id, int x, int y);
int __cdecl MapWin_right_menu_arty(int veh_id, int x, int y);
void __thiscall Console_arty_cursor_on(void* This, int cursor_type, int veh_id);


