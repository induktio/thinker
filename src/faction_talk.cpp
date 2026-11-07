
#include "faction_talk.h"


static int random_val(int min_val, int max_val) {
    return game_random(min_val, max_val);
}

static int base_trade_value(int base_id, int faction_id, int faction_id_2) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return 0;
    }
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    BASE* base = &Bases[base_id];
    bool own_base = base->faction_id == faction_id_2;
    bool pact = has_pact(faction_id, faction_id_2);
    int hq_id = find_hq(faction_id_2);
    int value = 50*base->pop_size;
    int score = max(0, *DiffLevel*2 + clamp(*DiploFriction, 0, 20)
        + 2*clamp((plr.pop_total - cmp.pop_total)/32, -4, 4)
        + 2*clamp(plr.ranking - cmp.ranking, -4, 4)
        + 4*diplo_relation(faction_id, faction_id_2)
        + 2*plr.atrocities);

    for (int i = Fac_ID_First; i <= Fac_ID_Last; i++) {
        if (has_fac_built((FacilityId)i, base_id)) {
            value += max(0, Facility[i].cost) * 20;
        }
    }
    for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
        if (project_base((FacilityId)i) == base_id) {
            value += max(0, Facility[i].cost) * (own_base ? 60 : 40);
        }
    }
    MAP* sq;
    TileSearch ts;
    ts.init(base->x, base->y, TRIAD_AIR);
    int num_own = 0;
    int num_all = 0;
    while ((sq = ts.get_next()) != NULL && ts.dist <= 10 && num_all < 5) {
        if (sq->is_base()) {
            if (sq->owner == faction_id_2) {
                num_own++;
            }
            num_all++;
        }
    }
    for (const auto& m : iterate_tiles(base->x, base->y, 0, 21)) {
        if (base_find_3(m.x, m.y, -1, m.sq->region, -1, -1) == base_id) {
            if (m.sq->landmarks & ~(LM_DUNES|LM_SARGASSO|LM_UNITY)) {
                value += (m.sq->landmarks & LM_JUNGLE ? 20 : 15);
            }
            if (bonus_at(m.x, m.y) > 0) {
                value += 20;
            }
            if (m.sq->items & (BIT_MONOLITH|BIT_CONDENSER|BIT_THERMAL_BORE)) {
                value += 20;
            }
        }
    }
    if (own_base) {
        value += 50;
        for (int i = 0; i < *VehCount; i++) {
            if ((Vehs[i].x == base->x && Vehs[i].y == base->y)
            || Vehs[i].home_base_id == base_id) {
                value += max(0, Units[Vehs[i].unit_id].cost * 20);
            }
        }
        if (num_own > 0) {
            value = value * (num_own == num_all ? 5 : 3) / 2;
        }
        value = value * (base->assimilation_turns_left > 8 ? 3 : 4)
            * (score + (pact ? 32 : 40)) / (32 * 4);
    } else {
        if (num_own < num_all) {
            value = value * (num_own ? 3 : 2) / 4;
        }
    }
    if (hq_id >= 0) {
        int dist = map_range(base->x, base->y, Bases[hq_id].x, Bases[hq_id].y);
        value = value * (72 - clamp(dist, 8, 40)) / 64;
    }
    if (is_objective(base_id)) {
        value *= 2;
    }
    value = ((value + 25) / 25) * 25;
    debug("base_trade_value %s %d %d score: %d num_own: %d num_all: %d value: %d\n",
    base->name, faction_id, faction_id_2, score, num_own, num_all, value);
    flushlog();
    return value;
}

void __cdecl scan_prototypes(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    *DiploBestUnitID = -1;
    *DiploMentionUnitID = -1;
    int best_score = 0;
    int mention_score = 0;
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        int unit_id = faction_id_2 * MaxProtoFactionNum + i;
        UNIT* unit = &Units[unit_id];
        if (unit->is_active() && unit->is_prototyped()
        && !(unit->obsolete_factions & (1 << faction_id_2))) {
            bool is_better = true;
            int score = unit->speed()
                + 3 * unit->offense_value()
                + 4 * unit->defense_value()
                + 4 * (7 - (uint8_t)unit->plan) * (1 << 14);
            for (int j = 0; j < MaxProtoFactionNum && is_better; j++) {
                UNIT* other = &Units[faction_id * MaxProtoFactionNum + j];
                if (other->is_active() && other->is_prototyped()
                && !(other->obsolete_factions & (1 << faction_id))
                && other->plan == unit->plan) {
                    if (other->triad() == unit->triad()) {
                        if (other->reactor_id > unit->reactor_id) {
                            is_better = false;
                        }
                        if (other->speed() > unit->speed()) {
                            is_better = false;
                        }
                        int other_wpn = weap_strat(other->weapon_id, faction_id);
                        int unit_wpn = weap_strat(unit->weapon_id, faction_id);
                        // Fix: original used incorrect parameter weapon_id and not armor_id
                        int other_arm = arm_strat(other->armor_id, faction_id);
                        int unit_arm = arm_strat(unit->armor_id, faction_id);
                        if (other->speed() >= unit->speed()
                        && other_wpn >= unit_wpn
                        && other_arm >= unit_arm
                        && (other_wpn > unit_wpn || other_arm > unit_arm)) {
                            is_better = false;
                        }
                    }
                }
            }
            int final_score = (is_better ? score : score >> 4);
            if (final_score > best_score) {
                best_score = final_score;
                *DiploBestUnitID = unit_id;
            }
            if (plr.diplo_status[faction_id_2] & DIPLO_PACT) {
                if (unit->unit_flags & UNIT_UNK_20) {
                    continue;
                }
            } else if (unit->unit_flags & (UNIT_UNK_10|UNIT_UNK_20)) {
                continue;
            }
            if ((uint8_t)unit->plan <= PLAN_NAVAL_SUPERIORITY && unit->plan != PLAN_RECON) {
                if (!is_better) {
                    if ((plr.diplo_status[faction_id_2] & DIPLO_VENDETTA) && unit->plan == PLAN_DEFENSE) {
                        score >>= 4;
                    } else {
                        score = 0;
                    }
                }
                if (score > mention_score) {
                    mention_score = score;
                    *DiploMentionUnitID = unit_id;
                }
            }
        }
    }
}

void __cdecl mention_prototypes(int faction_id, int faction_id_2) {
    *DiploSecondFaction = faction_id_2;
    if (*DiploMentionUnitID >= 0 && wants_to_speak(faction_id, faction_id_2)) {
        for (int i = 0; i < MaxProtoFactionNum; i++) {
            Units[faction_id_2 * MaxProtoFactionNum + i].unit_flags |= UNIT_UNK_10;
        }
        int unit_id = *DiploMentionUnitID;
        parse_says(0, Units[unit_id].name, -1, -1);
        StrBuffer[0] = 0;
        say_stats_2(StrBuffer, unit_id);
        parse_says(1, StrBuffer, -1, -1);
        Units[unit_id].unit_flags |= UNIT_UNK_10;
        bool pact = (Factions[faction_id].diplo_status[faction_id_2] & DIPLO_PACT);
        // Fix: restore dialog for PROTOPACT that was previously never displayed due to
        // an inverted condition. The code could have transferred all prototypes between factions
        // but this version only attempts to transfer the currently mentioned prototype.
        if (pact) {
            if (X_dialog("PROTOPACT", faction_id_2)) {
                net_tech(faction_id, unit_id + Tech_ID_Units, faction_id_2, 1);
            }
        } else {
            X_dialog("PROTORIVAL", faction_id_2);
        }
        Units[unit_id].unit_flags |= UNIT_UNK_20;
        *DiploBestUnitID = -1;
        *DiploMentionUnitID = -1;
    }
}

int __cdecl pact_withdraw(int faction_id, int faction_id_2) {
    int num = 0;
    for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
        VEH* veh = &Vehs[veh_id];
        if (veh->faction_id != faction_id) {
            continue;
        }
        MAP* sq = mapsq(veh->x, veh->y);
        int base_id = base_find(veh->x, veh->y);
        // Fix: check for invalid veh coordinates
        if (!sq || base_id < 0 || (Bases[base_id].faction_id != faction_id_2
        && whose_territory(faction_id, veh->x, veh->y, 0, 0) != faction_id_2)) {
            continue;
        }
        bool is_sea = (veh->triad() == TRIAD_SEA);
        int best_id = -1;
        int best_dist = 9999;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction_id
            && (!is_sea || (is_coast(base->x, base->y, 0)
            && (sq->alt_level() >= ALT_SHORE_LINE || base_on_sea(i, sq->region))))) {
                int dist = vector_dist(base->x, base->y, veh->x, veh->y);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_id = i;
                }
            }
        }
        if (best_id >= 0) {
            veh_drop(veh_lift(veh_id), Bases[best_id].x, Bases[best_id].y);
            veh->order = ORDER_NONE;
            veh->state &= ~(VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
            if (veh->triad() == TRIAD_AIR) {
                veh->movement_turns = 0;
            }
            num++;
            if (MapWin->InVehMode && veh_id == MapWin->VehID) {
                Console_set_move(MapWin, 1);
            }
        }
    }
    return num;
}

void __cdecl pact_ends(int faction_id, int faction_id_2) {
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    *DiploSecondFaction = faction_id_2;
    treaty_off(faction_id, faction_id_2, DIPLO_PACT);
    int moved_1 = pact_withdraw(faction_id, faction_id_2);
    int moved_2 = pact_withdraw(faction_id_2, faction_id);
    draw_map(1);
    int num;
    if (faction_id == MapWin->cOwner) {
        parse_says(0, get_noun(faction_id_2), -1, -1);
        parse_says(1, get_adjective(faction_id_2), -1, -1);
        num = moved_1;
    } else if (faction_id_2 == MapWin->cOwner) {
        parse_says(0, get_noun(faction_id), -1, -1);
        parse_says(1, get_adjective(faction_id), -1, -1);
        num = moved_2;
    } else {
        if (spying(faction_id)
        || spying(faction_id_2)
        || (*GameState & STATE_OMNISCIENT_VIEW)
        || (Factions[MapWin->cOwner].diplo_status[faction_id] & DIPLO_PACT)
        || (Factions[MapWin->cOwner].diplo_status[faction_id_2] & DIPLO_PACT)) {
            parse_says(0, get_noun(faction_id), -1, -1);
            parse_says(1, get_noun(faction_id_2), -1, -1);
            parse_says(2, get_pact_hood(faction_id, faction_id_2), -1, -1);
            X_pop("ENDPACT3", 0);
        }
        return;
    }
    parse_num(0, num);
    parse_says(2, get_pact_hood(faction_id, faction_id_2), -1, -1);
    Popup_start(&cur_popup, PopupScriptFile, "ENDPACT1", -1, 0,
        *MultiplayerActive ? 0 : POPUP_EXTERNAL, 0);
    if (num) {
        Popup_start(&cur_popup, PopupScriptFile, "ENDPACT2", -1, 0,
            (*MultiplayerActive ? 0 : POPUP_EXTERNAL)|POPUP_PRE_INIT, 0);
    }
    BasePop_exec_3(&cur_popup, 0, 0);
}

void __cdecl pact_of_brotherhood(int faction_id, int faction_id_2) {
    *DiploSecondFaction = faction_id_2;
    net_treaty_on(faction_id_2, faction_id, DIPLO_UNK_1000000|DIPLO_PACT, 0);
    Factions[faction_id_2].diplo_patience[faction_id] = 0;
    Factions[faction_id].diplo_spoke[faction_id_2] = *CurrentTurn;
    diplomacy_check(faction_id, faction_id_2, 0);
    *DiploCanBetray = 0;
    ++Factions[faction_id].diplo_reputation;
    ++Factions[faction_id_2].diplo_reputation;
    diplomacy_caption(faction_id, faction_id_2);
    parse_says(0, get_title(faction_id_2), -1, -1);
    parse_says(1, get_name(faction_id_2), -1, -1);
    parse_says(2, get_noun(faction_id_2), -1, -1);
    parse_says(3, get_pact(faction_id), -1, -1);
    parse_says(4, get_phrase(faction_id, 0), -1, -1);
    parse_says(5, get_phrase(faction_id_2, 0), -1, -1);
    X_dialog("MAKEPACT", faction_id_2);
    net_maps(faction_id, faction_id_2, 1);
    parse_says(0, get_pact(faction_id), -1, -1);
    X_dialog("HERESTHEMAP", faction_id_2);
    scan_prototypes(faction_id, faction_id_2);
    mention_prototypes(faction_id, faction_id_2);
    if ((*DiffLevel <= 1 || tut_check2(0x1000)) && !*MultiplayerActive) {
        parse_says(0, get_noun(faction_id_2), -1, -1);
        parse_says(1, get_title(faction_id_2), -1, -1);
        parse_says(2, get_name(faction_id_2), -1, -1);
        parse_says(3, get_adjective(faction_id_2), -1, -1);
        X_pop_2(TutorFile, "PACT", 0);
    }
}

void __cdecl make_treaty(int faction_id, int faction_id_2) {
    *DiploSecondFaction = faction_id_2;
    net_treaty_on(faction_id_2, faction_id, DIPLO_UNK_1000000|DIPLO_UNK_200|DIPLO_TREATY, 1);
    Factions[faction_id_2].diplo_patience[faction_id] = 0;
    Factions[faction_id].diplo_spoke[faction_id_2] = *CurrentTurn;
    diplomacy_check(faction_id, faction_id_2, 0);
    ++Factions[faction_id].diplo_reputation;
    ++Factions[faction_id_2].diplo_reputation;
    diplomacy_caption(faction_id, faction_id_2);
    parse_says(0, get_title(faction_id), -1, -1);
    parse_says(1, get_name(faction_id), -1, -1);
    parse_says(2, get_noun(faction_id), -1, -1);
    parse_says(3, get_title(faction_id_2), -1, -1);
    parse_says(4, get_name(faction_id_2), -1, -1);
    parse_says(5, get_noun(faction_id_2), -1, -1);
    parse_says(6, get_noun_phrase(faction_id, 9), -1, -1);
    parse_says(7, get_noun_phrase(faction_id_2, 9), -1, -1);
    snprintf(StrBuffer, StrBufLen, "MAKETREATY%d", random_val(0, 2));
    X_dialog(StrBuffer, faction_id_2);
    if ((*DiffLevel <= 1 || tut_check2(0x800)) && !*MultiplayerActive) {
        parse_says(0, get_noun(faction_id_2), -1, -1);
        X_pop_2(TutorFile, "COMMERCE", 0);
    }
    if (territory(faction_id_2, faction_id, 2, 0, 0)) {
        parse_says(0, get_noun(faction_id_2), -1, -1);
        parse_says(1, get_adjective(faction_id_2), -1, -1);
        NetMsg_pop(NetMsg, "TIMETOLEAVE", 5000, 0, 0);
    }
}

void __cdecl pledge_truce(int faction_id, int faction_id_2, int flag) {
    *DiploSecondFaction = faction_id_2;
    net_treaty_on(faction_id_2, faction_id, DIPLO_UNK_200|DIPLO_TRUCE, 0);
    Factions[faction_id].diplo_spoke[faction_id_2] = *CurrentTurn;
    diplomacy_check(faction_id, faction_id_2, 0);
    ++Factions[faction_id].diplo_reputation;
    ++Factions[faction_id_2].diplo_reputation;
    if (flag) {
        diplomacy_caption(faction_id, faction_id_2);
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_says(2, get_noun(faction_id), -1, -1);
        parse_says(3, get_title(faction_id_2), -1, -1);
        parse_says(4, get_name(faction_id_2), -1, -1);
        parse_says(5, get_noun(faction_id_2), -1, -1);
        X_dialog("MAKETRUCE", faction_id_2);
    }
}

void __cdecl introduce(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    *DiploSecondFaction = faction_id_2;
    *DiploWinState = 1;
    diplomacy_caption(faction_id, faction_id_2);
    parse_says(0, get_title(faction_id), -1, -1);
    parse_says(1, get_name(faction_id), -1, -1);
    parse_says(2, get_title(faction_id_2), -1, -1);
    parse_says(3, get_name(faction_id_2), -1, -1);
    parse_says(4, get_noun(faction_id_2), -1, -1);
    parse_says(8, get_adjective(faction_id_2), -1, -1);
    char* adj_leader = get_adjective_leader(faction_id);
    parse_says(9, adj_leader, -1, -1);
    const char* kind = "";
    int roll;
    bool pact = (plr.diplo_status[faction_id_2] & DIPLO_PACT);
    bool flag = (pact || !*DiploHostility);
    if (!(plr.diplo_status[faction_id_2] & DIPLO_UNK_8000000)
    || ((*MultiplayerActive || *PbemActive) && (DiploStateA[faction_id_2] & DST_UNK_2))) {
        kind = "NEW";
        roll = random_val(0, 6);
        parse_says(6, get_phrase(faction_id_2, 1), -1, -1);
        if (roll == 2) {
            parse_says(5, get_noun(faction_id), -1, -1);
        } else if (roll == 3) {
            char buf[StrBufLen];
            const char* might = Might[clamp(7 - plr.ranking, 0, 6)].adj;
            if (*GameLanguage) {
                snprintf(buf, StrBufLen, "%s", might);
            } else {
                // Fix: check for both lower/uppercase
                bool is_vowel = strchr("AEIOUaeiou", might[0]) != NULL;
                snprintf(buf, StrBufLen, "%s %s", label_get(is_vowel ? TL_An : TL_A), might);
            }
            parse_says(5, buf, -1, -1);
        }
    } else if (plr.diplo_status[faction_id_2] & DIPLO_WANT_REVENGE) {
        kind = "MAD";
        roll = random_val(0, 5);
        switch (roll) {
            case 0:
                parse_says(5, get_phrase(faction_id, 3), -1, -1);
                parse_says(6, get_noun_phrase(faction_id, 8), -1, -1);
                break;
            case 1:
                parse_says(5, MFactions[faction_id].adj_insult_faction, -1, -1);
                parse_says(6, get_phrase(faction_id, 3), -1, -1);
                break;
            case 2:
                parse_says(6, get_noun_phrase(faction_id, 8), -1, -1);
                break;
            case 3:
                parse_says(8, get_phrase(faction_id, 6), -1, -1);
                break;
            case 4:
                parse_says(5, MFactions[faction_id].adj_insult_leader, -1, -1);
                parse_says(6, get_insult_leader(faction_id), -1, -1);
                parse_says(8, get_phrase(faction_id, 6), -1, -1);
                break;
            default:
                break;
        }
    } else {
        roll = random_val(0, 4);
        switch (roll) {
            case 0:
                parse_says(5, get_noun_phrase(faction_id, flag ? 9 : 10), -1, -1);
                break;
            case 1:
                parse_says(9, flag ? adj_leader : MFactions[faction_id].adj_insult_leader, -1, -1);
                break;
            case 2:
                parse_says(5, flag ? get_adjective_faction(faction_id)
                    : MFactions[faction_id].adj_insult_faction, -1, -1);
                break;
            case 3:
                parse_says(9, flag ? adj_leader : MFactions[faction_id].adj_insult_leader, -1, -1);
                parse_says(5, flag ? get_adjective_faction(faction_id)
                    : MFactions[faction_id].adj_insult_faction, -1, -1);
                break;
            default:
                break;
        }
    }
    snprintf(StrBuffer, StrBufLen, "INTRO%s%d", kind, roll);
    X_dialog(StrBuffer, faction_id_2);
    // Fix: previously AI mentioned (related to URGENTCOMM) their own planet busters each time when
    // DIPLO_UNK_4000 was not set. This also adds mood-related modifiers to sometimes mention them.
    if (cmp.planet_busters && (!(cmp.diplo_status[faction_id] & DIPLO_UNK_4000)
    || (!pact && clamp(*DiploFriction + *DiploHostility * 2, 0, 16) > 4 + random_val(0, 20)))) {
        X_dialog("PLANETBUSTER0", faction_id_2);
        net_set_treaty(faction_id_2, faction_id, DIPLO_UNK_4000, 1, 1);
    }
    if (plr.planet_busters && !(plr.diplo_status[faction_id_2] & DIPLO_UNK_4000)) {
        snprintf(StrBuffer, StrBufLen, "PLANETBUSTER%d", (cmp.planet_busters != 0) + 1);
        X_dialog(StrBuffer, faction_id_2);
        net_set_treaty(faction_id, faction_id_2, DIPLO_UNK_4000, 1, 1);
    }
}

int __cdecl mod_buy_tech(int faction_id, int faction_id_2, int counter_id, int high_price, int proposal_id) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];

    if (proposal_id == DiploProposalTechTrade
    && (counter_id == DiploCounterEnergyPayment
    || counter_id == DiploCounterNameAPrice
    || counter_id == DiploCounterResearchData)) {
        if (*DiploTradeOfferTech1 < 0) {
            return 0; // "no technical data in which you would be interested"
        }
        if (plr.sanction_turns > 0 || cmp.diplo_betrayed[faction_id]
        || want_revenge(faction_id_2, faction_id)) {
            X_dialog(random_val(0, 2) ? "REJTECHLATER0" : "REJTECHLATER1", faction_id_2);
            return 1; // skip additional dialog replies
        }
        int value = (tech_alt_val(*DiploTradeOfferTech1, faction_id_2, 0) + 50)
            * clamp(*DiffLevel + 3, 4, 8)
            * clamp(32 + clamp(*DiploFriction, 0, 20)
            + 4*diplo_relation(faction_id, faction_id_2)
            + 2*plr.atrocities, 16, 96) / 256;
        if (plr.ranking > cmp.ranking) {
            value = value * 5 / 4;
        }
        if (plr.pop_total > cmp.pop_total) {
            value = value * 5 / 4;
        }
        if (has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
            value = value * 3 / 4;
        } else if (has_treaty(faction_id, faction_id_2, DIPLO_TREATY)) {
            value = value * 7 / 8;
        }
        if (high_price) {
            value = value * 5 / 4;
        }
        if (plr.tech_cost > 50 && plr.tech_research_id == *DiploTradeOfferTech1) {
            value = (int)(value * (1.0 - clamp(
                1.0 * plr.tech_accumulated / plr.tech_cost, 0.0, 0.7)));
        }
        value = clamp((value / 25) * 25, 25, 10000);

        StrBuffer[0] = 0;
        say_tech(StrBuffer, *DiploTradeOfferTech1, 1);
        parse_says(0, StrBuffer, -1, -1);
        parse_num(0, value);

        if (8*plr.energy_credits < value) {
            X_dialog(random_val(0, 2) ? "REJTECHLATER0" : "REJTECHLATER1", faction_id_2);
            return 1;
        }
        if (plr.energy_credits < value) {
            X_dialog(random_val(0, 2) ? "BUYTECHHIGH0" : "BUYTECHHIGH1", faction_id_2);
            return 1;
        }
        if (X_dialog(random_val(0, 2) ? "BUYTECH0" : "BUYTECH1", faction_id_2) == 1) {
            net_energy(faction_id, -value, faction_id_2, value, 1);
            net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
        }
        return 1;
    }
    return buy_tech(faction_id, faction_id_2, counter_id, high_price, proposal_id);
}

int __cdecl mod_energy_trade(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    bool is_pact = has_treaty(faction_id, faction_id_2, DIPLO_PACT);
    bool is_treaty = has_treaty(faction_id, faction_id_2, DIPLO_TREATY);
    int prop_counter = *DiploCounterProposal;
    *DiploSecondFaction = faction_id_2;
    tech_analysis(faction_id, faction_id_2);

    if (*DiploCurrentProposal != DiploProposalNeedEnergy) {
        assert(0);
        return 0;
    }

    if (is_pact && prop_counter != DiploCounterLoanPayment) {
        if (cmp.ranking > plr.ranking) {
            // Modify value for possible free gift for pact factions
            int value = 25 * (min(cmp.energy_credits/2 - 50,
                cmp.energy_credits/16 + cmp.base_count*5 + *CurrentTurn) / 25);
            if (value < 25 || value > cmp.energy_credits/2
            || cmp.energy_credits <= 2 * plr.energy_credits
            || cmp.energy_credits <= 4 * *CurrentTurn
            || cmp.tech_ranking <= plr.tech_ranking
            || cmp.diplo_patience[faction_id] >= 2) {
                // skipped
            } else if (*DiploTradeWantTech1 < 0 || prop_counter == DiploCounterFriendship) {
                net_energy(faction_id, value, faction_id_2, -value, 1);
                cmp.diplo_patience[faction_id] += 2;
                parse_num(0, value);
                X_dialog(random_val(0, 2) ? "ENERGYFREEBIE0" : "ENERGYFREEBIE1", faction_id_2);
                return 0;
            }
        }
    }

    if (prop_counter == DiploCounterLoanPayment) {
        int friction = clamp(*DiploFriction, 0, 20);
        int score = 5 - friction
            - 8*diplo_relation(faction_id, faction_id_2)
            - 4*plr.atrocities
            + (is_pact ? 16 : (is_treaty ? 4 : 0))
            + (want_revenge(faction_id_2, faction_id) ? -20 : 0)
            + clamp((cmp.pop_total - plr.pop_total)/32, -8, 8)
            + clamp((cmp.labs_total - plr.labs_total)/64, -8, 8);

        for (int i = 1; i < MaxPlayerNum; i++) {
            if (Factions[i].base_count && i != faction_id && i != faction_id_2) {
                if (at_war(i, faction_id) && has_pact(i, faction_id_2)) {
                    score -= 5;
                }
                if (at_war(i, faction_id_2) && has_pact(i, faction_id)) {
                    score -= 5;
                }
                if (at_war(i, faction_id) && at_war(i, faction_id_2)) {
                    score += 5;
                }
            }
        }
        int reserve = max(0, min(cmp.energy_credits - 50 - 20*friction,
            cmp.energy_credits/16 + 40*max(4, cmp.base_count + plr.base_count)));
        int amount = (reserve
            * clamp(32 - friction - 4*diplo_relation(faction_id, faction_id_2)
            - 8*clamp(cmp.AI_fight, -1, 1)
            + 8*clamp(cmp.SE_economy_base, -1, 1)
            + 4*clamp(cmp.ranking - plr.ranking, -4, 4)
            + (is_pact ? 32 : (is_treaty ? 8 : 0)), 12, 96) / 256) / 20 * 20;
        int turns = clamp(50 - *DiffLevel*5 - friction + score/2, 10, 50);
        int payment = ((20 + friction/4 + *DiffLevel*2)*amount + 15) / (16*turns);

        debug("energy_trade %s score: %d friction: %d credits: %d reserve: %d amount: %d turns: %d payment: %d\n",
        MFactions[faction_id_2].filename, score, friction, cmp.energy_credits, reserve, amount, turns, payment);
        flushlog();

        if (plr.sanction_turns > 0 || score < -15) {
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            X_dialog_alt("modmenu", "REJECTIDEA", faction_id_2);
            return 0;
        }
        if (plr.loan_balance[faction_id_2] > 0) {
            X_dialog_alt("modmenu", "REJECTLOAN", faction_id_2);
            return 0;
        }
        if (amount < 10 || payment < 1 || score < 0 || cmp.energy_credits < 50) {
            X_dialog(random_val(0, 2) ? "REJENERGY0" : "REJENERGY1", faction_id_2);
            return 0;
        }
        parse_num(0, amount);
        parse_num(1, payment);
        parse_num(2, turns);
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);

        int value = X_dialog(random_val(0, 2) ? "ENERGYLOAN1" : "ENERGYLOAN2", faction_id_2);
        if (value == 1) {
            net_loan(faction_id, faction_id_2, turns * payment, payment);
            net_energy(faction_id, amount, faction_id_2, -amount, 1);
            return 1;
        }
        return 0;
    }

    if ((prop_counter != DiploCounterNameAPrice || *DiploTradeWantTech1 >= 0)
    && (prop_counter == DiploCounterResearchData || prop_counter == DiploCounterNameAPrice)) {
        bool difficult = cmp.AI_fight > 0 && cmp.AI_power > 0;
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        // Replace faction label reference on Believers with more generic code
        if (*DiploTradeWantTech1 >= 0 && (!difficult || cmp.SE_research_base > -2)) {
            int tech_value = tech_alt_val(*DiploTradeWantTech1, faction_id_2, 0);
            int alt_tech_value = (*DiploTradeWantTech2 < 0 ? 0 : tech_alt_val(*DiploTradeWantTech2, faction_id_2, 0));
            int cost_limit = min(cmp.energy_credits/4,
                cmp.energy_credits/16 + cmp.base_count*5 + *CurrentTurn);
            int cost_value = tech_value
                * clamp(64 - clamp(*DiploFriction, 0, 20)
                - 8*diplo_relation(faction_id, faction_id_2)
                - 4*plr.atrocities, 16, 96) / 64;
            if (plr.ranking > cmp.ranking) {
                cost_value = cost_value * 7 / 8;
            }
            if (plr.pop_total > cmp.pop_total) {
                cost_value = cost_value * 7 / 8;
            }
            if (is_pact) {
                cost_value = cost_value * 5 / 4;
            } else if (is_treaty) {
                cost_value = cost_value * 9 / 8;
            }
            cost_value = clamp((min(cost_value / 4, cost_limit) / 25) * 25, 25, 1000);

            if (cost_value > cmp.energy_credits) { // Fix: check for sufficient energy to buy techs
                X_dialog("REJSELLAFFORD", faction_id_2);
                return 0;
            }
            while (true) {
                StrBuffer[0] = 0;
                say_tech(StrBuffer, *DiploTradeWantTech1, 1);
                parse_says(0, StrBuffer, -1, -1);
                if (*DiploTradeWantTech2 >= 0) {
                    StrBuffer[0] = 0;
                    say_tech(StrBuffer, *DiploTradeWantTech2, 1);
                    parse_says(1, StrBuffer, -1, -1);
                }
                parse_num(0, cost_value);
                int value;
                if (*DiploTradeWantTech2 < 0) {
                    value = X_dialog(random_val(0, 2) ? "ENERGYTECH0" : "ENERGYTECH1", faction_id_2);
                } else {
                    value = X_dialog("ENERGYTECH2", faction_id_2);
                }
                if (!value) { // "Sorry, not interested."
                    break;
                }
                if (value == 3 || (value == 2 && *DiploTradeWantTech2 < 0)) { // Consult Datalinks.
                    help_topic(14, *DiploTradeWantTech1);
                } else {
                    if (value == 2) { // "No, but I will sell you $TECH1 for the same price."
                        // Replace faction label references on Hive/University with more generic code
                        if ((difficult && tech_value > alt_tech_value && cmp.SE_research_base > -2)
                        || (tech_value > 2*alt_tech_value && cmp.SE_research_base < 2)) {
                            X_dialog("REJSELLSECOND", faction_id_2);
                            return 0;
                        }
                        X_dialog("SELLSECOND", faction_id_2);
                        net_tech(faction_id_2, *DiploTradeWantTech2, faction_id, 0);
                    } else { // Transmit information on $TECH0.
                        net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 0);
                        *DiploTradeWantTech1 = *DiploTradeWantTech2;
                    }
                    InfoWin->override_state = 0;
                    *DiploTradeWantTech2 = -1;
                    net_energy(faction_id, cost_value, faction_id_2, -cost_value, 1);
                    return 1;
                }
            }
            return 0;
        }
        X_dialog("REJSELLNONE", faction_id_2);
        return 0;
    }
    // Energy trade rejected for any reason / DiploCounterFriendship
    X_dialog(random_val(0, 2) ? "REJENERGY0" : "REJENERGY1", faction_id_2);
    return 0;
}

void __cdecl threaten(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    *DiploSecondFaction = faction_id_2;
    // Modified rejection decision making while pact is active
    if (!*DiploCanBetray
    && has_pact(faction_id_2, faction_id)
    && !has_treaty(faction_id_2, faction_id, DIPLO_HAVE_SURRENDERED)
    && (*DiploCurrentProposal == DiploProposalTechTrade
    || *DiploCurrentProposal == DiploProposalNeedEnergy)) {
        int friction = clamp(*DiploFriction, 0, 20);
        int score = 2*(*DiffLevel + friction)
            + 8*diplo_relation(faction_id, faction_id_2)
            + 8*clamp(cmp.AI_fight, -1, 1)
            + 4*clamp(cmp.ranking - plr.ranking, -4, 4)
            + clamp((cmp.pop_total - plr.pop_total)/32, -8, 8)
            + clamp((cmp.mil_strength_1 - plr.mil_strength_1)/32, -8, 8);
        if (score > random_val(0, 64)) {
            cmp.diplo_patience[faction_id] = 4 - (friction + 3) / 8;
            cause_friction(faction_id_2, faction_id, clamp(score/16, 2, 4));
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            parse_says(2, get_pact(faction_id), -1, -1);
            X_dialog("ENDPACT", faction_id_2);
            diplo_lock(0);
            net_pact_ends(faction_id, faction_id_2, 1);
            diplo_unlock();
            return;
        }
    }
    // Original threaten response starts here
    cmp.diplo_patience[faction_id] = get_patience(faction_id, faction_id_2);
    tech_analysis(faction_id, faction_id_2);
    if (*DiploCanBetray) {
        plr.diplo_spoke[faction_id_2] = *CurrentTurn;
        net_treaty_on(faction_id_2, faction_id, DIPLO_VENDETTA, 1);
        diplomacy_caption(faction_id, faction_id_2);
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_says(2, get_noun_phrase(faction_id, 8), -1, -1);
        snprintf(StrBuffer, StrBufLen, "PROVOKED%d", random_val(0, 2));
        X_dialog(StrBuffer, faction_id_2);
        return;
    }
    if ((plr.diplo_status[faction_id_2] & DIPLO_PACT) && !*DiploFearScale) {
        if (plr.ranking <= 5 && cmp.ranking >= 6 && *DiploCurrentProposal != DiploProposalJointAttack) {
            int score = cmp.ranking
                + cmp.tech_commerce_bonus
                + cmp.tech_ranking / 2
                - plr.tech_ranking / 2
                - plr.tech_commerce_bonus
                - cmp.AI_fight
                - plr.ranking;
            if (*DiploCurrentProposal == DiploProposalTechTrade) {
                score += cmp.AI_tech - cmp.AI_power;
            } else if (*DiploCurrentProposal == DiploProposalNeedEnergy) {
                score += cmp.AI_wealth - cmp.AI_power;
            }
            int min_fear = (cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED) ? 4 : 0;
            *DiploFearScale = clamp(score, min_fear, 999);
        }
    }
    if ((cmp.diplo_status[faction_id] & DIPLO_PACT)
    && (cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED) && *DiploFearScale < 2) {
        *DiploFearScale = 2;
    }
    int value = 25 * (*DiploFearScale) * (*CurrentTurn - *DiploSpokeTurn) / 16;
    if (value > cmp.energy_credits) {
        value = 25 * (cmp.energy_credits / 25);
    }
    if (value < 0) {
        value = 0;
    }
    if (!*DiploFearScale) {
        cause_friction(faction_id_2, faction_id, 2);
    }
    diplomacy_caption(faction_id, faction_id_2);
    bool extorted = false;
    int& tgt_fc_id = *DiploProposalFaction;
    if (*DiploCurrentProposal == DiploProposalJointAttack) {
        if (cmp.diplo_status[tgt_fc_id] & DIPLO_VENDETTA) {
            parse_says(0, get_title(tgt_fc_id), -1, -1);
            parse_says(1, get_name(tgt_fc_id), -1, -1);
            X_dialog("REJWARALREADY", faction_id_2);
            return;
        }
        bool skip_pact = !(plr.diplo_status[faction_id_2] & DIPLO_PACT)
            || ((!(plr.diplo_status[tgt_fc_id] & DIPLO_VENDETTA)
            || (cmp.diplo_status[tgt_fc_id] & DIPLO_PACT))
            && !(cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED));
        if (!skip_pact) {
            if (mod_wants_to_attack(faction_id_2, tgt_fc_id, 0)
            || mod_wants_to_attack(tgt_fc_id, faction_id_2, 0)
            || (cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED)
            || (cmp.ranking <= 4 && plr.ranking >= 6)
            || ((plr.diplo_status[tgt_fc_id] & DIPLO_WANT_REVENGE)
            && Factions[tgt_fc_id].ranking > plr.ranking)
            || (Factions[tgt_fc_id].ranking > plr.ranking
            && Factions[tgt_fc_id].ranking > cmp.ranking)) {
                parse_says(0, get_title(tgt_fc_id), -1, -1);
                parse_says(1, get_name(tgt_fc_id), -1, -1);
                parse_says(2, get_noun(tgt_fc_id), -1, -1);
                parse_says(3, get_pact(faction_id), -1, -1);
                X_dialog("WARPACT", faction_id_2);
                net_double_cross(faction_id_2, tgt_fc_id, faction_id, 1);
                extorted = true;
            }
        }
        if ((skip_pact || *DiploCurrentProposal == DiploProposalJointAttack) && *DiploFearScale) {
            // Fix: cast to int64_t to prevent possible signed overflow
            if (!(cmp.diplo_status[tgt_fc_id] & DIPLO_PACT)
            && ((plr.diplo_status[faction_id_2] & DIPLO_TREATY)
            || !(cmp.diplo_status[tgt_fc_id] & DIPLO_TREATY))
            && (int64_t)*DiploFearScale * plr.mil_strength_1
            > (int64_t)Factions[tgt_fc_id].mil_strength_1 * Factions[tgt_fc_id].mil_strength_1
            / (cmp.mil_strength_1 + 1)) {
                parse_says(0, get_title(tgt_fc_id), -1, -1);
                parse_says(1, get_name(tgt_fc_id), -1, -1);
                parse_says(2, get_noun(tgt_fc_id), -1, -1);
                X_dialog("WARTHREAT", faction_id_2);
                net_double_cross(faction_id_2, tgt_fc_id, faction_id, 1);
                extorted = true;
            }
        }
    }
    if (value && *DiploCurrentProposal == DiploProposalNeedEnergy) {
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_num(0, value);
        X_dialog("EXTORTENERGY", faction_id_2);
        net_energy(faction_id, value, faction_id_2, -value, 1);
        InfoWin->override_state = 0;
        Console_update_data(MapWin, 0);
        extorted = true;
    }
    if (*DiploFearScale && *DiploCurrentProposal == DiploProposalTradeCommlink) {
        parse_says(0, get_adjective(tgt_fc_id), -1, -1);
        X_dialog("EXTORTCOMMLINK", faction_id_2);
        net_tech(faction_id, tgt_fc_id + Tech_ID_Commlink, faction_id_2, 1);
        extorted = true;
    }
    if (*DiploTradeOfferTech1 >= 0 && *DiploFearScale && *DiploCurrentProposal == DiploProposalTechTrade) {
        parse_says(0, get_title(faction_id), -1, -1);
        StrBuffer[0] = 0;
        say_tech(StrBuffer, *DiploTradeOfferTech1, 1);
        parse_says(1, StrBuffer, -1, -1);
        X_dialog("EXTORTTECH", faction_id_2);
        net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
        *DiploTradeOfferTech1 = -1;
        extorted = true;
    }
    if (*DiploCurrentProposal == DiploProposalBaseSwap) {
        if (cmp.diplo_status[faction_id] & DIPLO_PACT) {
            int region = region_at(Bases[*DiploAskBaseSwapID].x, Bases[*DiploAskBaseSwapID].y);
            if (!(cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED)
            || cmp.base_count <= 1
            || ((plr.diplo_agenda[faction_id_2] & AGENDA_UNK_1000)
            && Bases[*DiploAskBaseSwapID].faction_id_former != faction_id
            && (plr.region_total_bases[region] < cmp.region_total_bases[region]
            || cmp.region_total_bases[region] > cmp.base_count / 2))) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                X_dialog("BASENOCEDE", faction_id_2);
                return;
            }
            net_agenda_on(faction_id, faction_id_2, AGENDA_UNK_1000, 1);
        }
        int is_hostile = *DiploPrevHostility || *DiploAttackChance;
        int base_value = value_of_base(*DiploAskBaseSwapID, faction_id_2, faction_id, *DiploFearScale, is_hostile);
        if (base_value > 0 && base_value < 25 * (*DiploFearScale)) {
            diplo_lock(0);
            parse_says(0, Bases[*DiploAskBaseSwapID].name, -1, -1);
            net_cede_base(*DiploAskBaseSwapID, faction_id, 1);
            X_dialog("BASEOK", faction_id_2);
            if (!(cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED)) {
                net_set_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE, 1, 1);
            }
            diplo_unlock();
            extorted = true;
        }
    }
    if (extorted) {
        if (!(plr.diplo_status[faction_id_2] & DIPLO_TREATY)) {
            net_treaty_on(faction_id_2, faction_id, DIPLO_TRUCE, 1);
        }
        return;
    }
    if (cmp.diplo_status[faction_id] & DIPLO_PACT) {
        if (*DiploPrevHostility || *DiploGreatSatan || *DiploFriction > 10) {
            if ((plr.ranking > cmp.ranking || plr.tech_ranking > cmp.tech_ranking)
            && !(cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED) && plr.ranking >= 5) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_pact(faction_id), -1, -1);
                X_dialog("ENDPACT", faction_id_2);
                diplo_lock(0);
                net_pact_ends(faction_id, faction_id_2, 1);
                diplo_unlock();
            }
        }
        parse_says(0, get_name(faction_id), -1, -1);
        X_dialog("FEND", faction_id_2);
    } else if (*DiploFriction > (*DiploPrevHostility != 0 ? 5 : 15)
    && !*DiploFearScale
    && (cmp.best_weapon_value >= plr.best_weapon_value
    || *DiploAttackChance || *DiploExposedBaseCount)
    && cmp.mil_strength_1 > plr.mil_strength_1 * (plr.diff_level / 2 + plr.integrity_blemishes + 4) / 4) {
        net_treaty_on(faction_id, faction_id_2, DIPLO_VENDETTA, 1);
        plr.diplo_spoke[faction_id_2] = *CurrentTurn;
        diplomacy_caption(faction_id, faction_id_2);
        snprintf(StrBuffer, StrBufLen, "PROVOKED%d", random_val(0, 2));
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_says(2, get_noun_phrase(faction_id, 8), -1, -1);
        X_dialog(StrBuffer, faction_id_2);
    } else {
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        X_dialog("IGNORE", faction_id_2);
    }
}

int __cdecl suggest_plan(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    // Fix 52. [BUG] Fixed an issue where diplomacy dialog could be incorrectly displayed
    // due to DiploSecondFaction value being set incorrectly. This is best exhibited where
    // Progenitors switch into "Human" dialog syntax.
    int best_score = 9999;
    int best_base_id = -1;
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE* base = &Bases[base_id];
        int owner = base->faction_id;
        if (owner != faction_id && owner != faction_id_2
        && (plr.diplo_status[owner] & DIPLO_VENDETTA)
        && (cmp.diplo_status[owner] & DIPLO_VENDETTA)
        && (base->visibility & (1 << faction_id))) {
            int dist = 0;
            int region = region_at(base->x, base->y);
            if (plr.target_x >= 0) {
                dist = 2 * vector_dist(plr.target_x, plr.target_y, base->x, base->y);
                if (region != region_at(plr.target_x, plr.target_y)) {
                    dist += 512;
                }
            }
            if (base_find_3(base->x, base->y, faction_id, region, -1, -1) < 0) {
                base_find_2(base->x, base->y, faction_id);
                *BaseFindDist += 1024;
            }
            int score = *BaseFindDist * ((plr.target_x >= 0) + 1) + dist;
            if (base_find_3(base->x, base->y, faction_id_2, region, -1, -1) < 0) {
                base_find_2(base->x, base->y, faction_id_2);
                *BaseFindDist += 256;
            }
            if (plr.base_id_attack_target == base_id) {
                return base_id;
            }
            if (*BaseFindDist + score < best_score) {
                best_score = *BaseFindDist + score;
                best_base_id = base_id;
            }
        }
    }
    return best_base_id;
}

int __cdecl attack_from(int base_id, int faction_id) {
    Faction& plr = Factions[faction_id];
    int best_dist = 9999;
    int best_base_id = 0;
    int region = region_at(Bases[base_id].x, Bases[base_id].y);
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            int dist = vector_dist(Bases[i].x, Bases[i].y, Bases[base_id].x, Bases[base_id].y);
            if (Bases[i].x == plr.target_x && Bases[i].y == plr.target_y) {
                dist = 0;
            }
            if (region_at(Bases[i].x, Bases[i].y) != region) {
                dist += 1000;
            }
            if (dist <= best_dist) {
                best_dist = dist;
                best_base_id = i;
            }
        }
    }
    return best_base_id;
}

void __cdecl battle_routine(int base_id) {
    int player_from = attack_from(base_id, *DiploBattlePlayer);
    int rival_from = attack_from(base_id, *DiploBattleRival);
    PlanWin_clear_lines(PlanWin);
    PlanWin_add_line_2(PlanWin, player_from, base_id, DrawFcColorA[*DiploBattlePlayer]);
    PlanWin_add_line_2(PlanWin, rival_from, base_id, DrawFcColorA[*DiploBattleRival]);
    PlanWin_on_redraw((PlanWindow*)&PlanWin->graphicWin);
}

void __cdecl battle_plans(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    *DiploSecondFaction = faction_id_2;
    *DiploBattlePlayer = faction_id;
    *DiploBattleRival = faction_id_2;
    int player_target = suggest_plan(faction_id, faction_id_2);
    int rival_target = suggest_plan(faction_id_2, faction_id);
    if (!(plr.diplo_status[faction_id_2] & DIPLO_PACT)) {
        if ((cmp.diplo_status[faction_id] & (DIPLO_UNK_8000|DIPLO_WANT_REVENGE))
        || *DiploCapturedBases || *DiploFriction >= 18 || *DiploHostility) {
            X_dialog("NEVERBATTLEPLANS", faction_id_2);
            return;
        }
        if (!*DiploMapsTraded && !X_dialog("MAPBATTLEPLANS", faction_id_2)) {
            return;
        }
        net_maps(faction_id, faction_id_2, 1);
    }
    if (player_target < 0 || rival_target < 0) {
        X_dialog("NOBATTLEPLANS", faction_id_2);
        return;
    }
    // Fix: game called attack_from with base_id -1 when no target was found
    int player_from = attack_from(player_target, faction_id);
    int rival_from = attack_from(rival_target, faction_id_2);
    PlanWin_init(PlanWin, 0);
    PlanWin_find_view(PlanWin,
        Bases[player_target].x, Bases[player_target].y,
        Bases[rival_target].x, Bases[rival_target].y,
        Bases[player_from].x, Bases[player_from].y,
        Bases[rival_from].x, Bases[rival_from].y);
    PlanWin_back_redraw(PlanWin);
    PlanWin_add_line_2(PlanWin, player_from, player_target, DrawFcColorA[faction_id]);
    PlanWin_add_line_2(PlanWin, rival_from, rival_target, DrawFcColorA[faction_id_2]);
    Win_show((Win*)((char*)PlanWin + ((DWORD*)PlanWin->vtable)[1]), 0);
    cmp.base_id_attack_target = rival_target;
    parse_says(0, Bases[rival_target].name, -1, -1);
    parse_says(1, Bases[player_target].name, -1, -1);
    if (X_dialog("BATTLEPLANS", faction_id_2)) {
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        Popup_start(&cur_popup, PopupScriptFile, "BATTLEPLAN", -1, 0,
            POPUP_EXTERNAL|POPUP_BTN_CANCEL|POPUP_LISTBOX, 0);
        Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], faction_id_2, 0);
        int item_count = 0;
        for (int i = 0; i < *BaseCount; i++) {
            int owner = Bases[i].faction_id;
            if (owner != faction_id && owner != faction_id_2
            && (plr.diplo_status[owner] & DIPLO_VENDETTA)
            && (cmp.diplo_status[owner] & DIPLO_VENDETTA)
            && (Bases[i].visibility & ((1 << faction_id) | (1 << faction_id_2)))) {
                Dialogs_item(&cur_popup.dialogs, Bases[i].name, i);
                if (!item_count) {
                    battle_routine(i);
                }
                item_count++;
            }
        }
        if (item_count) {
            cur_popup.dialogs.listBox.cb_select_item = battle_routine;
            int chosen_id = BasePop_exec_3(&cur_popup, 0, pop_wait);
            if (chosen_id >= 0) {
                parse_says(0, Bases[chosen_id].name, -1, -1);
                if (Bases[chosen_id].faction_id != faction_id_2
                && !(Bases[chosen_id].visibility & (1 << faction_id_2))) {
                    spot_loc(Bases[chosen_id].x, Bases[chosen_id].y, faction_id_2);
                }
                int chosen_region = region_at(Bases[chosen_id].x, Bases[chosen_id].y);
                int rival_region = region_at(Bases[rival_target].x, Bases[rival_target].y);
                if ((chosen_region >= MaxRegionLandNum || !cmp.region_total_bases[chosen_region])
                && cmp.region_total_bases[rival_region]) {
                    int region = 0;
                    for (; region < MaxRegionLandNum; region++) {
                        if (!bad_reg(region)
                        && cmp.region_total_bases[region]
                        && cmp.region_base_plan[region] == PLAN_NAVAL_TRANSPORT) {
                            break;
                        }
                    }
                    if (region >= MaxRegionLandNum) {
                        X_dialog("BATTLENO", faction_id_2);
                        PlanWin_close(PlanWin);
                        return;
                    }
                }
                base_find_2(Bases[rival_target].x, Bases[rival_target].y, faction_id_2);
                int rival_dist = *BaseFindDist;
                base_find_2(Bases[chosen_id].x, Bases[chosen_id].y, faction_id_2);
                if (*BaseFindDist < 2 * rival_dist) {
                    cmp.base_id_attack_target = chosen_id;
                    X_dialog("BATTLEYES", faction_id_2);
                    if (cmp.target_x >= 0
                    && region_at(cmp.target_x, cmp.target_y) != region_at(Bases[chosen_id].x, Bases[chosen_id].y)) {
                        for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
                            if (Vehs[veh_id].faction_id == faction_id_2 && (Vehs[veh_id].state & VSTATE_UNK_40000)) {
                                Vehs[veh_id].state &= ~(VSTATE_UNK_40000|VSTATE_UNK_20000);
                            }
                        }
                        cmp.target_x = -1;
                        cmp.target_y = -1;
                        cmp.unk_71 = 0;
                        cmp.unk_72 = 0;
                    }
                } else {
                    X_dialog("BATTLENO", faction_id_2);
                }
            }
        }
    }
    PlanWin_close(PlanWin);
}

void __cdecl call_off_vendetta(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    *DiploSecondFaction = faction_id_2;
    Popup_start(&cur_popup, PopupScriptFile, "CALLOFF", -1, 0,
        POPUP_EXTERNAL|POPUP_BTN_CANCEL|POPUP_LISTBOX, 0);
    Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], faction_id_2, 0);
    int item_count = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_alive(i) && i != faction_id && i != faction_id_2
        && (cmp.diplo_status[i] & DIPLO_VENDETTA)) {
            if ((plr.diplo_status[i] & DIPLO_COMMLINK)
            && (plr.diplo_status[i] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))) {
                if (X_text_open(ScriptFile, "ATTACKSTRING")) {
                    break;
                }
                text_get();
                parse_says(0, get_title(i), -1, -1);
                parse_says(1, get_name(i), -1, -1);
                parse_says(2, get_noun(i), -1, -1);
                StrBuffer[0] = 0;
                parse_string(text_buf_ptr(), StrBuffer);
                item_count++;
                Dialogs_item(&cur_popup.dialogs, StrBuffer, i);
            }
        }
    }
    if (!item_count) {
        return;
    }
    int chosen_id = BasePop_exec_3(&cur_popup, 0, pop_wait);
    if (chosen_id < 0) {
        return;
    }
    parse_says(0, get_title(chosen_id), -1, -1);
    parse_says(1, get_name(chosen_id), -1, -1);
    parse_says(2, get_adjective_insult_faction(chosen_id), -1, -1);
    parse_says(3, get_noun(chosen_id), -1, -1);
    if ((cmp.diplo_status[chosen_id] & DIPLO_WANT_REVENGE)
    || (Factions[chosen_id].major_atrocities && !cmp.major_atrocities)) {
        X_pops_11("TOTHEDEATH", POPUP_EXTERNAL, FactionPortraits[faction_id_2], 0);
        return;
    }
    if ((cmp.diplo_status[chosen_id] & DIPLO_UNK_40)
    && !(cmp.diplo_status[faction_id] & DIPLO_UNK_80)) {
        X_pops_11("NOPEHIRED", POPUP_EXTERNAL, FactionPortraits[faction_id_2], 0);
        return;
    }
    net_set_treaty(faction_id_2, chosen_id, DIPLO_UNK_4000000, 1, 0);
    net_set_treaty(faction_id_2, chosen_id, DIPLO_UNK_40, 0, 0);
    if (mod_wants_to_attack(chosen_id, faction_id_2, 0)) {
        X_pops_11("THEIRFAULT", POPUP_EXTERNAL, FactionPortraits[faction_id_2], 0);
        return;
    }
    X_pops_11("CALLEDOFF", POPUP_EXTERNAL, FactionPortraits[faction_id_2], 0);
    if (*MultiplayerActive) {
        log_say_2("Sending TREATY ON", faction_id_2, chosen_id, DIPLO_TREATY);
        message_data(0x2441, 0, faction_id_2, chosen_id, DIPLO_TREATY, 0);
        NetDaemon_await_diplo(NetState, 0x441);
    } else {
        enemies_treaty(faction_id_2, chosen_id, 0);
    }
}

int __cdecl mod_base_swap(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    bool surrender = has_treaty(faction_id_2, faction_id, DIPLO_PACT)
        && has_treaty(faction_id_2, faction_id, DIPLO_HAVE_SURRENDERED);

    if (*MultiplayerActive || *PbemActive || *DiploAskBaseSwapID < 0 || is_human(faction_id_2)) {
        if (surrender) {
            return base_swap(faction_id, faction_id_2);
        }
        X_dialog("NEVERBASESWAP", faction_id_2);
        return 0;
    }
    if (want_revenge(faction_id_2, faction_id) || cmp.diplo_betrayed[faction_id] > 0) {
        X_dialog("NEVERBASESWAP", faction_id_2);
        return 0;
    }
    if (*DiploCounterProposal == DiploCounterThreaten) {
        cause_friction(faction_id_2, faction_id, 2);
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        X_dialog("BASENOCEDE", faction_id_2);
        return 0;
    }
    int cost_ask = base_trade_value(*DiploAskBaseSwapID, faction_id, faction_id_2);
    parse_says(0, Bases[*DiploAskBaseSwapID].name, -1, -1);
    parse_num(0, cost_ask);

    if (cost_ask < 1 || cmp.base_count == 1
    || has_fac_built(FAC_HEADQUARTERS, *DiploAskBaseSwapID)) {
        X_dialog("NOBASESWAP2", faction_id_2);
        return 0;
    }
    if (*DiploCounterProposal == DiploCounterEnergyPayment
    || *DiploCounterProposal == DiploCounterNameAPrice) {
        if (cost_ask > 5000 + clamp(*CurrentTurn, 0, 500)*40) {
            X_dialog("NOBASESWAP2", faction_id_2);
            return 0;
        }
        if (8*plr.energy_credits < cost_ask) {
            X_dialog("NOBASESWAP2", faction_id_2);
            return 0;
        }
        if (plr.energy_credits < cost_ask) {
            X_dialog("NOBASESWAP", faction_id_2);
            return 0;
        }
        if (!X_dialog("PAYBASESWAP", faction_id_2)) {
            return 0;
        }
        // net_energy also calls Console_update_data and StatusWin_redraw
        net_cede_base(*DiploAskBaseSwapID, faction_id, 1);
        net_energy(faction_id, -cost_ask, faction_id_2, cost_ask, 1);
        draw_map(1);
        return 0;
    }
    if (*DiploCounterProposal == DiploCounterGiveBase && *DiploBidBaseSwapID >= 0) {
        int cost_bid = base_trade_value(*DiploBidBaseSwapID, faction_id, faction_id_2);
        parse_says(0, Bases[*DiploAskBaseSwapID].name, -1, -1);
        parse_says(1, Bases[*DiploBidBaseSwapID].name, -1, -1);

        if (cost_bid < 1 && !surrender) {
            X_dialog("NOBASESWAP2", faction_id_2);
            return 0;
        }
        if (4*cost_bid < cost_ask && !surrender) {
            X_dialog("NOBASESWAP3", faction_id_2);
            return 0;
        }
        if (cost_bid < cost_ask && !surrender) {
            X_dialog("NOBASESWAP", faction_id_2);
            return 0;
        }
        X_dialog("YESBASESWAP", faction_id_2);
        net_cede_base(*DiploAskBaseSwapID, faction_id, 1);
        net_cede_base(*DiploBidBaseSwapID, faction_id_2, 1);
        StatusWin_redraw(StatusWin);
        draw_map(1);
        return 0;
    }
    if (*DiploCounterProposal == DiploCounterFriendship) {
        // skipped
    }
    if (*DiploCounterProposal == DiploCounterResearchData) {
        // skipped
    }
    parse_says(0, get_title(faction_id), -1, -1);
    parse_says(1, get_name(faction_id), -1, -1);
    X_dialog_alt("modmenu", "REJECTIDEA", faction_id_2);
    return 0;
}

