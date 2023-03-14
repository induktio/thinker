
#include "veh.h"

static TileSearch ts;


bool __cdecl can_arty(int unit_id, bool allow_sea_arty) {
    UNIT& u = Units[unit_id];
    if ((Weapon[u.weapon_id].offense_value <= 0 // PSI + non-combat
    || Armor[u.armor_id].defense_value < 0) // PSI
    && unit_id != BSC_SPORE_LAUNCHER) { // Spore Launcher exception
        return false;
    }
    if (u.triad() == TRIAD_SEA) {
        return allow_sea_arty;
    }
    if (u.triad() == TRIAD_AIR) {
        return false;
    }
    return has_abil(unit_id, ABL_ARTILLERY); // TRIAD_LAND
}

bool __cdecl has_abil(int unit_id, VehAbilityFlag ability) {
    int faction_id = unit_id / MaxProtoFactionNum;
    // workaround fix for legacy base_build that may incorrectly use negative unit_id
    if (unit_id < 0) {
        return false;
    }
    if (Units[unit_id].ability_flags & ability) {
        return true;
    }
    if (!faction_id) {
        return false; // skip basic prototypes from #UNITS
    }
    if (is_alien(faction_id) && ability == ABL_DEEP_RADAR) {
        return true; // Caretakers + Usurpers > "Deep Radar" ability for all units
    }
    // Pirates > "Marine Detachment" ability for combat sea units with Adaptive Doctrine
    for (int i = 0; i < MFactions[faction_id].faction_bonus_count; i++) {
        if (MFactions[faction_id].faction_bonus_id[i] == FCB_FREEABIL_PREQ) {
            int abil_bonus_id = MFactions[faction_id].faction_bonus_val1[i];
            if (has_tech(Ability[abil_bonus_id].preq_tech, faction_id)
            && (ability & (1 << abil_bonus_id))) {
                return true;
            }
        }
    }
    // Nethack Terminus allows all probe teams to act as they have "Algorithmic Enhancement".
    // Manual implies fusion reactor or better is required for this but has_abil does not check it.
    if (ability == ABL_ALGO_ENHANCEMENT && Units[unit_id].is_probe()
    && has_project(FAC_NETHACK_TERMINUS, faction_id)) {
        return true;
    }
    return false;
}

bool has_ability(int faction, VehAbility abl, VehChassis chs, VehWeapon wpn) {
    int F = Ability[abl].flags;
    int triad = Chassis[chs].triad;

    if ((triad == TRIAD_LAND && ~F & AFLAG_ALLOWED_LAND_UNIT)
    || (triad == TRIAD_SEA && ~F & AFLAG_ALLOWED_SEA_UNIT)
    || (triad == TRIAD_AIR && ~F & AFLAG_ALLOWED_AIR_UNIT)) {
        return false;
    }
    if ((~F & AFLAG_ALLOWED_COMBAT_UNIT && wpn <= WPN_PSI_ATTACK)
    || (~F & AFLAG_ALLOWED_TERRAFORM_UNIT && wpn == WPN_TERRAFORMING_UNIT)
    || (~F & AFLAG_ALLOWED_NONCOMBAT_UNIT
    && wpn > WPN_PSI_ATTACK && wpn != WPN_TERRAFORMING_UNIT)) {
        return false;
    }
    if ((F & AFLAG_NOT_ALLOWED_PROBE_TEAM && wpn == WPN_PROBE_TEAM)
    || (F & AFLAG_ONLY_PROBE_TEAM && wpn != WPN_PROBE_TEAM)) {
        return false;
    }
    if (F & AFLAG_TRANSPORT_ONLY_UNIT && wpn != WPN_TROOP_TRANSPORT) {
        return false;
    }
    if (F & AFLAG_NOT_ALLOWED_FAST_UNIT && Chassis[chs].speed > 1) {
        return false;
    }
    if (F & AFLAG_NOT_ALLOWED_PSI_UNIT && wpn == WPN_PSI_ATTACK) {
        return false;
    }
    return has_tech(Ability[abl].preq_tech, faction);
}

/*
Renamed from veh_at. This version never calls rebuild_vehicle_bits on failed searches.
*/
int __cdecl veh_stack(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (!sq || !(sq->items & BIT_VEH_IN_TILE)) {
        return -1; // invalid or empty map tile
    }
    for (int veh_id = 0; veh_id < *total_num_vehicles; veh_id++) {
        if (Vehs[veh_id].x == x && Vehs[veh_id].y == y) {
            return veh_top(veh_id);
        }
    }
    return -1;
}

/*
Renamed from speed_proto. Calculate the speed of the specified prototype on roads.
*/
int __cdecl proto_speed(int unit_id) {
    if (unit_id == BSC_FUNGAL_TOWER) {
        return 0; // cannot move
    }
    UNIT* u = &Units[unit_id];
    int speed_val = u->speed();
    int triad = u->triad();
    int weapon_id = Units[unit_id].weapon_id;

    if (triad == TRIAD_AIR) {
        speed_val += Units[unit_id].reactor_id * 2;
    }
    if (Weapon[weapon_id].mode == WMODE_TRANSPORT) {
        speed_val--;
    }
    if (has_abil(unit_id, ABL_SLOW)) {
        speed_val--;
    }
    if (has_abil(unit_id, ABL_ANTIGRAV_STRUTS)) {
        speed_val += (triad == TRIAD_AIR) ? Units[unit_id].reactor_id * 2 : 1;
    }
    if (triad == TRIAD_AIR) {
        if (has_abil(unit_id, ABL_FUEL_NANOCELLS)) {
            speed_val += 2;
        }
        if (has_project(FAC_CLOUDBASE_ACADEMY, unit_id / MaxProtoFactionNum)) {
            speed_val += 2; // bonus from Aerospace Complex
        }
        if (has_abil(unit_id, ABL_AIR_SUPERIORITY)) {
            // generally -20% to -25%, in some cases higher due to lossy division rounding
            speed_val = (speed_val * 4) / 5;
        }
        if (Weapon[weapon_id].mode == WMODE_TRANSPORT) {
            speed_val /= 2; // 2nd penalty for air transports: -50%
        }
    }
    speed_val = clamp(speed_val, 1, 99) * Rules->move_rate_roads;
    assert(speed_val == speed_proto(unit_id));
    return speed_val;
}

/*
Renamed from speed. Calculate the speed of a unit on roads taking into consideration
various factors. The skip_morale parameter seems to only be set to true
for certain combat calculations in battle_fight().
*/
int __cdecl veh_speed(int veh_id, bool skip_morale) {
    int unit_id = Vehs[veh_id].unit_id;
    UNIT* u = &Units[unit_id];
    if (unit_id == BSC_FUNGAL_TOWER) {
        return 0; // cannot move
    }
    int speed_val = proto_speed(unit_id);
    int triad = u->triad();
    if (triad == TRIAD_SEA && has_project(FAC_MARITIME_CONTROL_CENTER, Vehs[veh_id].faction_id)) {
        speed_val += Rules->move_rate_roads * 2;
    }
    if (!skip_morale && morale_veh(veh_id, true, 0) == MORALE_ELITE
    && (unit_id >= MaxProtoFactionNum || u->offense_value() >= 0)) {
        speed_val += Rules->move_rate_roads; // Non-PSI elite units only
    }
    if (Vehs[veh_id].damage_taken && triad != TRIAD_AIR) {
        int moves = speed_val / Rules->move_rate_roads;
        int reactor_val;
        if (u->plan == PLAN_ALIEN_ARTIFACT) {
            reactor_val = 1;
            speed_val = 1;
        } else {
            reactor_val = clamp((int)u->reactor_id, 1, 100) * 10;
            speed_val = clamp(reactor_val, 1, 99);
        }
        speed_val = (moves * clamp(reactor_val - Vehs[veh_id].damage_taken, 0, 9999)
            + speed_val - 1) / speed_val;
        speed_val = clamp(speed_val, (triad == TRIAD_SEA) ? 2 : 1, 999) * Rules->move_rate_roads;
    }
    assert(speed_val == speed(veh_id, skip_morale));
    return speed_val;
}

int __cdecl veh_speed(int veh_id) {
    return veh_speed(veh_id, 0);
}

/*
Original version assigned cargo capacity on Spore Launchers but it seems this feature is not used.
*/
int __cdecl veh_cargo(int veh_id) {
    VEH* v = &Vehicles[veh_id];
    UNIT* u = &Units[v->unit_id];
    if (u->carry_capacity > 0 && veh_id < MaxProtoFactionNum
    && Weapon[u->weapon_id].offense_value < 0) {
        return v->morale + 1;
    }
    return u->carry_capacity;
}

/*
Get the basic offense value for an attacking unit with an optional defender unit parameter.
*/
int __cdecl mod_get_basic_offense(int veh_id_atk, int veh_id_def, int psi_combat_type, bool is_bombard, bool unk_tgl) {
    // unk_tgl is only set to true in battle_compute when veh_id_atk and veh_id_def are switched places.
    int faction_id_atk = Vehs[veh_id_atk].faction_id;
    int unit_id_atk = Vehs[veh_id_atk].unit_id;
    int morale = faction_id_atk ? morale_veh(veh_id_atk, true, 0)
        : morale_alien(veh_id_atk, veh_id_def >= 0 ? Vehs[veh_id_def].faction_id : -1);
    int base_id_atk = base_at(Vehs[veh_id_atk].x, Vehs[veh_id_atk].y);
    if (base_id_atk >= 0) {
        if (has_fac_built(FAC_CHILDREN_CRECHE, base_id_atk)) {
            morale++;
            int morale_active = clamp(Factions[faction_id_atk].SE_morale, -4, 4);
            if (morale_active <= -2) {
                morale_active++;
            }
            morale -= morale_active;
        } else if (has_fac_built(FAC_BROOD_PIT, base_id_atk) && unit_id_atk < MaxProtoFactionNum
        && (Units[unit_id_atk].offense_value() < 0 || unit_id_atk == BSC_SPORE_LAUNCHER)) {
            morale++;
            int morale_active = clamp(Factions[faction_id_atk].SE_morale, -4, 4);
            if (morale_active <= -2) {
                morale_active++;
            }
            morale -= morale_active;
        }
        if (unk_tgl) {
            int morale_pending = Factions[faction_id_atk].SE_morale_pending;
            if (morale_pending >= 2 && morale_pending <= 3) {
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
    assert(offense == get_basic_offense(veh_id_atk, veh_id_def, psi_combat_type, is_bombard, unk_tgl));
    return offense;
}

/*
Get the basic defense value for a defending unit with an optional attacker unit parameter.
*/
int __cdecl mod_get_basic_defense(int veh_id_def, int veh_id_atk, int psi_combat_type, bool is_bombard) {
    int faction_id_def = Vehs[veh_id_def].faction_id;
    int unit_id_def = Vehs[veh_id_def].unit_id;
    int base_id_def = base_at(Vehs[veh_id_def].x, Vehs[veh_id_def].y);
    int morale = faction_id_def ? morale_veh(veh_id_def, true, 0)
        : morale_alien(veh_id_def, veh_id_atk >= 0 ? Vehs[veh_id_atk].faction_id : -1);

    if (base_id_def >= 0) {
        if (has_fac_built(FAC_CHILDREN_CRECHE, base_id_def)) {
            morale++;
            int morale_active = clamp(Factions[faction_id_def].SE_morale, -4, 0);
            if (morale_active <= -2) {
                morale_active++;
            }
            morale -= morale_active;
        } else if (has_fac_built(FAC_BROOD_PIT, base_id_def)
        && unit_id_def < MaxProtoFactionNum
        && (Units[unit_id_def].offense_value() < 0 || unit_id_def == BSC_SPORE_LAUNCHER)) {
            morale++;
            int morale_active = clamp(Factions[faction_id_def].SE_morale, -4, 4);
            if (morale_active <= -2) {
                morale_active++;
            }
            morale -= morale_active;
        }
        // Fix: manual has "Units in a headquarters base automatically gain +1 Morale when defending."
        if (has_fac_built(FAC_HEADQUARTERS, base_id_def)) {
            morale++;
        }
        int morale_pending = Factions[faction_id_def].SE_morale_pending;
        if (morale_pending >= 2 && morale_pending <= 3) {
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
    assert(has_fac_built(FAC_HEADQUARTERS, base_id_def)
        || defense == get_basic_defense(veh_id_def, veh_id_atk, psi_combat_type, is_bombard));
    return defense;
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
Bonus functions are only used to patch more features on get_basic_offense or get_basic_defense.
*/
int __cdecl neural_amplifier_bonus(int value) {
    return value * conf.neural_amplifier_bonus / 100;
}

int __cdecl dream_twister_bonus(int value) {
    return value * conf.dream_twister_bonus / 100;
}

int __cdecl fungal_tower_bonus(int value) {
    return value * conf.fungal_tower_bonus / 100;
}

int __cdecl mod_veh_init(int unit_id, int faction, int x, int y) {
    int veh_id = veh_init(unit_id, faction, x, y);
    if (veh_id >= 0) {
        Vehicles[veh_id].home_base_id = -1;
        // Set these flags to disable any non-Thinker unit automation.
        Vehicles[veh_id].state |= VSTATE_UNK_40000;
        Vehicles[veh_id].state &= ~VSTATE_UNK_2000;
    }
    return veh_id;
}

int __cdecl mod_veh_kill(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    debug("disband %2d %2d %s\n", veh->x, veh->y, veh->name());
    return veh_kill(veh_id);
}

int __cdecl mod_veh_skip(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    int moves = veh_speed(veh_id);

    if (is_human(veh->faction_id)) {
        if (conf.activate_skipped_units) {
            if (!veh->road_moves_spent && veh->order < ORDER_FARM && !(veh->state & VSTATE_HAS_MOVED)) {
                veh->flags |= VFLAG_FULL_MOVE_SKIPPED;
            } else {
                veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
            }
        }
    } else if (veh->faction_id > 0) {
        veh->waypoint_1_x = veh->x;
        veh->waypoint_1_y = veh->y;
        veh->status_icon = '-';
        if (veh->need_monolith()) {
            MAP* sq = mapsq(veh->x, veh->y);
            if (sq && sq->items & BIT_MONOLITH) {
                monolith(veh_id);
            }
        }
    }
    veh->road_moves_spent = moves;
    return moves;
}

int __cdecl mod_veh_wake(int veh_id) {
    VEH* veh = &Vehicles[veh_id];
    if (veh->order >= ORDER_FARM && veh->order < ORDER_MOVE_TO && !(veh->state & VSTATE_CRAWLING)) {
        veh->road_moves_spent = veh_speed(veh_id) - Rules->move_rate_roads;
        if (veh->terraforming_turns) {
            int turns = veh->terraforming_turns - contribution(veh_id, veh->order - 4);
            veh->terraforming_turns = max(0, turns);
        }
    }
    if (veh->state & VSTATE_ON_ALERT && !(veh->state & VSTATE_HAS_MOVED) && veh->order_auto_type == ORDERA_ON_ALERT) {
        veh->road_moves_spent = 0;
    }
    /*
    Formers are a special case since they might not move but can build items immediately.
    When formers build something, VSTATE_HAS_MOVED should be set to prevent them from moving twice per turn.
    */
    if (conf.activate_skipped_units) {
        if (veh->flags & VFLAG_FULL_MOVE_SKIPPED && !(veh->state & VSTATE_HAS_MOVED)) {
            veh->road_moves_spent = 0;
        }
        veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
    }
    veh->order = ORDER_NONE;
    veh->state &= ~(VSTATE_UNK_8000000|VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
    return veh_id;
}

int __cdecl find_return_base(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq;
    debug("find_return_base %2d %2d %s\n", veh->x, veh->y, veh->name());
    ts.init(veh->x, veh->y, veh->triad() == TRIAD_SEA ? TS_SEA_AND_SHORE : veh->triad());
    while ((sq = ts.get_next()) != NULL) {
        if (sq->is_base() && sq->owner == veh->faction_id) {
            return base_at(ts.rx, ts.ry);
        }
    }
    int region = region_at(veh->x, veh->y);
    int min_dist = INT_MAX;
    int base_id = -1;
    bool sea = region > MaxRegionLandNum;
    for (int i = 0; i < *total_num_bases; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == veh->faction_id) {
            int base_region = region_at(base->x, base->y);
            int dist = map_range(base->x, base->y, veh->x, veh->y)
                * ((base_region > MaxRegionLandNum) == sea ? 1 : 4)
                * (base_region == region ? 1 : 8);
            if (dist < min_dist) {
                base_id = i;
                min_dist = dist;
            }
        }
    }
    return base_id;
}

int __cdecl probe_return_base(int UNUSED(x), int UNUSED(y), int veh_id) {
    return find_return_base(veh_id);
}

VehArmor best_armor(int faction, bool cheap) {
    int ci = ARM_NO_ARMOR;
    int cv = 4;
    for (int i=ARM_SYNTHMETAL_ARMOR; i<=ARM_RESONANCE_8_ARMOR; i++) {
        if (has_tech(Armor[i].preq_tech, faction)) {
            int val = Armor[i].defense_value;
            int cost = Armor[i].cost;
            if (cheap && (cost > 5 || cost > val))
                continue;
            int iv = val * (i >= ARM_PULSE_3_ARMOR ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return (VehArmor)ci;
}

VehWeapon best_weapon(int faction) {
    int ci = WPN_HAND_WEAPONS;
    int cv = 4;
    for (int i=WPN_LASER; i<=WPN_STRING_DISRUPTOR; i++) {
        if (has_tech(Weapon[i].preq_tech, faction)) {
            int iv = Weapon[i].offense_value *
                (i == WPN_RESONANCE_LASER || i == WPN_RESONANCE_BOLT ? 5 : 4);
            if (iv > cv) {
                cv = iv;
                ci = i;
            }
        }
    }
    return (VehWeapon)ci;
}

VehReactor best_reactor(int faction) {
    for (VehReactor r : {REC_SINGULARITY, REC_QUANTUM, REC_FUSION}) {
        if (has_tech(Reactor[r - 1].preq_tech, faction)) {
            return r;
        }
    }
    return REC_FISSION;
}

int offense_value(UNIT* u) {
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_id);
    if (u->weapon_id == WPN_CONVENTIONAL_PAYLOAD) {
        int wpn = best_weapon(*active_faction);
        return max(1, Weapon[wpn].offense_value * w);
    }
    return Weapon[u->weapon_id].offense_value * w;
}

int defense_value(UNIT* u) {
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_id);
    return Armor[u->armor_id].defense_value * w;
}

int set_move_to(int veh_id, int x, int y) {
    VEH* veh = &Vehicles[veh_id];
    debug("set_move_to %2d %2d -> %2d %2d %s\n", veh->x, veh->y, x, y, veh->name());
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->order = ORDER_MOVE_TO;
    veh->status_icon = 'G';
    veh->terraforming_turns = 0;
    mapnodes.erase({x, y, NODE_PATROL});
    mapnodes.erase({x, y, NODE_COMBAT_PATROL});
    if (veh->x == x && veh->y == y) {
        return mod_veh_skip(veh_id);
    }
    pm_target[x][y]++;
    return VEH_SYNC;
}

int set_move_next(int veh_id, int offset) {
    VEH* veh = &Vehicles[veh_id];
    int x = wrap(veh->x + BaseOffsetX[offset]);
    int y = veh->y + BaseOffsetY[offset];
    return set_move_to(veh_id, x, y);
}

int set_road_to(int veh_id, int x, int y) {
    VEH* veh = &Vehicles[veh_id];
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->order = ORDER_ROAD_TO;
    veh->status_icon = 'R';
    return VEH_SYNC;
}

int set_action(int veh_id, int act, char icon) {
    VEH* veh = &Vehicles[veh_id];
    if (act == ORDER_THERMAL_BOREHOLE) {
        mapnodes.insert({veh->x, veh->y, NODE_BOREHOLE});
    } else if (act == ORDER_TERRAFORM_UP) {
        mapnodes.insert({veh->x, veh->y, NODE_RAISE_LAND});
    } else if (act == ORDER_SENSOR_ARRAY) {
        mapnodes.insert({veh->x, veh->y, NODE_SENSOR_ARRAY});
    }
    veh->order = act;
    veh->status_icon = icon;
    veh->state &= ~VSTATE_UNK_10000;
    return VEH_SYNC;
}

int set_convoy(int veh_id, ResType res) {
    VEH* veh = &Vehicles[veh_id];
    mapnodes.insert({veh->x, veh->y, NODE_CONVOY});
    veh->order_auto_type = res-1;
    veh->order = ORDER_CONVOY;
    veh->status_icon = 'C';
    return veh_skip(veh_id);
}

int set_board_to(int veh_id, int trans_veh_id) {
    VEH* veh = &Vehicles[veh_id];
    VEH* v2 = &Vehicles[trans_veh_id];
    assert(veh_id != trans_veh_id);
    assert(veh->x == v2->x && veh->y == v2->y);
    assert(veh_cargo(trans_veh_id) > 0);
    veh->order = ORDER_SENTRY_BOARD;
    veh->waypoint_1_x = trans_veh_id;
    veh->waypoint_1_y = 0;
    veh->status_icon = 'L';
    debug("set_board_to %2d %2d %s -> %s\n", veh->x, veh->y, veh->name(), v2->name());
    return VEH_SYNC;
}

int cargo_loaded(int veh_id) {
    int n=0;
    for (int i=0; i < *total_num_vehicles; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->order == ORDER_SENTRY_BOARD && veh->waypoint_1_x == veh_id) {
            n++;
        }
    }
    return n;
}

