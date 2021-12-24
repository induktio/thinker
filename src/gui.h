#pragma once

#include "main.h"

enum {
    PopDialogCheckbox = 0x1, // Multiple choices
    PopDialogListChoice = 0x2, // Only one choice
    PopDialogTextInput = 0x4,
    PopDialogBtnCancel = 0x40,
    PopDialogLargeWindow = 0x80, // Large square window placed on top left
    PopDialogSmallWindow = 0x400, // Narrow window
    PopDialogUnk1000 = 0x1000,
    PopDialogUnk10000 = 0x10000, // Removes portrait and ok buttons?
    PopDialogUnk40000 = 0x40000, // Removes ok buttons?
    PopDialogUnk100000 = 0x100000,
};

enum {
    DiploProposalUnk1 = 1, // ?
    DiploProposalUnk2 = 2, // ?
    DiploProposalUnk3 = 3, // threaten
    DiploProposalUnk4 = 4, // tech trade?
    DiploProposalUnk5 = 5, // tech trade?
    DiploProposalNeedEnergy = 6,
    DiploProposalUnk8 = 8, // propose attack
    DiploProposalUnk9 = 9, // base swap
    DiploProposalUnk11 = 11, // repay loan
    DiploProposalUnk12 = 12, // buy council vote
    DiploProposalUnk13 = 13, // tech trade?
};

enum {
    DiploCounterFriendship = 1, // goodwill and friendship
    DiploCounterNameAPrice = 2, // need but name your price
    DiploCounterResearchData = 4, // valuable research data
    DiploCounterLoanPayment = 6, // schedule of loan payments
};


ATOM WINAPI ModRegisterClassA(WNDCLASS* pstWndClass);
int WINAPI ModGetSystemMetrics(int nIndex);
int show_mod_menu();
int __cdecl X_pop2(const char* label, int a2);
int __cdecl X_pop7(const char* label, int a2, int a3);
int __cdecl X_pops4(const char* label, int a2, Sprite* a3, int a4);
int __cdecl mod_blink_timer();
void __cdecl mod_turn_timer();
int __thiscall mod_calc_dim(Console* This);
int __thiscall mod_gen_map(Console* This, int iOwner, int fUnitsOnly);
void __thiscall MapWin_gen_overlays(Console* This, int x, int y);
int __thiscall basewin_popup_start(
    Win* This, const char* filename, const char* label, int a4, int a5, int a6, int a7);
int __cdecl basewin_ask_number(const char* label, int value, int a3);
void __cdecl basewin_draw_name(char* dest, char* name);
void __cdecl basewin_action_staple(int base_id);
int __thiscall ReportWin_close_handler(void* This);
void __thiscall Console_editor_fungus(Console* UNUSED(This));
void __cdecl mod_say_loc(char* dest, int x, int y, int a4, int a5, int a6);
void __cdecl mod_diplomacy_caption(int faction1, int faction2);
int __cdecl mod_energy_trade(int faction1, int faction2);




