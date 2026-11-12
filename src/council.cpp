
#include "council.h"

void __cdecl clear_council_notify(int faction_id) {
    CouncilProposal[faction_id] = 0;
    CouncilVoteState[faction_id] = 0;
}

void __cdecl clear_council_notify_2() {
    memset(CouncilProposal, 0, 0x20u);
    memset(CouncilVoteState, 0, 0x20u);
}

void __cdecl council(int faction_id, int proposal_id, int candidate_id) {
    int old_governor = *GovernorFaction;
    for (int i = 1; i < MaxPlayerNum; i++) {
        for (int j = 1; j < i; j++) {
            if (!MFactions[j].is_alien() && !MFactions[i].is_alien()) {
                treaty_on(i, j, DIPLO_COMMLINK);
            }
        }
    }
    if (*PbemActive && *CouncilSessionPending) {
        faction_id = *CouncilSessionPending;
        proposal_id = CouncWin->proposal_type;
        candidate_id = CouncWin->candidate_faction_id;
    } else {
        if (proposal_id != PROP_ELECT_PLANETARY_GOVERNOR) {
            Factions[candidate_id].council_call_turn = *CurrentTurn;
        }
        ProposalCallTurn[proposal_id] = *CurrentTurn;
    }
    if (faction_id != MapWin->cOwner || !is_human(faction_id)) {
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_says(2, Proposal[proposal_id].name, -1, -1);
        popp(ScriptFile, "CALLSCOUNCIL", 0, "council_sm.pcx", 0);
    }
    *GameState |= STATE_COUNCIL_HAS_CONVENED;
    CouncWin_init(CouncWin, proposal_id, candidate_id);
    if (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR
    && *GovernorFaction == MapWin->cOwner
    && old_governor != MapWin->cOwner
    && !*MultiplayerActive
    && (!*PbemActive || !*CouncilSessionPending)) {
        Faction& plr = Factions[MapWin->cOwner];
        if (!(plr.player_flags & PFLAG_BEEN_ELECTED_GOVERNOR)) {
            plr.player_flags |= PFLAG_BEEN_ELECTED_GOVERNOR;
            popp(ScriptFile, "GOVERNOR", 0, "council_sm.pcx", 0);
        }
    }
}

int __cdecl wants_prop(int faction_id_1, int proposal_id, int faction_id_2) {
    Faction& plr = Factions[faction_id_1];
    MFaction& m_plr = MFactions[faction_id_1];
    int score = 0;
    switch (proposal_id) {
        case PROP_ELECT_PLANETARY_GOVERNOR: {
            if (!is_alive(faction_id_2) || !eligible(faction_id_2)) {
                return -9;
            }
            Faction& tgt = Factions[faction_id_2];
            int soc_conflict = 0;
            int higher_count = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (i != faction_id_1) {
                    int votes = council_votes(i);
                    if (votes >= council_votes(faction_id_1)) {
                        higher_count++;
                    }
                }
            }
            uint32_t status = plr.diplo_status[faction_id_2];
            if (faction_id_2 == faction_id_1) {
                score = 5;
                if (higher_count <= 1) {
                    score = 6;
                }
                if (!higher_count) {
                    score++;
                }
                if (*GovernorFaction < 0) {
                    score = 2 * score - 1;
                }
                if (*GovernorFaction == faction_id_1) {
                    score += 2;
                } else if (*GovernorFaction >= 0
                && (plr.diplo_status[*GovernorFaction] & (DIPLO_UNK_40|DIPLO_WANT_REVENGE|DIPLO_VENDETTA))) {
                    score++;
                }
            } else if ((status & DIPLO_PACT) && (status & DIPLO_HAVE_SURRENDERED)) {
                score = 99;
            } else {
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (i != faction_id_1
                    && (plr.diplo_status[i] & DIPLO_PACT)
                    && (plr.diplo_status[i] & DIPLO_HAVE_SURRENDERED)
                    && CouncWin->faction_vote[i] == faction_id_2) {
                        score = 999;
                    }
                }
                if (*GovernorFaction == faction_id_1) {
                    score -= 2;
                }
                if (status & DIPLO_VENDETTA) {
                    score--;
                }
                if (status & (DIPLO_WANT_REVENGE|DIPLO_UNK_40)) {
                    score--;
                }
                if (status & DIPLO_ATROCITY_VICTIM) {
                    score--;
                }
                if (!score) {
                    if (status & DIPLO_PACT) {
                        score = 1;
                        if (!mod_wants_to_attack(faction_id_1, faction_id_2, 0)
                        && !mod_wants_to_attack(faction_id_2, faction_id_1, 0)) {
                            score = 2;
                        }
                    } else {
                        if (plr.AI_fight == -tgt.AI_fight) {
                            score = -1;
                        }
                        int priority_category = m_plr.soc_priority_category;
                        if (priority_category >= 0) {
                            int model = ((CSocialCategory*)&tgt.SE_Politics_pending)->models[priority_category];
                            if (model) {
                                if (model == m_plr.soc_priority_model) {
                                    score++;
                                } else {
                                    score--;
                                    soc_conflict = 1;
                                }
                            }
                        }
                        int opposition_category = m_plr.soc_opposition_category;
                        if (opposition_category >= 0) {
                            int model = ((CSocialCategory*)&tgt.SE_Politics_pending)->models[opposition_category];
                            if (model && model == m_plr.soc_opposition_model) {
                                soc_conflict = 1;
                                score--;
                            }
                        }
                        if (priority_category == MFactions[faction_id_2].soc_opposition_category
                        && m_plr.soc_priority_model == MFactions[faction_id_2].soc_opposition_model) {
                            soc_conflict = 2;
                            score -= 2;
                        }
                        if ((status & DIPLO_TREATY)
                        && !mod_wants_to_attack(faction_id_1, faction_id_2, 0)
                        && !mod_wants_to_attack(faction_id_2, faction_id_1, 0)) {
                            score++;
                        }
                        if (great_satan(faction_id_2, 0)) {
                            score--;
                        } else if (is_human(faction_id_2)) {
                            if (*MultiplayerActive
                            && faction_id_2 == *RankingFactionIDUnk1
                            && tgt.ranking >= 5) {
                                score--;
                            } else if (faction_id_2 != *RankingFactionIDUnk1
                            || tgt.diff_level < DIFF_TALENT) {
                                score++;
                            }
                        } else {
                            // Fix: the original used rank instead of FactionRankings[rank] as faction_id
                            // when checking diplo_status so the wrong faction could be favored.
                            int favored_id = -1;
                            for (int rank = MaxFacRank; rank >= 0; --rank) {
                                int ranked_id = FactionRankings[rank];
                                if (!is_human(ranked_id)
                                && !(plr.diplo_status[ranked_id] & (DIPLO_VENDETTA|DIPLO_WANT_REVENGE|DIPLO_UNK_40))) {
                                    favored_id = ranked_id;
                                    break;
                                }
                            }
                            if (faction_id_2 == favored_id) {
                                score++;
                            }
                        }
                    }
                    if (*GovernorFaction >= 0
                    && faction_id_2 != *GovernorFaction
                    && faction_id_1 != *GovernorFaction) {
                        uint32_t gov_status = plr.diplo_status[*GovernorFaction];
                        if ((gov_status & DIPLO_PACT) && !(status & DIPLO_PACT)) {
                            score -= 2;
                        }
                        if ((gov_status & DIPLO_TREATY) && !(status & DIPLO_TREATY)) {
                            score--;
                        }
                        if (gov_status & DIPLO_VENDETTA) {
                            score++;
                        }
                        if (gov_status & (DIPLO_UNK_40|DIPLO_WANT_REVENGE)) {
                            score += 2;
                        }
                    }
                    if (score >= 0) {
                        if (higher_count <= 1) {
                            score++;
                        }
                        if (!score && faction_id_2 == *GovernorFaction) {
                            score = 1;
                        }
                    }
                    if (!(status & DIPLO_PACT)) {
                        if (is_human(faction_id_2)
                        && tgt.diff_level >= DIFF_LIBRARIAN
                        && score > -2) {
                            score--;
                        }
                        if (is_human(faction_id_2)
                        && (*GameRules & RULES_INTENSE_RIVALRY)
                        && score > -2) {
                            score--;
                        }
                    }
                }
            }
            int vote = CouncWin->faction_vote[faction_id_2];
            if (vote > 0 && vote != faction_id_2) {
                if (faction_id_2 == faction_id_1) {
                    return max(score, 1);
                }
                score--;
                if (!is_human(faction_id_2)) {
                    score /= 2;
                }
            }
            if (faction_id_1 == faction_id_2) {
                return max(score, 1);
            }
            if (soc_conflict && !(status & DIPLO_PACT)) {
                score = min(score, 0);
            }
            if ((!is_human(faction_id_1) && !is_human(faction_id_2)) || (status & DIPLO_UNK_8000000)) {
                return score;
            }
            return min(score, 0);
        }
        case PROP_UNITE_SUPREME_LEADER: {
            if (!is_alive(faction_id_2) || !eligible(faction_id_2)) {
                return -9;
            }
            if (faction_id_2 == faction_id_1) {
                return plr.ranking != MaxFacRank ? 0 : 5;
            }
            Faction& tgt = Factions[faction_id_2];
            uint32_t status = plr.diplo_status[faction_id_2];
            if ((status & DIPLO_PACT) && (status & DIPLO_HAVE_SURRENDERED)) {
                return 2;
            }
            if (status & DIPLO_PACT) {
                if (!(*GameRules & RULES_VICTORY_COOPERATIVE)) {
                    return 0;
                }
                if (tgt.ranking <= plr.ranking
                || tgt.pop_total <= plr.pop_total
                || *CurrentTurn <= 200) {
                    if (plr.ranking <= tgt.ranking || plr.pop_total <= tgt.pop_total) {
                        return (*CurrentTurn > 200) - 1;
                    }
                    return -2;
                }
                return 1;
            }
            if (status & DIPLO_ATROCITY_VICTIM) {
                return -5;
            }
            if ((status & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA))
            || tgt.ranking != MaxFacRank
            || plr.ranking > 5
            || *CurrentTurn < 200
            || tgt.mil_strength_1 < 2 * plr.mil_strength_1
            || tgt.mil_strength_2 < 2 * plr.mil_strength_2
            || tgt.pop_total < 2 * plr.pop_total) {
                return -4;
            }
            return -2;
        }
        case PROP_SALVAGE_UNITY_CORE: {
            score = (plr.AI_power != 0);
            if (plr.AI_fight < 0) {
                score--;
            }
            if (plr.energy_credits >= 500) {
                score--;
            }
            if (plr.energy_credits < 100) {
                score++;
            }
            if (plr.energy_credits < 200) {
                score++;
            }
            int diff = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (i != faction_id_1) {
                    diff += (Factions[i].energy_credits <= plr.energy_credits) ? -1 : 1;
                    diff += (Factions[i].energy_surplus_total - Factions[i].facility_maint_total
                        <= plr.energy_surplus_total - plr.facility_maint_total) ? -1 : 1;
                }
            }
            if (diff <= 0) {
                return score;
            }
            return diff / 2 + score;
        }
        case PROP_GLOBAL_TRADE_PACT:
        case PROP_REPEAL_GLOBAL_TRADE_PACT: {
            int diff = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (i != faction_id_1) {
                    if (Factions[i].tech_commerce_bonus > plr.tech_commerce_bonus) {
                        diff--;
                    }
                    diff += bit_count(plr.diplo_status[i] & (DIPLO_PACT|DIPLO_TREATY));
                }
            }
            score = diff;
            if (plr.AI_wealth) {
                score++;
            }
            if (plr.AI_power) {
                score--;
            }
            if (plr.AI_fight > 0) {
                score--;
            }
            if (plr.SE_economy_pending > 1) {
                score++;
            }
            if (plr.SE_economy_pending < 0) {
                score--;
            }
            if (proposal_id != PROP_REPEAL_GLOBAL_TRADE_PACT) {
                return score;
            }
            return -score;
        }
        case PROP_LAUNCH_SOLAR_SHADE:
        case PROP_INCREASE_SOLAR_SHADE:
        case PROP_MELT_POLAR_CAPS: {
            int direction = (proposal_id != PROP_MELT_POLAR_CAPS) ? -1 : 1;
            int sea_dir = clamp(*MapSeaLevelCouncil, -1, 1);
            if (plr.AI_growth || m_plr.rule_psi) {
                if (!sea_dir || direction == sea_dir) {
                    score = m_plr.rule_psi ? -4 : -2;
                } else {
                    score = m_plr.rule_psi ? 4 : 2;
                }
            }
            if (sea_dir == direction) {
                score -= 2;
            } else if (!sea_dir) {
                score--;
            } else {
                if (*MapSeaLevel >= 0) {
                    score++;
                }
                if (plr.AI_power) {
                    score += (direction <= 0) ? 1 : -1;
                }
                if (plr.AI_wealth) {
                    score += (direction <= 0) ? -1 : 1;
                }
                if (plr.AI_tech) {
                    score += (direction > 0) ? 1 : -1;
                }
            }
            int total = 0;
            for (int i = 0; i < *BaseCount; i++) {
                BASE* base = &Bases[i];
                MAP* sq = mapsq(base->x, base->y);
                int impact = 0;
                if (is_ocean(sq)) {
                    if (direction < 0) {
                        impact = 1;
                        if (sq->contour - *MapSeaLevel >= AltNatural[3] - WorldBuilder->sea_level_rises) {
                            impact = 2;
                        }
                    } else {
                        impact = -1;
                    }
                } else if (direction <= 0) {
                    if (sq->alt_level() > ALT_SHORE_LINE) {
                        impact = -1;
                    }
                } else if (!(base->has_fac_built(FAC_PRESSURE_DOME) && is_shore_level(sq))) {
                    if (sq->contour - *MapSeaLevel > AltNatural[3] + WorldBuilder->sea_level_rises) {
                        if (is_shore_level(sq)) {
                            impact = -1;
                        } else if (sq->alt_level() > ALT_SHORE_LINE) {
                            impact = 1;
                        }
                    } else {
                        impact = -3;
                    }
                }
                if (base->faction_id == faction_id_1) {
                    total += 4 * impact * base->pop_size;
                } else {
                    total -= 2 * impact * base->pop_size * bit_count(plr.diplo_status[base->faction_id]
                        & (DIPLO_MAJOR_ATROCITY_VICTIM|DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE|DIPLO_VENDETTA));
                }
            }
            return total / clamp(plr.pop_total, 1, 99999) + score;
        }
        case PROP_REPEAL_UN_CHARTER: {
            if (!(has_tech(Chassis[CHS_MISSILE].preq_tech, faction_id_1)
            && has_tech(Weapon[WPN_PLANET_BUSTER].preq_tech, faction_id_1))) {
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (has_tech(Weapon[WPN_PLANET_BUSTER].preq_tech, i)
                    || has_tech(Chassis[CHS_MISSILE].preq_tech, i)) {
                        score = -2;
                        break;
                    }
                }
            }
            if (!plr.planet_busters) {
                if (score < 0) {
                    score--;
                }
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (Factions[i].planet_busters) {
                        score--;
                    }
                }
            }
            if (plr.AI_fight < 0) {
                score--;
            }
            if (plr.AI_tech && plr.AI_growth) {
                score -= 2;
            }
            score += plr.atrocities;
            if (score < 0) {
                return score;
            }
            if (plr.AI_fight > 0) {
                score++;
            }
            if (plr.AI_power) {
                score++;
            }
            return score;
        }
        case PROP_REINSTATE_UN_CHARTER: {
            if (!(has_tech(Chassis[CHS_MISSILE].preq_tech, faction_id_1)
            && has_tech(Weapon[WPN_PLANET_BUSTER].preq_tech, faction_id_1))) {
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (has_tech(Weapon[WPN_PLANET_BUSTER].preq_tech, i)
                    || has_tech(Chassis[CHS_MISSILE].preq_tech, i)) {
                        score = 2;
                        break;
                    }
                }
            }
            if (!plr.planet_busters) {
                if (score > 0) {
                    score++;
                }
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (Factions[i].planet_busters) {
                        score++;
                    }
                }
            }
            if (plr.AI_fight < 0) {
                score++;
            }
            if (plr.AI_tech && plr.AI_growth) {
                score += 2;
            }
            score -= plr.atrocities;
            if (score > 0) {
                return score;
            }
            if (plr.AI_fight > 0) {
                score--;
            }
            if (plr.AI_power) {
                score--;
            }
            return score;
        }
        case MaxProposalNum - 1:
            // last proposal slot has no vote logic
            return -9;
        default:
            return score;
    }
}

int __cdecl council_get_vote(int faction_id, int proposal_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    if (proposal_id != PROP_ELECT_PLANETARY_GOVERNOR) {
        int score = wants_prop(faction_id, proposal_id, faction_id_2);
        uint32_t status = 0;
        // Fix: motions without any candidate pass none (-1)
        // and the original then reads diplo_status outside bounds.
        if (faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
            status = plr.diplo_status[faction_id_2];
        }
        if (status & DIPLO_VENDETTA) {
            score--;
        }
        if (status & DIPLO_WANT_REVENGE) {
            score--;
        }
        if (status & DIPLO_PACT) {
            score++;
        }
        if (plr.unk_29[proposal_id]) {
            return plr.unk_29[proposal_id];
        }
        if (score > 0) {
            return COUNC_VOTE_YES;
        }
        if (score < 0) {
            return COUNC_VOTE_NO;
        }
        return COUNC_VOTE_YES - (faction_id + *CurrentTurn) % 3;
    }
    int best_score = 0;
    int best_id = COUNC_VOTE_ABSTAIN;
    for (int i = 1; i < MaxPlayerNum; i++) {
        int score = wants_prop(faction_id, PROP_ELECT_PLANETARY_GOVERNOR, i);
        if (score > best_score) {
            best_score = score;
            best_id = i;
        }
    }
    if (!plr.unk_29[0]) {
        return best_id;
    }
    int vote_id = plr.unk_30[0];
    // Fix: added bounds checking for the stored candidate. Fall back to the computed choice.
    if (vote_id < 0 || vote_id >= MaxPlayerNum) {
        return best_id;
    }
    if (plr.diplo_status[vote_id]
    & (DIPLO_ATROCITY_VICTIM|DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE|DIPLO_VENDETTA)) {
        return best_id;
    }
    return vote_id;
}

void __cdecl council_action(int proposal_id, int faction_id) {
    ProposalPassCount[proposal_id]++;
    ProposalCallTurn[proposal_id] = *CurrentTurn;
    switch (proposal_id) {
        case PROP_UNITE_SUPREME_LEADER: {
            *GameState |= STATE_VICTORY_DIPLOMATIC;
            if (faction_id == MapWin->cOwner) {
                bool defied = false;
                if (!*MultiplayerActive) {
                    for (int i = 1; i < MaxPlayerNum; i++) {
                        if (i != faction_id
                        && is_alive(i)
                        && (*GameRules & RULES_VICTORY_CONQUEST)
                        && ((Factions[i].diplo_status[faction_id] & (DIPLO_ATROCITY_VICTIM|DIPLO_MAJOR_ATROCITY_VICTIM))
                        || (Factions[faction_id].diplo_status[i] & DIPLO_MAJOR_ATROCITY_VICTIM)
                        || (Factions[i].diplo_agenda[faction_id] & AGENDA_UNK_800)
                        || (Factions[i].diplo_agenda[faction_id] & AGENDA_FIGHT_TO_DEATH))) {
                            defied = true;
                            parse_says(0, get_title(i), -1, -1);
                            parse_says(1, get_name(i), -1, -1);
                            parse_says(2, get_noun(i), -1, -1);
                            popp(ScriptFile, "DEFIES", 0, "dipvic_sm.pcx", 0);
                            treaty_on(i, faction_id, DIPLO_VENDETTA);
                            agenda_on(i, faction_id, AGENDA_PERMANENT|AGENDA_FIGHT_TO_DEATH);
                        }
                    }
                }
                if (!defied) {
                    *GameVictoryType = VIC_DIPLOMATIC_SOLO;
                    Factions[faction_id].player_flags |= PFLAG_UNK_20000;
                    *GameState |= STATE_GAME_DONE;
                } else {
                    for (int i = 1; i < MaxPlayerNum; i++) {
                        if (i != faction_id
                        && is_alive(i)
                        && !(Factions[i].diplo_agenda[faction_id] & AGENDA_FIGHT_TO_DEATH)) {
                            treaty_on(i, faction_id, DIPLO_UNK_40|DIPLO_TREATY|DIPLO_PACT);
                            treaty_off(i, faction_id, DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE);
                            agenda_off(i, faction_id, AGENDA_UNK_800|AGENDA_UNK_400);
                            set_treaty(i, faction_id, DIPLO_HAVE_SURRENDERED, 1);
                        }
                    }
                }
                return;
            }
            if ((*GameRules & RULES_VICTORY_COOPERATIVE)
            && (Factions[MapWin->cOwner].diplo_status[faction_id] & DIPLO_PACT)) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_noun(faction_id), -1, -1);
                if (*MultiplayerActive || !popp(ScriptFile, "ACCEDECOOP", 0, "dipvic_sm.pcx", 0)) {
                    *GameState |= STATE_GAME_DONE;
                    *GameVictoryType = VIC_DIPLOMATIC_COOP;
                    Factions[faction_id].player_flags |= PFLAG_UNK_20000;
                    return;
                }
            } else {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_noun(faction_id), -1, -1);
                if (*MultiplayerActive || !popp(ScriptFile, "ACCEDE", 0, "dipvic_sm.pcx", 0)) {
                    *GameState |= STATE_GAME_DONE;
                    *GameVictoryType = VIC_DIPLOMATIC_LOSS;
                    Factions[faction_id].player_flags |= PFLAG_UNK_20000;
                    return;
                }
            }
            popp(ScriptFile, "DEFY", 0, "council_sm.pcx", 0);
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (i != MapWin->cOwner && is_alive(i)) {
                    uint32_t status = Factions[i].diplo_status[MapWin->cOwner];
                    if (!(status & DIPLO_PACT) || !(status & DIPLO_HAVE_SURRENDERED)) {
                        treaty_on(i, MapWin->cOwner, DIPLO_UNK_40|DIPLO_VENDETTA);
                        if (i != faction_id) {
                            treaty_on(i, faction_id, DIPLO_PACT);
                            agenda_on(i, faction_id, AGENDA_PERMANENT);
                        }
                        set_agenda(i, MapWin->cOwner, AGENDA_FIGHT_TO_DEATH, 1);
                    }
                }
            }
            return;
        }
        case PROP_SALVAGE_UNITY_CORE:
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (!MFactions[i].is_alien()) {
                    Factions[i].energy_credits += 500;
                }
            }
            Console_update_data(MapWin, 0);
            return;
        case PROP_LAUNCH_SOLAR_SHADE:
        case PROP_INCREASE_SOLAR_SHADE:
            *MapSeaLevelCouncil -= 3;
            *ClimateFutureChange -= WorldBuilder->sea_level_rises;
            *ClimateValueA = abs(*ClimateFutureChange);
            *ClimateValueB = 20;
            *ClimateValueC = 0;
            parse_num(0, 20);
            parse_num(1, 1000 * WorldBuilder->sea_level_rises / 15);
            if (!MFactions[MapWin->cOwner].is_alien()) {
                popp(ScriptFile, "WILLFALL", 0, "seafal_sm.pcx", 0);
            } else {
                popp(ScriptFile, "WILLFALLALIEN", 0, "seafal_sm.pcx", 0);
            }
            return;
        case PROP_MELT_POLAR_CAPS:
            *MapSeaLevelCouncil += 3;
            *ClimateFutureChange += WorldBuilder->sea_level_rises;
            *ClimateValueA = abs(*ClimateFutureChange);
            *ClimateValueB = 20;
            *ClimateValueC = 0;
            parse_num(0, 20);
            parse_num(1, 1000 * WorldBuilder->sea_level_rises / 15);
            if (!MFactions[MapWin->cOwner].is_alien()) {
                popp(ScriptFile, "WILLRISE", 0, "searis_sm.pcx", 0);
            } else {
                popp(ScriptFile, "WILLRISEALIEN", 0, "searis_sm.pcx", 0);
            }
            return;
        default:
            return;
    }
}

int __cdecl can_call_council(int faction_id, int is_player) {
    if (MFactions[faction_id].is_alien()) {
        return 0;
    }
    if ((*GameMoreRules & MRULES_NO_PLANETARY_COUNCIL)
    && (!(*GameState & STATE_SCENARIO_EDITOR) || !(*GameState & STATE_OMNISCIENT_VIEW))) {
        return 0;
    }
    if (*SunspotDuration > 0) {
        if (is_player && faction_id == MapWin->cOwner && is_human(faction_id)) {
            NetMsg_pop(NetMsg, "SUNSPOTSNO2", -5000, 0, "solar_sm.pcx");
        }
        return 0;
    }
    if (*PbemActive && *CouncilSessionPending) {
        if (is_player && faction_id == MapWin->cOwner && is_human(faction_id)) {
            NetMsg_pop(NetMsg, "COUNCILHOT", -5000, 0, 0);
        }
        return 0;
    }
    if (has_project(FAC_EMPATH_GUILD, faction_id)) {
        return 1;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (MFactions[i].is_alien()
        || !is_alive(i)
        || i == faction_id
        || (Factions[faction_id].diplo_status[i] & (DIPLO_UNK_8000000|DIPLO_COMMLINK))
        || ((*GameState & STATE_OMNISCIENT_VIEW) && is_human(faction_id) && faction_id == MapWin->cOwner)) {
            continue;
        }
        if (!*MultiplayerActive || !is_human(faction_id) || !is_human(i)) {
            if (is_player && faction_id == MapWin->cOwner && is_human(faction_id)) {
                NetMsg_pop(NetMsg, "COUNCILNEEDALL", -5000, 0, "council_sm.pcx");
            }
            return 0;
        }
    }
    return 1;
}

void __cdecl call_council(int faction_id) {
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    if (is_alien(faction_id)) {
        return;
    }
    if (!faction_id) {
        return;
    }
    if (*ControlTurnC && (is_human(faction_id) || *MultiplayerActive)) {
        return;
    }
    if (!can_call_council(faction_id, faction_id == MapWin->cOwner && is_human(faction_id))) {
        return;
    }
    if (!is_human(faction_id) && *GovernorFaction > 0 && ((*CurrentTurn + 2 * faction_id) & 0xF)) {
        return;
    }
    if (!is_human(faction_id) && (*GameState & STATE_GAME_DONE)) {
        return;
    }
    if (*MultiplayerActive
    && (*GameMoreRules & MRULES_UNK_10)
    && is_human(faction_id)
    && *CurrentFaction != faction_id) {
        NetMsg_pop(NetMsg, "SIMULNO", 5000, 0, 0);
        return;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (!is_human(faction_id) && is_human(i) && MFactions[i].is_alien()) {
            return;
        }
    }
    // Fix: the original only set alien_alive flag while determining the Supreme Leader motion
    // and used undefined value otherwise. It is now computed before the loop and only blocks
    // the Supreme Leader motion, see also NOTWHILEALIENSLIVE.
    bool alien_alive = false;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (MFactions[i].is_alien() && is_alive(i)) {
            alien_alive = true;
        }
    }
    int best_score = 0;
    int best_proposal = -1;
    int target_id = faction_id;
    int item_count = 0;
    int w = 0, h = 0;
    Popup_start(&cur_popup, PopupScriptFile, "COUNCILISSUES", -1, 0, POPUP_BTN_CANCEL, 0);
    Popup_init_icon(&cur_popup, "council_sm.pcx", &w, &h);
    for (int proposal_id = 0; proposal_id < MaxProposalNum; proposal_id++) {
        if ((has_tech(Proposal[proposal_id].preq_tech, faction_id)
        || ((*GameState & STATE_OMNISCIENT_VIEW) && (*GameState & STATE_DEBUG_MODE)))
        && Proposal[proposal_id].preq_tech >= -1) {
            bool add = false;
            switch (proposal_id) {
                case PROP_ELECT_PLANETARY_GOVERNOR: {
                    if (!eligible(faction_id) && !is_human(faction_id) && *GovernorFaction > 0) {
                        break;
                    }
                    if (*GovernorFaction != faction_id || is_human(faction_id)) {
                        if (is_human(faction_id)) {
                            add = true;
                        } else if (*GovernorFaction <= 0
                        || !(Factions[faction_id].diplo_status[*GovernorFaction] & DIPLO_PACT)
                        || (Factions[faction_id].ranking >= Factions[*GovernorFaction].ranking
                        && !(Factions[faction_id].diplo_status[*GovernorFaction] & DIPLO_HAVE_SURRENDERED))) {
                            int votes_total = 0;
                            int votes_for = 0;
                            for (int i = 1; i < MaxPlayerNum; i++) {
                                votes_total += council_votes(i);
                                if (council_get_vote(i, 0, target_id) == faction_id) {
                                    votes_for += council_votes(i);
                                }
                            }
                            if (!(2 * votes_for < votes_total
                            && *GovernorFaction > 0
                            && !is_human(*GovernorFaction))) {
                                add = true;
                            }
                        }
                    }
                    break;
                }
                case PROP_UNITE_SUPREME_LEADER:
                    if ((*GameRules & RULES_VICTORY_DIPLOMATIC)
                    && !(*GameState & STATE_GAME_DONE)
                    && !ProposalPassCount[PROP_UNITE_SUPREME_LEADER]
                    && eligible(faction_id)) {
                        add = true;
                    }
                    break;
                case PROP_SALVAGE_UNITY_CORE:
                case PROP_LAUNCH_SOLAR_SHADE:
                    if (!ProposalPassCount[proposal_id]) {
                        add = true;
                    }
                    break;
                case PROP_GLOBAL_TRADE_PACT:
                    if (ProposalPassCount[PROP_GLOBAL_TRADE_PACT]
                    <= ProposalPassCount[PROP_REPEAL_GLOBAL_TRADE_PACT]) {
                        add = true;
                    }
                    break;
                case PROP_REPEAL_GLOBAL_TRADE_PACT:
                    if (ProposalPassCount[PROP_REPEAL_GLOBAL_TRADE_PACT]
                    != ProposalPassCount[PROP_GLOBAL_TRADE_PACT]) {
                        add = true;
                    }
                    break;
                case PROP_INCREASE_SOLAR_SHADE:
                    if (ProposalPassCount[PROP_LAUNCH_SOLAR_SHADE]) {
                        add = true;
                    }
                    break;
                case PROP_REPEAL_UN_CHARTER:
                    if (ProposalPassCount[PROP_REPEAL_UN_CHARTER]
                    <= ProposalPassCount[PROP_REINSTATE_UN_CHARTER]) {
                        add = true;
                    }
                    break;
                case PROP_REINSTATE_UN_CHARTER:
                    if (ProposalPassCount[PROP_REINSTATE_UN_CHARTER]
                    < ProposalPassCount[PROP_REPEAL_UN_CHARTER]) {
                        add = true;
                    }
                    break;
                default:
                    add = true;
                    break;
            }
            if (add) {
                snprintf(StrBuffer, StrBufLen, "%s|%s",
                    Proposal[proposal_id].name, Proposal[proposal_id].description);
                Dialogs_item(&cur_popup.dialogs, StrBuffer, proposal_id);
                item_count++;
                if (!is_human(faction_id)) {
                    bool pick_governor = false;
                    if (Factions[faction_id].unk_29[proposal_id]) {
                        if (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR) {
                            int preset_id = Factions[faction_id].unk_30[0];
                            // Fix: when stored preset_id is outside the faction range, the candidate is replaced
                            pick_governor = preset_id < 0 || preset_id >= MaxPlayerNum || !eligible(preset_id);
                        }
                    } else if (proposal_id == PROP_ELECT_PLANETARY_GOVERNOR) {
                        pick_governor = true;
                    } else {
                        int score = wants_prop(faction_id, proposal_id, faction_id);
                        // Fix: only the Supreme Leader motion is blocked while an alien faction is alive
                        bool blocked = (proposal_id == PROP_UNITE_SUPREME_LEADER && alien_alive);
                        if (!blocked && score > best_score) {
                            best_score = score;
                            best_proposal = proposal_id;
                            target_id = faction_id;
                        }
                    }
                    if (pick_governor) {
                        for (int i = 1; i < MaxPlayerNum; i++) {
                            if (i != *GovernorFaction && eligible(i)) {
                                int score = wants_prop(faction_id, PROP_ELECT_PLANETARY_GOVERNOR, i);
                                if (score > best_score) {
                                    best_score = score;
                                    best_proposal = PROP_ELECT_PLANETARY_GOVERNOR;
                                    target_id = i;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (!item_count) {
        return;
    }
    int choice;
    if (is_human(faction_id)) {
        choice = BasePop_exec_3(&cur_popup, 0, 0);
    } else {
        choice = best_proposal;
    }
    if (alien_alive && choice == PROP_UNITE_SUPREME_LEADER) {
        if (faction_id == MapWin->cOwner && is_human(faction_id)) {
            NetMsg_pop(NetMsg, "NOTWHILEALIENSLIVE", -5000, 0, "council_sm.pcx");
        }
        return;
    }
    if (choice < 0) {
        return;
    }
    if (choice
    && (!(*GameState & STATE_OMNISCIENT_VIEW) || !(*GameState & STATE_DEBUG_MODE) || !is_human(faction_id))) {
        int min_turns = Rules->min_turns_between_councils;
        if (faction_id == *GovernorFaction) {
            min_turns = Rules->min_turns_between_councils / 2;
        }
        if (*CurrentTurn - Factions[faction_id].council_call_turn < min_turns) {
            if (faction_id == MapWin->cOwner && is_human(faction_id)) {
                int year = game_year(Factions[MapWin->cOwner].council_call_turn);
                parse_num(0, year);
                parse_num(1, Rules->min_turns_between_councils);
                parse_num(2, Rules->min_turns_between_councils / 2);
                NetMsg_pop(NetMsg, "COUNCILRECENTCALL", -5000, 0, "council_sm.pcx");
            }
            return;
        }
    }
    if (*CurrentTurn - ProposalCallTurn[choice] >= Rules->min_turns_between_councils
    || ((*GameState & STATE_OMNISCIENT_VIEW) && (*GameState & STATE_DEBUG_MODE) && is_human(faction_id))) {
        if (*MultiplayerActive) {
            if (!is_human(faction_id)) {
                *CouncilNetA = faction_id;
                *CouncilNetB = choice;
                *CouncilNetC = target_id;
                return;
            }
            NetMsg_pop(NetMsg, "CONVENING", 3000, 0, 0);
            if (diplo_lock(50)) {
                NetMsg_close(NetMsg);
                return;
            }
            message_data(0x2600, 0, faction_id, choice, target_id, 0);
            *ControlWaitLoop = 1;
            do {
                if (*ControlTurnA) {
                    break;
                }
                wait_loop();
            } while (*ControlWaitLoop);
            diplo_unlock();
        } else {
            council(faction_id, choice, target_id);
        }
        return;
    }
    if (faction_id == MapWin->cOwner && is_human(faction_id)) {
        int year = game_year(ProposalCallTurn[choice]);
        parse_num(0, year);
        parse_num(1, Rules->min_turns_between_councils);
        NetMsg_pop(NetMsg, "COUNCILRECENTPROP", -5000, 0, "council_sm.pcx");
    }
}

