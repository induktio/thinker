
#include "score.h"

/*
Calculate score percentage scaling for the end game screen.
*/
int __cdecl get_rating(int faction_id, int score) {
    int diff_level = Factions[faction_id].diff_level;
    int value = diff_level;
    for (int level = 3; level <= diff_level; level++) {
        value += (level - 1) / 2;
    }
    if (*GameRules & RULES_INTENSE_RIVALRY) {
        value++;
    }
    return score * (value + 6) / 160;
}

/*
Determine whether the base is an objective for scenario victory conditions.
*/
int __cdecl is_objective(int base_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        assert(0);
        return false;
    }
    if (*GameRules & RULES_SCN_VICT_ALL_BASE_COUNT_OBJ
    || Bases[base_id].event_flags & BEVENT_OBJECTIVE) {
        return true;
    }
    if (*GameRules & RULES_SCN_VICT_SP_COUNT_OBJ) {
        for (int i = 0; i < MaxSecretProjectNum; i++) {
            if (SecretProjects[i] == base_id) {
                return true;
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_BASE_FACIL_COUNT_OBJ
    && *ScnVictFacilityObj >= 0 && *ScnVictFacilityObj <= 64
    && has_fac_built((FacilityId)(*ScnVictFacilityObj), base_id)) {
        return true;
    }
    return false;
}

int __cdecl num_objectives(int faction_id, int incl_pact) {
    if (!is_alive(faction_id)) {
        return 0;
    }
    int score = Factions[faction_id].unk_101;
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].faction_id != faction_id || !(Vehs[i].flags & VFLAG_IS_OBJECTIVE)) {
            continue;
        }
        if (*GameRules & RULES_SCN_VICT_OBJ_UNITS_REACH_FRIEND_OBJ_BASE) {
            int base_id = base_at(Vehs[i].x, Vehs[i].y);
            if (base_id < 0 || !is_objective(base_id)) {
                continue;
            }
            int owner = Bases[base_id].faction_id;
            if (owner == faction_id || ((Factions[faction_id].diplo_status[owner] & DIPLO_PACT)
            && (*GameRules & RULES_VICTORY_COOPERATIVE))) {
                score++;
            }
        } else if (*GameRules & RULES_SCN_VICT_OBJ_UNITS_REACH_FRIEND_HQ_BASE) {
            int base_id = base_at(Vehs[i].x, Vehs[i].y);
            if (base_id < 0 || !has_fac_built(FAC_HEADQUARTERS, base_id)) {
                continue;
            }
            int owner = Bases[base_id].faction_id;
            if (owner == faction_id || ((Factions[faction_id].diplo_status[owner] & DIPLO_PACT)
            && (*GameRules & RULES_VICTORY_COOPERATIVE))) {
                score++;
            }
        } else {
            score++;
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id != faction_id) {
            continue;
        }
        if (*GameState & STATE_SCN_VICT_POPULATION_COUNT_OBJ) {
            score += Bases[i].pop_size;
        }
        if ((*GameRules & RULES_SCN_VICT_ALL_BASE_COUNT_OBJ)
        || (Bases[i].event_flags & BEVENT_OBJECTIVE)) {
            score++;
        }
    }
    if (*GameState & STATE_SCN_VICT_TECH_COUNT_OBJ) {
        for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
            if (has_tech(tech_id, faction_id)) {
                score++;
            }
        }
        score += Factions[faction_id].tech_count_transcendent;
    }
    if (*GameState & STATE_SCN_VICT_CREDITS_COUNT_OBJ) {
        score += Factions[faction_id].energy_credits;
    }
    if (*GameState & STATE_SCN_VICT_BASE_FACIL_COUNT_OBJ) {
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == faction_id
            && *ScnVictFacilityObj <= Fac_ID_Last
            && has_fac_built((FacilityId)*ScnVictFacilityObj, i)) {
                score++;
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_TERRAIN_ENH_COUNT_OBJ) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y & 1; x < *MapAreaX; x += 2) {
                MAP* sq = mapsq(x, y);
                if (sq->owner == faction_id) {
                    score += bit_count(sq->items & (BIT_SENSOR|BIT_THERMAL_BORE|BIT_ECH_MIRROR|BIT_CONDENSER|\
                        BIT_FOREST|BIT_AIRBASE|BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD));
                }
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_TERRITORY_COUNT_OBJ) {
        for (int y = 0; y < *MapAreaY; y++) {
            for (int x = y & 1; x < *MapAreaX; x += 2) {
                if (mapsq(x, y)->owner == faction_id) {
                    score++;
                }
            }
        }
    }
    if (*GameRules & RULES_SCN_VICT_SP_COUNT_OBJ) {
        for (int i = 0; i < MaxSecretProjectNum; i++) {
            if (SecretProjects[i] >= 0 && Bases[SecretProjects[i]].faction_id == faction_id) {
                score++;
            }
        }
    }
    if (incl_pact && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && (Factions[faction_id].diplo_status[i] & DIPLO_PACT)) {
                score += num_objectives(i, 0);
            }
        }
    }
    return score;
}

int __cdecl most_objectives(int* winner_id, int* tied_rivals) {
    int best_score = 0;
    int best_fc_id = 0;
    if (tied_rivals) {
        *tied_rivals = 0;
    }
    for (int fc_id = 1; fc_id < MaxPlayerNum; fc_id++) {
        int score = num_objectives(fc_id, *GameRules & RULES_VICTORY_COOPERATIVE);
        if (score >= best_score) {
            if (tied_rivals) {
                if (score > best_score) {
                    *tied_rivals = 0;
                } else {
                    if (!(Factions[fc_id].diplo_status[best_fc_id] & DIPLO_PACT)
                    || !(*GameRules & RULES_VICTORY_COOPERATIVE)) {
                        ++(*tied_rivals);
                    }
                }
            }
            best_score = score;
            best_fc_id = fc_id;
        }
    }
    if (winner_id) {
        *winner_id = best_fc_id;
    }
    return best_score;
}

void __cdecl rankings(int flag) {
    int max_weapon_value = 1;
    for (int unit_id = MaxProtoFactionNum; unit_id < MaxProtoNum; unit_id++) {
        if (Units[unit_id].is_active() && Units[unit_id].is_prototyped()) {
            int offense_value = Units[unit_id].offense_value();
            if (offense_value < 99) {
                max_weapon_value = max(max_weapon_value, offense_value);
            }
        }
    }
    for (int fc_id = 0; fc_id < MaxPlayerNum; fc_id++) {
        Faction* plr = &Factions[fc_id];
        if (flag) {
            plr->ranking = 0;
        }
        if (fc_id == 0) {
            continue;
        }
        int score;
        if (!is_alive(fc_id)) {
            score = 0;
        } else if ((*ObjectiveReqVictory < 9000 || *ObjectivesSuddenDeathVictory < 9000)
        && !*ObjectiveAchievePts) {
            score = 10 * num_objectives(fc_id, 0);
        } else {
            score = 4 * (plr->pop_total + plr->tech_count_transcendent);
            for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
                if (has_tech(tech_id, fc_id)) {
                    score += Tech[tech_id].AI_growth + Tech[tech_id].AI_tech
                        + Tech[tech_id].AI_wealth + Tech[tech_id].AI_power;
                }
            }
            for (int i = 0; i < MaxSecretProjectNum; i++) {
                if (SecretProjects[i] >= 0 && Bases[SecretProjects[i]].faction_id == fc_id) {
                    score += 10;
                }
            }
            score += *ObjectiveAchievePts * num_objectives(fc_id, 0);
            // Fix: original version considered plr->units_active[unit_id] when the count
            // is 250 or below and otherwise excludes unit_id from scoring but this does not
            // properly handle plr->units_active wrapping to zero when it reaches 256.
            std::map<int,int> units_active;
            for (int i = *VehCount - 1; i >= 0; --i) {
                if (Vehs[i].faction_id == fc_id) {
                    ++units_active[Vehs[i].unit_id];
                }
            }
            for (int unit_id = 0; unit_id < MaxProtoNum; unit_id++) {
                if (unit_id < MaxProtoFactionNum && !has_tech(Units[unit_id].preq_tech, fc_id)) {
                    continue;
                }
                int unit_value = units_active[unit_id] * Units[unit_id].cost;
                if (!unit_value) {
                    continue;
                }
                // Fix: original version used score += unit_value * (offense_value / max_weapon_value);
                // for non-PB, non-PSI combat units but this is likely a mistake since integer division
                // truncates the value towards zero when offense_value is below current max_weapon_value.
                int offense_value = Units[unit_id].offense_value();
                if (offense_value == 0) {
                    score += unit_value / 4;
                } else if (offense_value < 0) {
                    score += unit_value / 2;
                } else if (offense_value < 99) {
                    score += unit_value * offense_value / max_weapon_value;
                } else {
                    score += unit_value;
                }
            }
        }
        FactionRankingsUnk[fc_id] = score;
        if (*CurrentTurn < 1000) {
            FactionTurnMight[*CurrentTurn][fc_id] = (int16_t)score;
        }
    }
    if (!flag) {
        return;
    }
    for (int rnk = 7; rnk > 0; --rnk) {
        int best_score = -1;
        int best_id = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (FactionRankingsUnk[i] > best_score) {
                best_score = FactionRankingsUnk[i];
                best_id = i;
            }
        }
        FactionRankingsUnk[best_id] = -1;
        Factions[best_id].ranking = rnk;
    }
    memset(FactionRankings, 0, 32u);
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_alive(i)) {
            FactionRankings[Factions[i].ranking] = i;
        }
    }
    *RankingFactionIDUnk1 = 0;
    *RankingFactionIDUnk2 = 0;
    for (int rnk = 7; rnk >= 0; --rnk) {
        if (is_human(FactionRankings[rnk])) {
            *RankingFactionIDUnk1 = FactionRankings[rnk];
            break;
        }
    }
    for (int rnk = 0; rnk < MaxPlayerNum; ++rnk) {
        if (is_human(FactionRankings[rnk])) {
            *RankingFactionIDUnk2 = FactionRankings[rnk];
            break;
        }
    }
    int leader_faction = great_satan(FactionRankings[7], 0) ? FactionRankings[7] : 0;
    if (climactic_battle()) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (is_human(i) && (Factions[i].diff_level >= 4 || (*GameRules & RULES_INTENSE_RIVALRY))) {
                leader_faction = i;
            }
        }
    }
    if (!leader_faction) {
        *GameState &= ~STATE_UNK_200;
        return;
    }
    *GameState |= STATE_UNK_200;
    int vendetta_count = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (Factions[i].diplo_status[leader_faction] & DIPLO_VENDETTA) {
            vendetta_count++;
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_human(i)) {
            continue;
        }
        int status = Factions[i].diplo_status[leader_faction];
        if (!(status & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))) {
            continue;
        }
        if (status & (DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_PACT|DIPLO_UNK_4000000)) {
            continue;
        }
        int difficulty = (*GameRules & RULES_INTENSE_RIVALRY) ? 5 : Factions[leader_faction].diff_level;
        bool wants_revenge = (Factions[leader_faction].diplo_status[i] & DIPLO_WANT_REVENGE) != 0;
        int chance = difficulty * ((wants_revenge ? 4 : 0)
            + ((Factions[i].player_flags_ext & PFLAG_EXT_SHAMELESS_BETRAY_HUMANS) ? 4 : 0)
            + (climactic_battle() ? 4 : 0)
            + reputation(leader_faction, i));
        int roll_range = (vendetta_count - Factions[i].AI_fight + 2) << 8;
        int roll = roll_range > 1 ? game_rand() % roll_range : 0;
        if (roll < chance) {
            set_treaty(i, leader_faction, DIPLO_SHALL_BETRAY, 1);
        }
    }
}

void __cdecl compute_score(int faction_id, int* score_val, int* table_val, int flag) {
    Faction* const plr = &Factions[faction_id];

    if (*GameState & STATE_FINAL_SCORE_DONE && !flag) {
        return;
    }
    if (!score_val) {
        score_val = &plr->unk_109;
    }
    if (!table_val) {
        table_val = &plr->unk_110;
    }
    *score_val = 0;
    if (!flag) {
        plr->unk_107 = 0;
    }
    memset(table_val, 0, 0x24u);
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE* base = &Bases[base_id];
        if (base->faction_id == faction_id) {
            table_val[0] += base->pop_size;
            set_base(base_id);
            base_compute(0);
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (!MFactions[faction_id].is_alien() && !MFactions[i].is_alien()
                && !plr->sanction_turns && !Factions[i].sanction_turns) {
                    table_val[1] += BaseCommerceImport[i];
                }
            }
        } else if (plr->player_flags & (PFLAG_UNK_20000|PFLAG_UNK_40000)) {
            if (plr->diplo_status[base->faction_id] & DIPLO_PACT
            && *GameRules & RULES_VICTORY_COOPERATIVE) {
                table_val[0] += base->pop_size;
            } else {
                table_val[0] += base->pop_size / 2;
            }
        } else {
            int status = Factions[base->faction_id].diplo_status[faction_id];
            if (status & DIPLO_HAVE_SURRENDERED) {
                if (status & DIPLO_PACT) {
                    table_val[0] += base->pop_size;
                }
            } else if (status & DIPLO_PACT && *GameRules & RULES_VICTORY_COOPERATIVE) {
                table_val[0] += base->pop_size / 2;
            }
        }
    }
    for (int i = 0; i < MaxTechnologyNum; i++) {
        if (has_tech(i, faction_id)) {
            ++table_val[2];
        }
    }
    table_val[3] = 10 * plr->tech_count_transcendent;
    table_val[6] = max(1, *ObjectiveAchievePts)
        * num_objectives(faction_id, *GameRules & RULES_VICTORY_COOPERATIVE);
    for (int i = 0; i < MaxSecretProjectNum; i++) {
        if (SecretProjects[i] >= 0 && Bases[SecretProjects[i]].faction_id == faction_id) {
            table_val[4] += 25;
        }
    }
    *ScoreBonusPts = 0;
    memset(ScorePopTotal, 0, 0x20u);
    int asc_base_id = project_base(FAC_ASCENT_TO_TRANSCENDENCE);
    if (asc_base_id >= 0) {
        *ScoreBonusPts = max(300, 2 * (1000 - *CurrentTurn));
        int fc_id = -1;
        int num_plr = 0;
        int num_all = 0;
        if (Bases[asc_base_id].faction_id > 0 && Bases[asc_base_id].faction_id < 8) {
            fc_id = Bases[asc_base_id].faction_id;
            num_all = 2 * Factions[fc_id].pop_total;
            ScorePopTotal[fc_id] = num_all;
            if (fc_id == faction_id) {
                num_plr = num_all;
            }
        }
        // Fix: added separate checks for cases where fc_id is not defined
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (fc_id >= 0 && fc_id != i && Factions[i].diplo_status[fc_id] & DIPLO_PACT) {
                num_all += Factions[i].pop_total;
                ScorePopTotal[i] = Factions[i].pop_total;
                if (i == faction_id) {
                    num_plr += Factions[i].pop_total;
                }
            }
        }
        table_val[6] = (!num_all ? 0 : num_plr * *ScoreBonusPts / num_all);
        ScorePopTotal[0] = num_all;
    }
    table_val[7] = max(0, 2 * (500 - *CurrentTurn));
    if (!(*GameRules & RULES_VICTORY_CONQUEST)) {
        table_val[7] = 0;
    }
    if (*GameState & STATE_VICTORY_CONQUER && *GameRules & RULES_VICTORY_CONQUEST && table_val[7]) {
        int pop_plr = 0;
        int pop_all = 0;
        for (int i = 0; i < *BaseCount; i++) {
            pop_all += Bases[i].pop_size;
            int status = Factions[Bases[i].faction_id].diplo_status[faction_id];
            if (Bases[i].faction_id == faction_id
            || (status & DIPLO_PACT && status & DIPLO_HAVE_SURRENDERED)) {
                pop_plr += Bases[i].pop_size;
            }
        }
        if (pop_all) {
            table_val[7] = pop_plr * plr->unk_117 / pop_all;
        }
    } else if (plr->player_flags & (PFLAG_UNK_20000|PFLAG_UNK_40000)) {
        table_val[7] = max(200, 2 * (600 - *CurrentTurn));
    } else if (plr->player_flags & PFLAG_UNK_100000) {
        table_val[7] = max(300, 2 * (1000 - *CurrentTurn));
    } else {
        table_val[7] = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if ((Factions[i].player_flags & (PFLAG_UNK_20000|PFLAG_UNK_40000))
            && (plr->diplo_status[i] & DIPLO_PACT)
            && (*GameRules & RULES_VICTORY_COOPERATIVE)) {
                table_val[7] = max(100, 600 - *CurrentTurn);
            }
        }
    }
    if (!is_alive(faction_id)) {
        table_val[7] = 0;
        table_val[8] = 0;
    }
    if ((*ObjectiveReqVictory < 9000 || *ObjectivesSuddenDeathVictory < 9000)
    && !(*GameState & STATE_SCN_VICT_HIGHEST_AC_SCORE_WINS)) {
        plr->unk_110 = 0;
        plr->unk_111 = 0;
        plr->unk_112 = 0;
        plr->unk_113 = 0;
        plr->unk_114 = 0;
        plr->unk_115 = 0;
        // skipped unk_116
        plr->unk_117 = 0;
        plr->unk_118 = 0;
    }
    if (!plr->unk_117) {
        int num_obj = num_objectives(faction_id, *GameRules & RULES_VICTORY_COOPERATIVE);
        if (*ObjectiveReqVictory) {
            if (num_obj >= *ObjectiveReqVictory) {
                table_val[7] = *VictoryAchieveBonusPts;
            }
        } else {
            if (num_obj >= max(1, most_objectives(0, 0))) {
                table_val[7] = *VictoryAchieveBonusPts;
            }
        }
    }
    for (int i = 0; i < 9; i++) {
        *score_val += table_val[i];
    }
    if (*MapNativeLifeForms != 1 && !(*GameState & STATE_IS_SCENARIO)) {
        table_val[5] = *score_val * (*MapNativeLifeForms - 1) / 4;
        *score_val += table_val[5];
    }
    if (flag) {
        return;
    }
    if (*GameState & STATE_SCN_VICT_HIGHEST_AC_SCORE_WINS) {
        int score_alt = 0;
        int table_alt[9] = {};
        int score_max = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            compute_score(i, &score_alt, &table_alt[0], 1);
            if (score_alt > score_max) {
                score_max = score_alt;
            }
        }
        if (*score_val >= score_max) {
            if (!*VictoryAchieveBonusPts) {
                table_val[7] = 1000;
            } else {
                table_val[7] = *VictoryAchieveBonusPts;
            }
        }
    }
    plr->unk_107 = *score_val;
    if (*GameRules & RULES_IRONMAN && !(*GameState & STATE_SCENARIO_CHEATED_FLAG)) {
        plr->unk_107 = 2 * (*score_val);
    }
}

