
#include "veh_combat.h"


/*
These functions are used to patch more features on get_basic_offense or get_basic_defense.
*/
int __cdecl fungal_tower_bonus(int value) {
    return value * conf.fungal_tower_bonus / 100;
}

int __cdecl dream_twister_bonus(int value) {
    return value * conf.dream_twister_bonus / 100;
}

int __cdecl neural_amplifier_bonus(int value) {
    return value * conf.neural_amplifier_bonus / 100;
}

/*
Calculate the psi combat factor for an attacking or defending unit.
*/
int __cdecl psi_factor(int value, int faction_id, bool is_attack, bool is_fungal_twr) {
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
Calculate the lifecycle (morale) of the specified planet-owned native unit.
*/
int __cdecl mod_morale_alien(int veh_id, int faction_id_vs_native) {
    int morale;
    // Fungal Tower specific code, shifted to start and added veh_id check to prevent crash
    if (veh_id >= 0 && Vehs[veh_id].unit_id == BSC_FUNGAL_TOWER) {
        morale = 0;
        int16_t x = Vehs[veh_id].x;
        int16_t y = Vehs[veh_id].y;
        // similar to is_coast() > except with fungus check + Ocean Shelf included
        for (int i = TableRange[0]; i < TableRange[1]; i++) {
            int x2 = wrap(x + TableOffsetX[i]);
            int y2 = y + TableOffsetY[i];
            MAP* sq = mapsq(x2, y2);
            if (sq && sq->is_fungus() && sq->alt_level() >= ALT_OCEAN_SHELF) {
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
            morale += (Vehs[veh_id].flags & VFLAG_ADD_ONE_MORALE ? 1 : 0);
            morale += (Vehs[veh_id].flags & VFLAG_ADD_TWO_MORALE ? 2 : 0);
        }
    }
    morale = clamp(morale, 0, 6);
//    assert(morale == morale_alien(veh_id, faction_id_vs_native));
    return morale;
}

/*
Calculate unit morale. TODO: Determine if 2nd param is a toggle for display vs actual morale.
*/
int __cdecl mod_morale_veh(int veh_id, bool check_drone_riot, int faction_id_vs_native) {
    int faction_id = Vehs[veh_id].faction_id;
    int unit_id = Vehs[veh_id].unit_id;
    int home_base_id = Vehs[veh_id].home_base_id;
    int value;
    if (!conf.modify_unit_morale) {
        return morale_veh(veh_id, check_drone_riot, faction_id_vs_native);
    }
    if (!faction_id) {
        return mod_morale_alien(veh_id, faction_id_vs_native);
    }
    if (Units[unit_id].plan == PLAN_INFO_WARFARE) {
        int probe_morale = clamp(Factions[faction_id].SE_probe, 0, 3);
        probe_morale += has_project(FAC_TELEPATHIC_MATRIX, faction_id) ? 2 : 0;
        for (int i = 0; i < MaxTechnologyNum; i++) {
            if (Tech[i].flags & TFLAG_IMPROVE_PROBE && has_tech(i, faction_id)) {
                probe_morale++;
            }
        }
        probe_morale += Vehs[veh_id].morale;
        value = clamp(probe_morale, 2, 6);
        return value;
    }
    if (unit_id < MaxProtoFactionNum && Units[unit_id].offense_value() < 0) {
        value = clamp((int)Vehs[veh_id].morale, 0, 6); // Basic Psi Veh
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
    value = clamp(Vehs[veh_id].morale + morale_modifier, 0, 6);
    return value;
}

/*
Get the basic offense value for an attacking unit with an optional defender unit parameter.
*/
int __cdecl mod_get_basic_offense(int veh_id_atk, int veh_id_def, int psi_combat_type, bool is_bombard, bool unk_tgl) {
    // unk_tgl is only set to true in battle_compute when veh_id_atk and veh_id_def are switched places.
    int faction_id_atk = Vehs[veh_id_atk].faction_id;
    int unit_id_atk = Vehs[veh_id_atk].unit_id;
    int morale;
    if (faction_id_atk) {
        morale = mod_morale_veh(veh_id_atk, true, 0);
    } else {
        morale = mod_morale_alien(veh_id_atk, veh_id_def >= 0 ? Vehs[veh_id_def].faction_id : -1);
    }
    int base_id_atk = base_at(Vehs[veh_id_atk].x, Vehs[veh_id_atk].y);
    if (base_id_atk >= 0) {
        // Morale effects with CC/BP are modified. SE Morale has no longer effect on native units.
        bool native_unit = unit_id_atk < MaxProtoFactionNum
            && (Units[unit_id_atk].offense_value() < 0 || unit_id_atk == BSC_SPORE_LAUNCHER);
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
                    if ((unit_id_atk >= MaxProtoFactionNum
                    || (Units[unit_id_atk].offense_value() >= 0
                    && unit_id_atk != BSC_SPORE_LAUNCHER))
                    && !has_abil(unit_id_atk, ABL_DISSOCIATIVE_WAVE)
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
    int offense = offense_proto(unit_id_atk, veh_id_def, is_bombard);
    if (psi_combat_type) {
        offense = psi_factor(offense, faction_id_atk, true, false);
    }
    offense = offense * morale * 4;
    return offense;
}

/*
Get the basic defense value for a defending unit with an optional attacker unit parameter.
*/
int __cdecl mod_get_basic_defense(int veh_id_def, int veh_id_atk, int psi_combat_type, bool is_bombard) {
    int faction_id_def = Vehs[veh_id_def].faction_id;
    int unit_id_def = Vehs[veh_id_def].unit_id;
    int base_id_def = base_at(Vehs[veh_id_def].x, Vehs[veh_id_def].y);
    int morale;
    if (faction_id_def) {
        morale = mod_morale_veh(veh_id_def, true, 0);
    } else {
        morale = mod_morale_alien(veh_id_def, veh_id_atk >= 0 ? Vehs[veh_id_atk].faction_id : -1);
    }
    if (base_id_def >= 0) {
        // Morale effects with CC/BP are modified. SE Morale has no longer effect on native units.
        bool native_unit = unit_id_def < MaxProtoFactionNum
            && (Units[unit_id_def].offense_value() < 0 || unit_id_def == BSC_SPORE_LAUNCHER);
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
    if (veh_id_atk >= 0 && Vehs[veh_id_atk].faction_id && (unit_id_def >= MaxProtoFactionNum
    || (Units[unit_id_def].offense_value() >= 0 && unit_id_def != BSC_SPORE_LAUNCHER))
    && !has_abil(unit_id_def, ABL_DISSOCIATIVE_WAVE)
    && has_abil(Vehs[veh_id_atk].unit_id, ABL_SOPORIFIC_GAS)) {
        morale -= 2;
    }
    morale = clamp(morale, 1, 6);
    VehBasicBattleMorale[1] = morale;
    morale += 6;
    int plan_def = Units[unit_id_def].plan;
    if (plan_def == PLAN_ALIEN_ARTIFACT) {
        return 1;
    }
    // Fix: added veh_id_atk bounds check to prevent potential read outside array
    if (plan_def == PLAN_INFO_WARFARE && Units[unit_id_def].defense_value() == 1
    && (veh_id_atk < 0 || Units[Vehs[veh_id_atk].unit_id].plan != PLAN_INFO_WARFARE)) {
        return 1;
    }
    int defense = armor_proto(unit_id_def, veh_id_atk, is_bombard);
    if (psi_combat_type) {
        defense = psi_factor(defense, faction_id_def, false, unit_id_def == BSC_FUNGAL_TOWER);
    }
    defense *= morale;
    return defense;
}

static int current_atk_veh_id = -1;
static int current_def_veh_id = -1;

/*
Determine the best defender in a stack for a given attacker.
*/
int __cdecl mod_best_defender(int veh_id_def, int veh_id_atk, bool check_arty) {
    // TODO: potentially rewrite best_defender function here
    int best_def_veh_id = best_defender(veh_id_def, veh_id_atk, check_arty);

    if (check_arty) {
        current_atk_veh_id = -1;
        current_def_veh_id = -1;
    } else {
        current_atk_veh_id = veh_id_atk;
        current_def_veh_id = best_def_veh_id;
    }
    return best_def_veh_id;
}

int __cdecl battle_fight_parse_num(int index, int value) {
    if (index > 9) {
        return 3;
    }
    ParseNumTable[index] = value;

    if (conf.ignore_reactor_power && index == 1
    && current_atk_veh_id >= 0 && current_def_veh_id >= 0) {
        VEH* v1 = &Vehicles[current_atk_veh_id];
        VEH* v2 = &Vehicles[current_def_veh_id];
        UNIT* u1 = &Units[v1->unit_id];
        UNIT* u2 = &Units[v2->unit_id];
        // calculate attacker and defender power
        // artifact gets 1 HP regardless of reactor
        int attack_power = (u1->is_artifact() ? 1 :
                             u1->reactor_id * 10 - v1->damage_taken);
        int defend_power = (u2->is_artifact() ? 1 :
                             u2->reactor_id * 10 - v2->damage_taken);
        // calculate firepower
        int attack_fp = u2->reactor_id;
        int defend_fp = u1->reactor_id;
        // calculate hitpoints
        int attack_hp = (attack_power + (defend_fp - 1)) / defend_fp;
        int defend_hp = (defend_power + (attack_fp - 1)) / attack_fp;
        // calculate correct odds
        if (Weapon[u1->weapon_id].offense_value >= 0
        && Armor[u2->armor_id].defense_value >= 0) {
            // psi combat odds are already correct
            // reverse engineer conventional combat odds in case of ignored reactor
            int attack_odds = ParseNumTable[0] * attack_hp * defend_power;
            int defend_odds = ParseNumTable[1] * defend_hp * attack_power;
            int gcd = std::__gcd(attack_odds, defend_odds);
            attack_odds /= gcd;
            defend_odds /= gcd;
            // reparse their odds into dialog
            ParseNumTable[0] = attack_odds;
            ParseNumTable[1] = defend_odds;
        }
    }
    return 0;
}

/*
This function is only called from battle_fight_1 and not when combat results are simulated.
When chopper attack rate is modified, one full movement point is always reserved for the attack.
This avoids overflow issues with vehicle speed and incorrect hasty penalties for air units.
*/
int __cdecl mod_battle_fight_2(int veh_id, int offset, int tx, int ty, int is_table_offset, int a6, int a7)
{
    VEH* veh = &Vehs[veh_id];
    UNIT* u = &Units[veh->unit_id];
    if (conf.chopper_attack_rate > 1 && conf.chopper_attack_rate < 256) {
        if (u->triad() == TRIAD_AIR && u->range() == 1 && !u->is_missile()) {
            int moves = veh_speed(veh_id, 0) - veh->moves_spent - Rules->move_rate_roads;
            int addon = (conf.chopper_attack_rate - 1) * Rules->move_rate_roads;
            if (moves > 0 && addon > 0) {
                veh->moves_spent += min(moves, addon);
            }
        }
    }
    int value = battle_fight_2(veh_id, offset, tx, ty, is_table_offset, a6, a7);
    return value;
}

void __cdecl add_bat(int type, int modifier, char* display_str) {
    int offset = VehBattleModCount[type];
    if (modifier && offset >= 0 && offset < 4 && (type == 0 || type == 1)) {
        strncpy(VehBattleDisplay[type][offset], display_str, 80);
        VehBattleDisplay[type][offset][79] = '\0';
        VehBattleModifier[type][offset] = modifier;
        VehBattleModCount[type]++;
    }
}

void __cdecl mod_battle_compute(int veh_id_atk, int veh_id_def, int* offense_out, int* defense_out, int combat_type)
{
    battle_compute(veh_id_atk, veh_id_def, offense_out, defense_out, combat_type);

    VEH* v1 = &Vehs[veh_id_atk];
    VEH* v2 = &Vehs[veh_id_def];

    if (conf.planet_defense_bonus && v2->faction_id > 0
    && (v1->offense_value() < 0 || v2->defense_value() < 0)) {
        int planet_value = Factions[v2->faction_id].SE_planet;
        int combat_bonus = clamp(planet_value * Rules->combat_psi_bonus_per_planet / 2, -50, 50);
        if (combat_bonus != 0) {
            *defense_out = *defense_out * (100 + combat_bonus) / 100;
            add_bat(1, combat_bonus, (*TextLabels)[625]); // Planet
        }
    }
}



