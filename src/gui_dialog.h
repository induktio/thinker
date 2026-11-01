#pragma once

#include "main.h"

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
    DiploCounterMakeGift = 7, // grant all research data
    DiploCounterGiveBase = 8, // turn over one of my bases
};

int __thiscall Popup_load_icon(Popup* This, Sprite* a2, int a3, char* a4);
int __cdecl X_pop(const char* label, fp_none fn);
int __cdecl X_pop_2(const char* filename, const char* label, fp_none fn);
int __cdecl X_pop_6(const char* label, int a2, fp_none fn);
int __cdecl X_pops(const char* label, Sprite* a2, fp_none fn);
int __cdecl X_pops_11(const char* label, int a2, Sprite* a3, fp_none fn);
int __cdecl X_dialog(const char* label, int faction_id);
int __cdecl X_dialog_alt(const char* filename, const char* label, int faction_id);

