
#include "faction_event.h"


static bool player_spying(int player_id, int faction_id_1, int faction_id_2) {
    Faction& plr = Factions[player_id];
    return has_project(FAC_EMPATH_GUILD, player_id)
        || (plr.diplo_status[faction_id_1] & DIPLO_HAVE_INFILTRATOR)
        || (player_id == *GovernorFaction && !MFactions[faction_id_1].is_alien())
        || (plr.diplo_status[faction_id_2] & DIPLO_HAVE_INFILTRATOR)
        || (player_id == *GovernorFaction && !MFactions[faction_id_2].is_alien());
}

/*
Pact status counts as infiltration on diplomacy window for displaying the energy reserves.
In the original game energy reserves are always displayed on faction profile if pact is active.
*/
int __cdecl DiploPop_spying(int faction_id) {
    return has_treaty(MapWin->cOwner, faction_id, DIPLO_PACT|DIPLO_HAVE_INFILTRATOR)
        || has_project(FAC_EMPATH_GUILD, MapWin->cOwner)
        || (MapWin->cOwner == *GovernorFaction && !MFactions[faction_id].is_alien());
}

void __cdecl pact_attacks(int faction_id_1, int faction_id_2) {
    parse_says(0, get_title(faction_id_1), -1, -1);
    parse_says(1, get_name(faction_id_1), -1, -1);
    parse_says(2, get_noun(faction_id_1), -1, -1);
    parse_says(3, get_he_she(faction_id_1, 0), -1, -1);
    parse_says(4, get_noun(faction_id_2), -1, -1);
    mod_NetMsg_pop(NetMsg, "PACTATTACKS", 5000, 0, 0);
}

void __cdecl pact_unpact(int faction_id_1, int faction_id_2) {
    parse_says(0, get_title(faction_id_1), -1, -1);
    parse_says(1, get_name(faction_id_1), -1, -1);
    parse_says(2, get_noun(faction_id_1), -1, -1);
    parse_says(3, get_he_she(faction_id_1, 0), -1, -1);
    parse_says(4, get_his_her(faction_id_1, 0), -1, -1);
    parse_says(5, get_pact_hood(faction_id_1, faction_id_2), -1, -1);
    parse_says(6, get_title(faction_id_2), -1, -1);
    parse_says(7, get_name(faction_id_2), -1, -1);
    parse_says(8, get_noun(faction_id_2), -1, -1);
    mod_NetMsg_pop(NetMsg, "PACTUNPACT", 5000, 0, 0);
}

void __cdecl pact_truce(int faction_id_1, int faction_id_2) {
    parse_says(0, get_title(faction_id_1), -1, -1);
    parse_says(1, get_name(faction_id_1), -1, -1);
    parse_says(2, get_noun(faction_id_1), -1, -1);
    parse_says(3, get_he_she(faction_id_1, 0), -1, -1);
    parse_says(4, get_title(faction_id_2), -1, -1);
    parse_says(5, get_name(faction_id_2), -1, -1);
    parse_says(6, get_noun(faction_id_2), -1, -1);
    mod_NetMsg_pop(NetMsg, "PACTTRUCE", 5000, 0, 0);
}

void __cdecl pact_treaty(int faction_id_1, int faction_id_2) {
    parse_says(0, get_title(faction_id_1), -1, -1);
    parse_says(1, get_name(faction_id_1), -1, -1);
    parse_says(2, get_noun(faction_id_1), -1, -1);
    parse_says(3, get_he_she(faction_id_1, 0), -1, -1);
    parse_says(4, get_noun(faction_id_2), -1, -1);
    mod_NetMsg_pop(NetMsg, "PACTTREATY", 5000, 0, 0);
}

void __cdecl enemies_war(int faction_id_1, int faction_id_2) {
    const int player_id = MapWin->cOwner;
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (plr1.diplo_status[faction_id_2] & DIPLO_VENDETTA) {
        return;
    }
    if (Factions[player_id].diplo_status[faction_id_1] & DIPLO_PACT) {
        pact_attacks(faction_id_1, faction_id_2);
    } else if (Factions[player_id].diplo_status[faction_id_2] & DIPLO_PACT) {
        pact_attacks(faction_id_2, faction_id_1);
    } else if (player_spying(player_id, faction_id_1, faction_id_2)
    || (*GameState & STATE_OMNISCIENT_VIEW)) {
        parse_says(0, get_noun(faction_id_1), -1, -1);
        parse_says(1, get_noun(faction_id_2), -1, -1);
        mod_NetMsg_pop(NetMsg, "ENEMYWAR", 5000, 0, 0);
    }
    treaty_on(faction_id_1, faction_id_2, DIPLO_VENDETTA);
    if (!(plr1.diplo_status[faction_id_2] & DIPLO_UNK_40000000)) {
        trade_maps(faction_id_1, faction_id_2);
    }
    uint32_t status = plr1.diplo_status[faction_id_2];
    if ((status & DIPLO_WANT_REVENGE) && !(status & DIPLO_ATROCITY_VICTIM)) {
        if (*DiffLevel > 1 && game_rand() % *DiffLevel) {
            plr1.diplo_agenda[faction_id_2] &= ~(AGENDA_UNK_400|AGENDA_UNK_800);
            plr2.diplo_agenda[faction_id_1] &= ~(AGENDA_UNK_400|AGENDA_UNK_800);
            plr1.diplo_status[faction_id_2] &= ~DIPLO_WANT_REVENGE;
            plr2.diplo_status[faction_id_1] &= ~DIPLO_WANT_REVENGE;
            plr1.diplo_status[faction_id_2] |= DIPLO_UNK_40;
            plr1.diplo_merc[faction_id_2] = 50;
        }
    }
    status = plr2.diplo_status[faction_id_1];
    if ((status & DIPLO_WANT_REVENGE) && !(status & DIPLO_ATROCITY_VICTIM)) {
        if (*DiffLevel > 1 && game_rand() % *DiffLevel) {
            plr2.diplo_agenda[faction_id_1] &= ~(AGENDA_UNK_400|AGENDA_UNK_800);
            plr1.diplo_agenda[faction_id_2] &= ~(AGENDA_UNK_400|AGENDA_UNK_800);
            plr2.diplo_status[faction_id_1] &= ~DIPLO_WANT_REVENGE;
            plr1.diplo_status[faction_id_2] &= ~DIPLO_WANT_REVENGE;
            set_treaty(faction_id_2, faction_id_1, DIPLO_UNK_40, true);
        }
    }
}

void __cdecl enemies_unpact(int faction_id_1, int faction_id_2) {
    const int player_id = MapWin->cOwner;
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (plr1.diplo_status[faction_id_2] & DIPLO_PACT) {
        if (plr1.diplo_status[player_id] & DIPLO_PACT) {
            pact_unpact(faction_id_1, faction_id_2);
        } else if (plr2.diplo_status[player_id] & DIPLO_PACT) {
            pact_unpact(faction_id_2, faction_id_1);
        }
        if (player_spying(player_id, faction_id_1, faction_id_2)
        || (*GameState & STATE_OMNISCIENT_VIEW)) {
            parse_says(0, get_noun(faction_id_1), -1, -1);
            parse_says(1, get_noun(faction_id_2), -1, -1);
            parse_says(2, get_pact_hood(faction_id_1, faction_id_2), -1, -1);
            mod_NetMsg_pop(NetMsg, "ENEMYPACTENDS", 5000, 0, 0);
        }
        plr1.diplo_status[faction_id_2] &= ~(DIPLO_UNK_1000000|DIPLO_UNK_100000|DIPLO_PACT);
        plr2.diplo_status[faction_id_1] &= ~(DIPLO_UNK_1000000|DIPLO_UNK_100000|DIPLO_PACT);
    }
}

void __cdecl enemies_team_up(int faction_id_1, int faction_id_2, int faction_id_3) {
    const int player_id = MapWin->cOwner;
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (!(plr1.diplo_status[faction_id_3] & DIPLO_VENDETTA) && is_human(faction_id_3)) {
        if (plr2.diplo_status[faction_id_3] & DIPLO_VENDETTA) {
            if (!great_satan(faction_id_3, 0) && !(plr1.diplo_status[faction_id_3] & DIPLO_WANT_REVENGE)) {
                treaty_on(faction_id_1, faction_id_3, DIPLO_WANT_TO_TALK);
                return;
            }
            if (faction_id_3 == player_id) {
                parse_says(0, get_noun(faction_id_1), -1, -1);
                mod_NetMsg_pop(NetMsg, "TEAM", 5000, 0, 0);
            }
            Factions[faction_id_3].diplo_spoke[faction_id_2] = *CurrentTurn;
            Factions[faction_id_3].diplo_spoke[faction_id_1] = *CurrentTurn;
            treaty_on(faction_id_1, faction_id_3, DIPLO_VENDETTA);
        } else if (great_satan(faction_id_3, 0)
        || (plr1.diplo_status[faction_id_3] & DIPLO_WANT_REVENGE)
        || (plr2.diplo_status[faction_id_3] & DIPLO_WANT_REVENGE)) {
            plr1.diplo_status[faction_id_3] |= DIPLO_SHALL_BETRAY;
        } else {
            treaty_on(faction_id_1, faction_id_3, DIPLO_WANT_TO_TALK);
        }
    }
}

void __cdecl enemies_trade_tech(int faction_id_1, int faction_id_2) {
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (*GameRules & RULES_SCN_NO_TECH_TRADING) {
        return;
    }
    int tech_to1 = -1;
    int tech_to2 = -1;
    int score_to1 = -9999;
    int score_to2 = -9999;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        const bool has1 = has_tech(i, faction_id_1);
        const bool has2 = has_tech(i, faction_id_2);
        if (has1 && !has2) {
            // Fix: original version compared plr2.diplo_friction[plr2.tech_trade_source[i]]
            // instead of the symmetric condition plr1.diplo_friction[source]
            const int source = plr1.tech_trade_source[i];
            if ((!source || !(plr1.diplo_status[source] & DIPLO_PACT))
            && (!(plr1.diplo_status[source] & DIPLO_TREATY)
            || plr1.diplo_friction[source]
            >= ((plr1.diplo_status[faction_id_2] & DIPLO_PACT) ? 6 : 16))) {
                int value = mod_tech_val(i, faction_id_2, 0);
                int score = (value > 1 ? game_rand() % value : 0);
                if (score > score_to2) {
                    score_to2 = score;
                    tech_to2 = i;
                }
            }
        } else if (!has1 && has2) {
            const int source = plr2.tech_trade_source[i];
            if (!source || !(plr2.diplo_status[source] & DIPLO_PACT)) {
                if (!(plr2.diplo_status[source] & DIPLO_TREATY)
                || plr2.diplo_friction[source]
                >= ((plr1.diplo_status[faction_id_2] & DIPLO_PACT) ? 6 : 16)) {
                    int value = mod_tech_val(i, faction_id_1, 0);
                    int score = (value > 1 ? game_rand() % value : 0);
                    if (score > score_to1) {
                        score_to1 = score;
                        tech_to1 = i;
                    }
                }
            }
        }
    }
    if (tech_to1 >= 0 && tech_to2 >= 0) {
        tech_achieved(faction_id_1, tech_to1, faction_id_2, 0);
        tech_achieved(faction_id_2, tech_to2, faction_id_1, 0);
    } else if ((plr1.diplo_status[faction_id_2] & DIPLO_PACT) && great_satan(FactionRankings[7], 0)) {
        if (tech_to1 >= 0 && Factions[*RankingFactionIDUnk1].tech_ranking >= plr2.tech_ranking) {
            tech_achieved(faction_id_1, tech_to1, faction_id_2, 0);
        }
        if (tech_to2 >= 0 && Factions[*RankingFactionIDUnk1].tech_ranking >= plr1.tech_ranking) {
            tech_achieved(faction_id_2, tech_to2, faction_id_1, 0);
        }
    }
}

void __cdecl enemies_treaty(int faction_id_1, int faction_id_2, int trade_tech) {
    const int player_id = MapWin->cOwner;
    Faction& plr1 = Factions[faction_id_1];
    Faction& plr2 = Factions[faction_id_2];
    if (trade_tech) {
        enemies_trade_tech(faction_id_1, faction_id_2);
    }
    const uint32_t status12 = plr1.diplo_status[faction_id_2];
    if (!(status12 & DIPLO_TREATY) && (status12 & (DIPLO_VENDETTA|DIPLO_TRUCE))) {
        const int difficulty = ((*GameRules & RULES_INTENSE_RIVALRY) ? DIFF_TRANSCEND : *DiffLevel);
        if ((status12 & (DIPLO_WANT_REVENGE|DIPLO_UNK_40))
        || (plr2.diplo_status[faction_id_1] & (DIPLO_UNK_40|DIPLO_WANT_REVENGE))
        || (plr2.ranking >= 6 && plr1.ranking >= 6)
        || game_rand() % 16 < plr2.AI_fight + plr1.AI_fight - difficulty + 8) {
            if (Factions[player_id].diplo_status[faction_id_1] & DIPLO_PACT) {
                pact_truce(faction_id_1, faction_id_2);
            } else if (Factions[player_id].diplo_status[faction_id_2] & DIPLO_PACT) {
                pact_truce(faction_id_2, faction_id_1);
            } else if (player_spying(player_id, faction_id_1, faction_id_2)
            || (*GameState & STATE_OMNISCIENT_VIEW)) {
                parse_says(0, get_noun(faction_id_1), -1, -1);
                parse_says(1, get_noun(faction_id_2), -1, -1);
                mod_NetMsg_pop(NetMsg, "ENEMYTRUCE", 5000, 0, 0);
            }
            treaty_on(faction_id_1, faction_id_2, DIPLO_TRUCE);
            return;
        }
    }
    if (!(status12 & DIPLO_TREATY)) {
        if (Factions[player_id].diplo_status[faction_id_1] & DIPLO_PACT) {
            pact_treaty(faction_id_1, faction_id_2);
        } else if (Factions[player_id].diplo_status[faction_id_2] & DIPLO_PACT) {
            pact_treaty(faction_id_2, faction_id_1);
        } else if (player_spying(player_id, faction_id_1, faction_id_2)
        || (*GameState & STATE_OMNISCIENT_VIEW)) {
            parse_says(0, get_noun(faction_id_1), -1, -1);
            parse_says(1, get_noun(faction_id_2), -1, -1);
            mod_NetMsg_pop(NetMsg, "ENEMYTREATY", 5000, 0, 0);
        }
        treaty_on(faction_id_1, faction_id_2, DIPLO_TREATY);
        return;
    }
    if ((FactionRankings[7] == faction_id_1 && plr2.ranking >= 5)
    || (FactionRankings[7] == faction_id_2 && plr1.ranking >= 5)
    || (status12 & (DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE))
    || (plr2.diplo_status[faction_id_1] & (DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE))
    || plr1.major_atrocities
    || plr2.major_atrocities) {
        enemies_unpact(faction_id_1, faction_id_2);
        return;
    }
    int pact_count = 0;
    int faction_id_3;
    for (faction_id_3 = 1; faction_id_3 < MaxPlayerNum; faction_id_3++) {
        const uint32_t status13 = plr1.diplo_status[faction_id_3];
        const uint32_t status23 = plr2.diplo_status[faction_id_3];
        if (faction_id_3 != faction_id_1
        && faction_id_3 != faction_id_2
        && ((status13 & DIPLO_COMMLINK) || (status23 & DIPLO_COMMLINK))
        && (((status13 & DIPLO_COMMLINK) && (status23 & DIPLO_COMMLINK)) || great_satan(faction_id_3, 0))) {
            if ((status13 & DIPLO_PACT) || (status23 & DIPLO_PACT)) {
                if (Factions[faction_id_3].ranking >= plr1.ranking
                || Factions[faction_id_3].ranking >= plr2.ranking) {
                    pact_count++;
                }
                continue;
            }
            if (Factions[faction_id_3].diff_level
            && (great_satan(faction_id_3, 0)
            || ((status13 & (DIPLO_PACT|DIPLO_TREATY|DIPLO_TRUCE)) == 0
            && (status23 & (DIPLO_PACT|DIPLO_TREATY|DIPLO_TRUCE)) == 0
            && ((status12 & DIPLO_PACT)
            || ((status13 & DIPLO_VENDETTA)
            && (plr1.ranking < Factions[faction_id_3].ranking
            || plr2.ranking < Factions[faction_id_3].ranking)))))) {
                break;
            }
        }
    }
    if (faction_id_3 >= MaxPlayerNum) {
        enemies_unpact(faction_id_1, faction_id_2);
        return;
    }
    if (status12 & DIPLO_PACT) {
        return;
    }
    const int blemishes = (great_satan(faction_id_3, 0) ? Factions[faction_id_3].integrity_blemishes : 0);
    if (pact_count > blemishes) {
        enemies_unpact(faction_id_1, faction_id_2);
        return;
    }
    treaty_on(faction_id_1, faction_id_2, DIPLO_PACT);
    enemies_team_up(faction_id_1, faction_id_2, faction_id_3);
    enemies_team_up(faction_id_2, faction_id_1, faction_id_3);
    if (faction_id_3 == player_id || player_spying(player_id, faction_id_1, faction_id_2)) {
        parse_says(0, get_noun(faction_id_1), -1, -1);
        parse_says(1, get_noun(faction_id_2), -1, -1);
        parse_says(2, get_noun(faction_id_3), -1, -1);
        const char* suffix = "";
        if (faction_id_3 == player_id) {
            if (!(plr1.diplo_status[faction_id_3] & DIPLO_VENDETTA)
            && !(plr2.diplo_status[faction_id_3] & DIPLO_VENDETTA)) {
                suffix = "0";
            }
        } else {
            suffix = "1";
        }
        snprintf(StrBuffer, StrBufLen, "ENEMIESTEAMUP%s", suffix);
        mod_NetMsg_pop(NetMsg, StrBuffer, 5000, 0, 0);
    }
}

void __cdecl comm_check(int faction_id_1, int faction_id_2) {
    const int player_id = MapWin->cOwner;
    if (!is_human(faction_id_1)) {
        return;
    }
    uint32_t status = Factions[faction_id_1].diplo_status[faction_id_2];
    bool has_commlink = status & DIPLO_COMMLINK;
    if (has_commlink && !(status & DIPLO_WANT_TO_TALK)) {
        int spoke = Factions[faction_id_1].diplo_spoke[faction_id_2];
        if (spoke >= 0 && *CurrentTurn - spoke < 16) {
            return;
        }
    }
    if (*MultiplayerActive) {
        if (faction_id_1 != player_id || is_human(faction_id_2)) {
            if (is_human(faction_id_2) && !has_commlink) {
                treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_10000|DIPLO_TRUCE);
            }
            if (faction_id_1 == player_id
            && !(Factions[faction_id_1].diplo_status[faction_id_2] & DIPLO_UNK_8000000)) {
                parse_says(0, get_title(faction_id_2), -1, -1);
                parse_says(1, get_name(faction_id_2), -1, -1);
                parse_says(2, get_noun(faction_id_2), -1, -1);
                NetMsg_pop(NetMsg, "NETMET", 5000, 0, 0);
            }
            if (faction_id_2 == player_id
            && !(Factions[faction_id_1].diplo_status[faction_id_2] & DIPLO_UNK_8000000)) {
                parse_says(0, get_title(faction_id_1), -1, -1);
                parse_says(1, get_name(faction_id_1), -1, -1);
                parse_says(2, get_noun(faction_id_1), -1, -1);
                NetMsg_pop(NetMsg, "NETMET", 5000, 0, 0);
            }
        } else {
            DiploStateA[faction_id_2] |= DST_UNK_1;
            if (!has_commlink) {
                DiploStateA[faction_id_2] |= DST_UNK_2;
                net_treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_10000|DIPLO_TRUCE, 0);
            }
        }
        return;
    }
    if (!*PbemActive) {
        communicate(faction_id_1, faction_id_2, 0);
        return;
    }
    if (is_human(faction_id_2)) {
        if (!has_commlink) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_10000|DIPLO_TRUCE);
        }
        return;
    }
    if (faction_id_1 == player_id) {
        communicate(faction_id_1, faction_id_2, 0);
        return;
    }
    DiploStateB[faction_id_1][faction_id_2] |= DST_UNK_1;
    if (!has_commlink) {
        DiploStateB[faction_id_1][faction_id_2] |= DST_UNK_2;
        treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_10000|DIPLO_TRUCE);
    }
}

void __cdecl encounter(int faction_id_1, int faction_id_2, int x, int y, int fungal_tower, int native_unit) {
    if (!faction_id_1 || !*CurrentTurn) {
        return;
    }
    if (!faction_id_2) {
        if (faction_id_1 == MapWin->cOwner && MFactions[faction_id_1].is_alien()) {
            if (fungal_tower) {
                interlude(44, 0, 1, 0);
            }
            if (native_unit) {
                interlude(43, 0, 1, 0);
            }
        }
        return;
    }
    if (MFactions[faction_id_1].is_alien() && MFactions[faction_id_2].is_alien()) {
        treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_COMMLINK);
        if (on_map(x, y)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
        }
        if (!_stricmp(MFactions[faction_id_1].search_key, "CARETAKE")) {
            if (faction_id_1 == MapWin->cOwner) {
                interlude(39, 0, 1, 0);
            }
            if (faction_id_2 == MapWin->cOwner) {
                interlude(40, 0, 1, 0);
            }
        } else {
            if (faction_id_1 == MapWin->cOwner) {
                interlude(40, 0, 1, 0);
            }
            if (faction_id_2 == MapWin->cOwner) {
                interlude(39, 0, 1, 0);
            }
        }
        return;
    }
    if (MFactions[faction_id_1].is_alien()) {
        if (!has_tech(TECH_PrPsych, faction_id_2) && !has_tech(TECH_Psych, faction_id_1)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_COMMLINK);
            if (on_map(x, y)) {
                treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
            }
            if (faction_id_1 == MapWin->cOwner) {
                interlude(34, 0, 1, 0);
            }
            if (faction_id_2 == MapWin->cOwner) {
                interlude(22, 0, 1, 0);
            }
            return;
        }
        if (faction_id_1 == MapWin->cOwner) {
            interlude(35, 0, 1, 0);
        }
        if (faction_id_2 == MapWin->cOwner) {
            interlude(23, 0, 1, 0);
        }
    }
    if (MFactions[faction_id_2].is_alien()) {
        if (!has_tech(TECH_PrPsych, faction_id_1) && !has_tech(TECH_Psych, faction_id_2)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_COMMLINK);
            if (on_map(x, y)) {
                treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
            }
            if (faction_id_1 == MapWin->cOwner) {
                interlude(22, 0, 1, 0);
            }
            if (faction_id_2 == MapWin->cOwner) {
                interlude(34, 0, 1, 0);
            }
            return;
        }
        if (faction_id_1 == MapWin->cOwner) {
            interlude(23, 0, 1, 0);
        }
        if (faction_id_2 == MapWin->cOwner) {
            interlude(35, 0, 1, 0);
        }
    }
    if (*SunspotDuration <= 0) {
        *EncounterLocX = x;
        *EncounterLocY = y;
        comm_check(faction_id_1, faction_id_2);
        comm_check(faction_id_2, faction_id_1);
        *EncounterLocX = -1;
        *EncounterLocY = -1;

        bool had_commlink = Factions[faction_id_1].diplo_status[faction_id_2] & DIPLO_COMMLINK;
        auto show_meet_tutorial = [&](int self_id, int other_id) {
            parse_says(0, get_noun(other_id), -1, -1);
            if (MFactions[self_id].is_alien()) {
                if (!MFactions[other_id].is_alien()) {
                    if (has_tech(TECH_Psych, self_id) || has_tech(TECH_PrPsych, other_id)) {
                        if (tut_check2(0x4000)) {
                            TutWin_tut_map(TutWin, "ALMEETSGUY", x, y, -1, 0, 0, -1, -1);
                        }
                    } else if (tut_check2(0x4000)) {
                        TutWin_tut_map(TutWin, "ALMEETSGUYNOPSYCH", x, y, -1, 0, 0, -1, -1);
                    }
                } else if (tut_check2(0x10000)) {
                    TutWin_tut_map(TutWin, "ALMEETSAL", x, y, -1, 0, 0, -1, -1);
                }
            } else if (has_tech(TECH_PrPsych, self_id) || has_tech(TECH_Psych, other_id)) {
                if (tut_check2(0x8000)) {
                    TutWin_tut_map(TutWin, "GUYMEETSAL", x, y, -1, 0, 0, -1, -1);
                }
            } else if (tut_check2(0x8000)) {
                TutWin_tut_map(TutWin, "GUYMEETSALNOPSYCH", x, y, -1, 0, 0, -1, -1);
            }
        };
        if (!had_commlink && (MFactions[faction_id_1].is_alien() || MFactions[faction_id_2].is_alien())) {
            if (faction_id_1 == MapWin->cOwner) {
                show_meet_tutorial(faction_id_1, faction_id_2);
            } else if (faction_id_2 == MapWin->cOwner) {
                show_meet_tutorial(faction_id_2, faction_id_1);
            }
        }
        treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_UNK_8000000|DIPLO_COMMLINK);
        if (on_map(x, y)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
        }
        if (!is_human(faction_id_1)
        && !is_human(faction_id_2)
        && (((faction_id_1 + faction_id_2 + *CurrentTurn) & 3) == 0 || !had_commlink)
        && (!MFactions[faction_id_1].is_alien() || !MFactions[faction_id_2].is_alien())) {
            if (!(Factions[faction_id_1].diplo_agenda[faction_id_2] & AGENDA_PERMANENT)
            || (MFactions[faction_id_1].is_alien() && MFactions[faction_id_2].is_alien())) {
                if (mod_wants_to_attack(faction_id_1, faction_id_2, 0)
                || mod_wants_to_attack(faction_id_2, faction_id_1, 0)) {
                    enemies_war(faction_id_1, faction_id_2);
                } else {
                    enemies_treaty(faction_id_1, faction_id_2, 1);
                }
            } else {
                enemies_trade_tech(faction_id_1, faction_id_2);
            }
        }
    } else {
        treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_80000000|DIPLO_COMMLINK);
        if (on_map(x, y)) {
            treaty_on(faction_id_1, faction_id_2, DIPLO_UNK_40000000);
        }
    }
}

int __cdecl territory(int faction_id_1, int faction_id_2, int flags, int* out_base_id, int* out_count) {
    int cnt = 0;
    if (out_base_id) {
        *out_base_id = -1;
    }
    if (out_count) {
        *out_count = 0;
    }
    const uint32_t status = Factions[faction_id_1].diplo_status[faction_id_2];
    if (status & (DIPLO_VENDETTA|DIPLO_PACT)) {
        return 0;
    }
    if (!(flags & 2) && (status & DIPLO_UNK_200)) {
        return 0;
    }
    int skip_region[MaxRegionNum] = {}; // includes all land/ocean regions
    for (int i = 0; i < MaxRegionLandNum; i++) {
        if (!(status & DIPLO_WANT_REVENGE) && Factions[faction_id_2].integrity_blemishes < 5) {
            for (int j = 1; j < MaxPlayerNum; j++) {
                if (j != faction_id_1
                && j != faction_id_2
                && (Factions[faction_id_1].diplo_status[j] & DIPLO_VENDETTA)
                && (Factions[faction_id_2].diplo_status[j] & DIPLO_VENDETTA)
                && Factions[j].region_total_bases[i]) {
                    skip_region[i] = 1;
                    break;
                }
            }
        }
    }
    int base_id;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        veh->state &= ~VSTATE_UNK_400;
        if (veh->faction_id != faction_id_2) {
            continue;
        }
        if ((uint8_t)veh->plan() <= PLAN_COLONY || (veh->plan() == PLAN_SUPPLY && veh->order == ORDER_CONVOY)) {
            MAP* sq = mapsq(veh->x, veh->y);
            if (sq && sq->alt_level() >= ALT_SHORE_LINE && !skip_region[sq->region]) {
                if (whose_territory(faction_id_2, veh->x, veh->y, &base_id, 1) == faction_id_1 && base_id >= 0) {
                    if (out_base_id) {
                        *out_base_id = base_id;
                    }
                    veh->state |= VSTATE_UNK_400;
                    cnt++;
                    if (sq->items & (BIT_SENSOR|BIT_THERMAL_BORE|BIT_CONDENSER|BIT_FOREST
                    |BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_MINE)) {
                        cnt++;
                    }
                    if (flags & 1) {
                        if (map_range(Bases[base_id].x, Bases[base_id].y, veh->x, veh->y) <= 2
                        || (status & (DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE))
                        || *DiploFriction > 12) {
                            if (out_count && (veh->flags & VFLAG_UNK_1)) {
                                ++*out_count;
                            }
                            veh->flags |= VFLAG_UNK_1|VFLAG_UNK_2;
                            if (!(veh->state & VSTATE_UNK_800) && *SunspotDuration <= 0) {
                                veh->state |= VSTATE_UNK_800;
                                continue;
                            }
                            cnt += ((status & DIPLO_TREATY) ? 5 : 1);
                        }
                    }
                }
            }
        }
    }
    if (cnt) {
        Factions[faction_id_1].player_flags |= PFLAG_UNK_10000;
    }
    return cnt;
}

void __cdecl enemy_diplomacy(int faction_id) {
    if (!faction_id) {
        return;
    }
    Faction& plr = Factions[faction_id];
    for (int i = 1; i < MaxPlayerNum; i++) {
        plr.diplo_status[i] &= ~DIPLO_UNK_100;
        uint32_t agenda = plr.diplo_agenda[i];
        if (agenda & AGENDA_UNK_20) {
            if (agenda & AGENDA_UNK_40) {
                plr.diplo_agenda[i] = agenda & ~AGENDA_UNK_20;
                Factions[i].diplo_agenda[faction_id] &= ~AGENDA_UNK_20;
            } else {
                plr.diplo_agenda[i] = agenda | AGENDA_UNK_40;
                Factions[i].diplo_agenda[faction_id] |= AGENDA_UNK_40;
            }
        }
    }
    if (is_human(faction_id)) {
        return;
    }
    int vendetta_count = 0;
    int base_id = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id == i || !is_alive(i)) {
            continue;
        }
        if (plr.diplo_status[i] & DIPLO_VENDETTA) {
            vendetta_count++;
        }
        if (plr.diplo_merc[i]) {
            plr.diplo_merc[i]--;
            if (!plr.diplo_merc[i]) {
                plr.diplo_status[i] &= ~DIPLO_UNK_40;
            }
        }
        if ((*CurrentTurn & 1) && plr.diplo_patience[i] > 0) {
            plr.diplo_patience[i]--;
        }
        if (plr.diplo_status[i] & DIPLO_SHALL_BETRAY) {
            int betray_chance = clamp(8 - ((plr.diplo_agenda[i] & AGENDA_UNK_400) ? 2 : 0)
                - Factions[i].integrity_blemishes, 1, 8);
            if (betray_chance <= 1 || !(game_rand() % betray_chance) || (plr.diplo_status[i] & DIPLO_PACT)) {
                if (plr.diplo_status[i] & DIPLO_PACT) {
                    cause_friction(faction_id, i, 20);
                    plr.diplo_status[i] &= ~(DIPLO_UNK_20000 | DIPLO_UNK_800 | DIPLO_SHALL_BETRAY);
                    treaty_on(faction_id, i, DIPLO_WANT_TO_TALK);
                } else if (Factions[i].target_x >= 0) {
                    plr.diplo_status[i] |= DIPLO_UNK_20000;
                } else {
                    if (Factions[i].diplo_spoke[faction_id] < *CurrentTurn - 8) {
                        Factions[i].diplo_spoke[faction_id] = *CurrentTurn - 8;
                    }
                    plr.diplo_status[i] = (plr.diplo_status[i] & ~(DIPLO_UNK_20000 | DIPLO_SHALL_BETRAY))
                        | DIPLO_UNK_800 | DIPLO_UNK_40;
                    plr.diplo_merc[i] = 50;
                }
            }
        }
        if ((*CurrentTurn & 0xF) == 2 * faction_id + 1) {
            plr.diplo_status[i] &= ~DIPLO_UNK_8000;
            Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_8000;
        }
        if (!is_human(i) || *SunspotDuration > 0) {
            if (faction_id > i && *SunspotDuration <= 0
            && (((faction_id + i + *CurrentTurn) & 7) == 0 || (plr.diplo_status[i] & DIPLO_WANT_TO_TALK))) {
                if (!(plr.diplo_status[i] & DIPLO_UNK_80000000) || (plr.diplo_status[i] & DIPLO_WANT_TO_TALK)) {
                    plr.diplo_status[i] &= ~DIPLO_WANT_TO_TALK;
                    Factions[i].diplo_status[faction_id] &= ~DIPLO_WANT_TO_TALK;
                    if (plr.diplo_status[i] & DIPLO_PACT) {
                        enemies_trade_tech(faction_id, i);
                    } else if (((plr.diplo_status[i] & DIPLO_TREATY) && (*GameState & STATE_UNK_200))
                    || has_project(FAC_EMPATH_GUILD, faction_id)
                    || has_project(FAC_EMPATH_GUILD, i)
                    || (plr.diplo_status[i] & DIPLO_UNK_20000000)
                    || ((plr.diplo_status[i] & DIPLO_COMMLINK) && (plr.diplo_status[i] & DIPLO_UNK_10000000))) {
                        encounter(faction_id, i, -1, -1, 0, 0);
                    }
                    treaty_on(faction_id, i, DIPLO_UNK_80000000);
                    plr.diplo_status[i] &= ~DIPLO_UNK_4000000;
                    Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_4000000;
                } else {
                    plr.diplo_status[i] &= ~DIPLO_UNK_80000000;
                    Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_80000000;
                    for (int j = 1; j < MaxPlayerNum; j++) {
                        if (is_human(j) && ((plr.diplo_status[j] & DIPLO_VENDETTA)
                        || (Factions[i].diplo_status[j] & DIPLO_VENDETTA))) {
                            plr.diplo_status[i] &= ~DIPLO_UNK_4000000;
                            Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_4000000;
                        }
                    }
                }
            }
            continue;
        }
        if (i == MapWin->cOwner
        && (plr.diplo_status[i] & (DIPLO_ATROCITY_VICTIM | DIPLO_VENDETTA | DIPLO_COMMLINK)) == DIPLO_COMMLINK) {
            int j;
            for (j = 1; j < 8; j++) {
                if (is_alive(j) && j != faction_id && j != i
                && (plr.diplo_status[j] & (DIPLO_UNK_8000 | DIPLO_VENDETTA | DIPLO_COMMLINK))
                    == (DIPLO_UNK_8000 | DIPLO_VENDETTA | DIPLO_COMMLINK)
                && (Factions[i].diplo_status[j] & (DIPLO_VENDETTA | DIPLO_COMMLINK)) == DIPLO_COMMLINK
                && Factions[j].mil_strength_1 > plr.mil_strength_1) {
                    break;
                }
            }
            if (j < 8 && !(plr.diplo_status[i] & DIPLO_PACT) && reputation(i, faction_id) < 5
            && ((*CurrentTurn - Factions[i].diplo_spoke[faction_id] > 8 && (*CurrentTurn & 0xF) == 2 * faction_id)
            || (plr.diplo_status[i] & DIPLO_WANT_TO_TALK))) {
                DiploStateA[faction_id] |= DST_UNK_40|DST_UNK_1;
                DiploStateB[i][faction_id] |= DST_UNK_40|DST_UNK_1;
                if (!*MultiplayerActive && !*PbemActive) {
                    communicate(i, faction_id, 0);
                    treaty_on(i, faction_id, DIPLO_UNK_80000000 | DIPLO_UNK_8000000 | DIPLO_COMMLINK);
                }
            } else if (!(plr.diplo_status[i] & DIPLO_WANT_REVENGE)
            && (*CurrentTurn - Factions[i].diplo_spoke[faction_id] > 32
            || ((plr.diplo_status[i] & DIPLO_UNK_20000000) && *CurrentTurn == faction_id + 8)
            || ((plr.diplo_status[i] & DIPLO_PACT) && (plr.diplo_status[i] & DIPLO_WANT_TO_TALK)))
            && (!game_random(0, 16) || (plr.diplo_status[i] & DIPLO_WANT_TO_TALK) || *CurrentTurn == faction_id + 8)) {
                if (!*MultiplayerActive && !*PbemActive) {
                    communicate(i, faction_id, 0);
                    treaty_on(i, faction_id, DIPLO_UNK_80000000 | DIPLO_UNK_8000000 | DIPLO_COMMLINK);
                } else {
                    DiploStateA[faction_id] |= DST_UNK_1;
                    DiploStateB[i][faction_id] |= DST_UNK_1;
                }
            }
        }
        if (*PbemActive ? !DiploStateB[i][faction_id] : !DiploStateA[faction_id]) {
            diplomacy_check(i, faction_id, 0);
            if (*DiploAttackChance) {
                if ((plr.diplo_status[i] & DIPLO_WANT_REVENGE)
                || (plr.player_flags_ext & PFLAG_EXT_SHAMELESS_BETRAY_HUMANS)
                || (Factions[i].diff_level > DIFF_THINKER && (*GameRules & RULES_INTENSE_RIVALRY))) {
                    plr.diplo_status[i] = (plr.diplo_status[i] & ~(DIPLO_UNK_20000 | DIPLO_SHALL_BETRAY))
                        | DIPLO_UNK_800 | DIPLO_UNK_40;
                    plr.diplo_merc[i] = 50;
                }
            }
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (!is_human(i) || !(plr.diplo_status[i] & DIPLO_COMMLINK)) {
            continue;
        }
        int territory_result = 0;
        int territory_aux = 0;
        if (*CurrentTurn - Factions[i].diplo_spoke[faction_id] >= 4
        || *SunspotDuration > 0
        || (plr.diplo_status[i] & DIPLO_WANT_REVENGE)) {
            territory_result = territory(faction_id, i, 1, &base_id, &territory_aux);
            if (territory_result && i == MapWin->cOwner && !(plr.diplo_status[i] & DIPLO_VENDETTA)) {
                if (*SunspotDuration <= 0) {
                    uint32_t agenda = plr.diplo_agenda[i];
                    if (plr.diplo_status[i] & DIPLO_TREATY) {
                        if (territory_result >= 5) {
                            DiploStateA[faction_id] |= DST_UNK_4;
                            DiploStateB[i][faction_id] |= DST_UNK_4;
                        } else if (((faction_id + *CurrentTurn) & 3) == 0) {
                            DiploStateA[faction_id] |= DST_UNK_20;
                            DiploStateB[i][faction_id] |= DST_UNK_20;
                        }
                    } else if ((agenda & AGENDA_UNK_800) || ((agenda & AGENDA_UNK_400) && territory_aux)) {
                        DiploStateA[faction_id] |= DST_UNK_10|DST_UNK_8;
                        DiploStateB[i][faction_id] |= DST_UNK_10|DST_UNK_8;
                    } else {
                        bool escalate = false;
                        if (!(plr.diplo_status[i] & DIPLO_WANT_REVENGE) || territory_aux) {
                            if (((faction_id + *CurrentTurn) & 3) == 0) {
                                DiploStateA[faction_id] |= DST_UNK_8;
                                DiploStateB[i][faction_id] |= DST_UNK_8;
                                escalate = game_random(0, territory_aux + Factions[i].integrity_blemishes
                                    + territory_result + 4 * (territory_aux + Factions[i].integrity_blemishes)
                                    + (plr.diplo_status[i] & DIPLO_WANT_REVENGE ? 5 : 0)) > 10;
                            }
                        } else {
                            DiploStateA[faction_id] |= DST_UNK_8;
                            DiploStateB[i][faction_id] |= DST_UNK_8;
                            escalate = game_random(0, territory_result
                                + 4 * (Factions[i].integrity_blemishes + 1)
                                + Factions[i].integrity_blemishes + 1) > 10;
                        }
                        if (escalate) {
                            if (i == MapWin->cOwner) {
                                DiploStateA[faction_id] |= DST_UNK_10;
                            }
                            DiploStateB[i][faction_id] |= DST_UNK_10;
                        }
                    }
                    if (DiploStateA[faction_id] && !*MultiplayerActive && !*PbemActive) {
                        communicate(i, faction_id, 0);
                        treaty_on(i, faction_id, DIPLO_UNK_80000000 | DIPLO_UNK_8000000 | DIPLO_COMMLINK);
                    }
                } else {
                    diplomacy_check(i, faction_id, 0);
                    if (*DiploHostility
                    || (plr.diplo_status[i] & DIPLO_WANT_REVENGE)
                    || territory_result + *DiploFriction + 2 * Factions[i].integrity_blemishes
                    >= 2 * (9 - plr.AI_fight)) {
                        if (plr.diplo_status[i] & DIPLO_TREATY) {
                            plr.diplo_status[i] |= DIPLO_UNK_800;
                        } else if (!(plr.diplo_status[i] & (DIPLO_SHALL_BETRAY | DIPLO_UNK_800))) {
                            if (plr.diplo_status[i] & DIPLO_WANT_REVENGE) {
                                plr.diplo_status[i] |= DIPLO_UNK_800;
                            } else {
                                plr.diplo_status[i] |= DIPLO_SHALL_BETRAY;
                            }
                        }
                    }
                }
            }
        }
        int friction = ((*CurrentTurn & 1) + territory_result + 3) / 5;
        if (plr.diplo_status[i] & DIPLO_PACT) {
            if (vendetta_count) {
                friction += 1 - vendetta_count;
            } else {
                int our_tech = plr.tech_ranking / 2;
                int their_tech = Factions[i].tech_ranking / 2;
                if (their_tech > plr.AI_tech - plr.AI_power - plr.AI_fight + our_tech + 5) {
                    friction++;
                }
                if (our_tech > their_tech + 5) {
                    friction--;
                }
                if (Factions[i].ranking > plr.ranking + 1
                && Factions[i].diff_level != DIFF_CITIZEN) {
                    friction++;
                }
            }
            if (!(plr.diplo_status[i] & DIPLO_UNK_1000000)) {
                for (int j = 1; j < MaxPlayerNum; j++) {
                    if ((plr.diplo_status[j] & DIPLO_VENDETTA)
                    && (Factions[i].diplo_status[j] & DIPLO_COMMLINK)
                    && !(Factions[i].diplo_status[j] & DIPLO_VENDETTA)) {
                        friction++;
                    }
                }
            }
            if ((*GameRules & RULES_VICTORY_COOPERATIVE) && plr.ranking < 6
            && *CurrentTurn >= 200 && friction >= 0) {
                friction = -1;
            }
        } else {
            if ((*GameRules & RULES_INTENSE_RIVALRY) && (*CurrentTurn & 1)) {
                friction++;
            }
            int their_strength = Factions[i].mil_strength_1;
            int our_strength = plr.mil_strength_1;
            int their_scale = clamp(3 * their_strength, 1, 99999);
            friction += 2 * our_strength / their_scale;
            int our_scale = clamp(3 * our_strength, 1, 99999);
            friction += -2 * their_strength / our_scale;
            if (great_satan(i, 0)) {
                friction++;
            }
            if (aah_ooga(i, -1)) {
                friction++;
            }
        }
        if (*CurrentTurn & 1) {
            int soc_category = MFactions[faction_id].soc_priority_category;
            if (soc_category >= 0) {
                int soc_model = MFactions[faction_id].soc_priority_model;
                if (soc_model) {
                    int their_model = (&Factions[i].SE_Politics_pending)[soc_category];
                    if (their_model == soc_model) {
                        friction--;
                    } else if (their_model) {
                        friction++;
                    }
                }
            }
        }
        friction += plr.AI_fight;
        if ((*CurrentTurn & 3) == 0) {
            if (plr.diplo_status[i] & DIPLO_TREATY) {
                friction--;
            }
            int our_rank = plr.ranking;
            int their_rank = Factions[i].ranking;
            if (our_rank >= 6 && their_rank >= 6) {
                friction += 3;
            } else if (our_rank < 5 || their_rank < 5) {
                int rank_diff = their_rank - our_rank;
                int rank_sign = (rank_diff > 0) - (rank_diff < 0);
                friction += 3 * rank_sign - (rank_sign + rank_diff) / 2;
            } else if (*CurrentTurn >= 100) {
                friction++;
            }
            if (Factions[i].energy_credits > plr.energy_credits
            && !strcmp(MFactions[faction_id].filename, "MORGAN")) {
                friction++;
            }
            if (Factions[i].tech_ranking > plr.tech_ranking) {
                friction += plr.AI_power - plr.AI_tech + 1;
            }
        }
        if ((plr.diplo_status[i] & DIPLO_PACT) && (*GameRules & RULES_VICTORY_COOPERATIVE)
        && Factions[i].ranking > plr.ranking && *CurrentTurn >= 200) {
            if (friction > 0) {
                friction = 0;
            }
        }
        plr.diplo_status[i] &= ~DIPLO_UNK_200;
        Factions[i].diplo_status[faction_id] &= ~DIPLO_UNK_200;
        if (!(plr.diplo_status[i] & DIPLO_VENDETTA) || friction >= 0) {
            cause_friction(faction_id, i, friction);
        }
    }
    int max_busters = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i == faction_id) {
            continue;
        }
        uint32_t status = plr.diplo_status[i];
        if (status & DIPLO_PACT) {
            continue;
        }
        int busters = Factions[i].planet_busters;
        if ((status & DIPLO_TREATY) && !(status & DIPLO_WANT_REVENGE)) {
            busters /= 2;
        }
        if (max_busters <= busters) {
            max_busters = busters;
        }
    }
    while (plr.satellites_ODP - plr.ODP_deployed > max_busters) {
        int sat_type = -1;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (!(plr.diplo_status[i] & (DIPLO_VENDETTA | DIPLO_UNK_800))) {
                continue;
            }
            int32_t their_sats = Factions[i].satellites_ODP;
            if (plr.satellites_ODP <= their_sats) {
                continue;
            }
            if (their_sats && their_sats >= plr.planet_busters) {
                sat_type = 3;
            } else {
                int geosync_count = 0;
                for (base_id = 0; base_id < *BaseCount; base_id++) {
                    if (Bases[base_id].faction_id == i && Bases[base_id].has_fac_built(FAC_GEOSYNC_SURVEY_POD)) {
                        geosync_count++;
                    }
                }
                if (Factions[i].satellites_mineral) {
                    sat_type = 1;
                } else if (Factions[i].satellites_energy) {
                    sat_type = 2;
                } else if (Factions[i].satellites_nutrient) {
                    sat_type = 0;
                } else if (geosync_count) {
                    sat_type = 4;
                    for (base_id = 0; base_id < *BaseCount; base_id++) {
                        if (Bases[base_id].faction_id == i && Bases[base_id].has_fac_built(FAC_GEOSYNC_SURVEY_POD)) {
                            break;
                        }
                    }
                } else if (their_sats) {
                    sat_type = 3;
                } else {
                    continue;
                }
            }
            action_sat_attack(faction_id, i, sat_type, base_id);
            break;
        }
        if (sat_type < 0) {
            break;
        }
    }
}

void __cdecl enemy_capabilities(int faction_id) {
    Faction& plr = Factions[faction_id];
    plr.best_psi_land_offense = veh_avail(BSC_MIND_WORMS, faction_id, -1)
        ? weap_strat(WPN_PSI_ATTACK, faction_id) : 0;
    plr.best_weapon_value = 1;
    for (int weapon_id = 0; weapon_id < MaxWeaponNum; weapon_id++) {
        if (has_tech(Weapon[weapon_id].preq_tech, faction_id)
        && Weapon[weapon_id].offense_value < 99) {
            int value = weap_strat(weapon_id, faction_id);
            if (Weapon[weapon_id].offense_value < 0 && value >= plr.best_psi_land_offense) {
                plr.best_psi_land_offense = value;
            }
            if (value >= plr.best_weapon_value) {
                plr.best_weapon_value = value;
            }
        }
    }
    plr.best_psi_land_defense = veh_avail(BSC_MIND_WORMS, faction_id, -1)
        ? arm_strat(ARM_PSI_DEFENSE, faction_id) : 0;
    plr.best_armor_value = 1;
    for (int armor_id = 0; armor_id < MaxArmorNum; armor_id++) {
        if (has_tech(Armor[armor_id].preq_tech, faction_id)) {
            int value = arm_strat(armor_id, faction_id);
            if (Armor[armor_id].defense_value < 0 && value >= plr.best_psi_land_defense) {
                plr.best_psi_land_defense = value;
            }
            if (value >= plr.best_armor_value) {
                plr.best_armor_value = value;
            }
        }
    }
    plr.best_land_speed = 1;
    for (int chassis_id = 0; chassis_id < MaxChassisNum; chassis_id++) {
        if (has_tech(Chassis[chassis_id].preq_tech, faction_id)
        && Chassis[chassis_id].triad == TRIAD_LAND) {
            int speed = Chassis[chassis_id].speed;
            if (speed >= plr.best_land_speed) {
                plr.best_land_speed = speed;
            }
        }
    }
    plr.enemy_best_land_speed = 0;
    plr.enemy_best_armor_value = 0;
    plr.enemy_best_weapon_value = 0;
    plr.enemy_best_psi_land_defense = 0;
    plr.enemy_best_psi_land_offense = 0;
    for (int iter = 0; iter < 4; iter++) {
        if (plr.enemy_best_weapon_value) {
            return;
        }
        for (int enemy_id = 1; enemy_id < MaxPlayerNum; enemy_id++) {
            uint32_t status = plr.diplo_status[enemy_id];
            if (enemy_id == faction_id || (status & DIPLO_PACT)) {
                continue;
            }
            if (iter == 0 && !(status & DIPLO_VENDETTA)) {
                continue;
            }
            if (iter <= 1 && (status & DIPLO_TREATY)) {
                continue;
            }
            if (iter > 2 || (status & DIPLO_COMMLINK)) {
                Faction& enemy = Factions[enemy_id];
                if (plr.enemy_best_weapon_value <= enemy.best_weapon_value) {
                    plr.enemy_best_weapon_value = enemy.best_weapon_value;
                }
                if (plr.enemy_best_armor_value <= enemy.best_armor_value) {
                    plr.enemy_best_armor_value = enemy.best_armor_value;
                }
                if (plr.enemy_best_land_speed <= enemy.best_land_speed) {
                    plr.enemy_best_land_speed = enemy.best_land_speed;
                }
                if (plr.enemy_best_psi_land_offense <= enemy.best_psi_land_offense) {
                    plr.enemy_best_psi_land_offense = enemy.best_psi_land_offense;
                }
                if (plr.enemy_best_psi_land_defense <= enemy.best_psi_land_defense) {
                    plr.enemy_best_psi_land_defense = enemy.best_psi_land_defense;
                }
            }
        }
    }
}

void __cdecl enemy_strategy(int faction_id) {
    Faction& plr = Factions[faction_id];
    int faction_bit = 1 << faction_id;
    int region_formers[MaxRegionNum];
    int goal_assigned[75];
    int waiting_units = 0;
    int transport_count = 0;
    int max_cargo = 2;

    plr.total_combat_units = 0;
    plr.base_count = 0;
    plr.mil_strength_1 = 0;
    plr.mil_strength_2 = 0;
    plr.pop_total = 0;
    plr.unk_70 = 0;
    plr.planet_busters = 0;
    plr.unk_71 = 0;
    plr.unk_72 = 0;

    for (int i = 0; i < MaxRegionNum; i++) {
        plr.region_total_combat_units[i] = 0;
        plr.region_total_bases[i] = 0;
        plr.region_force_rating[i] = 0;
        plr.region_flags[i] &= ((faction_id + *CurrentTurn + i) & 0xF) != 0 ? REGION_UNK_8 : 0;
    }
    for (int i = 0; i < MaxProtoNum; i++) {
        plr.units_active[i] = 0;
        plr.units_queue[i] = 0;
    }
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE& base = Bases[base_id];
        int x = base.x;
        int y = base.y;
        MAP* sq = mapsq(x, y);
        if (!sq) {
            assert(0);
            continue;
        }
        int region = sq->region;
        if (base.faction_id == faction_id) {
            base.state_flags &= ~(BSTATE_UNK_4000000|BSTATE_UNK_8000000);
            if (base.ai_plan_status) {
                base.ai_plan_status--;
            }
            plr.base_count++;
            plr.pop_total += base.pop_size;
            int stack_id = stack_fix(veh_at(x, y));
            if (!stack_check(stack_id, 14, faction_id, -1, -1)
            && !stack_check(stack_id, 2, PLAN_RECON, faction_id, -1)) {
                add_goal(faction_id, AI_GOAL_DEFEND, 4, x, y, -1);
            }
            assert(base.queue_size < 10);
            for (int i = 0; i <= base.queue_size && i < 10; i++) {
                if (base.queue_items[i] >= 0) {
                    plr.units_queue[base.queue_items[i]]++;
                }
            }
            plr.region_total_bases[region]++;
            if (is_coast(x, y, 0)) {
                plr.region_flags[region] |= REGION_UNK_80;
            }
            if (((base_id + *CurrentTurn) & 3) == 0) {
                base.event_flags &= ~0xFF; // TODO BaseEvent
            }
            continue;
        }
        if (!(base.visibility & faction_bit)) {
            continue;
        }
        uint32_t status = plr.diplo_status[base.faction_id];
        if (!(status & (DIPLO_VENDETTA|DIPLO_SHALL_BETRAY|DIPLO_UNK_800)) || (status & DIPLO_PACT)) {
            continue;
        }
        int priority = 1;
        if (sq->veh_who() < 0) {
            priority = 2;
            add_goal(faction_id, AI_GOAL_DEFEND, 2, x, y, -1);
            add_goal(faction_id, AI_GOAL_SCOUT, 2, x, y, -1);
        }
        if (base.faction_id_former == faction_id) {
            priority++;
        }
        if (plr.region_total_bases[region]) {
            priority++;
        }
        if (!has_fac_built(FAC_PERIMETER_DEFENSE, base_id)) {
            priority++;
        }
        if (base.queue_items[0] == -FAC_ASCENT_TO_TRANSCENDENCE) {
            priority *= 4;
        }
        if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
            priority++;
            if (plr.player_flags & PFLAG_STRAT_ATK_ENEMY_HQ) {
                priority *= 2;
            }
            if (Factions[base.faction_id].corner_market_active()) {
                priority *= 8;
            }
        }
        if ((plr.base_id_attack_target == base_id && (plr.diplo_status[base.faction_id] & DIPLO_VENDETTA))
        || is_objective(base_id)) {
            priority *= 4;
        }
        add_goal(faction_id, AI_GOAL_ATTACK, priority, x, y, -1);
        if (on_map(base.unk_x, base.unk_y)) {
            int target_base = base_find(base.unk_x, base.unk_y);
            if (target_base >= 0 && Bases[target_base].faction_id == base.faction_id) {
                if (terraform_cost(base.unk_x, base.unk_y, faction_id) <= plr.energy_credits / 8) {
                    add_goal(faction_id, AI_GOAL_UNK_4, 2, base.unk_x, base.unk_y, -1);
                }
            }
        }
    }

    if (!is_human(faction_id)) {
        for (int base_id = 0; base_id < *BaseCount; base_id++) {
            if (Bases[base_id].faction_id == faction_id
            && Bases[base_id].queue_items[0] == -FAC_STOCKPILE_ENERGY) {
                mod_base_reset(base_id, 0);
            }
        }
    }

    memset(region_formers, 0, sizeof(region_formers));
    enemy_capabilities(faction_id);
    plr.unk_68 = 0;
    plr.unk_66 = 0;
    plr.unk_65 = 0;
    plr.unk_64 = 0;
    plr.unk_106 = 0;
    plr.unk_105 = 0;

    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
        if (veh_id >= *VehCount) {
            continue;
        }
        VEH& veh = Vehs[veh_id];
        MAP* veh_sq = mapsq(veh.x, veh.y);
        assert(veh_sq);
        int veh_base_who = (veh_sq ? veh_sq->base_who() : -1);
        int unit_id = veh.unit_id;
        UNIT& unit = Units[unit_id];
        if (veh.faction_id != faction_id) {
            uint32_t status = plr.diplo_status[veh.faction_id];
            if ((status & DIPLO_COMMLINK)
            && (!(status & (DIPLO_TREATY|DIPLO_PACT))
            || (status & (DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE)))) {
                if (unit.plan == PLAN_NAVAL_TRANSPORT) {
                    plr.unk_64++;
                }
                if (unit.plan == PLAN_NAVAL_SUPERIORITY) {
                    plr.unk_65 += unit.reactor_id;
                }
                if (unit.triad() == TRIAD_AIR) {
                    if (unit.plan == PLAN_OFFENSE && !unit.is_missile()
                    && weap_val(unit_id, faction_id) >= plr.enemy_best_armor_value) {
                        plr.unk_66++;
                    }
                    if (unit.is_missile() && !planet_buster(veh_id)) {
                        plr.unk_68++;
                    }
                }
                if (unit.plan == PLAN_TERRAFORM && unit.triad() == TRIAD_SEA) {
                    if (veh_sq && is_ocean(veh_sq) && veh_base_who < 0
                    && ((veh.faction_id == faction_id && !veh.is_invisible_lurker())
                    || (faction_bit & veh.visibility))) {
                        add_goal(faction_id, AI_GOAL_UNK_1,
                            (plr.player_flags & PFLAG_UNK_80000) ? 4 : 2, veh.x, veh.y, -1);
                    }
                }
            }
            continue;
        }
        if (!veh_sq) {
            continue;
        }
        int region = veh_sq->region;
        bool on_water = is_ocean(veh_sq);
        int plan = (uint8_t)unit.plan;
        int triad = unit.triad();
        plr.units_active[unit_id]++;
        if (veh.year_end_lurking) {
            veh.year_end_lurking--;
            if (!veh.year_end_lurking) {
                veh.flags &= ~(VFLAG_INVISIBLE|VFLAG_LURKER);
            }
        }
        if (!is_human(faction_id)
        && (faction_bit & unit.obsolete_factions)
        && conf.factions_enabled < faction_id) {
            Console_upgrade(MapWin, veh_id);
            // Fix: remove additional code that considered disbanding units if the upgrade failed
            // because incorrect parameters to arm_val compared undefined memory locations.
        }
        if (!is_human(faction_id)
        && ((*CurrentTurn + veh_id) & 3) != 0
        && *CurrentTurn > 25
        && unit_id == BSC_SCOUT_PATROL
        && (plr.region_base_plan[region] == PLAN_NAVAL_TRANSPORT || veh.order == ORDER_HOLD)) {
            if (!on_water
            && (veh_base_who < 0 || stack_check(veh_id, 14, faction_id, -1, -1) > 1)
            && plr.region_base_plan[region] != PLAN_DEFENSE
            && !zoc_sea(veh.x, veh.y, faction_id)
            && veh.status_icon != '1') {
                kill(veh_id);
                continue;
            }
        }
        veh.state &= ~(VSTATE_UNK_10000000|VSTATE_UNK_20000000|VSTATE_MADE_AIRDROP|VSTATE_HAS_MOVED);
        if (!is_human(faction_id) && veh.order == ORDER_SENTRY_BOARD
        && !(triad == TRIAD_LAND && on_water)) {
            veh.order = ORDER_NONE;
        }
        if (on_water) {
            if (triad == TRIAD_LAND && veh_base_who < 0) {
                if (!stack_check(veh_id, 2, PLAN_NAVAL_TRANSPORT, -1, -1)) {
                    if (!(veh.flags & VFLAG_IS_OBJECTIVE)) {
                        veh_kill(veh_id);
                    }
                    continue;
                }
                if (veh.order != ORDER_SENTRY_BOARD) {
                    sleep(veh_id);
                }
            }
        } else if (triad == TRIAD_SEA && veh_base_who < 0) {
            if (!(veh.flags & VFLAG_IS_OBJECTIVE)) {
                veh_kill(veh_id);
            }
            continue;
        }
        if (plan <= PLAN_COMBAT && !(veh.state & VSTATE_UNK_40000)) {
            int garrison_base_id = base_at(veh.x, veh.y);
            if (garrison_base_id < 0 && veh.status_icon == 'F') {
                garrison_base_id = veh.home_base_id;
            }
            if (garrison_base_id >= 0) {
                int garrison = garrison_check(garrison_base_id);
                int needed = garrison_check(garrison_base_id);
                if (needed > 5) {
                    needed--;
                }
                if (needed > 2) {
                    needed--;
                }
                int base_stack = veh_at(Bases[garrison_base_id].x, Bases[garrison_base_id].y);
                int designated = stack_check(base_stack, 11, VSTATE_UNK_10000000, VSTATE_UNK_10000000, faction_id);
                if (garrison > needed + designated) {
                    veh.state |= VSTATE_UNK_10000000;
                }
            }
        }
        if (veh.status_icon == 'F' && plan == PLAN_DEFENSE) {
            int home_id = veh.home_base_id;
            if (home_id >= 0) {
                if (Bases[home_id].x != veh.x || Bases[home_id].y != veh.y) {
                    Bases[home_id].state_flags |= BSTATE_UNK_4000000;
                }
            }
        } else {
            switch (plan) {
                case PLAN_TERRAFORM: {
                    veh.flags &= ~VFLAG_UNK_800;
                    region_formers[region]++;
                    int home_id = veh.home_base_id;
                    if (home_id >= 0) {
                        Bases[home_id].state_flags |= (triad != TRIAD_SEA ? BSTATE_UNK_20 : BSTATE_UNK_80000);
                    }
                    // Fix: remove additional code that was inconsistently used to reassign AI formers
                    // to nearby bases without checking the base ownership. That code did not first
                    // move units to the base tile and only picked bases that had mineral_surplus < 2.
                    break;
                }
                case PLAN_COLONY:
                    if (plr.region_flags[region] & REGION_UNK_2) {
                        plr.region_flags[region] |= REGION_UNK_800;
                    } else {
                        plr.region_flags[region] |= REGION_UNK_2;
                    }
                    break;
                case PLAN_PROBE:
                    plr.region_flags[region] |= REGION_UNK_1000;
                    break;
                case PLAN_RECON:
                    if (veh_base_who < 0) {
                        plr.region_flags[region] |= REGION_UNK_2000;
                    }
                    break;
            }
        }
        if (plan <= PLAN_NAVAL_TRANSPORT) {
            plr.total_combat_units++;
            if (region >= 0 && triad == TRIAD_LAND && !on_water) {
                plr.region_total_combat_units[region]++;
            }
        }
        if (triad == TRIAD_SEA) {
            plr.unk_70++;
        } else if (triad == TRIAD_AIR) {
            if (plan == PLAN_OFFENSE) {
                plr.unk_105++;
            } else if (plan == PLAN_AIR_SUPERIORITY) {
                plr.unk_106++;
            }
        }
        if (plan == PLAN_NAVAL_TRANSPORT) {
            transport_count++;
            if (max_cargo <= veh_cargo(veh_id)) {
                max_cargo = veh_cargo(veh_id);
            }
        }

        if (planet_buster(veh_id)) {
            plr.planet_busters++;
            continue;
        }
        int wpn = weap_strat(veh.weapon_type(), faction_id);
        int arm = arm_strat(veh.armor_type(), faction_id);
        int morale = mod_morale_veh(veh_id, 0, 0) + 6;
        plr.mil_strength_1 += morale * (wpn + arm) / 8;
        if (wpn > arm || veh.order != ORDER_HOLD) {
            int atk_val = wpn * morale / 8;
            plr.mil_strength_2 += atk_val;
            if (region >= 0) {
                plr.region_force_rating[region] += atk_val;
            }
        }
        uint32_t target_state = veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000);
        if (target_state == (VSTATE_UNK_40000|VSTATE_UNK_20000)) {
            waiting_units++;
            continue;
        }
        if (target_state != VSTATE_UNK_40000) {
            continue;
        }
        MAP* nsq = mapsq(plr.target_x, plr.target_y);
        if (!nsq || region != nsq->region) {
            veh.state |= VSTATE_UNK_20000;
            waiting_units++;
            continue;
        }
        plr.unk_71++;
        if (plan == PLAN_PROBE) {
            plr.unk_72 |= 2;
        } else if (arm > wpn) {
            plr.unk_72 |= 1;
        }
    }

    plr.unk_70 += (transport_count + (max_cargo << 11)) << 11;
    int max_rival_cargo = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id) {
            uint32_t status = plr.diplo_status[i];
            if ((status & DIPLO_COMMLINK) && !(status & DIPLO_PACT)
            && max_rival_cargo <= Factions[i].unk_70 / 0x400000) {
                max_rival_cargo = Factions[i].unk_70 / 0x400000;
            }
        }
    }
    plr.unk_64 += max_rival_cargo << 11;

    if (transport_count > 1) {
        for (int i = 0; i < *VehCount; i++) {
            int unit_id = Vehs[i].unit_id;
            if (Vehs[i].faction_id == faction_id
            && Units[unit_id].triad() == TRIAD_SEA
            && Units[unit_id].plan == PLAN_NAVAL_TRANSPORT
            && (unit_id == BSC_UNITY_FOIL || plr.units_active[unit_id] >= 2)) {
                Vehs[i].state |= VSTATE_UNK_20000000;
                break;
            }
        }
    }

    for (int i = 0; i < *VehCount; i++) {
        VEH& veh = Vehs[i];
        if (veh.faction_id == faction_id && veh.plan() == PLAN_TERRAFORM) {
            unsigned region = mapsq(veh.x, veh.y)->region;
            if (region < MaxRegionLandNum) {
                if (region_formers[region] >= 3 && plr.region_total_bases[region] >= 3) {
                    veh.flags |= VFLAG_UNK_800;
                    if (veh.home_base_id >= 0) {
                        Bases[veh.home_base_id].state_flags &= ~BSTATE_UNK_20;
                    }
                    region_formers[region] = 0;
                }
            }
        }
    }

    int target_region = -1;
    bool skip_targeting = is_human(faction_id);
    if (plr.target_x >= 0) {
        int target_base = base_at(plr.target_x, plr.target_y);
        MAP* sq = mapsq(plr.target_x, plr.target_y);
        target_region = (sq ? sq->region : -1);
        assert(sq);
        if (target_base < 0 || Bases[target_base].faction_id != faction_id) {
            skip_targeting = true;
        } else {
            int target_stack = stack_fix(veh_at(Bases[target_base].x, Bases[target_base].y));
            if (target_stack >= 0) {
                int defenders = stack_check(target_stack, 11, VSTATE_UNK_40000|VSTATE_UNK_20000, VSTATE_UNK_40000, -1);
                int needed = clamp(2 * plr.region_total_bases[target_region] / 3, 3, 8);
                if (defenders - bit_count(plr.unk_72) >= needed) {
                    skip_targeting = true;
                }
                unsigned region_plan = plr.region_base_plan[target_region];
                if (region_plan == PLAN_DEFENSE || (plr.region_flags[target_region] & (REGION_UNK_4000|REGION_UNK_200))) {
                    skip_targeting = true;
                }
                if (region_plan > PLAN_DEFENSE
                && stack_check(target_stack, 2, PLAN_NAVAL_TRANSPORT, -1, -1)) {
                    if (defenders >= 2 * (plr.unk_70 / 0x400000)) {
                        skip_targeting = true;
                    }
                    if (stack_check(target_stack, 2, PLAN_NAVAL_TRANSPORT, -1, -1) > 1) {
                        int ground_count = stack_check(target_stack, 8, -1, -1, -1);
                        int land_count = stack_check(target_stack, 3, TRIAD_LAND, -1, -1);
                        if (defenders >= land_count + ground_count) {
                            skip_targeting = true;
                        }
                    }
                }
            }
            if (!skip_targeting) {
                unsigned region_plan = plr.region_base_plan[target_region];
                if (region_plan <= PLAN_DEFENSE) {
                    plr.player_flags &= ~PFLAG_UNK_800;
                } else if (region_plan == PLAN_COLONY
                || !is_coast(Bases[target_base].x, Bases[target_base].y, 0)) {
                    skip_targeting = true;
                } else {
                    plr.player_flags |= PFLAG_UNK_800;
                }
            }
        }
    }
    if (skip_targeting) {
        for (int i = 0; i < *VehCount; i++) {
            if (Vehs[i].faction_id == faction_id && (Vehs[i].state & VSTATE_UNK_40000)) {
                Vehs[i].state |= VSTATE_UNK_20000;
            }
        }
        plr.target_x = -1;
        plr.target_y = -1;
        plr.unk_71 = 0;
        plr.unk_72 = 0;
    }
    int best_value = 0;
    int best_region = -1;
    for (int region = 1; region < MaxRegionNum; region++) {
        plr.region_total_offensive_units[region] = 0;
        if (bad_reg(region) && !plr.region_total_bases[region]) {
            plr.region_base_plan[region] = PLAN_COLONY;
            continue;
        }
        int old_plan = plr.region_base_plan[region];
        int plan_flags = 0;
        int threat_count = 0;
        int weak_count = 0;
        int factions_present = 0;
        int contact_count = 0;
        int land_contact_count = 0;
        bool conflict = false;
        int my_units = plr.region_total_combat_units[region];
        int my_force = plr.region_force_rating[region];
        int my_bases = plr.region_total_bases[region];

        for (int enemy_id = 1; enemy_id < 8; enemy_id++) {
            Faction& enemy = Factions[enemy_id];
            uint32_t enemy_status = plr.diplo_status[enemy_id];
            int enemy_units = enemy.region_total_combat_units[region];
            int enemy_force = enemy.region_force_rating[region];
            int enemy_bases = enemy.region_total_bases[region];
            if (enemy_status & DIPLO_PACT) {
                enemy_units *= 2;
                enemy_bases *= 2;
            }
            if (enemy_bases) {
                factions_present++;
                plr.region_flags[region] |= enemy_id != faction_id ? REGION_UNK_20 : REGION_UNK_10;
                if (enemy_status & DIPLO_VENDETTA) {
                    plr.region_flags[region] |= REGION_UNK_40;
                }
            } else if (!enemy_units || my_force > 5 * enemy_force) {
                continue;
            }
            if (faction_id == enemy_id || (enemy_status & DIPLO_PACT) || !(enemy_status & DIPLO_COMMLINK)) {
                continue;
            }

            bool contact_land = false;
            bool contact_any = false;
            bool estimated = false;
            if (*CurrentTurn > 1) {
                int tile_count = Continents[region].tile_count;
                if (tile_count >= 500 || my_bases * enemy.region_total_bases[region] >= 50) {
                    int period = tile_count >= 1000
                        ? ((*CurrentTurn + 2 * faction_id) & 0xF)
                        : ((faction_id + *CurrentTurn) & 7);
                    if (period) {
                        contact_land = contact_any = !(enemy_bases && plr.region_base_plan[region] > PLAN_DEFENSE);
                        estimated = true;
                    }
                }
            }
            if (!estimated) {
                if (!enemy_bases || contiguous(faction_id, enemy_id, region, 0)) {
                    contact_land = true;
                    contact_any = true;
                } else {
                    contact_land = false;
                    contact_any = contiguous(faction_id, enemy_id, region, 1) != 0;
                }
            }

            if (enemy_status & (DIPLO_VENDETTA|DIPLO_SHALL_BETRAY|DIPLO_UNK_800)) {
                if (contact_any) {
                    contact_count++;
                }
                if (contact_land) {
                    if (++land_contact_count > 1) {
                        plr.region_flags[region] |= REGION_UNK_200;
                    }
                }
                if ((my_force <= enemy_units || my_force <= enemy_force)
                && my_units <= enemy_force && my_bases && contact_any) {
                    threat_count++;
                } else {
                    weak_count++;
                }
                if (enemy_force >= 3 * my_force / 2 && my_bases > enemy_bases && contact_any) {
                    plr.region_flags[region] |= REGION_UNK_4000;
                }
                if (is_human(enemy_id) && (contact_land || contact_any)) {
                    for (int i = 0; i < *BaseCount; i++) {
                        if (Bases[i].faction_id == enemy_id
                        && Bases[i].faction_id_former == faction_id
                        && mapsq(Bases[i].x, Bases[i].y)->region == region) {
                            plr.region_flags[region] |= REGION_UNK_400;
                            break;
                        }
                    }
                }
            } else if (my_bases && (enemy_status & DIPLO_WANT_REVENGE) && enemy_force > my_units) {
                if ((my_force > enemy_units && enemy_bases && enemy_force < 2 * my_force) || !contact_any) {
                    plan_flags |= 2;
                } else {
                    plan_flags |= 1;
                }
            }

            if (!(enemy_status & (DIPLO_PACT|DIPLO_TREATY))
            || ((mod_wants_to_attack(faction_id, enemy_id, 0) || mod_wants_to_attack(enemy_id, faction_id, 0))
            && contact_any)) {
                conflict = true;
            }
        }

        int unk_6 = plr.region_unk_6[region];
        int satan_bonus = great_satan(FactionRankings[7], 0);
        int growth_base = (plr.base_count > 4) + (unk_6 < 4) - ((*GameState & STATE_UNK_100) ? 2 : 0)
            + satan_bonus + factions_present + (*MapAreaTiles > 3200);
        int expand = 0;
        int growth_limit = (*CurrentTurn > 75) + growth_base + 3;
        if (my_bases) {
            int expand_flags;
            if (unk_6 >= 2 || !plr.region_total_bases[region]) {
                expand_flags = 0;
            } else {
                expand_flags = 1;
            }
            int good_tiles = plr.region_good_tiles[region];
            int free_tiles = good_tiles - plr.region_unk_5[region];
            expand_flags |= (free_tiles < 4) | (my_bases >= 20) | (plr.region_flags[region] & REGION_UNK_8);
            expand = (2 * my_bases * (growth_limit + plr.AI_growth - plr.AI_wealth)
                > good_tiles + Factions[0].region_good_tiles[region]
                && unk_6 + free_tiles / ((my_bases + 1) / 2) < 2 * growth_limit) | expand_flags;
        }
        if (conflict) {
            plr.region_flags[region] |= REGION_UNK_4;
        }
        if (expand) {
            plr.region_flags[region] |= REGION_UNK_1;
        }
        plr.region_base_plan[region] = expand ? PLAN_NAVAL_TRANSPORT : PLAN_COLONY;
        if (contact_count == 0 && plr.region_total_bases[region]) {
            plr.region_flags[region] |= REGION_UNK_100;
        } else {
            if (weak_count) {
                plan_flags = 0;
                plr.region_base_plan[region] = PLAN_OFFENSE;
            } else if (plan_flags & 2) {
                plr.region_base_plan[region] = PLAN_OFFENSE;
            }
            if (threat_count || (plan_flags & 1)) {
                plr.region_base_plan[region] = PLAN_DEFENSE;
            }
        }
        if (region >= MaxRegionLandNum) {
            plr.region_base_plan[region] = PLAN_NAVAL_TRANSPORT;
            continue;
        }
        if (!skip_targeting && region == target_region && land_contact_count >= 2) {
            for (int i = 0; i < *VehCount; i++) {
                if (Vehs[i].faction_id == faction_id && (Vehs[i].state & VSTATE_UNK_40000)) {
                    Vehs[i].state |= VSTATE_UNK_20000;
                }
            }
            plr.target_y = -1;
            plr.target_x = -1;
            plr.unk_72 = 0;
            plr.unk_71 = 0;
            target_region = -1;
        }
        if (plr.target_x < 0 && !skip_targeting && !is_human(faction_id)) {
            uint8_t region_plan = plr.region_base_plan[region];
            if (region_plan >= PLAN_DEFENSE || land_contact_count >= 2
            || (plr.region_flags[region] & REGION_UNK_4000)) {
                if (region_plan == PLAN_NAVAL_TRANSPORT && plr.region_total_bases[region] > best_value) {
                    best_value = plr.region_total_bases[region];
                    best_region = region;
                }
            } else if (plr.region_total_bases[region] + 1000 > best_value) {
                best_value = plr.region_total_bases[region] + 1000;
                best_region = region;
                int attack_base = plr.base_id_attack_target;
                if (attack_base >= 0
                && (plr.diplo_status[Bases[attack_base].faction_id] & DIPLO_VENDETTA)
                && mapsq(Bases[attack_base].x, Bases[attack_base].y)->region == region) {
                    best_value = 0;
                }
            }
        }

        if (!is_human(faction_id)
        || (faction_id == MapWin->cOwner && (*GamePreferences & PREF_AUTO_FORMER_RAISE_LWR_TERRAIN))) {
            if (plr.region_total_bases[region]) {
                uint8_t region_plan = plr.region_base_plan[region];
                if (region_plan == PLAN_NAVAL_TRANSPORT
                || (region_plan == PLAN_COLONY && Continents[region].tile_count < 150)) {
                    int best_score = 0;
                    int best_x = 0;
                    int best_y = 0;
                    for (int ay = 0; ay < *MapAbstractY; ay++) {
                        int tile_y = ay * 5;
                        for (int ax = ay & 1; ax < *MapAbstractX; ax += 2) {
                            int tile_x = ax * 5;
                            if (abstract_at(ax, ay) != region) {
                                continue;
                            }
                            int cell_score = 0;
                            int best_dir = -1;
                            int cand_x = 0;
                            int cand_y = 0;
                            for (int dir = 0; dir < 8; dir++) {
                                int nx = BaseOffsetX[dir] + ax;
                                if (!*MapToggleFlat) {
                                    if (nx >= 0) {
                                        if (nx >= *MapAbstractX) {
                                            nx -= *MapAbstractX;
                                        }
                                    } else {
                                        nx += *MapAbstractX;
                                    }
                                }
                                int ny = BaseOffsetY[dir] + ay;
                                if (nx < 0 || nx >= *MapAbstractX || ny < 0 || ny >= *MapAbstractY) {
                                    continue;
                                }
                                uint8_t neighbor = abstract_at(nx, ny);
                                if (neighbor == region) {
                                    cell_score--;
                                    continue;
                                }
                                cell_score += 2;
                                int tx = 0;
                                int ty = 0;
                                bool reached = false;
                                for (int j = 0; j < 8; j++) {
                                    tx = wrap(tile_x + BaseOffsetX[j]);
                                    ty = tile_y + BaseOffsetY[j];
                                    if (!on_map(tx, ty) || is_ocean(mapsq(tx, ty))) {
                                        continue;
                                    }
                                    int steps = 1;
                                    reached = true;
                                    while (true) {
                                        int wx = wrap(tx + BaseOffsetX[dir]);
                                        int wy = ty + BaseOffsetY[dir];
                                        if (!on_map(wx, wy) || is_ocean(mapsq(wx, wy))) {
                                            break;
                                        }
                                        tx = wx;
                                        ty = wy;
                                        if (++steps >= 100) {
                                            reached = false;
                                            break;
                                        }
                                    }
                                    if (reached) {
                                        break;
                                    }
                                }
                                if (!reached) {
                                    continue;
                                }
                                if (mapsq(tx, ty)->base_who() < 0) {
                                    best_dir = dir;
                                    cand_x = tx;
                                    cand_y = ty;
                                    cell_score += 4;
                                    if (neighbor && Continents[neighbor].tile_count >= 40) {
                                        cell_score += 2;
                                    }
                                }
                            }
                            if (cell_score > best_score && best_dir >= 0) {
                                best_score = cell_score;
                                best_x = cand_x;
                                best_y = cand_y;
                            }
                        }
                    }
                    if (best_score > 0) {
                        if (terraform_cost(best_x, best_y, faction_id) <= plr.energy_credits / 8) {
                            add_goal(faction_id, AI_GOAL_UNK_4, 2, best_x, best_y, -1);
                        }
                    }
                }
            }
        }
        if (!is_human(faction_id)) {
            mod_bases_reset(region, faction_id, plr.region_base_plan[region] == old_plan);
        }
    }

    if (best_region > 0 && best_region < 63 && !skip_targeting && !is_human(faction_id)
    && !(plr.player_flags & PFLAG_STRAT_SEARCH_OBJECTIVES)) {
        int best_rival_value = -1;
        int rival_id = -1;
        for (int enemy_id = 1; enemy_id < 8; enemy_id++) {
            if (faction_id != enemy_id) {
                uint32_t status = plr.diplo_status[enemy_id];
                int enemy_bases = Factions[enemy_id].region_total_bases[best_region];
                int vendetta_bonus = (status & DIPLO_VENDETTA) ? 2048 : 0;
                if ((status & (DIPLO_VENDETTA|DIPLO_SHALL_BETRAY|DIPLO_UNK_800|DIPLO_TRUCE))
                && ((status & (DIPLO_UNK_800|DIPLO_VENDETTA)) || (waiting_units <= 3 && enemy_bases))
                && enemy_bases + vendetta_bonus > best_rival_value) {
                    best_rival_value = enemy_bases + vendetta_bonus;
                    rival_id = enemy_id;
                }
            }
        }
        if (rival_id > 0) {
            bool coast_only = best_rival_value == 2048;
            int best_dist = 9999;
            int best_base = -1;
            for (int base_id = 0; base_id < *BaseCount; base_id++) {
                BASE& base = Bases[base_id];
                if (base.faction_id != faction_id) {
                    continue;
                }
                MAP* sq = mapsq(base.x, base.y);
                if (is_ocean(sq) || sq->region != best_region
                || (coast_only && !is_coast(base.x, base.y, 0))) {
                    continue;
                }
                int dist;
                int attack_base = plr.base_id_attack_target;
                if (attack_base >= 0
                && (plr.diplo_status[Bases[attack_base].faction_id] & (DIPLO_UNK_800|DIPLO_VENDETTA))
                && region_at(Bases[attack_base].x, Bases[attack_base].y) == best_region) {
                    dist = vector_dist(base.x, base.y, Bases[attack_base].x, Bases[attack_base].y);
                    *BaseFindDist = dist;
                } else if (base_find_3(base.x, base.y, rival_id, best_region, -1, -1) >= 0) {
                    dist = *BaseFindDist;
                } else {
                    base_find_2(base.x, base.y, rival_id);
                    dist = 2 * *BaseFindDist;
                    *BaseFindDist *= 2;
                }
                if (dist < best_dist) {
                    best_dist = dist;
                    best_base = base_id;
                }
            }
            if (best_base >= 0) {
                plr.target_x = Bases[best_base].x;
                plr.target_y = Bases[best_base].y;
                if (coast_only) {
                    plr.player_flags |= PFLAG_UNK_800;
                } else {
                    plr.player_flags &= ~PFLAG_UNK_800;
                }
                for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
                    VEH& veh = Vehs[veh_id];
                    int unit_id = veh.unit_id;
                    if (veh.faction_id == faction_id
                    && !(veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000))
                    && (uint8_t)Units[unit_id].plan <= PLAN_DEFENSE
                    && Units[unit_id].triad() == TRIAD_LAND
                    && !has_abil(unit_id, ABL_DROP_POD)
                    && region_at(veh.x, veh.y) == best_region
                    && (veh.order == ORDER_NONE || veh.order == ORDER_MOVE_TO)) {
                        base_find_2(veh.x, veh.y, rival_id);
                        if (*BaseFindDist > vector_dist(veh.x, veh.y, plr.target_x, plr.target_y)) {
                            int wpn = weap_strat(Units[unit_id].weapon_id, faction_id);
                            int arm = arm_strat(Units[unit_id].armor_id, faction_id);
                            if (wpn >= arm || (!(plr.unk_72 & 1) && !coast_only)) {
                                veh.state = (veh.state & ~(VSTATE_UNK_40000|VSTATE_UNK_20000)) | VSTATE_UNK_40000;
                                plr.unk_71++;
                                if (wpn < arm) {
                                    plr.unk_72 |= 1;
                                }
                                if (plr.unk_71 >= 8 || (coast_only && plr.unk_71 >= 2 * plr.unk_70 / 0x400000)) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (!*CurrentTurn || !faction_id) {
        return;
    }
    memset(goal_assigned, 0, sizeof(goal_assigned));
    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
        if (Vehs[veh_id].faction_id == faction_id && Vehs[veh_id].status_icon == '1') {
            Vehs[veh_id].status_icon = '2';
        }
    }
    for (int pass = 0; pass < 2; pass++) {
        for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
            if (veh_id >= *VehCount) {
                continue;
            }
            VEH& veh = Vehs[veh_id];
            if (veh.faction_id != faction_id || (!pass && veh.status_icon != '2')) {
                continue;
            }
            int veh_x = veh.x;
            int veh_y = veh.y;
            MAP* veh_sq = mapsq(veh_x, veh_y);
            assert(veh_sq);
            int veh_base_who = (veh_sq ? veh_sq->base_who() : -1);
            int region = (veh_sq ? veh_sq->region : 0);
            int plan = (uint8_t)veh.plan();
            if (pass && veh.status_icon == '1') {
                if (plan == PLAN_TERRAFORM) {
                    continue;
                }
                if (plr.region_base_plan[region] <= PLAN_DEFENSE && plan <= PLAN_DEFENSE) {
                    continue;
                }
            }
            veh.state &= ~VSTATE_UNK_40000000;
            if (veh.order >= ORDER_FARM && veh.order < ORDER_MOVE_TO) {
                continue;
            }
            if (is_human(faction_id)) {
                if (!(veh.state & VSTATE_ON_ALERT)
                || (veh.plan() == PLAN_TERRAFORM && veh.order_auto_type != ORDERA_TERRA_AUTO_FULL)
                || (veh.state & (VSTATE_UNK_1000000|VSTATE_ON_ALERT)) == (VSTATE_UNK_1000000|VSTATE_ON_ALERT)
                || ((veh.state & (VSTATE_UNK_2000000|VSTATE_ON_ALERT)) == (VSTATE_UNK_2000000|VSTATE_ON_ALERT)
                && veh.patrol_current_point < veh.waypoint_count)) {
                    continue;
                }
            }
            if (veh.state & VSTATE_UNK_10000000) {
                plr.region_total_offensive_units[region]++;
                plan = PLAN_DEFENSE;
            } else if (region < MaxRegionLandNum) {
                int divisor = 2 - plr.AI_fight;
                uint8_t region_plan = plr.region_base_plan[region];
                if (region_plan == PLAN_COLONY) {
                    divisor += 2;
                } else if (region_plan == PLAN_DEFENSE) {
                    divisor = 1;
                }
                if (plr.player_flags & PFLAG_STRAT_DEF_OBJECTIVES) {
                    divisor = 1;
                }
                int max_defenders = (plr.region_total_bases[region] + divisor - 1) / divisor;
                if (max_defenders && plan <= 1) {
                    if (region_plan > PLAN_DEFENSE
                    || (veh_base_who == faction_id
                    && (region_plan == PLAN_DEFENSE || (plr.region_flags[region] & REGION_UNK_4000))
                    && stack_check(veh_id, 11, VSTATE_UNK_10000000, VSTATE_UNK_10000000, -1) < 1)) {
                        if (!(veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000))) {
                            uint8_t offensive = plr.region_total_offensive_units[region];
                            if (offensive < max_defenders) {
                                plan = PLAN_DEFENSE;
                                plr.region_total_offensive_units[region] = offensive + 1;
                                veh.state |= VSTATE_UNK_10000000;
                            }
                        }
                    }
                }
            }
            int triad = veh.triad();
            if (triad != TRIAD_AIR && is_ocean(veh_sq) != (triad == TRIAD_SEA)) {
                if (veh_base_who < 0) {
                    continue;
                }
                // Fix: remove unused region change code when land/sea units are located at
                // sea/land tiles because the original compared contradictory statements.
            }
            if (plan == PLAN_COMBAT) {
                uint8_t region_plan = plr.region_base_plan[region];
                plan = (region_plan == PLAN_DEFENSE || region_plan == PLAN_COLONY) ? PLAN_DEFENSE : PLAN_OFFENSE;
            }
            if (plr.target_x >= 0 && (veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000)) == VSTATE_UNK_40000) {
                veh.order = ORDER_MOVE_TO;
                veh.status_icon = '0';
                veh.waypoint_x[0] = plr.target_x;
                veh.waypoint_y[0] = plr.target_y;
                if (veh.plan() == PLAN_COLONY) {
                    veh.movement_turns = 0;
                }
                continue;
            }
            if (plan == PLAN_NAVAL_TRANSPORT) {
                int loaded = stack_check(veh_id, 8, -1, -1, -1);
                if (loaded < veh_cargo(veh_id)) {
                    continue;
                }
            }
            if (veh.status_icon == 'F' && plan == PLAN_DEFENSE) {
                continue;
            }
            bool reassign = false;
            if ((veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000)) == (VSTATE_UNK_40000|VSTATE_UNK_20000)) {
                plan = PLAN_OFFENSE;
                reassign = true;
            }
            bool wants_goal = true;
            if (veh.order == ORDER_HOLD && !reassign && veh_base_who >= 0) {
                int artifact_count = stack_check(veh_id, 2, PLAN_ARTIFACT, -1, -1);
                wants_goal = false;
                if (stack_check(veh_id, 1, -1, -1, -1) - artifact_count > 1) {
                    if (plan != PLAN_DEFENSE && plan != PLAN_RECON && plan != PLAN_COMBAT) {
                        wants_goal = true;
                    } else {
                        int stack_base_id = base_at(veh_x, veh_y);
                        int guards = stack_check(veh_id, 16, -1, -1, -1);
                        wants_goal = guards >= garrison_check(stack_base_id);
                    }
                }
            }
            if (!wants_goal) {
                continue;
            }
            if (plan == PLAN_TERRAFORM && (veh.flags & VFLAG_UNK_800)) {
                continue;
            }
            int best_cost = 99999;
            int best_goal = -1;
            for (int g = 0; g < 75; g++) {
                Goal& goal = plr.goals[g];
                if (goal.priority <= 0) {
                    continue;
                }
                int goal_type = goal.type;
                if (goal_type < 0) {
                    continue;
                }
                if ((goal_type & 0xF) != plan
                && (reassign || goal_type != AI_GOAL_SCOUT || plan > PLAN_RECON || veh.order == ORDER_HOLD)) {
                    continue;
                }
                if (!on_map(goal.x, goal.y)) {
                    goal.x = -1;
                    goal.y = -1;
                    continue;
                }
                MAP* goal_sq = mapsq(goal.x, goal.y);
                if (region != goal_sq->region) {
                    continue;
                }
                int unit_id = veh.unit_id;
                if (triad == TRIAD_AIR
                || is_ocean(goal_sq) != (triad == TRIAD_SEA)
                || Units[unit_id].group_id == 21) {
                    continue;
                }
                if (plan == PLAN_OFFENSE) {
                    if ((goal_sq->items & BIT_MONOLITH)
                    && (!want_monolith(veh_id) || reassign || goal_assigned[g])) {
                        continue;
                    }
                } else if (plan == PLAN_COLONY) {
                    if (base_territory(faction_id, goal.x, goal.y) >= 0) {
                        del_site(faction_id, AI_GOAL_COLONIZE, goal.x, goal.y, 0);
                        continue;
                    }
                } else if (plan == PLAN_RECON && has_abil(unit_id, ABL_EMPATH)) {
                    continue;
                }
                int goal_base_id = base_at(goal.x, goal.y);
                int cost = vector_dist(veh_x, veh_y, goal.x, goal.y) << 8;
                int assigned;
                if (reassign
                || (goal_base_id >= 0 && Bases[goal_base_id].faction_id == faction_id
                && Bases[goal_base_id].ai_plan_status)) {
                    assigned = 0;
                } else {
                    assigned = goal_assigned[g];
                }
                int priority = goal.priority - assigned;
                if (priority <= 0) {
                    continue;
                }
                if (reassign && goal_sq->base_who() < 0) {
                    continue;
                }
                if (goal_type == AI_GOAL_SCOUT && plan != PLAN_RECON) {
                    cost *= 2;
                    if ((veh.state & VSTATE_UNK_10000000) || goal_sq->base_who() < 0) {
                        continue;
                    }
                }
                if (assigned && (goal_type == AI_GOAL_SCOUT || goal_type == AI_GOAL_COLONIZE)) {
                    continue;
                }
                if (!(cost <= priority * Units[unit_id].speed() * (6 / (assigned + 1)) - assigned + 4
                || plr.region_base_plan[region] == plan
                || goal.type == AI_GOAL_SCOUT
                || (!assigned && goal.type == AI_GOAL_TERRAFORM_LAND))) {
                    continue;
                }
                if (goal.type == AI_GOAL_COLONIZE) {
                    int site;
                    if (goal_sq->items & BIT_BASE_RADIUS) {
                        site = 1;
                    } else {
                        site = goal_sq->val2 >> 4;
                        if (!site) {
                            site = world_site(goal.x, goal.y, 0);
                            site_set(goal.x, goal.y, site);
                        }
                        site = clamp(site, 1, 15);
                    }
                    priority *= site;
                }
                if (veh.state & VSTATE_UNK_10000000) {
                    int defender_stack = veh_at(goal.x, goal.y);
                    if (defender_stack >= 0) {
                        int designated = stack_check(defender_stack, 11, VSTATE_UNK_10000000, VSTATE_UNK_10000000, faction_id);
                        if (designated > (cost == 0)) {
                            continue;
                        }
                    }
                }
                if (goal.type == AI_GOAL_TERRAFORM_LAND) {
                    int goal_base = goal.base_id;
                    bool contested = goal_base >= 0 && (Bases[goal_base].state_flags & BSTATE_UNK_20);
                    if (contested && veh.home_base_id != goal_base) {
                        cost *= 3;
                    }
                    bool closer_former = false;
                    for (int i = 0; i < *VehCount; i++) {
                        VEH& other = Vehs[i];
                        if (other.faction_id == faction_id
                        && Units[other.unit_id].plan == PLAN_TERRAFORM
                        && i != veh_id) {
                            int other_cost = vector_dist(other.x, other.y, goal.x, goal.y) << 8;
                            if (contested && other.home_base_id != goal_base) {
                                other_cost *= 3;
                            }
                            if (other_cost < cost) {
                                closer_former = true;
                                break;
                            }
                        }
                    }
                    if (closer_former) {
                        continue;
                    }
                    if (goal_assigned[g] && (goal_base < 0 || veh.home_base_id != goal_base)) {
                        continue;
                    }
                }
                int owner = whose_territory(faction_id, goal.x, goal.y, 0, 0);
                int total_cost = cost;
                if (owner >= 0 && owner != faction_id) {
                    uint32_t owner_status = plr.diplo_status[owner];
                    if (!(owner_status & (DIPLO_VENDETTA|DIPLO_SHALL_BETRAY|DIPLO_UNK_800))) {
                        if (owner_status & DIPLO_UNK_100000) {
                            continue;
                        }
                        if (!(owner_status & DIPLO_PACT)) {
                            if (owner_status & DIPLO_TREATY) {
                                continue;
                            }
                            total_cost = 2 * cost;
                        }
                    }
                }
                if (goal.type == AI_GOAL_PREV_DEFEND) {
                    if (!has_abil(veh.unit_id, ABL_ARTILLERY) || veh.triad() != TRIAD_LAND) {
                        continue;
                    }
                    total_cost /= 2;
                }
                if (total_cost / priority < best_cost) {
                    best_cost = total_cost / priority;
                    best_goal = g;
                }
            }
            if (best_goal >= 0) {
                Goal& goal = plr.goals[best_goal];
                if (veh.status_icon < '3' || veh.status_icon > '5') {
                    goal_assigned[best_goal]++;
                }
                veh.order = ORDER_MOVE_TO;
                veh.status_icon = '1';
                veh.waypoint_x[0] = goal.x;
                veh.waypoint_y[0] = goal.y;
                if (veh.plan() == PLAN_COLONY) {
                    veh.movement_turns = 0;
                }
                if (goal.type == AI_GOAL_SCOUT) {
                    veh.state |= VSTATE_UNK_40000000;
                }
                if (goal.type == AI_GOAL_DEFEND) {
                    int base_id = base_at(goal.x, goal.y);
                    if (base_id >= 0 && Bases[base_id].faction_id == faction_id) {
                        Bases[base_id].state_flags |= BSTATE_UNK_8000000;
                        if (!is_human(faction_id)) {
                            mod_base_reset(base_id, 0);
                        }
                    }
                }
            }
        }
    }
    wipe_goals(faction_id);
}

