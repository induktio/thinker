
#include "faction_diplo.h"


static int range(int input, int min_val, int max_val) {
    return min(max(input, min_val), max(min_val, max_val));
}

static int random_val(int min_val, int max_val) {
    return game_random(min_val, max_val);
}

int __cdecl auto_contact() {
    if (*MultiplayerActive && Rules->tgl_humans_always_contact_net) {
        return 1;
    }
    return *PbemActive && Rules->tgl_humans_always_contact_pbem;
}

void __cdecl diplomacy_caption(int faction_id, int faction_id_2) {
    const uint32_t pact_surrender = DIPLO_PACT|DIPLO_HAVE_SURRENDERED;
    char caption[StrBufLen];
    int mood;
    if ((Factions[faction_id].diplo_status[faction_id_2] & DIPLO_VENDETTA) && *DiploFriction < 15) {
        *DiploFriction = 15;
        mood = MOOD_OBSTINATE;
    } else {
        mood = get_mood(*DiploFriction);
    }
    if ((mood == MOOD_MAGNANIMOUS && *DiploFearScale
    && !(Factions[faction_id].diplo_status[faction_id_2] & DIPLO_PACT))
    || (Factions[faction_id_2].diplo_status[faction_id] & pact_surrender) == pact_surrender) {
        snprintf(caption, StrBufLen, "%s - %s", get_noun(faction_id_2), label_get(TL_Submissive));
    } else {
        snprintf(caption, StrBufLen, "%s - ", get_noun(faction_id_2));
        say_fac_special(caption, Mood[mood], faction_id_2);
    }
    parse_says(7, caption, -1, -1);
    if (Win_is_visible(DiploPop)) {
        DiploPop_draw_info(DiploPop);
        GraphicWin_soft_update_3(DiploPop, &DiploPop->rcFactionInfo);
    }
}

void __cdecl diplomacy_check(int faction_id, int faction_id_2, int full_check) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];

    *DiploFrictionFactionIDWith = faction_id;
    *DiploFrictionFactionID = faction_id_2;
    *DiploSecondFaction = faction_id_2;
    *DiploHostility = 0;
    *DiploAttackChance = 0;
    *DiploTributeDemand = 0;
    *DiploFearScale = 0;
    *DiploConquestFear = 0;
    *DiploExposedBaseCount = 0;
    *DiploOpenBaseID = 0; // default value excludes the first base
    *DiploCapturedBases = 0;
    *DiploNearEndgame = at_climax(faction_id);
    *DiploRivalOutranks = cmp.ranking > plr.ranking;
    *DiploGreatSatan = great_satan(faction_id, 0)
        && cmp.base_count > 1
        && (!(plr.diplo_status[faction_id_2] & DIPLO_PACT)
        || (cmp.ranking >= 5 && (!(*GameRules & RULES_VICTORY_COOPERATIVE) || *CurrentTurn < 200)));
    *DiploRunawayLeader = FactionRankingsUnk[FactionRankings[7]] > 2 * FactionRankingsUnk[faction_id]
        && cmp.ranking != 7;

    if (cmp.player_flags & PFLAG_TEAM_UP_VS_HUMAN) {
        *DiploGreatSatan = 1;
        *DiploRunawayLeader = 0;
    }
    if (cmp.player_flags & PFLAG_COOP_WITH_HUMAN) {
        *DiploGreatSatan = 0;
        *DiploRunawayLeader = 1;
    }
    // modified effects are only applied on lines referencing conf options
    *DiploFriction = cmp.diplo_friction[faction_id];
    *DiploFriction += conf.rival_friction_bias;
    *DiploReputation = reputation(faction_id, faction_id_2);

    int soc_conflict = 0;
    int soc_category = MFactions[faction_id_2].soc_priority_category;
    if (soc_category >= 0) {
        int soc_model = MFactions[faction_id_2].soc_priority_model;
        if (soc_model) {
            int current = ((CSocialCategory*)&plr.SE_Politics_pending)->models[soc_category];
            if (current && current != soc_model) {
                soc_conflict = 1;
                if (cmp.diplo_agenda[faction_id] & AGENDA_UNK_1) {
                    soc_conflict = 2;
                }
            }
        }
    }
    int our_combat = plr.diplo_combat_count[faction_id_2] + *CurrentTurn / 50 + 1;
    int their_combat = cmp.diplo_combat_count[faction_id] + *CurrentTurn / 50 + 1;
    if (!(plr.diplo_status[faction_id_2] & DIPLO_VENDETTA)) {
        our_combat += *CurrentTurn / 25 + plr.diplo_combat_total[faction_id_2] + 2;
        their_combat += *CurrentTurn / 25 + cmp.diplo_combat_total[faction_id] + 2;
    }
    int ai_fight = cmp.AI_fight;
    *DiploPlayerPactCount = 0;
    *DiploRivalPactCount = 0;
    *DiploRivalStrongPacts = 0;
    *DiploRivalPactScore = 0;
    if ((cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE) && ai_fight < 0) {
        ai_fight = -ai_fight;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_alive(i)) {
            if ((plr.diplo_status[i] & DIPLO_PACT) && !(Factions[i].diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED)) {
                ++*DiploPlayerPactCount;
            }
            if (cmp.diplo_status[i] & DIPLO_PACT) {
                if (Factions[i].ranking >= 5 && cmp.ranking < 5 && i != faction_id && !*DiploRunawayLeader) {
                    ++*DiploRivalStrongPacts;
                }
                ++*DiploRivalPactCount;
                *DiploRivalPactScore += FactionRankingsUnk[i];
            }
        }
    }
    if (full_check) {
        *DiploSpokeTurn = plr.diplo_spoke[faction_id_2];
    }
    int our_morale = MFactions[faction_id].rule_morale + clamp(plr.SE_morale_pending, -4, 4) + 16;
    int their_morale = MFactions[faction_id_2].rule_morale + clamp(cmp.SE_morale_pending, -4, 4) + 16;

    int max_region_bases[MaxPlayerNum] = {};
    for (int i = 1; i < MaxPlayerNum; i++) {
        max_region_bases[i] = 1;
    }
    memset(&DiploRegionFactions[1], 0, 127 * sizeof(int32_t));
    for (int region = 1; region < MaxRegionNum; region++) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            int bases = Factions[i].region_total_bases[region];
            if (bases > max_region_bases[i]) {
                max_region_bases[i] = bases;
            }
        }
    }
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE& base = Bases[base_id];
        if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
            DiploRegionFactions[region_at(base.x, base.y)] |= 1 << base.faction_id;
        }
        if (base.faction_id == faction_id) {
            if (base.faction_id_former == faction_id_2) {
                ++*DiploCapturedBases;
            }
        } else if (base.faction_id != faction_id_2) {
            continue;
        }
        if (!(plr.diplo_status[faction_id_2] & DIPLO_PACT)) {
            MAP* sq = mapsq(base.x, base.y);
            if (sq->veh_who() < 0) {
                for (int i = 0; i < 21; i++) {
                    int x = wrap(base.x + TableOffsetX[i]);
                    int y = base.y + TableOffsetY[i];
                    if (on_map(x, y)) {
                        MAP* adj = mapsq(x, y);
                        if (adj->region == sq->region) {
                            if (base.faction_id == faction_id) {
                                if (adj->veh_who() == faction_id_2) {
                                    ++*DiploExposedBaseCount;
                                    break;
                                }
                            } else if (adj->veh_who() == faction_id) {
                                *DiploOpenBaseID = base_id;
                                *DiploFearScale += 8 * base.pop_size;
                                break;
                            }
                        }
                    }
                }
            } else if (base.faction_id == faction_id) {
                int stack_total = stack_check(veh_at(base.x, base.y), 14, -1, -1, -1);
                int adjacent_total = 0;
                for (int i = 0; i < 21; i++) {
                    int x = wrap(base.x + TableOffsetX[i]);
                    int y = base.y + TableOffsetY[i];
                    if (on_map(x, y)) {
                        int veh_id = stack_fix(veh_at(x, y));
                        // Fix: added bounds checking
                        if (veh_id >= 0 && Vehs[veh_id].faction_id == faction_id_2) {
                            int combat_total = stack_check(veh_id, 2, PLAN_COMBAT, -1, -1);
                            int offense_total = stack_check(veh_id, 2, PLAN_OFFENSE, -1, -1);
                            adjacent_total += combat_total + offense_total;
                        }
                    }
                }
                if (adjacent_total > stack_total) {
                    ++*DiploAttackChance;
                    ++*DiploTributeDemand;
                }
            }
        }
    }
    for (int region = 1; region < MaxRegionNum; region++) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (Factions[i].region_total_bases[region] >= max_region_bases[i] - max_region_bases[i] / 4) {
                DiploRegionFactions[region] |= 1 << i;
            }
        }
        int our_bases = plr.region_total_bases[region];
        int their_bases = cmp.region_total_bases[region];
        int our_units = plr.region_total_combat_units[region];
        int their_units = cmp.region_total_combat_units[region];
        if ((our_units || our_bases) && (their_units || their_bases)) {
            int our_weapon = plr.best_weapon_value + 1;
            int their_weapon = cmp.best_weapon_value + 1;
            int our_force = our_weapon * (our_morale * plr.region_force_rating[region] / their_morale) / their_weapon;
            int their_force = their_weapon * (their_morale * cmp.region_force_rating[region] / our_morale) / our_weapon;
            int reg_factions = DiploRegionFactions[region];
            int reg_has_other = reg_factions & (1 << faction_id_2);
            int their_total = their_units;
            if (reg_has_other && our_bases) {
                their_total += their_force / (((reg_factions & (1 << faction_id)) == 0) - cmp.AI_fight + 2);
            }
            if (reg_has_other && our_force > their_total) {
                *DiploFearScale += 32 * (our_force + 1) / clamp(their_total, 1, 99999);
            } else {
                *DiploHostility = 1;
                if (our_bases) {
                    *DiploTributeDemand += their_force;
                    if ((reg_factions & (1 << faction_id)) && their_force > our_units) {
                        *DiploTributeDemand += their_force;
                        if (their_force > 3 * (our_units + our_force) / 2) {
                            ++*DiploAttackChance;
                        }
                    }
                    if (their_bases || (plr.diplo_status[faction_id_2] & DIPLO_PACT)) {
                        *DiploTributeDemand -= our_force;
                    }
                } else if (their_bases) {
                    *DiploTributeDemand -= (our_force - their_units) / (2 - (reg_has_other != 0));
                }
            }
        }
    }
    *DiploTributeDemand *= *DiploExposedBaseCount + 1;
    if (*DiploExposedBaseCount && !(plr.diplo_status[faction_id_2] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))) {
        *DiploHostility = 1;
    }
    if (*DiploNearEndgame && *DiploRivalOutranks) {
        *DiploHostility = 3 - (*DiploGreatSatan != 0);
    }
    uint32_t& their_status = cmp.diplo_status[faction_id];
    uint32_t& their_agenda = cmp.diplo_agenda[faction_id];
    if (plr.base_count && cmp.base_count) {
        if ((their_status & DIPLO_WANT_REVENGE) || (cmp.diplo_agenda[faction_id] & AGENDA_UNK_200)) {
            int bonus = clamp(ai_fight + plr.tech_commerce_bonus - cmp.tech_commerce_bonus, 0, 99);
            *DiploTributeDemand = 2 * (*DiploTributeDemand + bonus * cmp.energy_surplus_total / cmp.base_count);
        } else {
            int bonus = clamp(cmp.tech_commerce_bonus + cmp.AI_wealth - plr.tech_commerce_bonus - ai_fight, 0, 99);
            *DiploTributeDemand -= bonus * (plr.energy_surplus_total / plr.base_count);
            if (their_status & DIPLO_WANT_TO_TALK) {
                *DiploTributeDemand /= 2;
            }
        }
    }
    if (plr.best_weapon_value >= 3 * cmp.best_weapon_value / 2 || plr.best_weapon_value > 2 * cmp.best_armor_value) {
        *DiploFearScale *= 2;
    }
    if (our_combat >= 2 * their_combat) {
        *DiploFearScale *= 2;
    }
    if (our_combat >= 4 * their_combat && !(their_status & DIPLO_WANT_REVENGE) && *DiploFearScale <= 8) {
        *DiploFearScale = 8;
    }
    if (2 * our_combat < their_combat) {
        *DiploFearScale /= 2;
    }
    if (4 * our_combat < their_combat) {
        *DiploFearScale /= 2;
    }
    if (*DiploFearScale
        || (plr.diff_level <= ((their_status & DIPLO_WANT_REVENGE) ? 2 : 4) && !(*GameRules & RULES_INTENSE_RIVALRY))) {
        *DiploAttackChance = 0;
    }
    *DiploTributeDemand = *DiploTributeDemand * (*CurrentTurn / 10 + their_combat) / (*CurrentTurn / 10 + our_combat);

    int turn_threshold = *MultiplayerActive ? 0 : 25 * (5 - plr.diff_level);
    if (*CurrentTurn >= turn_threshold + 50 && !*DiploRunawayLeader && plr.ranking >= (*DiffLevel < 3) + 5) {
        if (cmp.ranking >= (plr.ranking < 6) + 5) {
            *DiploGreatSatan = 1;
        } else if (*DiploRivalStrongPacts && (plr.diplo_status[faction_id_2] & DIPLO_UNK_8000000) && plr.ranking >= 6) {
            *DiploGreatSatan = 1;
        }
    }
    if (*DiploGreatSatan
    || !((!(their_status & DIPLO_WANT_REVENGE) && !(cmp.diplo_agenda[faction_id] & AGENDA_UNK_200))
    || *DiploOpenBaseID
    || (*DiploFearScale && 2 * cmp.mil_strength_1 < plr.mil_strength_1))) {
        *DiploFearScale = 0;
        *DiploHostility = (*DiploGreatSatan != 0) + 1;
        if (*DiploTributeDemand < 2 * cmp.ranking) {
            *DiploTributeDemand = 2 * cmp.ranking;
        }
    }
    if (cmp.planet_busters && cmp.major_atrocities) {
        *DiploFearScale = 0;
    }
    if (plr.planet_busters && plr.major_atrocities) {
        if (!cmp.planet_busters) {
            *DiploFearScale = 32;
        }
        *DiploTributeDemand = *DiploTributeDemand * cmp.planet_busters / plr.planet_busters;
    }
    *DiploFearScale -= *DiploFearScale * conf.rival_defiance_bias / 20;
    *DiploConquestFear = *DiploFearScale;
    if ((their_status & DIPLO_PACT) || !*DiploCapturedBases) {
        *DiploConquestFear = 0;
    }
    int diff_level = (*GameRules & RULES_INTENSE_RIVALRY) ? 5 : plr.diff_level;
    int blemishes = clamp(plr.integrity_blemishes / 2, 1, 99);
    *DiploFearScale /= (blemishes + 1) * (diff_level + 3) / 2;
    *DiploEnemyAllyCount = 0;
    *DiploEnemyAlly = 0;
    if (!(their_status & DIPLO_PACT) || !(their_status & DIPLO_HAVE_SURRENDERED)) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (is_alive(i)
            && i != faction_id
            && i != faction_id_2
            && (cmp.diplo_status[i] & DIPLO_VENDETTA)
            && (!(Factions[i].diplo_status[faction_id] & DIPLO_PACT)
            || !(Factions[i].diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED))
            && ((plr.diplo_status[i] & DIPLO_PACT)
            || ((plr.diplo_status[i] & DIPLO_TREATY) && ai_fight > 0))) {
                *DiploEnemyAlly = i;
                ++*DiploEnemyAllyCount;
            }
        }
    }
    *DiploFriction += 3 * *DiploEnemyAllyCount;
    if (their_status & DIPLO_PACT) {
        *DiploFriction -= 5;
    }
    if (their_status & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA)) {
        *DiploFriction += 5;
    }
    if (their_agenda & AGENDA_UNK_400) {
        *DiploFriction += 3;
    }
    if (their_agenda & AGENDA_UNK_800) {
        *DiploFriction += 3;
    }
    if (*DiploReputation > 1) {
        *DiploFriction += *DiploReputation - 1;
    }
    if (plr.tech_commerce_bonus < cmp.tech_commerce_bonus) {
        *DiploFriction += 2 * (plr.tech_commerce_bonus - cmp.tech_commerce_bonus);
    } else if (plr.tech_commerce_bonus > cmp.tech_commerce_bonus) {
        *DiploFriction += plr.tech_commerce_bonus - cmp.tech_commerce_bonus;
    }
    if (soc_conflict) {
        *DiploFriction += 5;
        if (*DiploFriction < 5) {
            *DiploFriction = 5;
        }
    }
    if (soc_conflict > 1 && !*DiploFearScale) {
        *DiploFriction += 5;
    }
    if (their_agenda & AGENDA_UNK_200) {
        *DiploHostility = 1;
    }
    if (*DiploFriction <= 4) {
        *DiploHostility = 0;
    }
    if (*DiploFriction >= 15) {
        *DiploHostility = 4;
    }
    if (their_status & DIPLO_UNK_40) {
        *DiploHostility = 1;
    }
    if (!(*GameRules & RULES_VICTORY_TRANSCENDENCE) && !cmp.SE_alloc_labs) {
        *DiploHostility = 1;
    }
    if (*DiploFearScale || plr.mil_strength_1 > 3 * cmp.mil_strength_1) {
        *DiploHostility = 0;
    }
    if (plr.best_weapon_value >= 2 * cmp.best_weapon_value) {
        *DiploHostility = 0;
    }
    int value = (*DiploTributeDemand * (soc_conflict + 1)
        * (*DiploFriction + 10) * (*DiploReputation + 2) * (diff_level + 1)) >> 6;
    if (their_status & DIPLO_UNK_40) {
        value *= 2;
    }
    int energy_credits = plr.energy_credits;
    if (energy_credits >= 2 * cmp.energy_credits) {
        if (energy_credits >= 2000) {
            value = 3 * value / 2;
        }
        if (energy_credits >= 4000) {
            value = 3 * value / 2;
        }
    }
    if (*DiploGreatSatan
    || (((their_status & DIPLO_WANT_REVENGE) || (their_agenda & AGENDA_UNK_200))
    && !*DiploOpenBaseID
    && (!*DiploFearScale || 2 * cmp.mil_strength_1 >= plr.mil_strength_1))) {
        if (plr.ranking >= 6) {
            int min_value = energy_credits * (plr.ranking + cmp.ranking) / 21;
            if (value < min_value) {
                value = min_value;
            }
        }
    }
    *DiploTributeDemand = 25 * ((value + 24) / 25);
    if (*DiploTributeDemand == 25) {
        *DiploTributeDemand = 50;
    }
    if (*DiploTributeDemand > energy_credits
    && *DiploTributeDemand < 2 * energy_credits
    && energy_credits >= ((their_status & DIPLO_UNK_40) ? 100 : 50)
    && !(plr.diplo_status[faction_id_2] & (DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE))) {
        *DiploTributeDemand = 25 * ((energy_credits + 24) / 25);
    }
    if (!(their_status & (DIPLO_UNK_40|DIPLO_WANT_REVENGE))
    && (!*DiploGreatSatan || (their_status & DIPLO_UNK_4000000))) {
        energy_compute(faction_id, 0);
        if (*DiploTributeDemand > 5 * *FactionCurrentNetIncome) {
            if (cmp.energy_credits >= cmp.ranking * (2 * plr.energy_credits / 3) / plr.ranking) {
                *DiploTributeDemand = 25 * (5 * *FactionCurrentNetIncome / 25);
                if (*DiploTributeDemand < 50) {
                    *DiploTributeDemand = 50;
                }
            }
        }
    }
    if ((((their_status & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) && !(their_status & DIPLO_UNK_10000))
    || *DiploFearScale)
    && !*DiploGreatSatan
    && !(their_status & DIPLO_WANT_REVENGE)) {
        *DiploAttackChance = 0;
    }
    if (!*DiploTributeDemand && !*DiploAttackChance && !(cmp.diplo_agenda[faction_id] & AGENDA_UNK_200)) {
        *DiploHostility = 0;
    }
    *DiploPrevHostility = *DiploHostility;
    *DiploGoodwill = 0;
    if (full_check) {
        *DiploHelpFaction = 0;
        *DiploUnmetEnemy = 0;
        *DiploPlayerEnemy = 0;
        *DiploUnmetFaction = 0;
        *DiploMutualFaction = 0;
    }
    *DiploRivalWarCount = 0;

    for (int i = 1; i < MaxPlayerNum; i++) {
        if (!is_alive(i) || i == faction_id || i == faction_id_2) {
            continue;
        }
        if (!(cmp.diplo_status[i] & DIPLO_COMMLINK)) {
            continue;
        }
        if (full_check
        && (plr.diplo_status[i] & DIPLO_VENDETTA)
        && !(cmp.diplo_status[i] & DIPLO_VENDETTA)
        && !(plr.diplo_status[i] & DIPLO_UNK_10000000)) {
            *DiploPlayerEnemy = i;
        }
        if (cmp.diplo_status[i] & DIPLO_PACT) {
            if (Factions[i].mil_strength_1 > plr.mil_strength_1) {
                --*DiploGoodwill;
            }
            if ((great_satan(faction_id, 0) || *DiploGreatSatan)
            && (Factions[i].diplo_status[faction_id] & DIPLO_VENDETTA)) {
                int reduced = *DiploFearScale - 3;
                if (reduced > 9999) {
                    *DiploFearScale = 9999;
                } else if (reduced <= 0) {
                    *DiploFearScale = 0;
                    *DiploConquestFear = 0;
                    *DiploPrevHostility = 2;
                    *DiploHostility = 2;
                } else {
                    *DiploFearScale = reduced;
                }
            }
        } else if (cmp.diplo_status[i] & DIPLO_VENDETTA) {
            int cmp_mil = cmp.mil_strength_1;
            int alt_mil = Factions[i].mil_strength_1;
            ++*DiploRivalWarCount;
            *DiploGoodwill += 2 * (alt_mil + cmp_mil / 2) / (cmp_mil + 1);
            if (DiploStateA[faction_id_2] & DST_UNK_40) {
                if ((plr.diplo_status[i] & (DIPLO_COMMLINK|DIPLO_VENDETTA)) == DIPLO_COMMLINK && alt_mil > cmp_mil) {
                    *DiploHelpFaction = i;
                }
                continue;
            }
            if (!(plr.diplo_status[i] & DIPLO_VENDETTA)
            && ((plr.diplo_status[i] & DIPLO_COMMLINK) || (plr.diplo_status[faction_id_2] & DIPLO_PACT))
            && (!(Factions[i].diplo_status[faction_id] & DIPLO_PACT)
            || !(Factions[i].diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED))) {
                *DiploHelpFaction = i;
                continue;
            }
            if (*DiploHelpFaction || !full_check || (plr.diplo_status[i] & DIPLO_COMMLINK) || is_human(i)) {
                continue;
            }
            if (alt_mil > cmp_mil || !mod_wants_to_attack(faction_id_2, i, 0) || *DiploFearScale) {
                *DiploUnmetEnemy = i;
            }
        } else if (full_check && !is_human(i)) {
            if (plr.diplo_status[i] & DIPLO_COMMLINK) {
                if (!(plr.diplo_status[i] & DIPLO_UNK_10000000)) {
                    *DiploMutualFaction = i;
                }
            } else if ((!*DiploUnmetFaction || contiguous(faction_id, i, 0, 0))
            && ((cmp.diplo_status[i] & DIPLO_TREATY) || !*DiploUnmetFaction)) {
                *DiploUnmetFaction = i;
            }
        }
    }
    if (their_agenda & AGENDA_UNK_800) {
        --*DiploGoodwill;
    }
    if (their_agenda & AGENDA_UNK_400) {
        --*DiploGoodwill;
    }
    if (their_status & DIPLO_WANT_REVENGE) {
        --*DiploGoodwill;
    }
    if (their_agenda & AGENDA_UNK_200) {
        --*DiploGoodwill;
    }
    *DiploGoodwill += -*DiploExposedBaseCount - (*DiploReputation - 1) / 3 - ai_fight;
    if (cmp.tech_commerce_bonus > plr.tech_commerce_bonus && ai_fight <= 0) {
        ++*DiploGoodwill;
    }
    if (!(*GameRules & RULES_VICTORY_TRANSCENDENCE) && !cmp.SE_alloc_labs) {
        --*DiploGoodwill;
    }
    if (cmp.mil_strength_1 > plr.mil_strength_1) {
        --*DiploGoodwill;
    }
    if (cmp.best_weapon_value > 2 * plr.best_armor_value) {
        --*DiploGoodwill;
    }
    if (*DiploGoodwill > ((plr.diplo_status[faction_id_2] & DIPLO_VENDETTA) != 0)
    && !*DiploGreatSatan
    && !(their_status & (DIPLO_UNK_40|DIPLO_PACT))
    && !*DiploAttackChance) {
        *DiploHostility = 0;
        *DiploTributeDemand = 0;
    }
    if ((their_status & DIPLO_PACT) && (their_status & DIPLO_HAVE_SURRENDERED)) {
        *DiploAttackChance = 0;
        *DiploExposedBaseCount = 0;
        *DiploPrevHostility = 0;
        *DiploHostility = 0;
    } else if ((*DiploExposedBaseCount || *DiploAttackChance) && (their_status & DIPLO_WANT_REVENGE)) {
        *DiploPrevHostility = 1;
        *DiploHostility = 1;
    }
    if (great_satan(faction_id, 1)
    && *DiploRivalPactCount < 2
    && FactionRankingsUnk[faction_id] >= *DiploRivalPactScore + 3 * FactionRankingsUnk[faction_id_2]
    && cmp.ranking < 6
    && cmp.mil_strength_1 < plr.mil_strength_1) {
        *DiploCanSurrender = 1;
        if ((cmp.diplo_status[faction_id] & DIPLO_VENDETTA)
        && !(cmp.diplo_agenda[faction_id] & AGENDA_FIGHT_TO_DEATH)
        && !(cmp.diplo_status[faction_id] & DIPLO_ATROCITY_VICTIM)) {
            *DiploHostility = 0;
        }
    } else {
        *DiploCanSurrender = 0;
    }
    if (full_check) {
        scan_prototypes(faction_id, faction_id_2);
        bool betray = (cmp.player_flags_ext & PFLAG_EXT_SHAMELESS_BETRAY_HUMANS) != 0;
        *DiploCanBetray = (plr.diff_level > 1 || betray)
            && !*DiploExposedBaseCount
            && !(their_status & DIPLO_PACT)
            && !(cmp.diplo_agenda[faction_id] & AGENDA_FIGHT_TO_DEATH)
            && (!(their_status & DIPLO_VENDETTA) || *DiploCapturedBases > 1 || betray)
            && (*DiploNearEndgame
                || *DiploHostility
                || soc_conflict
                || (betray && !*DiploFearScale)
                || (*DiploEnemyAllyCount && (their_status & DIPLO_UNK_800000) && *DiploFriction >= 12))
            && ((their_status & DIPLO_WANT_REVENGE)
                || (betray && *DiploHostility)
                || *DiploReputation > 5
                || ((*DiploHostility || soc_conflict > 1) && (plr.mil_strength_1 > cmp.mil_strength_1 || betray)));
    }
    if ((cmp.player_flags & PFLAG_COOP_WITH_HUMAN) && !(their_status & DIPLO_WANT_REVENGE)) {
        *DiploHostility = 0;
    }
    if (cmp.diplo_agenda[faction_id] & AGENDA_FIGHT_TO_DEATH) {
        *DiploHostility = 1;
    } else if (*DiploCanBetray) {
        *DiploHostility = 0;
        if (*DiploFriction > 6) {
            *DiploFriction = 6;
        }
    }
    uint32_t& our_status = plr.diplo_status[faction_id_2];
    if ((our_status & DIPLO_PACT)
    && (!(our_status & DIPLO_WANT_REVENGE) || *DiploRivalWarCount > 1)
    && (!(our_status & DIPLO_UNK_200) || *DiploRivalWarCount)
    && cmp.ranking < plr.ranking
    && ((*GameRules & RULES_VICTORY_COOPERATIVE) || *DiploRivalWarCount)) {
        *DiploHostility = 0;
    }
    if (cmp.diplo_agenda[faction_id] & AGENDA_PERMANENT) {
        *DiploCanBetray = 0;
        *DiploHostility = (their_status & DIPLO_VENDETTA) != 0;
    }
    if (!*DiploPrevHostility && (*DiploFearScale || (our_status & DIPLO_PACT)) && *DiploFriction > 14) {
        *DiploFriction = 14;
    }
    if ((*DiploHostility || (our_status & DIPLO_VENDETTA) || (their_status & DIPLO_WANT_REVENGE))
    && *DiploFriction < 6) {
        *DiploFriction = 6;
    }
}

void __cdecl diplomacy_ends(int faction_id, int faction_id_2) {
    DiploWinState[0] = 0;
    DiploWinState[1] = 0;
    *DiploCaptionOff = 0;
    *DiploCaptionMode = 0;
    DiploPop_stop(DiploPop);
    synch_diplo(faction_id, faction_id_2);
    NetDaemon_close_channel(NetState);
    *EncounterLocY = -1;
    *EncounterLocX = -1;
}

int __cdecl wants_to_speak(int faction_id, int faction_id_2) {
    *DiploSecondFaction = faction_id_2;
    if (MFactions[faction_id].is_alien() != MFactions[faction_id_2].is_alien()) {
        if (!MFactions[faction_id].is_alien()) {
            if (!has_tech(TECH_PrPsych, faction_id) && !has_tech(TECH_Psych, faction_id_2)) {
                *DiploCaptionMode = 0;
                return 0;
            }
        } else if (!has_tech(TECH_PrPsych, faction_id_2) && !has_tech(TECH_Psych, faction_id)) {
            *DiploCaptionMode = 0;
            return 0;
        }
    }
    if (*DiploCaptionOff) {
        return *DiploCaptionMode;
    }
    if (*DiploCaptionMode) {
        *DiploCaptionOff = 1;
        if (*DiploCaptionMode >= 8) {
            diplomacy_check(faction_id, faction_id_2, 0);
            diplomacy_caption(faction_id, faction_id_2);
            parse_says(0, get_title(faction_id_2), -1, -1);
            parse_says(1, get_name(faction_id_2), -1, -1);
            parse_says(2, get_noun(faction_id_2), -1, -1);
            parse_num(0, random_val(0, 10) + 1);
            parse_says(3, get_him_her(faction_id_2, 0), -1, -1);
            X_pops("URGENTCOMM", FactionPortraits[faction_id_2], pop_wait);
        }
    } else {
        InfoWin_set_state(InfoWin, 2, 1);
        Popup cur_popup;
        Popup_ctor(&cur_popup);
        auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
        while (!*DiploCaptionOff) {
            bool urgent_comm = Factions[faction_id_2].planet_busters
                && !(Factions[faction_id_2].diplo_status[faction_id] & DIPLO_UNK_4000);
            diplomacy_check(faction_id, faction_id_2, 0);
            if (on_map(*EncounterLocX, *EncounterLocY)
            && !Console_focus(MapWin, *EncounterLocX, *EncounterLocY, faction_id)) {
                draw_tiles(*EncounterLocX, *EncounterLocY, 2);
            }
            diplomacy_caption(faction_id, faction_id_2);
            parse_says(0, get_title(faction_id_2), -1, -1);
            parse_says(1, get_name(faction_id_2), -1, -1);
            parse_says(2, get_noun(faction_id_2), -1, -1);
            parse_num(0, random_val(0, 10) + 1);
            parse_says(3, get_him_her(faction_id_2, 0), -1, -1);
            if (urgent_comm) {
                X_pops("URGENTCOMM", FactionPortraits[faction_id_2], pop_wait);
                // Fix: URGENTCOMM "Switching to main viewscreen" message not opening the diplomacy
                // window as in DiploCaptionMode >= 8 branch above. Without these flags the loop
                // exited with both unset, the function returned 0 and the contact ended here.
                *DiploCaptionOff = 1;
                *DiploCaptionMode = 1;
                break;
            }
            Popup_start(&cur_popup, PopupScriptFile, "COMM", -1, 0, 0, 0);
            Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], 0, 0);
            int choice = BasePop_exec_3(&cur_popup, 0, pop_wait);
            if (choice <= 1) {
                *DiploCaptionOff = 1;
                *DiploCaptionMode = (choice == 0);
                break;
            }
            report_intel(faction_id_2);
        }
    }
    if (*DiploCaptionMode) {
        DiploPop_start(DiploPop, faction_id_2);
        introduce(faction_id, faction_id_2);
    }
    return *DiploCaptionMode;
}

int __cdecl diplomacy_menu(int faction_id, int faction_id_2) {
    auto read_line = []() {
        text_get();
        StrBuffer[0] = 0;
        parse_string(text_buf_ptr(), StrBuffer);
    };
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    *DiploSecondFaction = faction_id_2;
    parse_says(0, get_name(faction_id), -1, -1);
    parse_says(1, get_pact_hood(faction_id, faction_id_2), -1, -1);
    diplomacy_caption(faction_id, faction_id_2);
    Popup_start_2(&cur_popup, PopupScriptFile, "DIPLO", -1, 0, POPUP_EXTERNAL);
    Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], 0, 0);
    if (X_text_open(ScriptFile, "DIPLOMENU")) {
        return 0;
    }
    read_line();
    Dialogs_item(&cur_popup.dialogs, StrBuffer, 0);
    read_line();
    if (*DiploMenuFlagA) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 1);
    }
    read_line();
    if (!*DiploMenuFlagB) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 2);
    }
    read_line();
    if (*DiploMenuFlagB) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 2);
    }
    read_line();
    if (!*MultiplayerActive && *DiploCommonEnemy > 0) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 5);
    }
    read_line();
    if (!*MultiplayerActive && *DiploPeaceFaction > 0) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 6);
    }
    read_line();
    if (can_call_council(faction_id, 0)
    && !MFactions[faction_id].is_alien()
    && !MFactions[faction_id_2].is_alien()) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 7);
    }
    read_line();
    if (Factions[faction_id_2].diplo_status[faction_id] & DIPLO_PACT) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 3);
    }
    read_line();
    if (Factions[faction_id].diplo_status[faction_id_2] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 4);
    }
    text_close();
    return BasePop_exec_3(&cur_popup, 0, pop_wait);
}

/*
Rewritten ListBox select callback for PROPOSEBASE and COUNTERBASE.
Previously proposal_menu used battle_routine callback but it draws attack lines
from variables which are only set by battle_plans and are not updated correctly.
*/
static void __cdecl base_swap_routine(int base_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return;
    }
    BASE& src = Bases[base_id];
    int dst_id = find_hq(src.faction_id);
    if (dst_id < 0 || !(Bases[dst_id].visibility & (1 << MapWin->cOwner))) {
        dst_id = base_id;
    }
    BASE& dst = Bases[dst_id];
    PlanWin_clear_lines(PlanWin);
    PlanWin_opticenter(PlanWin, src.x, src.y, dst.x, dst.y, 0);
    PlanWin_add_line_2(PlanWin, base_id, dst_id, 0);
    PlanWin_on_redraw((PlanWindow*)&PlanWin->graphicWin);
}

int __cdecl proposal_menu(int faction_id, int faction_id_2) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    auto read_line = []() {
        text_get();
        StrBuffer[0] = 0;
        parse_string(text_buf_ptr(), StrBuffer);
    };
    auto start_popup = [&](const char* label, uint32_t flags) {
        Popup_start(&cur_popup, PopupScriptFile, label, -1, 0, flags, 0);
        Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], 0, 0);
    };
    *DiploBidBaseSwapID = -1;
    *DiploAskBaseSwapID = -1;
    *DiploSecondFaction = faction_id_2;
    *DiploCounterProposal = 0;
    *DiploCurrentProposal = 0;
    *DiploProposalFaction = 0;

    int base_swap_count = 0;
    for (int i = 0; i < *BaseCount; i++) {
        // Fix: remove unreachable alternative faction_id condition
        if (Bases[i].faction_id == faction_id_2
        && (Bases[i].visibility & (1 << faction_id))
        && !has_fac_built(FAC_HEADQUARTERS, i)) {
            base_swap_count++;
        }
    }

    int commlink_count = 0;
    int no_commlink_count = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_alive(i) && i != faction_id && i != faction_id_2) {
            if (plr.diplo_status[i] & DIPLO_COMMLINK) {
                commlink_count++;
            } else if (!has_project(FAC_EMPATH_GUILD, faction_id)) {
                no_commlink_count++;
            }
        }
    }

    parse_says(0, get_title(faction_id_2), -1, -1);
    parse_says(1, get_pact_hood(faction_id, faction_id_2), -1, -1);
    parse_num(0, plr.loan_balance[faction_id_2]);
    diplomacy_caption(faction_id, faction_id_2);
    start_popup("PROPOSAL", POPUP_EXTERNAL);
    if (X_text_open(ScriptFile, "PROPOSALMENU")) {
        return 0;
    }

    read_line();
    Dialogs_item(&cur_popup.dialogs, StrBuffer, 0);
    read_line();
    Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalMakeGift);
    read_line();
    if ((plr.diplo_status[faction_id_2] & (DIPLO_TREATY|DIPLO_PACT)) == DIPLO_TREATY) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalMakePact);
    }
    read_line();
    if (!(plr.diplo_status[faction_id_2] & DIPLO_TREATY)) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalMakeTreaty);
    }
    read_line();
    if (!(*GameRules & RULES_SCN_NO_TECH_TRADING)) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalTechTrade);
    }
    read_line();
    read_line();
    if (no_commlink_count) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalTradeCommlink);
    }
    read_line();
    Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalNeedEnergy);
    read_line();
    int loan = plr.loan_balance[faction_id_2];
    if (loan && plr.energy_credits >= loan) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalRepayLoan);
    }
    read_line();
    read_line();
    if (commlink_count) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalJointAttack);
    }
    read_line();
    if (cmp.base_count > 1 && base_swap_count && !*MultiplayerActive) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploProposalBaseSwap);
    }
    read_line();

    int proposal = BasePop_exec_3(&cur_popup, 0, pop_wait);
    *DiploCurrentProposal = proposal;
    auto cancel_proposal = [&]() {
        proposal = 0;
        *DiploCurrentProposal = 0;
    };

    if (proposal == DiploProposalJointAttack) {
        start_popup("PROPOSEATTACK", POPUP_EXTERNAL|POPUP_BTN_CANCEL);
        if (X_text_open(ScriptFile, "ATTACKSTRING")) {
            cancel_proposal();
        } else {
            text_get();
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (is_alive(i) && i != faction_id && i != faction_id_2
                && ((plr.diplo_status[i] & DIPLO_COMMLINK) || is_human(i))) {
                    parse_says(0, get_title(i), -1, -1);
                    parse_says(1, get_name(i), -1, -1);
                    parse_says(2, get_noun(i), -1, -1);
                    StrBuffer[0] = 0;
                    parse_string(text_buf_ptr(), StrBuffer);
                    Dialogs_item(&cur_popup.dialogs, StrBuffer, i);
                }
            }
            int target_id = BasePop_exec_3(&cur_popup, 0, pop_wait);
            *DiploProposalFaction = target_id;
            if (target_id < 0) {
                cancel_proposal();
            // Fix: original checks diplo_status[8]
            } else if (cmp.diplo_status[target_id] & DIPLO_VENDETTA) {
                parse_says(0, get_title(target_id), -1, -1);
                parse_says(1, get_name(target_id), -1, -1);
                X_dialog("REJWARALREADY", faction_id_2);
                cancel_proposal();
            } else {
                proposal = *DiploCurrentProposal;
            }
        }
    }

    if (proposal == DiploProposalTradeCommlink) {
        start_popup("PROPOSECOMMLINK", POPUP_EXTERNAL|POPUP_BTN_CANCEL);
        if (X_text_open(ScriptFile, "ATTACKSTRING")) {
            cancel_proposal();
        } else {
            text_get();
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (is_alive(i) && i != faction_id && i != faction_id_2
                && !(plr.diplo_status[i] & DIPLO_COMMLINK)) {
                    parse_says(0, get_title(i), -1, -1);
                    parse_says(1, get_name(i), -1, -1);
                    parse_says(2, get_noun(i), -1, -1);
                    StrBuffer[0] = 0;
                    parse_string(text_buf_ptr(), StrBuffer);
                    Dialogs_item(&cur_popup.dialogs, StrBuffer, i);
                }
            }
            int target_id = BasePop_exec_3(&cur_popup, 0, pop_wait);
            *DiploProposalFaction = target_id;
            if (target_id < 0) {
                cancel_proposal();
            } else {
                if (!(cmp.diplo_status[target_id] & DIPLO_COMMLINK)
                && !has_project(FAC_EMPATH_GUILD, faction_id_2)) {
                    parse_says(0, get_title(target_id), -1, -1);
                    parse_says(1, get_name(target_id), -1, -1);
                    parse_says(2, get_noun(target_id), -1, -1);
                    *DiploCurrentProposal = 0;
                    X_pops("NOCOMMLINK", FactionPortraits[faction_id_2], pop_wait);
                }
                proposal = *DiploCurrentProposal;
            }
        }
    }

    if (proposal == DiploProposalBaseSwap) {
        PlanWin_init(PlanWin, 0);
        PlanWin_back_redraw(PlanWin);
        bool listed = false;
        start_popup("PROPOSEBASE", POPUP_EXTERNAL|POPUP_BTN_CANCEL);
        for (int i = 0; i < *BaseCount; i++) {
            // Fix: remove unreachable alternative faction_id condition
            if (Bases[i].faction_id == faction_id_2
            && (Bases[i].visibility & (1 << faction_id))
            && !has_fac_built(FAC_HEADQUARTERS, i)) {
                StrBuffer[0] = 0;
                say_base(StrBuffer, i);
                Dialogs_item(&cur_popup.dialogs, StrBuffer, i);
                if (!listed) {
                    listed = true;
                    base_swap_routine(i);
                }
            }
        }
        if (listed) {
            Win_show((Win*)((char*)PlanWin + ((DWORD*)PlanWin->vtable)[1]), 0);
            cur_popup.dialogs.listBox.cb_select_item = base_swap_routine;
            *DiploAskBaseSwapID = BasePop_exec_3(&cur_popup, 0, pop_wait);
        } else {
            *DiploAskBaseSwapID = -1;
        }
        if (*DiploAskBaseSwapID < 0) {
            *DiploCurrentProposal = 0;
        }
        PlanWin_close(PlanWin);
        proposal = *DiploCurrentProposal;
    }

    if (proposal == 0
    || proposal == DiploProposalTradeMaps
    || proposal == DiploProposalCloseDialog
    || proposal == DiploProposalRepayLoan
    || proposal == DiploProposalBuyTech
    || proposal == DiploProposalCouncilVote) {
        *DiploCounterProposal = DiploCounterFriendship;
        debug("proposal_menu %d %d current: %d counter: %d\n",
        faction_id, faction_id_2, *DiploCurrentProposal, *DiploCounterProposal);
        return proposal;
    }

    parse_says(0, get_title(faction_id_2), -1, -1);
    parse_says(1, get_name(faction_id_2), -1, -1);
    parse_says(2, get_pact_hood(faction_id, faction_id_2), -1, -1);
    diplomacy_caption(faction_id, faction_id_2);
    snprintf(StrBuffer, StrBufLen, "COUNTER%d", proposal == DiploProposalMakeGift);
    start_popup(StrBuffer, POPUP_EXTERNAL);
    if (X_text_open(ScriptFile, "COUNTERMENU")) {
        return 0;
    }

    read_line();
    Dialogs_item(&cur_popup.dialogs, StrBuffer, 0);
    read_line();
    if (proposal == DiploProposalNeedEnergy) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterLoanPayment);
    }
    read_line();
    if (proposal != DiploProposalMakeGift) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterFriendship);
    }
    read_line();
    if (proposal != DiploProposalMakeGift) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterNameAPrice);
    }
    read_line();
    if (proposal != DiploProposalMakeGift
    && !(plr.diplo_status[faction_id_2] & DIPLO_PACT)) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterThreaten);
    }
    read_line();
    if (proposal != DiploProposalMakeGift
    && (plr.diplo_status[faction_id_2] & DIPLO_PACT)) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterThreaten);
    }
    read_line();
    if (!(*GameRules & RULES_SCN_NO_TECH_TRADING)) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterResearchData);
    }
    read_line();
    if (proposal == DiploProposalMakeGift) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterMakeGift);
    }
    read_line();
    if (proposal != DiploProposalNeedEnergy) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterEnergyPayment);
    }
    read_line();
    if (plr.base_count > 1 && !*MultiplayerActive
    && (proposal == DiploProposalBaseSwap || proposal == DiploProposalMakeGift)) {
        Dialogs_item(&cur_popup.dialogs, StrBuffer, DiploCounterGiveBase);
    }

    int counter = BasePop_exec_3(&cur_popup, 0, pop_wait);
    *DiploCounterProposal = counter;
    if (counter == DiploCounterGiveBase) {
        PlanWin_init(PlanWin, 0);
        PlanWin_back_redraw(PlanWin);
        bool listed = false;
        start_popup("COUNTERBASE", POPUP_EXTERNAL|POPUP_BTN_CANCEL);
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == faction_id && !has_fac_built(FAC_HEADQUARTERS, i)) {
                StrBuffer[0] = 0;
                say_base(StrBuffer, i);
                Dialogs_item(&cur_popup.dialogs, StrBuffer, i);
                if (!listed) {
                    listed = true;
                    base_swap_routine(i);
                }
            }
        }
        Win_show((Win*)((char*)PlanWin + ((DWORD*)PlanWin->vtable)[1]), 0);
        cur_popup.dialogs.listBox.cb_select_item = base_swap_routine;
        *DiploBidBaseSwapID = BasePop_exec_3(&cur_popup, 0, pop_wait);
        if (*DiploBidBaseSwapID < 0) {
            *DiploCurrentProposal = 0;
        }
        PlanWin_close(PlanWin);
        counter = *DiploCounterProposal;
    }
    if (!counter) {
        *DiploCurrentProposal = 0;
    }
    debug("proposal_menu %d %d current: %d counter: %d\n",
    faction_id, faction_id_2, *DiploCurrentProposal, *DiploCounterProposal);
    return *DiploCurrentProposal;
}

void __cdecl make_a_proposal(int faction_id, int faction_id_2, int skip_menu) {
    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    *DiploSecondFaction = faction_id_2;
    *DiploMenuFlagA = 0;
    if (!skip_menu && proposal_menu(faction_id, faction_id_2) == 0) {
        return;
    }
    *DiploMenuFlagB = 1;
    bool is_threat = *DiploCounterProposal == DiploCounterThreaten;
    switch (*DiploCurrentProposal) {
    case DiploProposalMakePact:
        if (!propose_pact(faction_id, faction_id_2) && is_threat) {
            threaten(faction_id, faction_id_2);
        }
        break;
    case DiploProposalMakeTreaty:
        if (!propose_treaty(faction_id, faction_id_2) && is_threat) {
            threaten(faction_id, faction_id_2);
        }
        break;
    case DiploProposalTradeMaps:
        propose_trade_maps(faction_id, faction_id_2);
        break;
    case DiploProposalCloseDialog:
        break;
    default:
        if (*DiploCurrentProposal == DiploProposalJointAttack
        && (Factions[faction_id_2].diplo_status[faction_id] & (DIPLO_PACT|DIPLO_HAVE_SURRENDERED))
        == (DIPLO_PACT|DIPLO_HAVE_SURRENDERED)) {
            propose_attack(faction_id, faction_id_2);
        } else if (*DiploCounterProposal == DiploCounterThreaten) {
            threaten(faction_id, faction_id_2);
        } else if (*DiploCurrentProposal == DiploProposalJointAttack) {
            propose_attack(faction_id, faction_id_2);
        } else if (*DiploCurrentProposal == DiploProposalBaseSwap) {
            mod_base_swap(faction_id, faction_id_2);
        } else if (*DiploCurrentProposal == DiploProposalTechTrade
        || *DiploCurrentProposal == DiploProposalBuyTech
        || *DiploCurrentProposal == DiploProposalTradeCommlink) {
            *DiploMenuFlagA = tech_trade(faction_id, faction_id_2, 0,
                *DiploCounterProposal, *DiploCurrentProposal);
        } else if (*DiploCurrentProposal == DiploProposalNeedEnergy) {
            *DiploMenuFlagA = mod_energy_trade(faction_id, faction_id_2);
        } else if (*DiploCurrentProposal == DiploProposalMakeGift) {
            make_gift(faction_id, faction_id_2);
        } else {
            if (*DiploCurrentProposal == DiploProposalRepayLoan) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                int loan = Factions[faction_id].loan_balance[faction_id_2];
                net_energy(faction_id, -loan, faction_id_2, loan, 1);
                net_loan(faction_id, faction_id_2, 0, 0);
                X_dialog("REPAIDLOAN", faction_id_2);
            }
            if (*DiploCurrentProposal == DiploProposalCouncilVote) {
                Popup_start(&cur_popup, PopupScriptFile, "COUNCILMATTER", -1, 0,
                    POPUP_EXTERNAL|POPUP_BTN_CANCEL, 0);
                Sprite* sprite = FactionPortraits[faction_id_2];
                Popup_load_icon(&cur_popup, sprite, 0, 0);
                for (int i = 0; i < MaxProposalNum; i++) {
                    if ((has_tech(Proposal[i].preq_tech, faction_id)
                    || ((*GameState & STATE_OMNISCIENT_VIEW) && (*GameState & STATE_DEBUG_MODE)))
                    && (i != PROP_UNITE_SUPREME_LEADER || (*GameRules & RULES_VICTORY_DIPLOMATIC))) {
                        bool listed = true;
                        switch (i) {
                            case PROP_UNITE_SUPREME_LEADER:
                                listed = !(*GameState & STATE_GAME_DONE)
                                    && !ProposalPassCount[i] && eligible(faction_id);
                                break;
                            case PROP_SALVAGE_UNITY_CORE:
                            case PROP_LAUNCH_SOLAR_SHADE:
                                listed = !ProposalPassCount[i];
                                break;
                            case PROP_GLOBAL_TRADE_PACT:
                                listed = ProposalPassCount[i] <= ProposalPassCount[PROP_REPEAL_GLOBAL_TRADE_PACT];
                                break;
                            case PROP_REPEAL_GLOBAL_TRADE_PACT:
                                listed = ProposalPassCount[i] != ProposalPassCount[PROP_GLOBAL_TRADE_PACT];
                                break;
                            case PROP_INCREASE_SOLAR_SHADE:
                                listed = ProposalPassCount[PROP_LAUNCH_SOLAR_SHADE] != 0;
                                break;
                            case PROP_REPEAL_UN_CHARTER:
                                listed = ProposalPassCount[i] <= ProposalPassCount[PROP_REINSTATE_UN_CHARTER];
                                break;
                            case PROP_REINSTATE_UN_CHARTER:
                                listed = ProposalPassCount[i] < ProposalPassCount[PROP_REPEAL_UN_CHARTER];
                                break;
                            default:
                                break;
                        }
                        if (listed) {
                            snprintf(StrBuffer, StrBufLen, "%s|%s",
                                Proposal[i].name, Proposal[i].description);
                            Dialogs_item(&cur_popup.dialogs, StrBuffer, i);
                        }
                    }
                }
                int proposal_id = BasePop_exec_3(&cur_popup, 0, pop_wait);
                if (proposal_id < 0) {
                    break;
                }
                if (proposal_id != PROP_ELECT_PLANETARY_GOVERNOR) {
                    int value = X_pops_11("COUNCILVOTE", POPUP_EXTERNAL, sprite, pop_wait);
                    if (value != -1) {
                        buy_council_vote(faction_id, faction_id_2, proposal_id, -1 - value);
                    }
                } else {
                    Popup_start(&cur_popup, PopupScriptFile, "COUNCILVOTEGOV", -1, 0, POPUP_EXTERNAL, 0);
                    Popup_load_icon(&cur_popup, sprite, 0, 0);
                    for (int i = 1; i < MaxPlayerNum; i++) {
                        if (is_alive(i) && eligible(i)) {
                            snprintf(StrBuffer, StrBufLen, "%s %s", get_title(i), get_name(i));
                            Dialogs_item(&cur_popup.dialogs, StrBuffer, i);
                            if (i == faction_id) {
                                cur_popup.active_id = i;
                            }
                        }
                    }
                    int voted_id = BasePop_exec_3(&cur_popup, 0, pop_wait);
                    if (voted_id >= 0) {
                        buy_council_vote(faction_id, faction_id_2, 0, voted_id);
                    }
                }
            }
        }
        break;
    }
}

void __cdecl commlink_attempt(int faction_id) {
    int plr_id = MapWin->cOwner;
    if (plr_id != faction_id && is_alive(faction_id)) {
        if (*SunspotDuration > 0) {
            NetMsg_pop(NetMsg, "SUNSPOTSNO", 5000, 0, 0);
            return;
        }
        if (MFactions[plr_id].is_alien() && MFactions[faction_id].is_alien()) {
            parse_says(1, get_title(plr_id), -1, -1);
            parse_says(3, get_name(plr_id), -1, -1);
            X_dialog("ALIENHATRED2", faction_id);
            return;
        }
        if (MFactions[plr_id].is_alien()
        && !has_tech(TECH_PrPsych, faction_id) && !has_tech(TECH_Psych, plr_id)) {
            NetMsg_pop(NetMsg, "WENEEDHUMANPSYCH", -5000, 0, "Alopdir.pcx");
            return;
        }
        if (MFactions[faction_id].is_alien()
        && !has_tech(TECH_PrPsych, plr_id) && !has_tech(TECH_Psych, faction_id)) {
            NetMsg_pop(NetMsg, "WENEEDALIENPSYCH", -5000, 0, "talent_sm.pcx");
            return;
        }
        if (!commlink_attempter(plr_id, faction_id)) {
            if (*MultiplayerActive) {
                if (is_human(faction_id)) {
                    *DiploWinCommlink = 1;
                    DiploWin->whom_faction_id = faction_id;
                    message_data(0x1501, faction_id, plr_id, faction_id, 0, 0);
                    parse_says(0, get_title(faction_id), -1, -1);
                    parse_says(1, get_name(faction_id), -1, -1);
                    parse_num(0, faction_id);
                    NetMsg_pop(NetMsg, "DIPLOCOMM", 5000, 0, 0);
                    return;
                }
                log_say_2("Commlink open channel", plr_id, faction_id, 0);
                if (NetDaemon_open_channel(NetState, plr_id, faction_id)) {
                    parse_says(0, get_title(faction_id), -1, -1);
                    parse_says(1, get_name(faction_id), -1, -1);
                    NetMsg_pop(NetMsg, "NOCHANNEL", 5000, 0, 0);
                    return;
                }
            }
            if (*PbemActive && is_human(faction_id)) {
                diplo(plr_id, faction_id);
            } else {
                communicate(plr_id, faction_id, 1);
                net_treaty_on(plr_id, faction_id, DIPLO_UNK_80000000|DIPLO_UNK_8000000|DIPLO_COMMLINK, 1);
            }
        }
    }
}

int __cdecl commlink_attempter(int faction_id, int faction_id_2) {
    *DiploSecondFaction = faction_id_2;
    if (!(faction_id_2 & 0x10)) {
        if ((*GameState & STATE_DEBUG_MODE) && (ctrl_key_down() || shift_key_down())) {
            diplo(faction_id, faction_id_2);
        } else {
            if (*MultiplayerActive && !(*GameMoreRules & MRULES_UNK_10)
            && NetState->dword_93E8EC & (1 << MapWin->cOwner)) {
                NetMsg_pop(NetMsg, "ALREADYDONE", 5000, 0, 0);
                return 1;
            }
            if (*DiploWinCommlink) {
                parse_says(0, get_title(DiploWin->whom_faction_id), -1, -1);
                parse_says(1, get_name(DiploWin->whom_faction_id), -1, -1);
                int choice = X_pop("DIPLOALREADY", 0);
                if (!choice) {
                    return 1;
                }
                message_data(0x1505, DiploWin->whom_faction_id, 0, 0, 0, 0);
                GraphicWin_close(DiploWin);
                if (choice > 1) {
                    return 1;
                }
            }
            if (*MultiplayerActive && (*ControlTurnC || NetState->dword_93E944)) {
                return 1;
            }
        }
        return 0;
    }
    int target_id = faction_id_2 & ~0x10;
    parse_says(0, get_title(target_id), -1, -1);
    parse_says(1, get_name(target_id), -1, -1);
    parse_says(2, get_noun(target_id), -1, -1);
    if (is_alive(target_id)) {
        NetMsg_pop(NetMsg, "NOTMETYET", 5000, 0, 0);
    } else {
        NetMsg_pop(NetMsg, "ERADICATED", 5000, 0, 0);
    }
    return 1;
}

void __cdecl communicate(int faction_id, int faction_id_2, int flag) {
    Faction& plr = Factions[faction_id];
    Faction& cmp = Factions[faction_id_2];
    int base_id;
    bool deal_done = false;
    bool refuse_demand = false;
    bool spoke_topic = false;
    bool tribute_paid = false;
    bool energy_traded = false;
    bool extort_demand = false;
    bool soft_demand = false;
    bool made_demand = false;

    Popup cur_popup;
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });
    *DiploSecondFaction = faction_id_2;
    *DiploWinState = 1;
    *DiploAcceptedFaction = 0;
    bool normal_contact = !(cmp.diplo_status[faction_id] & DIPLO_UNK_8000000)
        || (*MultiplayerActive && (DiploStateA[faction_id_2] & DST_UNK_2));
    *DiploMapsTraded = 0;
    bool has_truce = (plr.diplo_status[faction_id_2] & DIPLO_TRUCE)
        && !(plr.diplo_status[faction_id_2] & DIPLO_UNK_10000);
    *DiploCaptionMode = (flag != 0 ? 3 : 0);

    if (normal_contact) {
        int category = MFactions[faction_id_2].soc_priority_category;
        if (category >= 0) {
            int model = MFactions[faction_id_2].soc_priority_model;
            if (model) {
                int current = ((CSocialCategory*)&plr.SE_Politics_pending)->models[category];
                if (current && current != model) {
                    cause_friction(faction_id_2, faction_id, 3);
                }
            }
        }
        int effect = MFactions[faction_id_2].soc_priority_effect;
        if (effect >= 0 && ((CSocialEffect*)&plr.SE_economy_pending)->values[effect] < 0) {
            cause_friction(faction_id_2, faction_id, 3);
        }
    }
    diplomacy_check(faction_id, faction_id_2, 1);
    int diplo_state = DiploStateA[faction_id_2];
    DiploStateA[faction_id_2] = 0;

    if (diplo_state & (DST_UNK_20|DST_UNK_8|DST_UNK_4)) {
        if (diplo_state & (DST_UNK_20|DST_UNK_4)) {
            *DiploCaptionMode = 8;
        }
        if (!wants_to_speak(faction_id, faction_id_2)) {
            goto TALK_END;
        }
        if (diplo_state & DST_UNK_4) {
            territory(faction_id_2, faction_id, 0, &base_id, 0);
            diplomacy_caption(faction_id, faction_id_2);
            // Fix: added base_id bounds checking
            if (base_id >= 0 && Bases[base_id].faction_id != faction_id
            && !(Bases[base_id].visibility & (1 << faction_id))) {
                spot_base(base_id, faction_id);
                draw_tile(Bases[base_id].x, Bases[base_id].y, 2);
                Console_focus(MapWin, Bases[base_id].x, Bases[base_id].y, faction_id);
            }
            parse_says(0, base_id >= 0 ? Bases[base_id].name : "", -1, -1);
            parse_says(1, get_title(faction_id_2), -1, -1);
            parse_says(2, get_name(faction_id_2), -1, -1);
            parse_says(3, get_title(faction_id), -1, -1);
            parse_says(4, get_name(faction_id), -1, -1);
            parse_says(5, get_adjective_insult_faction(faction_id), -1, -1);
            parse_says(6, get_adjective(faction_id_2), -1, -1);
            snprintf(StrBuffer, StrBufLen, "GETOUT%d", random_val(0, 5) + 1);
            int choice = X_dialog(StrBuffer, faction_id_2);
            if (choice) {
                if (choice != 1) {
                    cause_friction(faction_id_2, faction_id, 10);
                    diplo_lock(0);
                    net_double_cross(faction_id, faction_id_2, -1, 1);
                    diplo_unlock();
                    parse_says(0, get_title(faction_id), -1, -1);
                    parse_says(1, get_name(faction_id), -1, -1);
                    parse_says(2, get_insult_leader(faction_id), -1, -1);
                    parse_says(3, get_adjective_insult_faction(faction_id), -1, -1);
                    snprintf(StrBuffer, StrBufLen, "OUTRAGE%d", random_val(0, 3));
                    X_dialog(StrBuffer, faction_id_2);
                    goto TALK_END;
                }
                if (*DiploHostility) {
                    if (((cmp.diplo_status[faction_id] & DIPLO_UNK_800) || !*DiploFearScale)
                    && ((cmp.diplo_status[faction_id] & DIPLO_WANT_REVENGE)
                    || (*DiploFriction >= 3 * (5 - cmp.AI_fight)
                    && cmp.mil_strength_1 > plr.mil_strength_1))) {
                        refuse_demand = true;
                        goto TALK_VENDETTA;
                    }
                }
                diplo_lock(0);
                net_withdrawal(faction_id_2, faction_id, 1);
                net_withdrawal(faction_id, faction_id_2, 1);
                diplo_unlock();
                X_dialog("BOTHOUT", faction_id_2);
            } else {
                diplo_lock(0);
                net_withdrawal(faction_id_2, faction_id, 1);
                diplo_unlock();
            }
        } else if (diplo_state & DST_UNK_8) {
            territory(faction_id_2, faction_id, 0, &base_id, 0);
            if (diplo_state & DST_UNK_10) {
                net_set_treaty(faction_id, faction_id_2, DIPLO_SHALL_BETRAY, 1, 1);
            }
            // Fix: added base_id bounds checking. Previously GOAWAY2 label
            // was always unused but it is used here when base_id is not set.
            if (base_id >= 0) {
                if (Bases[base_id].faction_id != faction_id
                && !(Bases[base_id].visibility & (1 << faction_id))) {
                    spot_base(base_id, faction_id);
                    draw_tile(Bases[base_id].x, Bases[base_id].y, 2);
                }
                Console_focus(MapWin, Bases[base_id].x, Bases[base_id].y, faction_id);
                parse_says(0, Bases[base_id].name, -1, -1);
                snprintf(StrBuffer, StrBufLen, "GOAWAY%d", random_val(0, 2));
                X_dialog(StrBuffer, faction_id_2);
            } else {
                parse_says(1, get_adjective(faction_id_2), -1, -1);
                X_dialog("GOAWAY2", faction_id_2);
            }
        } else if (diplo_state & DST_UNK_20) {
            territory(faction_id_2, faction_id, 0, &base_id, 0);
            if (base_id >= 0 && Bases[base_id].faction_id != faction_id_2
            && !(Bases[base_id].visibility & (1 << faction_id_2))) {
                spot_base(base_id, faction_id_2);
                draw_map(1);
            }
            parse_says(0, base_id >= 0 ? Bases[base_id].name : "", -1, -1);
            parse_says(1, get_adjective(faction_id_2), -1, -1);
            X_dialog("INOURTERRITORY2", faction_id_2);
        }
        if (!(diplo_state & (DST_UNK_2|DST_UNK_1))) {
            goto TALK_END;
        }
    }
    if (flag) {
        if (plr.diplo_status[faction_id_2] & DIPLO_WANT_TO_TALK) {
            cmp.diplo_patience[faction_id] = 0;
            net_treaty_off(faction_id_2, faction_id, DIPLO_UNK_100, 1);
        }
        parse_says(2, get_title(faction_id), -1, -1);
        parse_says(3, get_name(faction_id), -1, -1);
        if (!normal_contact) {
            if (cmp.diplo_patience[faction_id] >= get_patience(faction_id, faction_id_2)
            || (cmp.diplo_status[faction_id] & (DIPLO_UNK_100|DIPLO_UNK_40))) {
                if (!*DiploCaptionOff) {
                    diplomacy_caption(faction_id, faction_id_2);
                }
                if (plr.diplo_status[faction_id_2] & (DIPLO_TREATY|DIPLO_PACT)) {
                    parse_says(0, get_title(faction_id_2), -1, -1);
                    parse_says(1, get_name(faction_id_2), -1, -1);
                    X_dialog("NOPATIENCEFRIEND", faction_id_2);
                } else {
                    parse_says(0, get_noun(faction_id_2), -1, -1);
                    X_dialog("NOPATIENCE", faction_id_2);
                }
                goto TALK_END;
            }
        }
        wants_to_speak(faction_id, faction_id_2);
    } else {
        cmp.diplo_patience[faction_id] = 0;
        plr.diplo_spoke[faction_id_2] = *CurrentTurn;
        net_treaty_off(faction_id_2, faction_id, DIPLO_UNK_4000000, 1);
        net_agenda_off(faction_id_2, faction_id, AGENDA_UNK_100, 1);
        if (!(cmp.diplo_status[faction_id] & DIPLO_UNK_10000) && diplo_state < DST_UNK_4) {
            net_treaty_off(faction_id_2, faction_id, DIPLO_TRUCE, 1);
        }
    }
    if (cmp.diplo_status[faction_id] & DIPLO_ATROCITY_VICTIM) {
        *DiploHostility = 1;
        *DiploTributeDemand = -1;
        goto TALK_VENDETTA;
    }
    if (cmp.diplo_status[faction_id] & DIPLO_PACT) {
        if (cmp.diplo_status[faction_id] & DIPLO_HAVE_SURRENDERED) {
            tech_trade(faction_id, faction_id_2, 1, 4, 4);
            goto TALK_GREET;
        }
        if (*DiploNearEndgame && *DiploRivalOutranks) {
            *DiploHelpFaction = 0;
        }
    } else if (*DiploFearScale && !*DiploHostility && !flag && !(diplo_state & DST_UNK_40)
    && (cmp.diplo_status[faction_id] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))) {
        tech_trade(faction_id, faction_id_2, 1, 4, 4);
        goto TALK_END;
    }
    tech_analysis(faction_id, faction_id_2);

    if (!(diplo_state & DST_UNK_40)) {
        if (!(cmp.diplo_agenda[faction_id] & (AGENDA_PERMANENT|AGENDA_UNK_400|AGENDA_FIGHT_TO_DEATH))
        && !(cmp.diplo_status[faction_id] & DIPLO_UNK_40)
        && !(*DiploAttackChance && !normal_contact)) {
            if (*DiploHelpFaction && !(plr.diplo_status[*DiploHelpFaction] & DIPLO_UNK_80)
            && (*DiploHostility || !*DiploConquestFear)) {
                int help_id = *DiploHelpFaction;
                if ((!*DiploFearScale
                || *DiploAlterFear > 0
                || (*DiploHostility && !*DiploCanBetray)
                || (plr.ranking >= 6 && cmp.ranking <= 4 && !*DiploPlayerPactCount && !*DiploRivalStrongPacts)
                || great_satan(help_id, 0)
                || (cmp.diplo_status[faction_id] & DIPLO_PACT))
                && wants_to_speak(faction_id, faction_id_2)) {
                    parse_says(0, get_pact(help_id), -1, -1);
                    parse_says(1, get_title(help_id), -1, -1);
                    parse_says(2, get_name(help_id), -1, -1);
                    parse_says(3, get_noun(help_id), -1, -1);
                    parse_says(4, get_his_her(help_id, 0), -1, -1);
                    parse_says(5, get_him_her(help_id, 0), -1, -1);
                    parse_says(6, get_adjective(help_id), -1, -1);
                    const char* suffix = "";
                    int variant;
                    if ((plr.diplo_status[faction_id_2] & DIPLO_PACT)
                    && ((plr.diplo_status[faction_id_2] & DIPLO_WANT_TO_TALK)
                    || !(plr.diplo_status[help_id] & DIPLO_COMMLINK))) {
                        variant = 3;
                    } else if (plr.diplo_status[help_id] & DIPLO_PACT) {
                        variant = 0;
                        parse_says(9, get_phrase(help_id, 5), -1, -1);
                    } else if (plr.diplo_status[help_id] & DIPLO_TREATY) {
                        variant = 1;
                        parse_says(6, get_noun_phrase(help_id, 8), -1, -1);
                        parse_says(9, get_phrase(help_id, 5), -1, -1);
                        parse_says(8, get_phrase(help_id, 2), -1, -1);
                        suffix = (random_val(0, 2)
                            || (plr.diplo_status[faction_id_2] & DIPLO_WANT_REVENGE)) ? "A" : "B";
                    } else if (!(plr.diplo_status[help_id] & DIPLO_TRUCE)
                    || (plr.diplo_status[help_id] & DIPLO_UNK_10000)) {
                        variant = 4;
                    } else {
                        variant = 2;
                        if (random_val(0, 2)) {
                            suffix = "a";
                            parse_says(8, get_adjective_insult_leader(help_id), -1, -1);
                            parse_says(9, get_adjective_insult_faction(help_id), -1, -1);
                        } else {
                            suffix = "b";
                            parse_says(8, get_noun_phrase(faction_id_2, 9), -1, -1);
                            parse_says(9, get_phrase(help_id, 2), -1, -1);
                        }
                    }
                    snprintf(StrBuffer, StrBufLen, "DEMANDATTACK%d%s", variant, suffix);
                    if (X_dialog(StrBuffer, faction_id_2)) {
                        diplo_lock(0);
                        net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
                        net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
                        net_double_cross(faction_id, help_id, faction_id_2, 1);
                        diplo_unlock();
                        *DiploAcceptedFaction = help_id;
                        *DiploHelpFaction = 0;
                        deal_done = true;
                        *DiploEnemyAllyCount = 0;
                    } else {
                        if (cmp.diplo_status[faction_id] & DIPLO_PACT) {
                            refuse_demand = true;
                            if (cmp.diplo_status[faction_id] & DIPLO_WANT_TO_TALK) {
                                *DiploHostility = 1;
                            }
                            cause_friction(faction_id_2, faction_id, 4);
                        } else if (*DiploEnemyAllyCount && *DiploFriction >= 10 && !*DiploFearScale
                        && cmp.mil_strength_1 > plr.mil_strength_1) {
                            *DiploHostility = 1;
                        }
                        net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
                        net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
                        *DiploEnemyAlly = help_id;
                    }
                    diplomacy_caption(faction_id, faction_id_2);
                    goto TALK_DEMAND;
                }
            }
            if (*DiploHostility || *DiploCanBetray) {
                goto TALK_DEMAND;
            }

            if (*DiploUnmetEnemy
            && !(plr.diplo_status[*DiploUnmetEnemy] & (DIPLO_UNK_8000000|DIPLO_COMMLINK))
            && (!is_human(*DiploUnmetEnemy) || !auto_contact())
            && !(plr.diplo_status[faction_id_2] & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA|DIPLO_PACT))
            && !great_satan(faction_id, 0)
            && (plr.ranking <= 5 || cmp.ranking <= 4)
            && (!*MultiplayerActive + (*DiploPlayerPactCount == 0)
            + (Factions[*DiploUnmetEnemy].ranking > plr.ranking)
            + great_satan(*DiploUnmetEnemy, 0)) >= 2
            && !MFactions[faction_id].is_alien()
            && !MFactions[faction_id_2].is_alien()
            && wants_to_speak(faction_id, faction_id_2)) {
                int enemy_id = *DiploUnmetEnemy;
                parse_says(0, get_title(enemy_id), -1, -1);
                parse_says(1, get_name(enemy_id), -1, -1);
                parse_says(2, get_noun(enemy_id), -1, -1);
                parse_says(3, get_phrase(enemy_id, 5), -1, -1);
                parse_says(4, get_pact_hood(faction_id, faction_id_2), -1, -1);
                // Fix: original used DiploHelpFaction for is_alien check
                const char* label = !MFactions[enemy_id].is_alien()
                    ? "INTRODUCEENEMY" : "INTRODUCEENEMYALIEN";
                if (X_dialog(label, faction_id_2)) {
                    diplo_lock(0);
                    net_set_treaty(faction_id, enemy_id, DIPLO_UNK_10000000, 1, 0);
                    pact_of_brotherhood(faction_id, faction_id_2);
                    net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
                    net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
                    net_double_cross(faction_id, enemy_id, faction_id_2, 1);
                    net_treaty_on(faction_id, enemy_id, DIPLO_UNK_8000000|DIPLO_COMMLINK, 0);
                    diplo_unlock();
                    *DiploUnmetEnemy = 0;
                    *DiploAcceptedFaction = enemy_id;
                    *DiploHelpFaction = 0;
                    deal_done = true;
                    *DiploEnemyAllyCount = 0;
                    spoke_topic = true;
                }
            } else if (*DiploUnmetFaction
            && !has_treaty(faction_id, faction_id_2, DIPLO_WANT_REVENGE|DIPLO_VENDETTA|DIPLO_PACT)
            && !has_treaty(faction_id, *DiploUnmetFaction, DIPLO_VENDETTA)
            && (*DiploPlayerPactCount < 2 || great_satan(*DiploUnmetFaction, 0)
            || is_human(*DiploUnmetFaction))
            && mod_wants_to_attack(faction_id_2, *DiploUnmetFaction, faction_id)
            && (plr.ranking <= 5 || cmp.ranking <= 4 || great_satan(*DiploUnmetFaction, 0))
            && (contiguous(faction_id, *DiploUnmetFaction, 0, 0)
            || !contiguous(faction_id_2, *DiploUnmetFaction, 0, 0))
            && wants_to_speak(faction_id, faction_id_2)) {
                int enemy_id = *DiploUnmetFaction;
                parse_says(0, get_name(faction_id), -1, -1);
                parse_says(1, get_title(enemy_id), -1, -1);
                parse_says(2, get_name(enemy_id), -1, -1);
                parse_says(3, get_adjective_insult_faction(enemy_id), -1, -1);
                parse_says(4, get_phrase(enemy_id, 2), -1, -1);
                parse_says(5, get_pact_hood(faction_id, faction_id_2), -1, -1);
                parse_says(6, get_noun(enemy_id), -1, -1);
                if (X_dialog("BETRAYFRIEND", faction_id_2)) {
                    // Fix: moved diplo_lock before net_set_treaty
                    diplo_lock(0);
                    net_set_treaty(faction_id, enemy_id, DIPLO_UNK_10000000, 1, 0);
                    pact_of_brotherhood(faction_id, faction_id_2);
                    net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
                    net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
                    net_double_cross(faction_id, enemy_id, faction_id_2, 1);
                    net_treaty_on(faction_id, enemy_id, DIPLO_UNK_8000000|DIPLO_COMMLINK, 0);
                    diplo_unlock();
                    *DiploUnmetFaction = 0;
                    *DiploAcceptedFaction = enemy_id;
                    *DiploHelpFaction = 0;
                    deal_done = true;
                    *DiploEnemyAllyCount = 0;
                    spoke_topic = true;
                }
            } else if (*DiploMutualFaction
            && !has_treaty(faction_id, faction_id_2, DIPLO_WANT_REVENGE|DIPLO_VENDETTA|DIPLO_PACT)
            && !has_treaty(faction_id, *DiploMutualFaction, DIPLO_VENDETTA)
            && (*DiploPlayerPactCount < 2 || great_satan(*DiploMutualFaction, 0)
            || is_human(*DiploMutualFaction))
            && mod_wants_to_attack(faction_id_2, *DiploMutualFaction, faction_id)
            && (plr.ranking <= 5 || cmp.ranking <= 4 || great_satan(*DiploMutualFaction, 0))
            && (contiguous(faction_id, *DiploMutualFaction, 0, 0)
            || !contiguous(faction_id_2, *DiploMutualFaction, 0, 0))
            && wants_to_speak(faction_id, faction_id_2)) {
                int enemy_id = *DiploMutualFaction;
                parse_says(0, get_name(faction_id), -1, -1);
                parse_says(1, get_title(enemy_id), -1, -1);
                parse_says(2, get_name(enemy_id), -1, -1);
                parse_says(3, get_adjective_insult_faction(enemy_id), -1, -1);
                parse_says(4, get_phrase(enemy_id, 2), -1, -1);
                parse_says(5, get_pact_hood(faction_id, faction_id_2), -1, -1);
                parse_says(6, get_noun(enemy_id), -1, -1);
                if (X_dialog("BETRAYFRIEND", faction_id_2)) {
                    diplo_lock(0);
                    net_set_treaty(faction_id, enemy_id, DIPLO_UNK_10000000, 1, 0);
                    pact_of_brotherhood(faction_id, faction_id_2);
                    net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
                    net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
                    net_double_cross(faction_id, enemy_id, faction_id_2, 1);
                    net_treaty_on(faction_id, enemy_id, DIPLO_UNK_8000000|DIPLO_COMMLINK, 0);
                    diplo_unlock();
                    *DiploMutualFaction = 0;
                    *DiploAcceptedFaction = enemy_id;
                    *DiploHelpFaction = 0;
                    deal_done = true;
                    *DiploEnemyAllyCount = 0;
                    spoke_topic = true;
                }
            } else {
                if (*DiploPlayerEnemy
                && !has_treaty(faction_id, faction_id_2, DIPLO_WANT_REVENGE|DIPLO_VENDETTA|DIPLO_PACT)
                && !has_treaty(faction_id, *DiploPlayerEnemy, DIPLO_UNK_10000000|DIPLO_PACT)) {
                    int enemy_bit = 1 << *DiploPlayerEnemy;
                    if (!(plr.traded_maps & enemy_bit)
                    && (!is_human(*DiploPlayerEnemy) || (cmp.traded_maps & enemy_bit))
                    && ((*DiploTradeWantTech1 >= 0 && !(*GameRules & RULES_SCN_NO_TECH_TRADING))
                    || plr.energy_credits >= (*CurrentTurn < 50 ? 25 : 50))
                    && wants_to_speak(faction_id, faction_id_2)) {
                        int enemy_id = *DiploPlayerEnemy;
                        parse_says(0, get_title(faction_id), -1, -1);
                        parse_says(1, get_name(faction_id), -1, -1);
                        parse_says(2, get_title(enemy_id), -1, -1);
                        parse_says(3, get_name(enemy_id), -1, -1);
                        parse_says(4, get_adjective(enemy_id), -1, -1);
                        parse_tech(5, *DiploTradeWantTech1, 1);
                        int price = (*CurrentTurn < 50 ? 25 : 50);
                        parse_num(0, price);
                        clear();
                        says("ENEMYMAP");
                        says(plr.energy_credits < price ? "2" : "1");
                        net_set_treaty(faction_id, enemy_id, DIPLO_UNK_10000000, 1, 0);
                        if (X_dialog(StrBuffer, faction_id_2)) {
                            net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
                            net_tech(faction_id_2, Tech_ID_WorldMap, enemy_id, 1);
                            net_tech(faction_id, Tech_ID_WorldMap, enemy_id, 1);
                            if (plr.energy_credits < price) {
                                net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
                            } else {
                                net_energy(faction_id, -price, faction_id_2, price, 1);
                            }
                            *DiploMapsTraded = 1;
                            *DiploTradeWantTech1 = *DiploTradeWantTech2;
                            *DiploTradeWantTech2 = -1;
                            deal_done = true;
                            *DiploPlayerEnemy = 0;
                        }
                        goto TALK_DEMAND;
                    }
                }
                if (!*DiploUnmetFaction
                || has_treaty(faction_id, faction_id_2, DIPLO_WANT_REVENGE|DIPLO_VENDETTA)
                || has_project(FAC_EMPATH_GUILD, faction_id)
                || has_treaty(faction_id, *DiploUnmetFaction, DIPLO_COMMLINK)
                || (is_human(*DiploUnmetFaction) && auto_contact())
                || !wants_to_speak(faction_id, faction_id_2)) {
                    if (*DiploMutualFaction
                    && !has_treaty(faction_id, faction_id_2, DIPLO_WANT_REVENGE|DIPLO_VENDETTA)
                    && !has_treaty(faction_id, *DiploMutualFaction, DIPLO_UNK_10000000|DIPLO_PACT)) {
                        int mutual_bit = 1 << *DiploMutualFaction;
                        if (!(plr.traded_maps & mutual_bit)
                        && (!is_human(*DiploMutualFaction) || (cmp.traded_maps & mutual_bit))
                        && has_treaty(faction_id, *DiploMutualFaction, DIPLO_UNK_8000000)
                        && *DiploTradeWantTech1 >= 0
                        && wants_to_speak(faction_id, faction_id_2)) {
                            int mutual_id = *DiploMutualFaction;
                            net_maps(faction_id_2, mutual_id, 1);
                            parse_says(2, get_title(mutual_id), -1, -1);
                            parse_says(3, get_name(mutual_id), -1, -1);
                            parse_says(4, get_adjective_insult_faction(mutual_id), -1, -1);
                            parse_says(5, get_adjective(mutual_id), -1, -1);
                            parse_tech(6, *DiploTradeWantTech1, 1);
                            net_set_treaty(faction_id, mutual_id, DIPLO_UNK_10000000, 1, 0);
                            const char* label = !MFactions[mutual_id].is_alien()
                                ? "BOTHFRIEND" : "BOTHFRIENDALIEN";
                            if (X_dialog(label, faction_id_2)) {
                                net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
                                net_maps(faction_id, faction_id_2, 1);
                                net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
                                *DiploMapsTraded = 1;
                                *DiploTradeWantTech1 = *DiploTradeWantTech2;
                                *DiploTradeWantTech2 = -1;
                                deal_done = true;
                                *DiploPlayerEnemy = 0;
                            }
                            goto TALK_DEMAND;
                        }
                    }
                    if (has_treaty(faction_id, faction_id_2, DIPLO_WANT_REVENGE|DIPLO_VENDETTA)
                    || has_project(FAC_EMPATH_GUILD, faction_id_2)
                    || cmp.energy_credits < 10 * (*CurrentTurn / 25 + 1)) {
                        goto TALK_DEMAND;
                    }
                    int news_id = 0;
                    for (int i = 1; i < MaxPlayerNum; i++) {
                        if (is_alive(i) && i != faction_id && i != faction_id_2
                        && has_treaty(faction_id, i, DIPLO_COMMLINK)
                        && !has_treaty(faction_id_2, i, DIPLO_COMMLINK)
                        && !has_treaty(faction_id_2, i, DIPLO_UNK_10000000)
                        && !mod_wants_to_attack(i, faction_id_2, 0)
                        && !mod_wants_to_attack(faction_id_2, i, 0)) {
                            news_id = i;
                            break;
                        }
                    }
                    if (!news_id || !wants_to_speak(faction_id, faction_id_2)) {
                        goto TALK_DEMAND;
                    }
                    parse_says(0, get_title(news_id), -1, -1);
                    parse_says(1, get_name(news_id), -1, -1);
                    parse_says(2, get_adjective(news_id), -1, -1);
                    int news_price = 10 * (*CurrentTurn / 25 + 1);
                    parse_num(0, news_price);
                    const char* label = !MFactions[news_id].is_alien() ? "ANYNEWS" : "ANYALIENNEWS";
                    if (!X_dialog(label, faction_id_2)) {
                        goto TALK_DEMAND;
                    }
                    net_set_treaty(faction_id_2, news_id, DIPLO_UNK_10000000, 1, 0);
                    net_treaty_on(faction_id_2, news_id, DIPLO_COMMLINK, 0);
                    net_energy(faction_id_2, -news_price, faction_id, news_price, 1);
                    energy_traded = true;
                    cause_friction(faction_id_2, faction_id, -2);
                } else {
                    int offer_type = 0;
                    int unmet_id = *DiploUnmetFaction;
                    parse_says(2, get_title(unmet_id), -1, -1);
                    parse_says(3, get_name(unmet_id), -1, -1);
                    parse_says(4, get_noun(unmet_id), -1, -1);
                    parse_says(5, get_noun_phrase(unmet_id, 9), -1, -1);
                    int best_tech = -1;
                    int best_value = 0;
                    for (int i = 0; i < MaxTechnologyNum; i++) {
                        if (has_tech(i, unmet_id) && !has_tech(i, faction_id) && !has_tech(i, faction_id_2)) {
                            int value = mod_tech_val(i, faction_id, 1);
                            if (value > best_value) {
                                best_value = value;
                                best_tech = i;
                            }
                        }
                    }
                    int price = 10 * (*CurrentTurn / 25 + 1);
                    if (plr.ranking < 5) {
                        price /= 2;
                    }
                    if (plr.ranking == 7) {
                        price += price / 2;
                    }
                    if (plr.energy_credits >= price) {
                        offer_type = 2;
                        price += 5 * random_val(0, *CurrentTurn / 10 + 2);
                        if (price > plr.energy_credits) {
                            price -= 5 * ((price - plr.energy_credits + 4) / 5);
                        }
                        parse_num(0, price);
                        if (price <= 0) {
                            offer_type = 0;
                        }
                    }
                    if (offer_type == 0 && *DiploTradeWantTech1 >= 0 && *CurrentTurn > 25
                    && !(*GameRules & RULES_SCN_NO_TECH_TRADING)) {
                        offer_type = 1;
                        parse_tech(6, *DiploTradeWantTech1, 1);
                    }
                    if (best_tech >= 0) {
                        parse_says(8, Tech[best_tech].name, -1, -1);
                    }
                    clear();
                    says(!MFactions[unmet_id].is_alien() ? "METFRIEND" : "METALIEN");
                    say_num(offer_type);
                    if (best_tech >= 0) {
                        says("a");
                    }
                    if (X_dialog(StrBuffer, faction_id_2)) {
                        net_treaty_on(faction_id, unmet_id, DIPLO_COMMLINK, 0);
                        net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
                        if (offer_type == 1) {
                            cause_friction(faction_id_2, faction_id, -1);
                            net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
                            *DiploTradeWantTech1 = -1;
                        } else if (offer_type == 2) {
                            cause_friction(faction_id_2, faction_id, -1);
                            net_energy(faction_id, -price, faction_id_2, price, 1);
                        }
                    }
                }
            }
TALK_DEMAND:
            if (!has_treaty(faction_id_2, faction_id, DIPLO_TREATY|DIPLO_PACT)
            && !*DiploFearScale
            && !has_treaty(faction_id, faction_id_2, DIPLO_HAVE_SURRENDERED)
            && *DiploTributeDemand > 0
            && *DiploHostility && !*DiploCanBetray && !*MultiplayerActive && !deal_done
            && cmp.mil_strength_1 > plr.mil_strength_1) {
                int ransom_base = -1;
                int min_pop_size = 999;
                for (int i = 0; i < *BaseCount; i++) {
                    BASE* base = &Bases[i];
                    if (base->faction_id == faction_id && base->faction_id_former == faction_id_2
                    && base->pop_size < min_pop_size) {
                        min_pop_size = base->pop_size;
                        ransom_base = i;
                    }
                }
                if (ransom_base >= 0 && wants_to_speak(faction_id, faction_id_2)) {
                    parse_says(0, get_title(faction_id), -1, -1);
                    parse_says(1, get_name(faction_id), -1, -1);
                    parse_says(2, Bases[ransom_base].name, -1, -1);
                    if (X_dialog("RANSOMDEMAND", faction_id_2)) {
                        deal_done = true;
                        net_cede_base(ransom_base, faction_id_2, 1);
                        net_set_treaty(faction_id, faction_id_2, DIPLO_HAVE_SURRENDERED, 0, 0);
                        net_treaty_off(faction_id_2, faction_id, DIPLO_WANT_REVENGE, 1);
                        diplomacy_caption(faction_id, faction_id_2);
                    } else {
                        refuse_demand = true;
                    }
                }
            }
        }
    } else if (*DiploHelpFaction && wants_to_speak(faction_id, faction_id_2)) {
        int help_id = *DiploHelpFaction;
        bool agreed;
        int plea_amount = 0;
        int credits = cmp.energy_credits;
        if (Factions[help_id].mil_strength_1 < 2 * cmp.mil_strength_1 || credits < 10) {
            parse_says(0, get_title(help_id), -1, -1);
            parse_says(1, get_name(help_id), -1, -1);
            parse_says(2, get_noun(help_id), -1, -1);
            parse_says(3, get_him_her(help_id, 0), -1, -1);
            parse_says(4, get_pact_hood(faction_id, faction_id_2), -1, -1);
            parse_says(5, get_title(faction_id), -1, -1);
            parse_says(6, get_pact(help_id), -1, -1);
            bool help_pact = plr.diplo_status[help_id] & DIPLO_PACT;
            snprintf(StrBuffer, StrBufLen, "PROPOSE%s", help_pact ? "PACT" : "");
            if (help_pact) {
                parse_says(8, get_title(faction_id_2), -1, -1);
                parse_says(9, get_phrase(help_id, 2), -1, -1);
            } else {
                parse_says(8, get_adjective_insult_faction(help_id), -1, -1);
                parse_says(9, get_phrase(help_id, 5), -1, -1);
            }
            agreed = X_dialog(StrBuffer, faction_id_2);
            if (agreed) {
                diplo_lock(0);
                pact_of_brotherhood(faction_id, faction_id_2);
                net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_40|DIPLO_WANT_REVENGE, 0);
                net_double_cross(faction_id, help_id, faction_id_2, 1);
                diplo_unlock();
                *DiploAcceptedFaction = help_id;
            }
        } else {
            plea_amount = 25 * (credits / 2 / 25);
            if (plea_amount < 25) {
                plea_amount = cmp.energy_credits;
                if (credits >= 25) {
                    plea_amount = 25;
                }
            }
            parse_num(0, plea_amount);
            parse_says(0, get_title(help_id), -1, -1);
            parse_says(1, get_name(help_id), -1, -1);
            parse_says(2, get_noun(help_id), -1, -1);
            parse_says(3, get_pact_hood(faction_id, faction_id_2), -1, -1);
            parse_says(5, get_pact_hood(faction_id, help_id), -1, -1);
            parse_says(6, get_his_her(help_id, 0), -1, -1);
            parse_says(8, get_adjective_insult_leader(help_id), -1, -1);
            parse_says(9, get_phrase(help_id, 2), -1, -1);
            snprintf(StrBuffer, StrBufLen, "PLEA%s%s",
                (plr.diplo_status[help_id] & DIPLO_PACT) ? "PACT" : "",
                *DiploTradeOfferTech1 >= 0 ? "TECH" : "");
            if (*DiploTradeOfferTech1 >= 0) {
                parse_says(4, Tech[*DiploTradeOfferTech1].name, -1, -1);
            }
            agreed = X_dialog(StrBuffer, faction_id_2);
            if (agreed) {
                diplo_lock(0);
                pact_of_brotherhood(faction_id, faction_id_2);
                net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_40|DIPLO_WANT_REVENGE, 0);
                net_double_cross(faction_id, help_id, faction_id_2, 0);
                if (*DiploTradeOfferTech1 >= 0) {
                    net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 0);
                    *DiploTradeOfferTech1 = -1;
                }
                net_energy(faction_id_2, -plea_amount, faction_id, plea_amount, 1);
                energy_traded = true;
                diplo_unlock();
                *DiploAcceptedFaction = help_id;
            }
        }
        if (agreed) {
            *DiploHelpFaction = 0;
            *DiploFearScale = 0;
            *DiploTributeDemand = 0;
            *DiploPrevHostility = 0;
            *DiploHostility = 0;
            deal_done = true;
            *DiploEnemyAllyCount = 0;
            spoke_topic = true;
        } else {
            net_treaty_on(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
            net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_1000000, 0);
            *DiploEnemyAlly = help_id;
        }
    }
    if (!(*GameRules & RULES_SCN_NO_TECH_TRADING)) {
        tech_trade(faction_id, faction_id_2, 1, 4, 4);
    }
    if ((great_satan(faction_id, 0)
    || (plr.ranking >= 6 && cmp.ranking <= 4 && !*DiploRunawayLeader))
    && !*DiploHostility
    && !*DiploCanBetray
    && !has_agenda(faction_id, faction_id_2, AGENDA_PERMANENT)
    && (!has_treaty(faction_id, faction_id_2, DIPLO_PACT)
    || (!(*GameRules & RULES_VICTORY_COOPERATIVE) && !*DiploRivalWarCount))) {
        extort_demand = true;
        if (!*DiploTributeDemand && plr.energy_credits >= 125) {
            *DiploTributeDemand = 25 * (plr.energy_credits / 5 / 25);
        }
    }
    if (flag && !has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA)) {
        *DiploTributeDemand = 0;
        *DiploHostility = 0;
    } else {
        if (!flag
        && has_treaty(faction_id, faction_id_2, DIPLO_PACT)
        && !deal_done
        && !has_agenda(faction_id, faction_id_2, AGENDA_PERMANENT)
        && !(*GameRules & RULES_VICTORY_COOPERATIVE)
        && !*DiploRivalWarCount
        && ((*DiploNearEndgame && *DiploRivalOutranks) || *DiploReputation > 4 || *DiploFriction > 8)) {
            extort_demand = true;
        }
        if (*DiploTributeDemand > 0
        && plr.energy_credits >= *DiploTributeDemand
        && !deal_done
        && ((!*DiploExposedBaseCount && !*DiploAttackChance) || *DiploTradeWantTech2 < 0)
        && !has_agenda(faction_id_2, faction_id, AGENDA_PERMANENT|AGENDA_FIGHT_TO_DEATH)
        && (*DiploHostility || !*DiploConquestFear)) {
            bool bribe_wanted = refuse_demand
                || (extort_demand && plr.energy_credits >= 125 && *DiploTradeWantTech1 < 0);
            bool demand_bribe;
            if (*DiploHostility) {
                demand_bribe = !*DiploCanBetray || bribe_wanted;
            } else {
                demand_bribe = !(has_treaty(faction_id, faction_id_2, DIPLO_PACT)
                    && *DiploTradeWantTech1 >= 0) && bribe_wanted;
            }
            if (demand_bribe && wants_to_speak(faction_id, faction_id_2)) {
                if (!has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA|DIPLO_PACT)) {
                    mention_prototypes(faction_id, faction_id_2);
                }
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_title(faction_id_2), -1, -1);
                parse_says(3, get_name(faction_id_2), -1, -1);
                parse_says(4, get_adjective(faction_id_2), -1, -1);
                parse_says(5, get_pact(faction_id), -1, -1);
                parse_says(6, get_noun(faction_id_2), -1, -1);
                parse_num(0, *DiploTributeDemand);
                int alt_demand = 10 * (*DiploTributeDemand / 20);
                if (!alt_demand) {
                    alt_demand = 1;
                }
                parse_num(1, alt_demand);
                clear();
                says("DEMANDBRIBE");
                energy_traded = true;
                int variant = random_val(0, 4);
                if (extort_demand) {
                    variant = 8;
                }
                if (has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)
                && (*DiploFearScale || cmp.mil_strength_1 < plr.mil_strength_1)) {
                    variant = 9;
                }
                if (great_satan(faction_id, 0)
                && plr.mil_strength_1 > 4 * cmp.mil_strength_1 / 3
                && *DiploPrevHostility == 2) {
                    variant = 7;
                }
                if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
                    variant = 6;
                }
                if (has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA)) {
                    variant = 5 - (flag != 0);
                }
                if (has_treaty(faction_id_2, faction_id, DIPLO_UNK_40)) {
                    variant = 10;
                }
                if (!*DiploHostility && !*DiploPrevHostility) {
                    soft_demand = true;
                }
                made_demand = true;
                say_num(variant);
                switch (variant) {
                    case 0:
                        parse_says(5, get_adjective_leader(faction_id), -1, -1);
                        parse_says(6, get_insult_leader(faction_id_2), -1, -1);
                        parse_says(8, get_noun_phrase(faction_id_2, 11), -1, -1);
                        parse_says(9, get_phrase(faction_id_2, 12), -1, -1);
                        break;
                    case 1:
                        parse_says(8, get_noun_phrase(faction_id, 9), -1, -1);
                        parse_says(9, get_phrase(faction_id_2, 13), -1, -1);
                        break;
                    case 2:
                        parse_says(6, get_noun_phrase(faction_id_2, 10), -1, -1);
                        parse_says(8, get_noun_phrase(faction_id,
                            has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE) ? 8 : 9), -1, -1);
                        parse_says(9, get_phrase(faction_id_2, 1), -1, -1);
                        break;
                    case 3:
                        parse_says(9, get_phrase(faction_id_2, 14), -1, -1);
                        break;
                    case 5:
                        if (*DiploFearScale
                        || cmp.diplo_combat_count[faction_id] <= plr.diplo_combat_count[faction_id_2]) {
                            if (has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)) {
                                clear();
                                says("DEMANDBRIBE9");
                            } else {
                                says("b");
                            }
                        } else {
                            says("a");
                        }
                        break;
                    case 6:
                        parse_says(9, get_noun_phrase(faction_id_2, 9), -1, -1);
                        break;
                    default:
                        break;
                }
                int response = X_dialog(StrBuffer, faction_id_2);
                bool decided = false;
                bool pay = false;
                int amount = 0;
                if (response == 2) {
                    decided = true;
                    if (variant == 6) {
                        if (random_val(0, 2)
                        && (*DiploHostility || (*DiploNearEndgame && *DiploRivalOutranks)
                        || *DiploExposedBaseCount || *DiploAttackChance)) {
                            X_dialog("HALFBRIBE", faction_id_2);
                        } else {
                            amount = alt_demand;
                            *DiploTributeDemand = alt_demand;
                            pay = true;
                        }
                    } else {
                        int resolve = 0;
                        bool walked_out = false;
                        if (great_satan(faction_id, 0)
                        || (*DiploNearEndgame && *DiploRivalOutranks)
                        || *DiploExposedBaseCount || *DiploAttackChance
                        || has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)) {
                            resolve = has_treaty(faction_id_2, faction_id, DIPLO_SHALL_BETRAY)
                                ? 1 : random_val(0, 3);
                            walked_out = !random_val(0, 2);
                        }
                        if (!walked_out) {
                            if (!*DiploHostility || (!resolve && random_val(0, 2))) {
                                parse_num(2, alt_demand);
                                X_dialog("WEASELEDOUT", faction_id_2);
                                amount = alt_demand;
                                *DiploTributeDemand = alt_demand;
                                pay = true;
                            } else {
                                parse_says(6, get_insult_leader(faction_id_2), -1, -1);
                                response = X_dialog("WEASELOUT", faction_id_2);
                                decided = false;
                            }
                        }
                    }
                }
                if (!decided && response) {
                    amount = *DiploTributeDemand;
                    pay = true;
                }
                if (pay) {
                    deal_done = true;
                    if (variant == 10 && plr.diplo_gifts[faction_id_2]) {
                        plr.diplo_gifts[faction_id_2]--;
                    }
                    net_energy(faction_id, -amount, faction_id_2, amount, 1);
                    InfoWin->override_state = 0;
                    Console_update_data(MapWin, 0);
                    diplomacy_caption(faction_id, faction_id_2);
                    net_treaty_on(faction_id_2, faction_id, DIPLO_UNK_4000000|DIPLO_UNK_1000000, 0);
                    if (has_truce) {
                        net_treaty_on(faction_id_2, faction_id, DIPLO_TRUCE, 1);
                    }
                } else {
                    net_treaty_off(faction_id_2, faction_id, DIPLO_UNK_1000000, 0);
                    refuse_demand = true;
                }
            }
        }
    }

    {
        bool skip_threat_check = false;
        if (!normal_contact
        || deal_done
        || (*DiploTradeWantTech1 < 0 && !refuse_demand)
        || *DiploCanBetray
        || *DiploFearScale
        || (!*DiploPrevHostility
        && ((!*DiploRivalStrongPacts && cmp.ranking < 6) || plr.ranking < 6 || *DiploRunawayLeader))
        || *DiploFriction + 2 * cmp.AI_fight
        <= (5 - ((*GameRules & RULES_INTENSE_RIVALRY) ? 5 : plr.diff_level)) / 2 + 10) {
            skip_threat_check = !*DiploHostility || deal_done;
        } else {
            *DiploHostility = 1;
        }
        if (!skip_threat_check
        && !refuse_demand
        && !*DiploFearScale
        && ((!*DiploExposedBaseCount && !*DiploAttackChance) || *DiploTradeWantTech2 < 0)
        && *DiploTributeDemand > plr.energy_credits
        && (great_beelzebub(faction_id, 0)
        || (plr.ranking >= 6 && (*DiploRivalStrongPacts || cmp.ranking >= 6)))) {
            int multiplier = ((*GameRules & RULES_INTENSE_RIVALRY) ? 2 : 3);
            if (!great_satan(faction_id, 0)) {
                multiplier++;
            }
            if (*DiploTradeWantTech1 >= 0 && *DiploTributeDemand
            > multiplier * (*CurrentTurn + 7) / 8 * mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1) / 2) {
                *DiploTradeWantTech1 = -1;
            }
        }
    }
    if (*DiploTradeWantTech1 >= 0
    && *DiploTradeWantTech1 != Tech_ID_WorldMap
    && !deal_done
    && !refuse_demand
    && !(*GameRules & RULES_SCN_NO_TECH_TRADING)
    && !(*DiploAttackChance && *DiploTradeWantTech2 < 0)
    && !(*DiploAttackChance >= 2
    && *DiploAttackChance + cmp.tech_ranking / 2 >= plr.tech_ranking / 2)
    && !has_agenda(faction_id_2, faction_id, AGENDA_PERMANENT|AGENDA_FIGHT_TO_DEATH)
    && !(has_treaty(faction_id_2, faction_id, DIPLO_UNK_40)
    && *DiploTradeWantTech2 < 0 && cmp.ranking <= plr.ranking)
    && !(!*DiploHostility && *DiploConquestFear)
    && !((*DiploFearScale || has_treaty(faction_id_2, faction_id, DIPLO_PACT)) && !extort_demand)
    && !(cmp.tech_commerce_bonus > plr.tech_commerce_bonus
    && FactionRankings[7] != faction_id
    && !extort_demand
    && !has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)
    && (has_treaty(faction_id_2, faction_id, DIPLO_TREATY) || normal_contact))
    && wants_to_speak(faction_id, faction_id_2)) {
        if (!has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA|DIPLO_PACT)) {
            mention_prototypes(faction_id, faction_id_2);
        }
        int variant;
        int choice;
        int energy_price = 0;
        while (true) {
            if (*DiploTradeWantTech2 >= 0 && !*DiploExposedBaseCount && !*DiploAttackChance
            && *DiploTradeDeclined && !has_treaty(faction_id_2, faction_id, DIPLO_UNK_40)) {
                std::swap(*DiploTradeWantTech1, *DiploTradeWantTech2);
            }
            parse_tech(0, *DiploTradeWantTech1, 1);
            parse_says(1, get_title(faction_id), -1, -1);
            parse_says(2, get_name(faction_id), -1, -1);
            parse_says(3, get_title(faction_id_2), -1, -1);
            parse_says(4, get_name(faction_id_2), -1, -1);
            parse_says(5, get_pact(faction_id_2), -1, -1);
            if (*DiploTradeWantTech2 >= 0) {
                parse_tech(6, *DiploTradeWantTech2, 1);
            }
            clear();
            says("DEMANDTECH");
            variant = random_val(0, 3);
            if (!*DiploHostility) {
                if (!*DiploPrevHostility) {
                    soft_demand = true;
                }
                variant = random_val(0, 3) + 9;
                if (has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE) && !extort_demand) {
                    variant = 8;
                }
            }
            if (great_satan(faction_id, 0) || extort_demand) {
                variant = 7;
            }
            if (has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA)) {
                variant = (cmp.diplo_combat_count[faction_id]
                    <= plr.diplo_combat_count[faction_id_2]) + 3;
            }
            if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
                variant = 5;
            }
            if (has_treaty(faction_id_2, faction_id, DIPLO_UNK_40)
            && !has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)) {
                int gifts = plr.diplo_gifts[faction_id_2];
                bool friendly = gifts && gifts > plr.diplo_wrongs[faction_id_2];
                if (!friendly && *DiploTradeWantTech3 >= 0) {
                    parse_tech(8, *DiploTradeWantTech3, 1);
                    variant = 14;
                }
                if (!friendly && *DiploTradeWantTech4 >= 0) {
                    parse_tech(9, *DiploTradeWantTech4, 1);
                    variant = 15;
                } else if (variant < 14) {
                    variant = (*DiploTradeWantTech2 < 0) + 12;
                }
            }
            say_num(variant);
            if (variant >= 9 && variant <= 11) {
                if (variant != 11) {
                    parse_says(9, get_noun_phrase(faction_id_2, 9), -1, -1);
                }
                int holders = bit_count(TechOwners[*DiploTradeWantTech1]) + 1;
                int value = mod_tech_val(*DiploTradeWantTech1, faction_id, 1);
                energy_price = 25 * ((value * ((*CurrentTurn + 7) / 8) / holders + 24) / 25);
                parse_num(0, energy_price);
                Popup_start_3(&cur_popup, StrBuffer, POPUP_EXTERNAL);
                Popup_load_icon(&cur_popup, FactionPortraits[faction_id_2], 0, 0);
                says("A");
                if (!X_text_open(ScriptFile, StrBuffer)) {
                    if (*DiploTradeOfferTech1 >= 0) {
                        parse_tech(6, *DiploTradeOfferTech1, 1);
                    }
                    text_get();
                    clear();
                    parse_string(text_buf_ptr(), StrBuffer);
                    BasePop_item(&cur_popup, StrBuffer, 0);
                    text_get();
                    clear();
                    parse_string(text_buf_ptr(), StrBuffer);
                    BasePop_item(&cur_popup, StrBuffer, 1);
                    text_get();
                    clear();
                    parse_string(text_buf_ptr(), StrBuffer);
                    if (energy_price > 0) {
                        BasePop_item(&cur_popup, StrBuffer, 2);
                    }
                    text_get();
                    clear();
                    parse_string(text_buf_ptr(), StrBuffer);
                    if (*DiploTradeOfferTech1 >= 0) {
                        BasePop_item(&cur_popup, StrBuffer, 3);
                    }
                    text_get();
                    clear();
                    parse_string(text_buf_ptr(), StrBuffer);
                    BasePop_item(&cur_popup, StrBuffer, 4);
                    choice = BasePop_exec(&cur_popup, pop_wait);
                    if (choice == 4) {
                        help_topic(14, *DiploTradeWantTech1);
                        continue;
                    }
                } else {
                    choice = 0;
                }
            } else {
                switch (variant) {
                    case 0:
                        parse_says(9, get_noun_phrase(faction_id, 8), -1, -1);
                        break;
                    case 1:
                        parse_says(9, get_phrase(faction_id, 7), -1, -1);
                        break;
                    case 2:
                        parse_says(9, get_phrase(faction_id, 6), -1, -1);
                        break;
                    default:
                        break;
                }
                choice = X_dialog(StrBuffer, faction_id_2);
                if (choice == 2 || choice == 4) {
                    help_topic(14, *DiploTradeWantTech1);
                    continue;
                }
            }
            break;
        }

        parse_tech(0, *DiploTradeWantTech1, 1);
        parse_says(1, get_title(faction_id), -1, -1);
        parse_says(2, get_name(faction_id), -1, -1);
        parse_says(3, get_title(faction_id_2), -1, -1);
        parse_says(4, get_name(faction_id_2), -1, -1);
        bool tech_given = false;
        if (choice == 2) {
            if (cmp.energy_credits / 2 >= energy_price
            && (*CurrentTurn + 7) / 8 * mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1) > energy_price) {
                X_dialog("AGREEDENERGY", faction_id_2);
                net_energy(faction_id_2, -energy_price, faction_id, energy_price, 0);
                tech_given = true;
            } else if (X_dialog("DEMANDTECHAGAIN1", faction_id_2)) {
                tech_given = true;
            } else {
                choice = -1;
            }
        } else if (choice == 3) {
            bool too_costly = *DiploPrevHostility != 0;
            if (!too_costly) {
                int offer_value = 2 * mod_tech_val(*DiploTradeOfferTech1, faction_id_2, 1) / 3;
                int want_value = mod_tech_val(*DiploTradeWantTech1, faction_id_2, 1);
                too_costly = offer_value > want_value;
            }
            if (too_costly) {
                if (X_dialog("DEMANDTECHAGAIN2", faction_id_2)) {
                    tech_given = true;
                } else {
                    choice = -1;
                }
            } else {
                X_dialog("AGREEDTECH", faction_id_2);
                net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 0);
                *DiploTradeOfferTech1 = -1;
                tech_given = true;
            }
        } else if (choice > 0) {
            tech_given = true;
        }

        if (tech_given) {
            // Fix: original calculated cause_friction bonus below after DiploTradeWantTech1 was
            // replaced with DiploTradeWantTech2 and tech_val was evaluated for the wrong tech.
            int given_tech_id = *DiploTradeWantTech1;
            net_tech(faction_id_2, *DiploTradeWantTech1, faction_id, 1);
            if (variant >= 15) {
                net_tech(faction_id_2, *DiploTradeWantTech4, faction_id, 1);
            }
            int sent = variant;
            if (variant < 14) {
                if (variant == 12) {
                    sent = 6;
                    variant = 6;
                } else if ((*DiploExposedBaseCount || *DiploAttackChance)
                && *DiploHostility && *DiploTradeWantTech2 >= 0) {
                    variant = 6;
                    while (true) {
                        parse_tech(0, *DiploTradeWantTech1, 1);
                        parse_says(1, get_title(faction_id), -1, -1);
                        parse_says(2, get_name(faction_id), -1, -1);
                        parse_says(3, get_title(faction_id_2), -1, -1);
                        parse_says(4, get_name(faction_id_2), -1, -1);
                        clear();
                        says("DEMANDTECH");
                        say_num(6);
                        choice = X_dialog(StrBuffer, faction_id_2);
                        if (choice <= 1) {
                            break;
                        }
                        help_topic(14, *DiploTradeWantTech2);
                    }
                    sent = 6;
                }
            } else {
                net_tech(faction_id_2, *DiploTradeWantTech3, faction_id, 1);
                sent = 6;
                variant = 6;
            }
            if (choice) {
                int next_tech = *DiploTradeWantTech2;
                *DiploTradeWantTech1 = *DiploTradeWantTech2;
                *DiploTradeWantTech2 = -1;
                if (sent == 6) {
                    net_tech(faction_id_2, next_tech, faction_id, 1);
                    *DiploTradeWantTech1 = -1;
                }
                net_treaty_on(faction_id_2, faction_id, DIPLO_UNK_4000000|DIPLO_UNK_1000000, 0);
                if (!has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)) {
                    *DiploCanBetray = 0;
                }
                deal_done = true;
                if (has_truce) {
                    net_treaty_on(faction_id_2, faction_id, DIPLO_TRUCE, 1);
                }
                if (choice == 1) {
                    if (cmp.diplo_patience[faction_id]) {
                        cmp.diplo_patience[faction_id]--;
                    }
                    if (variant >= 9) {
                        if (variant >= 12) {
                            if (plr.diplo_gifts[faction_id_2]) {
                                plr.diplo_gifts[faction_id_2]--;
                            }
                        } else {
                            cause_friction(faction_id_2, faction_id,
                                mod_tech_val(given_tech_id, faction_id_2, 1) / -2);
                            int gifts = plr.diplo_gifts[faction_id_2];
                            if (!gifts || gifts < plr.integrity_blemishes) {
                                plr.diplo_gifts[faction_id_2] = gifts + 1;
                            }
                        }
                    }
                }
            } else {
                refuse_demand = true;
                net_treaty_off(faction_id_2, faction_id, DIPLO_UNK_1000000, 0);
            }
        } else {
            net_treaty_off(faction_id_2, faction_id, DIPLO_UNK_1000000, 0);
            refuse_demand = true;
            if (has_treaty(faction_id_2, faction_id, DIPLO_PACT) && !choice) {
                cause_friction(faction_id_2, faction_id, (2 * *DiploReputation + 3) / 5);
                cmp.diplo_patience[faction_id]++;
            }
        }
        diplomacy_caption(faction_id, faction_id_2);
    }
    if (has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
        ++*DiploRivalWarCount;
    }
    if (!strcmp(MFactions[faction_id_2].filename, "MORGAN")) {
        ++*DiploRivalWarCount;
    }
    if (plr.energy_credits > cmp.energy_credits
    && plr.energy_credits >= 200
    && *DiploReputation < 4
    && (plr.energy_credits >= 2 * cmp.energy_credits
    || (has_treaty(faction_id, faction_id_2, DIPLO_PACT) && plr.ranking > cmp.ranking))
    && (!*DiploHostility || deal_done)
    && !energy_traded
    && (deal_done || !refuse_demand || soft_demand || has_treaty(faction_id, faction_id_2, DIPLO_PACT))
    && !has_treaty(faction_id, faction_id_2, DIPLO_VENDETTA)
    && has_treaty(faction_id, faction_id_2, DIPLO_UNK_10000|DIPLO_TREATY|DIPLO_PACT)
    && !has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)
    && !plr.loan_balance[faction_id_2]
    && !cmp.loan_balance[faction_id]
    && (cmp.energy_credits < 20 * (5 * *DiploRivalWarCount + 5)
    || (*DiploHelpFaction > 0
    && cmp.energy_credits < 3 * Factions[*DiploHelpFaction].energy_credits / 4))
    && wants_to_speak(faction_id, faction_id_2)) {
        int loan_amount = min(plr.energy_credits / 2,
            100 * (cmp.energy_surplus_total - cmp.facility_maint_total));
        if (loan_amount >= 50) {
            int interest = 5 * (cmp.integrity_blemishes
                + 2 * (plr.tech_commerce_bonus - cmp.tech_commerce_bonus) + 10);
            if (has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
                interest -= 10;
            }
            if (interest < 10) {
                interest = 10;
            }
            int turns = 10 * (10 - cmp.integrity_blemishes);
            if (turns < 20) {
                turns = 20;
            }
            int multiplier = interest + 100;
            int total_due = loan_amount * multiplier / 100;
            if (total_due < turns) {
                turns = total_due;
            }
            int payment = (total_due + turns - 1) / max(1, turns);
            int loan_total = payment * turns;
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            parse_says(2, get_title(faction_id_2), -1, -1);
            parse_says(3, get_name(faction_id_2), -1, -1);
            parse_num(0, loan_amount);
            parse_num(1, payment);
            parse_num(2, turns);
            clear();
            says("ASKFORLOAN");
            say_num((payment != 1) + 1);
            int response = X_dialog(StrBuffer, faction_id_2);
            if (response) {
                if (response == 2) {
                    loan_amount /= 2;
                    total_due = loan_amount * multiplier / 100;
                    if (total_due < turns) {
                        turns = total_due;
                    }
                    payment = (total_due + turns - 1) / max(1, turns);
                    loan_total = payment * turns;
                }
                net_treaty_on(faction_id_2, faction_id, DIPLO_UNK_4000000|DIPLO_UNK_1000000, 0);
                net_energy(faction_id, -loan_amount, faction_id_2, loan_amount, 1);
                net_loan(faction_id_2, faction_id, loan_total, payment);
                cause_friction(faction_id_2, faction_id, -energy_value(loan_amount));
                deal_done = true;
            } else if (has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
                cause_friction(faction_id_2, faction_id, 1);
            }
        }
    }

    if (!(flag
    || deal_done
    || (!*DiploRivalStrongPacts && *DiploFearScale
    && !has_treaty(faction_id_2, faction_id, DIPLO_PACT)
    && !has_agenda(faction_id_2, faction_id, AGENDA_UNK_400))
    || !has_agenda(faction_id_2, faction_id, AGENDA_UNK_1)
    || has_agenda(faction_id, faction_id_2, AGENDA_PERMANENT))) {
        int category = MFactions[faction_id_2].soc_priority_category;
        int model = MFactions[faction_id_2].soc_priority_model;
        if (category >= 0 && model) {
            int current = ((CSocialCategory*)&plr.SE_Politics_pending)->models[category];
            if (current && current != model) {
                wants_to_speak(faction_id, faction_id_2);
                if (!*DiploCaptionOff) {
                    diplomacy_caption(faction_id, faction_id_2);
                }
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_pact_hood(faction_id, faction_id_2), -1, -1);
                parse_says(3, get_noun(faction_id_2), -1, -1);
                clear();
                says("SOCIAL");
                say_num(category);
                says("CAT");
                say_num(current);
                says(has_treaty(faction_id, faction_id_2, DIPLO_PACT) ? "PACT" : "WAR");
                say_num(model);
                X_dialog(StrBuffer, faction_id_2);
                if (has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
                    diplo_lock(0);
                    net_pact_ends(faction_id_2, faction_id, 1);
                    diplo_unlock();
                } else {
                    net_set_treaty(faction_id_2, faction_id, DIPLO_UNK_40, 1, 1);
                    net_treaty_on(faction_id_2, faction_id, DIPLO_VENDETTA, 1);
                }
                plr.diplo_spoke[faction_id_2] = *CurrentTurn;
                goto TALK_END;
            }
        }
    }

    {
        bool goodwill = false;
        int gifts = plr.diplo_gifts[faction_id_2];
        if (gifts && gifts > plr.diplo_wrongs[faction_id_2]
        && !has_treaty(faction_id_2, faction_id, DIPLO_UNK_40|DIPLO_VENDETTA)
        && !has_agenda(faction_id_2, faction_id, AGENDA_PERMANENT|AGENDA_FIGHT_TO_DEATH)) {
            goodwill = !has_treaty(faction_id_2, faction_id, DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE);
        }
        bool drop_hostility = false;
        if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
            if (deal_done) {
                drop_hostility = true;
            } else if (!flag && !has_agenda(faction_id_2, faction_id, AGENDA_PERMANENT)) {
                bool skip_check = false;
                if (!*DiploHostility) {
                    skip_check = (plr.ranking < 6 || cmp.ranking < 6)
                        && (*DiploFriction < 2 * (cmp.tech_commerce_bonus - plr.tech_commerce_bonus) + 10
                        || (cmp.ranking < 5 && (*GameRules & RULES_VICTORY_COOPERATIVE)
                        && *CurrentTurn >= 200))
                        && (!*DiploNearEndgame || !*DiploRivalOutranks
                        || (*GameRules & RULES_VICTORY_COOPERATIVE));
                }
                if (!skip_check) {
                    if (!goodwill) {
                        wants_to_speak(faction_id, faction_id_2);
                        if (!*DiploCaptionOff) {
                            diplomacy_caption(faction_id, faction_id_2);
                        }
                        parse_says(0, get_title(faction_id), -1, -1);
                        parse_says(1, get_name(faction_id), -1, -1);
                        parse_says(2, get_pacts_2(faction_id, faction_id_2), -1, -1);
                        parse_says(3, get_noun_phrase(faction_id, 8), -1, -1);
                        X_dialog("BREAKPACT", faction_id_2);
                        diplo_lock(0);
                        net_pact_ends(faction_id_2, faction_id, 1);
                        diplo_unlock();
                        plr.diplo_spoke[faction_id_2] = *CurrentTurn;
                        goto TALK_END;
                    }
                    *DiploHostility = 0;
                    plr.diplo_gifts[faction_id_2]--;
                }
            }
        } else if (deal_done) {
            drop_hostility = true;
        }
        if (drop_hostility || has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
            *DiploHostility = 0;
        } else if (*DiploHostility) {
            if (goodwill) {
                *DiploHostility = 0;
                plr.diplo_gifts[faction_id_2]--;
            } else if (plr.ranking > cmp.ranking
            && *DiploCapturedBases > *DiploRivalStrongPacts + plr.integrity_blemishes
            && !*DiploAttackChance
            && !*DiploExposedBaseCount
            && has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA)
            && !has_agenda(faction_id_2, faction_id, AGENDA_FIGHT_TO_DEATH)
            && has_treaty(faction_id_2, faction_id, DIPLO_WANT_TO_TALK)) {
                goto TALK_PACT;
            }
        }
    }
    if (*DiploEnemyAllyCount
    && !has_treaty(faction_id, faction_id_2, DIPLO_VENDETTA)
    && ((*DiploHostility && *DiploHelpFaction)
    || (MFactions[faction_id_2].is_alien() && MFactions[*DiploEnemyAlly].is_alien()))) {
        wants_to_speak(faction_id, faction_id_2);
        net_set_treaty(faction_id_2, faction_id, DIPLO_UNK_40, 1, 1);
        net_treaty_on(faction_id_2, faction_id, DIPLO_VENDETTA, 1);
        plr.diplo_spoke[faction_id_2] = *CurrentTurn;
        diplomacy_caption(faction_id, faction_id_2);
        clear();
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_says(2, get_title(*DiploEnemyAlly), -1, -1);
        parse_says(3, get_name(*DiploEnemyAlly), -1, -1);
        if (!MFactions[faction_id_2].is_alien() || !MFactions[*DiploEnemyAlly].is_alien()) {
            X_dialog("COLLABORATOR", faction_id_2);
        } else {
            X_dialog("ALIENHATRED", faction_id_2);
        }
        goto TALK_END;
    }
    if (*DiploHostility && refuse_demand
    && (has_treaty(faction_id, faction_id_2, DIPLO_TRUCE|DIPLO_TREATY)
    || (!has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA)
    && *DiploSpokeTurn > 0
    && !has_treaty(faction_id_2, faction_id, DIPLO_WANT_TO_TALK)))) {
        wants_to_speak(faction_id, faction_id_2);
        net_treaty_on(faction_id_2, faction_id, DIPLO_VENDETTA, 1);
        diplomacy_caption(faction_id, faction_id_2);
        plr.diplo_spoke[faction_id_2] = *CurrentTurn;
        clear();
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_says(2, get_adjective_insult_faction(faction_id), -1, -1);
        parse_says(3, get_adjective_leader(faction_id), -1, -1);
        parse_says(4, get_phrase(faction_id_2, 13), -1, -1);
        parse_says(5, get_noun_phrase(faction_id, 10), -1, -1);
        says("BULLY");
        say_num(random_val(0, 7));
        X_dialog(StrBuffer, faction_id_2);
        goto TALK_END;
    }

TALK_VENDETTA:
    if (*DiploHostility) {
        if (refuse_demand
        || *DiploSpokeTurn < 0
        || flag
        || has_treaty(faction_id_2, faction_id, DIPLO_WANT_TO_TALK)
        || has_agenda(faction_id_2, faction_id, AGENDA_UNK_4)) {
            if (*DiploTributeDemand >= 0) {
                wants_to_speak(faction_id, faction_id_2);
            }
            bool had_vendetta = has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA);
            net_treaty_on(faction_id_2, faction_id, DIPLO_VENDETTA, 1);
            plr.diplo_spoke[faction_id_2] = *CurrentTurn;
            diplomacy_caption(faction_id, faction_id_2);
            clear();
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            parse_says(2, get_adjective_insult_leader(faction_id), -1, -1);
            parse_says(3, get_adjective_insult_faction(faction_id), -1, -1);
            parse_says(4, get_noun_phrase(faction_id, 8), -1, -1);
            parse_says(5, get_phrase(faction_id, 3), -1, -1);
            parse_says(6, get_phrase(faction_id_2, 13), -1, -1);
            parse_says(8, get_phrase(faction_id, 4), -1, -1);
            parse_says(9, get_insult_leader(faction_id), -1, -1);
            says("VENDETTA");
            int message;
            if (!refuse_demand || had_vendetta) {
                message = random_val(0, 6) + 3;
            } else {
                message = random_val(0, 3);
            }
            if (great_satan(faction_id, 0) && random_val(0, 3)) {
                message = random_val(0, 4) + 15;
            }
            bool want_talk = *DiploSpokeTurn < 0 || flag
                || has_treaty(faction_id_2, faction_id, DIPLO_WANT_TO_TALK)
                || has_agenda(faction_id_2, faction_id, AGENDA_UNK_4);
            if (want_talk && *DiploTributeDemand >= 0) {
                message = 14;
            }
            if (has_treaty(faction_id_2, faction_id, DIPLO_ATROCITY_VICTIM)
            && has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)) {
                message = random_val(0, 5) + 9;
            }
            say_num(message);
            // Fix: remove unimplemented VENDETTA5X dialog reference for CYBORG faction filename
            if (has_agenda(faction_id_2, faction_id, AGENDA_UNK_4)) {
                clear();
                says("ATROCIOUS");
            }
            X_dialog(StrBuffer, faction_id_2);
        }
        goto TALK_END;
    }

TALK_PACT:
    if (has_truce && deal_done) {
        net_treaty_on(faction_id_2, faction_id, DIPLO_TRUCE, 1);
    }
    {
        bool pact_offer = !((!has_treaty(faction_id_2, faction_id, DIPLO_TREATY)
            && (!*DiploAcceptedFaction || has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA)))
            || has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE|DIPLO_PACT)
            || has_agenda(faction_id_2, faction_id, AGENDA_PERMANENT)
            || *DiploGreatSatan);
        int shared_enemies = 0;
        bool enemy_truce = false;
        if (pact_offer) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (is_alive(i) && has_treaty(faction_id_2, i, DIPLO_VENDETTA)) {
                    if (has_treaty(faction_id, i, DIPLO_VENDETTA|DIPLO_TRUCE)) {
                        enemy_truce = true;
                    }
                    if (has_treaty(faction_id, i, DIPLO_PACT)) {
                        pact_offer = false;
                        break;
                    }
                    for (int region = 1; region < MaxRegionNum; region++) {
                        if (!bad_reg(region) && cmp.region_total_bases[region]
                        && Factions[i].region_total_bases[region]) {
                            shared_enemies++;
                            break;
                        }
                    }
                }
            }
        }
        if (pact_offer && (great_satan(faction_id, 0)
        || has_agenda(faction_id_2, faction_id, AGENDA_UNK_400) || made_demand)) {
            pact_offer = false;
        }
        if (pact_offer) {
            bool hive = !strcmp(MFactions[faction_id_2].filename, "HIVE");
            bool coop = (*GameRules & RULES_VICTORY_COOPERATIVE) != 0;
            int bonus_diff = cmp.tech_commerce_bonus - plr.tech_commerce_bonus;
            if (*DiploReputation >= range(bonus_diff, 0, 99) + hive + 3 + coop + (*DiploAcceptedFaction != 0)) {
                pact_offer = false;
            } else {
                bool non_rival = (coop && !(*GameRules & RULES_INTENSE_RIVALRY));
                if (*DiploFriction + *DiploReputation - min(3, plr.diplo_reputation)
                >= range(bonus_diff + 1, 0, 99) + hive + non_rival + (enemy_truce ? 11 : 6)
                + (*DiploAcceptedFaction != 0)
                || ((!shared_enemies || *DiploPlayerPactCount || *DiploRivalStrongPacts
                || (!*DiploFearScale && !enemy_truce && plr.ranking > 5 && cmp.ranking > 5))
                && (plr.ranking > 5 || cmp.ranking > 5))) {
                    pact_offer = false;
                }
            }
        }
        if (pact_offer && wants_to_speak(faction_id, faction_id_2)) {
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            parse_says(2, get_pact_hood(faction_id, faction_id_2), -1, -1);
            if (!X_dialog("SWEARAPACT", faction_id_2)) {
                bool brotherhood = true;
                if (*DiploHelpFaction) {
                    parse_says(5, get_title(faction_id), -1, -1);
                    parse_says(6, get_noun(*DiploHelpFaction), -1, -1);
                    if (X_dialog("GIMMEHELP", faction_id_2)) {
                        diplo_lock(0);
                        net_double_cross(faction_id, *DiploHelpFaction, faction_id_2, 0);
                        net_treaty_off(faction_id, faction_id_2, DIPLO_UNK_800000, 0);
                        net_set_treaty(faction_id, *DiploHelpFaction, DIPLO_UNK_40, 1, 1);
                        diplo_unlock();
                        *DiploHelpFaction = 0;
                        *DiploEnemyAllyCount = 0;
                    } else if (*DiploFearScale) {
                        brotherhood = spoke_topic;
                    } else {
                        parse_says(2, get_pacts_2(faction_id, faction_id_2), -1, -1);
                        X_dialog("REJPACTNOHELP", faction_id_2);
                        brotherhood = false;
                    }
                }
                if (brotherhood) {
                    pact_of_brotherhood(faction_id, faction_id_2);
                    diplomacy_caption(faction_id, faction_id_2);
                }
                spoke_topic = true;
            }
        }
    }
    if (has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA)
    && *DiploFearScale
    && *DiploReputation <= 2
    && !has_treaty(faction_id_2, faction_id, DIPLO_ATROCITY_VICTIM)
    && !has_treaty(faction_id_2, faction_id, DIPLO_HAVE_SURRENDERED)
    && !has_agenda(faction_id_2, faction_id, AGENDA_PERMANENT)
    && *DiploFearScale > *DiploAlterFear + 2
    && !*MultiplayerActive
    && wants_to_speak(faction_id, faction_id_2)) {
        int ransom_base = -1;
        int min_pop_size = 999;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* base = &Bases[i];
            if (base->faction_id == faction_id_2 && base->faction_id_former == faction_id
            && base->pop_size < min_pop_size) {
                min_pop_size = base->pop_size;
                ransom_base = i;
            }
        }
        if (ransom_base >= 0 && wants_to_speak(faction_id, faction_id_2)) {
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            parse_says(2, Bases[ransom_base].name, -1, -1);
            if (!X_dialog("RANSOMTREATY", faction_id_2)) {
                spoke_topic = true;
                net_cede_base(ransom_base, faction_id, 1);
                net_set_treaty(faction_id_2, faction_id, DIPLO_HAVE_SURRENDERED, 1, 0);
                pledge_truce(faction_id, faction_id_2, 0);
                make_treaty(faction_id, faction_id_2);
                diplomacy_caption(faction_id, faction_id_2);
            }
        }
    }
    if (has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA) && wants_to_speak(faction_id, faction_id_2)) {
        int truce_req;
        if (*DiploCanSurrender
        && (has_agenda(faction_id_2, faction_id, AGENDA_UNK_800)
        || (has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)
        && !random_val(0, 3 - (has_treaty(faction_id_2, faction_id, DIPLO_SHALL_BETRAY) != 0))))) {
            truce_req = 1;
        } else if (deal_done) {
            truce_req = X_dialog("OFFERTRUCE", faction_id_2);
        } else if (*DiploHostility) {
            truce_req = X_dialog("MUSTTRUCE", faction_id_2);
        } else {
            mention_prototypes(faction_id, faction_id_2);
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            parse_says(2, get_title(faction_id_2), -1, -1);
            parse_says(3, get_name(faction_id_2), -1, -1);
            clear();
            int roll = random_val(0, 4);
            if ((roll == 2 && has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)) || roll >= 3) {
                if (MFactions[faction_id].is_alien()) {
                    says("ALIEN");
                }
                says("FACTIONTRUCE");
                truce_req = X_dialog_alt(MFactions[faction_id_2].filename, StrBuffer, faction_id_2);
            } else {
                says("WANTTOTRUCE");
                say_num(roll);
                truce_req = X_dialog(StrBuffer, faction_id_2);
            }
        }
        int surrender_odds = *DiploReputation + 2;
        if (has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)) {
            surrender_odds *= 2;
        }
        int odds_cap = range(surrender_odds, 0, cmp.base_count + 1);
        if (truce_req) {
            bool truce_agreed = false;
            if (random_val(0, odds_cap) < *DiploFearScale || *DiploCanSurrender) {
                int truce_value = truce_req;
                bool offer_energy = true;
                if (*DiploTradeOfferTech1 >= 0
                && *DiploFearScale >= mod_tech_val(*DiploTradeOfferTech1, faction_id_2, 1) / 2
                && !*DiploCanSurrender
                && !*DiploCanBetray
                && !(*GameRules & RULES_SCN_NO_TECH_TRADING)) {
                    parse_tech(0, *DiploTradeOfferTech1, 1);
                    int tech_choice = X_dialog("TECHTRUCE", faction_id_2);
                    while (tech_choice > 1) {
                        help_topic(14, *DiploTradeOfferTech1);
                        parse_tech(0, *DiploTradeOfferTech1, 1);
                        tech_choice = X_dialog("TECHTRUCE", faction_id_2);
                    }
                    if (!tech_choice) {
                        truce_value = 0;
                        net_tech(faction_id, *DiploTradeOfferTech1, faction_id_2, 1);
                        tribute_paid = true;
                        offer_energy = false;
                    }
                }
                if (offer_energy
                && (*DiploTradeOfferTech1 < 0 || *DiploFearScale < 4)
                && !*DiploCanSurrender
                && !*DiploCanBetray) {
                    int tribute = 25 * (min(100 * *DiploFearScale, cmp.energy_credits) / 25);
                    if (tribute) {
                        parse_num(0, tribute);
                        if (!X_dialog("ENERGYTRUCE", faction_id_2)) {
                            truce_value = 0;
                            tribute_paid = true;
                            net_energy(faction_id_2, -tribute, faction_id, tribute, 1);
                            InfoWin->override_state = 0;
                            Console_update_data(MapWin, 0);
                        }
                    }
                }
                int our_strength = (plr.diplo_combat_count[faction_id_2] + plr.diplo_combat_total[faction_id_2])
                    * (cmp.total_combat_units + plr.mil_strength_2) / 4 + 1;
                int their_strength = cmp.diplo_combat_total[faction_id] + cmp.diplo_combat_count[faction_id] + 1;
                for (int region = 0; region < MaxRegionNum; region++) {
                    if (cmp.region_total_bases[region]) {
                        their_strength += cmp.region_total_combat_units[region] + cmp.region_force_rating[region];
                        our_strength += plr.region_force_rating[region];
                        for (int i = 1; i < MaxPlayerNum; i++) {
                            if (has_treaty(i, faction_id_2, DIPLO_PACT) || !has_treaty(i, faction_id, DIPLO_VENDETTA)) {
                                their_strength += Factions[i].region_force_rating[region];
                            }
                        }
                    } else {
                        our_strength += plr.region_force_rating[region] >> 1;
                    }
                }
                if (!truce_value) {
                    truce_agreed = true;
                } else if (!MFactions[faction_id_2].is_alien()
                && (*DiploCanSurrender
                || (*DiploFearScale >= cmp.base_count
                && (cmp.base_count <= 1
                || (cmp.base_count <= our_strength / their_strength && cmp.base_count <= 4
                && cmp.mil_strength_1 < plr.mil_strength_1))))) {
                    parse_says(0, get_title(faction_id), -1, -1);
                    parse_says(1, get_name(faction_id), -1, -1);
                    parse_num(0, cmp.energy_credits);
                    int techs_lacking = 0;
                    for (int i = 0; i < MaxTechnologyNum; i++) {
                        if (has_tech(i, faction_id_2) && !has_tech(i, faction_id)) {
                            techs_lacking++;
                        }
                    }
                    parse_num(1, techs_lacking);
                    if (!X_dialog("TRUCEPLEASE", faction_id_2)) {
                        tribute_paid = true;
                        if (!(*GameRules & RULES_SCN_NO_TECH_TRADING)) {
                            for (int i = 0; i < MaxTechnologyNum; i++) {
                                if (has_tech(i, faction_id_2) && !has_tech(i, faction_id)) {
                                    net_tech(faction_id, i, faction_id_2, 0);
                                }
                            }
                        }
                        net_energy(faction_id, cmp.energy_credits, faction_id_2, -cmp.energy_credits, 1);
                        InfoWin->override_state = 0;
                        Console_update_data(MapWin, 0);
                        plr.diplo_friction[faction_id_2] = 0;
                        net_treaty_off(faction_id_2, faction_id, DIPLO_WANT_REVENGE, 1);
                        net_treaty_on(faction_id, faction_id_2, DIPLO_TREATY, 0);
                        net_set_treaty(faction_id_2, faction_id, DIPLO_HAVE_SURRENDERED, 1, 0);
                        pact_of_brotherhood(faction_id, faction_id_2);
                        diplomacy_caption(faction_id, faction_id_2);
                        if (!*MultiplayerActive && (*GameRules & RULES_VICTORY_CONQUEST)) {
                            int alive_count = 0;
                            int pact_count = 0;
                            for (int i = 1; i < MaxPlayerNum; i++) {
                                if (is_alive(i)) {
                                    alive_count++;
                                    if (has_treaty(i, MapWin->cOwner, DIPLO_PACT)
                                    && has_treaty(i, MapWin->cOwner, DIPLO_HAVE_SURRENDERED)) {
                                        alive_count--;
                                    } else if (has_treaty(i, MapWin->cOwner, DIPLO_PACT)
                                    && (*GameRules & RULES_VICTORY_COOPERATIVE) && pact_count < 2) {
                                        pact_count++;
                                    }
                                }
                            }
                            if (alive_count - pact_count < 2) {
                                *GameVictoryType = VIC_UNIFY_SOLO;
                                *GameState |= STATE_VICTORY_CONQUER|STATE_GAME_DONE;
                                if (pact_count) {
                                    *GameVictoryType = VIC_UNIFY_COOP;
                                }
                            }
                        }
                        truce_agreed = true;
                    } else {
                        net_set_agenda(faction_id_2, faction_id, AGENDA_FIGHT_TO_DEATH, 1, 0);
                        net_treaty_on(faction_id_2, faction_id, DIPLO_WANT_REVENGE, 1);
                    }
                }
            }
            if (!truce_agreed) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_adjective_insult_faction(faction_id), -1, -1);
                X_dialog("MOREWAR", faction_id_2);
                plr.diplo_spoke[faction_id_2] = *CurrentTurn;
                goto TALK_END;
            }
            *DiploSpokeTurn = *CurrentTurn;
        }
        spoke_topic = true;
        if (!has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
            pledge_truce(faction_id, faction_id_2, 1);
        }
        diplomacy_caption(faction_id, faction_id_2);
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        if (*DiploReputation > 5) {
            X_dialog("NOTORIETY", faction_id_2);
        }
        if (plr.loan_balance[faction_id_2]) {
            parse_num(0, plr.loan_payment[faction_id_2]);
            parse_num(1, plr.loan_balance[faction_id_2]);
            X_dialog("TRUCEMEPAY", faction_id_2);
        } else if (cmp.loan_balance[faction_id]) {
            parse_num(0, cmp.loan_payment[faction_id]);
            X_dialog("TRUCEYOUPAY", faction_id_2);
        }
    }

    if (normal_contact && !has_treaty(faction_id_2, faction_id, DIPLO_VENDETTA)) {
        net_treaty_on(faction_id_2, faction_id, DIPLO_UNK_10000|DIPLO_TRUCE, 1);
    }
    if (*DiploReputation > 5
    && has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)
    && (*DiploRivalPactCount || *DiploGreatSatan || cmp.mil_strength_1 > plr.mil_strength_1)) {
        *DiploCanBetray = 1;
    }
    if (!has_treaty(faction_id, faction_id_2, DIPLO_TREATY)
    && !has_agenda(faction_id, faction_id_2, AGENDA_PERMANENT)) {
        bool truce_ending = has_truce && !has_treaty(faction_id, faction_id_2, DIPLO_TRUCE);
        const char* faction_file = MFactions[faction_id_2].filename;
        bool gaians = !strcmp(faction_file, "GAIANS");
        int score = 2 * (4 - cmp.AI_fight) * (plr.mil_strength_1 + 1) / (cmp.mil_strength_1 + 1);
        if (gaians) {
            score += 2;
        }
        if (truce_ending) {
            score += 3 - cmp.AI_fight;
        }
        if (plr.base_count && cmp.tech_commerce_bonus >= plr.tech_commerce_bonus) {
            score += plr.energy_surplus_total / plr.base_count;
            if (cmp.tech_commerce_bonus > plr.tech_commerce_bonus) {
                score++;
            }
        }
        int friction_limit;
        if (great_satan(faction_id, 0) || (plr.ranking > 4 && cmp.ranking > 4)) {
            friction_limit = score;
        } else {
            friction_limit = score + 1;
            if (!*DiploRivalStrongPacts) {
                friction_limit = score + 2;
            }
        }
        if ((*DiploFearScale || *DiploFriction <= friction_limit) && !*DiploGreatSatan) {
            int extra = (truce_ending || gaians) ? 1 : 0;
            int bonus_diff = cmp.tech_commerce_bonus - plr.tech_commerce_bonus;
            if (*DiploCapturedBases + *DiploReputation
            < (*DiploFearScale != 0) + range(bonus_diff, 0, 99) + extra + 5
            && (!has_treaty(faction_id, faction_id_2, DIPLO_WANT_REVENGE)
            || (*DiploFearScale && truce_ending && *DiploReputation <= 4))
            && wants_to_speak(faction_id, faction_id_2)) {
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_adjective_faction(faction_id), -1, -1);
                parse_says(3, get_name(faction_id_2), -1, -1);
                parse_says(4, get_title(faction_id_2), -1, -1);
                int value;
                if (has_truce) {
                    clear();
                    says("OFFERTREATY");
                    if (!has_treaty(faction_id, faction_id_2, DIPLO_TRUCE)) {
                        says("2");
                    }
                    value = X_dialog(StrBuffer, faction_id_2);
                } else {
                    clear();
                    if (MFactions[faction_id].is_alien()) {
                        says("ALIEN");
                    }
                    says("FACTIONTREATY");
                    value = X_dialog_alt(faction_file, StrBuffer, faction_id_2);
                }
                if (value && !has_treaty(faction_id_2, faction_id, DIPLO_WANT_REVENGE)) {
                    int tribute = 25 * (min(50 * *DiploFearScale, cmp.energy_credits) / 25);
                    if (tribute) {
                        parse_says(0, get_title(faction_id_2), -1, -1);
                        parse_says(1, get_name(faction_id_2), -1, -1);
                        parse_says(2, get_noun_phrase(faction_id, 9), -1, -1);
                        parse_num(0, tribute);
                        if (!X_dialog("ENERGYTREATY", faction_id_2)) {
                            tribute_paid = true;
                            value = 0;
                            net_energy(faction_id_2, -tribute, faction_id, tribute, 1);
                            InfoWin->override_state = 0;
                            Console_update_data(MapWin, 0);
                        }
                    }
                }
                spoke_topic = true;
                if (!value) {
                    make_treaty(faction_id, faction_id_2);
                    diplomacy_caption(faction_id, faction_id_2);
                }
            }
        }
    }

TALK_GREET:
    if (normal_contact) {
        wants_to_speak(faction_id, faction_id_2);
    }
    if (!(*DiploCaptionOff && *DiploCaptionMode) && !flag) {
        goto TALK_END;
    }
    {
        bool mention = true;
        if (spoke_topic) {
            if (has_treaty(faction_id_2, faction_id, DIPLO_UNK_40|DIPLO_WANT_REVENGE)
            && (flag || *DiploReputation >= 5)) {
                mention = false;
            } else {
                int wronged_id = -1;
                int most_wrongs = 0;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (i != faction_id && i != faction_id_2
                    && !has_treaty(faction_id, i, DIPLO_VENDETTA)
                    && plr.diplo_wrongs[i] > most_wrongs) {
                        wronged_id = i;
                        most_wrongs = plr.diplo_wrongs[i];
                    }
                }
                if (wronged_id < 0) {
                    mention = false;
                } else {
                    parse_says(0, get_title(wronged_id), -1, -1);
                    parse_says(1, get_name(wronged_id), -1, -1);
                    parse_says(2, get_noun(wronged_id), -1, -1);
                    parse_says(3, get_him_her(wronged_id, 0), -1, -1);
                    parse_says(4, get_title(faction_id), -1, -1);
                    parse_says(5, get_name(faction_id), -1, -1);
                    plr.diplo_wrongs[wronged_id] = 0;
                    clear();
                    says("WRONGED");
                    if (has_treaty(faction_id_2, wronged_id, DIPLO_VENDETTA)) {
                        says("ENEMY");
                    } else if (cmp.mil_strength_1 <= plr.mil_strength_1) {
                        says("MEEK");
                    } else {
                        says("MIGHTY");
                    }
                    X_dialog(StrBuffer, faction_id_2);
                }
            }
        } else if (!refuse_demand || deal_done) {
            if (flag) {
                if (cmp.diplo_patience[faction_id] <= 1) {
                    parse_says(0, get_title(faction_id), -1, -1);
                    parse_says(1, get_name(faction_id), -1, -1);
                    if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
                        X_dialog("WHATSUPPACT", faction_id_2);
                    } else if (has_treaty(faction_id_2, faction_id, DIPLO_TREATY)) {
                        X_dialog("WHATSUPTREATY", faction_id_2);
                    } else {
                        X_dialog("WHATSUP", faction_id_2);
                    }
                } else if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
                    parse_says(0, get_pact(faction_id), -1, -1);
                    X_dialog("LOWPATIENCEPACT", faction_id_2);
                } else {
                    parse_says(0, get_title(faction_id), -1, -1);
                    X_dialog("LOWPATIENCE", faction_id_2);
                }
                mention = false;
            } else {
                parse_says(0, get_pact_hood(faction_id, faction_id_2), -1, -1);
                parse_says(1, get_title(faction_id), -1, -1);
                parse_says(2, get_name(faction_id), -1, -1);
                parse_says(3, get_noun_phrase(faction_id_2, 9), -1, -1);
                if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
                    X_dialog("HELLOPACT", faction_id_2);
                    mention = false;
                } else if (has_treaty(faction_id_2, faction_id, DIPLO_TREATY)) {
                    X_dialog("HELLOTREATY", faction_id_2);
                    mention = false;
                } else {
                    X_dialog("HELLO", faction_id_2);
                }
            }
        } else {
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            parse_says(2, get_pacts_2(faction_id, faction_id_2), -1, -1);
            parse_says(3, get_pact(faction_id), -1, -1);
            if (soft_demand) {
                if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
                    X_dialog("MILDPACT", faction_id_2);
                    mention = false;
                } else if (has_treaty(faction_id, faction_id_2, DIPLO_TREATY)) {
                    X_dialog("MILDTREATY", faction_id_2);
                    mention = false;
                } else {
                    X_dialog("MILD", faction_id_2);
                }
            } else if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
                X_dialog("REBUFFEDPACT", faction_id_2);
                mention = false;
            } else if (has_treaty(faction_id, faction_id_2, DIPLO_TRUCE|DIPLO_TREATY)) {
                X_dialog("REBUFFEDTREATY", faction_id_2);
            } else {
                X_dialog("REBUFFED", faction_id_2);
            }
        }
        if (mention) {
            mention_prototypes(faction_id, faction_id_2);
        }
    }

    *DiploSocConflict = 0;
    {
        int category = MFactions[faction_id_2].soc_priority_category;
        int model = MFactions[faction_id_2].soc_priority_model;
        if (category >= 0 && model) {
            parse_says(0, get_title(faction_id), -1, -1);
            parse_says(1, get_name(faction_id), -1, -1);
            clear();
            says("SOCIAL");
            say_num(category);
            says("CAT");
            int current = ((CSocialCategory*)&plr.SE_Politics_pending)->models[category];
            if (current == model) {
                say_num(model);
                says("GOOD");
                X_dialog(StrBuffer, faction_id_2);
            } else if (current) {
                *DiploSocConflict = 1;
                if (!*DiploFearScale
                && (!has_treaty(faction_id_2, faction_id, DIPLO_PACT)
                || !has_treaty(faction_id_2, faction_id, DIPLO_HAVE_SURRENDERED))) {
                    say_num(current);
                    says("BAD");
                    say_num(model);
                    X_dialog(StrBuffer, faction_id_2);
                    net_set_agenda(faction_id_2, faction_id, AGENDA_UNK_1, 1, 1);
                }
            }
        }
    }
    if (!*DiploSocConflict) {
        net_set_agenda(faction_id_2, faction_id, AGENDA_UNK_1, 0, 1);
    }
    if (plr.atrocities
    && (!has_treaty(faction_id_2, faction_id, DIPLO_UNK_200000)
    || has_treaty(faction_id_2, faction_id, DIPLO_UNK_400000))
    && plr.atrocities > cmp.AI_fight + 1
    && !MFactions[faction_id_2].is_alien()
    && (!has_treaty(faction_id_2, faction_id, DIPLO_PACT)
    || !has_treaty(faction_id_2, faction_id, DIPLO_HAVE_SURRENDERED))) {
        int victim_id = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && i != faction_id_2
            && has_treaty(i, faction_id, DIPLO_ATROCITY_VICTIM)) {
                victim_id = i;
                break;
            }
        }
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        if (victim_id) {
            parse_says(2, get_noun(victim_id), -1, -1);
            X_dialog("COMMITTEDATROCITY", faction_id_2);
        } else {
            X_dialog("SELFATROCITY", faction_id_2);
        }
        if (!has_treaty(faction_id_2, faction_id, DIPLO_UNK_200000) && plr.atrocities > 1) {
            plr.atrocities--;
            if (plr.integrity_blemishes > 1) {
                plr.integrity_blemishes--;
            }
        }
        net_set_treaty(faction_id_2, faction_id, DIPLO_UNK_200000, 1, 0);
        net_set_treaty(faction_id_2, faction_id, DIPLO_UNK_400000, 0, 0);
    }
    if (has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
        parse_says(0, get_pact(faction_id), -1, -1);
        net_maps(faction_id, faction_id_2, 1);
        if (*DiploMapMsg) {
            X_dialog("HERESTHEMAP", faction_id_2);
        }
    }
    if (!((!has_treaty(faction_id, faction_id_2, DIPLO_PACT)
    && !has_treaty(faction_id_2, faction_id, DIPLO_UNK_80))
    || (diplo_state & DST_UNK_40))) {
        int strongest_id = 0;
        int strongest_strength = 0;
        int target_id = 0;
        int target_strength = 9999;
        int other_id = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (is_alive(i) && i != faction_id && i != faction_id_2
            && has_treaty(faction_id_2, i, DIPLO_VENDETTA)
            && has_treaty(faction_id_2, i, DIPLO_COMMLINK)
            && has_treaty(faction_id_2, i, DIPLO_UNK_8000)
            && has_treaty(faction_id, i, DIPLO_COMMLINK)
            && (has_treaty(faction_id_2, i, DIPLO_UNK_40)
            || has_treaty(faction_id_2, faction_id, DIPLO_PACT))) {
                int strength = Factions[i].mil_strength_1;
                if (strength > strongest_strength) {
                    strongest_strength = strength;
                    strongest_id = i;
                }
                bool outmatched = false;
                if (strength < target_strength) {
                    for (int region = 1; region < MaxRegionLandNum; region++) {
                        if (!bad_reg(region)
                        && cmp.region_total_bases[region]
                        && Factions[i].region_total_bases[region]
                        && Factions[i].region_force_rating[region]
                        <= cmp.region_force_rating[region] + cmp.region_total_combat_units[region]
                        && contiguous(faction_id_2, i, region, 0)) {
                            outmatched = true;
                            break;
                        }
                    }
                }
                if (outmatched) {
                    target_id = i;
                    target_strength = strength;
                } else if (target_id != i && strongest_id != i) {
                    other_id = i;
                }
            }
        }
        bool reported = false;
        if (strongest_id) {
            bool contested = false;
            for (int region = 1; region < MaxRegionNum; region++) {
                if (!bad_reg(region)
                && (DiploRegionFactions[region] & (1 << faction_id_2))
                && Factions[strongest_id].region_force_rating[region]
                > cmp.region_total_combat_units[region]) {
                    contested = true;
                    break;
                }
            }
            if (contested || has_treaty(faction_id, faction_id_2, DIPLO_WANT_TO_TALK)) {
                parse_says(1, get_title(strongest_id), -1, -1);
                parse_says(2, get_name(strongest_id), -1, -1);
                parse_says(3, get_noun(strongest_id), -1, -1);
                parse_says(4, get_his_her(strongest_id, 0), -1, -1);
                parse_says(5, get_adjective_insult_faction(strongest_id), -1, -1);
                X_dialog("REPORTLOSING", faction_id_2);
                reported = true;
            }
        }
        if (!reported && target_id && !has_treaty(faction_id, faction_id_2, DIPLO_WANT_TO_TALK)) {
            for (int region = 1; region < MaxRegionNum; region++) {
                // Fix: corrected DiploRegionFactions bitmask check
                if (!bad_reg(region)
                && (DiploRegionFactions[region] & (1 << target_id))
                && cmp.region_force_rating[region]
                > Factions[target_id].region_total_combat_units[region]) {
                    parse_says(0, get_adjective(target_id), -1, -1);
                    parse_says(1, get_title(target_id), -1, -1);
                    parse_says(2, get_name(target_id), -1, -1);
                    parse_says(3, get_phrase(target_id, 5), -1, -1);
                    X_dialog("REPORTWINNING", faction_id_2);
                    reported = true;
                    break;
                }
            }
        }
        if (!reported && other_id) {
            for (int region = 1; region < MaxRegionNum; region++) {
                if (!bad_reg(region)
                && Factions[other_id].region_total_bases[region]
                && cmp.region_total_bases[region]) {
                    parse_says(1, get_title(other_id), -1, -1);
                    parse_says(2, get_name(other_id), -1, -1);
                    parse_says(3, get_noun(other_id), -1, -1);
                    parse_says(4, get_adjective(other_id), -1, -1);
                    parse_says(5, get_noun_phrase(other_id, 8), -1, -1);
                    X_dialog("REPORTEVEN", faction_id_2);
                    break;
                }
            }
        }
    }
    if (has_treaty(faction_id, faction_id_2, DIPLO_PACT)) {
        mention_prototypes(faction_id, faction_id_2);
        if (!(*GameRules & RULES_BLIND_RESEARCH)) {
            int research_id = cmp.tech_research_id;
            if (research_id >= 0 && !has_tech(research_id, faction_id)
            && cmp.tech_research_id != plr.tech_research_id
            && !(*GameRules & (RULES_SCN_NO_TECH_ADVANCES|RULES_SCN_NO_TECH_TRADING))
            && plr.tech_accumulated >= mod_tech_rate(faction_id) / 2) {
                int best_tech = -1;
                int best_value = 0;
                for (int i = 0; i < MaxTechnologyNum; i++) {
                    if (!has_tech(i, faction_id)
                    && i != plr.tech_research_id
                    && i != cmp.tech_research_id
                    && tech_avail(i, faction_id)
                    && (i - plr.tech_ranking + faction_id + 2) % 3 != 0
                    && has_tech(Tech[i].preq_tech1, faction_id)
                    && has_tech(Tech[i].preq_tech2, faction_id)) {
                        int value = mod_tech_val(i, faction_id_2, 0);
                        if (value > best_value) {
                            best_value = value;
                            best_tech = i;
                        }
                    }
                }
                if (best_tech >= 0) {
                    uint32_t offset;
                    uint32_t mask;
                    bitmask(research_id, &offset, &mask);
                    cmp.tech_pact_shared_goals[offset] |= mask;
                    parse_says(0, Tech[research_id].name, -1, -1);
                    bitmask(best_tech, &offset, &mask);
                    plr.tech_pact_shared_goals[offset] |= mask;
                    parse_says(1, Tech[best_tech].name, -1, -1);
                    X_dialog("TEAMTECH", faction_id_2);
                }
            }
        }
    }
    if (!has_treaty(faction_id_2, faction_id, DIPLO_TREATY|DIPLO_PACT)
    && territory(faction_id_2, faction_id, 0, &base_id, 0)) {
        spot_base(base_id, faction_id);
        mod_MapWin_focus(MapWinPtr[0], Bases[base_id].x, Bases[base_id].y);
        parse_says(1, get_adjective(faction_id_2), -1, -1);
        X_dialog("TERRITORY", faction_id_2);
        mention_prototypes(faction_id, faction_id_2);
    }
    if (*DiploEnemyAllyCount) {
        parse_says(0, get_title(*DiploEnemyAlly), -1, -1);
        parse_says(1, get_name(*DiploEnemyAlly), -1, -1);
        parse_says(2, get_adjective_insult_leader(*DiploEnemyAlly), -1, -1);
        X_dialog("QUISLING", faction_id_2);
    }
    *DiploMenuFlagA = 0;
    *DiploMenuFlagB = 0;
    {
        int8_t* patience = &cmp.diplo_patience[faction_id];
        while (true) {
            if (*patience < 0) {
                *patience = 0;
            }
            *DiploCommonEnemy = -1;
            *DiploPeaceFaction = -1;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (is_alive(i) && i != faction_id && i != faction_id_2
                && has_treaty(faction_id_2, i, DIPLO_VENDETTA)
                && has_treaty(faction_id, i, DIPLO_COMMLINK)) {
                    if (has_treaty(faction_id, i, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
                        *DiploPeaceFaction = i;
                    }
                    if (has_treaty(faction_id, i, DIPLO_VENDETTA)) {
                        *DiploCommonEnemy = i;
                    }
                }
            }
            if (*MultiplayerActive && (*GameState & STATE_UNK_800)) {
                break;
            }
            int choice = diplomacy_menu(faction_id, faction_id_2);
            if (choice == 0) {
                goto TALK_END;
            }
            switch (choice) {
                case 1:
                case 2:
                    make_a_proposal(faction_id, faction_id_2, choice == 1);
                    if (*DiploCounterProposal == DiploCounterLoanPayment) {
                        *DiploMenuFlagA = 0;
                    }
                    break;
                case 3:
                    if (X_dialog("REALLYBROKE", faction_id_2)) {
                        diplo_lock(0);
                        ++plr.integrity_blemishes;
                        net_treaty_off(faction_id_2, faction_id, DIPLO_PACT, 1);
                        diplomacy_check(faction_id, faction_id_2, 0);
                        diplomacy_caption(faction_id, faction_id_2);
                        parse_says(0, get_pact(faction_id), -1, -1);
                        parse_says(1, get_title(faction_id), -1, -1);
                        parse_says(2, get_name(faction_id), -1, -1);
                        X_dialog("BROKEPACT", faction_id_2);
                        net_pact_ends(faction_id, faction_id_2, 1);
                        diplo_unlock();
                    } else {
                        parse_says(0, get_pact_hood(faction_id, faction_id_2), -1, -1);
                        X_dialog("DROIDS", faction_id_2);
                        --*patience;
                    }
                    break;
                case 4:
                    demand_withdrawal(faction_id, faction_id_2);
                    break;
                case 5:
                    battle_plans(faction_id, faction_id_2);
                    break;
                case 6:
                    call_off_vendetta(faction_id, faction_id_2);
                    break;
                case 7:
                    *DiploCurrentProposal = DiploProposalCouncilVote;
                    *DiploCounterProposal = DiploCounterFriendship;
                    make_a_proposal(faction_id, faction_id_2, 1);
                    *DiploMenuFlagA = 0;
                    break;
                default:
                    break;
            }
            ++*patience;
            if (*patience >= get_patience(faction_id, faction_id_2)) {
                break;
            }
        }
    }
    diplomacy_caption(faction_id, faction_id_2);
    if (!has_treaty(faction_id, faction_id_2, DIPLO_VENDETTA)) {
        parse_says(0, get_title(faction_id), -1, -1);
        parse_says(1, get_name(faction_id), -1, -1);
        parse_says(2, get_name(faction_id_2), -1, -1);
        if (has_treaty(faction_id_2, faction_id, DIPLO_PACT)) {
            X_dialog("TIMESUPALLY", faction_id_2);
        } else {
            X_dialog("TIMESUP", faction_id_2);
        }
    }

TALK_END:
    net_treaty_off(faction_id_2, faction_id, DIPLO_WANT_TO_TALK, 0);
    net_agenda_off(faction_id_2, faction_id, AGENDA_UNK_4, 0);
    net_set_agenda(faction_id_2, faction_id, AGENDA_UNK_10,
        *DiploAttackChance && !has_treaty(faction_id, faction_id_2, DIPLO_VENDETTA|DIPLO_PACT), 0);
    net_set_treaty(faction_id_2, faction_id, DIPLO_SHALL_BETRAY,
        *DiploCanBetray && !has_treaty(faction_id, faction_id_2, DIPLO_VENDETTA|DIPLO_PACT), 1);
    if (tribute_paid || (*DiploHostility && !has_treaty(faction_id, faction_id_2, DIPLO_VENDETTA))) {
        cmp.diplo_patience[faction_id] = range(cmp.diplo_patience[faction_id],
            get_patience(faction_id, faction_id_2), 999);
    }
    diplomacy_ends(faction_id, faction_id_2);
    if (has_truce
    && !has_treaty(faction_id, faction_id_2, DIPLO_UNK_10000|DIPLO_VENDETTA|DIPLO_TREATY|DIPLO_PACT)
    && !flag) {
        parse_says(0, get_title(faction_id_2), -1, -1);
        parse_says(1, get_name(faction_id_2), -1, -1);
        parse_says(2, get_noun(faction_id_2), -1, -1);
        if (has_treaty(faction_id, faction_id_2, DIPLO_TRUCE)) {
            X_dialog("RENEWTRUCE", faction_id_2);
        } else {
            X_dialog("ENDTRUCE", faction_id_2);
        }
    }
}

