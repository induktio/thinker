
#include "probe.h"


/*
Calculate how vulnerable the coordinates are for the specified faction based on how far
away this tile is from the faction headquarters.
Return Value: Radial distance between coordinates and faction HQ or 12 if no HQ/bases
*/
int __cdecl vulnerable(int faction_id, int x, int y) {
    int dist = 12; // default value for no bases or no HQ
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id && has_fac_built(FAC_HEADQUARTERS, i)) {
            dist = vector_dist(x, y, Bases[i].x, Bases[i].y);
            break;
        }
    }
    return dist;
}

/*
Calculate the cost for the faction to corner the Global Energy Market (Economic Victory).
Return Value: Cost to corner the Global Energy Market
*/
int __cdecl corner_market(int faction_id) {
    int cost = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id != faction_id) {
            if (!has_treaty(base->faction_id, faction_id, DIPLO_PACT)
            || !has_treaty(base->faction_id, faction_id, DIPLO_HAVE_SURRENDERED)) {
                cost += mod_mind_control(i, faction_id, true);
            }
        }
    }
    return max(1000, cost);
}

/*
Calculate the amount of energy that can be stolen from a base based on its population.
Return Value: Energy taken
*/
int __cdecl steal_energy(int base_id) {
    BASE* base = &Bases[base_id];
    int energy = Factions[base->faction_id].energy_credits;
    return (energy <= 0) ? 0
        : ((energy * Bases[base_id].pop_size) / (Factions[base->faction_id].pop_total + 1));
}

/*
Calculate the cost for the faction to be able to mind control the specified base. The 3rd
parameter determines if this cost is for cornering the market (true) or via probe (false).
Return Value: Mind control cost
*/
int __cdecl mod_mind_control(int base_id, int faction_id, int is_corner_market) {
    BASE* base = &Bases[base_id];
    int dist = vulnerable(base->faction_id, base->x, base->y);
    if (dist <= 0) {
        if (!is_corner_market) {
            return -1;
        }
        dist = 1;
    }
    if (has_fac_built(FAC_GENEJACK_FACTORY, base_id)) {
        dist *= 2;
    }
    if (has_fac_built(FAC_CHILDREN_CRECHE, base_id)) {
        dist /= 2;
    }
    if (has_fac_built(FAC_PUNISHMENT_SPHERE, base_id)) {
        dist /= 2;
    }
    if (base->nerve_staple_turns_left) {
        dist /= 2;
    }
    int veh_id = stack_fix(veh_at(base->x, base->y));
    int cost = ((mod_stack_check(veh_id, 2, PLAN_COMBAT, -1, -1)
        + mod_stack_check(veh_id, 2, PLAN_OFFENSE, -1, -1))
        * (mod_stack_check(veh_id, 6, ABL_POLY_ENCRYPTION, -1, -1) + 1)
        + Factions[faction_id].mind_control_total / 4 + base->pop_size)
        * ((Factions[base->faction_id].corner_market_active
        + Factions[base->faction_id].energy_credits + 1200) / (dist + 4));
    if (!is_human(faction_id) && is_human(base->faction_id)) {
        int diff = Factions[base->faction_id].diff_level;
        if (diff > DIFF_LIBRARIAN) {
            cost = (cost * 3) / diff;
        }
    }
    bool is_pact = has_treaty(faction_id, base->faction_id, DIPLO_PACT);
    if (is_corner_market) {
        if (is_pact) {
            cost /= 2;
        }
        if (has_treaty(faction_id, base->faction_id, DIPLO_TREATY)) {
            cost /= 2;
        }
        int tech_comm_target = Factions[base->faction_id].tech_commerce_bonus;
        tech_comm_target *= tech_comm_target;
        int tech_comm_probe = Factions[faction_id].tech_commerce_bonus;
        tech_comm_probe *= tech_comm_probe;
        cost = (cost * (tech_comm_target + 1)) / (tech_comm_probe + 1);
    } else if (is_pact) {
        cost *= 2;
    }
    if (base->faction_id_former == faction_id) {
        cost /= 2;
    }
    if (base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE) {
        cost /= 2;
    }
    if (base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
        cost *= 2;
    }
    if (has_treaty(base->faction_id, faction_id, DIPLO_ATROCITY_VICTIM)) {
        cost *= 2;
    } else if (has_treaty(base->faction_id, faction_id, DIPLO_WANT_REVENGE)) {
        cost += cost / 2;
    }
    assert(cost == mind_control(base_id, faction_id, is_corner_market));
    return cost;
}

/*
Calculate the success and survival rates for a probe action based on the probe's morale and
the difficulty of the action. These are used to generate a chances probability string for
provided index. A base_id is an optional parameter to factor in its probe defenses.
Return Value: Success rate of probe
*/
int __cdecl mod_success_rates(size_t index, int morale, int diff_modifier, int base_id) {
    char chances[32];
    int success_rate;
    if (diff_modifier < 0) {
        snprintf(chances, 32, "100%%");
        success_rate = diff_modifier;
    } else {
        if (morale < 1) {
            morale = 1;
        }
        int prb_defense = (base_id >= 0 && has_fac_built(FAC_COVERT_OPS_CENTER, base_id)) ? 2 : 0;
        prb_defense = clamp(Factions[*ProbeTargetFactionID].SE_probe + prb_defense, -2, 0);
        int failure_rate = (diff_modifier * 100) / ((morale / 2) - prb_defense + 1);
        if (*ProbeHasAlgoEnhancement && !*ProbeTargetHasHSA) {
            failure_rate /= 2; // Algo Ench: failure cut in half when acting against normal targets
        }
        success_rate = 100 - failure_rate;
        if (*ProbeTargetHasHSA) {
            success_rate /= 2; // Chance of success is half what the chance would have been w/o HSA
        }
        int loss_rate = ((diff_modifier + 1) * 100) / (morale - prb_defense);
        if (*ProbeHasAlgoEnhancement && !*ProbeTargetHasHSA) {
            loss_rate /= 2;
        }
        int survival_rate = 100 - loss_rate;
        if (*ProbeTargetHasHSA) {
            survival_rate /= 2; // Fix: original had an erroneous 2nd hit to success_rate
        }
        // Original game did not display the other percentage if both are the same
        snprintf(chances, 32, "%d%%, %d%%", success_rate, survival_rate);
    }
    parse_says(index, chances, -1, -1);
    assert(success_rate == success_rates(index, morale, diff_modifier, base_id));
    return success_rate;
}

/*
Calculate current vehicle health only to determine
the damage from genetic warfare probe team action.
*/
int __cdecl probe_veh_health(int veh_id)
{
    VEH* veh = &Vehs[veh_id];
    int level = veh->reactor_type();
    if (veh->is_artifact()) {
        return 1;
    }
    if (veh->damage_taken > 5*level) {
        return 1;
    }
    return clamp(min(5*level, 10*level - veh->damage_taken), 0, 9999);
}

/*
Replace distance to determine if the adjacent units to the base should also be subverted.
*/
int __cdecl probe_mind_control_range(int x1, int y1, int x2, int y2)
{
    return (x1 == x2 && y1 == y2) ? 0 : 2;
}

/*
Replaces set_treaty call when Total Thought Control probe action is completed.
This does not result in automatic vendetta unlike normal Mind Control action.
*/
void __cdecl probe_thought_control(int faction_id_def, int faction_id_atk)
{
    if (!at_war(faction_id_def, faction_id_atk)) {
        cause_friction(faction_id_def, faction_id_atk, 5);
        Factions[faction_id_def].diplo_betrayed[faction_id_atk]++;
        Factions[faction_id_atk].diplo_wrongs[faction_id_def]++;
        if (has_pact(faction_id_def, faction_id_atk)) {
            set_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_TO_TALK, 1);
        }
    }
}

int probe_roll_value(int faction)
{
    int techs = 0;
    for (int i = Tech_ID_First; i <= Tech_ID_Last; i++) {
        if (Tech[i].preq_tech1 != TECH_Disable && has_tech(i, faction)
        && Tech[i].flags & TFLAG_IMPROVE_PROBE) {
            techs++;
        }
    }
    return 2*techs + 2*clamp(Factions[faction].SE_probe, -3, 3)
        + clamp(Factions[faction].SE_probe_base, -3, 3)
        + clamp(Factions[faction].SE_police, -3, 3);
}

int probe_active_turns(int faction1, int faction2)
{
    int value = clamp(15 + probe_roll_value(faction1) - probe_roll_value(faction2), 5, 50);
    value = value * (4 + (*MapAreaTiles >= 4000) + (*MapAreaTiles >= 8000)) / 4;
    value = value * (4 + (*DiffLevel < DIFF_TRANSCEND) + (*DiffLevel < DIFF_THINKER)) / 4;
    return clamp(value, 5, 50);
}

int probe_upkeep(int faction1)
{
    if (!faction1 || !is_alive(faction1) || !conf.counter_espionage) {
        return 0;
    }
    /*
    Do not expire infiltration while the faction is the governor or has the Empath Guild.
    Status can be renewed once per turn and sets the flag DIPLO_RENEW_INFILTRATOR.
    This is checked in patched version of probe() game code.
    */
    for (int faction2 = 1; faction2 < MaxPlayerNum; faction2++) {
        if (faction1 != faction2 && is_alive(faction2)
        && faction2 != *GovernorFaction && !has_project(FAC_EMPATH_GUILD, faction2)) {
            if (has_treaty(faction2, faction1, DIPLO_HAVE_INFILTRATOR)) {
                if (!MFactions[faction2].thinker_probe_end_turn[faction1]) {
                    MFactions[faction2].thinker_probe_end_turn[faction1] =
                        *CurrentTurn + probe_active_turns(faction2, faction1);
                }
                if (MFactions[faction2].thinker_probe_end_turn[faction1] <= *CurrentTurn) {
                    net_set_treaty(faction2, faction1, DIPLO_HAVE_INFILTRATOR, 0, 0);
                    MFactions[faction2].thinker_probe_lost |= (1 << faction1);
                    if (faction1 == MapWin->cOwner) {
                        parse_says(0, MFactions[faction2].adj_name_faction, -1, -1);
                        popp("modmenu", "SPYFOUND", 0, "infil_sm.pcx", 0);
                    }
                }
            }
        }
    }
    for (int faction2 = 1; faction2 < MaxPlayerNum; faction2++) {
        if (faction1 != faction2 && is_alive(faction2)) {
            debug("probe_upkeep %3d %d %d spying: %d ends: %d\n",
                *CurrentTurn, faction1, faction2,
                has_treaty(faction1, faction2, DIPLO_HAVE_INFILTRATOR),
                MFactions[faction1].thinker_probe_end_turn[faction2]
            );
            if (faction1 != *GovernorFaction && !has_project(FAC_EMPATH_GUILD, faction1)) {
                net_set_treaty(faction1, faction2, DIPLO_RENEW_INFILTRATOR, 0, 0);
            }
            if (faction1 == MapWin->cOwner && MFactions[faction1].thinker_probe_lost & (1 << faction2)) {
                parse_says(0, MFactions[faction2].noun_faction, -1, -1);
                popp("modmenu", "SPYLOST", 0, "capture_sm.pcx", 0);
            }
        }
    }
    MFactions[faction1].thinker_probe_lost = 0;
    return 0;
}

int __thiscall probe_popup_start(Win* This, int veh_id1, int base_id, int a4, int a5, int a6, int a7)
{
    if (base_id >= 0 && base_id < *BaseCount) {
        int faction1 = Vehs[veh_id1].faction_id;
        int faction2 = Bases[base_id].faction_id;
        int turns = MFactions[faction1].thinker_probe_end_turn[faction2] - *CurrentTurn;
        bool always_active = faction1 == *GovernorFaction || has_project(FAC_EMPATH_GUILD, faction1);

        if (!always_active) {
            if (has_treaty(faction1, faction2, DIPLO_HAVE_INFILTRATOR) && turns > 0) {
                ParseNumTable[0] =  turns;
                return Popup_start(This, "modmenu", "PROBE", a4, a5, a6, a7);
            }
            // Sometimes this flag is set even when infiltration is not active
            net_set_treaty(faction1, faction2, DIPLO_RENEW_INFILTRATOR, 0, 0);
        }
    }
    return Popup_start(This, ScriptFile, "PROBE", a4, a5, a6, a7);
}

