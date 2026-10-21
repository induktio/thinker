
#include "faction_diplo.h"

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
            int current_model = (&plr.SE_Politics_pending)[soc_category];
            if (current_model && current_model != soc_model) {
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

    int max_region_bases[7];
    for (int i = 0; i < 7; i++) {
        max_region_bases[i] = 1;
    }
    memset(&DiploRegionFactions[1], 0, 127 * sizeof(int32_t));
    for (int region = 1; region < MaxRegionNum; region++) {
        for (int i = 1; i < 8; i++) {
            int bases = Factions[i].region_total_bases[region];
            if (bases > max_region_bases[i - 1]) {
                max_region_bases[i - 1] = bases;
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
            if (Factions[i].region_total_bases[region] >= max_region_bases[i - 1] - max_region_bases[i - 1] / 4) {
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
    if (*DiploExposedBaseCount && !(plr.diplo_status[faction_id_2] & (DIPLO_TRUCE | DIPLO_TREATY | DIPLO_PACT))) {
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
    if (their_status & (DIPLO_WANT_REVENGE | DIPLO_VENDETTA)) {
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
    && !(plr.diplo_status[faction_id_2] & (DIPLO_ATROCITY_VICTIM | DIPLO_WANT_REVENGE))) {
        *DiploTributeDemand = 25 * ((energy_credits + 24) / 25);
    }
    if (!(their_status & (DIPLO_UNK_40 | DIPLO_WANT_REVENGE))
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
    if ((((their_status & (DIPLO_TRUCE | DIPLO_TREATY | DIPLO_PACT)) && !(their_status & DIPLO_UNK_10000))
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
            if (DiploStateA[faction_id_2] & 0x40) {
                if ((plr.diplo_status[i] & (DIPLO_COMMLINK | DIPLO_VENDETTA)) == DIPLO_COMMLINK && alt_mil > cmp_mil) {
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
    && !(their_status & (DIPLO_UNK_40 | DIPLO_PACT))
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

