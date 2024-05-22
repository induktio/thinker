
#include "probe.h"


/*
Calculate current vehicle health only to determine
the damage from genetic warfare probe team action.
*/
int __cdecl probe_veh_health(int veh_id)
{
    VEH* veh = &Vehs[veh_id];
    int level = clamp(veh->reactor_type(), 1, 100);
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

