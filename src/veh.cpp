
#include "veh.h"


int __cdecl can_arty(int unit_id, bool allow_sea_arty) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    UNIT* u = &Units[unit_id];
    if (unit_id < 0 || unit_id >= MaxProtoNum) {
        return false;
    }
    if ((u->offense_value() <= 0 // PSI + non-combat
    || u->defense_value() < 0) // PSI
    && unit_id != BSC_SPORE_LAUNCHER) { // Spore Launcher exception
        return false;
    }
    if (u->triad() == TRIAD_SEA) {
        return allow_sea_arty;
    }
    if (u->triad() == TRIAD_AIR) {
        return false;
    }
    return has_abil(unit_id, ABL_ARTILLERY); // TRIAD_LAND
}

int __cdecl has_abil(int unit_id, VehAblFlag ability) {
    int faction_id = unit_id / MaxProtoFactionNum;
    // workaround fix for legacy base_build that may use incorrect unit_id
    assert(!thinker_enabled(faction_id) || (unit_id >= 0 && unit_id < MaxProtoNum));
    if (unit_id < 0 || unit_id >= MaxProtoNum) {
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
        if (MFactions[faction_id].faction_bonus_id[i] == RULE_FREEABIL) {
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

int __cdecl arty_range(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    UNIT* u = &Units[unit_id];
    if (can_arty(unit_id, true)) {
        if (*MultiplayerActive) {
            return Rules->artillery_max_rng;
        }
        return min(8, Rules->artillery_max_rng
            + (conf.long_range_artillery > 0 && u->triad() == TRIAD_SEA
            && u->offense_value() > 0 && u->ability_flags & ABL_ARTILLERY
            ? conf.long_range_artillery : 0));
    }
    return 0;
}

int __cdecl drop_range(int faction_id) {
    if (!has_orbital_drops(faction_id)) {
        return max(0, Rules->max_airdrop_rng_wo_orbital_insert);
    }
    return max(*MapAreaX, *MapAreaY);
}

bool has_orbital_drops(int faction_id) {
    return has_tech(Rules->tech_preq_orb_insert_wo_space, faction_id)
        || has_project(FAC_SPACE_ELEVATOR, faction_id);
}

bool has_ability(int faction_id, VehAbl abl, VehChassis chs, VehWeapon wpn) {
    int F = Ability[abl].flags;
    int triad = Chassis[chs].triad;
    bool is_combat = Weapon[wpn].offense_value != 0;
    bool is_former = Weapon[wpn].mode == WMODE_TERRAFORM;
    bool is_probe = Weapon[wpn].mode == WMODE_PROBE;

    if ((triad == TRIAD_LAND && !(F & AFLAG_ALLOWED_LAND_UNIT))
    || (triad == TRIAD_SEA && !(F & AFLAG_ALLOWED_SEA_UNIT))
    || (triad == TRIAD_AIR && !(F & AFLAG_ALLOWED_AIR_UNIT))) {
        return false;
    }
    if ((!(F & AFLAG_ALLOWED_COMBAT_UNIT) && is_combat)
    || (!(F & AFLAG_ALLOWED_TERRAFORM_UNIT) && is_former)
    || (!(F & AFLAG_ALLOWED_NONCOMBAT_UNIT) && !is_combat && !is_former)) {
        return false;
    }
    if ((F & AFLAG_NOT_ALLOWED_PROBE_TEAM && is_probe)
    || (F & AFLAG_ONLY_PROBE_TEAM && !is_probe)) {
        return false;
    }
    if (F & AFLAG_TRANSPORT_ONLY_UNIT && Weapon[wpn].mode != WMODE_TRANSPORT) {
        return false;
    }
    if (F & AFLAG_NOT_ALLOWED_FAST_UNIT && Chassis[chs].speed > 1) {
        return false;
    }
    if (F & AFLAG_NOT_ALLOWED_PSI_UNIT && Weapon[wpn].offense_value < 0) {
        return false;
    }
    return has_tech(Ability[abl].preq_tech, faction_id);
}

bool can_repair(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    return conf.repair_battle_ogre > 0 || (unit_id != BSC_BATTLE_OGRE_MK1
        && unit_id != BSC_BATTLE_OGRE_MK2 && unit_id != BSC_BATTLE_OGRE_MK3);
}

int __cdecl veh_who(int x, int y) {
    MAP* sq = mapsq(x, y);
    return (sq ? sq->veh_who() : -1);
}

int __cdecl veh_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq && !sq->veh_in_tile()) {
        return -1;
    }
    for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
        if (Vehs[veh_id].x == x && Vehs[veh_id].y == y) {
            return veh_top(veh_id);
        }
    }
    if (!sq) {
        debug("VehLocError x: %d y: %d\n", x, y);
        return -1;
    }
    if (!*VehBitError) {
        debug("VehBitError x: %d y: %d\n", x, y);
    }
    if (*GameState & STATE_SCENARIO_EDITOR || *GameState & STATE_DEBUG_MODE || *MultiplayerActive) {
        if (*VehBitError) {
            return -1;
        }
        *VehBitError = 1;
    }
    rebuild_vehicle_bits();
    return -1;
}

int __cdecl veh_top(int veh_id) {
    if (veh_id < 0) {
        return -1;
    }
    int top_veh_id = veh_id;
    while (Vehs[top_veh_id].prev_veh_id_stack >= 0) {
        top_veh_id = Vehs[top_veh_id].prev_veh_id_stack;
    }
    return top_veh_id;
}

int __cdecl stack_fix(int veh_id) {
    if (veh_id < 0 || !*MultiplayerActive
    || (Vehs[veh_id].next_veh_id_stack < 0 && Vehs[veh_id].prev_veh_id_stack < 0)) {
        return veh_id;
    }
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[veh_id].x == Vehs[i].x && Vehs[veh_id].y == Vehs[i].y) {
            veh_promote(i);
            stack_sort(veh_id);
        }
    }
    return veh_top(veh_id);
}

void __cdecl stack_sort(int veh_id) {
    int16_t x = Vehs[veh_id].x;
    int16_t y = Vehs[veh_id].y;
    int next_veh_id = veh_top(veh_id);
    int veh_id_put = -1;
    int veh_id_loop;
    if (veh_id >= 0 && next_veh_id >= 0) {
        do {
            veh_id_loop = Vehs[next_veh_id].next_veh_id_stack; // removed redundant < 0 check
            if (veh_cargo(next_veh_id) || has_abil(Vehs[next_veh_id].unit_id, ABL_CARRIER)) {
                veh_id_put = next_veh_id;
                veh_put(next_veh_id, -3, -3);
            }
            next_veh_id = veh_id_loop;
        } while (veh_id_loop >= 0);
        stack_put(veh_id_put, x, y);
    }
}

void __cdecl veh_promote(int veh_id) {
    int veh_id_top = veh_top(veh_id);
    if (veh_id_top >= 0 && veh_id_top != veh_id) {
        veh_put(veh_id, Vehs[veh_id_top].x, Vehs[veh_id_top].y);
    }
}

/*
Renamed from speed_proto. Calculate the road move speed for specified prototype.
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
various factors. Parameter skip_morale should be normally set to false and
seems to be only set to true for certain combat calculations in battle_fight.
Fix: due to limited size of moves_spent field, speeds over 255 are not allowed.
*/
int __cdecl veh_speed(int veh_id, bool skip_morale) {
    int unit_id = Vehs[veh_id].unit_id;
    UNIT* u = &Units[unit_id];
    bool normal_unit = unit_id >= MaxProtoFactionNum || u->offense_value() >= 0;
    if (unit_id == BSC_FUNGAL_TOWER) {
        return 0; // cannot move
    }
    // Maximum speed is rounded down to nearest full movement point
    int max_speed = 255 - (255 % Rules->move_rate_roads);
    int speed_val = proto_speed(unit_id);
    int triad = u->triad();
    if (triad == TRIAD_SEA && has_project(FAC_MARITIME_CONTROL_CENTER, Vehs[veh_id].faction_id)) {
        speed_val += Rules->move_rate_roads * 2;
    }
    if (!skip_morale && mod_morale_veh(veh_id, true, 0) == MORALE_ELITE
    && ((normal_unit && conf.normal_elite_moves > 0)
    || (!normal_unit && conf.native_elite_moves > 0))) {
        speed_val += Rules->move_rate_roads;
    }
    if (Vehs[veh_id].damage_taken && triad != TRIAD_AIR) {
        int moves = speed_val / Rules->move_rate_roads;
        int reactor_val;
        if (u->plan == PLAN_ARTIFACT) {
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
    return min(max_speed, speed_val);
}

/*
Calculate the cargo capacity of a unit. This version removes a redundant reference on
Spore Launchers which was never triggered since their carry_capacity was set to zero.
*/
int __cdecl veh_cargo(int veh_id) {
    VEH* v = &Vehs[veh_id];
    UNIT* u = &Units[v->unit_id];
    if (u->carry_capacity && v->unit_id < MaxProtoFactionNum && u->offense_value() < 0) {
        return v->morale + 1;
    }
    return u->carry_capacity;
}

/*
Determine whether the specified unit is eligible for monolith healing or morale upgrade.
*/
int __cdecl want_monolith(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    return veh_id >= 0 && veh->triad() != TRIAD_AIR
        && ((veh->damage_taken && can_repair(veh->unit_id))
        || (!(veh->state & VSTATE_MONOLITH_UPGRADED)
        && veh->offense_value() != 0
        && veh->morale < MORALE_ELITE
        && mod_morale_veh(veh_id, true, 0) < MORALE_ELITE));
}

/*
Calculate the former rate to perform terrain enhancements. This is only called from veh_wake.
*/
int __cdecl contribution(int veh_id, int terraform_id) {
    int value = has_abil(Vehs[veh_id].unit_id, ABL_SUPER_TERRAFORMER) ? 4 : 2;
    if (terraform_id == (ORDER_REMOVE_FUNGUS - 4) || terraform_id == (ORDER_PLANT_FUNGUS - 4)) {
        if (has_project(FAC_XENOEMPATHY_DOME, Vehs[veh_id].faction_id)) {
            value *= 2; // Doubled
        }
    } else if (has_project(FAC_WEATHER_PARADIGM, Vehs[veh_id].faction_id)) {
        value = (value * 3) / 2; // +50%
    }
    return value;
}

/*
Calculate the transport capacity for the specified chassis, abilities and reactor.
*/
int __cdecl transport_val(VehChassis chassis_id, VehAblFlag abls, VehReactor reactor_id) {
    int value = Chassis[chassis_id].cargo;
    if (Chassis[chassis_id].triad == TRIAD_SEA) {
        value *= reactor_id;
    }
    if (abls & ABL_SLOW) {
        value /= 2; // -50%, rounded down
    }
    if (abls & ABL_HEAVY_TRANSPORT) {
        value = (3 * value + 1) / 2; // +50%, rounded up
    }
    return value;
}

/*
Determine the extra percent cost for building a prototype. Includes a check if the faction
has the free prototype flag set or if the player is using one of the easier difficulties.
*/
int __cdecl prototype_factor(int unit_id) {
    int faction_id = unit_id / MaxProtoFactionNum;
    if (MFactions[faction_id].rule_flags & RFLAG_FREEPROTO
    || Factions[faction_id].diff_level <= DIFF_SPECIALIST) {
        return 0;
    }
    int triad = Units[unit_id].triad();
    switch (triad) {
    case TRIAD_SEA:
        return Rules->extra_cost_prototype_sea;
    case TRIAD_AIR:
        return Rules->extra_cost_prototype_air;
    case TRIAD_LAND:
    default:
        return Rules->extra_cost_prototype_land;
    }
}

/*
Calculates cost of the prototype based on various factors. Optimized logic flow from
the original without any differences to the final calculation.
*/
int __cdecl mod_proto_cost(VehChassis chassis_id, VehWeapon weapon_id,
VehArmor armor_id, VehAblFlag ability, VehReactor reactor_id) {
    uint8_t weap_cost = Weapon[weapon_id].cost;
    // PB check: moved to start vs after 1st triad checks in original > no difference in logic
    if (Chassis[chassis_id].missile && Weapon[weapon_id].offense_value >= 99) {
        return weap_cost;
    }
    uint8_t triad = Chassis[chassis_id].triad;
    uint32_t armor_cost = Armor[armor_id].cost;
    uint32_t speed_cost = Chassis[chassis_id].cost;
    int abil_modifier = 0;
    int flags_modifier = 0;
    if (ability) {
        for (int i = 0; i < MaxAbilityNum; i++) {
            if ((1 << i) & ability) {
                if (abil_modifier) {
                    abil_modifier++; // Increased cost with more than one ability
                }
                int factor = Ability[i].cost;
                if (factor > 0) { // 1+ = Straight Cost; 25% increase per unit of cost
                    abil_modifier += factor;
                } else {
                    switch (factor) {
                      case 0: // None
                      default:
                        break;
                        // Increases w/ ratio of weapon to armor: 0, 1, or 2. Rounded DOWN.
                        // Never higher than 2.
                      case -1:
                        // fixed potential crash: will never trigger in vanilla but could with mods
                        if (armor_cost) {
                            abil_modifier += clamp(weap_cost / armor_cost, 0u, 2u);
                        }
                        break;
                      case -2: // Increases w/ weapon value
                        abil_modifier += weap_cost - 1;
                        break;
                      case -3: // Increases w/ armor value
                        abil_modifier += armor_cost - 1;
                        break;
                      case -4: // Increases w/ speed value
                        abil_modifier += speed_cost - 1;
                        break;
                      case -5: // Increases w/ weapon+armor value
                        abil_modifier += weap_cost + armor_cost - 2;
                        break;
                      case -6: // Increases w/ weapon+speed value
                        abil_modifier += weap_cost + speed_cost - 2;
                        break;
                      case -7: // Increases w/ armor+speed value
                        abil_modifier += armor_cost + speed_cost - 2;
                        break;
                    }
                }
                // 010000000000 - Cost increased for land units; Deep Radar
                // Shifted flag check into main ability loop rather than its
                // own loop at 1st triad checks
                if (Ability[i].flags & AFLAG_COST_INC_LAND_UNIT && triad == TRIAD_LAND) {
                    // separate variable keeps logic same (two abilities, both with cost 0,
                    // one with cost increase flag will trigger above "if (abil_modifier)" if
                    // this is directly abil_modifier++)
                    flags_modifier++;
                }
            }
        }
        abil_modifier += flags_modifier; // adding here keeps logic consistent after optimization
    }
    if (triad == TRIAD_SEA) {
        armor_cost /= 2;
        speed_cost += reactor_id;
    } else if (triad == TRIAD_AIR) {
        if (armor_cost > 1) {
            armor_cost *= reactor_id * 2;
        }
        speed_cost += reactor_id * 2;
    } // TRIAD_LAND ability flag check moved into ability loop above
    uint32_t combat_mod = armor_cost / 2 + 1;
    if (combat_mod < weap_cost) { // which ever is greater
        combat_mod = weap_cost;
    }
    int proto_mod;
    // shifted this check to top vs at end > no difference in logic
    if (combat_mod == 1 && armor_cost == 1 && speed_cost == 1 && reactor_id == REC_FISSION) {
        proto_mod = 1;
    } else {
        // (2 << n) == 2^(n + 1) ; (2 << n) / 2 == 2 ^ n;
        // will crash if reactor_id is 0xFF > divide by zero; not putting in error checking
        // since this is unlikely even with modding however noting for future
        proto_mod = ((speed_cost + armor_cost) * combat_mod + ((2 << reactor_id) / 2))
            / (2 << reactor_id);
        if (speed_cost == 1) {
            proto_mod = (proto_mod / 2) + 1;
        }
        if (weap_cost > 1 && Armor[armor_id].cost > 1) {
            proto_mod++;
            // moved inside nested check vs separate triad checks below > no difference in logic
            if (triad == TRIAD_LAND && speed_cost > 1) {
                proto_mod++;
            }
        }
        // excludes sea probes
        if (triad == TRIAD_SEA && Weapon[weapon_id].mode != WMODE_PROBE) {
            proto_mod = (proto_mod + 1) / 2;
        } else if (triad == TRIAD_AIR) {
            proto_mod /= (Weapon[weapon_id].mode > WMODE_MISSILE) ? 2 : 4; // Non-combat : Combat
        }
        int reactor_mod = (reactor_id * 3 + 1) / 2;
        if (proto_mod < reactor_mod) { // which ever is greater
            proto_mod = reactor_mod;
        }
    }
    int cost = (proto_mod * (abil_modifier + 4) + 2) / 4;
//    debug("proto_cost %d %d %d %d %d cost: %d\n",
//        chassis_id, weapon_id, armor_id, ability, reactor_id, cost);
    assert(cost == proto_cost(chassis_id, weapon_id, armor_id, ability, reactor_id));
    return cost;
}

/*
Calculates the base cost of the specified unit (without prototype modifiers).
*/
int __cdecl mod_base_cost(int unit_id) {
    return mod_proto_cost(
        (VehChassis)Units[unit_id].chassis_id,
        (VehWeapon)Units[unit_id].weapon_id,
        (VehArmor)Units[unit_id].armor_id,
        ABL_NONE,
        (VehReactor)Units[unit_id].reactor_id);
}

/*
Calculate the specified prototype's mineral row cost to build. Optional output parameter
whether there is an associated 1st time prototype cost (true) or just the base (false).
Note that has_proto_cost is treated as int32_t variable in the game engine.
*/
int __cdecl mod_veh_cost(int unit_id, int base_id, int32_t* has_proto_cost) {
    int cost = Units[unit_id].cost;
    if (base_id >= 0 && unit_id < MaxProtoFactionNum // Fix: added base_id bounds check
    && (Units[unit_id].offense_value() < 0 || unit_id == BSC_SPORE_LAUNCHER)
    && has_fac_built(FAC_BROOD_PIT, base_id)) {
        cost = (cost * 3) / 4; // Decrease the cost of alien units by 25%
    }
    if (Units[unit_id].plan == PLAN_COLONY && base_id >= 0) {
        cost = clamp(cost, 1, 999);
    }
    int proto_cost_first = 0;
    if (unit_id >= MaxProtoFactionNum && !Units[unit_id].is_prototyped()) {
        proto_cost_first = (base_id >= 0 && has_fac_built(FAC_SKUNKWORKS, base_id))
            ? 0 : (prototype_factor(unit_id) * mod_base_cost(unit_id) + 50) / 100; // moved checks up
        cost += proto_cost_first;
    }
    if (has_proto_cost) {
        *has_proto_cost = proto_cost_first != 0;
    }
    assert(cost == veh_cost(unit_id, base_id, 0));
    return cost;
}

/*
Calculate upgrade cost between two prototypes in mineral rows.
*/
int __cdecl mod_upgrade_cost(int faction_id, int new_unit_id, int old_unit_id) {
    UNIT* old_unit = &Units[old_unit_id];
    UNIT* new_unit = &Units[new_unit_id];
    int cost;

    if (conf.modify_upgrade_cost) {
        if (new_unit->is_supply()) {
            cost = max((int)new_unit->cost, 4*max(1, new_unit->cost - old_unit->cost));
        } else {
            cost = max((int)new_unit->cost, 2*max(1, new_unit->cost - old_unit->cost));
        }
        if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
            cost *= 2;
        }
        if (has_project(FAC_NANO_FACTORY, faction_id)) {
            cost /= 2;
        }
    } else {
        int modifier = new_unit->cost;
        if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
            modifier = ((new_unit->cost + 1) / 2 + new_unit->cost)
                * ((new_unit->cost + 1) / 2 + new_unit->cost + 1);
        }
        cost = max(0, new_unit->speed() - old_unit->speed())
            + max(0, new_unit->armor_cost() - old_unit->armor_cost())
            + max(0, new_unit->weapon_cost() - old_unit->weapon_cost())
            + modifier;
        if (has_project(FAC_NANO_FACTORY, faction_id)) {
            cost /= 2;
        }
        assert(cost == upgrade_cost(faction_id, new_unit_id, old_unit_id));
    }
    return cost;
}

int __cdecl mod_upgrade_prototype(int faction_id, int new_unit_id, int old_unit_id, int flag) {
    Faction* f = &Factions[faction_id];
    UNIT* new_unit = &Units[new_unit_id];
    UNIT* old_unit = &Units[old_unit_id];
    assert(new_unit_id != old_unit_id);
    assert(new_unit_id / MaxProtoFactionNum == faction_id);

    if (*GameState & STATE_GAME_DONE && !(*GameState & STATE_FINAL_SCORE_DONE)) {
        return 0;
    }
    // Fix issue with upgrade cost being incorrect for unit counts of more than 255.
    int unit_cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
    int unit_count = veh_count(faction_id, old_unit_id);
    int total_cost = unit_cost * unit_count;
    bool prototyped = new_unit_id < MaxProtoFactionNum || new_unit->is_prototyped();

    if (!total_cost && (flag || !is_human(faction_id))) {
        return 0;
    }
    if (!is_human(faction_id)) {
        // Simplify cost threshold for automatic upgrades
        if (total_cost > f->energy_credits/4
        || unit_cost >= 50 + min(50, f->energy_credits/32)) {
            return 0;
        }
        // Reduce non-important upgrades
        if (conf.design_units && (old_unit->plan > PLAN_RECON
        || (conf.ignore_reactor_power && new_unit->offense_value() <= old_unit->offense_value()
        && new_unit->defense_value() <= old_unit->defense_value()))) {
            return 0;
        }
        goto DO_UPGRADE;
    }
    if (faction_id != MapWin->cOwner) {
        return 0;
    }
    if (!total_cost) {
        goto DO_UPGRADE;
    }
    StrBuffer[0] = '\0';
    strncat(StrBuffer, old_unit->name, StrBufLen);
    strncat(StrBuffer, " (", StrBufLen);
    say_stats_2(StrBuffer, old_unit_id);
    if (old_unit->ability_flags) {
        strncat(StrBuffer, " ", StrBufLen);
        for (int i = 0; i < MaxAbilityNum; i++) {
            if (has_abil(old_unit_id, (VehAblFlag)(1 << i))) {
                strncat(StrBuffer, Ability[i].abbreviation, StrBufLen);
            }
        }
    }
    strncat(StrBuffer, ")", StrBufLen);
    parse_says(0, StrBuffer, -1, -1);
    StrBuffer[0] = '\0';
    strncat(StrBuffer, new_unit->name, StrBufLen);
    strncat(StrBuffer, " (", StrBufLen);
    say_stats_2(StrBuffer, new_unit_id);
    if (new_unit->ability_flags) {
        strncat(StrBuffer, " ", StrBufLen);
        for (int i = 0; i < MaxAbilityNum; i++) {
            if (has_abil(new_unit_id, (VehAblFlag)(1 << i))) {
                strncat(StrBuffer, Ability[i].abbreviation, StrBufLen);
            }
        }
    }
    strncat(StrBuffer, ")", StrBufLen);
    parse_says(1, StrBuffer, -1, -1);
    parse_num(0, total_cost);
    parse_num(1, f->energy_credits);
    parse_num(2, unit_count);

    const char* image_file;
    switch (new_unit->triad()) {
        case TRIAD_SEA: image_file = "navun_sm.pcx"; break;
        case TRIAD_AIR: image_file = (is_alien(faction_id) ? "alair_sm.pcx" : "air_sm.pcx"); break;
        default: image_file = (is_alien(faction_id) ? "alunit_sm.pcx" : "unit_sm.pcx"); break;
    }
    if (total_cost > f->energy_credits) {
        if (flag && !(*GameWarnings & WARN_STOP_PROTOTYPE_COMPLETE)) {
            return 0;
        }
        const char* text_label = (prototyped ? "UPGRADEPROTOCOST" : "UPGRADEPROTOCOST2");
        popp(ScriptFile, text_label, 0, image_file, 0);
        return 0;
    } else {
        const char* text_label = (prototyped ? "UPGRADEPROTO" : "UPGRADEPROTO2");
        if (!*dword_90EA3C) {
            int value = popp(ScriptFile, text_label, 0, image_file, 0);
            if (!value) {
                return 0;
            }
            if (value > 1) { // This dialog option is not visible by default
                *dword_90EA3C = 1;
            }
        }
    }
DO_UPGRADE:
    if (is_human(faction_id) && *MultiplayerActive) {
        message_data(0x2411, 0, faction_id, new_unit_id, old_unit_id, 0);
        if (NetDaemon_receive(NetState)) {
            while (NetDaemon_receive(NetState));
            return 1;
        }
    } else {
        full_upgrade(faction_id, new_unit_id, old_unit_id);
    }
    return 1;
}

static void veh_upgrade(VEH* veh, int new_unit_id, int old_unit_id, int cost) {
    Faction* f = &Factions[veh->faction_id];
    f->energy_credits -= cost;
    f->units_active[new_unit_id]++;
    f->units_active[old_unit_id]--;
    veh->unit_id = new_unit_id;
    int morale_diff = (has_abil(new_unit_id, ABL_TRAINED) != 0)
        - (has_abil(old_unit_id, ABL_TRAINED) != 0);
    veh->morale = clamp(veh->morale + morale_diff, 0, 6);
    assert(f->energy_credits >= 0);
    assert(new_unit_id / MaxProtoFactionNum == veh->faction_id);
    debug("upgrade_unit %d %d %2d %2d cost: %d %s / %s\n",
    *CurrentTurn, veh->faction_id, veh->x, veh->y, cost, Units[old_unit_id].name, veh->name());
}

/*
Partial upgrades until energy reserves run out, MP mode not implemented.
*/
void __cdecl part_upgrade(int faction_id, int new_unit_id, int old_unit_id) {
    Faction* f = &Factions[faction_id];
    if (!*MultiplayerActive && faction_id > 0
    && old_unit_id >= 0 && new_unit_id != old_unit_id
    && new_unit_id / MaxProtoFactionNum == faction_id) {
        int cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
        bool active = false;
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->faction_id == faction_id && veh->unit_id == old_unit_id
            && !veh->moves_spent && !veh->damage_taken && base_at(veh->x, veh->y) >= 0) {
                if (f->energy_credits < cost + 20) {
                    break;
                }
                veh_upgrade(veh, new_unit_id, old_unit_id, cost);
                active = true;
            }
        }
        if (active) {
            draw_map(1);
        }
    }
}

/*
Full upgrade to new prototype, retire old prototype and update base queues.
Total cost must be checked by the caller function.
*/
void __cdecl full_upgrade(int faction_id, int new_unit_id, int old_unit_id) {
    int cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
    bool active = false;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id && veh->unit_id == old_unit_id) {
            veh_upgrade(veh, new_unit_id, old_unit_id, cost);
            active = true;
        }
    }
    for (int i = *BaseCount-1; i >= 0; i--) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id) {
            for (int j = 0; j < 10 && j <= base->queue_size; j++) {
                if (base->queue_items[j] == old_unit_id) {
                    base->queue_items[j] = new_unit_id;
                }
            }
        }
    }
    if (new_unit_id >= MaxProtoFactionNum && active) {
        Units[new_unit_id].unit_flags |= UNIT_PROTOTYPED;
    }
    if (old_unit_id >= MaxProtoFactionNum) {
        Units[old_unit_id].unit_flags &= ~UNIT_ACTIVE;
    }
    if (is_human(faction_id)) {
        draw_map(1);
    }
    if (faction_id == MapWin->cOwner) {
        Console_update_data(MapWin, 0);
    }
}

/*
Check to see whether provided faction and base can build a specific prototype.
Includes checks to prevent SMACX specific units from being built in SMAC mode.
*/
int __cdecl mod_veh_avail(int unit_id, int faction_id, int base_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);

    if (!Units[unit_id].is_active()
    || (Units[unit_id].obsolete_factions & (1 << faction_id))) {
        return false;
    }
    if (unit_id < MaxProtoFactionNum) {
        if (!has_tech(Units[unit_id].preq_tech, faction_id)) {
            return false;
        }
    }
    if (Units[unit_id].plan == PLAN_COLONY
    && *GameRules & RULES_SCN_NO_COLONY_PODS) {
        return false;
    }
    if (base_id >= 0 && Units[unit_id].triad() == TRIAD_SEA
    && !is_coast(Bases[base_id].x, Bases[base_id].y, 0)) {
        return false;
    }
    int wpn_id = Units[unit_id].weapon_id;
    uint32_t abls = Units[unit_id].ability_flags;
    if (!*ExpansionEnabled && (Units[unit_id].armor_id > ARM_PSI_DEFENSE
    || wpn_id == WPN_RESONANCE_LASER || wpn_id == WPN_RESONANCE_BOLT
    || wpn_id == WPN_STRING_DISRUPTOR || wpn_id == WPN_TECTONIC_PAYLOAD
    || wpn_id == WPN_FUNGAL_PAYLOAD
    || abls & ABL_SOPORIFIC_GAS || abls & ABL_DISSOCIATIVE_WAVE
    || abls & ABL_MARINE_DETACHMENT || abls & ABL_FUEL_NANOCELLS
    || abls & ABL_ALGO_ENHANCEMENT
    || unit_id == BSC_SEALURK || unit_id == BSC_SPORE_LAUNCHER
    || unit_id == BSC_BATTLE_OGRE_MK1 || unit_id == BSC_BATTLE_OGRE_MK2
    || unit_id == BSC_BATTLE_OGRE_MK3 || unit_id == BSC_FUNGAL_TOWER
    || unit_id == BSC_UNITY_MINING_LASER)) {
        return false;
    }
    if (unit_id < MaxProtoFactionNum) {
        return true;
    }
    return (unit_id / MaxProtoFactionNum) == faction_id;
}

/*
Various unit stack related calculations based on type parameter (0-19) and conditions.
*/
int __cdecl mod_stack_check(int veh_id, int type, int cond1, int cond2, int cond3) {
    int value = 0;
    for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
        switch (type) {
        case 0:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2) && Vehs[i].unit_id == cond1) {
                value++;
            }
            break;
        case 1:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                value++;
            }
            break;
        case 2:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2)
            && Vehs[i].plan() == cond1) {
                value++;
            }
            break;
        case 3:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2) && Vehs[i].triad() == cond1) {
                value++;
            }
            break;
        case 4:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                value += Vehs[i].offense_value();
            }
            break;
        case 5:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                value += Vehs[i].defense_value();
            }
            break;
        case 6:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2)
            && has_abil(Vehs[i].unit_id, (VehAblFlag)cond1)) {
                value++;
            }
            break;
        case 7:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                value += Units[Vehs[i].unit_id].cost;
            }
            break;
        case 8:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                int triad = Vehs[i].triad();
                if (triad == TRIAD_LAND) {
                    value--;
                } else if (triad == TRIAD_SEA) {
                    value += veh_cargo(i);
                }
            }
            break;
        case 9:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2) && Vehs[i].order == cond1) {
                value++;
            }
            break;
        case 10:
            if (Vehs[i].faction_id == cond1) {
                value++;
            }
            break;
        case 11:
            if ((cond3 < 0 || Vehs[i].faction_id == cond3)
            && (Vehs[i].state & cond1) == (uint32_t)cond2) {
                value++;
            }
            break;
        case 12:
            if (cond1 < 0 || Vehs[i].faction_id == cond1) {
                if (arm_strat(Units[Vehs[i].unit_id].armor_id, Vehs[i].faction_id)
                > weap_strat(Units[Vehs[i].unit_id].weapon_id, Vehs[i].faction_id)) {
                    value++;
                }
            }
            break;
        case 13:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1) && Vehs[i].is_missile()) {
                value++;
            }
            break;
        case 14:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1)
            && (Vehs[i].plan() == PLAN_COMBAT || Vehs[i].plan() == PLAN_DEFENSE)) {
                value++;
            }
            break;
        case 15:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1) && can_arty(Vehs[i].unit_id, true)) {
                value++;
            }
            break;
        case 16:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1)
            && (Vehs[i].plan() == PLAN_COMBAT || Vehs[i].plan() == PLAN_DEFENSE
            || Vehs[i].plan() == PLAN_RECON)) {
                value++;
            }
            break;
        case 17:
            if ((cond2 < 0 || Vehs[i].faction_id == cond2)
            && Units[Vehs[i].unit_id].group_id == cond1) {
                value++;
            }
            break;
        case 18:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1)
            && (Vehs[i].triad() == TRIAD_AIR && Vehs[i].range() > 1)) {
                value++;
            }
            break;
        case 19:
            if ((cond1 < 0 || Vehs[i].faction_id == cond1) && !Vehs[i].offense_value()) {
                value++;
            }
            break;
        default:
            break;
        }
    }
    assert(value == stack_check(veh_id, type, cond1, cond2, cond3));
    return value;
}

int __cdecl mod_veh_init(int unit_id, int faction_id, int x, int y) {
    int veh_id = veh_init(unit_id, faction_id, x, y);
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
    veh_kill(veh_id);
    return VEH_SKIP;
}

/*
Check if the land unit inside an air transport can disembark.
The transport must be in either a base or an airbase to do so.
*/
int __cdecl mod_veh_jail(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (veh->triad() == TRIAD_LAND && veh->order == ORDER_SENTRY_BOARD
    && veh->waypoint_1_x >= 0 && Vehs[veh->waypoint_1_x].triad() == TRIAD_AIR
    && sq && !sq->is_airbase()) { // Fix: also allow airbases with owner = 0
        return true;
    }
    return false;
}

/*
Skip vehicle turn by adjusting spent moves to maximum available moves.
Formers are a special case since they can build items without moving on the same turn.
*/
int __cdecl mod_veh_skip(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    int moves = veh_speed(veh_id, 0);

    if (is_human(veh->faction_id)) {
        if (veh->is_former() && veh->order >= ORDER_FARM && veh->order < ORDER_MOVE_TO) {
            veh->state |= VSTATE_WORKING;
        }
        if (conf.activate_skipped_units) {
            if (!veh->moves_spent && veh->order < ORDER_FARM
            && !(veh->state & (VSTATE_HAS_MOVED|VSTATE_MADE_AIRDROP))) {
                veh->flags |= VFLAG_FULL_MOVE_SKIPPED;
            } else if (veh->triad() != TRIAD_AIR) {
                // veh_skip may get called twice on aircraft units
                veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
            }
        }
    } else if (veh->faction_id > 0) {
        veh->waypoint_1_x = veh->x;
        veh->waypoint_1_y = veh->y;
        veh->status_icon = '-';
        if (want_monolith(veh_id)) {
            if (bit_at(veh->x, veh->y) & BIT_MONOLITH) {
                monolith(veh_id);
            }
        }
    }
    veh->moves_spent = moves;
    return moves;
}

int __cdecl mod_veh_wake(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    if (veh->order >= ORDER_FARM && veh->order < ORDER_MOVE_TO && !(veh->state & VSTATE_WORKING)) {
        veh->moves_spent = veh_speed(veh_id, 0) - Rules->move_rate_roads;
        if (veh->movement_turns) {
            veh->movement_turns = max(0, veh->movement_turns - contribution(veh_id, veh->order - 4));
        }
    }
    if (veh->state & VSTATE_ON_ALERT && !(veh->state & VSTATE_HAS_MOVED)
    && veh->order_auto_type == ORDERA_ON_ALERT) {
        veh->moves_spent = 0;
    }
    if (conf.activate_skipped_units && is_human(veh->faction_id)) {
        if (veh->flags & VFLAG_FULL_MOVE_SKIPPED
        && !(veh->state & (VSTATE_HAS_MOVED|VSTATE_MADE_AIRDROP))) {
            veh->moves_spent = 0;
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
    TileSearch ts;
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
    for (int i = 0; i < *BaseCount; i++) {
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

int __cdecl create_proto(int faction_id, VehChassis chs, VehWeapon wpn, VehArmor arm,
VehAblFlag abls, VehReactor rec, VehPlan ai_plan) {
    char name[256];
    mod_name_proto(name, -1, faction_id, chs, wpn, arm, abls, rec);
    int unit_id = propose_proto(faction_id, chs, wpn, arm, abls, rec, ai_plan, (strlen(name) ? name : NULL));
    debug("create_proto %4d %d chs: %d rec: %d wpn: %2d arm: %2d %08X %d %s\n",
        *CurrentTurn, faction_id, chs, rec, wpn, arm, abls, unit_id, name);
    return unit_id;
}

/*
This function is only called from propose_proto to skip prototypes that are invalid.
*/
int __cdecl mod_is_bunged(int faction_id, VehChassis chs, VehWeapon wpn, VehArmor arm,
VehAblFlag abls, VehReactor rec) {
    int triad = Chassis[chs].triad;
    int arm_v = Armor[arm].defense_value;
    int wpn_v = Weapon[wpn].offense_value;
    debug("propose_proto %3d %d chs: %d rec: %d wpn: %2d arm: %2d %08X\n",
        *CurrentTurn, faction_id, chs, rec, wpn, arm, abls);

    if (!is_human(faction_id)) {
        if (conf.design_units) {
            if (triad == TRIAD_SEA && wpn_v > 0 && arm_v > 3
            && arm_v > wpn_v + 1) {
                return 1;
            }
        }
        return 0;
    }
    return is_bunged(faction_id, chs, wpn, arm, abls, rec);
}

/*
Create a new prototype. Sets initial values for everything except name and preq_tech.
*/
void __cdecl mod_make_proto(int unit_id, VehChassis chassis_id,
VehWeapon weapon_id, VehArmor armor_id, VehAblFlag abls, VehReactor reactor_id) {
    UNIT* unit = &Units[unit_id];
    int faction_id = unit_id / MaxProtoFactionNum;
    int option = 0;
    debug("make_proto %d %d chs: %d rec: %d wpn: %2d arm: %2d %08X\n",
        *CurrentTurn, unit_id, chassis_id, reactor_id, weapon_id, armor_id, abls);
    if (unit_id >= MaxProtoFactionNum) {
        bool cond1 = false;
        bool cond2 = false;
        bool cond3 = false;
        int unit_id_loop;
        for (int i = 0; i < 2*MaxProtoFactionNum; i++) {
            unit_id_loop = i % MaxProtoFactionNum;
            if (i >= MaxProtoFactionNum) {
                unit_id_loop += unit_id - unit_id % MaxProtoFactionNum;
            }
            int flags_check = Units[unit_id_loop].unit_flags;
            if (flags_check & UNIT_ACTIVE) {
                if ((unit_id_loop < MaxProtoFactionNum
                && has_tech(Units[unit_id_loop].preq_tech, faction_id))
                || (unit_id_loop >= MaxProtoFactionNum
                && (flags_check & UNIT_PROTOTYPED))) {
                    int loop_weapon_id = Units[unit_id_loop].weapon_id;
                    if (loop_weapon_id == weapon_id) {
                        cond1 = true;
                    }
                    int loop_armor_id = Units[unit_id_loop].armor_id;
                    if (loop_armor_id == armor_id) {
                        cond2 = true;
                    }
                    int loop_chassis_id = Units[unit_id_loop].chassis_id;
                    if (loop_chassis_id == chassis_id) {
                        cond3 = true;
                    }
                    int off_rating = Weapon[weapon_id].offense_value;
                    if (off_rating > 0 && Weapon[loop_weapon_id].offense_value >= off_rating) {
                        cond1 = true;
                    }
                    int def_rating = Armor[armor_id].defense_value;
                    if (def_rating > 0 && Armor[loop_armor_id].defense_value >= def_rating) {
                        cond2 = true;
                    }
                    if (Chassis[chassis_id].triad == Chassis[loop_chassis_id].triad
                    && Chassis[loop_chassis_id].speed >= Chassis[chassis_id].speed) {
                        cond3 = true;
                    }
                }
            }
        }
        if (cond1 && cond2 && cond3) {
            option = (unit_id_loop >= MaxProtoFactionNum
                && Units[unit_id_loop].unit_flags & UNIT_UNK_10) ? 3 : 1;
        }
    }
    unit->chassis_id = (uint8_t)chassis_id;
    unit->weapon_id = (uint8_t)weapon_id;
    unit->armor_id = (uint8_t)armor_id;
    unit->ability_flags = abls;
    unit->reactor_id = (uint8_t)reactor_id;
    unit->cost = (uint8_t)mod_proto_cost(chassis_id, weapon_id, armor_id, abls, reactor_id);
    unit->carry_capacity = (Weapon[weapon_id].mode == WMODE_TRANSPORT)
        ? (uint8_t)transport_val(chassis_id, abls, reactor_id) : 0;

    if (Chassis[chassis_id].missile) {
        if (Weapon[weapon_id].offense_value < 99) { // non-PB missiles
            if (weapon_id == WPN_TECTONIC_PAYLOAD) {
                unit->plan = PLAN_TECTONIC_MISSILE;
                unit->group_id = 22;
            } else if (weapon_id == WPN_FUNGAL_PAYLOAD) {
                unit->plan = PLAN_FUNGAL_MISSILE;
                unit->group_id = 23;
            } else { // Conventional Payload
                unit->plan = PLAN_OFFENSE;
                unit->group_id = 13;
            }
        } else {
            unit->plan = PLAN_PLANET_BUSTER;
            unit->group_id = 14;
        }
    } else if (Weapon[weapon_id].mode >= WMODE_TRANSPORT) { // Non-combat
        unit->plan = Weapon[weapon_id].mode;
        unit->group_id = Weapon[weapon_id].mode + 32;
    } else if (Chassis[chassis_id].triad == TRIAD_SEA) { // combat sea
        unit->plan = PLAN_NAVAL_SUPERIORITY;
        unit->group_id = 6; // same value as plan
    } else if (Chassis[chassis_id].triad == TRIAD_AIR) { // combat air
        if (has_abil(unit_id, ABL_AIR_SUPERIORITY)) {
            unit->plan = PLAN_AIR_SUPERIORITY;
            unit->group_id = 9;
        } else {
            unit->plan = PLAN_OFFENSE;
            unit->group_id = 8;
        }
    } else { // TRIAD_LAND combat unit
        unit->plan = PLAN_OFFENSE;
        if (Armor[armor_id].defense_value > 1) {
            unit->plan = PLAN_DEFENSE;
        }
        if (Weapon[weapon_id].offense_value >= Armor[armor_id].defense_value
        && unit->plan == PLAN_DEFENSE) {
            unit->plan = PLAN_COMBAT;
        }
        if (Chassis[chassis_id].speed > 1
        && Weapon[weapon_id].offense_value > Armor[armor_id].defense_value) {
            unit->plan = PLAN_OFFENSE;
        }
        if (abls & (ABL_ARTILLERY | ABL_DROP_POD | ABL_AMPHIBIOUS)) {
            unit->plan = PLAN_OFFENSE;
        }
        unit->group_id = 3;
        if (Armor[armor_id].defense_value > Weapon[weapon_id].offense_value) {
            unit->group_id = 2;
        }
        if (Weapon[weapon_id].offense_value > Armor[armor_id].defense_value * 2) {
            unit->group_id = 1;
        }
        if (Chassis[chassis_id].speed == 2) {
            unit->group_id = 4;
        } else if (Chassis[chassis_id].speed >= 3) {
            unit->group_id = 5;
        }
        if (Weapon[weapon_id].offense_value == 1 && Armor[armor_id].defense_value == 1) {
            if (Chassis[chassis_id].speed > 1) {
                unit->plan = PLAN_RECON;
            }
            if (has_abil(unit_id, ABL_POLICE_2X)) {
                unit->plan = PLAN_DEFENSE;
            }
            unit->group_id = 10;
        }
    }
    unit->icon_offset = -1;
    unit->obsolete_factions = 0;
    unit->combat_factions = (unit_id >= MaxProtoFactionNum) ? (1 << faction_id) : -1;
    unit->unit_flags = UNIT_ACTIVE;
    if (option) {
        if (option & 2) {
            unit->unit_flags = (UNIT_UNK_100|UNIT_UNK_10|UNIT_PROTOTYPED|UNIT_ACTIVE);
        } else {
            unit->unit_flags = (UNIT_UNK_100|UNIT_PROTOTYPED|UNIT_ACTIVE);
        }
    }
}

void parse_abl_name(char* buf, const char* name, bool reduce) {
    const int len = min(strlen(name), (size_t)MaxProtoNameLen);
    // Skip empty abbreviation names (Deep Radar)
    if (!len) {
        return;
    }
    for (int i = 0; i < len; i++) {
        if (i >= 3 && reduce && (len >= 8 || !isalpha(name[i]))) {
            strncat(buf, name, i+1);
            if (!isspace(name[i]) && name[i] != '-') {
                strncat(buf, " ", 2);
            }
            return;
        }
    }
    strncat(buf, name, len);
    if (!isspace(name[len-1]) && name[len-1] != '-') {
        strncat(buf, " ", 2);
    }
}

void parse_wpn_name(char* buf, VehWeapon wpn, bool reduce) {
    const char* name = Weapon[wpn].name_short;
    const int len = min(strlen(name), (size_t)MaxProtoNameLen);
    if (reduce) {
        for (int i = 0; i < len; i++) {
            if (isspace(name[i])) {
                strncat(buf, name, i+1);
                return;
            }
        }
    }
    if (reduce && strlen(name) >= 10) {
        strncat(buf, name, 8);
        strncat(buf, " ", 2);
        return;
    }
    strncat(buf, name, MaxProtoNameLen);
    strncat(buf, " ", 2);
}

void parse_arm_name(char* buf, VehArmor arm, bool reduce) {
    const char* name = Armor[arm].name_short;
    if (reduce && strlen(name) >= 8) {
        strncat(buf, name, 4);
        strncat(buf, " ", 2);
        return;
    }
    strncat(buf, name, MaxProtoNameLen);
    strncat(buf, " ", 2);
}

void parse_chs_name(char* buf, const char* name1, const char* name2) {
    // Convert Speeder to Rover (short form)
    if (strlen(name1) >= 7 && strlen(name1) > strlen(name2) && strlen(name2)) {
        strncat(buf, name2, MaxProtoNameLen);
        strncat(buf, " ", 2);
        return;
    }
    strncat(buf, name1, MaxProtoNameLen);
    strncat(buf, " ", 2);
}

void parse_chs_name(char* buf, const char* name) {
    strncat(buf, name, MaxProtoNameLen);
    strncat(buf, " ", 2);
}

int __cdecl mod_name_proto(char* name, int unit_id, int faction_id,
VehChassis chs, VehWeapon wpn, VehArmor arm, VehAblFlag abls, VehReactor rec) {
    char buf[256] = {};
    bool noncombat = Weapon[wpn].mode == WMODE_SUPPLY
        || Weapon[wpn].mode == WMODE_PROBE
        || Weapon[wpn].mode == WMODE_TERRAFORM
        || Weapon[wpn].mode == WMODE_TRANSPORT
        || Weapon[wpn].mode == WMODE_COLONY;
    bool combat = Weapon[wpn].offense_value != 0
        && Armor[arm].defense_value != 0;
    bool psi_arm = Armor[arm].defense_value < 0;
    bool psi_wpn = Weapon[wpn].offense_value < 0;
    int wpn_v = (psi_wpn ? 2 : Weapon[wpn].offense_value);
    int arm_v = (psi_arm ? 2 : Armor[arm].defense_value);
    int triad = Chassis[chs].triad;
    int spd_v = Chassis[chs].speed;
    bool arty = abls & ABL_ARTILLERY && triad == TRIAD_LAND;
    bool sea_arty = abls & ABL_ARTILLERY && triad == TRIAD_SEA;
    bool marine = abls & ABL_AMPHIBIOUS && triad == TRIAD_LAND;
    bool intercept = abls & ABL_AIR_SUPERIORITY && triad == TRIAD_AIR;
    bool garrison = combat && !arty && !marine && wpn_v < arm_v && spd_v < 2;
    uint32_t prefix_abls = abls & ~(ABL_ARTILLERY|ABL_AMPHIBIOUS|ABL_CARRIER|ABL_NERVE_GAS
        | (intercept ? ABL_AIR_SUPERIORITY : 0)); // SAM for ground units
    // Battleship names are used only for long range artillery
    bool lrg_names = (sea_arty || triad != TRIAD_SEA || conf.long_range_artillery < 1);

    if (!conf.new_unit_names || (!combat && !noncombat) || Chassis[chs].missile) {
        if (unit_id < 0) {
            if (name) {
                name[0] = '\0';
            }
            return 0;
        }
        return name_proto(name, unit_id, faction_id, chs, wpn, arm, abls, rec);
    }
    for (int i = 0; i < 2; i++) {
        buf[0] = '\0';
        if (abls & ABL_NERVE_GAS) {
            parse_abl_name(buf, Ability[ABL_ID_NERVE_GAS].abbreviation, i > 0);
        }
        for (int j = 0; j < MaxAbilityNum; j++) {
            if (prefix_abls & (1 << j)) {
                parse_abl_name(buf, Ability[j].abbreviation, i > 0);
            }
        }
        if (arm_v != 1 && !(combat && !psi_arm && wpn_v >= (i > 0 ? 2 : 4)*arm_v)) {
            parse_arm_name(buf, arm, i > 0 || prefix_abls || spd_v > 1 || wpn_v >= arm_v
                || (noncombat && strlen(Weapon[wpn].name_short) >= 10));
        }
        if (combat) {
            if (!garrison) {
                if (triad == TRIAD_LAND && wpn_v == 1) {
                    strncat(buf, label_get(146), MaxProtoNameLen); // Scout
                    strncat(buf, " ", 2);
                } else if (triad == TRIAD_LAND && wpn_v == 2 && !psi_wpn && spd_v > 1) {
                    strncat(buf, label_get(182), MaxProtoNameLen); // Recon
                    strncat(buf, " ", 2);
                } else if (triad == TRIAD_LAND || wpn_v != 1 || arm_v == 1) {
                    parse_wpn_name(buf, wpn, i > 0);
                }
            }
            if ((!arty && !marine && !intercept) || (arty && spd_v > 1)) {
                int value = wpn_v - arm_v;
                if (value <= -1) { // Defensive
                    if ((arm_v >= 8 && wpn_v*2 >= min(24, arm_v) && lrg_names)
                    || (sea_arty && wpn_v + arm_v >= 4)) {
                        parse_chs_name(buf, Chassis[chs].defsv_name_lrg);
                    } else if (wpn_v >= 2) {
                        parse_chs_name(buf, Chassis[chs].defsv1_name);
                    } else {
                        parse_chs_name(buf, Chassis[chs].defsv2_name);
                    }
                } else if (value >= 1 || sea_arty) { // Offensive
                    if ((wpn_v >= 8 && arm_v*2 >= min(24, wpn_v) && lrg_names)
                    || (sea_arty && wpn_v + arm_v >= 4)) {
                        parse_chs_name(buf, Chassis[chs].offsv_name_lrg);
                    } else if (triad != TRIAD_AIR && arm_v >= 2) {
                        parse_chs_name(buf, Chassis[chs].offsv1_name);
                    } else if (triad == TRIAD_AIR && !((wpn_v + arm_v) & 4)) {
                        parse_chs_name(buf, Chassis[chs].offsv1_name);
                    } else {
                        parse_chs_name(buf, Chassis[chs].offsv2_name);
                    }
                } else if (triad == TRIAD_LAND && spd_v == 1) { // Scout units
                    parse_chs_name(buf, Chassis[chs].offsv1_name);
                } else {
                    parse_chs_name(buf, Chassis[chs].offsv2_name);
                }
            }
            if (arty) {
                if ((wpn_v + arm_v) & 4) {
                    strncat(buf, label_get(543), MaxProtoNameLen); // Artillery
                } else {
                    strncat(buf, label_get(544), MaxProtoNameLen); // Battery
                }
                strncat(buf, " ", 2);
            }
            if (marine) {
                if ((wpn_v + arm_v) & 4) {
                    strncat(buf, label_get(614), MaxProtoNameLen); // Marines
                } else {
                    strncat(buf, label_get(615), MaxProtoNameLen); // Invaders
                }
                strncat(buf, " ", 2);
            }
            if (intercept) {
                if ((wpn_v + arm_v) & 4) {
                    parse_chs_name(buf, Chassis[chs].defsv1_name);
                } else {
                    parse_chs_name(buf, Chassis[chs].defsv2_name);
                }
            }
        } else {
            if (spd_v > 1) {
                parse_chs_name(buf, Chassis[chs].offsv1_name, Chassis[chs].offsv2_name);
            }
            if (abls & ABL_CARRIER) {
                parse_abl_name(buf, Ability[ABL_ID_CARRIER].abbreviation, i > 0);
            } else {
                parse_wpn_name(buf, wpn, i > 0);
            }
        }
        if (Weapon[wpn].mode != WMODE_PROBE && rec >= REC_FISSION && rec <= REC_SINGULARITY) {
            if (strlen(buf) + strlen(label_unit_reactor[rec-1]) + 2 <= MaxProtoNameLen) {
                strncat(buf, label_unit_reactor[rec-1], MaxProtoNameLen);
            } else if (i == 0) {
                continue;
            }
        }
        strtrail(buf);
        if (strlen(buf) + 4 <= MaxProtoNameLen) {
            break;
        }
    }
    strncpy(name, buf, MaxProtoNameLen);
    name[MaxProtoNameLen - 1] = '\0';
    return 0;
}

VehArmor best_armor(int faction_id, int max_cost) {
    int best_id = ARM_NO_ARMOR;
    for (int i = ARM_NO_ARMOR; i <= ARM_RESONANCE_8_ARMOR; i++) {
        if (has_tech(Armor[i].preq_tech, faction_id)) {
            int cost = Armor[i].cost;
            if (max_cost > 0 && (cost > max_cost || cost > Armor[i].defense_value + 3)) {
                continue;
            }
            int diff = Armor[i].defense_value - Armor[best_id].defense_value;
            if (diff > 0 || (diff == 0 && Armor[best_id].cost >= cost)) {
                best_id = i;
            }
        }
    }
    return (VehArmor)best_id;
}

VehWeapon best_weapon(int faction_id) {
    int best_id = WPN_HAND_WEAPONS;
    for (int i = WPN_HAND_WEAPONS; i <= WPN_STRING_DISRUPTOR; i++) {
        if (has_tech(Weapon[i].preq_tech, faction_id)) {
            int diff = Weapon[i].offense_value - Weapon[best_id].offense_value;
            if (diff > 0 || (diff == 0 && Weapon[best_id].cost >= Weapon[i].cost)) {
                best_id = i;
            }
        }
    }
    return (VehWeapon)best_id;
}

VehReactor best_reactor(int faction_id) {
    for (VehReactor r : {REC_SINGULARITY, REC_QUANTUM, REC_FUSION}) {
        if (has_tech(Reactor[r - 1].preq_tech, faction_id)) {
            return r;
        }
    }
    return REC_FISSION;
}

int proto_offense(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    UNIT* u = &Units[unit_id];
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_id);
    if (u->is_missile() && !u->is_planet_buster()) {
        // Conv missiles might have nominally low attack rating
        int faction_id = unit_id / MaxProtoFactionNum;
        int max_value = max(1, (int)Weapon[best_weapon(faction_id)].offense_value);
        return max_value * w;
    }
    return Weapon[u->weapon_id].offense_value * w;
}

int proto_defense(int unit_id) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    UNIT* u = &Units[unit_id];
    int w = (conf.ignore_reactor_power ? (int)REC_FISSION : u->reactor_id);
    return Armor[u->armor_id].defense_value * w;
}

int set_move_to(int veh_id, int x, int y) {
    VEH* veh = &Vehs[veh_id];
    debug("set_move_to %2d %2d -> %2d %2d %s\n", veh->x, veh->y, x, y, veh->name());
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->order = ORDER_MOVE_TO;
    veh->status_icon = 'G';
    if (veh->is_former()) {
        veh->movement_turns = 0;
    }
    mapnodes.erase({x, y, NODE_PATROL});
    mapnodes.erase({x, y, NODE_COMBAT_PATROL});
    if (veh->x == x && veh->y == y) {
        return mod_veh_skip(veh_id);
    }
    mapdata[{x, y}].target++;
    return VEH_SYNC;
}

int set_move_next(int veh_id, int offset) {
    VEH* veh = &Vehs[veh_id];
    int x = wrap(veh->x + BaseOffsetX[offset]);
    int y = veh->y + BaseOffsetY[offset];
    return set_move_to(veh_id, x, y);
}

int set_road_to(int veh_id, int x, int y) {
    VEH* veh = &Vehs[veh_id];
    veh->waypoint_1_x = x;
    veh->waypoint_1_y = y;
    veh->order = ORDER_ROAD_TO;
    veh->status_icon = 'R';
    return VEH_SYNC;
}

int set_action(int veh_id, int act, char icon) {
    VEH* veh = &Vehs[veh_id];
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

int set_order_none(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    veh->order = ORDER_NONE;
    veh->order_auto_type = ORDERA_TERRA_AUTO_FULL;
    veh->waypoint_1_x = -1;
    veh->waypoint_1_y = -1;
    veh->status_icon = '-';
    return mod_veh_skip(veh_id);
}

int set_convoy(int veh_id, ResType res) {
    VEH* veh = &Vehs[veh_id];
    veh->order_auto_type = res-1;
    veh->order = ORDER_CONVOY;
    veh->status_icon = 'C';
    return mod_veh_skip(veh_id);
}

int set_board_to(int veh_id, int trans_veh_id) {
    VEH* veh = &Vehs[veh_id];
    VEH* v2 = &Vehs[trans_veh_id];
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

int veh_cargo_loaded(int veh_id) {
    int num = 0;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->order == ORDER_SENTRY_BOARD && veh->waypoint_1_x == veh_id
        && veh->x == Vehs[veh_id].x && veh->y == Vehs[veh_id].y) {
            assert(veh_id != i);
            num++;
        }
    }
    return num;
}

