
#include "faction_talk.h"


static int random_val(int min_val, int max_val) {
    return game_random(min_val, max_val);
}

static int tech_owner_count(int tech_id) {
    // Fix: the original did not check tech_id bounds when
    // considering any pseudo tech_ids outside TechOwners.
    if (tech_id < 0 || tech_id >= MaxTechnologyNum) {
        return 1;
    }
    return bit_count(TechOwners[tech_id]);
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

void __cdecl demands_withdrawal(int faction_id, int faction_id_2) {
    *DiploSecondFaction = faction_id_2;
    if (!commlink_attempter(faction_id, faction_id_2)
    && (!*MultiplayerActive || !is_human(faction_id_2))) {
        int result = demand_withdrawal(faction_id, faction_id_2);
        if (*SunspotDuration > 0) {
            parse_says(0, get_noun(faction_id_2), -1, -1);
            switch (result) {
                case 0:
                    NetMsg_pop(NetMsg, "WITHSUNYES", 5000, 0, 0);
                    break;
                case 1:
                    NetMsg_pop(NetMsg, "WITHSUNNO", 5000, 0, 0);
                    break;
                case 2:
                    NetMsg_pop(NetMsg, "WITHSUNWAR", 5000, 0, 0);
                    break;
            }
        }
    }
}

void __cdecl renounce_pact(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];

    *DiploSecondFaction = faction_id_2;
    if (!commlink_attempter(faction_id, faction_id_2)) {
        if (diplo_lock(1)) {
            return;
        }
        plr.integrity_blemishes++;
        net_treaty_off(faction_id_2, faction_id, DIPLO_PACT, 1);
        if (*SunspotDuration <= 0 && !is_human(faction_id_2)) {
            diplomacy_check(faction_id, faction_id_2, 0);
            diplomacy_caption(faction_id, faction_id_2);
            parse_says(0, get_pact(faction_id), -1, -1);
            parse_says(1, get_title(faction_id), -1, -1);
            parse_says(2, get_name(faction_id), -1, -1);
            X_dialog("BROKEPACT", faction_id_2);
        }
        net_pact_ends(faction_id, faction_id_2, 1);
        diplo_unlock();
    }
}

void __cdecl tech_analysis(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];

    *DiploTradeTechProject = -1;
    *DiploTradeWantTech4 = -1;
    *DiploTradeWantTech3 = -1;
    *DiploTradeWantTech2 = -1;
    *DiploTradeWantTech1 = -1;
    *DiploTradeOfferTech2 = -1;
    *DiploTradeOfferTech1 = -1;
    *DiploTradeOfferValue2 = 0;
    *DiploTradeWantValue2 = 0;
    *DiploTradeOfferValue1 = 0;
    *DiploTradeWantValue1 = 0;
    *DiploTradeWantValue4 = 0;
    *DiploTradeWantValue3 = 0;
    *DiploTradeOfferProject2 = -1;
    *DiploTradeOfferProject1 = -1;
    if (*GameRules & RULES_SCN_NO_TECH_TRADING) {
        return;
    }
    for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
        if (has_tech(tech_id, faction_id_2)) {
            if (!has_tech(tech_id, faction_id)) {
                int project = -1;
                if (plr.diff_level >= DIFF_LIBRARIAN
                || !strcmp(MFactions[faction_id_2].filename, "HIVE")) {
                    for (int i = 0; i < MaxSecretProjectNum; i++) {
                        if (SecretProjects[i] == SP_Unbuilt && Facility[SP_ID_First + i].preq_tech == tech_id) {
                            for (int base_id = 0; base_id < *BaseCount; base_id++) {
                                if (Bases[base_id].faction_id == faction_id_2
                                && Bases[base_id].queue_items[0] == -(SP_ID_First + i)) {
                                    project = i;
                                    *DiploTradeTechProject = i;
                                }
                            }
                            if (((plr.diff_level >= DIFF_TRANSCEND
                            && (!*MultiplayerActive || faction_id != *RankingFactionIDUnk2))
                            || (*MultiplayerActive && faction_id == *RankingFactionIDUnk1))
                            && *DiploTradeTechProject == -1) {
                                project = -2 - i;
                                *DiploTradeTechProject = -2 - i;
                            }
                        }
                    }
                }
                int value = mod_tech_val(tech_id, faction_id, 0);
                if (project != -1) {
                    value = (value + 1) / 2;
                }
                if (value < *DiploTradeOfferValue1) {
                    if (value >= *DiploTradeOfferValue2) {
                        *DiploTradeOfferValue2 = value;
                        *DiploTradeOfferTech2 = tech_id;
                        *DiploTradeOfferProject2 = project;
                    }
                } else {
                    *DiploTradeOfferValue2 = *DiploTradeOfferValue1;
                    *DiploTradeOfferProject2 = *DiploTradeOfferProject1;
                    *DiploTradeOfferTech2 = *DiploTradeOfferTech1;
                    *DiploTradeOfferValue1 = value;
                    *DiploTradeOfferTech1 = tech_id;
                    *DiploTradeOfferProject1 = project;
                }
            }
        } else if (has_tech(tech_id, faction_id)) {
            int value = mod_tech_val(tech_id, faction_id_2, 0);
            if (value < *DiploTradeWantValue1) {
                if (value < *DiploTradeWantValue2) {
                    if (value < *DiploTradeWantValue3) {
                        if (value >= *DiploTradeWantValue4) {
                            *DiploTradeWantValue4 = value;
                            *DiploTradeWantTech4 = tech_id;
                        }
                    } else {
                        *DiploTradeWantValue4 = *DiploTradeWantValue3;
                        *DiploTradeWantValue3 = value;
                        *DiploTradeWantTech4 = *DiploTradeWantTech3;
                        *DiploTradeWantTech3 = tech_id;
                    }
                } else {
                    *DiploTradeWantValue4 = *DiploTradeWantValue3;
                    *DiploTradeWantValue3 = *DiploTradeWantValue2;
                    *DiploTradeWantValue2 = value;
                    *DiploTradeWantTech4 = *DiploTradeWantTech3;
                    *DiploTradeWantTech3 = *DiploTradeWantTech2;
                    *DiploTradeWantTech2 = tech_id;
                }
            } else {
                *DiploTradeWantValue4 = *DiploTradeWantValue3;
                *DiploTradeWantValue3 = *DiploTradeWantValue2;
                *DiploTradeWantValue2 = *DiploTradeWantValue1;
                *DiploTradeWantValue1 = value;
                *DiploTradeWantTech4 = *DiploTradeWantTech3;
                *DiploTradeWantTech3 = *DiploTradeWantTech2;
                *DiploTradeWantTech2 = *DiploTradeWantTech1;
                *DiploTradeWantTech1 = tech_id;
            }
        }
    }
    if (*DiploTradeOfferProject2 != -1) {
        *DiploTradeOfferTech2 = -1;
    }
}

void __cdecl buy_council_vote(int faction_id, int faction_id_2, int proposal_id, int vote_id) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    auto read_line = []() {
        text_get();
        clear();
        parse_string(text_buf_ptr(), StrBuffer);
    };
    *DiploSecondFaction = faction_id_2;
    diplomacy_check(faction_id, faction_id_2, 0);
    diplomacy_caption(faction_id, faction_id_2);
    tech_analysis(faction_id, faction_id_2);
    if (is_alien(faction_id) || is_alien(faction_id_2)) {
        return;
    }
    if (cmp.diplo_status[faction_id]
    & (DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE|DIPLO_VENDETTA)) {
        if (*ControlWaitLoop) {
            parse_says(0, get_title(faction_id_2), -1, -1);
            parse_says(1, get_name(faction_id_2), -1, -1);
            X_pop("BUYNOTALK", pop_wait);
        }
        return;
    }
    int wants = wants_prop(faction_id_2, proposal_id, (vote_id > 0 ? vote_id : faction_id));
    if (proposal_id != PROP_UNITE_SUPREME_LEADER && *DiploFearScale) {
        wants++;
    }
    if (vote_id == COUNC_VOTE_NO) {
        wants = -wants;
    }
    if (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR) {
        int best = wants;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != vote_id && eligible(i)
            && best >= wants - wants_prop(faction_id_2, PROP_ELECT_PLANETARY_GOVERNOR, i)) {
                best = wants - wants_prop(faction_id_2, PROP_ELECT_PLANETARY_GOVERNOR, i);
            }
        }
        wants = best;
    }
    if (cmp.unk_29[proposal_id]) {
        int promised = cmp.unk_30[proposal_id];
        if (!(cmp.diplo_status[promised] & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA))) {
            parse_says(0, get_title(promised), -1, -1);
            parse_says(1, get_name(promised), -1, -1);
            X_dialog("BUYPROMISED", faction_id_2);
            return;
        }
    }
    if (cmp.SE_economy_base > 0 && proposal_id > PROP_UNITE_SUPREME_LEADER
    && !(cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE) && wants < -2) {
        wants = -2;
    }
    if (*ControlWaitLoop && vote_id == CouncWin->faction_vote[faction_id_2]) {
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        X_pop("BUYALREADY", pop_wait);
        if (*MultiplayerActive && (plr.diplo_status[faction_id_2] & DIPLO_TREATY)
        && !(cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)) {
            cmp.unk_29[proposal_id] = vote_id;
            cmp.unk_30[proposal_id] = (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR ? vote_id : faction_id);
            synch_leader(faction_id_2);
        }
        return;
    }
    int multiplier;
    uint32_t status = cmp.diplo_status[faction_id];
    if ((status & DIPLO_PACT) && (status & DIPLO_HAVE_SURRENDERED)) {
        multiplier = 0;
    } else {
        if (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR && vote_id == faction_id_2) {
            if (wants < 0) {
                X_dialog("BUYNO", faction_id_2);
            }
            return;
        }
        if (wants < -3) {
            X_dialog("BUYNO", faction_id_2);
            return;
        }
        if (wants != -3) {
            multiplier = (wants == -2 ? 3 : (wants == -1) + 1);
        } else if (proposal_id != PROP_ELECT_PLANETARY_GOVERNOR) {
            vote_id = COUNC_VOTE_ABSTAIN;
            multiplier = 3;
        } else {
            multiplier = 4;
        }
    }
    int rivals = 0;
    if (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id_2) {
                int votes = council_votes(i);
                if (votes > council_votes(faction_id_2)) {
                    rivals++;
                }
            }
        }
    }
    int buyer_income = max(plr.energy_surplus_total - plr.facility_maint_total,
        plr.base_count);
    int seller_income = max(cmp.energy_surplus_total - cmp.facility_maint_total,
        cmp.base_count);
    int cost = plr.energy_credits / 5 + 2 * multiplier * (buyer_income + seller_income);
    if (*DiploFearScale) {
        cost /= 2;
    }
    if (*DiploFearScale >= cmp.base_count) {
        cost /= 2;
    }
    if (faction_id_2 == *GovernorFaction) {
        cost *= 2;
    }
    if (!*ControlWaitLoop) {
        cost = 3 * cost / 4;
    }
    if (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR && !rivals
    && CouncWin->faction_vote[faction_id_2] == faction_id_2) {
        cost *= 2;
    }
    if (*DiploTradeWantTech1 >= 0) {
        int turn_factor = (*CurrentTurn + 7) / 8;
        int tech_total = turn_factor * mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1);
        int limit = 3 * cost / 4;
        if (tech_total >= limit || *DiploTradeWantTech2 < 0) {
            *DiploTradeWantTech4 = -1;
            *DiploTradeWantTech3 = -1;
            *DiploTradeWantTech2 = -1;
        } else {
            tech_total += turn_factor * mod_tech_val(*DiploTradeWantTech2, faction_id_2, 1);
            if (tech_total >= limit || *DiploTradeWantTech3 < 0) {
                *DiploTradeWantTech4 = -1;
                *DiploTradeWantTech3 = -1;
            } else {
                tech_total += turn_factor * mod_tech_val(*DiploTradeWantTech3, faction_id_2, 1);
                if (tech_total >= limit || *DiploTradeWantTech4 < 0) {
                    *DiploTradeWantTech4 = -1;
                } else {
                    tech_total += turn_factor * mod_tech_val(*DiploTradeWantTech4, faction_id_2, 1);
                }
            }
        }
        if (tech_total < limit) {
            *DiploTradeWantTech4 = -1;
            *DiploTradeWantTech3 = -1;
            *DiploTradeWantTech2 = -1;
            *DiploTradeWantTech1 = -1;
        }
    }
    clear();
    says("BUYVOTE");
    if (!cost) {
        say_num(0);
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        X_dialog(StrBuffer, faction_id_2);
    } else {
        if (proposal_id != PROP_ELECT_PLANETARY_GOVERNOR) {
            switch (vote_id) {
                case COUNC_VOTE_ABSTAIN:
                    says("ABSTAIN");
                    break;
                case COUNC_VOTE_NO:
                    says("NAY");
                    break;
                case COUNC_VOTE_YES:
                    says("YEA");
                    break;
            }
        } else {
            says("GOV");
        }
        parse_says(2, get_title(faction_id), -1, -1);
        parse_says(3, get_name(faction_id), -1, -1);
        // Fix: motion votes pass -1/-2/-3 as vote_id and the original reads Faction outside bounds
        if (vote_id > 0) {
            parse_says(6, get_title(vote_id), -1, -1);
            parse_says(8, get_name(vote_id), -1, -1);
        }
        parse_num(0, cost);
        if (*DiploTradeWantTech1 >= 0) {
            parse_says(0, Tech[*DiploTradeWantTech1].name, -1, -1);
            if (*DiploTradeWantTech2 < 0) {
                says("1");
            } else {
                parse_says(1, Tech[*DiploTradeWantTech2].name, -1, -1);
                if (*DiploTradeWantTech3 < 0) {
                    says("2");
                } else {
                    parse_says(4, Tech[*DiploTradeWantTech3].name, -1, -1);
                    if (*DiploTradeWantTech4 < 0) {
                        says("3");
                    } else {
                        parse_says(5, Tech[*DiploTradeWantTech4].name, -1, -1);
                        says("4");
                    }
                }
            }
        }
        if (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR && vote_id > 0 && vote_id != faction_id) {
            says("A");
        }
        Popup_start(&cur_popup, PopupScriptFile, StrBuffer, -1, 0, POPUP_EXTERNAL, 0);
        Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], 0, 0);
        if (X_text_open(ScriptFile, "BUYVOTEMENU")) {
            return;
        }
        read_line();
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 0);
        read_line();
        if (plr.energy_credits >= cost) {
            Dialogs_item(&cur_popup.dialogs, StrBuffer, 1);
        }
        read_line();
        if (*DiploTradeWantTech1 >= 0 && *DiploTradeWantTech2 < 0) {
            Dialogs_item(&cur_popup.dialogs, StrBuffer, 2);
        }
        read_line();
        if (*DiploTradeWantTech1 >= 0 && *DiploTradeWantTech2 >= 0 && *DiploTradeWantTech3 < 0) {
            Dialogs_item(&cur_popup.dialogs, StrBuffer, 3);
        }
        read_line();
        if (*DiploTradeWantTech1 >= 0 && *DiploTradeWantTech2 >= 0 && *DiploTradeWantTech3 >= 0
        && *DiploTradeWantTech4 < 0) {
            Dialogs_item(&cur_popup.dialogs, StrBuffer, 4);
        }
        read_line();
        if (*DiploTradeWantTech1 >= 0 && *DiploTradeWantTech2 >= 0 && *DiploTradeWantTech3 >= 0
        && *DiploTradeWantTech4 >= 0) {
            Dialogs_item(&cur_popup.dialogs, StrBuffer, 5);
        }
        int choice = BasePop_exec_3(&cur_popup, 0, pop_wait);
        if (choice <= 0) {
            return;
        }
        if (choice == 1) {
            net_energy(faction_id, -cost, faction_id_2, cost, 1);
        } else {
            net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
            if (choice >= 3) {
                net_tech(faction_id_2, *DiploTradeWantTech2, faction_id, 1);
            }
            if (choice >= 4) {
                net_tech(faction_id_2, *DiploTradeWantTech3, faction_id, 1);
            }
            if (choice >= 5) {
                net_tech(faction_id_2, *DiploTradeWantTech4, faction_id, 1);
            }
        }
        *DiploTradeWantTech4 = -1;
        *DiploTradeWantTech3 = -1;
        *DiploTradeWantTech2 = -1;
        *DiploTradeWantTech1 = -1;
    }
    cmp.unk_29[proposal_id] = vote_id;
    cmp.unk_30[proposal_id] = (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR ? vote_id : faction_id);
    if (*MultiplayerActive) {
        synch_leader(faction_id_2);
        if (*ControlWaitLoop) {
            message_data(0x2601, 0, faction_id_2, vote_id, 0, 0);
        }
    } else if (*ControlWaitLoop) {
        CouncWin_set_vote(CouncWin, faction_id_2, vote_id);
    }
}

int __cdecl buy_tech(int faction_id, int faction_id_2, int counter_id, int high_price, int proposal_id) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];

    *DiploSecondFaction = faction_id_2;
    if (proposal_id == DiploProposalBuyTech) {
        if (*DiploBestUnitID < 0) {
            scan_prototypes(faction_id, faction_id_2);
            if (*DiploBestUnitID < 0) {
                return 0;
            }
        }
        *DiploTradeOfferTech1 = *DiploBestUnitID + Tech_ID_Units;
    } else if (proposal_id != DiploProposalTradeCommlink) {
        if (*DiploTradeOfferTech1 < 0) {
            return 0;
        }
        if (!strcmp(MFactions[faction_id_2].filename, "UNIV") && *DiploTradeWantTech1 >= 0) {
            return 0;
        }
    }
    if (counter_id == DiploCounterFriendship) {
        return 0;
    }
    int cost = (*CurrentTurn + 7) / 8 * mod_tech_val(*DiploTradeOfferTech1, faction_id_2, 1);
    if (great_satan(faction_id, 0)) {
        cost *= 2;
    }
    if (*DiploGreatSatan) {
        cost *= 2;
    }
    if (high_price) {
        cost = 3 * cost / 2;
    }
    // Fix: added bounds checking, pseudo tech_ids outside TechOwners get no owner discount
    int price = cost / clamp(tech_owner_count(*DiploTradeOfferTech1) - 1, 1, 999);
    if ((plr.diplo_status[faction_id_2] & DIPLO_PACT) && cmp.ranking > plr.ranking) {
        if (cmp.tech_ranking <= plr.tech_ranking + 8) {
            if (cmp.tech_ranking > plr.tech_ranking) {
                price /= 2;
            }
        } else {
            price /= 4;
        }
    }
    price = 25 * ((price + 25) / 25);
    if (proposal_id != DiploProposalTradeCommlink && price < 100) {
        price = 100;
    }
    if (plr.energy_credits < price && counter_id != DiploCounterEnergyPayment && proposal_id != DiploProposalBuyTech) {
        return 0;
    }
    parse_tech(0, *DiploTradeOfferTech1, 1);
    parse_num(0, price);
    clear();
    says("BUY");
    if (proposal_id == DiploProposalBuyTech) {
        says("PROTO");
    } else if (proposal_id == DiploProposalTradeCommlink) {
        says("COMMLINK");
        parse_says(0, get_adjective(*DiploProposalFaction), -1, -1);
    } else {
        says("TECH");
    }
    if (plr.energy_credits >= price) {
        say_num(proposal_id == DiploProposalBuyTech || proposal_id == DiploProposalTradeCommlink
            ? 0 : random_val(0, 2));
        if (X_dialog(StrBuffer, faction_id_2) == 1) {
            net_energy(faction_id, -price, faction_id_2, price, 0);
            net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
            if (proposal_id == DiploProposalBuyTech) {
                *DiploBestUnitID = -1;
            }
            InfoWin->override_state = 0;
            Console_update_data(MapWin, 0);
            diplomacy_caption(faction_id, faction_id_2);
        }
        return 1;
    }
    says("HIGH");
    say_num(proposal_id == DiploProposalBuyTech || proposal_id == DiploProposalTradeCommlink ? 0 : random_val(0, 2));
    X_dialog(StrBuffer, faction_id_2);
    return 1;
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

int __cdecl energy_trade(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    MFaction& m_cmp = MFactions[faction_id_2];

    *DiploSecondFaction = faction_id_2;
    tech_analysis(faction_id, faction_id_2);
    int counter = *DiploCounterProposal;
    bool tech_offer = (counter == DiploCounterResearchData || counter == DiploCounterNameAPrice);
    if ((cmp.diplo_status[faction_id] & DIPLO_PACT)
    && counter != DiploCounterLoanPayment
    && cmp.ranking > plr.ranking + 1) {
        int credits = cmp.energy_credits;
        if (credits > 2 * plr.energy_credits
        && cmp.tech_ranking > plr.tech_ranking
        && credits > 4 * *CurrentTurn
        && cmp.diplo_patience[faction_id] < 2
        && (*DiploTradeWantTech1 < 0 || counter == DiploCounterFriendship)) {
            int gift = 25 * ((clamp(credits / 8, 1, 500) + 24) / 25);
            // Fix: the original used < here and gave away the whole treasury. It also never set $NUM0.
            if (gift > credits) {
                gift = credits;
            }
            parse_num(0, gift);
            net_energy(faction_id_2, -gift, faction_id, gift, 1);
            cmp.diplo_patience[faction_id] += 2;
            snprintf(StrBuffer, StrBufLen, "ENERGYFREEBIE%d", random_val(0, 2));
            X_dialog(StrBuffer, faction_id_2);
            return 0;
        }
    }
    if (counter == DiploCounterLoanPayment || counter == DiploCounterFriendship
    || (counter == DiploCounterNameAPrice && *DiploTradeWantTech1 < 0)) {
        int bonus = plr.tech_commerce_bonus;
        int bonus_2 = cmp.tech_commerce_bonus;
        int credits_2 = cmp.energy_credits;
        int loan_max;
        if (bonus_2 < bonus) {
            loan_max = credits_2 / 4;
        } else if (bonus_2 == bonus) {
            loan_max = credits_2 / 3;
        } else {
            loan_max = credits_2 / 2;
        }
        if (credits_2 < plr.energy_credits && strcmp(m_cmp.filename, "MORGAN")) {
            loan_max = 0;
        }
        if (plr.loan_balance[faction_id_2] || cmp.loan_balance[faction_id]) {
            loan_max = 0;
        }
        uint32_t status = cmp.diplo_status[faction_id];
        if (status & (DIPLO_VENDETTA|DIPLO_WANT_REVENGE)) {
            loan_max = 0;
        }
        if (!(status & DIPLO_TREATY) || loan_max < 10) {
            loan_max = 0;
        }
        int interest = 5 * (*DiploReputation + 2 * (bonus_2 - bonus) + 10);
        if (plr.diplo_status[faction_id_2] & DIPLO_PACT) {
            interest -= 10;
            if (cmp.ranking > plr.ranking) {
                interest -= 10;
            }
        }
        interest = max(interest, 0);
        int payment = max(10 * (10 - *DiploReputation), 20);
        if (loan_max) {
            int owed = loan_max * (interest + 100) / 100;
            if (owed < 10) {
                payment = 10;
            } else if (payment > owed) {
                payment = owed;
            }
            int turns = (owed + payment - 1) / payment;
            parse_num(0, loan_max);
            parse_num(1, turns);
            parse_num(2, payment);
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            snprintf(StrBuffer, StrBufLen, "ENERGYLOAN%d", (turns != 1) + 1);
            if (!X_dialog(StrBuffer, faction_id_2)) {
                return 0;
            }
            net_loan(faction_id, faction_id_2, payment * turns, turns);
            net_energy(faction_id_2, -loan_max, faction_id, loan_max, 1);
            return 1;
        }
    } else if (tech_offer) {
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        if (*DiploTradeWantTech1 >= 0) {
            if (strcmp(m_cmp.filename, "BELIEVE")) {
                int base_price = 32 * ((*CurrentTurn + 7) / 8);
                if (great_satan(faction_id, 0)) {
                    base_price = 3 * base_price / 4;
                }
                // Fix: the original counted owners of DiploTradeOfferTech1 (may be -1), not of the tech sold
                int unit_price = base_price / clamp(tech_owner_count(*DiploTradeWantTech1) - 1, 1, 99);
                int price = unit_price * mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1) / 32;
                int price_2 = 0;
                if (*DiploTradeWantTech2 >= 0) {
                    price_2 = unit_price * mod_tech_val(*DiploTradeWantTech2, faction_id_2, 1) / 32;
                }
                price = clamp(price, 0, max(cmp.energy_credits / 2, 0));
                price = 25 * (price / 25);
                if (price < 25) {
                    price = 25;
                }
                if (price_2 < 25) {
                    price_2 = 25;
                }
                while (1) {
                    parse_tech(0, *DiploTradeWantTech1, 1);
                    if (*DiploTradeWantTech2 >= 0) {
                        parse_tech(1, *DiploTradeWantTech2, 1);
                    }
                    parse_num(0, price);
                    snprintf(StrBuffer, StrBufLen, "ENERGYTECH%d", (*DiploTradeWantTech2 < 0 ? random_val(0, 2) : 2));
                    int choice = X_dialog(StrBuffer, faction_id_2);
                    if (choice == 0) {
                        return 0;
                    }
                    if (choice == 3 || (choice == 2 && *DiploTradeWantTech2 < 0)) {
                        help_topic(14, *DiploTradeWantTech1);
                        continue;
                    }
                    if (choice == 2) {
                        if (!strcmp(m_cmp.filename, "HIVE")
                        || (price < price_2 && strcmp(m_cmp.filename, "UNIV"))) {
                            X_dialog("REJSELLSECOND", faction_id_2);
                            return 0;
                        }
                        X_dialog("SELLSECOND", faction_id_2);
                        net_tech(faction_id_2, *DiploTradeWantTech2, faction_id, 0);
                    } else {
                        net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 0);
                        *DiploTradeWantTech1 = *DiploTradeWantTech2;
                    }
                    *DiploTradeWantTech2 = -1;
                    net_energy(faction_id_2, -price, faction_id, price, 1);
                    InfoWin->override_state = 0;
                    Console_update_data(MapWin, 0);
                    return 1;
                }
            }
        }
        X_dialog("REJSELLNONE", faction_id_2);
        return 0;
    }
    snprintf(StrBuffer, StrBufLen, "REJENERGY%d", random_val(0, 2));
    X_dialog(StrBuffer, faction_id_2);
    return 0;
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

int __cdecl tech_trade(int faction_id, int faction_id_2, int rival_init, int counter_id, int proposal_id) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    MFaction& m_plr = MFactions[faction_id];
    MFaction& m_cmp = MFactions[faction_id_2];
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    uint32_t& status_12 = plr.diplo_status[faction_id_2];
    uint32_t& status_21 = cmp.diplo_status[faction_id];
    uint32_t& agenda_12 = plr.diplo_agenda[faction_id_2];
    uint32_t& agenda_21 = cmp.diplo_agenda[faction_id];
    *DiploSecondFaction = faction_id_2;
    bool tech_counter = (counter_id == DiploCounterResearchData || counter_id == DiploCounterNameAPrice);
    *DiploTradeDeclined = 0;
    while (1) {
        tech_analysis(faction_id, faction_id_2);
        if (proposal_id == DiploProposalTradeCommlink) {
            *DiploTradeOfferTech1 = *DiploProposalFaction + Tech_ID_Commlink;
        }
        diplomacy_caption(faction_id, faction_id_2);
        if (!(status_12 & DIPLO_PACT)
        && ((status_21 & (DIPLO_UNK_8000|DIPLO_WANT_REVENGE))
        || *DiploCapturedBases
        || *DiploFriction >= 18
        || (*DiploHostility && strcmp(m_cmp.filename, "UNIV")))) {
            if (!rival_init) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_insult_leader(faction_id), -1, -1);
                parse_says(3, m_plr.adj_insult_faction, -1, -1);
                parse_says(4, get_phrase(faction_id, 3), -1, -1);
                snprintf(StrBuffer, StrBufLen, "REJTECHHATE%d",
                    (proposal_id == DiploProposalTradeCommlink ? 0 : random_val(0, 3)));
                X_dialog(StrBuffer, faction_id_2);
            }
            return 0;
        }
        if (!((status_21 & DIPLO_PACT) && (status_21 & DIPLO_HAVE_SURRENDERED))) {
            int reactor_lead = 0;
            for (int i = 0; i < MaxReactorNum; i++) {
                if (has_tech(Reactor[i].preq_tech, faction_id_2) && !has_tech(Reactor[i].preq_tech, faction_id)) {
                    reactor_lead = 1;
                }
            }
            bool fear = false;
            if (great_satan(faction_id, 0) && !(status_12 & DIPLO_PACT)
            && !(cmp.player_flags & PFLAG_COOP_WITH_HUMAN)) {
                int cost_diff = m_cmp.rule_techcost - 100;
                int cost_sign = (cost_diff > 0) - (cost_diff < 0);
                if (*DiploFriction >= ((status_21 & DIPLO_TREATY) ? 3 : 0) + 2 * (2 - cost_sign)) {
                    fear = true;
                }
            }
            if (!fear
            && (*MultiplayerActive || (*GameRules & RULES_INTENSE_RIVALRY))
            && plr.ranking == MaxFacRank
            && !(status_12 & DIPLO_PACT)
            && !(cmp.player_flags & PFLAG_COOP_WITH_HUMAN)
            && (great_satan(faction_id, 0)
            || (3 * FactionRankingsUnk[faction_id] > 4 * FactionRankingsUnk[*RankingFactionIDUnk2]
            && 3 * plr.tech_ranking > 4 * Factions[*RankingFactionIDUnk2].tech_ranking))) {
                fear = true;
            }
            if (fear) {
                if (!rival_init && !mod_buy_tech(faction_id, faction_id_2, counter_id, 1, proposal_id)) {
                    parse_says(0, get_title(faction_id), -1, -1);
                    parse_says(1, get_name(faction_id), -1, -1);
                    parse_says(2, get_phrase(faction_id, 3), -1, -1);
                    snprintf(StrBuffer, StrBufLen, "REJTECHFEAR%d",
                        (proposal_id == DiploProposalTradeCommlink ? 0 : random_val(0, 2)));
                    X_dialog(StrBuffer, faction_id_2);
                }
                return 0;
            }
            if (cmp.tech_ranking > plr.tech_ranking || reactor_lead) {
                if (rival_init) {
                    return 0;
                }
                int friction_limit = (reactor_lead ? 3 : 9)
                    + ((status_21 & DIPLO_TREATY) ? 3 : 0)
                    - (strcmp(m_cmp.filename, "HIVE") ? 0 : 3);
                if (*DiploFriction >= friction_limit && !(status_12 & DIPLO_PACT)
                && !(cmp.player_flags & PFLAG_COOP_WITH_HUMAN)) {
                    if (reactor_lead || !mod_buy_tech(faction_id, faction_id_2, counter_id, 1, proposal_id)) {
                        if (counter_id != DiploCounterEnergyPayment || proposal_id == DiploProposalTradeCommlink) {
                            if (tech_counter) {
                                parse_says(0, m_plr.adj_insult_faction, -1, -1);
                                snprintf(StrBuffer, StrBufLen, "REJTECHADVANCED%d",
                                    (reactor_lead ? 2 : random_val(0, 3)));
                                X_dialog(StrBuffer, faction_id_2);
                            }
                        } else {
                            parse_says(0, get_title(faction_id), -1, -1);
                            parse_says(1, get_name(faction_id), -1, -1);
                            parse_says(2, get_phrase(faction_id, 3), -1, -1);
                            X_dialog("REJTECHFEAR1", faction_id_2);
                        }
                    }
                    return 0;
                }
            }
            if (*DiploTradeOfferProject1 != -1 && proposal_id != DiploProposalTradeCommlink) {
                if (!rival_init) {
                    int project = *DiploTradeOfferProject1;
                    int fac_id = (project < 0 ? SP_ID_First - 2 - project : SP_ID_First + project);
                    parse_says(0, Facility[fac_id].name, -1, -1);
                    parse_says(1, get_title(faction_id), -1, -1);
                    parse_says(2, get_name(faction_id), -1, -1);
                    snprintf(StrBuffer, StrBufLen, "REJTECHPROJ%d", (project < 0 ? 2 : random_val(0, 3)));
                    X_dialog(StrBuffer, faction_id_2);
                }
                return 0;
            }
        }
        if (proposal_id == DiploProposalTradeCommlink) {
            if (status_21 & DIPLO_PACT) {
                if (!wants_to_speak(faction_id, faction_id_2)) {
                    return 0;
                }
                parse_says(0, get_adjective(*DiploProposalFaction), -1, -1);
                X_pops("COMMLINKFREE", FactionPortraits[faction_id_2], pop_wait);
                net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
                *DiploTradeOfferTech1 = -1;
                return 0;
            }
        } else if (*DiploTradeWantTech1 < 0 && proposal_id != DiploProposalBuyTech
        && !((1 << faction_id) & cmp.traded_maps)
        && !*DiploMapsTraded
        && !(status_12 & DIPLO_PACT)
        && !map_is_known(faction_id_2)) {
            int shared_bases = 0;
            for (int i = 0; i < *BaseCount; i++) {
                if (Bases[i].faction_id == faction_id
                && (Bases[i].faction_id == faction_id_2 || (Bases[i].visibility & (1 << faction_id_2)))) {
                    shared_bases++;
                }
            }
            bool few_shared = ((*GameRules & RULES_NO_UNITY_SURVEY)
                ? !shared_bases
                : shared_bases < (plr.base_count + 1) / 2);
            if (few_shared && *DiploFriction <= 7) {
                *DiploTradeWantTech1 = Tech_ID_WorldMap;
            }
        }
        if (*DiploTradeOfferTech1 >= 0 && proposal_id != DiploProposalTradeCommlink) {
            if ((status_21 & DIPLO_PACT) && (status_21 & DIPLO_HAVE_SURRENDERED)) {
                if (!wants_to_speak(faction_id, faction_id_2)) {
                    return 0;
                }
                parse_tech(0, *DiploTradeOfferTech1, 1);
                X_dialog("TECHSLAVE", faction_id_2);
                net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
                *DiploTradeOfferTech1 = -1;
                return 0;
            }
        }
        if (*DiploTradeWantTech1 < 0 && *DiploTradeOfferTech1 >= 0
        && proposal_id != DiploProposalBuyTech && proposal_id != DiploProposalTradeCommlink
        && (status_21 & DIPLO_PACT)
        && *DiploFriction < 10
        && ((!*MultiplayerActive && !(*GameRules & RULES_INTENSE_RIVALRY)) || faction_id != *RankingFactionIDUnk1)
        && !*DiploGreatSatan) {
            if (cmp.ranking > plr.ranking || (plr.ranking >= 6 && cmp.ranking >= 3)) {
                if (cmp.tech_ranking > plr.tech_ranking
                + cmp.AI_fight + (plr.ranking - cmp.ranking) / 2
                + 2 * cmp.AI_power + 4 - cmp.AI_tech) {
                    if (!wants_to_speak(faction_id, faction_id_2)) {
                        return 0;
                    }
                    parse_tech(0, *DiploTradeOfferTech1, 1);
                    X_dialog("TECHFREEBIE", faction_id_2);
                    net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
                    *DiploTradeOfferTech1 = -1;
                    int patience;
                    if (status_21 & DIPLO_VENDETTA) {
                        patience = 1;
                    } else if (status_21 & DIPLO_PACT) {
                        patience = ((status_21 & DIPLO_HAVE_SURRENDERED) ? -12 : 6);
                    } else {
                        patience = ((status_21 & DIPLO_TREATY) != 0) - (*DiploFriction + 3) / 8 + 3;
                    }
                    cmp.diplo_patience[faction_id] = patience;
                    return 0;
                }
            }
        }
        int visible_bases = 0;
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == faction_id_2
            && (Bases[i].faction_id == faction_id || (Bases[i].visibility & (1 << faction_id)))) {
                visible_bases++;
            }
        }
        bool refuse = (*DiploTradeWantTech1 < 0 || !tech_counter || *DiploGreatSatan);
        if (!refuse && *MultiplayerActive) {
            int rival = *RankingFactionIDUnk2;
            if ((agenda_12 & AGENDA_UNK_80) && faction_id == *RankingFactionIDUnk1 && !(status_12 & DIPLO_PACT)
            && (Factions[rival].ranking < 5 || 2 * FactionRankingsUnk[faction_id] > 3 * FactionRankingsUnk[rival])) {
                refuse = true;
            } else if ((agenda_12 & AGENDA_UNK_100)
            && (faction_id != rival || (plr.ranking >= 6 && !*DiploRunawayLeader))) {
                refuse = true;
            }
        }
        if (!refuse) {
            refuse = (status_21 & DIPLO_WANT_REVENGE)
                || (agenda_21 & AGENDA_UNK_200)
                || plr.integrity_blemishes > 3
                || ((*GameRules & RULES_INTENSE_RIVALRY)
                && (agenda_12 & AGENDA_UNK_80)
                && faction_id == *RankingFactionIDUnk1
                && !(status_12 & DIPLO_PACT)
                && 3 * FactionRankingsUnk[FactionRankings[MaxFacRank]]
                > 4 * FactionRankingsUnk[FactionRankings[MaxFacRank - 1]])
                || ((*GameRules & RULES_INTENSE_RIVALRY)
                && (agenda_12 & AGENDA_UNK_100)
                && !(status_12 & DIPLO_PACT)
                && plr.ranking >= 6
                && !*DiploRunawayLeader)
                || (*DiploTradeOfferTech1 < 0
                && (*DiploMapsTraded
                || (status_12 & DIPLO_PACT)
                || map_is_known(faction_id)
                || (((1 << faction_id_2) & plr.traded_maps)
                && visible_bases >= ((*GameRules & RULES_NO_UNITY_SURVEY)
                ? (cmp.base_count + 3) / 4
                : (cmp.base_count + 1) / 2))));
        }
        if (refuse) {
            if (!rival_init && !mod_buy_tech(faction_id, faction_id_2, counter_id, 0, proposal_id)) {
                if (proposal_id == DiploProposalTradeCommlink) {
                    snprintf(StrBuffer, StrBufLen, "REJCOMMLINK");
                } else {
                    int variant = random_val(0, 3);
                    if (status_12 & (DIPLO_TREATY|DIPLO_PACT)) {
                        if (*DiploTradeWantTech1 >= 0) {
                            if (*DiploTradeOfferTech1 < 0) {
                                variant = 3;
                            }
                        } else {
                            variant = 4;
                        }
                    }
                    snprintf(StrBuffer, StrBufLen, "REJTECHLATER%d", variant);
                }
                X_dialog(StrBuffer, faction_id_2);
            }
            return 0;
        }
        if (!wants_to_speak(faction_id, faction_id_2)) {
            return 0;
        }
        int choice;
        int value;
        while (1) {
            while (1) {
                if (*DiploTradeWantTech1 != Tech_ID_WorldMap) {
                    parse_tech(0, *DiploTradeWantTech1, 1);
                }
                if (*DiploTradeOfferTech1 >= 0) {
                    parse_tech(1, *DiploTradeOfferTech1, 1);
                }
                if (proposal_id == DiploProposalTradeCommlink) {
                    parse_says(2, get_adjective(*DiploProposalFaction), -1, -1);
                    if (!X_pops("COMMLINKTECH", FactionPortraits[faction_id_2], pop_wait)) {
                        return 0;
                    }
                    net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
                    net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
                    *DiploTradeOfferTech1 = -1;
                    *DiploTradeWantTech1 = *DiploTradeWantTech2;
                    *DiploTradeWantTech2 = -1;
                    return 1;
                }
                if (*DiploTradeWantTech2 >= 0) {
                    parse_tech(2, *DiploTradeWantTech2, 1);
                }
                parse_says(3, get_title(faction_id), -1, -1);
                parse_says(4, get_name(faction_id), -1, -1);
                parse_says(5, get_noun_phrase(faction_id, 9), -1, -1);
                parse_says(6, get_noun_phrase(faction_id_2, 9), -1, -1);
                parse_says(8, get_name(faction_id_2), -1, -1);
                parse_says(9, get_noun_phrase(faction_id_2, 7), -1, -1);
                int variant;
                if (*DiploTradeWantTech1 == Tech_ID_WorldMap) {
                    variant = (*DiploTradeOfferTech1 >= 0 ? 4 : 5);
                } else if (*DiploTradeOfferTech1 >= 0) {
                    variant = random_val(0, 3);
                } else {
                    variant = 3;
                }
                snprintf(StrBuffer, StrBufLen, "TRADETECH%d", variant);
                Popup_start(&cur_popup, PopupScriptFile, StrBuffer, -1, 0, POPUP_EXTERNAL, 0);
                Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], 0, 0);
                if (!X_text_open(ScriptFile, "TRADETECHOPT")) {
                    text_get();
                    if (*DiploTradeOfferTech2 >= 0 && *DiploTradeOfferTech2 != *DiploTradeOfferTech1) {
                        parse_tech(9, *DiploTradeOfferTech2, 1);
                        clear();
                        parse_string(text_buf_ptr(), StrBuffer);
                        Dialogs_item(&cur_popup.dialogs, StrBuffer, 4);
                    }
                    text_get();
                    if (*DiploTradeWantTech2 >= 0 && *DiploTradeWantTech2 != *DiploTradeWantTech1) {
                        clear();
                        parse_string(text_buf_ptr(), StrBuffer);
                        Dialogs_item(&cur_popup.dialogs, StrBuffer, 2);
                    }
                    text_get();
                    Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 3);
                }
                choice = BasePop_exec_3(&cur_popup, 0, pop_wait);
                if (!choice) {
                    *DiploTradeDeclined = 1;
                    return 0;
                }
                if (choice != 3) {
                    break;
                }
                int topic = *DiploTradeWantTech1;
                if (topic == Tech_ID_WorldMap) {
                    topic = *DiploTradeOfferTech1;
                }
                help_topic(14, topic);
            }
            value = *DiploTradeWantValue1 / 4;
            if (choice != 4) {
                break;
            }
            if ((status_21 & DIPLO_PACT) || *DiploTradeWantTech2 >= 0
            || mod_tech_val(*DiploTradeOfferTech2, faction_id_2, 0)
            <= mod_tech_val(*DiploTradeWantTech1, faction_id_2, 0)) {
                std::swap(*DiploTradeOfferTech1, *DiploTradeOfferTech2);
                value = *DiploTradeWantValue1 / 8;
                break;
            }
            if (*DiploTradeOfferTech1 >= 0) {
                parse_tech(0, *DiploTradeOfferTech1, 1);
            }
            X_dialog("REJTECHINSTEAD", faction_id_2);
            *DiploTradeOfferTech2 = -1;
        }
        if (choice == 2) {
            if ((!(status_21 & DIPLO_PACT) || !(status_21 & DIPLO_HAVE_SURRENDERED))
            && (!strcmp(m_cmp.filename, "HIVE")
            || (*MultiplayerActive && faction_id == *RankingFactionIDUnk1)
            || (strcmp(m_cmp.filename, "UNIV")
            && (great_satan(faction_id, 0)
            || cmp.tech_ranking > plr.tech_ranking)))) {
                X_dialog("REJTECHSECOND", faction_id_2);
                cmp.diplo_patience[faction_id]++;
                return 0;
            }
            std::swap(*DiploTradeWantTech1, *DiploTradeWantTech2);
            value = *DiploTradeWantValue2 / 8;
        }
        if (rival_init) {
            net_treaty_on(faction_id_2, faction_id, DIPLO_UNK_1000000, 0);
        }
        net_agenda_on(faction_id_2, faction_id, AGENDA_UNK_100|AGENDA_UNK_80, 0);
        if (*DiploTradeOfferTech1 < 0) {
            *DiploTradeOfferTech1 = Tech_ID_WorldMap;
        }
        net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
        net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
        if (*DiploTradeWantTech1 == Tech_ID_WorldMap) {
            *DiploMapsTraded = 1;
        }
        if (*DiploTradeOfferTech1 == Tech_ID_WorldMap) {
            *DiploMapsTraded = 1;
        }
        *DiploTradeWantTech1 = *DiploTradeWantTech2;
        *DiploTradeWantTech2 = -1;
        cause_friction(faction_id_2, faction_id, -value);
        *DiploHostility = 0;
        if (!rival_init) {
            return 1;
        }
    }
}

void __cdecl trade_maps(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];

    *DiploMapMsg = 0;
    int traded = cmp.traded_maps | (1 << faction_id_2) | plr.traded_maps;
    plr.traded_maps = traded;
    cmp.traded_maps |= traded | (1 << faction_id);
    for (int i = 0; i < *MapAreaTiles; i++) {
        MAP* sq = &(*MapTiles)[i];
        if (sq->visibility & (1 << faction_id)) {
            sq->visibility |= (1 << faction_id_2);
            if (faction_id && faction_id_2) {
                sq->visible_items[faction_id_2 - 1] |= sq->items & sq->visible_items[faction_id - 1];
            }
        } else if (sq->visibility & (1 << faction_id_2)) {
            *DiploMapMsg = 1;
            sq->visibility |= (1 << faction_id);
            if (faction_id && faction_id_2) {
                sq->visible_items[faction_id - 1] |= sq->items & sq->visible_items[faction_id_2 - 1];
            }
        }
    }
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE* base = &Bases[base_id];
        if ((base->visibility & (1 << faction_id)) || base->faction_id == faction_id) {
            spot_base(base_id, faction_id_2);
        } else if ((base->visibility & (1 << faction_id_2)) || base->faction_id == faction_id_2) {
            spot_base(base_id, faction_id);
        }
    }
    if (faction_id == MapWin->cOwner || faction_id_2 == MapWin->cOwner) {
        *GameDrawState |= 4;
        GraphicWin_redraw(WorldWin);
        draw_map(1);
    }
}

int __cdecl propose_pact(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    MFaction& m_cmp = MFactions[faction_id_2];

    *DiploSecondFaction = faction_id_2;
    bool threatened = false;
    int score = *DiploPlayerPactCount + *DiploRivalPactCount;
    if (*DiploPlayerPactCount) {
        score++;
    }
    int helper = (*DiploHelpFaction ? *DiploHelpFaction : *DiploAcceptedFaction);
    if (*MultiplayerActive) {
        if (faction_id == *RankingFactionIDUnk1
        && (Factions[*RankingFactionIDUnk2].ranking < 6
        || 2 * FactionRankingsUnk[faction_id] > 3 * FactionRankingsUnk[*RankingFactionIDUnk2])) {
            score++;
        }
        if (faction_id == *RankingFactionIDUnk2
        && (plr.ranking < 6
        || 2 * FactionRankingsUnk[FactionRankings[MaxFacRank]] > 3 * FactionRankingsUnk[faction_id])) {
            score--;
        }
    }
    if ((*GameRules & RULES_INTENSE_RIVALRY) && faction_id == *RankingFactionIDUnk1) {
        score++;
    }
    if ((*GameRules & RULES_VICTORY_COOPERATIVE) && !(*GameRules & RULES_INTENSE_RIVALRY)) {
        score--;
    }
    if (cmp.player_flags & PFLAG_COOP_WITH_HUMAN) {
        score--;
    }
    int rank_plr = plr.ranking;
    int rank_cmp = cmp.ranking;
    if (rank_plr < 6) {
        if (*DiploRivalStrongPacts) {
            score++;
        }
    } else if (!*DiploRunawayLeader && rank_cmp >= (rank_plr != MaxFacRank) + 5) {
        score++;
        if (*CurrentTurn >= (*MultiplayerActive ? 50 : 100)) {
            score++;
        }
        if (*DiploRivalStrongPacts) {
            score++;
        }
    } else if (rank_cmp < 4 || *CurrentTurn < 50 || *DiploRunawayLeader) {
        if (*DiploRivalStrongPacts) {
            score++;
        }
    } else if (*DiploRivalStrongPacts) {
        score += 2;
    } else if (!*DiploPlayerPactCount) {
        score--;
    }
    if (helper) {
        score--;
        if (great_satan(helper, 0)) {
            score--;
        }
        if (Factions[helper].ranking > rank_cmp) {
            score--;
        }
        if (cmp.diplo_status[helper] & DIPLO_UNK_8000) {
            int mil_strength = Factions[helper].mil_strength_1;
            int mil_strength_2 = cmp.mil_strength_1;
            if (mil_strength_2 < mil_strength) {
                threatened = true;
                score -= (mil_strength + mil_strength_2 / 2) / (mil_strength_2 + 1);
            }
        }
    }
    if (*DiploFriction >= 10 && *DiploCounterProposal != DiploCounterThreaten) {
        score = score + *DiploFriction - 10;
    }
    int reputation = plr.diplo_reputation;
    if (reputation >= 2 && !*DiploReputation) {
        score--;
    }
    if (reputation <= 1) {
        score++;
    }
    int bonus = plr.tech_commerce_bonus;
    int bonus_2 = cmp.tech_commerce_bonus;
    if (bonus_2 < bonus + 1) {
        score++;
    }
    if (bonus_2 > bonus) {
        score--;
    }
    if (!*MultiplayerActive && !strcmp(m_cmp.filename, "HIVE")) {
        score--;
    }
    if (*DiploFearScale) {
        score--;
    }
    if (*DiploPrevHostility) {
        score++;
    }
    if (*DiploGreatSatan) {
        score++;
    }
    if (rank_cmp <= rank_plr) {
        if (rank_plr == MaxFacRank) {
            score += *DiploPlayerPactCount;
        }
        if (rank_plr - rank_cmp >= 3) {
            score--;
        }
        if (great_satan(faction_id, 0)) {
            score++;
        }
    } else {
        if (rank_cmp == MaxFacRank) {
            score += 2;
        }
        if (rank_cmp == MaxFacRank - 1) {
            score++;
        }
        if (rank_plr <= 4) {
            score--;
        }
    }
    if (rank_plr <= 5 && rank_cmp <= 5) {
        score--;
    }
    bool willing = !((*DiploGoodwill <= score && plr.diff_level)
        || (plr.diplo_agenda[faction_id_2] & AGENDA_PERMANENT)
        || *DiploReputation > ((*GameRules & RULES_VICTORY_COOPERATIVE) != 0)
            + (plr.diff_level != 0 ? 0 : 2) + (threatened ? 2 : 0) + 2
        || (cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)
        || *DiploCanBetray);
    diplomacy_caption(faction_id, faction_id_2);
    parse_says(0, get_pact(faction_id), -1, -1);
    parse_says(1, get_title(faction_id), -1, -1);
    parse_says(2, get_name(faction_id), -1, -1);
    if (*DiploCounterProposal == DiploCounterThreaten && *DiploCanSurrender) {
        net_set_treaty(faction_id_2, faction_id, DIPLO_HAVE_SURRENDERED, 1, 0);
        pact_of_brotherhood(faction_id, faction_id_2);
        return 1;
    }
    if (*DiploCounterProposal == DiploCounterThreaten || !willing) {
        if ((*DiploReputation > 2 || *DiploCapturedBases)
        && (*DiploFearScale || *DiploFriction <= 2)
        && !(cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)) {
            X_dialog("REJPACTMEEK", faction_id_2);
            return 1;
        }
        if (*DiploCounterProposal == DiploCounterThreaten) {
            return 0;
        }
        if ((cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)
        || *DiploReputation > 2 || *DiploCapturedBases) {
            parse_says(3, get_phrase(faction_id, 5), -1, -1);
            X_dialog("REJPACTHATE", faction_id_2);
            cmp.diplo_patience[faction_id]++;
            return 1;
        }
        if (great_satan(faction_id, 0) || *DiploGreatSatan) {
            parse_says(3, get_phrase(faction_id, 3), -1, -1);
            snprintf(StrBuffer, StrBufLen, "REJPACTAHEAD%d", random_val(0, 2));
            X_dialog(StrBuffer, faction_id_2);
            return 1;
        }
        if (rank_cmp > rank_plr || *DiploFriction > 10) {
            parse_says(3, get_noun_phrase(faction_id_2, 9), -1, -1);
            X_dialog("REJPACTPOWER", faction_id_2);
            return 1;
        }
        parse_says(3, get_noun_phrase(faction_id_2, 9), -1, -1);
        X_dialog("REJPACT", faction_id_2);
        return 1;
    }
    int credits = plr.energy_credits;
    int price = 50 * (cmp.mil_strength_1 * (credits + 50 * (*DiploReputation + bonus - bonus_2))
        / (plr.mil_strength_1 + cmp.mil_strength_1 + 1) / 50);
    // Fix pact proposals where a negative computed price could make the AI demand
    // a negative amount of energy credits and transfer credits from the AI to the player.
    price = max(price, 0);
    if (*DiploCounterProposal != DiploCounterEnergyPayment && price) {
        while (price > credits) {
            price = clamp(price - 50, 0, 99999);
            if (!price) {
                break;
            }
        }
    }
    bool bargain = false;
    bool paid = false;
    bool asked = false;
    bool reluctant = false;
    if (!threatened) {
        reluctant = (*DiploFriction > 4 && bonus >= bonus_2)
            || *DiploFriction > 10
            || (*DiploReputation && *DiploReputation > bonus_2 - bonus);
    }
    if (*DiploCounterProposal == DiploCounterResearchData || *DiploCounterProposal == DiploCounterEnergyPayment) {
        bargain = true;
    }
    if (*DiploCounterProposal == DiploCounterNameAPrice) {
        if (!threatened && !*DiploHelpFaction && (*DiploTradeWantTech1 >= 0 || price)) {
            bargain = true;
        }
        if (*DiploTradeWantTech1 < 0 && !price) {
            reluctant = false;
        }
    }
    if (great_satan(faction_id, 0) || *DiploPlayerPactCount > 1
    || (*MultiplayerActive && faction_id == *RankingFactionIDUnk1
    && (*DiploPlayerPactCount || cmp.ranking > Factions[*RankingFactionIDUnk2].ranking))) {
        price = 50 * (max(2 * price, 5 * *CurrentTurn) / 50);
        bool proceed;
        if (plr.energy_credits >= price) {
            proceed = (*DiploTradeWantTech2 >= 0 || *DiploCounterProposal != DiploCounterResearchData);
        } else {
            proceed = (*DiploCounterProposal != DiploCounterEnergyPayment && *DiploTradeWantTech2 >= 0);
        }
        if (!proceed) {
            parse_says(3, get_noun_phrase(faction_id_2, 9), -1, -1);
            X_dialog("REJPACT", faction_id_2);
            return 1;
        }
    } else {
        *DiploTradeWantTech2 = -1;
    }
    if (reluctant || bargain) {
        if (*DiploTradeWantTech1 >= 0 && strcmp(m_cmp.filename, "BELIEVE")
        && (strcmp(m_cmp.filename, "MORGAN") || !price || plr.energy_credits < price)) {
            int counter = *DiploCounterProposal;
            if (counter == DiploCounterResearchData || counter == DiploCounterNameAPrice
            || (counter == DiploCounterFriendship && *DiploFriction < 4)) {
                asked = true;
                int choice;
                while (1) {
                    parse_says(1, get_title(faction_id_2), -1, -1);
                    parse_tech(2, *DiploTradeWantTech1, 1);
                    parse_says(3, get_noun_phrase(faction_id_2, 9), -1, -1);
                    if (*DiploTradeWantTech2 >= 0) {
                        // Fix: Restore missing dialog for GIMMETECH2. It used duplicated labels
                        // in script.txt and is now provided by script_label option and modmenu.txt.
                        parse_tech(4, *DiploTradeWantTech2, 1);
                    }
                    snprintf(StrBuffer, StrBufLen, "GIMMETECH%d", (*DiploTradeWantTech2 < 0 ? random_val(0, 2) : 2));
                    choice = X_dialog(StrBuffer, faction_id_2);
                    if (choice <= 1) {
                        break;
                    }
                    help_topic(14, *DiploTradeWantTech1);
                }
                if (choice) {
                    paid = true;
                    net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
                    if (*DiploTradeWantTech2 >= 0) {
                        net_tech(faction_id_2, *DiploTradeWantTech2, faction_id, 1);
                    }
                    int tech_friction = mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1) / -2;
                    cause_friction(faction_id_2, faction_id, tech_friction);
                    diplomacy_caption(faction_id, faction_id_2);
                    *DiploTradeWantTech2 = -1;
                    *DiploTradeWantTech1 = -1;
                }
            }
        }
        if (price && plr.energy_credits >= price && !asked
        && strcmp(m_cmp.filename, "PEACE")) {
            int counter = *DiploCounterProposal;
            if (counter == DiploCounterEnergyPayment || counter == DiploCounterNameAPrice
            || (counter == DiploCounterFriendship && *DiploFriction < 4)) {
                asked = true;
                parse_num(0, price);
                parse_says(1, get_title(faction_id_2), -1, -1);
                snprintf(StrBuffer, StrBufLen, "GIMMEENERGY%d", random_val(0, 2));
                if (X_dialog(StrBuffer, faction_id_2)) {
                    net_energy(faction_id, -price, faction_id_2, price, 1);
                    paid = true;
                    int friction = -energy_value(price);
                    cause_friction(faction_id_2, faction_id, friction);
                    InfoWin->override_state = 0;
                    Console_update_data(MapWin, 0);
                    diplomacy_caption(faction_id, faction_id_2);
                }
            }
        }
        if ((reluctant || asked) && !paid) {
            if (asked) {
                X_dialog("REJPACTNOBRIBE", faction_id_2);
                return 1;
            }
            if (*DiploCounterProposal == DiploCounterThreaten) {
                return 0;
            }
            X_dialog("REJPACTBADBRIBE", faction_id_2);
            return 1;
        }
    }
    if (helper && !*DiploAcceptedFaction) {
        parse_says(5, get_title(faction_id), -1, -1);
        parse_says(6, get_noun(helper), -1, -1);
        if (X_dialog("GIMMEHELP", faction_id_2)) {
            diplo_lock(0);
            net_double_cross(faction_id, helper, faction_id_2, 0);
            net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
            net_set_treaty(faction_id_2, helper, DIPLO_UNK_40, 1, 1);
            diplo_unlock();
            *DiploHelpFaction = 0;
            *DiploEnemyAllyCount = 0;
        } else if (threatened
        || (FactionRankings[MaxFacRank] == faction_id && *DiploAlterFear >= 0)
        || *DiploReputation > 2 || *DiploAlterFear > 0) {
            cmp.diplo_patience[faction_id]++;
            if (!paid) {
                parse_says(2, get_pacts_2(faction_id, faction_id_2), -1, -1);
                X_dialog("REJPACTNOHELP", faction_id_2);
                return 1;
            }
        }
    }
    pact_of_brotherhood(faction_id, faction_id_2);
    return 1;
}

int __cdecl propose_treaty(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    MFaction& m_cmp = MFactions[faction_id_2];

    *DiploSecondFaction = faction_id_2;
    int tolerance = (!strcmp(m_cmp.filename, "GAIANS") && *DiploAlterFear < 0) + (*DiploAlterFear < 0);
    int helper = (*DiploHelpFaction ? *DiploHelpFaction : *DiploAcceptedFaction);
    bool threatened = helper
        && (cmp.diplo_status[helper] & DIPLO_UNK_8000)
        && Factions[helper].mil_strength_1 > cmp.mil_strength_1
        && !(cmp.diplo_agenda[faction_id] & AGENDA_UNK_800);
    bool bargaining = (*DiploCounterProposal == DiploCounterNameAPrice
        || *DiploCounterProposal == DiploCounterResearchData
        || *DiploCounterProposal == DiploCounterEnergyPayment
        || *DiploCounterProposal == DiploCounterGiveBase);
    bool willing = false;
    if (!*DiploGreatSatan && !*DiploRivalStrongPacts && cmp.ranking <= 4
    && plr.ranking >= 5 && !*DiploPlayerPactCount) {
        tolerance++;
    }
    if (*DiploFriction <= (bargaining ? tolerance + 2 : 0) + 15) {
        if (!(cmp.diplo_agenda[faction_id] & AGENDA_UNK_400)
        && (!(cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)
        || (*DiploReputation <= 6
        && *DiploReputation <= (bargaining ? 2 : 0) + cmp.tech_commerce_bonus
        - plr.tech_commerce_bonus - *DiploAlterFear + 2))) {
            if (threatened) {
                willing = true;
            } else if (!*DiploGreatSatan) {
                int diff_level = clamp(plr.diff_level, 0, 2);
                if (*DiploReputation <= (bargaining ? 2 : 0) - diff_level + tolerance + 5
                && *DiploCapturedBases <= bargaining + (*DiploCounterProposal != DiploCounterThreaten ? 0 : 2)
                    + tolerance + 4
                && (*DiploFriction <= 3 * tolerance + 5 * bargaining + 10
                || *DiploCounterProposal == DiploCounterThreaten)) {
                    willing = true;
                }
            }
        }
    }
    if ((cmp.player_flags & PFLAG_COOP_WITH_HUMAN)
    && !(cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)) {
        willing = true;
    }
    if (plr.diplo_agenda[faction_id_2] & AGENDA_PERMANENT) {
        willing = false;
    }
    parse_says(0, get_name(faction_id), -1, -1);
    parse_says(1, get_title(faction_id), -1, -1);
    parse_says(2, get_noun(faction_id), -1, -1);
    parse_says(3, get_adjective(faction_id), -1, -1);
    if (!willing) {
        if (*DiploReputation <= 4 || (!*DiploFearScale && *DiploFriction > 2)
        || (cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)) {
            if (*DiploCounterProposal == DiploCounterThreaten) {
                return 0;
            }
            if ((cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE) || *DiploReputation > 4) {
                parse_says(4, get_insult_leader(faction_id), -1, -1);
                X_dialog("REJTREATYHATE", faction_id_2);
                return 1;
            }
            if (*DiploGreatSatan) {
                parse_says(4, get_noun_phrase(faction_id_2, 9), -1, -1);
                parse_says(5, get_phrase(faction_id, 3), -1, -1);
                X_dialog("REJTREATYAHEAD", faction_id_2);
            } else {
                X_dialog("REJTREATY", faction_id_2);
            }
            return 1;
        }
        X_dialog("REJTREATYMEEK", faction_id_2);
        return 1;
    }
    int price = 50 * (cmp.mil_strength_1 * (*DiploReputation + plr.energy_credits)
        / (plr.mil_strength_1 + cmp.mil_strength_1 + 1) / 50);
    if (!(cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)) {
        price -= 50 * clamp(cmp.tech_commerce_bonus
            - plr.tech_commerce_bonus, 0, 99);
    }
    // Fix: the original values the tech for faction_id while the receiving AI is faction_id_2.
    if (!strcmp(m_cmp.filename, "MORGAN") && price > plr.energy_credits) {
        if (*DiploTradeWantTech1 < 0) {
            price = plr.energy_credits;
        } else if (plr.energy_credits
        >= (*CurrentTurn + 7) / 8 * mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1)) {
            price = plr.energy_credits;
        }
    }
    if (*DiploCounterProposal != DiploCounterEnergyPayment && price > plr.energy_credits) {
        price -= 50 * ((price - plr.energy_credits + 49) / 50);
    }
    if (price < 50) {
        price = 0;
    }
    bool bargain = false;
    bool paid = false;
    bool asked = false;
    bool reluctant = (!threatened && *DiploFriction > 2 * tolerance + 10
        && plr.tech_commerce_bonus >= cmp.tech_commerce_bonus)
        || *DiploFriction > 15
        || (cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)
        || *DiploReputation > clamp(cmp.tech_commerce_bonus - plr.tech_commerce_bonus, 0, 99) + 2;
    if (*DiploCounterProposal == DiploCounterEnergyPayment || *DiploCounterProposal == DiploCounterResearchData) {
        bargain = true;
    }
    if (*DiploCounterProposal == DiploCounterNameAPrice) {
        bargain = true;
        reluctant = false;
    } else if (*DiploCounterProposal == DiploCounterThreaten) {
        reluctant = false;
    }
    if (reluctant || bargain) {
        // Fix: the tech is valued for the receiving AI (faction_id_2), not for the player
        if (*DiploTradeWantTech1 >= 0
        && strcmp(m_cmp.filename, "BELIEVE")
        && (strcmp(m_cmp.filename, "MORGAN") || !price || plr.energy_credits < price
        || price < 100 * mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1))
        && (*DiploCounterProposal == DiploCounterResearchData
        || *DiploCounterProposal == DiploCounterNameAPrice
        || (*DiploCounterProposal == DiploCounterFriendship && *DiploFriction < 10))) {
            asked = true;
            int choice;
            while (1) {
                parse_says(1, get_title(faction_id_2), -1, -1);
                parse_tech(2, *DiploTradeWantTech1, 1);
                parse_says(3, get_noun_phrase(faction_id_2, 9), -1, -1);
                snprintf(StrBuffer, StrBufLen, "GIMMETECH%d", random_val(0, 2));
                choice = X_dialog(StrBuffer, faction_id_2);
                if (choice <= 1) {
                    break;
                }
                help_topic(14, *DiploTradeWantTech1);
            }
            if (choice) {
                paid = true;
                net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
                // Fix: the tech is valued for the receiving AI (faction_id_2), not for the player
                int tech_friction = mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1) / -2;
                cause_friction(faction_id_2, faction_id, tech_friction);
                diplomacy_caption(faction_id, faction_id_2);
                *DiploTradeWantTech1 = *DiploTradeWantTech2;
                *DiploTradeWantTech2 = -1;
            } else {
                willing = false;
            }
        }
        if (price && plr.energy_credits >= price && !asked
        && strcmp(m_cmp.filename, "PEACE")
        && (*DiploCounterProposal == DiploCounterEnergyPayment
        || *DiploCounterProposal == DiploCounterNameAPrice
        || (*DiploCounterProposal == DiploCounterFriendship && *DiploFriction < 10))) {
            asked = true;
            parse_num(0, price);
            parse_says(1, get_title(faction_id_2), -1, -1);
            snprintf(StrBuffer, StrBufLen, "GIMMEENERGY%d", random_val(0, 2));
            if (!X_dialog(StrBuffer, faction_id_2)) {
                X_dialog("REJTREATYNOBRIBE", faction_id_2);
                return 1;
            }
            net_energy(faction_id, -price, faction_id_2, price, 1);
            paid = true;
            int friction = -energy_value(price);
            cause_friction(faction_id_2, faction_id, friction);
            diplomacy_caption(faction_id, faction_id_2);
            InfoWin->override_state = 0;
            Console_update_data(MapWin, 0);
        }
    }
    if (!willing && asked) {
        X_dialog("REJTREATYNOBRIBE", faction_id_2);
        return 1;
    }
    if (!willing || (reluctant && !asked)) {
        if (*DiploCounterProposal == DiploCounterFriendship) {
            return 0;
        }
        X_dialog("REJTREATYBADBRIBE", faction_id_2);
        return 1;
    }
    if ((*DiploCounterProposal == DiploCounterResearchData || *DiploCounterProposal == DiploCounterEnergyPayment)
    && paid) {
        *DiploHelpFaction = 0;
    }
    if (helper && !*DiploAcceptedFaction) {
        parse_says(5, get_title(faction_id), -1, -1);
        parse_says(6, get_noun(helper), -1, -1);
        if (X_dialog("GIMMEHELP", faction_id_2)) {
            diplo_lock(0);
            net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
            net_double_cross(faction_id, helper, faction_id_2, 1);
            diplo_unlock();
            *DiploHelpFaction = 0;
            *DiploEnemyAllyCount = 0;
        } else if ((threatened || FactionRankings[MaxFacRank] == faction_id || *DiploReputation > 3
        || *DiploCounterProposal == DiploCounterFriendship) && !paid && strcmp(m_cmp.filename, "GAIANS")) {
            X_dialog("REJTREATYNOHELP", faction_id_2);
            cmp.diplo_patience[faction_id]++;
            return 1;
        }
    }
    make_treaty(faction_id, faction_id_2);
    return 1;
}

void __cdecl propose_trade_maps(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];

    *DiploSecondFaction = faction_id_2;
    if (((plr.diplo_status[faction_id_2] & DIPLO_PACT) || *DiploFriction <= 0)
    && *DiploFriction <= 4) {
        X_dialog("TRADEMAPS", faction_id_2);
        cmp.diplo_patience[faction_id] += 2;
        net_maps(faction_id, faction_id_2, 1);
        *DiploMapMsg = 1;
    } else {
        parse_says(0, get_adjective(faction_id_2), -1, -1);
        X_dialog("REJMAPS", faction_id_2);
    }
}

int __cdecl propose_attack(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    MFaction& m_cmp = MFactions[faction_id_2];
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    int remoteness = 0;
    *DiploSecondFaction = faction_id_2;
    int target = *DiploProposalFaction;
    Faction& tgt = Factions[target];
    uint32_t& status_2t = cmp.diplo_status[target];
    uint32_t& status_21 = cmp.diplo_status[faction_id];
    uint32_t& status_1t = plr.diplo_status[target];
    if (status_2t & DIPLO_VENDETTA) {
        parse_says(0, get_title(target), -1, -1);
        parse_says(1, get_name(target), -1, -1);
        X_dialog("REJWARALREADY", faction_id_2);
        return 1;
    }
    if (!(status_2t & DIPLO_COMMLINK) && !contiguous(faction_id_2, target, 0, 1)) {
        if (!(status_21 & DIPLO_PACT)) {
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(2, get_adjective(target), -1, -1);
            X_dialog("REJWARFAR", faction_id_2);
            return 1;
        }
        remoteness = 2;
        for (int region = 1; region < MaxRegionNum; region++) {
            if (!bad_reg(region) && cmp.region_total_bases[region]
            && tgt.region_total_bases[region]) {
                remoteness = 1;
                break;
            }
        }
    }
    int rank_tgt = tgt.ranking;
    int rank_cmp = cmp.ranking;
    int score = (2 - cmp.AI_fight) * (5 * tgt.mil_strength_1
        + *CurrentTurn / 10 * (*DiploReputation - tgt.integrity_blemishes));
    if (rank_tgt > rank_cmp) {
        if (rank_tgt > plr.ranking && !*DiploRivalStrongPacts && (plr.ranking >= 4 || rank_cmp >= 4)) {
            score /= 2;
        }
    }
    if (FactionRankings[MaxFacRank] == target) {
        int top_strength = FactionRankingsUnk[FactionRankings[MaxFacRank]];
        int second_strength = FactionRankingsUnk[FactionRankings[MaxFacRank - 1]];
        if (top_strength >= 2 * second_strength) {
            score /= 4;
        } else if (great_satan(target, 0)) {
            score /= 3;
        } else if (2 * top_strength >= 3 * second_strength) {
            score /= 2;
        }
    }
    if (cmp.player_flags & PFLAG_COOP_WITH_HUMAN) {
        score /= 2;
    }
    bool allied = (status_2t & DIPLO_PACT);
    if (allied) {
        score *= 3 - cmp.AI_fight;
        if (plr.ranking >= 6 && rank_tgt >= 5 && !*DiploRunawayLeader
        && rank_cmp <= 4 && *CurrentTurn >= 50
        && (*DiploPlayerPactCount || plr.integrity_blemishes > 2)) {
            score *= 2;
        }
    } else if (status_21 & DIPLO_PACT) {
        score /= 2;
    }
    if (tgt.integrity_blemishes > plr.integrity_blemishes) {
        score /= 2;
        if ((status_1t & DIPLO_WANT_REVENGE)
        && !(status_21 & DIPLO_WANT_REVENGE)
        && !(tgt.diplo_status[faction_id] & DIPLO_WANT_REVENGE)) {
            score /= 2;
        }
    }
    if (*DiploGreatSatan) {
        score *= 2;
    }
    if ((status_21 & DIPLO_PACT) && (status_21 & DIPLO_HAVE_SURRENDERED)) {
        score /= 2;
    }
    bool vendetta = (status_1t & DIPLO_VENDETTA);
    if (!vendetta && !is_human(target)) {
        score += score / 2;
    }
    if (rank_tgt >= 6 && rank_cmp >= 6) {
        score /= 2;
    }
    int gifts = tgt.diplo_gifts[faction_id_2];
    int wrongs = tgt.diplo_wrongs[faction_id_2];
    if (gifts > wrongs) {
        score *= gifts - wrongs + 1;
    }
    int price = 25 * ((score * (4 - remoteness) / 4 + 24) / 25);
    if ((plr.diplo_status[faction_id_2] & DIPLO_PACT)
    && (*DiploCounterProposal == DiploCounterFriendship || *DiploCounterProposal == DiploCounterThreaten)
    && vendetta && !allied) {
        if (mod_wants_to_attack(faction_id_2, target, 0)
        || mod_wants_to_attack(target, faction_id_2, 0)
        || ((status_1t & DIPLO_WANT_REVENGE) && rank_tgt > plr.ranking)
        || (rank_tgt > plr.ranking && rank_tgt > rank_cmp)) {
            parse_says(0, get_title(target), -1, -1);
            parse_says(1, get_name(target), -1, -1);
            parse_says(2, get_noun(target), -1, -1);
            parse_says(3, get_pact(faction_id), -1, -1);
            X_dialog("WARPACT", faction_id_2);
            net_double_cross(faction_id_2, target, faction_id, 1);
            return 1;
        }
    }
    bool incite = false;
    bool bribe = false;
    if ((status_21 & DIPLO_PACT) && (status_21 & DIPLO_HAVE_SURRENDERED)) {
        if (*DiploCounterProposal == DiploCounterThreaten || *DiploCounterProposal == DiploCounterFriendship) {
            incite = true;
        } else {
            bribe = true;
        }
    } else if ((status_21 & DIPLO_PACT) || (!(status_21 & DIPLO_WANT_REVENGE) && *DiploReputation <= 5)) {
        if ((*DiploCounterProposal != DiploCounterFriendship && *DiploCounterProposal != DiploCounterThreaten)
        || mod_wants_to_attack(faction_id_2, target, 0)
        || (((status_21 & DIPLO_PACT) || *DiploFearScale)
        && mod_wants_to_attack(target, faction_id_2, 0)
        && (status_1t & DIPLO_VENDETTA))) {
            bribe = true;
        } else if (*DiploCounterProposal != DiploCounterThreaten) {
            parse_says(0, get_title(faction_id), -1, -1);
            X_dialog("REJWARNOBRIBE", faction_id_2);
            return 1;
        } else {
            return 0;
        }
    } else if (*DiploCounterProposal != DiploCounterThreaten) {
        parse_says(0, get_adjective(faction_id), -1, -1);
        parse_says(1, get_title(faction_id), -1, -1);
        parse_says(2, get_name(faction_id), -1, -1);
        parse_says(3, get_noun(target), -1, -1);
        X_dialog("REJWARTRUST", faction_id_2);
        return 1;
    } else {
        return 0;
    }
    if (bribe) {
        bool asked = false;
        bool paid = false;
        int turn_factor = (*CurrentTurn + 7) / 8;
        int sum_1 = 0;
        if (*DiploTradeWantTech1 >= 0) {
            sum_1 = turn_factor * mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1);
        }
        int sum_2 = sum_1;
        if (*DiploTradeWantTech2 >= 0) {
            sum_2 += turn_factor * mod_tech_val(*DiploTradeWantTech2, faction_id_2, 1);
        }
        int sum_3 = sum_2;
        if (*DiploTradeWantTech3 >= 0) {
            sum_3 += turn_factor * mod_tech_val(*DiploTradeWantTech3, faction_id_2, 1);
        }
        int sum_4 = sum_3;
        if (*DiploTradeWantTech4 >= 0) {
            sum_4 += turn_factor * mod_tech_val(*DiploTradeWantTech4, faction_id_2, 1);
        }
        if (!strcmp(m_cmp.filename, "UNIV")) {
            sum_1 *= 2;
            sum_2 *= 2;
            sum_3 *= 2;
            sum_4 *= 2;
        } else if (!strcmp(m_cmp.filename, "PEACE")) {
            // Fix: the original scales sum_4 first and then sets sums 1-3 to 1.5x that value
            sum_1 = 3 * sum_1 / 2;
            sum_2 = 3 * sum_2 / 2;
            sum_3 = 3 * sum_3 / 2;
            sum_4 = 3 * sum_4 / 2;
        } else if (!strcmp(m_cmp.filename, "MORGAN")) {
            sum_1 /= 2;
            sum_2 /= 2;
            sum_3 /= 2;
            sum_4 /= 2;
        }
        int limit = 4 * price / 3;
        if (sum_3 >= limit) {
            *DiploTradeWantTech4 = -1;
        }
        if (sum_2 >= limit) {
            *DiploTradeWantTech3 = -1;
        }
        if (sum_1 >= limit) {
            *DiploTradeWantTech2 = -1;
        }
        if (*DiploTradeWantTech1 >= 0 && strcmp(m_cmp.filename, "BELIEVE") && sum_4 >= price
        && (*DiploCounterProposal == DiploCounterResearchData
        || (*DiploCounterProposal == DiploCounterNameAPrice
        && (cmp.energy_credits > plr.energy_credits
        || sum_4 >= 3 * price / 2)))) {
            parse_says(0, get_noun(target), -1, -1);
            parse_says(1, get_title(target), -1, -1);
            parse_says(2, get_name(target), -1, -1);
            parse_tech(3, *DiploTradeWantTech1, 1);
            if (*DiploTradeWantTech2 >= 0) {
                parse_tech(4, *DiploTradeWantTech2, 1);
            }
            if (*DiploTradeWantTech3 >= 0) {
                parse_tech(8, *DiploTradeWantTech3, 1);
            }
            if (*DiploTradeWantTech4 >= 0) {
                parse_tech(9, *DiploTradeWantTech4, 1);
            }
            parse_says(5, get_title(faction_id_2), -1, -1);
            parse_says(6, get_adjective(target), -1, -1);
            asked = true;
            const char* tech_count = (*DiploTradeWantTech4 >= 0 ? "4" : *DiploTradeWantTech3 >= 0 ? "3"
                : *DiploTradeWantTech2 >= 0 ? "2" : "");
            snprintf(StrBuffer, StrBufLen, "MAYBEWARTECH%s%s", (allied ? "PACT" : remoteness ? "FAR" : ""), tech_count);
            if (X_dialog(StrBuffer, faction_id_2)) {
                paid = true;
                if (*DiploTradeWantTech4 >= 0) {
                    net_tech(faction_id_2, *DiploTradeWantTech4, faction_id, 1);
                }
                if (*DiploTradeWantTech3 >= 0) {
                    net_tech(faction_id_2, *DiploTradeWantTech3, faction_id, 1);
                }
                if (*DiploTradeWantTech2 >= 0) {
                    net_tech(faction_id_2, *DiploTradeWantTech2, faction_id, 1);
                }
                net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
                *DiploTradeWantTech4 = -1;
                *DiploTradeWantTech3 = -1;
                *DiploTradeWantTech2 = -1;
                *DiploTradeWantTech1 = -1;
            }
        } else if (strcmp(m_cmp.filename, "PEACE")
        && (*DiploCounterProposal == DiploCounterEnergyPayment || *DiploCounterProposal == DiploCounterNameAPrice)) {
            parse_says(0, get_noun(target), -1, -1);
            parse_says(1, get_pact(target), -1, -1);
            parse_says(2, MFactions[target].adj_insult_faction, -1, -1);
            parse_num(0, price);
            parse_says(6, get_adjective(target), -1, -1);
            snprintf(StrBuffer, StrBufLen, "MAYBEWARENERGY%s", (allied ? "PACT" : remoteness ? "FAR" : ""));
            Popup_start(&cur_popup, PopupScriptFile, StrBuffer, -1, 0, POPUP_EXTERNAL, 0);
            if (plr.energy_credits >= price) {
                Popup_start(&cur_popup, PopupScriptFile, "MAYBEWARPRICE", -1, 0,
                    POPUP_EXTERNAL|POPUP_PRE_INIT, 0);
            }
            Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], 0, 0);
            asked = true;
            if (BasePop_exec_3(&cur_popup, 0, pop_wait) > 0) {
                net_energy(faction_id, -price, faction_id_2, price, 1);
                paid = true;
            }
        }
        InfoWin->override_state = 0;
        Console_update_data(MapWin, 0);
        if (!paid) {
            if (!asked) {
                parse_says(0, get_title(target), -1, -1);
                parse_says(1, get_name(target), -1, -1);
                parse_says(2, get_him_her(target, 0), -1, -1);
                X_dialog("REJWAR", faction_id_2);
            }
            return 1;
        }
        incite = true;
    }
    if (incite) {
        diplo_lock(0);
        net_treaty_on(faction_id_2, target, DIPLO_UNK_80000000|DIPLO_VENDETTA|DIPLO_COMMLINK, 0);
        net_set_treaty(faction_id_2, target, DIPLO_UNK_40, 1, 0);
        net_set_treaty(faction_id_2, faction_id, DIPLO_UNK_80, 1, 0);
        parse_says(0, get_title(target), -1, -1);
        parse_says(1, get_name(target), -1, -1);
        parse_says(2, get_noun(target), -1, -1);
        parse_says(3, MFactions[target].adj_insult_faction, -1, -1);
        parse_says(4, get_phrase(target, 5), -1, -1);
        snprintf(StrBuffer, StrBufLen, "INCITED%d", random_val(0, 2));
        X_dialog(StrBuffer, faction_id_2);
        parse_says(3, get_pacts(target), -1, -1);
        parse_says(4, get_adjective(target), -1, -1);
        if (status_1t & DIPLO_PACT) {
            if (plr.diff_level > DIFF_SPECIALIST && !random_val(0, 2)) {
                if (!*MultiplayerActive) {
                    cause_friction(target, faction_id, 4);
                }
                X_pop("INCITEDPACT", pop_wait);
                net_pact_ends(faction_id, target, 1);
            }
        } else if ((status_1t & DIPLO_TREATY) && plr.diff_level > DIFF_SPECIALIST
        && !random_val(0, 2)) {
            if (!*MultiplayerActive) {
                cause_friction(target, faction_id, 4);
            }
            X_pop("INCITEDTREATY", pop_wait);
            net_treaty_on(faction_id, target, DIPLO_VENDETTA, 0);
            net_set_treaty(target, faction_id, DIPLO_WANT_REVENGE, 1, 1);
        }
        diplo_unlock();
    }
    return 1;
}

void __cdecl make_gift(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    MFaction& m_cmp = MFactions[faction_id_2];
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    *DiploSecondFaction = faction_id_2;
    char label[StrBufLen];
    switch (*DiploCounterProposal) {
        case DiploCounterResearchData:
        case DiploCounterMakeGift: {
            tech_analysis(faction_id, faction_id_2);
            if (*DiploTradeWantTech1 < 0) {
                parse_says(1, get_noun(faction_id_2), -1, -1);
                X_dialog("REJTECHNOTHING", faction_id_2);
                return;
            }
            int value = 0;
            if (*DiploCounterProposal == DiploCounterMakeGift) {
                for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
                    if (has_tech(tech_id, faction_id) && !has_tech(tech_id, faction_id_2)) {
                        value += mod_tech_val(tech_id, faction_id_2, 1);
                        net_tech(faction_id_2, tech_id, faction_id, 1);
                    }
                }
            } else {
                parse_tech(0, *DiploTradeWantTech1, 1);
                parse_says(2, get_noun_phrase(faction_id_2, 9), -1, -1);
                int choice;
                if (*DiploTradeWantTech2 >= 0) {
                    parse_tech(1, *DiploTradeWantTech2, 1);
                    choice = X_dialog("OFFERTECH2", faction_id_2);
                } else {
                    choice = X_dialog("OFFERTECH", faction_id_2);
                }
                if (!choice) {
                    return;
                }
                int tech_id = (choice == 1 ? *DiploTradeWantTech1 : *DiploTradeWantTech2);
                value = mod_tech_val(tech_id, faction_id_2, 1);
                net_tech(faction_id_2, tech_id, faction_id, 1);
            }
            // Rewrite all m_cmp.search_key references to m_cmp.filename in make_gift function
            if (!strcmp(m_cmp.filename, "UNIV") || !strcmp(m_cmp.filename, "SPARTANS")) {
                value *= 2;
            }
            if (!strcmp(m_cmp.filename, "BELIEVE")) {
                value /= 2;
            }
            cause_friction(faction_id_2, faction_id, -value);
            int32_t& gifts = plr.diplo_gifts[faction_id_2];
            const char* suffix = "";
            bool check_gifts = true;
            if (value < 16) {
                if (gifts) {
                    check_gifts = false;
                }
            } else {
                suffix = ((cmp.diplo_status[faction_id] & (DIPLO_UNK_40|DIPLO_WANT_REVENGE))
                    && *DiploFriction ? "2" : "1");
                net_treaty_off(faction_id_2, faction_id, DIPLO_UNK_40|DIPLO_WANT_REVENGE, 1);
                if (gifts < plr.integrity_blemishes) {
                    gifts++;
                }
            }
            if (check_gifts && (gifts < plr.integrity_blemishes
            || great_satan(faction_id, 0) || *MultiplayerActive)) {
                gifts++;
            }
            snprintf(label, sizeof(label), "GAVETECH%s", suffix);
            diplomacy_caption(faction_id, faction_id_2);
            X_dialog(label, faction_id_2);
            cmp.diplo_patience[faction_id] -= 2;
            return;
        }
        case DiploCounterEnergyPayment: {
            Popup_start(&cur_popup, PopupScriptFile, "OFFERENERGY", -1, 0, POPUP_EXTERNAL, 0);
            Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], 0, 0);
            int amount = 2 * plr.energy_credits;
            do {
                amount = 25 * (amount / 2 / 25);
                snprintf(label, sizeof(label), "%d", amount);
                Dialogs_item(&cur_popup.dialogs, label, amount);
            } while (amount);
            int gift = BasePop_exec_3(&cur_popup, 0, pop_wait);
            if (!gift) {
                return;
            }
            int value = gift;
            // Fix: the original tests the player faction here when the receiving AI is meant.
            // It also had two checks for multiply/divide for SPARTANS which cancel each other out.
            // BELIEVE is likely meant for the second matching the behavior in DiploCounterMakeGift.
            if (!strcmp(m_cmp.filename, "MORGAN") || !strcmp(m_cmp.filename, "SPARTANS")) {
                value = 2 * gift;
            }
            if (!strcmp(m_cmp.filename, "BELIEVE")) {
                value /= 2;
            }
            bool generous = !(value < cmp.energy_credits / 2
                || value < plr.energy_credits / 3
                || value < 2 * *CurrentTurn);
            bool decent = !(value < cmp.energy_credits / 4
                || value < 25
                || value < plr.energy_credits / 6
                || value < *CurrentTurn);
            net_energy(faction_id, -gift, faction_id_2, gift, 1);
            InfoWin->override_state = 0;
            Console_update_data(MapWin, 0);
            int friction = -3 * energy_value(value) / 2;
            cause_friction(faction_id_2, faction_id, friction);
            int32_t& gifts = plr.diplo_gifts[faction_id_2];
            bool check_gifts = true;
            const char* suffix = "";
            if (generous) {
                suffix = (((cmp.diplo_status[faction_id] & (DIPLO_UNK_40|DIPLO_WANT_REVENGE))
                    || great_satan(faction_id, 0)) ? "2" : "1");
                net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_40|DIPLO_WANT_REVENGE, 1);
                if (gifts < plr.integrity_blemishes) {
                    gifts++;
                }
            } else if (!decent || gifts) {
                check_gifts = false;
            }
            if (check_gifts && (gifts < plr.integrity_blemishes
            || great_satan(faction_id, 0) || *MultiplayerActive)) {
                gifts++;
            }
            // Fix diplomacy dialog to show the missing response messages (GAVEENERGY) when gifting
            // energy credits to another faction. The error happens when the label is written to
            // StrBuffer in make_gift but diplomacy_caption overwrites it with other data.
            snprintf(label, sizeof(label), "GAVEENERGY%s", suffix);
            diplomacy_caption(faction_id, faction_id_2);
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            X_dialog(label, faction_id_2);
            cmp.diplo_patience[faction_id] += -2 - generous;
            return;
        }
        case DiploCounterGiveBase: {
            int value = value_of_base(*DiploBidBaseSwapID, faction_id_2, faction_id, *DiploFearScale,
                (*DiploPrevHostility || *DiploAttackChance));
            int friction = -3 * value / 2;
            cause_friction(faction_id_2, faction_id, friction);
            // Fix: the original tests the player faction here and for BELIEVE below when the receiving AI is meant
            if (!strcmp(m_cmp.filename, "SPARTANS")) {
                value *= 2;
            }
            bool generous = !(value < cmp.energy_credits / 2
                || value < plr.energy_credits / 3
                || value < 2 * *CurrentTurn);
            if (!strcmp(m_cmp.filename, "BELIEVE")) {
                generous = true;
            }
            diplo_lock(0);
            diplomacy_caption(faction_id, faction_id_2);
            int32_t& gifts = plr.diplo_gifts[faction_id_2];
            const char* suffix = "";
            if (generous) {
                // Fix: restore missing dialog for BASEGIFT1/2 when the original always used plain BASEGIFT
                suffix = (((cmp.diplo_status[faction_id] & (DIPLO_UNK_40|DIPLO_WANT_REVENGE))
                    || great_satan(faction_id, 0)) ? "2" : "1");
                net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_40|DIPLO_WANT_REVENGE, 0);
                if (gifts < plr.integrity_blemishes) {
                    gifts++;
                }
            }
            if (gifts < plr.integrity_blemishes || great_satan(faction_id, 0) || *MultiplayerActive) {
                gifts++;
            }
            parse_says(0, get_title(faction_id), -1, -1);
            net_cede_base(*DiploBidBaseSwapID, faction_id_2, 1);
            diplo_unlock();
            cmp.diplo_patience[faction_id] += -2 - generous;
            snprintf(label, sizeof(label), "BASEGIFT%s", suffix);
            X_dialog(label, faction_id_2);
            break;
        }
    }
}

int __cdecl dont_withdrawal(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];

    *DiploSecondFaction = faction_id_2;
    if (plr.diplo_status[faction_id_2] & DIPLO_UNK_800000) {
        for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
            VEH* veh = &Vehs[veh_id];
            if (veh->faction_id == faction_id_2
            && veh->plan() != PLAN_PROBE
            && !is_ocean(veh->x, veh->y)
            && base_find_2(veh->x, veh->y, faction_id) >= 0
            && whose_territory(faction_id, veh->x, veh->y, 0, 0) == faction_id_2
            && veh->order == ORDER_MOVE_TO) {
                int base_id = base_at(veh->waypoint_x[0], veh->waypoint_y[0]);
                if (base_id >= 0) {
                    int owner = Bases[base_id].faction_id;
                    if (owner != faction_id && owner != faction_id_2 && owner
                    && (cmp.diplo_status[owner] & DIPLO_VENDETTA)) {
                        uint32_t status = plr.diplo_status[owner];
                        if ((status & DIPLO_PACT) || ((status & DIPLO_TREATY) && *DiploAlterFear > 0)) {
                            return owner;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int __cdecl do_withdrawal(int faction_id, int faction_id_2) {
    int count = 0;
    for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
        VEH* veh = &Vehs[veh_id];
        if (veh->faction_id != faction_id_2 || veh->plan() == PLAN_PROBE) {
            continue;
        }
        int x = veh->x;
        int y = veh->y;
        MAP* sq = mapsq(x, y);
        if (is_ocean(sq) || sq->owner != faction_id) {
            continue;
        }
        if (veh->triad() == TRIAD_LAND && veh_cargo(veh_id)) {
            for (int i = 0; i < 8; i++) {
                int nx = wrap(x + BaseOffsetX[i]);
                int ny = y + BaseOffsetY[i];
                if (!on_map(nx, ny)) {
                    continue;
                }
                MAP* nsq = mapsq(nx, ny);
                if (is_ocean(nsq)) {
                    if (stack_check(stack_fix(veh_at(nx, ny)), 8, faction_id_2, -1, -1) <= 0) {
                        continue;
                    }
                } else if (nsq->owner != faction_id_2) {
                    continue;
                }
                int owner = nsq->anything_at();
                if (owner < 0 || owner == faction_id_2) {
                    veh_drop(veh_lift(veh_id), nx, ny);
                    veh->visibility = 0;
                    veh->reset_order();
                    draw_tiles(x, y, 2);
                    count++;
                    break; // Fix: added break after the first suitable tile is found
                }
            }
        }
        int base_id = base_find_2(x, y, faction_id_2);
        if (base_id >= 0) {
            veh_drop(veh_lift(veh_id), Bases[base_id].x, Bases[base_id].y);
            veh->visibility = 0;
            veh->reset_order();
            draw_tile_fixup(x, y, has_abil(veh->unit_id, ABL_DEEP_RADAR), 2);
            count++;
        }
    }
    if (count) {
        set_agenda(faction_id_2, faction_id, AGENDA_UNK_2, 1);
        set_treaty(faction_id_2, faction_id, DIPLO_UNK_100000, 1);
    }
    return count;
}

int __cdecl demand_withdrawal(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];

    *DiploSecondFaction = faction_id_2;
    diplomacy_caption(faction_id, faction_id_2);
    uint32_t& status_21 = cmp.diplo_status[faction_id];
    uint32_t& status_12 = plr.diplo_status[faction_id_2];
    if (!(status_21 & DIPLO_UNK_200) && !(status_21 & DIPLO_PACT)
    && (*DiploCanBetray || *DiploAttackChance
    || (*DiploFriction > (*DiploPrevHostility != 0 ? 5 : 15) && !*DiploFearScale))) {
        net_treaty_on(faction_id_2, faction_id, DIPLO_VENDETTA, 1);
        plr.diplo_spoke[faction_id_2] = *CurrentTurn;
        if (*SunspotDuration <= 0) {
            diplomacy_caption(faction_id, faction_id_2);
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            snprintf(StrBuffer, StrBufLen, "NEVER");
            X_dialog(StrBuffer, faction_id_2);
        }
        return 2;
    }
    if (!(status_12 & DIPLO_PACT)) {
        int escort = dont_withdrawal(faction_id, faction_id_2);
        if (escort && (status_12 & DIPLO_TREATY)) {
            net_double_cross(faction_id_2, faction_id, -1, 1);
            if (*SunspotDuration <= 0) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_title(escort), -1, -1);
                parse_says(3, get_name(escort), -1, -1);
                X_dialog("RUNNINGDOG", faction_id_2);
                *DiploCanBetray = 0;
                plr.diplo_spoke[faction_id_2] = *CurrentTurn;
            }
            return 2;
        }
    }
    bool turnabout = false;
    if (!(status_12 & DIPLO_TREATY)) {
        cause_friction(faction_id_2, faction_id, 1);
        if (!(status_21 & DIPLO_WANT_REVENGE)
        && !*DiploCanBetray
        && !*DiploAttackChance
        && *DiploReputation <= *DiploFearScale + 3
        && (!*DiploPrevHostility || *DiploFearScale)) {
            if (territory(faction_id_2, faction_id, 0, 0, 0)) {
                if (*SunspotDuration > 0) {
                    return 1;
                }
                turnabout = true;
                parse_says(0, get_title(faction_id), -1, -1);
                // Fix TURNABOUT counter proposal when demanding withdrawal so that AI units are no longer
                // withdrawn when the player chooses "Never mind. Let our forces remain in place."
                if (!X_dialog("TURNABOUT", faction_id_2)) {
                    return 1;
                }
                net_withdrawal(faction_id_2, faction_id, 0);
            }
        } else {
            if (*SunspotDuration <= 0) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                X_dialog("NOWAYJOSE", faction_id_2);
            }
            return 1;
        }
    }
    int moved;
    if (*MultiplayerActive) {
        net_withdrawal(faction_id, faction_id_2, 1);
        moved = 1;
    } else {
        moved = do_withdrawal(faction_id, faction_id_2);
    }
    if (!turnabout && *SunspotDuration <= 0) {
        parse_says(0, get_name(faction_id), -1, -1);
        parse_says(1, get_adjective_leader(faction_id), -1, -1);
        if (moved) {
            if (status_12 & DIPLO_PACT) {
                parse_says(2, get_pact(faction_id), -1, -1);
                X_dialog("WITHDRAWPACT", faction_id_2);
            } else if (status_12 & DIPLO_TREATY) {
                X_dialog("WITHDRAW", faction_id_2);
            } else {
                X_dialog("WITHDRAWTRUCE", faction_id_2);
            }
            return 0;
        }
        X_dialog("NOWITHDRAW", faction_id_2);
    }
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

int __cdecl value_of_base(int base_id, int faction_id, int faction_id_2, int low_price, int high_price) {
    Faction& plr1 = Factions[faction_id];
    Faction& plr2 = Factions[faction_id_2];
    if (base_id < 0) {
        return -1;
    }
    BASE* base = &Bases[base_id];
    int vuln = vulnerable(faction_id, base->x, base->y);
    if (vuln <= 0) {
        return -1;
    }
    MAP* sq = mapsq(base->x, base->y);
    int region = sq->region;
    for (int i = 1; i < 169; i++) {
        int x = wrap(base->x + TableOffsetX[i]);
        int y = base->y + TableOffsetY[i];
        if (!on_map(x, y)) {
            continue;
        }
        int adj_base_id = base_at(x, y);
        if (adj_base_id < 0) {
            continue;
        }
        int adj_faction_id = Bases[adj_base_id].faction_id;
        bool same_region = mapsq(x, y)->region == region;
        if (adj_faction_id == faction_id) {
            if (same_region) {
                vuln--;
            }
            break;
        }
        if (same_region) {
            if (adj_faction_id != faction_id_2) {
                break;
            }
            vuln++;
        }
    }
    int veh_id = stack_fix(veh_at(base->x, base->y));
    int credits = max(plr1.energy_credits, plr2.energy_credits);
    int value = max((int)base->pop_size, 3) * ((credits + 1000) / (vuln + 4));
    value = max(value, 100);
    if (sq->veh_who() < 0) {
        value /= 2;
    }
    int bases1 = plr1.region_total_bases[region];
    int bases2 = plr2.region_total_bases[region];
    if (!bases2) {
        value *= 2;
    }
    if (bases1 == 1) {
        if (!bases2) {
            value *= 2;
        }
        if (bases2 == 1) {
            value *= 2;
        }
    }
    if (high_price) {
        value *= 4;
    }
    if (bases1 && bases2) {
        if (bases2 >= 5 * bases1) {
            value /= 2;
        }
        if (low_price) {
            value /= 2;
        }
    }
    int fac_count = 0;
    for (int i = Fac_ID_First; i <= Fac_ID_Last; i++) {
        if (base->has_fac_built((FacilityId)i)) {
            fac_count++;
            value += fac_count * Facility[i].cost;
        }
    }
    for (int i = 0; i < MaxSecretProjectNum; i++) {
        if (SecretProjects[i] == base_id) {
            value += 25 * Facility[SP_ID_First + i].cost;
        }
    }
    for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
        if (Vehs[i].faction_id == faction_id) {
            value += 2 * Units[Vehs[i].unit_id].cost;
        }
    }
    if (!strcmp(MFactions[faction_id].filename, "BELIEVE")) {
        value *= 2;
    }
    if (is_objective(base_id)) {
        value *= 4;
    }
    return value;
}

void __cdecl give_a_base(int base_id, int faction_id) {
    BASE* base = &Bases[base_id];
    int faction_id_old = base->faction_id;
    Faction& old_owner = Factions[faction_id_old];
    Faction& new_owner = Factions[faction_id];
    int x = base->x;
    int y = base->y;
    for (int i = 0; i < 21; i++) {
        int tile_x = wrap(x + TableOffsetX[i]);
        int tile_y = y + TableOffsetY[i];
        if (on_map(tile_x, tile_y)) {
            mapsq(tile_x, tile_y)->visibility |= 1 << faction_id;
            synch_bit(tile_x, tile_y, faction_id);
        }
    }
    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
        VEH* veh = &Vehs[veh_id];
        if (veh->faction_id != faction_id_old) {
            continue;
        }
        int dist = vector_dist(veh->x, veh->y, x, y);
        bool is_land = !is_ocean(veh->x, veh->y);
        if (dist > (is_land ? 1 : 0)) {
            continue;
        }
        int plan = veh->plan();
        bool transfer = ((plan == PLAN_DEFENSE || plan == PLAN_RECON) && !dist)
            || (veh->triad() == TRIAD_SEA && (!dist || is_land));
        int near_base_id = -1;
        if (!transfer) {
            base->faction_id = faction_id;
            near_base_id = base_find_2(x, y, faction_id_old);
            base->faction_id = faction_id_old;
            transfer = near_base_id < 0
                || map_range(Bases[near_base_id].x, Bases[near_base_id].y, x, y) == 0;
        }
        if (transfer) {
            --old_owner.units_active[veh->unit_id];
            veh->faction_id = faction_id;
            ++new_owner.units_active[veh->unit_id];
            veh->home_base_id = base_id;
            veh->moves_spent = 0;
            if (veh->order != ORDER_HOLD) {
                veh->order = ORDER_NONE;
            }
            owner_set(veh->x, veh->y, faction_id);
        } else {
            BASE* near_base = &Bases[near_base_id];
            stack_put(stack_veh(veh_id, 1), near_base->x, near_base->y);
            veh->order = ORDER_NONE;
            if (veh->home_base_id == base_id) {
                veh->home_base_id = near_base_id;
            }
        }
    }
    base->faction_id = faction_id;
    base->minerals_accumulated_2 = 0;
    base->minerals_accumulated = 0;
    for (int i = 0; i <= base->queue_size && i < 10; i++) {
        if (base->queue_items[i] >= 0) {
            --old_owner.units_queue[base->queue_items[i]];
        }
    }
    base->queue_size = 0;
    base->production_id_last = -FAC_STOCKPILE_ENERGY;
    base->queue_items[0] = -FAC_STOCKPILE_ENERGY;
    replay_base(1, x, y, faction_id);
    base->state_flags &= ~(BSTATE_DRONE_RIOTS_ACTIVE|BSTATE_GOLDEN_AGE_ACTIVE|BSTATE_COMBAT_LOSS_LAST_TURN
       |BSTATE_RESEARCH_DATA_STOLEN|BSTATE_ARTIFACT_ALREADY_LINKED|BSTATE_RENAME_BASE|BSTATE_UNK_200000);
    base->nerve_staple_turns_left = 0;
    if (*DiploCounterProposal == DiploCounterThreaten) {
        if (base->faction_id_former == faction_id) {
            base->assimilation_turns_left = 0;
        } else {
            base->faction_id_former = faction_id_old;
            base->assimilation_turns_left = conf.base_capture_fix
                ? base_capture_turns(base_id, faction_id) : 50;
        }
    } else {
        if (base->faction_id_former == faction_id) {
            base->assimilation_turns_left = 0;
        }
        base->faction_id_former = faction_id;
    }
    spot_base(base_id, faction_id_old);
    reset_territory();
    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
        if (veh_id < *VehCount && Vehs[veh_id].faction_id == faction_id_old
        && Vehs[veh_id].home_base_id == base_id) {
            kill(veh_id);
        }
    }
    owner_set(x, y, faction_id);
    draw_tile_fixup(x, y, 1, 2);
    set_base(base_id);
    base_compute(1);
}

int __cdecl base_swap(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    *DiploSecondFaction = faction_id_2;
    int ask_value = value_of_base(*DiploAskBaseSwapID, faction_id_2, faction_id, *DiploFearScale,
        *DiploPrevHostility || *DiploAttackChance);
    int bid_value = value_of_base(*DiploBidBaseSwapID, faction_id_2, faction_id, 0, 0);
    if (*DiploAskBaseSwapID >= 0) {
        BASE* ask_base = &Bases[*DiploAskBaseSwapID];
        parse_says(0, ask_base->name, -1, -1);
        // Fix: check that ask_value is non-negative to avoid situations where -1 is truncated to zero
        if (ask_value > 0) {
            if (ask_base->faction_id_former == faction_id) {
                ask_value /= 2;
            }
            if ((cmp.diplo_status[faction_id] & DIPLO_PACT)
            && (cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED)) {
                ask_value /= 2;
            }
        }
    }
    if (*DiploBidBaseSwapID >= 0) {
        BASE* bid_base = &Bases[*DiploBidBaseSwapID];
        parse_says(1, bid_base->name, -1, -1);
        if (bid_base->faction_id_former == faction_id_2) {
            bid_value *= 2;
        }
        // Fix: added separate headquarters check to refuse base swaps
        if (bid_value < 0 || plr.base_count == 1 || bid_base->has_fac_built(FAC_HEADQUARTERS)) {
            X_dialog("NOHQ", faction_id_2);
            return 1;
        }
    }
    if (*DiploAskBaseSwapID >= 0) {
        if (ask_value < 0 || cmp.base_count == 1
        || Bases[*DiploAskBaseSwapID].has_fac_built(FAC_HEADQUARTERS)) {
            X_dialog("NOMYHQ", faction_id_2);
            return 1;
        }
    }
    if ((cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE) && *DiploCounterProposal != DiploCounterThreaten) {
        X_dialog("NEVERBASESWAP", faction_id_2);
        return 1;
    }
    if (*DiploBidBaseSwapID >= 0) {
        if (*DiploAskBaseSwapID < 0) {
            return 1;
        }
        int bid_roll = bid_value + random_val(0, 100);
        // Fix: original used inverted check: ask_value + random_val(0, 100) <= bid_roll
        if (ask_value + random_val(0, 100) > bid_roll) {
            X_dialog("NOBASESWAP3", faction_id_2);
            return 1;
        }
        diplo_lock(0);
        net_cede_base(*DiploAskBaseSwapID, faction_id, 1);
        net_cede_base(*DiploBidBaseSwapID, faction_id_2, 1);
        diplo_unlock();
        X_dialog("YESBASESWAP", faction_id_2);
        return 0;
    }
    if (*DiploAskBaseSwapID < 0) {
        return 1;
    }
    // Fix: added zero division check
    int price = ask_value * (cmp.ranking + plr.ranking) / max(1, 2 * cmp.ranking);
    if (FactionRankings[7] == faction_id) {
        price *= 2;
    }
    int counter = *DiploCounterProposal;
    if ((counter == DiploCounterEnergyPayment || counter == DiploCounterNameAPrice)
    && plr.energy_credits >= price) {
        parse_num(0, price);
        if (!X_dialog("PAYBASESWAP", faction_id_2)) {
            return 1;
        }
        diplo_lock(0);
        net_energy(faction_id, -price, faction_id_2, price, 0);
        net_cede_base(*DiploAskBaseSwapID, faction_id, 1);
        diplo_unlock();
        X_dialog("BASEOK", faction_id_2);
        return 0;
    }
    if (counter == DiploCounterResearchData || counter == DiploCounterNameAPrice) {
        int tech_total = 0;
        for (int i = Tech_ID_First; i <= Tech_ID_Last; i++) {
            if (!has_tech(i, faction_id_2) && has_tech(i, faction_id)) {
                tech_total += mod_tech_val(i, faction_id_2, 1);
            }
        }
        if (cmp.tech_ranking / 2 > plr.tech_ranking / 2 + 2
        || tech_total * ((*CurrentTurn + 7) / 8) < price) {
            X_dialog("NOBASESWAP2", faction_id_2);
            return 1;
        }
        parse_says(2, get_name(faction_id_2), -1, -1);
        if (!X_dialog("TECHBASESWAP", faction_id_2)) {
            return 1;
        }
        diplo_lock(0);
        for (int i = Tech_ID_First; i <= Tech_ID_Last; i++) {
            if (!has_tech(i, faction_id_2) && has_tech(i, faction_id)) {
                net_tech(faction_id_2, i, faction_id, 0);
            }
        }
        net_cede_base(*DiploAskBaseSwapID, faction_id, 1);
        diplo_unlock();
        X_dialog("BASEOK", faction_id_2);
        return 0;
    }
    parse_num(0, price);
    X_dialog("NOBASESWAP", faction_id_2);
    return 1;
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

