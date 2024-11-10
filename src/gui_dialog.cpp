
#include "gui_dialog.h"


void parse_gen_name(int faction_id, size_t title_value, size_t name_value)
{
    *plurality_default = 0;
    *gender_default = MFactions[faction_id].is_leader_female;
    parse_says(title_value, MFactions[faction_id].title_leader, -1, -1);
    parse_says(name_value, MFactions[faction_id].name_leader, -1, -1);
}

void parse_noun_name(int faction_id, size_t title_value, size_t name_value)
{
    *plurality_default = 0;
    *gender_default = MFactions[faction_id].noun_gender;
    parse_says(title_value, MFactions[faction_id].title_leader, -1, -1);
    parse_says(name_value, MFactions[faction_id].name_leader, -1, -1);
}

int __cdecl X_pop2(const char* label, int a2)
{
    if (!conf.warn_on_former_replace && !strcmp(label, "MIMIMI")) {
        return 1;
    }
    /*
    Disable unnecessary warning if the map is larger than 128x128.
    In any case the dialog will limit all map sizes to 256x256.
    */
    if (!strcmp(label, "VERYLARGEMAP")) {
        return 0;
    }
    return X_pop(ScriptFile, label, -1, 0, 0, a2);
}

int __cdecl X_pop3(const char* filename, const char* label, int a3)
{
    return X_pop(filename, label, -1, 0, 0, a3);
}

int __cdecl X_pop7(const char* label, int a2, int a3)
{
    return X_pop(ScriptFile, label, -1, 0, a2, a3);
}

int __cdecl X_pops4(const char* label, int a2, Sprite* a3, int a4)
{
    return X_pops(ScriptFile, label, -1, 0, a2, (int)a3, 1, 1, a4);
}

int __cdecl X_dialog(const char* label, int faction2)
{
    return X_pops(ScriptFile, label, -1, 0, PopDialogUnk100000,
        (int)FactionPortraits[faction2], 1, 1, (int)sub_5398E0);
}

int __cdecl X_dialog(const char* filename, const char* label, int faction2)
{
    return X_pops(filename, label, -1, 0, PopDialogUnk100000,
        (int)FactionPortraits[faction2], 1, 1, (int)sub_5398E0);
}

/*
Pact status counts as infiltration on diplomacy window for displaying the energy reserves.
In original game energy reserves are always displayed on faction profile if pact is active.
*/
int __cdecl DiploPop_spying(int faction_id)
{
    return has_treaty(MapWin->cOwner, faction_id, DIPLO_PACT|DIPLO_HAVE_INFILTRATOR)
        || has_project(FAC_EMPATH_GUILD, MapWin->cOwner)
        || (MapWin->cOwner == *GovernorFaction && !is_alien(faction_id));
}

/*
Skip social engineering choices dialog SOCIETY while diplomacy is active.
*/
int __cdecl tech_achieved_pop3(const char* filename, const char* label, int a3)
{
    if (*DiploWinState) {
        return 0;
    }
    return X_pop3(filename, label, a3);
}

/*
Higher values indicate more hostile relations (broken treaties or similar).
*/
static int diplo_relation(int faction1, int faction2)
{
    assert(!is_human(faction2));
    return clamp(Factions[faction1].integrity_blemishes
        + Factions[faction2].diplo_betrayed[faction1]
        - Factions[faction2].diplo_gifts[faction1], -8, 8);
}

int __cdecl mod_threaten(int faction1, int faction2)
{
    MFaction& m_plr = MFactions[faction1];
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];

    if (!*MultiplayerActive && has_pact(faction2, faction1)
    && !has_treaty(faction2, faction1, DIPLO_HAVE_SURRENDERED)
    && (*diplo_current_proposal_id == DiploProposalTechTrade
    || *diplo_current_proposal_id == DiploProposalNeedEnergy)) {

        int friction = clamp(*DiploFriction, 0, 20);
        int score = 2*(*DiffLevel + friction)
            + 8*diplo_relation(faction1, faction2)
            + 8*clamp(f_cmp.AI_fight, -1, 1)
            + 4*clamp(f_cmp.ranking - f_plr.ranking, -4, 4)
            + clamp((f_cmp.pop_total - f_plr.pop_total)/32, -8, 8)
            + clamp((f_cmp.mil_strength_1 - f_plr.mil_strength_1)/32, -8, 8);

        if (score > random(64)) {
            f_cmp.diplo_patience[faction1] = 4 - (friction + 3) / 8;
            cause_friction(faction2, faction1, 2);
            *gender_default = m_plr.noun_gender;
            *plurality_default = 0;
            parse_says(0, m_plr.title_leader, -1, -1);
            parse_says(1, m_plr.name_leader, -1, -1);
            parse_says(2, (const char*)get_pact(faction1), -1, -1);
            X_dialog("ENDPACT", faction2);
            return pact_ends(faction1, faction2);
        }
    }
    return threaten(faction1, faction2);
}

int base_trade_value(int base_id, int faction1, int faction2)
{
    if (base_id < 0 || base_id >= *BaseCount) {
        return 0;
    }
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];
    BASE* base = &Bases[base_id];
    bool own_base = base->faction_id == faction2;
    bool pact = has_pact(faction1, faction2);
    int hq_id = find_hq(faction2);
    int value = 50*base->pop_size;
    int score = max(0, *DiffLevel*2 + clamp(*DiploFriction, 0, 20)
        + 2*clamp((f_plr.pop_total - f_cmp.pop_total)/32, -4, 4)
        + 2*clamp(f_plr.ranking - f_cmp.ranking, -4, 4)
        + 4*diplo_relation(faction1, faction2)
        + 2*f_plr.atrocities);

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
            if (sq->owner == faction2) {
                num_own++;
            }
            num_all++;
        }
    }
    for (const auto& m : iterate_tiles(base->x, base->y, 0, 21)) {
        if (mod_base_find3(m.x, m.y, -1, m.sq->region, -1, -1) == base_id) {
            if (m.sq->landmarks & ~(LM_DUNES|LM_SARGASSO|LM_UNITY)) {
                value += (m.sq->landmarks & LM_JUNGLE ? 20 : 15);
            }
            if (mod_bonus_at(m.x, m.y) > 0) {
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
    base->name, faction1, faction2, score, num_own, num_all, value);
    flushlog();
    return value;
}

int __cdecl mod_base_swap(int faction1, int faction2)
{
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];
    bool surrender = has_treaty(faction2, faction1, DIPLO_PACT)
        && has_treaty(faction2, faction1, DIPLO_HAVE_SURRENDERED);

    if (*MultiplayerActive || *PbemActive || *diplo_ask_base_swap_id < 0 || is_human(faction2)) {
        if (surrender) {
            return base_swap(faction1, faction2);
        }
        X_dialog("NEVERBASESWAP", faction2);
        return 0;
    }
    if (want_revenge(faction2, faction1) || f_cmp.diplo_betrayed[faction1] > 0) {
        X_dialog("NEVERBASESWAP", faction2);
        return 0;
    }
    if (*diplo_counter_proposal_id == DiploCounterThreaten) {
        cause_friction(faction2, faction1, 2);
        parse_gen_name(faction1, 0, 1);
        X_dialog("BASENOCEDE", faction2);
        return 0;
    }
    int cost_ask = base_trade_value(*diplo_ask_base_swap_id, faction1, faction2);
    parse_says(0, Bases[*diplo_ask_base_swap_id].name, -1, -1);
    ParseNumTable[0] = cost_ask;

    if (cost_ask < 1 || f_cmp.base_count == 1
    || has_fac_built(FAC_HEADQUARTERS, *diplo_ask_base_swap_id)) {
        X_dialog("NOBASESWAP2", faction2);
        return 0;
    }
    if (*diplo_counter_proposal_id == DiploCounterEnergyPayment
    || *diplo_counter_proposal_id == DiploCounterNameAPrice) {
        if (cost_ask > 5000 + clamp(*CurrentTurn, 0, 500)*40) {
            X_dialog("NOBASESWAP2", faction2);
            return 0;
        }
        if (8*f_plr.energy_credits < cost_ask) {
            X_dialog("NOBASESWAP2", faction2);
            return 0;
        }
        if (f_plr.energy_credits < cost_ask) {
            X_dialog("NOBASESWAP", faction2);
            return 0;
        }
        if (!X_dialog("PAYBASESWAP", faction2)) {
            return 0;
        }
        // net_energy also calls Console_update_data and StatusWin_redraw
        net_cede_base(*diplo_ask_base_swap_id, faction1, 1);
        net_energy(faction1, -cost_ask, faction2, cost_ask, 1);
        draw_map(1);
        return 0;
    }
    if (*diplo_counter_proposal_id == DiploCounterGiveBase && *diplo_bid_base_swap_id >= 0) {
        int cost_bid = base_trade_value(*diplo_bid_base_swap_id, faction1, faction2);
        parse_says(0, Bases[*diplo_ask_base_swap_id].name, -1, -1);
        parse_says(1, Bases[*diplo_bid_base_swap_id].name, -1, -1);

        if (cost_bid < 1 && !surrender) {
            X_dialog("NOBASESWAP2", faction2);
            return 0;
        }
        if (4*cost_bid < cost_ask && !surrender) {
            X_dialog("NOBASESWAP3", faction2);
            return 0;
        }
        if (cost_bid < cost_ask && !surrender) {
            X_dialog("NOBASESWAP", faction2);
            return 0;
        }
        X_dialog("YESBASESWAP", faction2);
        net_cede_base(*diplo_ask_base_swap_id, faction1, 1);
        net_cede_base(*diplo_bid_base_swap_id, faction2, 1);
        StatusWin_redraw(StatusWin);
        draw_map(1);
        return 0;
    }
    if (*diplo_counter_proposal_id == DiploCounterFriendship) {
        // skipped
    }
    if (*diplo_counter_proposal_id == DiploCounterResearchData) {
        // skipped
    }
    parse_gen_name(faction1, 0, 1);
    X_dialog("modmenu", "REJECTIDEA", faction2);
    return 0;
}

int __cdecl mod_energy_trade(int faction1, int faction2)
{
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];
    bool is_pact = has_treaty(faction1, faction2, DIPLO_PACT);
    bool is_treaty = has_treaty(faction1, faction2, DIPLO_TREATY);
    int prop_counter = *diplo_counter_proposal_id;
    *diplo_second_faction = faction2;
    tech_analysis(faction1, faction2); // diplo_entry_id

    if (*diplo_current_proposal_id != DiploProposalNeedEnergy) {
        assert(0);
        return 0;
    }

    if (is_pact && prop_counter != DiploCounterLoanPayment) {
        if (f_cmp.ranking > f_plr.ranking) {
            // Modify value for possible free gift for pact factions
            int value = 25 * (min(f_cmp.energy_credits/2 - 50,
                f_cmp.energy_credits/16 + f_cmp.base_count*5 + *CurrentTurn) / 25);
            if (value < 25 || value > f_cmp.energy_credits/2
            || f_cmp.energy_credits <= 2 * f_plr.energy_credits
            || f_cmp.energy_credits <= 4 * *CurrentTurn
            || f_cmp.tech_ranking <= f_plr.tech_ranking
            || f_cmp.diplo_patience[faction1] >= 2) {
                // Nothing
            } else if (*diplo_entry_id < 0 || prop_counter == DiploCounterFriendship) {
                net_energy(faction1, value, faction2, -value, 1);
                f_cmp.diplo_patience[faction1] += 2;
                parse_num(0, value);
                X_dialog(random(2) ? "ENERGYFREEBIE0" : "ENERGYFREEBIE1", faction2);
                return 0;
            }
        }
    }

    if (prop_counter == DiploCounterLoanPayment) {
        int friction = clamp(*DiploFriction, 0, 20);
        int score = 5 - friction
            - 8*diplo_relation(faction1, faction2)
            - 4*f_plr.atrocities
            + (is_pact ? 16 : (is_treaty ? 4 : 0))
            + (want_revenge(faction2, faction1) ? -20 : 0)
            + clamp((f_cmp.pop_total - f_plr.pop_total)/32, -8, 8)
            + clamp((f_cmp.labs_total - f_plr.labs_total)/64, -8, 8);

        for (int i = 1; i < MaxPlayerNum; i++) {
            if (Factions[i].base_count && i != faction1 && i != faction2) {
                if (at_war(i, faction1) && has_pact(i, faction2)) {
                    score -= 5;
                }
                if (at_war(i, faction2) && has_pact(i, faction1)) {
                    score -= 5;
                }
                if (at_war(i, faction1) && at_war(i, faction2)) {
                    score += 5;
                }
            }
        }
        int reserve = max(0, min(f_cmp.energy_credits - 50 - 20*friction,
            f_cmp.energy_credits/16 + 40*max(4, f_cmp.base_count + f_plr.base_count)));
        int amount = (reserve
            * clamp(32 - friction - 4*diplo_relation(faction1, faction2)
            - 8*clamp(f_cmp.AI_fight, -1, 1)
            + 8*clamp(f_cmp.SE_economy_base, -1, 1)
            + 4*clamp(f_cmp.ranking - f_plr.ranking, -4, 4)
            + (is_pact ? 32 : (is_treaty ? 8 : 0)), 12, 96) / 256) / 20 * 20;
        int turns = clamp(50 - *DiffLevel*5 - friction + score/2, 10, 50);
        int payment = ((20 + friction/4 + *DiffLevel*2)*amount + 15) / (16*turns);

        debug("energy_trade %s score: %d friction: %d credits: %d reserve: %d amount: %d turns: %d payment: %d\n",
        MFactions[faction2].filename, score, friction, f_cmp.energy_credits, reserve, amount, turns, payment);
        flushlog();

        if (f_plr.sanction_turns > 0 || score < -15) {
            parse_gen_name(faction1, 0, 1);
            X_dialog("modmenu", "REJECTIDEA", faction2);
            return 0;
        }
        if (f_plr.loan_balance[faction2] > 0) {
            X_dialog("modmenu", "REJECTLOAN", faction2);
            return 0;
        }
        if (amount < 10 || payment < 1 || score < 0 || f_cmp.energy_credits < 50) {
            X_dialog(random(2) ? "REJENERGY0" : "REJENERGY1", faction2);
            return 0;
        }
        ParseNumTable[0] = amount;
        ParseNumTable[1] = payment;
        ParseNumTable[2] = turns;
        parse_gen_name(faction1, 0, 1);

        int value = X_dialog(random(2) ? "ENERGYLOAN1" : "ENERGYLOAN2", faction2);
        if (value == 1) {
            net_loan(faction1, faction2, turns * payment, payment);
            net_energy(faction1, amount, faction2, -amount, 1);
            return 1;
        }
        return 0;
    }

    if ((prop_counter != DiploCounterNameAPrice || *diplo_entry_id >= 0)
    && (prop_counter == DiploCounterResearchData || prop_counter == DiploCounterNameAPrice)) {
        bool difficult = f_cmp.AI_fight > 0 && f_cmp.AI_power > 0;
        parse_gen_name(faction1, 0, 1);
        // Replace faction label reference on Believers with more generic code
        if (*diplo_entry_id >= 0 && (!difficult || f_cmp.SE_research_base > -2)) {
            int tech_value = tech_alt_val(*diplo_entry_id, faction2, 0);
            int alt_tech_value = (*diplo_tech_id2 < 0 ? 0 : tech_alt_val(*diplo_tech_id2, faction2, 0));
            int cost_limit = min(f_cmp.energy_credits/4,
                f_cmp.energy_credits/16 + f_cmp.base_count*5 + *CurrentTurn);
            int cost_value = tech_value
                * clamp(64 - clamp(*DiploFriction, 0, 20)
                - 8*diplo_relation(faction1, faction2)
                - 4*f_plr.atrocities, 16, 96) / 64;
            if (f_plr.ranking > f_cmp.ranking) {
                cost_value = cost_value * 7 / 8;
            }
            if (f_plr.pop_total > f_cmp.pop_total) {
                cost_value = cost_value * 7 / 8;
            }
            if (is_pact) {
                cost_value = cost_value * 5 / 4;
            } else if (is_treaty) {
                cost_value = cost_value * 9 / 8;
            }
            cost_value = clamp((min(cost_value / 4, cost_limit) / 25) * 25, 25, 1000);

            if (cost_value > f_cmp.energy_credits) { // Fix: check for sufficient energy to buy techs
                X_dialog("REJSELLAFFORD", faction2);
                return 0;
            }
            while (true) {
                StrBuffer[0] = '\0';
                say_tech(StrBuffer, *diplo_entry_id, 1);
                parse_says(0, StrBuffer, -1, -1);
                if (*diplo_tech_id2 >= 0) {
                    StrBuffer[0] = '\0';
                    say_tech(StrBuffer, *diplo_tech_id2, 1);
                    parse_says(1, StrBuffer, -1, -1);
                }
                parse_num(0, cost_value);
                int value;
                if (*diplo_tech_id2 < 0) {
                    value = X_dialog(random(2) ? "ENERGYTECH0" : "ENERGYTECH1", faction2);
                } else {
                    value = X_dialog("ENERGYTECH2", faction2);
                }
                if (!value) { // "Sorry, not interested."
                    break;
                }
                if (value == 3 || (value == 2 && *diplo_tech_id2 < 0)) { // Consult Datalinks.
                    help_topic(14, *diplo_entry_id);
                } else {
                    if (value == 2) { // "No, but I will sell you $TECH1 for the same price."
                        // Replace faction label references on Hive/University with more generic code
                        if ((difficult && tech_value > alt_tech_value && f_cmp.SE_research_base > -2)
                        || (tech_value > 2*alt_tech_value && f_cmp.SE_research_base < 2)) {
                            X_dialog("REJSELLSECOND", faction2);
                            return 0;
                        }
                        X_dialog("SELLSECOND", faction2);
                        net_tech(faction2, *diplo_tech_id2, faction1, 0);
                    } else { // Transmit information on $TECH0.
                        net_tech(faction2, *diplo_entry_id, faction1, 0);
                        *diplo_entry_id = *diplo_tech_id2;
                    }
                    *dword_7AD330 = 0;
                    *diplo_tech_id2 = -1;
                    net_energy(faction1, cost_value, faction2, -cost_value, 1); // NetDaemon_await_diplo
                    return 1;
                }
            }
            return 0;
        }
        X_dialog("REJSELLNONE", faction2);
        return 0;
    }
    // Energy trade rejected for any reason / DiploCounterFriendship
    X_dialog(random(2) ? "REJENERGY0" : "REJENERGY1", faction2);
    return 0;
}

int __cdecl mod_buy_tech(int faction1, int faction2, int counter_id, int high_price, int proposal_id)
{
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];

    if (proposal_id == DiploProposalTechTrade
    && (counter_id == DiploCounterEnergyPayment
    || counter_id == DiploCounterNameAPrice
    || counter_id == DiploCounterResearchData)) {
        if (*diplo_tech_id1 < 0) {
            return 0; // "no technical data in which you would be interested"
        }
        if (f_plr.sanction_turns > 0 || f_cmp.diplo_betrayed[faction1]
        || want_revenge(faction2, faction1)) {
            X_dialog(random(2) ? "REJTECHLATER0" : "REJTECHLATER1", faction2);
            return 1; // skip additional dialog replies
        }
        int value = (tech_alt_val(*diplo_tech_id1, faction2, 0) + 50)
            * clamp(*DiffLevel + 3, 4, 8)
            * clamp(32 + clamp(*DiploFriction, 0, 20)
            + 4*diplo_relation(faction1, faction2)
            + 2*f_plr.atrocities, 16, 96) / 256;
        if (f_plr.ranking > f_cmp.ranking) {
            value = value * 5 / 4;
        }
        if (f_plr.pop_total > f_cmp.pop_total) {
            value = value * 5 / 4;
        }
        if (has_treaty(faction1, faction2, DIPLO_PACT)) {
            value = value * 3 / 4;
        } else if (has_treaty(faction1, faction2, DIPLO_TREATY)) {
            value = value * 7 / 8;
        }
        if (high_price) {
            value = value * 5 / 4;
        }
        if (f_plr.tech_cost > 50 && f_plr.tech_research_id == *diplo_tech_id1) {
            value = (int)(value * (1.0 - clamp(
                1.0 * f_plr.tech_accumulated / f_plr.tech_cost, 0.0, 0.7)));
        }
        value = clamp((value / 25) * 25, 25, 10000);

        StrBuffer[0] = '\0';
        say_tech(StrBuffer, *diplo_tech_id1, 1);
        parse_says(0, StrBuffer, -1, -1);
        ParseNumTable[0] = value;

        if (8*f_plr.energy_credits < value) {
            X_dialog(random(2) ? "REJTECHLATER0" : "REJTECHLATER1", faction2);
            return 1;
        }
        if (f_plr.energy_credits < value) {
            X_dialog(random(2) ? "BUYTECHHIGH0" : "BUYTECHHIGH1", faction2);
            return 1;
        }
        if (X_dialog(random(2) ? "BUYTECH0" : "BUYTECH1", faction2) == 1) {
            net_energy(faction1, -value, faction2, value, 1);
            net_tech(faction1, *diplo_tech_id1, faction2, 1);
        }
        return 1;
    }
    return buy_tech(faction1, faction2, counter_id, high_price, proposal_id);
}

