
#include "veh_combat.h"


static void reset_veh_draw() {
    *VehDrawAttackID = -1;
    *VehDrawDefendID = -1;
}

static int net_pop(const char* label, const char* filename) {
    return NetMsg_pop(NetMsg, label, 5000, 0, filename);
}

static int combat_rand(int value) {
    return (value > 1 ? game_rand() % value : 0);
}

static int stack_kill_check(int veh_id) {
    return mod_stack_check(veh_id, 1, -1, -1, -1)
        - mod_stack_check(veh_id, 2, PLAN_COLONY, -1, -1)
        - mod_stack_check(veh_id, 2, PLAN_SUPPLY, -1, -1)
        - mod_stack_check(veh_id, 2, PLAN_PROBE, -1, -1)
        - mod_stack_check(veh_id, 2, PLAN_ARTIFACT, -1, -1);
}

static void combat_odds_fix(VEH* veh_atk, VEH* veh_def, int* attack_odds, int* defend_odds) {
    if (conf.ignore_reactor_power // psi combat odds are already correct
    && veh_atk->offense_value() >= 0 && veh_def->defense_value() >= 0) {
        // calculate attacker and defender power
        int attack_power = veh_atk->cur_hitpoints();
        int defend_power = veh_def->cur_hitpoints();
        // calculate firepower
        int attack_fp = veh_def->reactor_type();
        int defend_fp = veh_atk->reactor_type();
        // calculate hitpoints
        int attack_hp = (attack_power + (defend_fp - 1)) / defend_fp;
        int defend_hp = (defend_power + (attack_fp - 1)) / attack_fp;
        *attack_odds = *attack_odds * attack_hp * defend_power;
        *defend_odds = *defend_odds * defend_hp * attack_power;
        int divisor = std::__gcd(*attack_odds, *defend_odds);
        *attack_odds /= divisor;
        *defend_odds /= divisor;
    }
}

/*
Calculate the psi combat factor for an attacking or defending unit.
*/
int __cdecl psi_factor(int value, int faction_id, int is_attack, int is_fungal_twr) {
    int rule_psi = MFactions[faction_id].rule_psi;
    if (rule_psi) {
        value = ((rule_psi + 100) * value) / 100;
    }
    if (is_attack) {
        if (has_project(FAC_DREAM_TWISTER, faction_id)) {
            value += value * conf.dream_twister_bonus / 100;
        }
    } else {
        if (has_project(FAC_NEURAL_AMPLIFIER, faction_id)) {
            value += value * conf.neural_amplifier_bonus / 100;
        }
        if (is_fungal_twr) {
            value += value * conf.fungal_tower_bonus / 100;
        }
    }
    return value;
}

/*
Modify planetpearls income after wiping out any planet-owned units.
*/
int __cdecl battle_kill_credits(int veh_id) {
    if (conf.planetpearls > 1) { // Original value
        return 10 * (mod_morale_alien(veh_id, 0) + 1);
    } else if (conf.planetpearls == 1) {
        return 10 + 5 * mod_morale_alien(veh_id, 0);
    } else {
        return 0;
    }
}

/*
Generate an output string for the specified unit's morale.
Contains rewrites to update Children's Creche, Brood Pit and Headquarters effects.
*/
void __cdecl mod_say_morale2(char* output, int veh_id, int faction_id_vs_native) {
    VEH* veh = &Vehs[veh_id];
    // Drone riot effect is displayed separately by ignoring it in morale_veh
    int morale = mod_morale_veh(veh_id, false, faction_id_vs_native);
    int faction_id = Vehs[veh_id].faction_id;
    bool native_unit = veh->is_native_unit();
    output[0] = '\0';
    if (native_unit) {
        strncat(output, Morale[morale].name_lifecycle, StrBufLen);
    } else {
        strncat(output, Morale[morale].name, StrBufLen);
    }
    if (veh->plan() < PLAN_COLONY) {
        int morale_modifier = 0;
        int morale_penalty = !native_unit && morale > 0 && veh->home_base_id >= 0
            && Bases[veh->home_base_id].state_flags & BSTATE_DRONE_RIOTS_ACTIVE
            && !(MFactions[faction_id].rule_flags & RFLAG_MORALE);
        int base_id = base_at(Vehs[veh_id].x, Vehs[veh_id].y);
        if (base_id >= 0 && morale < MORALE_ELITE) {
            bool has_creche = has_fac_built(FAC_CHILDREN_CRECHE, base_id);
            bool has_brood_pit = native_unit && has_fac_built(FAC_BROOD_PIT, base_id);
            if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
                morale_modifier++;
            }
            if (has_creche || has_brood_pit) {
                morale_modifier++;
                int morale_active = clamp(Factions[faction_id].SE_morale, -4, 4);
                if (morale_active <= -2) {
                    morale_active++;
                }
                int morale_cap = morale + morale_modifier;
                while (morale_active < 0 && morale_cap < MORALE_ELITE) {
                    morale_cap++;
                    morale_active++;
                    morale_modifier++;
                }
            }
        }
        int morale_pending = Factions[faction_id].SE_morale_pending;
        if (!native_unit && (morale_pending == 2 || morale_pending == 3)) {
            morale_modifier++;
        }
        if (!morale && !morale_modifier) {
            morale_modifier = 1;
        }
        bool flag = false;
        if (morale_penalty) {
            strncat(output, " (-)", StrBufLen);
            flag = true;
        }
        if (morale_modifier) {
            strncat(output, flag ? "(" : " (", StrBufLen);
            for (int i = 0; i < morale_modifier; i++) {
                strncat(output, "+", StrBufLen);
            }
            strncat(output, ")", StrBufLen);
            flag = true;
        }
        if (Vehs[veh_id].state & VSTATE_DESIGNATE_DEFENDER) {
            strncat(output, flag ? "(d)" : " (d)", StrBufLen);
        }
    }
}

/*
Calculate the lifecycle (morale) of the specified planet-owned native unit.
*/
int __cdecl mod_morale_alien(int veh_id, int faction_id_vs_native) {
    int morale;
    // Fungal Tower specific code, shifted to start and added veh_id check to prevent crash
    if (veh_id >= 0 && Vehs[veh_id].unit_id == BSC_FUNGAL_TOWER) {
        VEH* veh = &Vehs[veh_id];
        morale = 0;
        // similar to is_coast() > except with fungus check + Ocean Shelf included
        for (int i = 1; i < 9; i++) {
            int x2 = wrap(veh->x + TableOffsetX[i]);
            int y2 = veh->y + TableOffsetY[i];
            MAP* sq = mapsq(x2, y2);
            if (sq && sq->is_fungus()) {
                morale++;
            }
        }
        morale -= 2;
    } else { // everything else
        if (*CurrentTurn < conf.native_lifecycle_levels[0]) {
            morale = 0;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[1]) {
            morale = 1;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[2]) {
            morale = 2;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[3]) {
            morale = 3;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[4]) {
            morale = 4;
        } else if (*CurrentTurn < conf.native_lifecycle_levels[5]) {
            morale = 5;
        } else {
            morale = 6;
        }
        if (faction_id_vs_native > 0) {
            morale += (Factions[faction_id_vs_native].major_atrocities != 0)
                + (TectonicDetonationCount[faction_id_vs_native] != 0);
        }
        if (veh_id >= 0) {
            if (Vehs[veh_id].state & VSTATE_MONOLITH_UPGRADED) {
                morale++;
            }
            if (Vehs[veh_id].unit_id == BSC_LOCUSTS_OF_CHIRON) {
                morale++;
            }
            morale += Vehs[veh_id].lifecycle_value();
        }
    }
    morale = clamp(morale, 0, 6);
    return morale;
}

/*
Calculate unit morale. TODO: Determine if 2nd param is a toggle for display vs actual morale.
*/
int __cdecl mod_morale_veh(int veh_id, int check_drone_riot, int faction_id_vs_native) {
    VEH* veh = &Vehs[veh_id];
    int faction_id = veh->faction_id;
    int home_base_id = veh->home_base_id;
    int value;
    if (!faction_id) {
        return mod_morale_alien(veh_id, faction_id_vs_native);
    }
    if (veh->plan() == PLAN_PROBE) {
        int probe_morale = clamp(Factions[faction_id].SE_probe, 0, 3);
        probe_morale += has_project(FAC_TELEPATHIC_MATRIX, faction_id) ? 2 : 0;
        for (int i = 0; i < MaxTechnologyNum; i++) {
            if (Tech[i].flags & TFLAG_IMPROVE_PROBE && has_tech(i, faction_id)) {
                probe_morale++;
            }
        }
        probe_morale += veh->morale;
        value = clamp(probe_morale, 2, 6);
        return value;
    }
    if (veh->unit_id < MaxProtoFactionNum && veh->offense_value() < 0) {
        value = clamp((int)veh->morale, 0, 6); // Basic Psi Veh
        return value;
    }
    // everything else
    int morale_modifier = clamp(Factions[faction_id].SE_morale, -4, 4);
    if (morale_modifier <= -2) {
        morale_modifier++;
    } else if (morale_modifier >= 2) {
        morale_modifier--;
    }
    int rule_morale = MFactions[faction_id].rule_morale; // different from 'SOCIAL, MORALE'
    if (rule_morale < 0) { // negative effects 1st
        morale_modifier += rule_morale;
    }
    /*
    Fix: Removed checks for Children's Creche and Brood Pit in home_base_id.
    CC is not supposed to affect units outside base tile according to the manual.
    Brood Pit implementation is bugged in the game engine and the original check
    for BP in this location is never reached due to above 'Basic Psi Veh' checks.
    */
    if (rule_morale > 0) {
        morale_modifier += rule_morale;
    }
    bool morale_flag = MFactions[faction_id].rule_flags & RFLAG_MORALE;
    if (morale_flag && morale_modifier < 0) {
        morale_modifier = 0;
    }
    if (check_drone_riot && home_base_id >= 0
    && Bases[home_base_id].state_flags & BSTATE_DRONE_RIOTS_ACTIVE && !morale_flag) {
        // Fix: removed premature range bounding negating negative morale effects
        morale_modifier--;
    }
    value = clamp(veh->morale + morale_modifier, 0, 6);
    return value;
}

/*
Calculate the offense of the specified prototype. Optional param of the unit defending
against (-1 to ignore) as well as whether artillery or missile combat is being utilized.
*/
int __cdecl mod_offense_proto(int unit_id, int veh_id_def, int is_bombard) {
    UNIT* unit = &Units[unit_id];
    int value;
    if (unit->weapon_mode() == WMODE_PROBE
    && veh_id_def >= 0 && Vehs[veh_id_def].plan() == PLAN_PROBE) {
        value = 16; // probe attacking another probe
    // Fix: veh_id_def -1 could cause out of bounds memory read on reactor struct
    // due to lack of bounds checking when comparing veh_id_def unit_id to Spore Launcher
    } else if ((is_bombard || (unit->offense_value() >= 0
    && (veh_id_def < 0 || Vehs[veh_id_def].defense_value() >= 0)))
    && (veh_id_def < 0 || Vehs[veh_id_def].unit_id != BSC_SPORE_LAUNCHER)
    && unit_id != BSC_SPORE_LAUNCHER) {
        int off_rating = abs(unit->offense_value());
        if (unit->is_missile() && off_rating < 99) {
            off_rating = (off_rating * 3) / 2;
        }
        value = (veh_id_def < 0) ? off_rating : off_rating * 8; // conventional
    } else {
        value = (veh_id_def < 0) ? Rules->psi_combat_ratio_atk[TRIAD_LAND] : // PSI
           Rules->psi_combat_ratio_atk[Vehs[veh_id_def].triad()] * 8;
    }
    assert(value == offense_proto(unit_id, veh_id_def, is_bombard));
    return value;
}

/*
Calculate the defense of the specified prototype. Optional param if unit is being attacked
(-1 to ignore) as well as whether artillery or missile combat is being utilized.
*/
int __cdecl mod_armor_proto(int unit_id, int veh_id_atk, int is_bombard) {
    UNIT* unit = &Units[unit_id];
    int value;
    if (unit->weapon_mode() == WMODE_PROBE
    && veh_id_atk >= 0 && Vehs[veh_id_atk].plan() == PLAN_PROBE) {
        value = 16; // probe defending against another probe
    // Fix: veh_id_def -1 could cause out of bounds memory read on reactor struct
    // due to lack of bounds checking when comparing veh_id_def unit_id to Spore Launcher
    } else if ((is_bombard && unit_id != BSC_SPORE_LAUNCHER
    && (veh_id_atk < 0 || Vehs[veh_id_atk].unit_id != BSC_SPORE_LAUNCHER))
    || (unit->defense_value() >= 0 && (veh_id_atk < 0 || Vehs[veh_id_atk].offense_value() >= 0))) {
        int def_rating = clamp(unit->defense_value(), 1, 9999);
        value = (veh_id_atk < 0) ? def_rating : def_rating * 8; // conventional
    } else {
        value = (veh_id_atk < 0) ? Rules->psi_combat_ratio_def[TRIAD_LAND] : // PSI
            Rules->psi_combat_ratio_def[unit->triad()] * 8;
    }
    assert(value == armor_proto(unit_id, veh_id_atk, is_bombard));
    return value;
}

/*
Get the basic offense value for an attacking unit with an optional defender unit parameter.
*/
int __cdecl mod_get_basic_offense(int veh_id_atk, int veh_id_def, int psi_combat_type, int is_bombard, int unk_tgl) {
    // unk_tgl is only set to true in battle_compute when veh_id_atk and veh_id_def are switched places.
    int morale;
    int faction_id_atk = Vehs[veh_id_atk].faction_id;
    int unit_id_atk = Vehs[veh_id_atk].unit_id;
    bool native_unit = Vehs[veh_id_atk].is_native_unit();
    if (faction_id_atk) {
        morale = mod_morale_veh(veh_id_atk, true, 0);
    } else {
        morale = mod_morale_alien(veh_id_atk, veh_id_def >= 0 ? Vehs[veh_id_def].faction_id : -1);
    }
    int base_id_atk = base_at(Vehs[veh_id_atk].x, Vehs[veh_id_atk].y);
    if (base_id_atk >= 0) {
        // Morale effects with CC/BP are modified. SE Morale has no longer effect on native units.
        if (has_fac_built(FAC_CHILDREN_CRECHE, base_id_atk)) {
            morale++;
            int morale_active = clamp(Factions[faction_id_atk].SE_morale, -4, 0);
            if (morale_active <= -2) {
                morale_active++;
            }
            if (!native_unit) {
                morale -= morale_active;
            }
        } else if (native_unit && has_fac_built(FAC_BROOD_PIT, base_id_atk)) {
            morale++;
        }
        if (unk_tgl) {
            int morale_pending = Factions[faction_id_atk].SE_morale_pending;
            if (!native_unit && morale_pending >= 2 && morale_pending <= 3) {
                morale++;
            }
            if (veh_id_def >= 0) {
                if (Vehs[veh_id_def].faction_id) {
                    if (!native_unit && !has_abil(unit_id_atk, ABL_DISSOCIATIVE_WAVE)
                    && has_abil(Vehs[veh_id_def].unit_id, ABL_SOPORIFIC_GAS)) {
                        morale -= 2;
                    }
                } else {
                    morale++;
                }
            }
        }
    }
    if (unk_tgl) {
        morale = clamp(morale, 1, 6);
    }
    VehBasicBattleMorale[unk_tgl != 0] = morale; // shifted up from original
    morale += 6;
    int offense = mod_offense_proto(unit_id_atk, veh_id_def, is_bombard);
    if (psi_combat_type) {
        offense = psi_factor(offense, faction_id_atk, true, false);
    }
    offense = offense * morale * 4;
    return offense;
}

/*
Get the basic defense value for a defending unit with an optional attacker unit parameter.
*/
int __cdecl mod_get_basic_defense(int veh_id_def, int veh_id_atk, int psi_combat_type, int is_bombard) {
    int morale;
    int faction_id_def = Vehs[veh_id_def].faction_id;
    int unit_id_def = Vehs[veh_id_def].unit_id;
    int base_id_def = base_at(Vehs[veh_id_def].x, Vehs[veh_id_def].y);
    bool native_unit = Vehs[veh_id_def].is_native_unit();
    if (faction_id_def) {
        morale = mod_morale_veh(veh_id_def, true, 0);
    } else {
        morale = mod_morale_alien(veh_id_def, veh_id_atk >= 0 ? Vehs[veh_id_atk].faction_id : -1);
    }
    if (base_id_def >= 0) {
        // Morale effects with CC/BP are modified. SE Morale has no longer effect on native units.
        if (has_fac_built(FAC_CHILDREN_CRECHE, base_id_def)) {
            morale++;
            int morale_active = clamp(Factions[faction_id_def].SE_morale, -4, 0);
            if (morale_active <= -2) {
                morale_active++;
            }
            if (!native_unit) {
                morale -= morale_active;
            }
        } else if (native_unit && has_fac_built(FAC_BROOD_PIT, base_id_def)) {
            morale++;
        }
        // Fix: manual has "Units in a headquarters base automatically gain +1 Morale when defending."
        if (has_fac_built(FAC_HEADQUARTERS, base_id_def)) {
            morale++;
        }
        int morale_pending = Factions[faction_id_def].SE_morale_pending;
        if (!native_unit && morale_pending >= 2 && morale_pending <= 3) {
            morale++;
        }
        if (veh_id_atk >= 0 && !Vehs[veh_id_atk].faction_id) {
            morale++;
        }
    }
    if (veh_id_atk >= 0 && Vehs[veh_id_atk].faction_id
    && has_abil(Vehs[veh_id_atk].unit_id, ABL_SOPORIFIC_GAS)
    && !native_unit && !has_abil(unit_id_def, ABL_DISSOCIATIVE_WAVE)) {
        morale -= 2;
    }
    morale = clamp(morale, 1, 6);
    VehBasicBattleMorale[1] = morale;
    morale += 6;
    int plan_def = Units[unit_id_def].plan;
    if (plan_def == PLAN_ARTIFACT) {
        return 1;
    }
    // Fix: added veh_id_atk bounds check to prevent potential read outside array
    if (plan_def == PLAN_PROBE && Units[unit_id_def].defense_value() == 1
    && (veh_id_atk < 0 || Units[Vehs[veh_id_atk].unit_id].plan != PLAN_PROBE)) {
        return 1;
    }
    int defense = mod_armor_proto(unit_id_def, veh_id_atk, is_bombard);
    if (psi_combat_type) {
        defense = psi_factor(defense, faction_id_def, false, unit_id_def == BSC_FUNGAL_TOWER);
    }
    defense *= morale;
    return defense;
}

/*
Determine the best defender in a stack for a given attacker.
Value comparisons start with INT_MIN to enable more generic code.
*/
static int __cdecl find_defender(int veh_id_def, int veh_id_atk, int check_arty) {
    assert(veh_id_atk >= 0 && veh_id_atk < *VehCount);
    assert(veh_id_def >= 0 && veh_id_def < *VehCount);
    VEH* veh_def = &Vehs[veh_id_def];
    VEH* veh_atk = &Vehs[veh_id_atk];
    int offense_val;
    int best_veh_id = veh_id_def;
    int best_score = INT_MIN;

    if (veh_id_def < 0) {
        return veh_id_def;
    }
    if (veh_id_atk < 0) {
        offense_val = 8;
    } else {
        offense_val = mod_get_basic_offense(veh_id_atk, veh_id_def, 0, 0, 0);
    }
    MAP* sq = mapsq(veh_def->x, veh_def->y);
    int base_id = base_at(veh_def->x, veh_def->y);
    bool at_ocean = base_id < 0 && is_ocean(sq);
    bool is_airbase = sq && sq->items & BIT_AIRBASE;

    for (int veh_id = veh_top(veh_id_def); veh_id >= 0; veh_id = Vehs[veh_id].next_veh_id_stack) {
        VEH* veh = &Vehs[veh_id];
        if (veh->triad() == TRIAD_LAND && at_ocean) {
            continue;
        }
        // Added veh_id_atk bounds checking to avoid issues
        if (veh_id_atk >= 0 && veh_atk->is_probe() && !veh->is_probe()) {
            continue;
        }
        int flags = 0;
        if (veh_id_atk >= 0 && can_arty(veh->unit_id, true) && can_arty(veh_atk->unit_id, true)) {
            flags = CT_WEAPON_ONLY;
        }
        if (veh_id_atk >= 0 && veh_atk->triad() == TRIAD_AIR
        && has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)
        && !veh->is_missile() && !veh_atk->is_missile()
        && veh_atk->offense_value() > 0 && veh_atk->defense_value() > 0
        && veh->offense_value() > 0) {
            flags |= (CT_WEAPON_ONLY | CT_AIR_DEFENSE);
            if (veh->triad() == TRIAD_AIR) {
                flags |= CT_INTERCEPT;
            }
        }
        int offense_out = 0;
        int defense_out = 0;
        mod_battle_compute(veh_id_atk, veh_id, &offense_out, &defense_out, flags);
        if (!offense_out) {
            break;
        }
        int score = offense_val * defense_out * min(veh->cur_hitpoints(), 9999)
            / veh->max_hitpoints() / offense_out / 8 - veh->offense_value();
        if (veh->plan() <= PLAN_NAVAL_TRANSPORT || veh->plan() == PLAN_TERRAFORM) {
            score *= 16;
        }
        if (veh_id_atk >= 0
        && veh->triad() == TRIAD_AIR && veh_atk->triad() == TRIAD_AIR
        && !veh->is_missile() && !veh_atk->is_missile()
        && (has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)
        || (has_abil(veh_atk->unit_id, ABL_AIR_SUPERIORITY)
        && base_id < 0 && !is_airbase && !mod_stack_check(veh_id, 6, ABL_CARRIER, -1, -1)))) {
            score += 0x80000;
        } else if (check_arty) {
            if (can_arty(veh->unit_id, true)) {
                score += 0x80000;
            }
        } else if (veh->state & VSTATE_DESIGNATE_DEFENDER
        && (base_id >= 0 || is_airbase
        || mod_stack_check(veh_id, 6, ABL_CARRIER, -1, -1)
        || !mod_stack_check(veh_id, 3, TRIAD_AIR, -1, -1))) {
            score += 0x80000;
        }
        // Replace old score method to avoid overflow issues with larger veh_ids
        if (score > best_score || (score == best_score && veh_id > best_veh_id)) {
            best_score = score;
            best_veh_id = veh_id;
        }
    }
    return best_veh_id;
}

static int current_atk_veh_id = -1;
static int current_def_veh_id = -1;

int __cdecl mod_best_defender(int veh_id_def, int veh_id_atk, int check_arty) {
    int veh_id = find_defender(veh_id_def, veh_id_atk, check_arty);
    if (check_arty) {
        current_atk_veh_id = -1;
        current_def_veh_id = -1;
    } else {
        current_atk_veh_id = veh_id_atk;
        current_def_veh_id = veh_id;
    }
    return veh_id;
}

int __cdecl battle_fight_parse_num(int index, int value) {
    if (index > 9) {
        return 3;
    }
    ParseNumTable[index] = value;

    if (conf.ignore_reactor_power && index == 1
    && current_atk_veh_id >= 0 && current_def_veh_id >= 0) {
        VEH* veh_atk = &Vehs[current_atk_veh_id];
        VEH* veh_def = &Vehs[current_def_veh_id];
        combat_odds_fix(veh_atk, veh_def, &ParseNumTable[0], &ParseNumTable[1]);
    }
    return 0;
}

/*
This function replaces defense_value and uses less parameters for simplicity.
*/
int terrain_defense(VEH* veh_def, VEH* veh_atk)
{
    MAP* sq;
    *VehBattleDisplayTerrain = NULL;
    if (!veh_def || veh_def->triad() == TRIAD_AIR
    || !(sq = mapsq(veh_def->x, veh_def->y))
    || is_ocean(sq) || sq->base_who() >= 0) {
        return 2;
    }
    bool native = veh_atk && (veh_atk->is_native_unit()
        || has_project(FAC_PHOLUS_MUTAGEN, veh_atk->faction_id));
    if (sq->is_fungus() && veh_atk && (!veh_atk->faction_id || native)) {
        return 2;
    }
    int defense = sq->is_rocky();
    if (defense) {
        *VehBattleDisplayTerrain = label_get(91); // Rocky
    }
    if (!native && !defense && sq->is_fungus() && veh_def->triad() != TRIAD_AIR) {
        *VehBattleDisplayTerrain = label_get(338); // Fungus
        defense = (has_project(FAC_PHOLUS_MUTAGEN, veh_def->faction_id)
            || veh_def->is_native_unit()) ? 2 : 1;
    }
    if (sq->items & BIT_FOREST && !defense && (!veh_atk || veh_atk->triad() == TRIAD_LAND)) {
        *VehBattleDisplayTerrain = label_get(291); // Forest
        defense = 1;
    }
    return defense + 2;
}

/*
Original game decision making for using nerve gas ability.
*/
bool use_nerve_gas(int faction_id_atk, int faction_id_def) {
    Faction* f_atk = &Factions[faction_id_atk];
    Faction* f_def = &Factions[faction_id_def];
    int value = 0;
    if (*GameRules & RULES_INTENSE_RIVALRY && (f_def->ranking > f_atk->ranking
    || f_def->diplo_unk_4[faction_id_atk] > f_atk->diplo_unk_4[faction_id_def])) {
        value = 3 * (f_atk->AI_fight + 1);
    }
    if ((f_atk->diplo_status[faction_id_def] & DIPLO_ATROCITY_VICTIM)
    || (f_atk->player_flags & PFLAG_COMMIT_ATROCITIES_WANTONLY)
    || (is_human(faction_id_def) && !un_charter())
    || (is_human(faction_id_def)
    && f_atk->diplo_status[faction_id_def] & DIPLO_WANT_REVENGE
    && value + f_def->integrity_blemishes >= 6)) {
        return true;
    }
    return false;
}

bool use_nerve_gas(int faction_id_atk) {
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id_atk && is_alive(i) && !has_pact(faction_id_atk, i)
        && use_nerve_gas(faction_id_atk, i)) {
            return true;
        }
    }
    return false;
}

void __cdecl add_bat(int type, int modifier, const char* display_str)
{
    int offset = VehBattleModCount[type];
    if (modifier && offset >= 0 && offset < 4 && (type == 0 || type == 1)) {
        strcpy_n(VehBattleDisplay[type][offset], 80, display_str);
        VehBattleModifier[type][offset] = modifier;
        VehBattleModCount[type]++;
    }
}

void __cdecl mod_battle_compute(int veh_id_atk, int veh_id_def, int* offense_out, int* defense_out, int combat_type)
{
    enum DefenseType {
        DF_Enable = 1,
        DF_Flechette = 2,
        DF_Sensor = 4,
        DF_GSP = 8,
    };
    assert(veh_id_atk >= 0 && veh_id_atk < *VehCount);
    assert(veh_id_def >= 0 && veh_id_def < *VehCount);
    VehBattleModCount[0] = 0;
    VehBattleModCount[1] = 0;
    VehBattleState[0] = 0;
    VehBattleState[1] = 0;

    VEH* veh_atk = {};
    VEH* veh_def = {};
    MAP* sq_atk = {};
    MAP* sq_def = {};
    int defense = 8;
    int offense = 8;
    int psi_combat = 0;
    int faction_id_atk = 0;
    int faction_id_def = 0;

    if (veh_id_atk >= 0) {
        veh_atk = &Vehs[veh_id_atk];
        faction_id_atk = veh_atk->faction_id;
        sq_atk = mapsq(veh_atk->x, veh_atk->y);
        assert(veh_atk->chassis_type() >= 0);
        assert(veh_atk->weapon_type() >= 0);
        assert(veh_atk->armor_type() >= 0);
    }
    if (veh_id_def >= 0) {
        veh_def = &Vehs[veh_id_def];
        faction_id_def = veh_def->faction_id;
        sq_def = mapsq(veh_def->x, veh_def->y);
        assert(veh_def->chassis_type() >= 0);
        assert(veh_def->weapon_type() >= 0);
        assert(veh_def->armor_type() >= 0);
    }
    const bool is_arty = combat_type & CT_CAN_ARTY;
    const bool is_bombard = (is_arty || (veh_id_atk >= 0 && veh_atk->is_missile()));
    const bool is_rocky = sq_def && sq_def->is_rocky();
    const int alt_atk = sq_atk ? sq_atk->alt_level() : 0;
    const int alt_def = sq_def ? sq_def->alt_level() : 0;

    if (!is_bombard
    || (veh_id_atk >= 0 && veh_atk->unit_id == BSC_SPORE_LAUNCHER)
    || (veh_id_def >= 0 && veh_def->unit_id == BSC_SPORE_LAUNCHER)) {
        if (veh_id_atk >= 0 && veh_atk->offense_value() < 0) {
            psi_combat = 1;
        }
        if (veh_id_def >= 0 && veh_def->defense_value() < 0) {
            psi_combat = psi_combat | 2;
        }
        if (psi_combat) {
            // Display PSI attack/defend bonuses
            int rule_psi_atk = MFactions[faction_id_atk].rule_psi;
            if (rule_psi_atk) {
                add_bat(0, rule_psi_atk, label_get(342)); // Gaian Psi
            }
            if (has_project(FAC_DREAM_TWISTER, faction_id_atk)) {
                add_bat(0, conf.dream_twister_bonus, label_get(343)); // Dream Twist
            }
            int rule_psi_def = MFactions[faction_id_def].rule_psi;
            if (rule_psi_def) {
                add_bat(1, rule_psi_def, label_get(342)); // Gaian Psi
            }
            if (has_project(FAC_NEURAL_AMPLIFIER, faction_id_def)) {
                add_bat(1, conf.neural_amplifier_bonus, label_get(344)); // Neural Amp
            }
            if (veh_id_def >= 0 && veh_def->unit_id == BSC_FUNGAL_TOWER) {
                add_bat(1, conf.fungal_tower_bonus, Units[BSC_FUNGAL_TOWER].name); // Fungal Tower
            }
        }
    }
    if (veh_id_atk >= 0) {
        offense = mod_get_basic_offense(veh_id_atk, veh_id_def, psi_combat, is_bombard, 0);
        if (!veh_atk->is_probe()) {
            if (veh_id_def >= 0 && !combat_type) { // checking if isn't wpn vs wpn ; air toggle
                if (sq_def && sq_def->is_fungus()
                && (veh_atk->is_native_unit() || has_project(FAC_PHOLUS_MUTAGEN, faction_id_atk))
                && veh_def->offense_value() >= 0) {
                    offense = offense * 150 / 100;
                    add_bat(0, 50, label_get(338)); // Fungus
                }
                if (psi_combat & 2 && veh_atk->is_resonance_weapon()) {
                    offense = offense * (ResonanceWeaponValue + 100) / 100;
                    add_bat(0, ResonanceWeaponValue, label_get(1110)); // Resonance Attack
                }
                if (psi_combat
                && has_abil(veh_atk->unit_id, ABL_EMPATH)
                && !has_abil(veh_def->unit_id, ABL_DISSOCIATIVE_WAVE)
                && Rules->combat_bonus_empath_song_vs_psi) {
                    offense = offense * (Rules->combat_bonus_empath_song_vs_psi + 100) / 100;
                    add_bat(0, Rules->combat_bonus_empath_song_vs_psi, Ability[ABL_ID_EMPATH].name);
                }
            }
            if (Rules->combat_penalty_atk_airdrop
            && veh_atk->state & VSTATE_MADE_AIRDROP
            && has_abil(veh_atk->unit_id, ABL_DROP_POD)) {
                offense = offense * (100 - Rules->combat_penalty_atk_airdrop) / 100;
                add_bat(0, -Rules->combat_penalty_atk_airdrop,
                    (drop_range(faction_id_atk) <= Rules->max_airdrop_rng_wo_orbital_insert
                    ? label_get(437) // Air Drop
                    : label_get(438))); // Orbital Insertion
            }
            if (MFactions[faction_id_atk].rule_flags & RFLAG_FANATIC
            && Rules->combat_bonus_fanatic && !combat_type && !psi_combat) {
                offense = offense * (Rules->combat_bonus_fanatic + 100) / 100;
                add_bat(0, Rules->combat_bonus_fanatic, label_get(528)); // Belief
            }
            int bonus_count = MFactions[faction_id_atk].faction_bonus_count;
            for (int i = 0; i < bonus_count; i++) {
                if (MFactions[faction_id_atk].faction_bonus_id[i] == RULE_OFFENSE) {
                    int rule_off_bonus = MFactions[faction_id_atk].faction_bonus_val1[i];
                    offense = offense * rule_off_bonus / 100;
                    add_bat(0, rule_off_bonus - 100, label_get(1108)); // Alien Offense
                }
            }
            if (psi_combat && faction_id_atk) {
                int planet_atk = Factions[faction_id_atk].SE_planet;
                if (planet_atk && Rules->combat_psi_bonus_per_planet) {
                    int modifier = planet_atk * Rules->combat_psi_bonus_per_planet;
                    offense = offense * (modifier + 100) / 100;
                    add_bat(0, modifier, label_get(625)); // Planet
                }
            }
        }
    }
    if ((combat_type & CT_WEAPON_ONLY) && veh_id_def >= 0) { // Weapon-to-weapon battles
        defense = mod_get_basic_offense(veh_id_def, veh_id_atk, psi_combat, is_bombard, 1);
        if (!(combat_type & (CT_INTERCEPT|CT_AIR_DEFENSE))) { // Artillery duel
            int gun_vs_ship_art = Rules->combat_land_vs_sea_artillery;
            if (veh_atk->triad() == TRIAD_LAND && veh_def->triad() == TRIAD_SEA) {
                if (gun_vs_ship_art) {
                    offense = offense * (gun_vs_ship_art + 100) / 100;
                    add_bat(0, gun_vs_ship_art, label_get(435)); // Land Based Guns
                }
            } else if (veh_atk->triad() == TRIAD_SEA && veh_def->triad() == TRIAD_LAND) {
                if (gun_vs_ship_art) {
                    defense = defense * (gun_vs_ship_art + 100) / 100;
                    add_bat(1, gun_vs_ship_art, label_get(435)); // Land Based Guns
                }
            } else if (Rules->combat_artillery_bonus_altitude && sq_atk && sq_def) {
                if (alt_def > alt_atk) {
                    int modifier = (alt_def - alt_atk) * Rules->combat_artillery_bonus_altitude;
                    defense = defense * (modifier + 100) / 100;
                    add_bat(1, modifier, label_get(576)); // Altitude
                } else if (alt_def < alt_atk) {
                    int modifier = (alt_atk - alt_def) * Rules->combat_artillery_bonus_altitude;
                    offense = offense * (modifier + 100) / 100;
                    add_bat(0, modifier, label_get(576)); // Altitude
                }
            }
        }
    } else if (veh_id_def >= 0) {
        defense = mod_get_basic_defense(veh_id_def, veh_id_atk, psi_combat, is_bombard);
        int bonus_count = MFactions[faction_id_def].faction_bonus_count;
        for (int i = 0; i < bonus_count; i++) {
            if (MFactions[faction_id_def].faction_bonus_id[i] == RULE_DEFENSE) {
                int rule_def_bonus = MFactions[faction_id_def].faction_bonus_val1[i];
                defense = defense * rule_def_bonus / 100;
                add_bat(1, rule_def_bonus - 100, label_get(1109)); // Alien Defense
            }
        }
        if (veh_id_atk >= 0 && veh_atk->is_probe()) {
            defense *= 4;
        } else {
            int base_id_def = base_at(veh_def->x, veh_def->y);
            if (psi_combat) {
                if (base_id_def >= 0 && Rules->combat_bonus_intrinsic_base_def) {
                    add_bat(1, Rules->combat_bonus_intrinsic_base_def, label_get(332)); // Base
                    defense = defense * (Rules->combat_bonus_intrinsic_base_def + 100) / 100;
                }
                if (conf.planet_defense_bonus && faction_id_def) {
                    int planet_def = Factions[faction_id_def].SE_planet;
                    int modifier = planet_def * Rules->combat_psi_bonus_per_planet
                        / (conf.planet_defense_bonus <= 1 ? 2 : 1);
                    if (modifier != 0) {
                        defense = defense * (modifier + 100) / 100;
                        add_bat(1, modifier, label_get(625)); // Planet
                    }
                }
                defense *= 4;
            } else {
                int terrain_def = terrain_defense(veh_def, veh_atk);
                assert(terrain_def == (veh_def->triad() == TRIAD_AIR ? 2 :
                    defense_value(faction_id_def, veh_def->x, veh_def->y, veh_id_def, veh_id_atk)));
                bool plain_terrain = terrain_def <= 2;
                if (veh_id_atk >= 0 && veh_atk->triad() == TRIAD_LAND) {
                    if (Rules->combat_artillery_bonus_altitude
                    && combat_type && veh_def->triad() == TRIAD_LAND && alt_atk > alt_def) {
                        offense = offense * (Rules->combat_artillery_bonus_altitude
                            * (alt_atk - alt_def) + 100) / 100;
                        add_bat(0, Rules->combat_artillery_bonus_altitude, label_get(576)); // Altitude
                    }
                    if (Rules->combat_mobile_open_ground
                    && !combat_type && base_id_def < 0
                    && plain_terrain && !is_rocky) {
                        int speed_atk = proto_speed(veh_atk->unit_id);
                        int speed_def = proto_speed(veh_def->unit_id);
                        if (speed_atk > Rules->move_rate_roads
                        && speed_atk >= speed_def) {
                            offense = offense * (Rules->combat_mobile_open_ground + 100) / 100;
                            add_bat(0, Rules->combat_mobile_open_ground, label_get(611)); // Mobile in open
                        }
                    }
                    if (Rules->combat_mobile_def_in_rough
                    && !combat_type && (!plain_terrain || base_id_def >= 0)
                    && proto_speed(veh_atk->unit_id) > Rules->move_rate_roads) {
                        defense = defense * (Rules->combat_mobile_def_in_rough + 100) / 100;
                        // "Rough vs. Mobile" / "Mobile vs. Base"
                        add_bat(1, Rules->combat_mobile_def_in_rough, base_id_def < 0
                            ? label_get(548) : label_get(612));
                    }
                    // Modify this bonus to apply only when both tiles have roads or magtubes
                    // and the defender does not have a base or bunker
                    if (Rules->combat_bonus_atk_road
                    && !combat_type && !is_ocean(sq_atk) && !is_ocean(sq_def)
                    && !sq_def->is_base_or_bunker()
                    && sq_atk->items & (BIT_BASE_IN_TILE|BIT_ROAD|BIT_MAGTUBE)
                    && sq_def->items & (BIT_ROAD|BIT_MAGTUBE)) {
                        offense = offense * (Rules->combat_bonus_atk_road + 100) / 100;
                        add_bat(0, Rules->combat_bonus_atk_road, label_get(606)); // Road Attack
                    }
                    if (Rules->combat_penalty_atk_lower_elevation
                    && !combat_type && alt_def > alt_atk
                    && !has_abil(veh_atk->unit_id, ABL_ANTIGRAV_STRUTS)) {
                        defense = defense * (Rules->combat_penalty_atk_lower_elevation + 100) / 100;
                        add_bat(1, Rules->combat_penalty_atk_lower_elevation, label_get(441)); // Uphill
                    }
                    if (Rules->combat_bonus_atk_higher_elevation
                    && !combat_type && alt_atk > alt_def
                    && !has_abil(veh_def->unit_id, ABL_ANTIGRAV_STRUTS)) {
                        offense = offense * (Rules->combat_bonus_atk_higher_elevation + 100) / 100;
                        add_bat(0, Rules->combat_bonus_atk_higher_elevation, label_get(330)); // Downhill
                    }
                }
                defense *= terrain_def;
                if (!plain_terrain) {
                    add_bat(1, 50 * terrain_def - 100,
                        (*VehBattleDisplayTerrain ? *VehBattleDisplayTerrain : label_get(331))); // Terrain
                }
                char* display_def = NULL; // only one is displayed
                int def_multi = 100;
                if (base_id_def >= 0 && veh_id_atk >= 0) {
                    display_def = label_get(332); // Base
                    int fac_modifier = 0;
                    // Fix: original code checks if faction_id_def has Citizens Defense Force
                    switch (veh_atk->triad()) {
                      case TRIAD_LAND:
                        if (has_facility(FAC_PERIMETER_DEFENSE, base_id_def)) {
                            fac_modifier = conf.facility_defense_value[0];
                            display_def = label_get(354); // Perimeter
                        }
                        break;
                      case TRIAD_SEA:
                        if (has_facility(FAC_NAVAL_YARD, base_id_def)) {
                            fac_modifier = conf.facility_defense_value[1];
                            display_def = Facility[FAC_NAVAL_YARD].name;
                        }
                        break;
                      case TRIAD_AIR:
                        if (has_facility(FAC_AEROSPACE_COMPLEX, base_id_def)) {
                            fac_modifier = conf.facility_defense_value[2];
                            display_def = Facility[FAC_AEROSPACE_COMPLEX].name;
                        }
                        break;
                      default:
                        break;
                    }
                    if (has_fac_built(FAC_TACHYON_FIELD, base_id_def)) {
                        fac_modifier += conf.facility_defense_value[3];
                        display_def = label_get(357); // Tachyon
                    }
                    if (Rules->combat_bonus_intrinsic_base_def
                    && !fac_modifier && !has_abil(veh_atk->unit_id, ABL_BLINK_DISPLACER)) {
                        defense = defense * (Rules->combat_bonus_intrinsic_base_def + 100) / 100;
                        add_bat(1, Rules->combat_bonus_intrinsic_base_def, label_get(332)); // Base
                    }
                    if (fac_modifier && has_abil(veh_atk->unit_id, ABL_BLINK_DISPLACER)) {
                        fac_modifier = 0;
                        display_def = label_get(428); // Base vs. Blink
                    }
                    if (Rules->combat_infantry_vs_base
                    && faction_id_atk && !combat_type
                    && !veh_atk->is_native_unit()
                    && veh_atk->triad() == TRIAD_LAND
                    && veh_atk->speed() == 1) {
                        offense = offense * (Rules->combat_infantry_vs_base + 100) / 100;
                        add_bat(0, Rules->combat_infantry_vs_base, label_get(547)); // Infantry vs. Base
                    }
                    def_multi += fac_modifier;
                }
                else if (base_id_def < 0 && sq_def && sq_def->is_bunker()
                && (veh_id_atk < 0 || veh_atk->triad() != TRIAD_AIR)) {
                    def_multi = 150;
                    display_def = label_get(358); // Bunker
                }
                else if (is_arty && base_id_def < 0 && sq_def && !is_rocky
                && !(sq_def->items & BIT_FOREST) && !sq_def->is_fungus()) {
                    def_multi = 150;
                    display_def = label_get(525); // Open Ground
                }
                defense = defense * def_multi / 50;
                if (def_multi > 100) {
                    add_bat(1, def_multi - 100, display_def);
                }
            }
            if (faction_id_def) {
                // Sensor search is optimized to avoid repeated calls to is_sensor and base_find
                int def_state = (conf.sensor_defense_ocean || !is_ocean(sq_def) ? DF_Enable : 0);
                int flechette = 100;
                for (int i = 0; i < *BaseCount; i++) {
                    if (Bases[i].faction_id == faction_id_def) {
                        int dist = map_range(veh_def->x, veh_def->y, Bases[i].x, Bases[i].y);
                        if (dist <= FlechetteDefenseRange
                        && has_fac_built(FAC_FLECHETTE_DEFENSE_SYS, i)
                        && veh_id_atk >= 0 && veh_atk->is_missile()) {
                            // Multiple facilities are cumulative with missile defense
                            defense = defense * (FlechetteDefenseValue + 100) / 100;
                            flechette = flechette * (FlechetteDefenseValue + 100) / 100;
                            def_state |= DF_Flechette;
                        }
                        if ((def_state & DF_Enable) && dist <= GeosyncSurveyPodRange
                        && has_fac_built(FAC_GEOSYNC_SURVEY_POD, i)) {
                            def_state |= (DF_Sensor | DF_GSP);
                        }
                    }
                }
                if ((def_state & DF_Enable) && !(def_state & DF_Sensor)) {
                    for (auto& m : iterate_tiles(veh_def->x, veh_def->y, 0, 25)) {
                        if (m.sq->items & BIT_SENSOR) {
                            int owner = mod_whose_territory(faction_id_def, m.x, m.y, 0, 0);
                            if (owner < 0 || owner == faction_id_def) {
                                def_state |= DF_Sensor;
                            }
                        }
                    }
                }
                if (def_state & DF_Flechette) {
                    add_bat(1, flechette - 100, label_get(1113)); // Flechette
                }
                if (def_state & DF_Sensor) {
                    defense = defense * (Rules->combat_defend_sensor + 100) / 100;
                    add_bat(1, Rules->combat_defend_sensor, label_get(613)); // Sensor
                }
                if (def_state & DF_GSP) {
                    defense = defense * (Rules->combat_defend_sensor + 100) / 100;
                    add_bat(1, Rules->combat_defend_sensor, label_get(1123)); // GSP
                }
            }
            if (veh_id_atk >= 0 && psi_combat & 1
            && Rules->combat_bonus_trance_vs_psi
            && has_abil(veh_def->unit_id, ABL_TRANCE)
            && !has_abil(veh_atk->unit_id, ABL_DISSOCIATIVE_WAVE)) {
                defense = defense * (Rules->combat_bonus_trance_vs_psi + 100) / 100;
                add_bat(1, Rules->combat_bonus_trance_vs_psi, label_get(329)); // Trance
            }
            if (veh_id_atk >= 0 && psi_combat & 1 && veh_def->is_resonance_armor()) {
                defense = defense * (ResonanceArmorValue + 100) / 100;
                add_bat(1, ResonanceArmorValue, label_get(1111)); // Resonance Def.
            }
            // added veh_id_atk checks below when needed
            if (veh_id_atk >= 0 && !psi_combat
            && veh_atk->triad() == TRIAD_AIR
            && has_abil(veh_atk->unit_id, ABL_AIR_SUPERIORITY)) {
                if (veh_def->triad() != TRIAD_AIR) {
                    int penalty = Rules->combat_penalty_air_supr_vs_ground;
                    if (penalty) {
                        offense = offense * (100 - penalty) / 100;
                        add_bat(0, -penalty, label_get(448)); // Ground Strike
                    }
                } else if (Rules->combat_bonus_air_supr_vs_air
                && !has_abil(veh_def->unit_id, ABL_AIR_SUPERIORITY)) {
                    offense = offense * (Rules->combat_bonus_air_supr_vs_air + 100) / 100;
                    add_bat(0, Rules->combat_bonus_air_supr_vs_air, label_get(449)); // Air-to-Air
                }
            }
            if (veh_id_atk >= 0 && !psi_combat
            && Rules->combat_bonus_air_supr_vs_air
            && veh_atk->triad() == TRIAD_AIR
            && veh_def->triad() == TRIAD_AIR
            && !has_abil(veh_atk->unit_id, ABL_AIR_SUPERIORITY)
            && has_abil(veh_def->unit_id, ABL_AIR_SUPERIORITY)
            && !veh_atk->is_missile() && !veh_def->is_missile()) {
                defense = defense * (Rules->combat_bonus_air_supr_vs_air + 100) / 100;
                add_bat(1, Rules->combat_bonus_air_supr_vs_air, label_get(449)); // Air-to-Air
            }
            if (Rules->combat_penalty_non_combat_vs_combat
            && !veh_def->offense_value() && veh_def->defense_value() == 1
            && (faction_id_atk || base_id_def < 0)) {
                defense = defense * (100 - Rules->combat_penalty_non_combat_vs_combat) / 100;
                add_bat(1, -Rules->combat_penalty_non_combat_vs_combat, label_get(439)); // Non Combat
            }
            if (veh_id_atk >= 0 && base_id_def >= 0
            && Rules->combat_bonus_vs_ship_port
            && veh_atk->triad() != TRIAD_SEA
            && veh_def->triad() == TRIAD_SEA) {
                offense = offense * (Rules->combat_bonus_vs_ship_port + 100) / 100;
                add_bat(0, Rules->combat_bonus_vs_ship_port, label_get(335)); // In Port
            }
            if (veh_id_atk >= 0
            && veh_def->is_pulse_armor()
            && veh_atk->triad() == TRIAD_LAND
            && veh_atk->speed() > 1) {
                defense = defense * (PulseArmorValue + 100) / 100;
                add_bat(1, PulseArmorValue, label_get(1112)); // Pulse Defense
            }
            if (veh_id_atk >= 0
            && Rules->combat_comm_jammer_vs_mobile
            && has_abil(veh_def->unit_id, ABL_COMM_JAMMER)
            && !has_abil(veh_atk->unit_id, ABL_DISSOCIATIVE_WAVE)
            && veh_atk->triad() == TRIAD_LAND
            && veh_atk->speed() > 1) {
                defense = defense * (Rules->combat_comm_jammer_vs_mobile + 100) / 100;
                add_bat(1, Rules->combat_comm_jammer_vs_mobile, label_get(336)); // Jammer
            }
            if (veh_id_atk >= 0
            && Rules->combat_aaa_bonus_vs_air
            && veh_atk->triad() == TRIAD_AIR
            && has_abil(veh_def->unit_id, ABL_AAA)
            && !has_abil(veh_atk->unit_id, ABL_DISSOCIATIVE_WAVE)) {
                defense = defense * (Rules->combat_aaa_bonus_vs_air + 100) / 100;
                add_bat(1, Rules->combat_aaa_bonus_vs_air, label_get(337)); // Tracking
            }
        }
    }
    if (offense_out) {
        *offense_out = offense;
    }
    if (defense_out) {
        *defense_out = defense;
    }
}

/*
Check for possible promotions after victorious combat event unless capturing artifacts.
*/
void __cdecl promote(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    if (veh_id >= 0 && !veh->is_missile()
    && (veh->plan() < PLAN_COLONY || veh->defense_value() > 1)) {
        int morale = mod_morale_veh(veh_id, 1, 0);
        if (veh->morale < MORALE_ELITE && morale < MORALE_ELITE) {
            if (morale <= MORALE_DISCIPLINED || combat_rand(2) == 0) {
                veh->morale++;
                int mod_morale = mod_morale_veh(veh_id, 1, 0);
                if (morale != mod_morale && veh->faction_id == MapWin->cOwner && is_human(veh->faction_id)) {
                    parse_says(1, veh->name(), -1, -1);
                    if (veh->is_native_unit()) {
                        parse_says(0, Morale[mod_morale].name_lifecycle, -1, -1);
                        // Change promotions to delayed notifications instead of popups
                        net_pop("MORALE2", "batwon_sm.pcx");
                    } else {
                        parse_says(0, Morale[mod_morale].name, -1, -1);
                        net_pop("MORALE", "batwon_sm.pcx");
                    }
                }
            }
        }
    }
}

/*
Search nearby tiles for interceptors at airbases to defend from any attacks.
Returns non-zero when suitable aircraft is found, zero if none is available.
*/
int __cdecl interceptor(int faction_id_def, int faction_id_atk, int tx, int ty) {
    int veh_id_def = -1;
    int best_score = INT_MIN;
    for (int i = 0; i < *VehCount; ++i) {
        MAP* sq;
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id_def && veh->triad() == TRIAD_AIR
        && has_abil(veh->unit_id, ABL_AIR_SUPERIORITY)
        && map_range(veh->x, veh->y, tx, ty) <= conf.intercept_max_range
        && (sq = mapsq(veh->x, veh->y))) {
            if (sq->is_airbase() || mod_stack_check(i, 6, ABL_CARRIER, -1, -1)) {
                // Modified priority for highest HP * Attack, then distance, then newer units
                int score = 2 * veh->cur_hitpoints() * max(1, veh->offense_value())
                    - map_range(veh->x, veh->y, tx, ty);
                if (score >= best_score) {
                    best_score = score;
                    veh_id_def = i;
                }
            }
        }
    }
    if (veh_id_def < 0) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id_def];
    if (faction_id_def == MapWin->cOwner || faction_id_atk == MapWin->cOwner) {
        parse_says(0, &MFactions[faction_id_def].adj_name_faction[0], -1, -1);
        *VehLiftX = veh->x;
        *VehLiftY = veh->y;
        int base_id = base_find(veh->x, veh->y);
        if (base_id >= 0) {
            parse_says(1, &Bases[base_id].name[0], -1, -1);
        }
        Console_focus(MapWin, tx, ty, faction_id_def);
        net_pop("AIRSCRAMBLE", 0);
        int vx = veh->x;
        int vy = veh->y;
        while (map_range(vx, vy, tx, ty) > 0) {
            int offset = -1;
            int best_value = INT_MAX;
            for (auto& m : iterate_tiles(vx, vy, 1, 9)) {
                int value = map_range(m.x, m.y, tx, ty);
                if (value < best_value) {
                    best_value = value;
                    offset = m.i - 1; // TableOffset -> BaseOffset
                }
            }
            if (offset >= 0) {
                veh_scoot(veh_id_def, vx, vy, offset, 0);
                vx = wrap(vx + BaseOffsetX[offset]);
                vy = vy + BaseOffsetY[offset];
            } else {
                assert(0);
                return 0;
            }
        }
    }
    veh_drop(veh_lift(veh_id_def), tx, ty);
    veh->order = ORDER_NONE;
    veh->state &= ~(VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
    return 1;
}

int __cdecl mod_battle_fight(int veh_id, int offset, int table_offset, int option, int* def_id)
{
    VEH* veh = &Vehs[veh_id];
    int x2, y2;
    if (table_offset) {
        x2 = wrap(veh->x + TableOffsetX[offset]);
        y2 = veh->y + TableOffsetY[offset];
    } else {
        x2 = wrap(veh->x + BaseOffsetX[offset]);
        y2 = veh->y + BaseOffsetY[offset];
    }
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].x == x2 && Vehs[i].y == y2 && Vehs[i].faction_id == veh->faction_id) {
            // Skip alien_move or action_destroy artillery attack on friendly units
            return 0;
        }
    }
    return mod_battle_fight_2(veh_id, offset, x2, y2, table_offset, option, def_id);
}

int __cdecl mod_battle_fight_2(int veh_id_atk, int offset, int tx, int ty, int table_offset, int option, int* def_id)
{
    const int player_id = MapWin->cOwner;
    char base_name[LineBufLen] = {};
    bool interlude_base_attack = 0;
    int faction_id_atk = 0;
    int faction_id_def = 0;
    int veh_id_def = -1;
    int defense_out = 0;
    int offense_out = 0;
    int combat_type = 0;
    int nerve_gas = 0;
    int x = -1;
    int y = -1;
    int intercept_state = 0;
    int credits_income = 0;
    int def_has_moved = 0;
    int treaty_report = 0;
    int marine_detach = 0;
    int detach_value = 0;

    VEH* veh_atk = {};
    VEH* veh_def = {};
    MAP* sq_atk = {};
    MAP* sq_def = mapsq(tx, ty);

    if (veh_id_atk >= 0) {
        veh_atk = &Vehs[veh_id_atk];
        faction_id_atk = veh_atk->faction_id;
        sq_atk = mapsq(veh_atk->x, veh_atk->y);
        x = veh_atk->x;
        y = veh_atk->y;
    }
    assert(veh_id_atk >= 0 && veh_id_atk < *VehCount);
    assert(sq_atk);
    assert(sq_def);

    veh_id_def = stack_fix(veh_at(tx, ty)); // First call stack_fix before any other checks
    if (table_offset || (can_arty(veh_atk->unit_id, false) && veh_atk->triad() == TRIAD_LAND)) {
        combat_type = CT_CAN_ARTY;
    } else if (can_arty(veh_atk->unit_id, true) && veh_atk->triad() == TRIAD_SEA && !is_ocean(sq_def)) {
        if (sq_def->is_base()
        || mod_stack_check(veh_id_def, 3, 2, -1, -1) < mod_stack_check(veh_id_def, 1, -1, -1, -1)) {
            combat_type = CT_CAN_ARTY;
        }
    }
    if (veh_id_def >= 0
    && veh_atk->triad() == TRIAD_AIR
    && !veh_atk->is_missile()
    && !mod_stack_check(veh_id_def, 6, ABL_AIR_SUPERIORITY, -1, -1)
    && option && *VehAttackFlags & 2) {
        intercept_state = interceptor(Vehs[veh_id_def].faction_id, faction_id_atk, tx, ty);
    }
    veh_id_def = find_defender(veh_id_def, veh_id_atk, combat_type);
    if (veh_id_def >= 0) {
        veh_def = &Vehs[veh_id_def];
        faction_id_def = veh_def->faction_id;
        if (DEBUG && option && faction_id_atk == faction_id_def) {
            print_veh(veh_id_atk);
            print_veh_stack(tx, ty);
        }
        assert(faction_id_atk != faction_id_def || !option);
    } else {
        assert(veh_id_def >= 0);
        return 2;
    }
    bool combat_defend = veh_def->is_combat_unit();
    bool psi_combat = (veh_atk->offense_value() < 0 || veh_def->defense_value() < 0);
    int base_id = base_at(tx, ty);
    Faction* f_atk = &Factions[faction_id_atk];
    Faction* f_def = &Factions[faction_id_def];
    MFaction* m_atk = &MFactions[faction_id_atk];
    MFaction* m_def = &MFactions[faction_id_def];

    if (can_arty(veh_atk->unit_id, true) && can_arty(veh_def->unit_id, true)) {
        if (veh_atk->triad() != TRIAD_LAND) {
            if (combat_type || !is_ocean(sq_def)) {
                combat_type |= CT_WEAPON_ONLY;
            }
        } else {
            combat_type |= CT_WEAPON_ONLY;
        }
    }
    if (has_abil(veh_def->unit_id, ABL_AIR_SUPERIORITY)
    && veh_atk->triad() == TRIAD_AIR
    && veh_def->triad() == TRIAD_AIR
    && !veh_atk->is_missile()
    && !veh_def->is_missile()) {
        combat_type |= (CT_INTERCEPT|CT_AIR_DEFENSE|CT_WEAPON_ONLY);
    }
    mod_battle_compute(veh_id_atk, veh_id_def, &offense_out, &defense_out, combat_type);
    int veh_atk_hp = veh_atk->cur_hitpoints();
    int veh_def_hp = veh_def->cur_hitpoints();
    int veh_atk_val;
    int veh_def_val;

    if (conf.ignore_reactor_power || psi_combat) {
        veh_atk_val = veh_atk->reactor_type(); // Added range limits
        veh_def_val = veh_def->reactor_type(); // Added range limits
    } else {
        veh_atk_val = 1;
        veh_def_val = 1;
    }

    if (!option) { // Only evaluate battle odds
        int offense = offense_out * veh_def_val * veh_atk_hp / veh_atk->max_hitpoints();
        int defense = defense_out * veh_atk_val * veh_def_hp / veh_def->max_hitpoints();
        if (def_id) {
            *def_id = veh_id_def;
        }
        int value = offense * ((combat_type & CT_CAN_ARTY) ? 4 : 8) / (defense + 1);
        if (DEBUG) {
            int res_id = -1;
            assert(value == battle_fight_2(veh_id_atk, offset, tx, ty, table_offset, option, &res_id));
            assert(veh_id_def == res_id);
        }
        return value;
    }
    if (!is_human(faction_id_atk)
    || !(*VehAttackFlags & 1)
    || veh_atk->is_probe()
    || (veh_def->is_artifact() && base_id < 0)
    || (*MultiplayerActive && intercept_state)) {
        // nothing
    } else if (break_treaty(faction_id_atk, faction_id_def, (DIPLO_COMMLINK|DIPLO_TREATY|DIPLO_PACT))) {
        return 2;
    }

    int veh_atk_speed = veh_speed(veh_id_atk, 0); // Value is always limited to 0-255 range
    int veh_atk_moves = max(0, veh_atk_speed - veh_atk->moves_spent);
    int veh_atk_cost;
    if (combat_type & (CT_CAN_ARTY|CT_WEAPON_ONLY) && !(combat_type & ~(CT_CAN_ARTY|CT_WEAPON_ONLY))) {
        veh_atk_cost = veh_atk_moves;
    } else {
        veh_atk_cost = Rules->move_rate_roads;
    }
    if (conf.chopper_attack_rate > 1 && veh_atk->triad() == TRIAD_AIR
    && veh_atk->range() == 1 && !veh_atk->is_missile()) {
        veh_atk_cost = clamp(conf.chopper_attack_rate * Rules->move_rate_roads, veh_atk_cost, veh_atk_moves);
    }

    debug("battle_fight atk: %3d %2d %2d def: %3d %2d %2d %s / %s\n",
        veh_id_atk, x, y, veh_id_def, tx, ty, veh_atk->name(), veh_def->name());

    nerve_gas = 0;
    if (!psi_combat && has_abil(veh_atk->unit_id, ABL_NERVE_GAS)) {
        if (!*MultiplayerActive || *VehAttackFlags & 1) {
            veh_atk->state &= ~VSTATE_USED_NERVE_GAS;
            if (faction_id_atk == player_id && is_human(faction_id_atk)) {
                parse_says(0, veh_atk->name(), -1, -1);
                parse_says(1, Ability[ABL_ID_NERVE_GAS].name, -1, -1);
                BattleWin_stop_timer(BattleWin);
                if (popp(ScriptFile, "USENERVE", 0, "biohazd_sm.pcx", 0)) {
                    veh_atk->state |= VSTATE_USED_NERVE_GAS;
                }
                BattleWin_pulse_timer(BattleWin);
            }
            else if (!is_human(faction_id_atk) && use_nerve_gas(faction_id_atk, faction_id_def)) {
                veh_atk->state |= VSTATE_USED_NERVE_GAS;
            }
            if (*MultiplayerActive) {
                synch_veh(veh_id_atk);
            }
        }
        if (*VehAttackFlags & 2 && veh_atk->state & VSTATE_USED_NERVE_GAS) {
            nerve_gas = 1;
        }
    }
    if (base_id >= 0) {
        veh_atk->state &= ~VSTATE_UNK_40000;
    }
    if (!(veh_atk->state & VSTATE_UNK_1000) && *VehAttackFlags & 2) {
        // Possibly add more generic native unit condition
        if (faction_id_atk && veh_atk->unit_id != BSC_MIND_WORMS && veh_atk->unit_id != BSC_SPORE_LAUNCHER) {
            int modifier = clamp(veh_atk_moves, 0, Rules->move_rate_roads);
            offense_out = offense_out * modifier / Rules->move_rate_roads;
            int penalty = 100 * (modifier - Rules->move_rate_roads) / Rules->move_rate_roads;
            if (penalty) {
                add_bat(0, penalty, label_get(339)); // Hasty
            }
        }
        if (combat_type != CT_CAN_ARTY) {
            veh_atk->moves_spent += veh_atk_cost;
        }
    }
    veh_atk->state &= ~VSTATE_UNK_1000;
    veh_atk->state |= VSTATE_HAS_MOVED;
    veh_def->state |= VSTATE_HAS_MOVED;
    if (!(*VehAttackFlags & 2)) {
        return 0;
    }
    veh_atk->state |= VSTATE_UNK_40;

    if (!veh_atk->is_probe() && !veh_def->is_artifact()) {
        uint32_t atk_status = f_atk->diplo_status[faction_id_def];
        uint32_t def_status = f_def->diplo_status[faction_id_atk];
        if (is_human(faction_id_def)
        && ((def_status & (DIPLO_TRUCE|DIPLO_TREATY))
        || ((def_status & DIPLO_COMMLINK || atk_status & DIPLO_UNK_800)
        && !(def_status & DIPLO_VENDETTA)))) {
            if (faction_id_def == player_id || *PbemActive) {
                *plurality_default = 0;
                *gender_default = m_atk->is_leader_female;
                parse_says(0, m_atk->title_leader, -1, -1);
                parse_says(1, m_atk->name_leader, -1, -1);
                *plurality_default = m_atk->is_noun_plural;
                *gender_default = m_atk->noun_gender;
                parse_says(2, m_atk->noun_faction, -1, -1);
            }
            if (faction_id_def == player_id) {
                NetMsg_pop(NetMsg, "SURPRISE", -5000, 0, 0);
            } else if (*PbemActive) {
                POP2("SURPRISE", 0, -1 - faction_id_def);
            }
            f_def->diplo_spoke[faction_id_atk] = *CurrentTurn;
        }
        else if (((atk_status & DIPLO_TREATY) || (def_status & DIPLO_TREATY))
        && faction_id_atk != player_id && faction_id_def != player_id) {
            if (Factions[player_id].diplo_status[faction_id_def] & DIPLO_PACT) {
                treaty_report = 2;
            } else if (Factions[player_id].diplo_status[faction_id_atk] & DIPLO_PACT) {
                treaty_report = 3;
            } else if (*GameState & STATE_OMNISCIENT_VIEW) {
                treaty_report = 1;
            }
        }
        act_of_aggression(faction_id_atk, faction_id_def);
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id_atk && i != faction_id_def && sq_atk->owner == i) {
                if (f_atk->diplo_status[i] & DIPLO_PACT) {
                    if (!(f_def->diplo_status[i] & DIPLO_VENDETTA)
                    && (f_def->diplo_status[i] & DIPLO_COMMLINK)
                    && f_def->mil_strength_1 > Factions[i].mil_strength_1) {
                        treaty_on(i, faction_id_def, DIPLO_WANT_TO_TALK);
                        if (Factions[i].diplo_status[faction_id_def] & DIPLO_UNK_800000) {
                            cause_friction(faction_id_def, i, 3);
                        }
                    }
                }
            }
        }
    }
    if (nerve_gas) {
        offense_out += offense_out / 2;
        add_bat(0, 50, label_get(341)); // Nerve Gas
    }

    if ((faction_id_atk == player_id || faction_id_def == player_id)
    && psi_combat && !*MultiplayerActive && tut_check2(2)) {
        TutWin_tut_map(TutWin, "PSICOMBAT", tx, ty, (faction_id_atk == player_id ? veh_id_atk : veh_id_def), 0, 1, -1, -1);
    }

    // Added detailed display for difficulty and early base defense related combat modifiers
    int diff_val = 3 - (faction_id_atk != 0);
    if (f_def->diff_level < diff_val && is_human(faction_id_def) && !is_human(faction_id_atk)) {
        offense_out = offense_out * (f_def->diff_level + 1) / 4;
        add_bat(0, 100 * (f_def->diff_level + 1) / 4 - 100, Difficulty[f_def->diff_level]);
    }
    if (f_atk->diff_level < diff_val && is_human(faction_id_atk) && !is_human(faction_id_def)) {
        offense_out = offense_out * (4 - f_atk->diff_level) / 2;
        add_bat(0, 100 * (4 - f_atk->diff_level) / 2 - 100, Difficulty[f_atk->diff_level]);
    }
    if (!faction_id_atk) {
        if (!is_human(faction_id_def)) {
            if (veh_def->is_former()) {
                offense_out /= 2;
                // Removed additional AI bonus here for former units which further halved
                // native attacker offense based on various visibility conditions
            } else {
                offense_out = offense_out * clamp(f_def->ranking + 8, 8, 15) / 16;
            }
        }
        if (base_id >= 0) {
            int modifier = 1;
            if (faction_id_def == player_id) {
                interlude_base_attack = 1;
            }
            if (*CurrentTurn < 50) {
                offense_out /= 2;
                modifier *= 2;
            }
            if (f_def->base_count <= 1) {
                offense_out /= 2;
                modifier *= 2;
            }
            if (has_fac_built(FAC_HEADQUARTERS, base_id) && f_def->base_count < 4) {
                offense_out /= 2;
                modifier *= 2;
            }
            if (modifier > 1) {
                add_bat(0, 100 / modifier - 100, label_get(332)); // Base
            }
        }
    }
    if (*CurrentTurn < conf.native_weak_until_turn) {
        if (!faction_id_atk) {
            offense_out /= 3;
            add_bat(0, -67, label_get(312)); // Combat
        }
        if (!faction_id_def) {
            defense_out /= 2;
            add_bat(1, -50, label_get(312)); // Combat
        }
    }
    if (base_id >= 0 && veh_atk->triad() == TRIAD_AIR) {
        Bases[base_id].state_flags |= BSTATE_UNK_100000;
    }

    if (*VehAttackFlags & 1 && !intercept_state && is_human(faction_id_atk)
    && faction_id_atk == player_id && !combat_type && !veh_atk->is_missile()
    && !(veh_atk->state & (VSTATE_EXPLORE|VSTATE_ON_ALERT))) {
        if (2 * offense_out * veh_def_val * veh_atk_hp < defense_out * veh_atk_val * veh_def_hp) {
            SubInterface_set_iface_mode(BattleWin);
            BattleWin_clear(BattleWin);
            StatusWin_draw(StatusWin, veh_id_atk, veh_id_def, offense_out, defense_out, 0);
            *dword_8C6B3C = 1;
            parse_gen_name(faction_id_atk, 0, 1);
            BattleWin_stop_timer(BattleWin);
            if (!popp(ScriptFile, "BADIDEA", 0, "hasty_sm.pcx", 0)) {
                BattleWin_pulse_timer(BattleWin);
                if (*VehAttackFlags & 2) {
                    veh_atk->moves_spent -= veh_atk_cost;
                }
                veh_atk->reset_order();
                StatusWin_reset(StatusWin);
                StatusWin_redraw(StatusWin);
                return 1;
            }
            BattleWin_pulse_timer(BattleWin);
        }
        else if (*GameMorePreferences & MPREF_ADV_CONFIRM_ODDS_BF_ATTACKING) {
            SubInterface_set_iface_mode(BattleWin);
            BattleWin_clear(BattleWin);
            StatusWin_draw(StatusWin, veh_id_atk, veh_id_def, offense_out, defense_out, 0);
            *dword_8C6B3C = 1;
            parse_gen_name(faction_id_atk, 0, 1);

            StrBuffer[0] = '\0';
            snprintf(StrBuffer, StrBufLen, "%d.%d", offense_out >> 8, (10 * (offense_out & 0xFF)) >> 8);
            parse_says(2, StrBuffer, -1, -1);
            StrBuffer[0] = '\0';
            snprintf(StrBuffer, StrBufLen, "%d.%d", defense_out >> 8, (10 * (defense_out & 0xFF)) >> 8);
            parse_says(3, StrBuffer, -1, -1);

            int off_value = offense_out * veh_atk->cur_hitpoints();
            int def_value = defense_out * veh_def->cur_hitpoints();
            int divisor = std::__gcd(off_value, def_value);
            off_value /= divisor;
            def_value /= divisor;
            for (int value : {10000, 5000, 1000, 500, 100, 50}) {
                while (off_value >= 2*value && def_value >= 2*value) {
                    off_value /= value;
                    def_value /= value;
                }
            }
            combat_odds_fix(veh_atk, veh_def, &off_value, &def_value);
            BattleWin_stop_timer(BattleWin);
            parse_num(0, off_value);
            parse_num(1, def_value);
            const char* label = (veh_atk->damage_taken || veh_def->damage_taken ? "GOODIDEA2" : "GOODIDEA");
            if (!popp(ScriptFile, label, 0, "hasty_sm.pcx", 0)) {
                BattleWin_pulse_timer(BattleWin);
                if (*VehAttackFlags & 2) {
                    veh_atk->moves_spent -= veh_atk_cost;
                }
                veh_atk->reset_order();
                StatusWin_reset(StatusWin);
                StatusWin_redraw(StatusWin);
                return 1;
            }
            BattleWin_pulse_timer(BattleWin);
        }
    }
    if (nerve_gas) {
        atrocity(faction_id_atk, faction_id_def, base_id >= 0, 0);
        if (base_id >= 0) {
            strncpy(base_name, Bases[base_id].name, LineBufLen);
        }
    }
    if (veh_atk->order == ORDER_MOVE_TO) {
        veh_atk->reset_order();
    }
    bool plr_multi = (*MultiplayerActive && faction_id_def == player_id);
    bool plr_pbem = (*PbemActive && is_human(faction_id_def));
    int render_base = 0;
    int render_tile = 1;
    int render_battle = 0;
    if (faction_id_atk == player_id || faction_id_def == player_id || (*GameState & STATE_OMNISCIENT_VIEW)) {
        render_battle = 1;
    }
    if (faction_id_atk != player_id || (veh_atk->is_invisible_lurker() && !veh_atk->is_visible(player_id))) {
        if (faction_id_def != player_id || (veh_def->is_invisible_lurker() && !veh_def->is_visible(player_id))) {
            render_tile = 0;
        }
    }
    if (Factions[player_id].diplo_status[faction_id_atk] & DIPLO_PACT) {
        if(*GamePreferences & PREF_BSC_DONT_QUICK_MOVE_ALLY_VEH) {
            render_battle |= render_tile;
        }
    } else if (*GamePreferences & PREF_BSC_DONT_QUICK_MOVE_ENEMY_VEH) {
        render_battle |= render_tile;
    }
    assert(veh_atk->x != veh_def->x || veh_atk->y != veh_def->y);
    assert(veh_atk->faction_id != veh_def->faction_id);
    assert(veh_atk->is_probe() || veh_def->is_artifact() || at_war(faction_id_atk, faction_id_def));

    if (combat_type == CT_CAN_ARTY) {
        assert(can_arty(veh_atk->unit_id, true));
        add_goal(faction_id_def, AI_GOAL_PREV_DEFEND, 4, tx, ty, -1);
        add_goal(faction_id_def, AI_GOAL_ATTACK, 4, x, y, -1);

        if (base_id >= 0) {
            Bases[base_id].state_flags |= BSTATE_UNK_200000;
            Bases[base_id].event_flags |= BEVENT_UNK_100;
        }
        if (veh_id_def >= 0) {
            bool airbase = base_id >= 0 || (sq_def->items & BIT_AIRBASE);
            veh_id_def = veh_top(veh_id_def);
            while (veh_id_def >= 0) {
                veh_def = &Vehs[veh_id_def];
                if (!(airbase || veh_def->triad() != TRIAD_AIR
                || has_abil(veh_atk->unit_id, ABL_AIR_SUPERIORITY))) {
                    // Attack is not possible on this unit
                    veh_id_def = veh_def->next_veh_id_stack;
                    continue;
                }
                mod_veh_skip(veh_id_atk);
                mod_battle_compute(veh_id_atk, veh_id_def, &offense_out, &defense_out, 1);
                int def_value = max(1, defense_out * Rules->artillery_dmg_denominator);
                int damage_value = combat_rand(clamp(offense_out * Rules->artillery_dmg_numerator / def_value + 1, 1, 999));
                int damage_limit;
                if (base_id >= 0 || (sq_def->items & BIT_BUNKER)) {
                    damage_limit = (veh_def->max_hitpoints() * (100 - Rules->max_dmg_percent_arty_base_bunker) + 99) / 100;
                } else {
                    int val = Rules->max_dmg_percent_arty_open;
                    if (veh_def->triad() != TRIAD_LAND) {
                        val = Rules->max_dmg_percent_arty_sea;
                    }
                    damage_limit = (veh_def->max_hitpoints() * (100 - val) + 99) / 100;
                }
                damage_value = max(0, min(damage_value, veh_def->cur_hitpoints() - damage_limit));
                if (render_battle) {
                    combat_type = 0;
                    *VehDrawAttackID = veh_id_atk;
                    *VehDrawDefendID = veh_id_def;
                    veh_atk->visibility |= (1 << faction_id_def);
                    if (!Console_focus(MapWin, tx, ty, faction_id_atk)) {
                        draw_tile(x, y, 2);
                        draw_tile(tx, ty, 2);
                    }
                    SubInterface_set_iface_mode(BattleWin);
                    BattleWin_clear(BattleWin);
                    StatusWin_draw(StatusWin, veh_id_atk, veh_id_def, offense_out, defense_out, 1);
                    *dword_8C6B3C = 1;
                    flush_input();
                    if (!(veh_id_def >= 0 && damage_limit && veh_def->damage_taken == damage_limit
                    && veh_def->next_veh_id_stack >= 0)) {
                        boom(tx, ty, 0);
                        combat_type = CT_CAN_ARTY;
                    }
                }
                veh_def->damage_taken += damage_value;
                veh_def->state |= VSTATE_HAS_MOVED;
                if (veh_def->is_former()) {
                    veh_def->movement_turns = 0;
                }
                if (base_id < 0) {
                    veh_def->state |= VSTATE_UNK_8; // Veh needs repair for AI?
                }
                if (render_battle) {
                    StatusWin_draw(StatusWin, veh_id_atk, veh_id_def, offense_out, defense_out, 1);
                    draw_tile(tx, ty, 2);
                    reset_veh_draw();
                    if (*GameMorePreferences & MPREF_ADV_PAUSE_AFTER_BATTLES) {
                        BattleWin_stop_timer(BattleWin);
                        popp(ScriptFile, "BATTLEPAUSE", 0, "batwon_sm.pcx", 0);
                        BattleWin_pulse_timer(BattleWin);
                        if (*DialogToggle) {
                            *GameMorePreferences &= ~MPREF_ADV_PAUSE_AFTER_BATTLES;
                            prefs_save(0);
                        }
                    } else if (!shift_key_down()
                    && !(*GamePreferences & PREF_ADV_FAST_BATTLE_RESOLUTION)
                    && !*MultiplayerActive
                    && combat_type) {
                        clock_wait(200);
                    }
                }
                veh_id_def = veh_def->next_veh_id_stack;
            }
        }
        int iter_veh_id = stack_fix(veh_at(tx, ty));
        while (iter_veh_id >= 0) {
            if (Vehs[iter_veh_id].cur_hitpoints() > 0) {
                iter_veh_id = Vehs[iter_veh_id].next_veh_id_stack;
            } else {
                stack_veh(iter_veh_id, 2);
                stack_kill(iter_veh_id);
                iter_veh_id = stack_fix(veh_at(tx, ty));
                f_atk->diplo_unk_3[faction_id_def]++;
                f_atk->diplo_unk_4[faction_id_def]++;
            }
        }
        draw_tile_fixup(tx, ty, 1, 2);
        if (base_id >= 0 && !is_human(faction_id_def)) {
            mod_base_reset(base_id, 0);
        }
        return 0; // Always return here
    }
    assert(map_range(veh_atk, veh_def) == 1 || can_arty(veh_atk->unit_id, true));
    if (combat_type) {
        add_goal(faction_id_def, AI_GOAL_PREV_DEFEND, 2, tx, ty, -1);
    }
    if (render_battle) {
        if (*MultiplayerActive && *dword_6E8150) {
            render_base = 1;
        }
        veh_atk->visibility |= (1 << faction_id_def);
        *VehDrawAttackID = veh_id_atk;
        *VehDrawDefendID = veh_id_def;
        if (!Console_focus(MapWin, tx, ty, faction_id_atk)) {
            draw_tile(x, y, 2);
            draw_tile(tx, ty, 2);
        }
        stack_veh(veh_id_atk, 1);
        SubInterface_set_iface_mode(BattleWin);
        BattleWin_clear(BattleWin);
        StatusWin_draw(StatusWin, veh_id_atk, veh_id_def, offense_out, defense_out, combat_type);
        *dword_8C6B3C = 1;
        flush_input();
        if (!shift_key_down() && !(combat_type & CT_CAN_ARTY)) {
            veh_scoot(veh_id_atk, x, y, offset, 1);
        }
        stack_put(veh_id_atk, x, y);
        draw_tile(x, y, 2);
        draw_tile(tx, ty, 2);
    }
    int retreat_value = 0;
    if (!combat_type && veh_speed(veh_id_def, 1) > veh_speed(veh_id_atk, 1)) {
        if (veh_def->offense_value() && !(veh_def->state & VSTATE_UNK_40)) {
            if (veh_atk->triad() != TRIAD_AIR
            && veh_def->triad() != TRIAD_AIR
            && (!has_abil(veh_atk->unit_id, ABL_COMM_JAMMER)
            || has_abil(veh_def->unit_id, ABL_DISSOCIATIVE_WAVE))
            && veh_def->order != ORDER_HOLD
            && base_id < 0
            && !(sq_def->items & (BIT_AIRBASE|BIT_BUNKER))
            && mod_stack_check(veh_id_def, 1, -1, -1, -1) == 1) {
                retreat_value = (veh_def->cur_hitpoints() + 1) / 2;
            }
        }
    }
    if (has_abil(veh_atk->unit_id, ABL_MARINE_DETACHMENT)) {
        if (veh_atk->triad() == TRIAD_SEA && is_ocean(mapsq(tx, ty))) {
            if (veh_def->triad() == TRIAD_SEA && !veh_def->is_native_unit() && base_id < 0) {
                detach_value = 2 * veh_def->cur_hitpoints() / 3;
            }
        }
    }
    if (veh_atk->is_planet_buster() && Units[veh_atk->unit_id].reactor_id) {
        planet_busting(veh_id_atk, tx, ty);
        *GameDrawState |= 4;
        return 0;
    }
    int num_killed;
    int num_relics;
    int num_damaged;
    int damage_mod = 3;
    int combat_value = 0;
    if (veh_atk->is_missile() || *MultiplayerActive) {
        damage_mod = 0;
        boom(tx, ty, (veh_def->cur_hitpoints() ? 1 : 2));
    }
    int single_round = 0;
    if ((combat_type & CT_WEAPON_ONLY) && ((veh_atk->triad() == TRIAD_SEA) != (veh_def->triad() == TRIAD_SEA))) {
        single_round = 1;
    }
    int draw_x = 0;
    int draw_y = 0;

    while (veh_atk->cur_hitpoints() > 0 && veh_def->cur_hitpoints() > 0) {
        int off_rand = combat_rand(offense_out);
        int def_rand = combat_rand(defense_out);
        if (off_rand <= def_rand) {
            veh_atk->damage_taken += veh_atk_val;
            if (render_battle && damage_mod
            && veh_atk->damage_taken / (veh_atk_val * damage_mod)
            != (veh_atk->damage_taken - veh_atk_val) / (veh_atk_val * damage_mod)) {
                combat_value = 1;
                StatusWin_draw(StatusWin, veh_id_atk, veh_id_def, offense_out, defense_out, combat_type);
                boom(x, y, 9);
                if (veh_atk->cur_hitpoints() > 0) {
                    draw_tile(x, y, 2);
                    draw_tile(tx, ty, 2);
                }
                if (single_round) {
                    break;
                }
            }
        } else {
            veh_def->damage_taken += veh_def_val;
            if (render_battle && damage_mod
            && veh_def->damage_taken / (veh_def_val * damage_mod)
            != (veh_def->damage_taken - veh_def_val) / (veh_def_val * damage_mod)) {
                combat_value = 2;
                StatusWin_draw(StatusWin, veh_id_atk, veh_id_def, offense_out, defense_out, combat_type);
                boom(tx, ty, 1);
                if (veh_def->cur_hitpoints() > 0) {
                    draw_tile(x, y, 2);
                    draw_tile(tx, ty, 2);
                }
                if (single_round) {
                    break;
                }
            }
            if (detach_value && veh_def->cur_hitpoints() > 0) {
                detach_value -= veh_def_val;
                if (detach_value <= 0 && combat_rand(100) < 50) {
                    int capture_id = -1;
                    if (veh_id_def >= 0) {
                        int iter_veh_id = veh_top(veh_id_def);
                        int iter_base_id = base_find(tx, ty);
                        while (iter_veh_id >= 0) {
                            VEH* veh = &Vehs[iter_veh_id];
                            capture_id = veh->faction_id;
                            parse_says(0, veh->name(), -1, -1);
                            veh->faction_id = veh_atk->faction_id;
                            if (Bases[iter_base_id].faction_id != veh_atk->faction_id
                            || Bases[iter_base_id].pop_size < 3) {
                                veh->home_base_id = -1;
                            } else {
                                veh->home_base_id = iter_base_id;
                            }
                            debug("battle_fight detach %s -> %s\n", veh_atk->name(), veh->name());
                            mod_veh_skip(iter_veh_id);
                            iter_veh_id = veh->next_veh_id_stack;
                        };
                    }
                    bit_set(tx, ty, BIT_SUPPLY_REMOVE, 1);
                    owner_set(tx, ty, veh_atk->faction_id);
                    if (veh_atk->faction_id == player_id) {
                        draw_tile_fixup(tx, ty, 1, 2);
                    } else {
                        draw_tile(tx, ty, 2);
                    }
                    if (veh_atk->faction_id == player_id) {
                        parse_says(1, MFactions[capture_id].adj_name_faction, -1, -1);
                        popp(ScriptFile, "WEMARINEDETACH", 0, "navun_sm.pcx", 0);
                    }
                    else if (capture_id == player_id) {
                        parse_says(1, MFactions[veh_atk->faction_id].adj_name_faction, -1, -1);
                        popp(ScriptFile, "THEYMARINEDETACH", 0, "navun_sm.pcx", 0);
                    }
                    marine_detach = 1;
                    break;
                }
            }
            if (retreat_value && veh_def->cur_hitpoints() > 0) {
                retreat_value -= veh_def_val;
                if (retreat_value <= 0) {
                    retreat_value = 0;
                    int move_index = -1;
                    int move_value = 0;
                    int triad = veh_def->triad();
                    int val1 = 0, val2 = 0;
                    for (auto& m : iterate_tiles(tx, ty, 1, 9)) {
                        if ((triad == TRIAD_AIR || (is_ocean(m.sq) == (triad == TRIAD_SEA)))
                        && !mod_zoc_move(m.x, m.y, faction_id_def) && !goody_at(m.x, m.y)) {
                            int index = m.i - 1; // TableOffset -> BaseOffset
                            if (!m.sq->is_fungus()
                            || has_project(FAC_PHOLUS_MUTAGEN, faction_id_def)
                            || veh_def->is_native_unit()) {
                                int owner = m.sq->veh_owner();
                                if (!(m.sq->items & (BIT_BASE_IN_TILE|BIT_VEH_IN_TILE))
                                || owner < 0 || owner == faction_id_def
                                || Factions[owner].diplo_status[faction_id_def] & DIPLO_PACT) {
                                    if (index - (offset ^ 4) < 0) {
                                        val1 = (val1 & ~0xFF) | ((offset ^ 4) - index);
                                    } else {
                                        val1 = index - (offset ^ 4);
                                    }
                                    val2 = val1 & 7;
                                    if (val2 > 4) {
                                        val2 = 8 - val2;
                                    }
                                    if (val2 > 2 && val2 > move_value) {
                                        move_value = val2;
                                        move_index = index;
                                    }
                                }
                            }
                        }
                    }
                    if (move_index >= 0) {
                        draw_x = wrap(tx + BaseOffsetX[move_index]);
                        draw_y = ty + BaseOffsetY[move_index];
                        stack_put(stack_veh(veh_id_def, 1), draw_x, draw_y);
                        def_has_moved = 1;
                        debug("battle_fight retreat %2d %2d -> %2d %2d %s\n",
                            tx, ty, draw_x, draw_y, veh_def->name());
                        break;
                    }
                }
            }
        }
    }
    // Modified hitpoints after combat
    int veh_atk_last_hp = veh_atk->cur_hitpoints();
    int veh_def_last_hp = veh_def->cur_hitpoints();
    bool atk_alive = veh_atk_last_hp > 0;
    bool def_alive = veh_def_last_hp > 0;

    if (render_battle || render_tile) {
        draw_tile_fixup2(x, y, tx, ty, 2, 2);
        if (def_has_moved && (x != draw_x || y != draw_y) && (tx != draw_x || ty != draw_y)) {
            draw_tile_fixup(draw_x, draw_y, 2, 2);
        }
    }
    if (render_battle) {
        StatusWin_draw(StatusWin, veh_id_atk, veh_id_def, offense_out, defense_out, combat_type);

        if (!def_has_moved && atk_alive) {
            if (single_round) {
                if (combat_value != 1 && !*MultiplayerActive) { // Defender wins
                    boom(x, y, ((atk_alive ? 1 : 2) | 8));
                }
                boom(tx, ty, ((def_alive ? 1 : 2) | 8));
            } else if (combat_value == 2) { // Attacker wins
                if (*MultiplayerActive || !def_alive) {
                    boom(tx, ty, ((def_alive ? 0 : 2) | 8));
                }
            } else if (!*MultiplayerActive) {
                boom(tx, ty, ((def_alive ? 1 : 2) | 8));
            } else {
                boom(tx, ty, ((def_alive ? 0 : 2) | 8));
            }
        } else if (!def_has_moved) {
            if (single_round) {
                if (combat_value != 2 && !*MultiplayerActive) { // Attacker wins
                    boom(tx, ty, (def_alive ? 1 : 0));
                }
                boom(x, y, (atk_alive ? 1 : 2));
            } else if (combat_value == 1) { // Defender wins
                if (*MultiplayerActive || !atk_alive) {
                    boom(x, y, (atk_alive ? 0 : 2));
                }
            } else if (!*MultiplayerActive) {
                boom(x, y, (atk_alive ? 1 : 2));
            } else {
                boom(x, y, (atk_alive ? 0 : 2));
            }
        }
        reset_veh_draw();
    }
    if (single_round && veh_atk_last_hp > 0 && veh_def_last_hp > 0) {
        if (plr_multi) {
            parse_says(0, veh_def->name(), -1, -1);
            StrBuffer[0] = '\0';
            say_stats_3(StrBuffer, veh_def->unit_id);
            parse_says(1, StrBuffer, -1, -1);
            parse_says(2, m_atk->adj_name_faction, -1, -1);
            parse_says(3, veh_atk->name(), -1, -1);
            StrBuffer[0] = '\0';
            say_stats_3(StrBuffer, veh_atk->unit_id);
            parse_says(4, StrBuffer, -1, -1);
            int tgt_base_id = mod_base_find3(tx, ty, -1, -1, -1, player_id);
            if (tgt_base_id >= 0) {
                parse_says(5, Bases[tgt_base_id].name, -1, -1);
            }
            net_pop("COMBATSURVIVE", 0);
        }
        return 0;
    }
    MAP* sq = mapsq(tx, ty);
    if (!is_ocean(sq) && sq->owner >= 0) { // Added owner bounds checking
        if (sq->owner != faction_id_atk
        && sq->owner != faction_id_def
        && Factions[sq->owner].diplo_status[faction_id_def] & DIPLO_VENDETTA
        && Factions[sq->owner].diplo_status[faction_id_atk] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
            if (!Factions[faction_id_atk].diplo_gifts[sq->owner]) {
                Factions[faction_id_atk].diplo_gifts[sq->owner] = 1;
            }
            cause_friction(sq->owner, faction_id_atk, -1);
        }
    }
    // Removed likely unneeded feature that changed home_base_id when native faction was involved in combat
    if (atk_alive) {
        // Added reactor type bounds checking
        offense_out = offense_out * veh_atk_last_hp * veh_atk->reactor_type() / veh_atk_hp;
        defense_out = defense_out * veh_def_hp * veh_def->reactor_type() / veh_def->max_hitpoints();
        if (!veh_def->is_artifact()) {
            if (mod_morale_veh(veh_id_atk, 1, 0) <= MORALE_GREEN
            || combat_rand(offense_out + defense_out) <= defense_out) {
                promote(veh_id_atk);
            }
        }
    } else {
        offense_out = offense_out * veh_atk_hp * veh_atk->reactor_type() / veh_atk->max_hitpoints();
        defense_out = defense_out * veh_def_last_hp * veh_def->reactor_type() / veh_def_hp;
        if (!veh_atk->is_artifact()) {
            if (mod_morale_veh(veh_id_def, 1, 0) <= MORALE_GREEN
            || combat_rand(offense_out + defense_out) <= offense_out
            || (base_id >= 0 && mod_morale_veh(veh_id_def, 1, 0) <= MORALE_HARDENED)) {
                promote(veh_id_def);
            }
        }
    }
    num_killed = 0;
    num_relics = 0;
    num_damaged = 0;
    credits_income = 0;

    if (marine_detach || def_has_moved) {
        goto END_BATTLE;
    } else {
        if (!atk_alive) {
            dword_93A96C[faction_id_def]++;
            dword_93A98C[faction_id_atk]++;
            if (plr_multi || plr_pbem) {
                parse_says(0, veh_def->name(), -1, -1);
                StrBuffer[0] = '\0';
                say_stats_3(StrBuffer, veh_def->unit_id);
                parse_says(1, StrBuffer, -1, -1);
                parse_says(2, m_atk->adj_name_faction, -1, -1);
                parse_says(3, veh_atk->name(), -1, -1);
                StrBuffer[0] = '\0';
                say_stats_3(StrBuffer, veh_atk->unit_id);
                parse_says(4, StrBuffer, -1, -1);
                int tgt_base_id = mod_base_find3(tx, ty, -1, -1, -1, player_id);
                if (tgt_base_id >= 0) {
                    parse_says(5, Bases[tgt_base_id].name, -1, -1);
                }
                if (plr_multi) {
                    net_pop("COMBATWIN", 0);
                } else if (plr_pbem) {
                    POP3("COMBATWIN", 0, faction_id_def, tx, ty);
                }
            }
            stack_veh(veh_id_atk, 1);
            battle_kill_stack(veh_id_atk, &num_killed, &num_relics, &credits_income, -1, faction_id_def);
            f_def->diplo_unk_3[faction_id_atk]++;
            f_def->diplo_unk_4[faction_id_atk]++;
            goto END_UPKEEP;
        }
        f_atk->diplo_unk_3[faction_id_def]++;
        f_atk->diplo_unk_4[faction_id_def]++;
        dword_93A96C[faction_id_atk]++;
        dword_93A98C[faction_id_def]++;
        if (plr_multi || plr_pbem) {
            parse_says(0, veh_def->name(), -1, -1);
            StrBuffer[0] = '\0';
            say_stats_3(StrBuffer, veh_def->unit_id);
            parse_says(1, StrBuffer, -1, -1);
            parse_says(2, m_atk->adj_name_faction, -1, -1);
            parse_says(3, veh_atk->name(), -1, -1);
            StrBuffer[0] = '\0';
            say_stats_3(StrBuffer, veh_atk->unit_id);
            parse_says(4, StrBuffer, -1, -1);
            int tgt_base_id = mod_base_find3(tx, ty, -1, -1, -1, faction_id_def);
            if (tgt_base_id >= 0) {
                parse_says(5, Bases[tgt_base_id].name, -1, -1);
            }
            if (plr_multi) {
                net_pop("COMBATLOSS", 0);
            } else if (plr_pbem) {
                POP3("COMBATLOSS", 0, faction_id_def, tx, ty);
            }
        }
        bool check_stack = true;
        if (base_id >= 0
        || sq_def->items & BIT_BUNKER
        || veh_def->triad() == TRIAD_AIR
        || veh_atk->triad() == TRIAD_AIR
        || veh_atk->is_probe()
        || veh_def->plan() == PLAN_NAVAL_SUPERIORITY
        || (!is_ocean(sq_def) && mod_stack_check(veh_id_def, 2, PLAN_COLONY, -1, -1))) {
            if (!veh_def->is_artifact()) {
                if (!faction_id_atk
                || ((!veh_cargo(veh_id_def) || base_id >= 0 || veh_atk->is_probe())
                && (veh_atk->is_probe() || (stack_kill_check(veh_id_def) > 1)))) {
                    // Skip collateral damage
                    battle_kill(veh_id_def, &num_killed, &num_relics, &credits_income, veh_id_atk, faction_id_atk);
                    veh_id_def = -1;
                    check_stack = false;
                }
            }
        }
        if (veh_id_def >= 0 && check_stack) {
            stack_veh(veh_id_def, 0);
            int iter_veh_id = veh_top(veh_id_def);
            while (iter_veh_id >= 0) {
                VEH* veh = &Vehs[iter_veh_id];
                if (veh->triad() == TRIAD_LAND && is_ocean(sq_def) && !sq_def->is_base()
                && (veh->order != ORDER_SENTRY_BOARD
                || veh->waypoint_x[0] == veh_id_def
                || veh->waypoint_x[0] < 0)) {
                    veh->damage_taken = veh->max_hitpoints();
                    num_damaged++;
                }
                else if (conf.collateral_damage_value && (iter_veh_id == veh_id_def
                || !(veh->is_colony() || veh->is_probe() || veh->is_supply() || veh->is_artifact()))) {
                    int value;
                    if (veh_atk->offense_value() >= 0) {
                        value = veh_atk->reactor_type();
                    } else {
                        value = veh->reactor_type();
                    }
                    veh->damage_taken += conf.collateral_damage_value * value;
                    veh->state |= VSTATE_HAS_MOVED;
                    num_damaged++;
                }
                iter_veh_id = veh->next_veh_id_stack;
            }
            iter_veh_id = veh_top(veh_id_def);
            while (iter_veh_id >= 0) {
                VEH* veh = &Vehs[iter_veh_id];
                int next_veh_id = veh->next_veh_id_stack;
                if (veh->cur_hitpoints() <= 0 || !veh->faction_id) {
                    battle_kill(iter_veh_id, &num_killed, &num_relics, &credits_income, veh_id_atk, faction_id_atk);
                    if (iter_veh_id == veh_id_def) {
                        veh_id_def = -1;
                    }
                    if (iter_veh_id < veh_id_def) {
                        veh_id_def--;
                    }
                    if (iter_veh_id < next_veh_id) {
                        next_veh_id--;
                    }
                }
                iter_veh_id = next_veh_id;
            }
        }
        if (check_stack) {
            num_damaged = max(0, num_damaged - num_killed - num_relics);
            veh_id_def = stack_fix(veh_at(tx, ty));
            if (veh_id_def >= 0 && stack_kill_check(veh_id_def) == 0) {
                battle_kill_stack(veh_id_def, &num_killed, &num_relics, &credits_income, veh_id_atk, faction_id_atk);
                num_damaged = 0;
            }
        }
        if (base_id < 0) {
            goto END_BATTLE;
        }
        BASE* base = &Bases[base_id];
        base->state_flags |= BSTATE_COMBAT_LOSS_LAST_TURN;
        // Fix: Original checked for Citizens Defense Force built by faction_id_def instead of base owner
        if (!is_ocean(sq_def) // Fix: check for base tile instead of sq_atk
        && (base->pop_size > 1 || !is_objective(base_id))
        && !has_facility(FAC_PERIMETER_DEFENSE, base_id)
        && !has_fac_built(FAC_TACHYON_FIELD, base_id)
        && combat_defend
        && faction_id_atk
        && veh_who(tx, ty) < 0
        && (f_def->diff_level || !is_human(faction_id_def))) {
            base->pop_size--;
            nerve_gas = 0;
        }
        if (nerve_gas) {
            base->pop_size -= (base->pop_size + 1) / 2;
            if (base->pop_size < 1 && is_objective(base_id)) {
                base->pop_size = 1;
            }
        }
        if (base->pop_size > 0) {
            base->factions_pop_size_intel[faction_id_atk] = base->pop_size;
            if (!is_human(faction_id_def)) {
                mod_base_reset(base_id, 0);
            }
            if (nerve_gas) {
                spot_base(base_id, faction_id_atk);
            }
        } else {
            mod_base_kill(base_id);
            base_id = -1;
            for (int i = 0; i < MaxPlayerNum; i++) {
                synch_bit(tx, ty, i);
            }
            draw_map(1);
            if (mod_eliminate_player(faction_id_def, faction_id_atk) != 0) {
                treaty_report = 0;
            }
        }
        if (base_id >= 0 && veh_who(tx, ty) < 0 && is_human(faction_id_def)
        && !conf.factions_enabled) { // Function not relevant for Thinker
            invasions(base_id);
        }
    } // if (!marine_detach && !def_has_moved)

END_UPKEEP:
    if (faction_id_atk && faction_id_def) {
        mon_enemy_destroyed(faction_id_atk, faction_id_def);
    }
    if (num_relics && !is_human(faction_id_def)) {
        if (!has_treaty(faction_id_atk, faction_id_def, DIPLO_VENDETTA)) {
            cause_friction(faction_id_def, faction_id_atk, 5);
            treaty_on(faction_id_atk, faction_id_def, DIPLO_WANT_TO_TALK);
        }
    }

END_BATTLE:
    if (atk_alive && credits_income) {
        f_atk->energy_credits += credits_income;
        Console_update_data(MapWin, 0);
        // Increase morale level for other native units nearby
        for (auto& m : iterate_tiles(tx, ty, 1, 9)) {
            for (int iter_veh_id = veh_at(m.x, m.y); iter_veh_id >= 0; ) {
                if (!Vehs[iter_veh_id].faction_id) {
                    Vehs[iter_veh_id].set_lifecycle_value(
                        Vehs[iter_veh_id].lifecycle_value() + 1);
                }
                iter_veh_id = Vehs[iter_veh_id].next_veh_id_stack;
            }
        }
    }
    if (render_battle) {
        if (atk_alive) {
            draw_tile(x, y, 1);
            draw_tile_fixup(tx, ty, 2, 1);
        } else {
            draw_tile_fixup(x, y, 2, 1);
            draw_tile(tx, ty, 1);
        }
        do_all_draws();
        if (atk_alive && credits_income && faction_id_atk == player_id) {
            parse_num(0, credits_income);
            net_pop("HUSKS", 0);
        } else if ((num_killed > 1 || num_damaged)
        && (faction_id_atk == player_id || faction_id_def == player_id)) {
            parse_num(0, num_killed);
            parse_num(1, num_damaged);
            StrBuffer[0] = '\0';
            if ((faction_id_atk == player_id) == (atk_alive != 0)) {
                strncat(StrBuffer, "NUMGOT", StrBufLen);
            } else {
                strncat(StrBuffer, "NUMLOST", StrBufLen);
            }
            if (num_killed <= 1) {
                if (num_damaged) {
                    strncat(StrBuffer, "1", StrBufLen);
                }
            } else if (num_damaged) {
                strncat(StrBuffer, "2", StrBufLen);
            }
            net_pop(StrBuffer, 0);
        }
        if (num_relics) {
            parse_num(0, num_relics);
            if (faction_id_def == player_id) {
                StrBuffer[0] = '\0';
                if (has_treaty(faction_id_atk, faction_id_def, DIPLO_VENDETTA)) {
                    snprintf(StrBuffer, StrBufLen, "THEIRBOOTY%d", clamp(num_relics, 1, 2));
                } else { // Alien artifact captured
                    parse_says(0, parse_set(faction_id_atk), -1, -1);
                    snprintf(StrBuffer, StrBufLen, "THEIRBOOTY3");
                }
                net_pop(StrBuffer, 0);
            } else if (faction_id_atk == player_id) {
                StrBuffer[0] = '\0';
                snprintf(StrBuffer, StrBufLen, "OURBOOTY%d", clamp(num_relics, 1, 2));
                net_pop(StrBuffer, 0);
            }
        }
        if (nerve_gas && strlen(base_name) && atk_alive) {
            parse_says(0, base_name, -1, -1);
            net_pop("NERVEGAS", 0);
        }
        if (def_has_moved) {
            parse_says(0, Vehs[veh_id_def].name(), -1, -1);
            parse_says(1, m_def->adj_name_faction, -1, -1);
            if (faction_id_def == player_id) {
                net_pop("WEDISENGAGED", 0);
            } else if (faction_id_atk == player_id) {
                net_pop("THEYDISENGAGED", 0);
            }
        }
        StatusWin_redraw(StatusWin);
        if (render_base) {
            BaseWin_focus(BaseWin);
            GraphicWin_redraw(MainWin);
            do_all_draws();
        }
        if (*GameMorePreferences & MPREF_ADV_PAUSE_AFTER_BATTLES) {
            BattleWin_stop_timer(BattleWin);
            popp(ScriptFile, "BATTLEPAUSE", 0, "batwon_sm.pcx", 0);
            BattleWin_pulse_timer(BattleWin);
            if (*DialogToggle) {
                *GameMorePreferences &= ~MPREF_ADV_PAUSE_AFTER_BATTLES;
                prefs_save(0);
            }
        }
    }
    switch (treaty_report) {
    case 1:
        parse_says(0, m_atk->adj_name_faction, -1, -1);
        parse_says(1, parse_set(faction_id_def), -1, -1);
        NetMsg_pop2("VENDETTAREPORT", 0);
        break;
    case 2:
        parse_says(0, m_atk->adj_name_faction, -1, -1);
        parse_says(1, parse_set(faction_id_def), -1, -1);
        NetMsg_pop2("PACTATTACKED", 0);
        break;
    case 3:
        parse_says(0, parse_set(faction_id_atk), -1, -1);
        parse_says(1, parse_set(faction_id_def), -1, -1);
        NetMsg_pop2("PACTATTACKING", 0);
        break;
    }
    Units[veh_atk->unit_id].combat_factions |= (1 << faction_id_def);
    Units[veh_def->unit_id].combat_factions |= (1 << faction_id_atk);
    if (interlude_base_attack) {
        interlude(0, 0, 1, 0);
    }
    if (!atk_alive) {
        return 0;
    }
    assert(*CurrentVehID >= 0 && *CurrentVehID < *VehCount);
    VEH* veh = &Vehs[*CurrentVehID];
    int veh_range = veh->range();
    if (veh->triad() == TRIAD_SEA || (veh->triad() == TRIAD_AIR && veh_range != 1)) {
        mod_veh_skip(*CurrentVehID); // TODO check why increase moves_spent after veh_skip
        veh->moves_spent += Rules->move_rate_roads;
        veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
    }
    int result = 1;
    if (veh->is_missile()) {
        kill(*CurrentVehID);
        result = 0;
    }
    if (is_human(faction_id_atk)
    || base_id < 0
    || veh_who(tx, ty) >= 0
    || veh->triad() != TRIAD_AIR
    || veh_range == 0) {
        return result;
    }
    return (veh->movement_turns + 1 < veh_range ? result : 1);
}



