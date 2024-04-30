
#include "gui_dialog.h"


int __cdecl X_pop2(const char* label, int a2)
{
    if (!strcmp(label, "MIMIMI") && !conf.warn_on_former_replace) {
        return 1;
    }
    /*
    Disable unnecessary warning if the map is larger than 128x128.
    In any case the dialog will limit all map sizes to 256x256.
    */
    if (!strcmp(label, "VERYLARGEMAP")) {
        return 0;
    }
    return X_pop("SCRIPT", label, -1, 0, 0, a2);
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

int base_trade_value(int base_id, int faction1, int faction2)
{
    if (base_id < 0 || base_id >= *BaseCount) {
        return 0;
    }
    BASE* base = &Bases[base_id];
    bool own_base = base->faction_id == faction2;
    bool pact = has_pact(faction1, faction2);
    int hq_id = find_hq(faction2);
    int value = 50*base->pop_size;
    int friction_score = *DiffLevel*2 + clamp(*DiploFriction, 0, 20)
        + 4*max(0, Factions[faction1].ranking - 4)
        + (Factions[faction1].pop_total > Factions[faction2].pop_total ? 4 : 0)
        + 4*Factions[faction1].integrity_blemishes
        + 2*Factions[faction1].atrocities;

    for (int i = Fac_ID_First; i < Fac_ID_Last; i++) {
        if (has_fac_built((FacilityId)i, base_id)) {
            value += Facility[i].cost * 20;
        }
    }
    for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
        if (SecretProjects[i - SP_ID_First] == base_id) {
            value += Facility[i].cost * (own_base && !pact ? 40 : 20)
                + 60 * clamp(project_score(faction2, (FacilityId)i, false), 0, 10);
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
    for (auto& m : iterate_tiles(base->x, base->y, 0, 21)) {
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
        value += 80;
        for (int i = 0; i < *VehCount; i++) {
            if ((Vehs[i].x == base->x && Vehs[i].y == base->y)
            || Vehs[i].home_base_id == base_id) {
                value += Units[Vehs[i].unit_id].cost * 20;
            }
        }
        if (num_own > 0) {
            value = value * (num_own == num_all ? 5 : 3) / 2;
        }
        value = value * (base->assimilation_turns_left > 8 ? 3 : 4)
            * (max(0, friction_score) + (pact ? 32 : 40)) / (32 * 4);
    } else {
        if (num_own < num_all) {
            value = value * (num_own ? 5 : 3) / 8;
        }
    }
    if (hq_id >= 0) {
        int dist = map_range(base->x, base->y, Bases[hq_id].x, Bases[hq_id].y);
        value = value * (72 - clamp(dist, 8, 40)) / 64;
    }
    if (base->is_objective() || *GameRules & RULES_SCN_VICT_ALL_BASE_COUNT_OBJ) {
        value *= 2;
    }
    debug("base_trade_value %s %d %d friction: %d num_own: %d num_all: %d value: %d\n",
    base->name, faction1, faction2, friction_score, num_own, num_all, value);
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
        parse_says(0, MFactions[faction1].title_leader, -1, -1);
        parse_says(1, MFactions[faction1].name_leader, -1, -1);
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
        f_plr.energy_credits -= cost_ask;
        f_cmp.energy_credits += cost_ask;
        give_a_base(*diplo_ask_base_swap_id, faction1);
        StatusWin_redraw(StatusWin);
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
        give_a_base(*diplo_ask_base_swap_id, faction1);
        give_a_base(*diplo_bid_base_swap_id, faction2);
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
    parse_says(0, MFactions[faction1].title_leader, -1, -1);
    parse_says(1, MFactions[faction1].name_leader, -1, -1);
    X_dialog("modmenu", "REJECTIDEA", faction2);
    return 0;
}

int __cdecl mod_energy_trade(int faction1, int faction2)
{
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];

    if (*diplo_current_proposal_id == DiploProposalNeedEnergy
    && *diplo_counter_proposal_id == DiploCounterResearchData
    && f_cmp.energy_credits < 100) {
        tech_analysis(faction1, faction2);
        parse_says(0, MFactions[faction1].title_leader, -1, -1);
        parse_says(1, MFactions[faction1].name_leader, -1, -1);
        if (*diplo_entry_id < 0) {
            X_dialog("REJSELLNONE", faction2);
        } else {
            X_dialog("REJSELLAFFORD", faction2);
        }
        return 0;
    }
    if (*MultiplayerActive || *diplo_current_proposal_id != DiploProposalNeedEnergy
    || *diplo_counter_proposal_id != DiploCounterLoanPayment) {
        return energy_trade(faction1, faction2);
    }

    bool is_pact = has_treaty(faction1, faction2, DIPLO_PACT);
    bool is_treaty = has_treaty(faction1, faction2, DIPLO_TREATY);
    int friction = clamp(*DiploFriction, 0, 20);
    int score = 5 - friction
        - 16*f_cmp.diplo_betrayed[faction1]
        - 8*f_plr.integrity_blemishes
        - 4*f_plr.atrocities
        + 2*f_plr.diplo_gifts[faction2]
        + (is_pact ? 10 : 0)
        + (is_treaty ? 5 : 0)
        + (want_revenge(faction2, faction1) ? -50 : 0)
        + (f_plr.pop_total > f_cmp.pop_total ? -5 : 0)
        + (f_plr.labs_total > 2*f_cmp.labs_total ? -5 : 0)
        + (2*f_plr.pop_total > 3*f_cmp.pop_total ? -10 : 0)
        + (3*f_plr.pop_total < 2*f_cmp.pop_total ? 5 : 0);

    for (int i=1; i < MaxPlayerNum; i++) {
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
    int reserve = clamp(*CurrentTurn * f_cmp.base_count / 8, 50, 200 + 20*friction);
    int amount = clamp(f_cmp.energy_credits - reserve, 0, *CurrentTurn * 4)
        * (f_plr.pop_total > f_cmp.pop_total ? 1 : 2)
        * clamp(20 + score - 16*f_cmp.AI_fight + 16*clamp(f_cmp.SE_economy_base, -1, 1)
        - 20*(f_plr.ranking > 5) + 20*is_pact, 16, 64) / (128 * 20) * 20;
    int turns = clamp(50 - *DiffLevel*5 - friction + min(20, score), 10, 50);
    int payment = ((18 + friction/4 + *DiffLevel*2)*amount + 15) / (16*turns);

    debug("energy_trade %s score: %d friction: %d credits: %d reserve: %d amount: %d turns: %d payment: %d\n",
    MFactions[faction2].filename, score, friction, f_cmp.energy_credits, reserve, amount, turns, payment);
    flushlog();

    if (f_plr.sanction_turns > 0 || score < -15) {
        parse_says(0, MFactions[faction1].title_leader, -1, -1);
        parse_says(1, MFactions[faction1].name_leader, -1, -1);
        X_dialog("modmenu", "REJECTIDEA", faction2);
        return 0;
    }
    if (f_plr.loan_balance[faction2] > 0) {
        X_dialog("modmenu", "REJECTLOAN", faction2);
        return 0;
    }
    if (amount < 10 || payment < 1 || score < 0) {
        X_dialog(random(2) ? "REJENERGY0" : "REJENERGY1", faction2);
        return 0;
    }

    parse_says(0, MFactions[faction1].title_leader, -1, -1);
    parse_says(1, MFactions[faction1].name_leader, -1, -1);
    ParseNumTable[0] = amount;
    ParseNumTable[1] = payment;
    ParseNumTable[2] = turns;
    int choice = X_dialog(random(2) ? "ENERGYLOAN1" : "ENERGYLOAN2", faction2);

    if (choice == 1) {
        f_plr.loan_balance[faction2] = turns * payment;
        f_plr.loan_payment[faction2] = payment;
        f_plr.energy_credits += amount;
        f_cmp.energy_credits -= amount;
        StatusWin_redraw(StatusWin);
    }
    return 0;
}

int tech_ask_value(int faction1, int faction2, int tech_id, bool high_price)
{
    CTech& tech = Tech[tech_id];
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];

    int level = clamp(tech_level(tech_id, 0), 1, 16);
    int owners = __builtin_popcount(TechOwners[tech_id]);
    int weights = clamp((f_cmp.AI_growth + 1) * tech.AI_growth
        + (f_cmp.AI_power + 1) * tech.AI_power
        + (f_cmp.AI_tech + 1) * tech.AI_tech
        + (f_cmp.AI_wealth + 1) * tech.AI_wealth, 0, 32);
    int value = (20 + 35*level + 4*level*level)
        * (64 + weights/2 + *DiffLevel*4 + clamp(*DiploFriction, 0, 20)
        + 8*f_plr.integrity_blemishes + 4*f_plr.atrocities) / 64;

    if (*GameRules & RULES_TECH_STAGNATION) {
        value = value * 3 / 2;
    }
    if (high_price) {
        value = value * 3 / 2;
    }
    if (f_plr.ranking > 5) {
        value = value * 5 / 4;
    }
    if (f_plr.pop_total > f_cmp.pop_total) {
        value = value * 5 / 4;
    }
    if (owners > 1) {
        value = value * 3 / 4;
    }
    if (has_pact(faction1, faction2)) {
        value = value * 3 / 4;
    }
    if (conf.revised_tech_cost && f_plr.tech_research_id == tech_id) {
        value = (int)(value * (1.0 - clamp(
            1.0 * f_plr.tech_accumulated / tech_cost(faction1, tech_id), 0.0, 0.7)));
    }
    value = (value + 24) / 25 * 25;

    debug("tech_ask_value %d %d level: %d owners: %d weights: %d value: %d tech: %d %s\n",
        faction1, faction2, level, owners, weights, value, tech_id, Tech[tech_id].name);
    flushlog();
    return value;
}

int __cdecl mod_buy_tech(int faction1, int faction2, int counter_id, bool high_price, int proposal_id)
{
    Faction& f_plr = Factions[faction1];
    Faction& f_cmp = Factions[faction2];

    if (*MultiplayerActive) {
        if (!has_pact(faction1, faction2)) {
            X_dialog(random(2) ? "REJTECHLATER0" : "REJTECHLATER1", faction2);
            return 1;
        }
        return buy_tech(faction1, faction2, counter_id, high_price, proposal_id);
    }

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
        int value = tech_ask_value(faction1, faction2, *diplo_tech_id1, high_price);
        if (value < 1) {
            return 0;
        }
        StrBuffer[0] = '\0';
        say_tech((int)StrBuffer, *diplo_tech_id1, 1);
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
            f_plr.energy_credits -= value;
            f_cmp.energy_credits += value;
            tech_achieved(faction1, *diplo_tech_id1, faction2, 0);
            StatusWin_redraw(StatusWin);
        }
        return 1;
    }
    return buy_tech(faction1, faction2, counter_id, high_price, proposal_id);
}

