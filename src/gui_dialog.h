#pragma once

#include "main.h"

enum PopDialog {
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

enum DiploProposal {
    DiploProposalMakeGift = 1,
    DiploProposalMakePact = 2,
    DiploProposalMakeTreaty = 3,
    DiploProposalTechTrade = 4,
    DiploProposalBuyTech = 5, // mention related prototype?
    DiploProposalNeedEnergy = 6,
    DiploProposalTradeMaps = 7,
    DiploProposalJointAttack = 8,
    DiploProposalBaseSwap = 9,
    DiploProposalCloseDialog = 10,
    DiploProposalRepayLoan = 11,
    DiploProposalCouncilVote = 12,
    DiploProposalTradeCommlink = 13,
};

enum DiploCounter {
    DiploCounterFriendship = 1, // goodwill and friendship
    DiploCounterNameAPrice = 2, // need but name your price
    DiploCounterThreaten = 3, // threaten with attack or cancel pact
    DiploCounterResearchData = 4, // valuable research data
    DiploCounterEnergyPayment = 5, // modest sum of energy credits
    DiploCounterLoanPayment = 6, // schedule of loan payments
    DiploCounterGiveBase = 8, // turn over one of my bases
};

void parse_gen_name(int faction_id, size_t title_value, size_t name_value);
void parse_noun_name(int faction_id, size_t title_value, size_t name_value);
int __cdecl X_pop2(const char* label, int a2);
int __cdecl X_pop3(const char* filename, const char* label, int a3);
int __cdecl X_pop7(const char* label, int a2, int a3);
int __cdecl X_pops4(const char* label, int a2, Sprite* a3, int a4);
int __cdecl X_dialog(const char* label, int faction2);
int __cdecl X_dialog(const char* filename, const char* label, int faction2);
int __cdecl DiploPop_spying(int faction_id);
int __cdecl tech_achieved_pop3(const char* filename, const char* label, int a3);
int __cdecl mod_threaten(int faction1, int faction2);
int __cdecl mod_base_swap(int faction1, int faction2);
int __cdecl mod_energy_trade(int faction1, int faction2);
int __cdecl mod_buy_tech(int faction1, int faction2, int counter_id, int high_price, int proposal_id);

