
#include "probe.h"


static void popup_init_image(Popup* popup, const char* image, int* bx, int* by) {
    if (!Buffer_get_pcx_dimensions(image, bx, by)) {
        if (!Sprite_init(&popup->sprite, image, *bx, *by)) {
            popup->field_2144 = &popup->sprite;
        }
    }
}

static int find_probe_base(int veh_id, int veh_fc_id) {
    int reg = region_at(Vehs[veh_id].x, Vehs[veh_id].y);
    int sea = is_ocean(mapsq(Vehs[veh_id].x, Vehs[veh_id].y));
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == veh_fc_id && is_port(i, 0)) {
            if (!sea || base_on_sea(i, reg)) {
                return i;
            }
        }
    }
    return -1;
}

std::vector<int> captured_leaders(int faction_id) {
    std::vector<int> values;
    for (int i = 0; i < MaxPlayerNum - 1; i++) {
        if (Monuments[faction_id].data2[i][9]) {
            int plr_id = Monuments[faction_id].data2[i][7];
            if (plr_id >= 0 && plr_id < MaxPlayerNum
            && plr_id != faction_id && !is_alive(plr_id)) {
                values.push_back({plr_id});
            } else {
                assert(0);
            }
        }
    }
    return values;
}

void reset_captured_leader(int faction_id, int capture_id) {
    for (int i = 0; i < MaxPlayerNum - 1; i++) {
        if (Monuments[faction_id].data2[i][7] == capture_id
        && Monuments[faction_id].data2[i][9]) {
            Monuments[faction_id].data2[i][9] = 0;
        }
    }
}

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
        * ((Factions[base->faction_id].corner_market_cost
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
    assert(success_rate == success_rates(index, morale, diff_modifier, base_id));
    parse_says(index, chances, -1, -1);
    return success_rate;
}

void __cdecl probe_renew_set(int faction_id, int faction_id_tgt, int turns) {
    if (faction_id >= 0 && faction_id < MaxPlayerNum
    && faction_id_tgt >= 0 && faction_id_tgt < MaxPlayerNum) {
        if (turns > 0) {
            MFactions[faction_id].thinker_probe_end_turn[faction_id_tgt] = *CurrentTurn + turns;
            MFactions[faction_id].thinker_probe_renew |= (1 << faction_id_tgt);
        } else {
            MFactions[faction_id].thinker_probe_renew &= ~(1 << faction_id_tgt);
        }
    } else  {
        assert(0);
    }
}

int __cdecl probe_has_renew(int faction_id, int faction_id_tgt) {
    if (faction_id >= 0 && faction_id < MaxPlayerNum) {
        return MFactions[faction_id].thinker_probe_renew & (1 << faction_id_tgt);
    } else {
        assert(0);
        return 0;
    }
}

int probe_roll_value(int faction_id) {
    int techs = 0;
    for (int i = Tech_ID_First; i <= Tech_ID_Last; i++) {
        if (Tech[i].preq_tech1 != TECH_Disable && has_tech(i, faction_id)
        && Tech[i].flags & TFLAG_IMPROVE_PROBE) {
            techs++;
        }
    }
    return 2*techs + 2*clamp(Factions[faction_id].SE_probe, -3, 3)
        + clamp(Factions[faction_id].SE_probe_base, -3, 3)
        + clamp(Factions[faction_id].SE_police, -3, 3);
}

int probe_active_turns(int faction1, int faction2) {
    int value = clamp(15 + probe_roll_value(faction1) - probe_roll_value(faction2), 5, 50);
    value = value * (4 + (*MapAreaTiles >= 4000) + (*MapAreaTiles >= 8000)) / 4;
    value = value * (4 + (*DiffLevel < DIFF_TRANSCEND) + (*DiffLevel < DIFF_THINKER)) / 4;
    return clamp(value, 5, 50);
}

int probe_upkeep(int faction1) {
    if (!faction1 || !is_alive(faction1) || !conf.counter_espionage) {
        return 0;
    }
    /*
    Do not expire infiltration while the faction is the governor or has the Empath Guild.
    Status can be renewed once per turn and sets the flag on thinker_probe_renew.
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
            debug("probe_upkeep %d %d %d spying: %d ends: %d\n",
                *CurrentTurn, faction1, faction2,
                has_treaty(faction1, faction2, DIPLO_HAVE_INFILTRATOR),
                MFactions[faction1].thinker_probe_end_turn[faction2]
            );
            if (faction1 != *GovernorFaction && !has_project(FAC_EMPATH_GUILD, faction1)) {
                probe_renew_set(faction1, faction2, 0);
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

int __thiscall probe_popup_start(Popup* This, int veh_id, int base_id, int a4, char* a5, int a6, GraphicWin* a7) {
    if (base_id >= 0 && base_id < *BaseCount) {
        int faction1 = Vehs[veh_id].faction_id;
        int faction2 = Bases[base_id].faction_id;
        int turns = MFactions[faction1].thinker_probe_end_turn[faction2] - *CurrentTurn;
        bool always_active = faction1 == *GovernorFaction || has_project(FAC_EMPATH_GUILD, faction1);

        if (!always_active) {
            if (has_treaty(faction1, faction2, DIPLO_HAVE_INFILTRATOR) && turns > 0) {
                ParseNumTable[0] =  turns;
                return Popup_start(This, "modmenu", "PROBE", a4, a5, a6, a7);
            }
            probe_renew_set(faction1, faction2, 0);
        }
    }
    return Popup_start(This, ScriptFile, "PROBE", a4, a5, a6, a7);
}

/*
Option probe_action_fix enables several useful feature changes and fixes issues.
For random sources GameRandom instance replaces previous uses of engine game_random.
Externally reseeded game_rand is used as before but random replaces any instances of timeGetTime.
*/
int __cdecl probe(int veh_id, int tgt_base_id, int tgt_veh_id, int toggle) {
    debug("probe %d %d %d %d\n", veh_id, tgt_base_id, tgt_veh_id, toggle);
    Ftext_get text_get = (Ftext_get)0x5FD570;

    GameRandom loc_rnd;
    char name_buf[StrBufLen];
    int prb_action_val;
    int prb_free_leader;
    int prb_alt_action;
    int prb_action_check;
    int gene_warfare_known;
    int gene_warfare_allow;
    int gene_warfare_techs;
    int bx;
    int by;
    int action_id;
    int sabotage_id;
    int prb_tech_id;
    int prb_diff;
    int tgt_region;
    int tgt_arm_strat;
    int tgt_wpn_strat;
    int tgt_chs_speed;
    int mc_cost;
    int veh_mc_cost;

    Popup cur_popup = {};
    Popup_ctor(&cur_popup);
    auto guard = cleanup_handler([&] { Popup_dtor(&cur_popup); });

    const int veh_fc_id = Vehs[veh_id].faction_id;
    const int tgt_fc_id = (tgt_base_id >= 0 ? Bases[tgt_base_id].faction_id : Vehs[tgt_veh_id].faction_id);
    const int plr_alien = (MFactions[veh_fc_id].is_alien() != 0);
    const char* infil_img = (plr_alien ? "al_infil_sm.pcx" : "infil_sm.pcx");
    const bool veh_at_sea = is_ocean(mapsq(Vehs[veh_id].x, Vehs[veh_id].y));
    const bool tgt_at_sea = (tgt_base_id >= 0 ? is_ocean(&Bases[tgt_base_id]) :
        is_ocean(mapsq(Vehs[tgt_veh_id].x, Vehs[tgt_veh_id].y)));
    Faction* const plr = &Factions[veh_fc_id];
    Faction* const tgt = &Factions[tgt_fc_id];

    prb_alt_action = 0;
    prb_action_check = 0;
    prb_free_leader = -1;
    *ProbeTargetFactionID = tgt_fc_id;

    if (veh_at_sea && !tgt_at_sea && Vehs[veh_id].triad() != TRIAD_SEA) {
        if (!has_abil(Vehs[veh_id].unit_id, ABL_AMPHIBIOUS)) {
            if (veh_fc_id == MapWin->cOwner && toggle && is_human(veh_fc_id) && *VehAttackFlags & 1) {
                parse_says(0, Ability[abil_index(8)].name, -1, -1);
                NetMsg_pop(NetMsg, "PROBEABILAMPHIBIOUS", 5000, 0, 0);
            }
            return 0;
        }
    }
    if (!veh_at_sea && tgt_at_sea && !has_abil(Vehs[veh_id].unit_id, ABL_AMPHIBIOUS)) {
        if (veh_fc_id == MapWin->cOwner && toggle && is_human(veh_fc_id) && *VehAttackFlags & 1) {
            parse_says(0, Ability[abil_index(8)].name, -1, -1);
            NetMsg_pop(NetMsg, "PROBEABILAMPHIBIOUS2", 5000, 0, 0);
        }
        return 0;
    }
    *ProbeTargetHasHSA = 0;
    *ProbeHasAlgoEnhancement = 0;
    if (!has_abil(Vehs[veh_id].unit_id, ABL_ALGO_ENHANCEMENT)) {
        if (has_project(FAC_HUNTER_SEEKER_ALGORITHM, tgt_fc_id)) {
            if (!toggle && is_human(veh_fc_id)) {
                return 0;
            }
            if (*VehAttackFlags & 2 && toggle) {
                if (veh_fc_id == MapWin->cOwner) {
                    parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                    NetMsg_pop(NetMsg, "HUNTERSEEKER", -5000, 0, "huntsk_sm.pcx");
                } else if (tgt_fc_id == MapWin->cOwner) {
                    parse_says(0, MFactions[veh_fc_id].adj_name_faction, -1, -1);
                    NetMsg_pop(NetMsg, "HUNTERSEEKER2", -5000, 0, "huntsk_sm.pcx");
                }
                kill(veh_id);
            }
            return 1;
        }
    } else {
        *ProbeHasAlgoEnhancement = 1;
    }
    if (has_project(FAC_HUNTER_SEEKER_ALGORITHM, tgt_fc_id)) {
        if (!toggle && is_human(veh_fc_id)) {
            return 0;
        }
        if (!(*VehAttackFlags & 2) || !toggle) {
            return 1;
        }
        *ProbeTargetHasHSA = 1;
    }
    gene_warfare_techs = 0;
    gene_warfare_known = 0;
    gene_warfare_allow = 0;
    prb_tech_id = 0;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        if (Tech[i].flags & TFLAG_ALLOW_GENE_WARFARE && has_tech(i, veh_fc_id)) {
            gene_warfare_allow = 1;
        }
        if (Tech[i].flags & TFLAG_INC_GENE_WARFARE_DEFENSE) {
            ++gene_warfare_techs;
            if (has_tech(i, tgt_fc_id)) {
                ++gene_warfare_known;
            }
        }
        ++prb_tech_id;
    }
    if (tgt_base_id < 0) {
        action_id = -1;
    } else if (!is_human(veh_fc_id)) {
        goto MOV_CHECK;
    } else if (veh_fc_id != MapWin->cOwner || !(*VehAttackFlags & 1)) {
        action_id = Vehs[veh_id].probe_action & 7;
    } else {
        bool tgt_hq = has_fac_built(FAC_HEADQUARTERS, tgt_base_id);
        parse_says(1, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
        if (conf.counter_espionage) {
            probe_popup_start(&cur_popup, veh_id, tgt_base_id, -1, 0, 64, 0);
        } else {
            Popup_start(&cur_popup, PopupScriptFile, "PROBE", -1, 0, 64, 0);
        }
        popup_init_image(&cur_popup, infil_img, &bx, &by);
        if (X_text_open(ScriptFile, "PROBEMENU")) {
            return 0;
        }
        text_get();
        if (conf.counter_espionage ? !probe_has_renew(veh_fc_id, tgt_fc_id)
        : !(plr->diplo_status[tgt_fc_id] & DIPLO_HAVE_INFILTRATOR)) {
            Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 0);
        }
        text_get();
        if (Rules->tgl_probe_steal_tech) {
            Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 1);
        }
        text_get();
        Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 2);
        text_get();
        Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 3);
        text_get();
        Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 4);
        text_get();
        if (tgt_hq) {
            Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 5);
        }
        text_get();
        if (!tgt_hq) {
            Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 6);
        }
        text_get();
        if (gene_warfare_allow) {
            Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 7);
        }
        if (*ExpansionEnabled && !*MultiplayerActive) {
            text_get();
            if (tgt_hq && captured_leaders(tgt_fc_id).size()) {
                Dialogs_item(&cur_popup.dialogs, text_buf_ptr(), 8);
            }
        }
        action_id = BasePop_exec_3(&cur_popup, 0, 0);
        if (action_id < 0) {
            return 0;
        }
        Vehs[veh_id].probe_action = action_id;
    }
    while (true) {
MOV_START:
        while (true) {
            if (action_id == PRB_INFILTRATE_DATALINKS && *VehAttackFlags & 2) {
                if (conf.counter_espionage ? !probe_has_renew(veh_fc_id, tgt_fc_id)
                : !(plr->diplo_status[tgt_fc_id] & DIPLO_HAVE_INFILTRATOR)) {
                    set_treaty(veh_fc_id, tgt_fc_id, DIPLO_HAVE_INFILTRATOR, 1);
                    if (veh_fc_id == MapWin->cOwner) {
                        report_intel(tgt_fc_id);
                    }
                }
                if (tgt_fc_id == MapWin->cOwner) {
                    parse_says(0, get_title(veh_fc_id), -1, -1);
                    parse_says(1, get_name(veh_fc_id), -1, -1);
                    parse_says(2, get_noun(veh_fc_id), -1, -1);
                    NetMsg_pop(NetMsg, "DETECTINFILTRATE", -5000, 0, infil_img);
                }
            }
            if (veh_fc_id == MapWin->cOwner) {
                if (is_human(veh_fc_id)) {
                    if (*VehAttackFlags & 1) {
                        if (plr->diplo_status[tgt_fc_id] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
                            if (action_id >= PRB_PROCURE_RESEARCH_DATA
                            && (!(tgt->diplo_status[veh_fc_id] & DIPLO_PACT)
                            || !(tgt->diplo_status[veh_fc_id] & DIPLO_HAVE_SURRENDERED))) {
                                parse_says(0, get_title(tgt_fc_id), -1, -1);
                                parse_says(1, get_name(tgt_fc_id), -1, -1);
                                if (!popp(ScriptFile, "RISKEXCUSE", 0, infil_img, 0)) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
            prb_diff = 0;
            if (action_id != PRB_PROCURE_RESEARCH_DATA) {
                break;
            }
            if (Bases[tgt_base_id].state_flags & BSTATE_RESEARCH_DATA_STOLEN) {
                prb_diff = 1;
                if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id) && *VehAttackFlags & 1) {
                    parse_says(0, Bases[tgt_base_id].name, -1, -1);
                    mod_success_rates(1, mod_morale_veh(veh_id, 1, 0), -1, tgt_base_id);
                    if (!mod_success_rates(2, mod_morale_veh(veh_id, 1, 0), 1, tgt_base_id)) {
                        NetMsg_pop(NetMsg, "ADVDECIPHER1", -5000, 0, 0);
                        return 0;
                    }
                    if (!popp(ScriptFile, "ADVDECIPHER", 0, infil_img, 0)) {
                        return 0;
                    }
                }
            }
            int tech_rnd;
            if (*MultiplayerActive && is_human(veh_fc_id)) {
                loc_rnd.reseed(veh_fc_id + *CurrentTurn + *MapRandomSeed);
                tech_rnd = loc_rnd.get(0, MaxTechnologyNum);
            } else {
                tech_rnd = game_rand() % MaxTechnologyNum;
            }
            int num;
            for (num = 0; num < MaxTechnologyNum; num++) {
                // Fix issue that might prevent discovering new techs if tech_rnd is zero
                prb_tech_id = (tech_rnd + (conf.probe_action_fix ? num : prb_tech_id)) % MaxTechnologyNum;
                if (has_tech(prb_tech_id, tgt_fc_id) && !has_tech(prb_tech_id, veh_fc_id)) {
                    break;
                }
            }
            if (num >= MaxTechnologyNum) {
                prb_tech_id = 9999;
            }
            if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id)) {
                mod_success_rates(1, mod_morale_veh(veh_id, 1, 0), prb_diff, tgt_base_id);
                if (mod_success_rates(2, mod_morale_veh(veh_id, 1, 0), prb_diff + 1, tgt_base_id)) {
                    int choice;
                    if (*VehAttackFlags & 2) {
                        choice = (Vehs[veh_id].probe_action >> 3) & 1;
                    } else {
                        choice = popp(ScriptFile, "DECIPHER", 64, infil_img, 0);
                        if (choice < 0) {
                            return 0;
                        }
                        Vehs[veh_id].probe_action |= 8 * (uint8_t)choice;
                    }
                    if (choice == 1) {
                        prb_diff += 1;
                        prb_tech_id = -1;
                        goto MOV_FRAME;
                    }
                }
            }
            if (prb_tech_id != 9999 || !(plr->player_flags & PFLAG_MAP_REVEALED)) {
                goto MOV_FRAME;
            }
            if (is_human(veh_fc_id)) {
                if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 2) {
                    NetMsg_pop(NetMsg, "STOLENOTHING", -5000, 0, 0);
                    return 0;
                }
                goto MOV_FRAME;
            }
            action_id = (plr->diplo_status[tgt_fc_id] & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA))
                ? PRB_ACTIVATE_SABOTAGE_VIRUS : PRB_INFILTRATE_DATALINKS;
        }
        switch (action_id) {
        case PRB_DRAIN_ENERGY_RESERVES:
            if (Bases[tgt_base_id].state_flags & BSTATE_ENERGY_RESERVES_DRAINED) {
                prb_diff = 1;
                if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id) && *VehAttackFlags & 1) {
                    parse_says(0, Bases[tgt_base_id].name, -1, -1);
                    int morale = mod_morale_veh(veh_id, 1, 0);
                    mod_success_rates(1, morale, -1, tgt_base_id);
                    if (!mod_success_rates(2, morale, 1, tgt_base_id)) {
                        NetMsg_pop(NetMsg, "ADVENERGY1", -5000, 0, 0);
                        return 0;
                    }
                    if (!popp(ScriptFile, "ADVENERGY", 0, infil_img, 0)) {
                        return 0;
                    }
                }
            }
            goto MOV_FRAME;
        case PRB_ACTIVATE_SABOTAGE_VIRUS:
            int queue_id, item_cost, rnd_val, veh_rnd_val;
            sabotage_id = 0;
            queue_id = Bases[tgt_base_id].queue_items[0];
            if (queue_id < 0) {
                snprintf(name_buf, StrBufLen, "%s", Facility[-queue_id].name);
                item_cost = Facility[-Bases[tgt_base_id].queue_items[0]].cost;
            } else {
                snprintf(name_buf, StrBufLen, "%s", Units[queue_id].name);
                item_cost = Units[Bases[tgt_base_id].queue_items[0]].cost;
            }
            if (*MultiplayerActive && is_human(veh_fc_id)) {
                loc_rnd.reseed(veh_fc_id + *CurrentTurn + *MapRandomSeed);
                veh_rnd_val = loc_rnd.get(0, mod_morale_veh(veh_id, 1, 0) / 2 + 2);
            } else {
                veh_rnd_val = game_randv(mod_morale_veh(veh_id, 1, 0) / 2 + 2);
            }
            if (!veh_rnd_val && Bases[tgt_base_id].minerals_accumulated
            >= item_cost * mod_cost_factor(veh_fc_id, RSC_MINERAL, -1)) {
                goto MOV_SABOTAGE;
            }
            rnd_val = game_rand() % Fac_All_ID_Last;
            for (int i = 1; i <= Fac_All_ID_Last; i++) {
                FacilityId facility_id = (FacilityId)((rnd_val + i) % Fac_All_ID_Last + 1);
                if (facility_id == FAC_HEADQUARTERS) {
                    int cur_id = veh_at(Bases[tgt_base_id].x, Bases[tgt_base_id].y);
                    if (mod_stack_check(cur_id, 2, 5, -1, -1)) {
                        sabotage_id = 98;
                        goto MOV_SABOTAGE;
                    }
                } else if (facility_id <= Fac_ID_Last
                && (facility_id != FAC_PRESSURE_DOME || !is_ocean(&Bases[tgt_base_id]))
                && has_fac_built(facility_id, tgt_base_id)) {
                    sabotage_id = facility_id;
                    goto MOV_SABOTAGE;
                }
            }
            if (!(is_human(veh_fc_id))) {
                action_id = Bases[tgt_base_id].pop_size <= 3 ? PRB_INFILTRATE_DATALINKS : PRB_INCITE_DRONE_RIOTS;
                goto MOV_START;
            }
            goto MOV_SABOTAGE;
        case PRB_ASSASSINATE_PROMINENT_RESEARCHERS:
            prb_diff = 1;
            if (mod_success_rates(1, mod_morale_veh(veh_id, 1, 0), 1, tgt_base_id)) {
                if (veh_fc_id != MapWin->cOwner || !(is_human(veh_fc_id)) || !(*VehAttackFlags & 1)) {
                    goto MOV_FRAME;
                }
                if (popp(ScriptFile, "ASSASSIN", 0, infil_img, 0)) {
                    goto MOV_FRAME;
                }
            } else {
                if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id) && *VehAttackFlags & 1) {
                    NetMsg_pop(NetMsg, "NOASSASSIN", -5000, 0, 0);
                }
            }
            return 0;
        case PRB_FREE_CAPTURED_FACTION_LEADER:
            prb_diff = 1;
            if (mod_success_rates(1, mod_morale_veh(veh_id, 1, 0), 1, tgt_base_id)) {
                if (veh_fc_id != MapWin->cOwner || !(is_human(veh_fc_id)) || !(*VehAttackFlags & 1)) {
                    goto MOV_FRAME;
                }
                if (popp(ScriptFile, "FREEWILLY", 0, infil_img, 0)) {
                    goto MOV_FRAME;
                }
            } else {
                if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id)) {
                    NetMsg_pop(NetMsg, "NOFREEWILLY", -5000, 0, 0);
                }
            }
            return 0;
        case PRB_MIND_CONTROL_UNIT:
            if (mod_stack_check(tgt_veh_id, 1, -1, -1, -1) > 1) {
                return 0;
            }
            if (Vehs[tgt_veh_id].triad() == TRIAD_AIR
                    && !has_abil(Vehs[veh_id].unit_id, ABL_AIR_SUPERIORITY)) {
                if (veh_fc_id == MapWin->cOwner && toggle && is_human(veh_fc_id) && *VehAttackFlags & 1) {
                    parse_says(0, Ability[abil_index(32)].name, -1, -1);
                    NetMsg_pop(NetMsg, "PROBEABILSUPERIORITY", 5000, 0, 0);
                }
                return 0;
            }
            if (!tgt_fc_id || Vehs[tgt_veh_id].is_native_unit()) { // remove redundant Spore Launcher reference
                if (veh_fc_id == MapWin->cOwner && toggle && is_human(veh_fc_id) && *VehAttackFlags & 1) {
                    NetMsg_pop(NetMsg, "ALIENPROBE", -5000, 0, 0);
                }
                return 0;
            }
            int veh_mc_dist, plr_human;
            VEH* tgt_veh;
            tgt_veh = &Vehs[tgt_veh_id];
            veh_mc_dist = vulnerable(tgt_fc_id, tgt_veh->x, tgt_veh->y);
            plr_human = is_human(veh_fc_id);
            if (tgt_veh->home_base_id >= 0 && has_fac_built(FAC_PUNISHMENT_SPHERE, tgt_veh->home_base_id)) {
                veh_mc_dist /= 2;
            }
            veh_mc_cost = Units[Vehs[tgt_veh_id].unit_id].cost * ((tgt->energy_credits + 800) / (veh_mc_dist + 2));
            if (!plr_human && is_human(tgt_fc_id) && tgt->diff_level > 3) {
                veh_mc_cost = 3 * veh_mc_cost / tgt->diff_level; // original AI discount
            }
            if (MFactions[tgt_fc_id].rule_flags & RFLAG_MINDCONTROL) {
                if (veh_fc_id == MapWin->cOwner && plr_human && toggle && *VehAttackFlags & 1) {
                    parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                    NetMsg_pop(NetMsg, "MINDIMMUNITY2", -5000, 0, 0);
                }
                return 0;
            }
            for (int i = 0; i < MFactions[veh_fc_id].faction_bonus_count; ++i) {
                if (MFactions[veh_fc_id].faction_bonus_id[i] == RULE_PROBECOST) {
                    veh_mc_cost = (MFactions[veh_fc_id].faction_bonus_val1[i] * veh_mc_cost) / 100;
                }
            }
            if (has_project(FAC_NETHACK_TERMINUS, veh_fc_id)) {
                veh_mc_cost = 3 * veh_mc_cost / 4;
            }
            // Fix 15. [BUG] Enhanced probes (Algorithmic Enhancement) are now able to mind control
            // bases/units normally immune due to high SE morale as stated in SMAX manual.
            if (tgt->SE_probe <= -2) {
                veh_mc_cost /= 2;
            } else if (tgt->SE_probe == -1) {
                veh_mc_cost += (veh_mc_cost / -4);
            } else if (tgt->SE_probe == 0) {
                // nothing
            } else if (tgt->SE_probe == 1) {
                veh_mc_cost += (veh_mc_cost / 2);
            } else if (tgt->SE_probe == 2 || (*ProbeHasAlgoEnhancement == 1)) {
                veh_mc_cost *= 2;
            } else {
                if (veh_fc_id == MapWin->cOwner && plr_human && toggle && *VehAttackFlags & 1) {
                    parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                    NetMsg_pop(NetMsg, "IMMUNITY", -5000, 0, 0);
                }
                return 0;
            }
            if (has_abil(Vehs[tgt_veh_id].unit_id, ABL_POLY_ENCRYPTION)) {
                veh_mc_cost *= 2;
            }
            if (Vehs[tgt_veh_id].plan() != PLAN_COLONY && Vehs[tgt_veh_id].plan() != PLAN_TERRAFORM) {
                veh_mc_cost /= 2;
            }
            parse_says(0, Vehs[tgt_veh_id].name(), -1, -1);
            if (!is_human(veh_fc_id) || (Vehs[veh_id].state & VSTATE_ON_ALERT)) {
                UNIT* utgt = &Units[Vehs[tgt_veh_id].unit_id];
                int alt_cost_val = 4 * veh_mc_cost * (plr->AI_power - plr->AI_wealth + 2);
                if (is_human(tgt_fc_id)) {
                    alt_cost_val /= 2;
                }
                tgt_wpn_strat = weap_strat(utgt->weapon_id, veh_fc_id);
                tgt_arm_strat = arm_strat(utgt->armor_id, veh_fc_id);
                tgt_chs_speed = Chassis[utgt->chassis_id].speed;
                tgt_region = region_at(Vehs[tgt_veh_id].x, Vehs[tgt_veh_id].y);
                if (plr->region_total_bases[tgt_region]) {
                    if (tgt->region_force_rating[tgt_region] > plr->region_total_combat_units[tgt_region]
                    + (plr->region_force_rating[tgt_region] / 2)) {
                        alt_cost_val /= 2;
                    }
                }
                if (plr->region_force_rating[tgt_region] > tgt->region_total_combat_units[tgt_region]) {
                    alt_cost_val *= 2;
                }
                if (tgt_wpn_strat > plr->best_weapon_value) {
                    alt_cost_val /= 2;
                }
                if (tgt_arm_strat > plr->best_armor_value) {
                    alt_cost_val /= 2;
                }
                if (tgt_chs_speed > plr->best_land_speed) {
                    alt_cost_val = 2 * alt_cost_val / 3;
                }
                if (plr->diplo_status[tgt_fc_id] & DIPLO_TREATY) {
                    alt_cost_val *= 2;
                }
                if (!(plr->diplo_status[tgt_fc_id] & DIPLO_WANT_REVENGE)
                && !(*GameState & STATE_UNK_200) && !at_climax(veh_fc_id)) {
                    if (plr->diplo_status[tgt_fc_id] & DIPLO_UNK_1000000
                    && plr->diplo_status[tgt_fc_id] & (DIPLO_TRUCE|DIPLO_TREATY)
                    && whose_territory(veh_fc_id, Vehs[tgt_veh_id].x, Vehs[tgt_veh_id].y, 0, 0) != veh_fc_id) {
                        return 0;
                    }
                }
                if (tgt_wpn_strat < plr->best_weapon_value && tgt_arm_strat < plr->best_armor_value) {
                    alt_cost_val *= 2;
                }
                if (tgt_wpn_strat > plr->best_weapon_value) {
                    alt_cost_val /= 2;
                }
                MAP* sq = mapsq(Vehs[tgt_veh_id].x, Vehs[tgt_veh_id].y);
                if (sq->rocky_level() >= 2 || sq->is_fungus() || sq->items & BIT_BUNKER) {
                    alt_cost_val /= 2;
                }
                if (Vehs[tgt_veh_id].plan() <= PLAN_COMBAT) {
                    alt_cost_val /= 2;
                }
                if (Vehs[tgt_veh_id].plan() >= PLAN_DEFENSE) {
                    alt_cost_val *= 3;
                }
                if (plr->energy_credits < alt_cost_val) {
                    return 0;
                }
                if (plr->energy_credits < veh_mc_cost) {
                    return 0;
                }
            }
            if (!toggle) {
                return 1;
            }
            if (is_human(veh_fc_id)) {
                int choice;
                if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                    parse_num(0, veh_mc_cost);
                    parse_num(1, plr->energy_credits);
                    mod_success_rates(1, mod_morale_veh(veh_id, 1, 0), 0, -1);
                    mod_success_rates(2, mod_morale_veh(veh_id, 1, 0), 1, -1);
                    Popup_start(&cur_popup, PopupScriptFile, "SUBVERT", -1, 0, 0, 0);
                    if (plr->energy_credits >= veh_mc_cost) {
                        Popup_start(&cur_popup, PopupScriptFile, "SUBVERTMENU", -1, 0, 128, 0);
                    }
                    choice = BasePop_exec_3(&cur_popup, 0, 0);
                    if (!choice) {
                        return 0;
                    }
                    Vehs[veh_id].probe_action |= 8 * (choice + 31);
                } else {
                    choice = ((Vehs[veh_id].probe_action & 8) != 0) + 1;
                }
                prb_diff = choice - 1;
            }
            if (!(*VehAttackFlags & 2)) {
                goto MOV_FRAME;
            }
            plr->energy_credits -= veh_mc_cost;
            if (veh_fc_id == MapWin->cOwner) {
                Console_update_data(MapWin, 0);
            }
            goto MOV_FRAME;
        }
        if (action_id != PRB_MIND_CONTROL_CITY) {
            goto MOV_FRAME;
        }
        mc_cost = mod_mind_control(tgt_base_id, veh_fc_id, 0);
        if (mc_cost >= 0) {
            break;
        }
        if (is_human(veh_fc_id)) {
            return 0;
        }
        action_id = PRB_PROCURE_RESEARCH_DATA;
    }
    if (MFactions[tgt_fc_id].rule_flags & RFLAG_MINDCONTROL) {
        if (is_human(veh_fc_id)) {
            if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                NetMsg_pop(NetMsg, "MINDIMMUNITY", -5000, 0, 0);
            }
            return 0;
        }
        goto MOV_CHECK;
    }
    for (int i = 0; i < MFactions[veh_fc_id].faction_bonus_count; ++i) {
        if (MFactions[veh_fc_id].faction_bonus_id[i] == RULE_PROBECOST) {
            mc_cost = (MFactions[veh_fc_id].faction_bonus_val1[i] * mc_cost) / 100;
        }
    }
    if (has_project(FAC_NETHACK_TERMINUS, veh_fc_id)) {
        mc_cost = 3 * mc_cost / 4;
    }
    int probe_val;
    probe_val = tgt->SE_probe;
    if (has_fac_built(FAC_GENEJACK_FACTORY, tgt_base_id)) {
        probe_val -= 1;
    }
    if (has_fac_built(FAC_COVERT_OPS_CENTER, tgt_base_id)) {
        probe_val += 2;
    }
    // Fix 1 & 15. [BUG] Faction PROBE value is greater than 3 and probes with Algorithmic Enhancement.
    if (probe_val <= -2) {
        mc_cost /= 2;
    } else if (probe_val == -1) {
        mc_cost += (mc_cost / -4);
    } else if (probe_val == 0) {
        // nothing
    } else if (probe_val == 1) {
        mc_cost += (mc_cost / 2);
    } else if (probe_val == 2 || (*ProbeHasAlgoEnhancement == 1)) {
        mc_cost *= 2;
    } else {
        if (is_human(veh_fc_id)) {
            if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                parse_says(0, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                NetMsg_pop(NetMsg, "IMMUNITY", -5000, 0, 0);
            }
            return 0;
        }
        action_id = PRB_PROCURE_RESEARCH_DATA;
        goto MOV_START;
    }
    if (is_human(veh_fc_id)
    || (!is_human(tgt_fc_id) && plr->energy_credits >= 2 * mc_cost * (plr->AI_power - plr->AI_wealth + 3) / 3)
    || (is_human(tgt_fc_id) && plr->energy_credits >= mc_cost + 2 + clamp(plr->energy_credits / 10, 0, 50))) {
        if (is_human(veh_fc_id)) {
            int choice;
            if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
                parse_says(0, Bases[tgt_base_id].name, -1, -1);
                parse_num(0, mc_cost);
                parse_num(1, plr->energy_credits);
                mod_success_rates(1, mod_morale_veh(veh_id, 1, 0), 0, -1);
                mod_success_rates(2, mod_morale_veh(veh_id, 1, 0), 1, -1);
                if (plr->energy_credits < mc_cost) {
                    wave_it(9);
                    NetMsg_pop(NetMsg, "THOUGHTCONTROL", -5000, 0, 0);
                    return 0;
                }
                wave_it(32);
                Popup_start(&cur_popup, PopupScriptFile, "THOUGHTCONTROL", -1, 0, 0, 0);
                Popup_start(&cur_popup, PopupScriptFile, "THOUGHTMENU", -1, 0, 128, 0);
                choice = BasePop_exec_3(&cur_popup, 0, 0);
                if (choice <= 0) {
                    return 0;
                }
                Vehs[veh_id].probe_action |= 8 * (choice + 31);
            } else {
                choice = ((Vehs[veh_id].probe_action & 8) != 0) + 1;
            }
            if (choice > 1) {
                prb_diff = 1;
            }
        }
        if (!(*VehAttackFlags & 2)) {
            goto MOV_FRAME;
        }
        plr->energy_credits -= mc_cost;
        plr->mind_control_total += 4;
        tgt->diplo_mind_control[veh_fc_id] += 4;
        if (veh_fc_id == MapWin->cOwner) {
            Console_update_data(MapWin, 0);
        }
        goto MOV_FRAME;
    }
MOV_CHECK:
    bool check;
    check = plr->diplo_status[tgt_fc_id] & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA);
    action_id = check ? PRB_ACTIVATE_SABOTAGE_VIRUS : PRB_INFILTRATE_DATALINKS;
    tgt_region = region_at(Bases[tgt_base_id].x, Bases[tgt_base_id].y);
    if (check) {
        if (plr->region_base_plan[tgt_region]
        && Bases[tgt_base_id].talent_total == Bases[tgt_base_id].drone_total) {
            if (!(game_rand() & 1) && Bases[tgt_base_id].pop_size > 3) {
                action_id = PRB_INCITE_DRONE_RIOTS;
            }
        }
    }
    if (gene_warfare_allow) {
        if (Bases[tgt_base_id].pop_size >= 6) {
            if ((plr->diplo_status[tgt_fc_id] & DIPLO_ATROCITY_VICTIM
            || (!un_charter() && plr->diplo_status[tgt_fc_id] & DIPLO_WANT_REVENGE))
            && plr->region_base_plan[tgt_region] == PLAN_DEFENSE) {
                action_id = PRB_INTRODUCE_GENETIC_PLAGUE;
            }
        }
        if (Bases[tgt_base_id].pop_size >= 4) {
            if (plr->player_flags & PFLAG_COMMIT_ATROCITIES_WANTONLY) {
                if (!(plr->diplo_status[tgt_fc_id] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))
                || plr->diplo_status[tgt_fc_id] & DIPLO_WANT_REVENGE) {
                    action_id = PRB_INTRODUCE_GENETIC_PLAGUE;
                }
            }
        }
    }
    bool activate;
    activate = 0;
    if (plr->diplo_status[tgt_fc_id] & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA)) {
        for (int i = 0; i < 21; i++) {
            int px = wrap(Bases[tgt_base_id].x + TableOffsetX[i]);
            int py = Bases[tgt_base_id].y + TableOffsetY[i];
            int cur_id;
            if (on_map(px, py) && !is_ocean(mapsq(px, py))
            && (cur_id = stack_fix(veh_at(px, py))) >= 0
            && Vehs[cur_id].faction_id == veh_fc_id
            && mod_stack_check(cur_id, 4, veh_fc_id, -1, -1)) {
                activate = 1;
                action_id = PRB_ACTIVATE_SABOTAGE_VIRUS;
                break;
            }
        }
    }
    if (Rules->tgl_probe_steal_tech) {
        if (!(Bases[tgt_base_id].state_flags & BSTATE_RESEARCH_DATA_STOLEN)
        || mod_morale_veh(veh_id, 1, 0) >= 4
        || (action_id == PRB_INFILTRATE_DATALINKS
        && (plr->diplo_status[tgt_fc_id] & DIPLO_HAVE_INFILTRATOR))) {
            if (!activate && tgt->region_base_plan[tgt_region] != PLAN_DEFENSE) {
                action_id = PRB_PROCURE_RESEARCH_DATA;
            }
        }
    }
    if (gene_warfare_allow) {
        if (Bases[tgt_base_id].pop_size >= 4 && aah_ooga(veh_fc_id, veh_fc_id) == tgt_fc_id && climactic_battle()) {
            if (plr->AI_fight > 0 || (!plr->AI_fight && plr->diplo_status[tgt_fc_id] & DIPLO_WANT_REVENGE)) {
                if (plr->diplo_status[tgt_fc_id] & (DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE|DIPLO_VENDETTA)) {
                    action_id = PRB_INTRODUCE_GENETIC_PLAGUE;
                }
            }
        }
    }
    if (prb_action_check) {
        goto MOV_START;
    }
    if (has_fac_built(FAC_HEADQUARTERS, tgt_base_id)) {
        if (action_id != PRB_PROCURE_RESEARCH_DATA
        && tgt->tech_accumulated >= tgt->tech_cost / 2
        && (plr->diplo_status[tgt_fc_id] & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA)
        || tgt->tech_ranking / 2 > plr->tech_ranking / 2 + 4)) {
            action_id = PRB_ASSASSINATE_PROMINENT_RESEARCHERS;
        }
        if (*ExpansionEnabled && !*MultiplayerActive) {
            bool found = 0;
            // Fix: corrected code that checked wrong diplo status for captured leaders
            for (auto cur_id : captured_leaders(tgt_fc_id)) {
                if (!(plr->diplo_status[cur_id] & DIPLO_VENDETTA)) {
                    found = 1;
                    if (prb_free_leader < 0
                    || (plr->diplo_status[cur_id] & DIPLO_PACT)
                    || ((plr->diplo_status[cur_id] & DIPLO_TREATY)
                    && !(plr->diplo_status[prb_free_leader] & DIPLO_PACT))) {
                        prb_free_leader = cur_id;
                    }
                }
            }
            if (found) {
                action_id = PRB_FREE_CAPTURED_FACTION_LEADER;
                prb_action_check = 1;
                goto MOV_START;
            }
        }
    } else if (plr->diplo_status[tgt_fc_id] & (DIPLO_WANT_REVENGE|DIPLO_VENDETTA)
    && (tgt->diff_level > 2 || !(is_human(tgt_fc_id)))
    && tgt->SE_probe < 3) {
        action_id = PRB_MIND_CONTROL_CITY;
    }
    prb_action_check = 1;
    goto MOV_START;

MOV_SABOTAGE:
    if (!(is_human(veh_fc_id))) {
        if (mod_morale_veh(veh_id, 1, 0) >= 5) {
            if (mod_stack_check(veh_at(Bases[tgt_base_id].x, Bases[tgt_base_id].y), 2, 5, -1, -1)) {
                sabotage_id = 98;
                prb_diff = 1;
            } else {
                for (auto item_id : {FAC_TACHYON_FIELD, FAC_PERIMETER_DEFENSE, FAC_CHILDREN_CRECHE, FAC_COMMAND_CENTER}) {
                    if (has_fac_built(item_id, tgt_base_id)) {
                        sabotage_id = item_id;
                        prb_diff = 1;
                        break;
                    }
                }
            }
        }
        goto MOV_FRAME;
    }
    int act_val;
    mod_success_rates(1, mod_morale_veh(veh_id, 1, 0), 0, tgt_base_id);
    if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id) && *VehAttackFlags & 1) {
        if (mod_success_rates(2, mod_morale_veh(veh_id, 1, 0), 1, tgt_base_id)) {
            act_val = popp(ScriptFile, "ADVVIRUS", 0, infil_img, 0);
        } else {
            act_val = 0;
        }
        Vehs[veh_id].probe_action |= 8 * (uint8_t)act_val;
    } else {
        act_val = (Vehs[veh_id].probe_action >> 3) & 1;
    }
    if (!act_val) {
        goto MOV_FRAME;
    }
    prb_diff = 1;
    if (veh_fc_id != MapWin->cOwner || !is_human(veh_fc_id) || !(*VehAttackFlags & 1)) {
        sabotage_id = Vehs[veh_id].probe_sabotage_id;
    } else {
        Popup_start(&cur_popup, PopupScriptFile, "VIRUS", -1, 0, 0, 0);
        int cur_id = veh_at(Bases[tgt_base_id].x, Bases[tgt_base_id].y);
        if (mod_stack_check(cur_id, 2, 5, -1, -1)) {
            Dialogs_item(&cur_popup.dialogs, label_get(1028), 98); // Planet Buster Silos
        }
        for (int i = 1; i < 70; i++) {
            if (i != FAC_HEADQUARTERS && i <= Fac_ID_Last
            && (i != FAC_PRESSURE_DOME || !is_ocean(&Bases[tgt_base_id]))) {
                if (has_fac_built((FacilityId)i, tgt_base_id)) {
                    Dialogs_item(&cur_popup.dialogs, Facility[i].name, i);
                }
            }
        }
        snprintf(StrBuffer, StrBufLen, "%s (%s)", name_buf, label_get(249)); // Production
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 0);
        parse_it(ScriptFile, "ABORTSTRING");
        Dialogs_item(&cur_popup.dialogs, StrBuffer, 99);
        sabotage_id = BasePop_exec_3(&cur_popup, 0, 0);
        if (sabotage_id < 0) {
            return 0;
        }
        Vehs[veh_id].probe_sabotage_id = sabotage_id;
    }
    if (sabotage_id != 99) {
        if (!has_fac_built(FAC_HEADQUARTERS, tgt_base_id)) {
            if (sabotage_id == FAC_PERIMETER_DEFENSE || sabotage_id == FAC_TACHYON_FIELD) {
                if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id) && *VehAttackFlags & 1) {
                    parse_says(0, Bases[tgt_base_id].name, -1, -1);
                    parse_says(1, Facility[sabotage_id].name, -1, -1);
                    mod_success_rates(2, mod_morale_veh(veh_id, 1, 0), 2, tgt_base_id);
                    if (!popp(ScriptFile, "MILVIRUS", 0, infil_img, 0)) {
                        return 0;
                    }
                }
                prb_diff = 2;
            }
        } else {
            if (veh_fc_id == MapWin->cOwner && is_human(veh_fc_id) && *VehAttackFlags & 1) {
                parse_says(0, Bases[tgt_base_id].name, -1, -1);
                parse_says(1, MFactions[tgt_fc_id].adj_name_faction, -1, -1);
                mod_success_rates(2, mod_morale_veh(veh_id, 1, 0), 2, tgt_base_id);
                if (!popp(ScriptFile, "HQVIRUS", 0, infil_img, 0)) {
                    return 0;
                }
            }
            prb_diff = 2;
        }
    }
MOV_FRAME:
    int prb_state = 0;
    if ((action_id >= PRB_MIND_CONTROL_CITY || action_id <= PRB_INFILTRATE_DATALINKS)
    && action_id != PRB_FREE_CAPTURED_FACTION_LEADER) {
        // skipped
    } else if (is_human(veh_fc_id)) {
        if (is_human(tgt_fc_id)) {
            goto MOV_DEFEND;
        }
        if (veh_fc_id == MapWin->cOwner && *VehAttackFlags & 1) {
            Popup_start(&cur_popup, PopupScriptFile, "FRAME", -1, 0, 0, 0);
            mod_success_rates(2, mod_morale_veh(veh_id, 1, 0), prb_diff, tgt_base_id);
            if (mod_success_rates(3, mod_morale_veh(veh_id, 1, 0), prb_diff + 1, tgt_base_id)
            && !X_text_open(ScriptFile, "FRAMEMENU")) {
                text_get();
                parse_string(text_buf_ptr(), StrBuffer);
                Dialogs_item(&cur_popup.dialogs, StrBuffer, 0);
                text_get();
                prb_state = 0;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (i != veh_fc_id && i != tgt_fc_id
                    && plr->diplo_status[i] & DIPLO_COMMLINK
                    && is_alive(i)) {
                        ++prb_state;
                        parse_says(0, get_title(i), -1, -1);
                        parse_says(1, get_name(i), -1, -1);
                        clear();
                        parse_string(text_buf_ptr(), StrBuffer);
                        BasePop_item(&cur_popup, StrBuffer, i);
                    }
                }
                if (prb_state) {
                    prb_state = BasePop_exec_2(&cur_popup);
                } else {
                    prb_state = 0;
                }
            }
            Vehs[veh_id].probe_action |= 32 * (uint8_t)prb_state;
        } else {
            prb_state = Vehs[veh_id].probe_action >> 5;
        }
        if (prb_state) {
            ++prb_diff;
        }
    } else if (mod_morale_veh(veh_id, 1, 0) >= 5 && !prb_diff && !is_human(tgt_fc_id)) {
        prb_state = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != veh_fc_id && i != tgt_fc_id) {
                if (is_human(i) && is_alive(i) && i == *RankingFactionIDUnk1) {
                    if (!(tgt->diplo_status[i] & DIPLO_VENDETTA) && tgt->diplo_status[i] & DIPLO_COMMLINK) {
                        prb_state = i;
                        ++prb_diff;
                        break;
                    }
                }
            }
        }
    }
MOV_DEFEND:
    if (!(*VehAttackFlags & 2)) {
        return 1;
    }
    prb_action_val = action_id + 1;
    bool is_base_tgt = true, is_busted = false;
    int prb_val = clamp(tgt->SE_probe + (action_id == PRB_MIND_CONTROL_UNIT ? 0 :
        (has_fac(FAC_COVERT_OPS_CENTER, tgt_base_id, 0) ? 2 : 0)), -2, 0);
    int rnd_val = game_randv(max(1, mod_morale_veh(veh_id, 1, 0)) / 2 - prb_val + 1);
    if (rnd_val >= prb_diff) {
        if (*ProbeTargetHasHSA && random(2)) {
            is_busted = true;
        }
    } else if (!*ProbeHasAlgoEnhancement || *ProbeTargetHasHSA || random(2)) {
        is_busted = true;
    }
    if (is_busted) {
        prb_state = -prb_state;
        if (veh_fc_id == MapWin->cOwner) {
            if (!plr_alien) {
                NetMsg_pop_2("BUSTED", "capture_sm.pcx");
            } else {
                NetMsg_pop_2("BUSTED", "al_cap_sm.pcx");
            }
        }
        if (tgt_fc_id == MapWin->cOwner) {
            prb_alt_action = action_id;
        }
        kill(veh_id);
        goto MOV_UPKEEP;
    }
    switch (action_id) {
    case PRB_PROCURE_RESEARCH_DATA:
        tgt->diplo_stolen_techs[veh_fc_id]++;
        if (prb_tech_id >= 0) {
            if (veh_fc_id == MapWin->cOwner || tgt_fc_id == MapWin->cOwner) {
                parse_says(0, get_adjective(tgt_fc_id), -1, -1);
                parse_says(2, get_adjective(veh_fc_id), -1, -1);
                if (prb_tech_id == 9999) {
                    snprintf(StrBuffer, StrBufLen, "STOLEMAP%d", veh_fc_id != MapWin->cOwner);
                } else {
                    clear();
                    say_tech(StrBuffer, prb_tech_id, 0);
                    parse_says(1, StrBuffer, -1, -1);
                    snprintf(StrBuffer, StrBufLen, "DECIPHERED%d", veh_fc_id != MapWin->cOwner);
                }
                NetMsg_pop_2(StrBuffer, infil_img);
            }
            tech_achieved(veh_fc_id, prb_tech_id, tgt_fc_id, 0);
        } else {
            steal_tech(veh_fc_id, tgt_fc_id, 1);
        }
        Bases[tgt_base_id].state_flags |= BSTATE_RESEARCH_DATA_STOLEN;
        break;
    case PRB_ACTIVATE_SABOTAGE_VIRUS:
        if (sabotage_id == 99) {
            NetMsg_pop_2("VIRUSABORT", 0);
            return 1;
        }
        if (sabotage_id == 0) {
            Bases[tgt_base_id].minerals_accumulated_2 = 0;
            Bases[tgt_base_id].minerals_accumulated = 0;
            parse_says(0, name_buf, -1, -1);
            parse_says(1, Bases[tgt_base_id].name, -1, -1);
            parse_says(2, get_adjective(veh_fc_id), -1, -1);
            snprintf(StrBuffer, StrBufLen, "PRODVIRUS%d", veh_fc_id != MapWin->cOwner);
        } else if (sabotage_id != 98) {
            set_fac((FacilityId)sabotage_id, tgt_base_id, 0);
            parse_says(0, Facility[sabotage_id].name, -1, -1);
            parse_says(1, Bases[tgt_base_id].name, -1, -1);
            parse_says(2, get_adjective(veh_fc_id), -1, -1);
            snprintf(StrBuffer, StrBufLen, "FACVIRUS%d", veh_fc_id == MapWin->cOwner);
        }
        if (sabotage_id == 0 || sabotage_id != 98) {
            if (veh_fc_id == MapWin->cOwner || tgt_fc_id == MapWin->cOwner) {
                NetMsg_pop_2(StrBuffer, infil_img);
            }
            BaseWin_check_base(BaseWin, tgt_base_id);
            draw_tile(Bases[tgt_base_id].x, Bases[tgt_base_id].y, 2);
            break;
        }
        int cur_id;
        cur_id = veh_at(Bases[tgt_base_id].x, Bases[tgt_base_id].y);
        if (cur_id >= 0) {
            while (Units[Vehs[cur_id].unit_id].plan != PLAN_PLANET_BUSTER) {
                cur_id = Vehs[cur_id].next_veh_id_stack;
                if (cur_id < 0) {
                    BaseWin_check_base(BaseWin, tgt_base_id);
                    draw_tile(Bases[tgt_base_id].x, Bases[tgt_base_id].y, 2);
                    break;
                }
            }
            if (cur_id < 0) {
                break;
            }
            if (Vehs[cur_id].faction_id == MapWin->cOwner) {
                parse_says(0, Units[Vehs[cur_id].unit_id].name, -1, -1);
                parse_says(1, get_adjective(veh_fc_id), -1, -1);
                NetMsg_pop_2("BUSTMYPLANET", infil_img);
            } else if (veh_fc_id == MapWin->cOwner) {
                parse_says(0, Units[Vehs[cur_id].unit_id].name, -1, -1);
                parse_says(2, get_adjective(Vehs[cur_id].faction_id), -1, -1);
                NetMsg_pop_2("BUSTYOURPLANET", infil_img);
            }
            kill(cur_id);
            if (veh_id > cur_id) {
                --veh_id;
            }
            BaseWin_check_base(BaseWin, tgt_base_id);
            draw_tile(Bases[tgt_base_id].x, Bases[tgt_base_id].y, 2);
            break;
        }
        return 1;
    case PRB_DRAIN_ENERGY_RESERVES:
        int steal_val, mor_val;
        steal_val = steal_energy(tgt_base_id);
        mor_val = steal_val * mod_morale_veh(veh_id, 1, 0) / 6;
        steal_val = max(0, min(mor_val + game_randv(steal_val - mor_val) / 2, tgt->energy_credits));
        if (steal_val == 1) {
            steal_val = 0;
        }
        tgt->energy_credits -= steal_val;
        plr->energy_credits += steal_val;
        if (veh_fc_id == MapWin->cOwner) {
            Console_update_data(MapWin, 0);
            parse_num(0, steal_val);
            parse_says(0, Bases[tgt_base_id].name, -1, -1);
            parse_says(2, get_adjective(tgt_fc_id), -1, -1);
            NetMsg_pop_2(steal_val ? "GOTENERGY" : "GOTNOENERGY", infil_img);
        } else if (tgt_fc_id == MapWin->cOwner) {
            Console_update_data(MapWin, 0);
            parse_num(0, steal_val);
            parse_says(0, Bases[tgt_base_id].name, -1, -1);
            parse_says(2, get_adjective(veh_fc_id), -1, -1);
            NetMsg_pop_2(steal_val ? "TOOKENERGY" : "TOOKNOENERGY", infil_img);
        }
        break;
    case PRB_INCITE_DRONE_RIOTS:
        int pop_val, turn_val;
        pop_val = 10 * Bases[tgt_base_id].pop_size / 4;
        turn_val = Bases[tgt_base_id].assimilation_turns_left;
        if (turn_val > pop_val) {
            pop_val = Bases[tgt_base_id].assimilation_turns_left;
        }
        Bases[tgt_base_id].assimilation_turns_left = max(0, min(turn_val + 10, pop_val));
        set_base(tgt_base_id);
        base_compute(1);
        parse_says(0, Bases[tgt_base_id].name, -1, -1);
        if (CurrentBase[0]->drone_total <= CurrentBase[0]->talent_total) {
            if (veh_fc_id == MapWin->cOwner || tgt_fc_id == MapWin->cOwner) {
                NetMsg_pop_2("DRONEINCITE1", plr_alien ? "ALdrone_sm.pcx" : "drone_sm.pcx");
            }
        } else {
            CurrentBase[0]->state_flags |= BSTATE_DRONE_RIOTS_ACTIVE;
            draw_tile(Bases[tgt_base_id].x, Bases[tgt_base_id].y, 2);
            if (veh_fc_id == MapWin->cOwner || tgt_fc_id == MapWin->cOwner) {
                NetMsg_pop_2("DRONEINCITE0", plr_alien ? "ALdrone_sm.pcx" : "drone_sm.pcx");
            }
        }
        break;
    case PRB_ASSASSINATE_PROMINENT_RESEARCHERS:
        int res_rnd_val;
        res_rnd_val = game_randv(tgt->tech_accumulated);
        tgt->tech_accumulated -= res_rnd_val;
        if (veh_fc_id == MapWin->cOwner || tgt_fc_id == MapWin->cOwner) {
            parse_num(0, res_rnd_val);
            parse_says(0, Bases[tgt_base_id].name, -1, -1);
            parse_says(1, get_adjective(tgt_fc_id), -1, -1);
            NetMsg_pop_2("ASSASSINATED", infil_img);
        }
        break;
    case PRB_FREE_CAPTURED_FACTION_LEADER:
        int capture_id;
        if (!(is_human(veh_fc_id)) || veh_fc_id != MapWin->cOwner) {
            auto choices = captured_leaders(tgt_fc_id);
            // Fix 27. [BUG] Fixed bug when AI successfully completes probe action of freeing
            // a captured faction leader (FREEWILLY/FREEWHO) where it would reset a non-captured faction.
            if (choices.size()) {
                capture_id = choices[game_randv(choices.size())];
            } else {
                capture_id = -1;
            }
        } else {
            auto choices = captured_leaders(tgt_fc_id);
            parse_says(1, get_adjective(tgt_fc_id), -1, -1);
            Popup_start_3(&cur_popup, "FREEWHO", 64);
            popup_init_image(&cur_popup, infil_img, &bx, &by);
            for (auto plr_id : choices) {
                snprintf(StrBuffer, StrBufLen,
                    "%s %s %s %s", get_title(plr_id), get_name(plr_id),
                    label_get(180), get_noun(plr_id)); // of the
                BasePop_item(&cur_popup, StrBuffer, plr_id);
            }
            capture_id = BasePop_exec_2(&cur_popup);
            if (capture_id < 0) {
                return 0;
            }
        }
        if (capture_id >= 0) {
            if (Vehs[veh_id].home_base_id < 0) {
                veh_kill(veh_id);
            }
            set_alive(capture_id, true);
            mod_setup_player(capture_id, tgt_fc_id, 1);
            tgt->eliminated_count--;
            reset_captured_leader(tgt_fc_id, capture_id);
            if (veh_fc_id == MapWin->cOwner || tgt_fc_id == MapWin->cOwner) {
                parse_says(0, get_title(capture_id), -1, -1);
                parse_says(1, get_name(capture_id), -1, -1);
                parse_says(2, get_noun(capture_id), -1, -1);
                NetMsg_pop_2("WILLYFREE", "fac_escape_sm.pcx");
            }
            if (!(*GameRules & RULES_SCN_NO_TECH_TRADING)) {
                for (prb_tech_id = 0; prb_tech_id < MaxTechnologyNum; prb_tech_id++) {
                    if (has_tech(prb_tech_id, capture_id) && !has_tech(prb_tech_id, veh_fc_id)) {
                        net_tech(veh_fc_id, prb_tech_id, capture_id, 0);
                    }
                }
            }
            *dword_7AD330 = 0;
            Console_update_data(MapWin, 0);
            plr->diplo_friction[capture_id] = 0;
            net_treaty_off(capture_id, veh_fc_id, DIPLO_WANT_REVENGE, 1);
            net_treaty_on(veh_fc_id, capture_id, DIPLO_UNK_8000000|DIPLO_COMMLINK|DIPLO_TREATY, 0);
            net_set_treaty(capture_id, veh_fc_id, DIPLO_HAVE_SURRENDERED, 1, 0);
            net_treaty_on(capture_id, veh_fc_id, DIPLO_UNK_1000000|DIPLO_PACT, 0);
            Factions[capture_id].diplo_patience[veh_fc_id] = 0;
            plr->diplo_spoke[capture_id] = 0;
            diplomacy_check(veh_fc_id, capture_id, 0);
            plr->diplo_reputation++;
            Factions[capture_id].diplo_reputation++;
        }
        break;
    case PRB_INTRODUCE_GENETIC_PLAGUE:
        int pop_dmg;
        atrocity(veh_fc_id, tgt_fc_id, 1, 0);
        if (gene_warfare_techs < 1) {
            gene_warfare_techs = 1; // Fix divisor value always as positive
        }
        if (tgt->player_flags & PFLAG_GENETIC_PLAGUE_INTRO) {
            ++gene_warfare_known;
        }
        tgt->player_flags |= PFLAG_GENETIC_PLAGUE_INTRO;
        pop_dmg = Bases[tgt_base_id].pop_size * (gene_warfare_techs
            - clamp(gene_warfare_known, 1, gene_warfare_techs)) / gene_warfare_techs;
        if (Bases[tgt_base_id].state_flags & BSTATE_GENETIC_PLAGUE_INTRO) {
            pop_dmg /= 2;
        }
        if (has_fac(FAC_RESEARCH_HOSPITAL, tgt_base_id, 0)) {
            pop_dmg /= 2;
        }
        if (has_fac(FAC_NANOHOSPITAL, tgt_base_id, 0)) {
            pop_dmg /= 2;
        }
        pop_dmg += 1;
        if (pop_dmg >= Bases[tgt_base_id].pop_size) {
            pop_dmg = Bases[tgt_base_id].pop_size - 1;
        }
        for (int iter_id = veh_at(Bases[tgt_base_id].x, Bases[tgt_base_id].y); iter_id >= 0;) {
            int health = Vehs[iter_id].cur_hitpoints();
            int veh_dmg = (conf.probe_action_fix ? (health + 3) / 4 : health / 2)
                + game_randv((health + 1) / 2);
            if (has_fac(FAC_RESEARCH_HOSPITAL, tgt_base_id, 0)) {
                veh_dmg /= 2;
            }
            if (has_fac(FAC_NANOHOSPITAL, tgt_base_id, 0)) {
                veh_dmg /= 2;
            }
            if (Bases[tgt_base_id].state_flags & BSTATE_GENETIC_PLAGUE_INTRO) {
                veh_dmg /= 2;
            }
            Vehs[iter_id].damage_taken += veh_dmg;
            iter_id = Vehs[iter_id].next_veh_id_stack;
        }
        Bases[tgt_base_id].state_flags |= BSTATE_GENETIC_PLAGUE_INTRO;
        if (pop_dmg) {
            Bases[tgt_base_id].pop_size -= pop_dmg;
        }
        Bases[tgt_base_id].nutrients_accumulated = 0;
        spot_base(tgt_base_id, veh_fc_id);
        draw_tile(Bases[tgt_base_id].x, Bases[tgt_base_id].y, 2);
        if (veh_fc_id == MapWin->cOwner || tgt_fc_id == MapWin->cOwner
        || (*GameState & STATE_OMNISCIENT_VIEW) != 0) {
            parse_says(0, Bases[tgt_base_id].name, -1, -1);
            parse_says(1, get_noun(veh_fc_id), -1, -1);
            snprintf(StrBuffer, StrBufLen, "GENETICWARFARE%d", veh_fc_id != MapWin->cOwner);
            popp_2(StrBuffer, infil_img, 0);
        }
        break;
    case PRB_MIND_CONTROL_UNIT:
        int unit_id;
        unit_id = Vehs[tgt_veh_id].unit_id;
        --tgt->units_active[unit_id];
        if (unit_id > MaxProtoFactionNum) {
            int uid = mod_propose_proto(
                   veh_fc_id,
                   (VehChassis)Units[unit_id].chassis_id,
                   (VehWeapon)Units[unit_id].weapon_id,
                   (VehArmor)Units[unit_id].armor_id,
                   (VehAblFlag)Units[unit_id].ability_flags,
                   (VehReactor)Units[unit_id].reactor_id,
                   PLAN_PROTOTYPE_DONE,
                   Units[unit_id].name);
            if (uid >= 0) {
                Vehs[tgt_veh_id].unit_id = uid;
            }
        }
        Vehs[tgt_veh_id].faction_id = veh_fc_id;
        plr->units_active[Vehs[tgt_veh_id].unit_id]++;
        plr->mind_control_total++;
        tgt->diplo_mind_control[veh_fc_id]++;
        owner_set(Vehs[tgt_veh_id].x, Vehs[tgt_veh_id].y, veh_fc_id);
        Vehs[tgt_veh_id].home_base_id = base_find_2(
            Vehs[tgt_veh_id].x, Vehs[tgt_veh_id].y, veh_fc_id);
        Vehs[tgt_veh_id].moves_spent = 0;
        Vehs[tgt_veh_id].order = ORDER_NONE;
        Vehs[tgt_veh_id].state &= ~(VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
        spot_stack(tgt_veh_id, tgt_fc_id);
        draw_tile(Vehs[tgt_veh_id].x, Vehs[tgt_veh_id].y, 2);
        if (tgt_fc_id == MapWin->cOwner) {
            Console_focus(MapWin, Vehs[tgt_veh_id].x, Vehs[tgt_veh_id].y, veh_fc_id);
            parse_says(1, get_noun(veh_fc_id), -1, -1);
            NetMsg_pop_2("SUBVERTED", infil_img);
        }
        if (prb_diff) {
            return 1;
        }
        is_base_tgt = false;
        break;
    default:
        break;
    }
    if (is_base_tgt) {
        int prb_num, rnd_num, ret_base_id;
        prb_num = (prb_action_val && has_fac(FAC_COVERT_OPS_CENTER, tgt_base_id, 0) ? 2 : 0);
        rnd_num = game_randv(mod_morale_veh(veh_id, 1, 0) - clamp(tgt->SE_probe + prb_num, -2, 0));
        if (rnd_num <= prb_diff) {
            if (*ProbeTargetHasHSA || !*ProbeHasAlgoEnhancement || !random(2)) {
                ret_base_id = -1;
                prb_state = -prb_state;
            } else {
                if (conf.probe_action_fix) {
                    ret_base_id = find_return_base(veh_id);
                } else {
                    ret_base_id = base_find_2(Vehs[veh_id].x, Vehs[veh_id].y, veh_fc_id);
                }
                if (Vehs[veh_id].triad() == TRIAD_SEA && ret_base_id >= 0 && !is_port(ret_base_id, 0)) {
                    ret_base_id = find_probe_base(veh_id, veh_fc_id);
                }
            }
        } else {
            if (conf.probe_action_fix) {
                ret_base_id = find_return_base(veh_id);
            } else {
                ret_base_id = base_find_2(Vehs[veh_id].x, Vehs[veh_id].y, veh_fc_id);
            }
            if (Vehs[veh_id].triad() == TRIAD_SEA && ret_base_id >= 0 && !is_port(ret_base_id, 0)) {
                ret_base_id = find_probe_base(veh_id, veh_fc_id);
                if (*ProbeTargetHasHSA && random(2)) {
                    ret_base_id = -1;
                    prb_state = -prb_state;
                }
            }
        }
        if (ret_base_id < 0 && action_id == PRB_FREE_CAPTURED_FACTION_LEADER
        && Units[Vehs[veh_id].unit_id].weapon_id != WPN_PROBE_TEAM) {
            // unreachable with default config but preserved for modding
            ret_base_id = Vehs[veh_id].home_base_id;
        }
        if (ret_base_id < 0) {
            veh_kill(veh_id);
        } else {
            int morale;
            veh_put(veh_id, Bases[ret_base_id].x, Bases[ret_base_id].y);
            Vehs[veh_id].movement_turns = 0;
            draw_tiles(Bases[tgt_base_id].x, Bases[tgt_base_id].y, 2);
            draw_tile(Bases[ret_base_id].x, Bases[ret_base_id].y, 2);
            parse_says(1, Bases[ret_base_id].name, -1, -1);
            morale = mod_morale_veh(veh_id, 1, 0);
            Vehs[veh_id].morale = clamp(Vehs[veh_id].morale + 1, 0, 6);
            veh_skip(veh_id);
            if (veh_fc_id == MapWin->cOwner) {
                parse_says(0, Units[Vehs[veh_id].unit_id].name, -1, -1);
                parse_says(2, Morale[mod_morale_veh(veh_id, 1, 0)].name, -1, -1);
                clear();
                says("PROBESURVIVAL");
                if (action_id == PRB_FREE_CAPTURED_FACTION_LEADER) {
                    says("X");
                } else {
                    say_num(morale != mod_morale_veh(veh_id, 1, 0) && mod_morale_veh(veh_id, 1, 0) >= 1);
                }
                if (veh_fc_id == MapWin->cOwner) {
                    NetMsg_pop_2(StrBuffer, infil_img);
                }
            }
        }
        if (action_id == PRB_MIND_CONTROL_CITY) {
            if (veh_fc_id == MapWin->cOwner
            || tgt_fc_id == MapWin->cOwner
            || spying(veh_fc_id)
            || spying(tgt_fc_id)
            || (*GameState & STATE_OMNISCIENT_VIEW) != 0) {
                if ((*GameState & STATE_OMNISCIENT_VIEW) != 0
                || (Bases[tgt_base_id].faction_id == MapWin->cOwner)
                || Bases[tgt_base_id].visibility & (1 << MapWin->cOwner)) {
                    Console_focus(MapWin, Bases[tgt_base_id].x, Bases[tgt_base_id].y, Bases[tgt_base_id].faction_id);
                }
                parse_says(0, get_title(veh_fc_id), -1, -1);
                parse_says(1, get_name(veh_fc_id), -1, -1);
                parse_says(2, get_noun(veh_fc_id), -1, -1);
                parse_says(3, Bases[tgt_base_id].name, -1, -1);
                parse_says(4, get_title(tgt_fc_id), -1, -1);
                parse_says(5, get_name(tgt_fc_id), -1, -1);
                snprintf(StrBuffer, StrBufLen, "MINDCONTROL%d",
                    (veh_fc_id == MapWin->cOwner ? 0 : (tgt_fc_id == MapWin->cOwner ? 1 : 2)));
                popp_2(StrBuffer, infil_img, 0);
            }
            for (int i = *VehCount - 1; i >= 0; --i) {
                if (Vehs[i].faction_id == tgt_fc_id) {
                    int dist = vector_dist(&Vehs[i], &Bases[tgt_base_id]);
                    if (dist <= (conf.probe_action_fix ? 0 : 1)
                    && (!dist || base_at(Vehs[i].x, Vehs[i].y) < 0)) {
                        --tgt->units_active[Vehs[i].unit_id];
                        if (Vehs[i].unit_id > MaxProtoFactionNum) {
                            int uid = mod_propose_proto(
                               veh_fc_id,
                               (VehChassis)Units[Vehs[i].unit_id].chassis_id,
                               (VehWeapon)Units[Vehs[i].unit_id].weapon_id,
                               (VehArmor)Units[Vehs[i].unit_id].armor_id,
                               (VehAblFlag)Units[Vehs[i].unit_id].ability_flags,
                               (VehReactor)Units[Vehs[i].unit_id].reactor_id,
                               PLAN_PROTOTYPE_DONE,
                               Units[Vehs[i].unit_id].name);
                            if (uid >= 0) {
                                Vehs[i].unit_id = uid;
                            }
                        }
                        Vehs[i].faction_id = veh_fc_id;
                        Vehs[i].home_base_id = tgt_base_id;
                        Vehs[i].moves_spent = 0;
                        ++plr->units_active[Vehs[i].unit_id];
                        if (Vehs[i].range()) {
                            Vehs[i].movement_turns = 0;
                        }
                        if (Vehs[i].order != ORDER_HOLD) {
                            Vehs[i].order = ORDER_NONE;
                        }
                        owner_set(Vehs[i].x, Vehs[i].y, veh_fc_id);
                        Vehs[i].visibility |= 1 << tgt_fc_id;
                    }
                }
            }
            if (!prb_diff) {
                act_of_aggression(veh_fc_id, tgt_fc_id);
            }
            mod_capture_base(tgt_base_id, veh_fc_id, 1);
        }
        if (veh_fc_id == MapWin->cOwner && ret_base_id < 0) {
            NetMsg_pop_2(action_id == PRB_MIND_CONTROL_CITY ? "PROBESUCCUMB" : "PROBECAUGHT",
                plr_alien ? "al_cap_sm.pcx" : "capture_sm.pcx");
        }
MOV_UPKEEP:
        if (action_id == PRB_INFILTRATE_DATALINKS) {
            return 1;
        }
    }
    if (!has_treaty(veh_fc_id, tgt_fc_id, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        return 1;
    }
    if (has_treaty(tgt_fc_id, veh_fc_id, DIPLO_PACT) && has_treaty(tgt_fc_id, veh_fc_id, DIPLO_HAVE_SURRENDERED)) {
        return 1;
    }
    if (action_id == PRB_MIND_CONTROL_CITY && prb_diff) {
        if (!conf.probe_action_fix) {
            set_treaty(tgt_fc_id, veh_fc_id, DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE, 1);
        } else if (!at_war(tgt_fc_id, veh_fc_id)) {
            // Replaces set_treaty call when Total Thought Control probe action is completed.
            // This does not result in automatic vendetta unlike normal Mind Control action.
            cause_friction(tgt_fc_id, veh_fc_id, 5);
            Factions[tgt_fc_id].diplo_betrayed[veh_fc_id]++;
            Factions[veh_fc_id].diplo_wrongs[tgt_fc_id]++;
            if (has_pact(tgt_fc_id, veh_fc_id)) {
                set_treaty(tgt_fc_id, veh_fc_id, DIPLO_WANT_TO_TALK, 1);
            }
        }
        return 1;
    }
    if (prb_state < 0) {
        if (!is_human(-prb_state)) {
            if (veh_fc_id == MapWin->cOwner) {
                parse_says(0, get_title(-prb_state), -1, -1);
                parse_says(1, get_name(-prb_state), -1, -1);
                if (!plr_alien) {
                    NetMsg_pop_2("FRAMEFLOP", "capture_sm.pcx");
                } else {
                    NetMsg_pop_2("FRAMEFLOP", "al_cap_sm.pcx");
                }
            }
            treaty_on(-prb_state, veh_fc_id, DIPLO_VENDETTA);
            prb_state = -prb_state;
        } else {
            parse_says(0, get_title(veh_fc_id), -1, -1);
            parse_says(1, get_name(veh_fc_id), -1, -1);
            parse_says(2, get_pact_hood(veh_fc_id, -prb_state), -1, -1);
            parse_says(3, get_noun(veh_fc_id), -1, -1);
            parse_says(4, get_his_her(veh_fc_id, 0), -1, -1);
            parse_says(5, get_him_her(veh_fc_id, 0), -1, -1);
            parse_says(6, get_title(tgt_fc_id), -1, -1);
            parse_says(7, get_name(tgt_fc_id), -1, -1);
            parse_says(8, get_noun(tgt_fc_id), -1, -1);
            int choice;
            if (*MultiplayerActive) {
                agenda_on(-prb_state, veh_fc_id, AGENDA_UNK_20);
                if (-prb_state == MapWin->cOwner) {
                    NetMsg_pop_2("HAVEFRAMEEXCUSE", 0);
                }
                if (veh_fc_id == MapWin->cOwner) {
                    parse_says(0, get_title(-prb_state), -1, -1);
                    parse_says(1, get_name(-prb_state), -1, -1);
                    parse_says(2, get_noun(-prb_state), -1, -1);
                    if (!plr_alien) {
                        NetMsg_pop_2("HASFRAMEEXCUSE", "capture_sm.pcx");
                    } else {
                        NetMsg_pop_2("HASFRAMEEXCUSE", "al_cap_sm.pcx");
                    }
                }
            } else if (has_treaty(-prb_state, veh_fc_id, DIPLO_PACT)) {
                if (!plr_alien) {
                    choice = popp_2("PACTFRAMEEXCUSE", "capture_sm.pcx", 0);
                } else {
                    choice = popp_2("PACTFRAMEEXCUSE", "al_cap_sm.pcx", 0);
                }
                if (choice) {
                    pact_ends(veh_fc_id, -prb_state);
                    Factions[-prb_state].diplo_spoke[veh_fc_id] = *CurrentTurn;
                }
            } else {
                if (!plr_alien) {
                    choice = popp_2("FRAMEEXCUSE", "capture_sm.pcx", 0);
                } else {
                    choice = popp_2("FRAMEEXCUSE", "al_cap_sm.pcx", 0);
                }
                if (choice) {
                    treaty_on(veh_fc_id, -prb_state, DIPLO_VENDETTA);
                    Factions[-prb_state].diplo_spoke[veh_fc_id] = *CurrentTurn;
                }
            }
        }
    }
    if (is_human(tgt_fc_id)) {
        parse_says(0, get_title(veh_fc_id), -1, -1);
        parse_says(1, get_name(veh_fc_id), -1, -1);
        parse_says(2, get_pact_hood(veh_fc_id, tgt_fc_id), -1, -1);
        parse_says(3, get_noun(veh_fc_id), -1, -1);
        parse_says(4, get_his_her(veh_fc_id, 0), -1, -1);
        parse_says(5, get_him_her(veh_fc_id, 0), -1, -1);
        if (*MultiplayerActive) {
            agenda_on(tgt_fc_id, veh_fc_id, AGENDA_UNK_20);
            if (action_id >= PRB_MIND_CONTROL_CITY) {
                ++plr->integrity_blemishes;
            }
            if (tgt_fc_id == MapWin->cOwner) {
                if (prb_alt_action) {
                    snprintf(StrBuffer, StrBufLen, "BUSTED%d", prb_alt_action);
                    Popup_start_4(&cur_popup, StrBuffer);
                    if (!plr_alien) {
                        popup_init_image(&cur_popup, "capture_sm.pcx", &bx, &by);
                    } else {
                        popup_init_image(&cur_popup, "al_cap_sm.pcx", &bx, &by);
                    }
                }
                Popup_start_3(&cur_popup, "NETEXCUSE", prb_alt_action != 0 ? 0x80 : 0);
                BasePop_exec_2(&cur_popup);
            }
            if (veh_fc_id == MapWin->cOwner) {
                parse_says(0, get_title(tgt_fc_id), -1, -1);
                parse_says(1, get_name(tgt_fc_id), -1, -1);
                parse_says(2, get_noun(tgt_fc_id), -1, -1);
                NetMsg_pop_2("HASNETEXCUSE", 0);
                return 1;
            }
        } else {
            if (has_treaty(tgt_fc_id, veh_fc_id, DIPLO_PACT)) {
                if (prb_alt_action) {
                    snprintf(StrBuffer, StrBufLen, "BUSTED%d", prb_alt_action);
                    Popup_start_4(&cur_popup, StrBuffer);
                    if (!plr_alien) {
                        popup_init_image(&cur_popup, "capture_sm.pcx", &bx, &by);
                    } else {
                        popup_init_image(&cur_popup, "al_cap_sm.pcx", &bx, &by);
                    }
                }
                Popup_start_3(&cur_popup, "PACTEXCUSE", prb_alt_action != 0 ? 0x80 : 0);
                if (BasePop_exec_2(&cur_popup)) {
                    pact_ends(veh_fc_id, tgt_fc_id);
                    tgt->diplo_spoke[veh_fc_id] = *CurrentTurn;
                    return 1;
                }
            } else {
                if (prb_alt_action) {
                    snprintf(StrBuffer, StrBufLen, "BUSTED%d", prb_alt_action);
                    Popup_start_4(&cur_popup, StrBuffer);
                    if (!plr_alien) {
                        popup_init_image(&cur_popup, "capture_sm.pcx", &bx, &by);
                    } else {
                        popup_init_image(&cur_popup, "al_cap_sm.pcx", &bx, &by);
                    }
                }
                Popup_start_3(&cur_popup, "EXCUSE", prb_alt_action != 0 ? 0x80 : 0);
                if (BasePop_exec_2(&cur_popup)) {
                    if (action_id >= PRB_MIND_CONTROL_CITY) {
                        double_cross(veh_fc_id, tgt_fc_id, -1);
                    } else {
                        treaty_on(veh_fc_id, tgt_fc_id, DIPLO_VENDETTA);
                    }
                    tgt->diplo_spoke[veh_fc_id] = *CurrentTurn;
                }
            }
        }
        return 1;
    }
    if (prb_state > 0) {
        treaty_on(tgt_fc_id, prb_state, DIPLO_UNK_40|DIPLO_VENDETTA|DIPLO_COMMLINK);
        set_treaty(tgt_fc_id, prb_state, DIPLO_WANT_REVENGE, 1);
        if (prb_state == MapWin->cOwner) {
            parse_says(0, get_title(tgt_fc_id), -1, -1);
            parse_says(1, get_name(tgt_fc_id), -1, -1);
            parse_says(2, get_noun(tgt_fc_id), -1, -1);
            NetMsg_pop_2("FRAMEJOB", infil_img);
        }
        if (veh_fc_id == MapWin->cOwner) {
            parse_says(0, get_title(tgt_fc_id), -1, -1);
            parse_says(1, get_name(tgt_fc_id), -1, -1);
            parse_says(2, get_noun(tgt_fc_id), -1, -1);
            parse_says(3, get_title(prb_state), -1, -1);
            parse_says(4, get_name(prb_state), -1, -1);
            parse_says(5, get_noun(prb_state), -1, -1);
            NetMsg_pop_2("FRAMED", infil_img);
        }
        return 1;
    }
    diplomacy_check(veh_fc_id, tgt_fc_id, 0);
    diplomacy_caption(veh_fc_id, tgt_fc_id);
    parse_says(0, get_title(veh_fc_id), -1, -1);
    parse_says(1, get_name(veh_fc_id), -1, -1);
    parse_says(2, get_pact_hood(veh_fc_id, tgt_fc_id), -1, -1);
    if (has_treaty(veh_fc_id, tgt_fc_id, DIPLO_PACT)) {
        if (action_id == PRB_PROCURE_RESEARCH_DATA) {
            if (tgt->tech_ranking > plr->tech_ranking
            && plr->ranking < 6 && !*diplo_value_93FA3C) {
                cause_friction(tgt_fc_id, veh_fc_id, game_randv(4));
                if (veh_fc_id == MapWin->cOwner) {
                    clear();
                    says("KEPTPACT");
                    say_num(1);
                    X_pops(StrBuffer, FactionPortraits[tgt_fc_id], 0);
                }
                return 1;
            }
        }
        if (action_id == PRB_PROCURE_RESEARCH_DATA || action_id < PRB_MIND_CONTROL_CITY) {
            if (plr->ranking < tgt->ranking && plr->ranking < 5) {
                if (veh_fc_id == MapWin->cOwner) {
                    X_pops("EXCUSEDPACT", FactionPortraits[tgt_fc_id], 0);
                }
                return 1;
            }
        }
        if (veh_fc_id == MapWin->cOwner) {
            X_pops("USEDEXCUSE", FactionPortraits[tgt_fc_id], 0);
        }
        if (action_id >= PRB_MIND_CONTROL_CITY) {
            double_cross(veh_fc_id, tgt_fc_id, -1);
            treaty_on(veh_fc_id, tgt_fc_id, DIPLO_VENDETTA);
        } else {
            pact_ends(veh_fc_id, tgt_fc_id);
        }
        return 1;
    }
    bool friction = false;
    if (!has_treaty(tgt_fc_id, veh_fc_id, DIPLO_WANT_REVENGE) && action_id < PRB_MIND_CONTROL_CITY) {
        if ((action_id == PRB_PROCURE_RESEARCH_DATA || action_id == PRB_ASSASSINATE_PROMINENT_RESEARCHERS)
        && tgt->tech_ranking > plr->tech_ranking && plr->ranking < 6 && !*diplo_value_93FA3C) {
            if (game_randv(8)) {
                friction = true;
            }
        }
        if (!friction && (action_id == PRB_ACTIVATE_SABOTAGE_VIRUS || action_id == PRB_INCITE_DRONE_RIOTS)) {
            if (*diplo_value_93FA98 && !*diplo_value_93FA3C) {
                friction = true;
            } else if (tgt->ranking > plr->ranking && !*diplo_value_93FA88 && game_randv(8)) {
                friction = true;
            }
        }
        if (friction) {
            cause_friction(tgt_fc_id, veh_fc_id, game_randv(4));
            if (veh_fc_id == MapWin->cOwner) {
                snprintf(StrBuffer, StrBufLen, "KEPT%d", action_id);
                X_pops(StrBuffer, FactionPortraits[tgt_fc_id], 0);
            }
            return 1;
        }
    }
    if (veh_fc_id == MapWin->cOwner) {
        X_pops("USEDEXCUSE", FactionPortraits[tgt_fc_id], 0);
    }
    if (action_id >= PRB_MIND_CONTROL_CITY) {
        double_cross(veh_fc_id, tgt_fc_id, -1);
    } else {
        treaty_on(veh_fc_id, tgt_fc_id, DIPLO_VENDETTA);
    }
    return 1;
}

