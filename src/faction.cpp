
#include "faction.h"


bool has_chassis(int faction_id, VehChassis chs) {
    return has_tech(Chassis[chs].preq_tech, faction_id);
}

bool has_weapon(int faction_id, VehWeapon wpn) {
    return has_tech(Weapon[wpn].preq_tech, faction_id);
}

bool has_wmode(int faction_id, VehWeaponMode mode) {
    for (int i = 0; i < MaxWeaponNum; i++) {
        if (Weapon[i].mode == mode && has_tech(Weapon[i].preq_tech, faction_id)) {
            return true;
        }
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (mod_veh_avail(i, faction_id, -1) && u->weapon_mode() == mode) {
            return true;
        }
    }
    return false;
}

bool has_aircraft(int faction_id) {
    for (int i = 0; i < MaxChassisNum; i++) {
        if (Chassis[i].triad == TRIAD_AIR && !Chassis[i].missile
        && has_tech(Chassis[i].preq_tech, faction_id)) {
            return true;
        }
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (mod_veh_avail(i, faction_id, -1) && u->triad() == TRIAD_AIR && !u->is_missile()) {
            return true;
        }
    }
    return false;
}

bool has_ships(int faction_id) {
    for (int i = 0; i < MaxChassisNum; i++) {
        if (Chassis[i].triad == TRIAD_SEA
        && has_tech(Chassis[i].preq_tech, faction_id)) {
            return true;
        }
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (mod_veh_avail(i, faction_id, -1) && u->triad() == TRIAD_SEA) {
            return true;
        }
    }
    return false;
}

bool has_terra(FormerItem frm_id, bool ocean, int faction_id) {
    int preq_tech = (ocean ? Terraform[frm_id].preq_tech_sea : Terraform[frm_id].preq_tech);
    if (preq_tech < TECH_None || (*GameRules & RULES_SCN_NO_TERRAFORMING
    && (frm_id == FORMER_RAISE_LAND || frm_id == FORMER_LOWER_LAND))) {
        return false;
    }
    if (frm_id >= FORMER_CONDENSER && frm_id <= FORMER_LEVEL_TERRAIN
    && has_project(FAC_WEATHER_PARADIGM, faction_id)) {
        return true;
    }
    return has_tech(preq_tech, faction_id);
}

bool has_project(FacilityId item_id) {
    assert(item_id >= SP_ID_First && item_id <= SP_ID_Last);
    int base_id = SecretProjects[item_id - SP_ID_First];
    return base_id >= 0;
}

bool has_project(FacilityId item_id, int faction_id) {
    assert(item_id >= SP_ID_First && item_id <= SP_ID_Last);
    int base_id = SecretProjects[item_id - SP_ID_First];
    return base_id >= 0 && faction_id >= 0 && Bases[base_id].faction_id == faction_id;
}

int project_base(FacilityId item_id) {
    assert(item_id >= SP_ID_First && item_id <= SP_ID_Last);
    return SecretProjects[item_id - SP_ID_First];
}

int facility_count(FacilityId item_id, int faction_id) {
    assert(valid_player(faction_id) && item_id < SP_ID_First);
    int n = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id && has_fac_built(item_id, i)) {
            n++;
        }
    }
    return n;
}

int prod_count(int item_id, int faction_id, int base_skip_id) {
    /* Prototypes must be predefined units or created by the faction. */
    assert(valid_player(faction_id));
    assert(item_id < 0
        || item_id/MaxProtoFactionNum == 0
        || item_id/MaxProtoFactionNum == faction_id);
    int n = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id
        && base->item() == item_id
        && i != base_skip_id) {
            n++;
        }
    }
    return n;
}

/*
Determine if the specified faction is controlled by a human player or computer AI.
*/
bool is_human(int faction_id) {
    assert(faction_id >= 0);
    return FactionStatus[0] & (1 << faction_id);
}

/*
Determine if the specified faction is a Progenitor faction (Caretakers / Usurpers).
*/
bool is_alien(int faction_id) {
    assert(faction_id >= 0);
    return *ExpansionEnabled && MFactions[faction_id].rule_flags & RFLAG_ALIEN;
}

/*
Determine if the specified faction is alive or whether they've been eliminated.
*/
bool is_alive(int faction_id) {
    assert(faction_id >= 0);
    return FactionStatus[1] & (1 << faction_id);
}

void set_alive(int faction_id, bool active) {
    assert(faction_id >= 0);
    if (active) {
        FactionStatus[1] |= (1 << faction_id);
    } else {
        FactionStatus[1] &= ~(1 << faction_id);
    }
}

/*
Exclude native life since Thinker AI routines don't apply to them.
*/
bool thinker_enabled(int faction_id) {
    return faction_id > 0 && !is_human(faction_id) && faction_id <= conf.factions_enabled;
}

bool at_war(int faction1, int faction2) {
    return faction1 != faction2 && faction1 >= 0 && faction2 >= 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_VENDETTA;
}

bool has_pact(int faction1, int faction2) {
    return faction1 >= 0 && faction2 >= 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_PACT;
}

bool both_neutral(int faction1, int faction2) {
    return faction1 >= 0 && faction2 >= 0 && faction1 != faction2
        && !(Factions[faction1].diplo_status[faction2] & (DIPLO_PACT|DIPLO_VENDETTA));
}

bool both_non_enemy(int faction1, int faction2) {
    return faction1 >= 0 && faction2 >= 0 && faction1 != faction2
        && !(Factions[faction1].diplo_status[faction2] & DIPLO_VENDETTA);
}

bool want_revenge(int faction1, int faction2) {
    return Factions[faction1].diplo_status[faction2] & (DIPLO_ATROCITY_VICTIM | DIPLO_WANT_REVENGE);
}

bool allow_expand(int faction_id) {
    int bases = 0;
    if (*GameRules & RULES_SCN_NO_COLONY_PODS || *BaseCount >= MaxBaseNum) {
        return false;
    }
    if (is_human(faction_id)) {
        return true;
    }
    for (int i = 1; i < MaxPlayerNum && conf.expansion_autoscale > 0; i++) {
        if (is_human(i) && is_alive(i)) {
            bases = max(bases, Factions[i].base_count);
        }
    }
    if (conf.expansion_limit > 0) {
        int pods = 0;
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->faction_id == faction_id && veh->is_colony()) {
                pods++;
            }
        }
        return Factions[faction_id].base_count + pods < max(bases, conf.expansion_limit);
    }
    return true;
}

bool has_active_veh(int faction_id, VehPlan plan) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id && Units[veh->unit_id].plan == plan) {
            return true;
        }
    }
    return false;
}

int veh_count(int faction_id, int unit_id) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    int num = 0;
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].faction_id == faction_id && Vehs[i].unit_id == unit_id) {
            num++;
        }
    }
    return num;
}

int find_hq(int faction_id) {
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id && has_fac_built(FAC_HEADQUARTERS, i)) {
            return i;
        }
    }
    return -1;
}

int faction_might(int faction_id) {
    return plans[faction_id].mil_strength + 4*Factions[faction_id].pop_total;
}

void __cdecl treaty_off(int faction_id_1, int faction_id_2, uint32_t status) {
    debug("treaty_off %d %d %08X\n", faction_id_1, faction_id_2, status);
    Faction* plr1 = &Factions[faction_id_1];
    Faction* plr2 = &Factions[faction_id_2];

    if (status & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        status |= (DIPLO_UNK_1000000|DIPLO_UNK_100000);
    }
    if (status & DIPLO_VENDETTA) {
        status |= (DIPLO_UNK_8000|DIPLO_UNK_40);
    }
    if (status & DIPLO_WANT_REVENGE) {
        plr1->diplo_agenda[faction_id_2] &= ~(AGENDA_UNK_800|AGENDA_UNK_400);
        plr2->diplo_agenda[faction_id_1] &= ~(AGENDA_UNK_800|AGENDA_UNK_400);
    }
    plr1->diplo_status[faction_id_2] &= ~status;
    plr2->diplo_status[faction_id_1] &= ~status;
}

void __cdecl agenda_off(int faction_id_1, int faction_id_2, uint32_t status) {
    debug("agenda_off %d %d %08X\n", faction_id_1, faction_id_2, status);
    Faction* plr1 = &Factions[faction_id_1];
    Faction* plr2 = &Factions[faction_id_2];
    plr1->diplo_agenda[faction_id_2] &= ~status;
    plr2->diplo_agenda[faction_id_1] &= ~status;
}

void __cdecl treaty_on(int faction_id_1, int faction_id_2, uint32_t status) {
    debug("treaty_on %d %d %08X\n", faction_id_1, faction_id_2, status);
    const bool is_player = (faction_id_1 == *CurrentPlayerFaction || faction_id_2 == *CurrentPlayerFaction);
    Faction* plr1 = &Factions[faction_id_1];
    Faction* plr2 = &Factions[faction_id_2];

    if (status & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        if (is_player && plr1->diplo_status[faction_id_2] & DIPLO_VENDETTA) {
            *dword_74DA9C &= ~1u;
        }
        plr1->diplo_status[faction_id_2] &= ~DiploExcludeTruce;
        plr2->diplo_status[faction_id_1] &= ~DiploExcludeTruce;
    }
    if (status & DIPLO_PACT) {
        status |= DIPLO_TREATY;
    }
    if (status & DIPLO_VENDETTA) {
        if (plr1->diplo_status[faction_id_2] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
            plr1->diplo_unk_4[faction_id_2] = 0;
            plr2->diplo_unk_4[faction_id_1] = 0;
        }
        status |= (DIPLO_UNK_80000000|DIPLO_UNK_100|DIPLO_COMMLINK);
        plr1->diplo_status[faction_id_2] &= ~DiploExcludeFight;
        plr2->diplo_status[faction_id_1] &= ~DiploExcludeFight;
        if (is_player) {
            *dword_74DA9C |= 1u;
            ambience(204);
        }
    }
    if (status & (DIPLO_VENDETTA|DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        plr1->diplo_status[faction_id_2] &= ~DIPLO_UNK_800000;
        plr2->diplo_status[faction_id_1] &= ~DIPLO_UNK_800000;
    }
    plr1->diplo_status[faction_id_2] |= status;
    plr2->diplo_status[faction_id_1] |= status;
    if (is_player && status & (DIPLO_VENDETTA|DIPLO_COMMLINK|DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        if (*dword_7FE06C) {
            GraphicWin_redraw(MultiWin);
        }
    }
}

void __cdecl agenda_on(int faction_id_1, int faction_id_2, uint32_t status) {
    debug("agenda_on %d %d %08X\n", faction_id_1, faction_id_2, status);
    Faction* plr1 = &Factions[faction_id_1];
    Faction* plr2 = &Factions[faction_id_2];
    if (status & AGENDA_UNK_20) {
        plr1->diplo_agenda[faction_id_2] &= ~AGENDA_UNK_40;
        plr2->diplo_agenda[faction_id_1] &= ~AGENDA_UNK_40;
    }
    plr1->diplo_agenda[faction_id_2] |= status;
    plr2->diplo_agenda[faction_id_1] |= status;
}

void __cdecl set_treaty(int faction_id_1, int faction_id_2, uint32_t status, bool add) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        if (add) {
            Factions[faction_id_1].diplo_status[faction_id_2] |= status;
            if (status & DIPLO_UNK_40) {
                Factions[faction_id_1].diplo_merc[faction_id_2] = 50;
            }
            if (status & DIPLO_HAVE_INFILTRATOR && conf.counter_espionage) {
                int turns = probe_active_turns(faction_id_1, faction_id_2);
                probe_renew_set(faction_id_1, faction_id_2, turns);
                if (faction_id_1 == MapWin->cOwner) {
                    ParseNumTable[0] = turns;
                    parse_says(0, parse_set(faction_id_2), -1, -1);
                    NetMsg_pop(NetMsg, "SPYRENEW", 5000, 0, 0);
                }
            }
        } else {
            Factions[faction_id_1].diplo_status[faction_id_2] &= ~status;
        }
    } else {
        assert(0);
    }
}

void __cdecl set_agenda(int faction_id_1, int faction_id_2, uint32_t status, bool add) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        if (add) {
            Factions[faction_id_1].diplo_agenda[faction_id_2] |= status;
        } else {
            Factions[faction_id_1].diplo_agenda[faction_id_2] &= ~status;
        }
    } else {
        assert(0);
    }
}

int __cdecl has_treaty(int faction_id_1, int faction_id_2, uint32_t status) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        return Factions[faction_id_1].diplo_status[faction_id_2] & status;
    } else {
        assert(0);
        return 0;
    }
}

int __cdecl has_agenda(int faction_id_1, int faction_id_2, uint32_t status) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        return Factions[faction_id_1].diplo_agenda[faction_id_2] & status;
    } else {
        assert(0);
        return 0;
    }
}

void __cdecl atrocity(int faction_id, int faction_id_tgt, int skip_init_check, int skip_human_check) {
    MFaction* m = &MFactions[faction_id];
    Faction* plr = &Factions[faction_id];
    bool prev_victim = has_treaty(faction_id_tgt, faction_id, DIPLO_ATROCITY_VICTIM);
    set_treaty(faction_id_tgt, faction_id, DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE, 1);
    set_agenda(faction_id_tgt, faction_id, AGENDA_UNK_4, 1);

    if (un_charter()) {
        if (*SunspotDuration > 0) {
            if (faction_id == *CurrentPlayerFaction) {
                NetMsg_pop(NetMsg, "ATROCITYSPOTS", 5000, 0, 0);
            }
        } else if (!*ExpansionEnabled || skip_human_check || (!is_alien(faction_id) && !is_alien(faction_id_tgt))) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (i != faction_id && i != faction_id_tgt && has_treaty(i, faction_id, DIPLO_COMMLINK)
                && (!*ExpansionEnabled || !skip_human_check || is_alien(faction_id) || !is_alien(i))) {
                    if (has_treaty(i, faction_id, DIPLO_UNK_200000)) {
                        cause_friction(i, faction_id, 5);
                        if (!is_human(i) && is_human(faction_id) && !prev_victim
                        && has_treaty(faction_id, i, DIPLO_COMMLINK)
                        && !has_treaty(faction_id, i, DIPLO_VENDETTA|DIPLO_PACT)) {
                            bool toggle;
                            if (has_treaty(i, faction_id, DIPLO_WANT_REVENGE)) {
                                toggle = true;
                            } else {
                                if (great_satan(faction_id, 0)) {
                                    toggle = game_randv(16) <= plr->atrocities + 8;
                                } else {
                                    int val = (*GameRules & RULES_INTENSE_RIVALRY ? 5 : *DiffLevel);
                                    toggle = !game_randv(2 * (8 - val));
                                }
                            }
                            if (toggle) {
                                if (faction_id == *CurrentPlayerFaction) {
                                    *plurality_default = 0;
                                    *gender_default = m->is_leader_female;
                                    parse_says(0, m->title_leader, -1, -1);
                                    parse_says(1, m->name_leader, -1, -1);
                                    diplomacy_caption(faction_id, i);
                                    X_pops3("ATROCIOUSITY", FactionPortraits[i], 0);
                                }
                                treaty_on(faction_id, i, DIPLO_VENDETTA);
                                Factions[i].diplo_status[faction_id] |= DIPLO_UNK_40;
                                Factions[i].diplo_merc[faction_id] = 50;
                            }
                        }
                    }
                    if (!has_treaty(i, faction_id, DIPLO_VENDETTA)) {
                        treaty_on(i, faction_id, DIPLO_UNK_400000);
                    }
                }
            }
            if (skip_init_check || !plr->atrocities) {
                plr->atrocities++;
                if (!has_treaty(faction_id, faction_id_tgt, DIPLO_ATROCITY_VICTIM)) {
                    plr->integrity_blemishes++;
                }
            }
            if (plr->atrocities <= 4 * (8 - plr->diff_level)) {
                int turns = 10 * plr->atrocities;
                // Fix: game used wrong variable while checking !is_human(faction_id_tgt)
                if (is_human(faction_id) && !is_human(faction_id_tgt) && !prev_victim && plr->atrocities >= 5) {
                    Factions[faction_id_tgt].player_flags |= PFLAG_COMMIT_ATROCITIES_WANTONLY;
                }
                *plurality_default = 0;
                *gender_default = m->is_leader_female;
                parse_says(0, m->title_leader, -1, -1);
                parse_says(1, m->name_leader, -1, -1);
                *plurality_default = 0;
                *gender_default = MFactions[faction_id_tgt].is_leader_female;
                parse_says(2, MFactions[faction_id_tgt].title_leader, -1, -1);
                parse_says(3, MFactions[faction_id_tgt].name_leader, -1, -1);
                parse_num(0, turns);
                if (!is_alien(faction_id)) {
                    if (faction_id == *CurrentPlayerFaction) {
                        NetMsg_pop(NetMsg, "ATROCITY2", 5000, 0, 0);
                    } else if (faction_id_tgt == faction_id) {
                        NetMsg_pop(NetMsg, "ATROCITY1", 5000, 0, 0);
                    } else {
                        NetMsg_pop(NetMsg, "ATROCITY", 5000, 0, 0);
                    }
                }
                plr->sanction_turns += turns;
            } else {
                major_atrocity(faction_id, faction_id_tgt);
            }
        }
    }
}

void __cdecl major_atrocity(int faction_id, int faction_id_tgt) {
    MFaction* m = &MFactions[faction_id];
    Faction* plr = &Factions[faction_id];
    if (faction_id_tgt >= 0) {
        set_treaty(faction_id_tgt, faction_id, DIPLO_MAJOR_ATROCITY_VICTIM|DIPLO_ATROCITY_VICTIM|DIPLO_WANT_REVENGE, 1);
        set_agenda(faction_id_tgt, faction_id, AGENDA_UNK_4, 1);
    }
    if (un_charter()) {
        if (faction_id_tgt < 0) {
            plr->integrity_blemishes += 2;
        } else if (!has_treaty(faction_id, faction_id_tgt, DIPLO_MAJOR_ATROCITY_VICTIM)) {
            plr->integrity_blemishes = clamp(plr->integrity_blemishes + 2, 6, 99);
        }
        plr->major_atrocities++;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && i != faction_id_tgt && !is_human(i) && is_alive(i)) {
                if (!has_treaty(faction_id, i, DIPLO_VENDETTA)) {
                    if (has_treaty(faction_id, i, DIPLO_PACT)) {
                        pact_ends(faction_id, i);
                    }
                    treaty_on(i, faction_id, DIPLO_VENDETTA|DIPLO_COMMLINK);
                    Factions[i].diplo_status[faction_id] |= DIPLO_UNK_40;
                    Factions[i].diplo_merc[faction_id] = 50;
                    plr->diplo_spoke[i] = *CurrentTurn;
                    *plurality_default = 0;
                    *gender_default = MFactions[i].is_leader_female;
                    parse_says(0, MFactions[i].title_leader, -1, -1);
                    parse_says(1, MFactions[i].name_leader, -1, -1);
                    *plurality_default = MFactions[i].is_noun_plural;
                    *gender_default = MFactions[i].noun_gender;
                    parse_says(2, MFactions[i].noun_faction, -1, -1);
                    *plurality_default = 0;
                    *gender_default = m->is_leader_female;
                    parse_says(3, m->title_leader, -1, -1);
                    parse_says(4, m->name_leader, -1, -1);
                    *plurality_default = m->is_noun_plural;
                    *gender_default = m->noun_gender;
                    parse_says(5, m->noun_faction, -1, -1);
                    snprintf(StrBuffer, StrBufLen, "MAJORATROCITY%d", faction_id != *CurrentPlayerFaction);
                    popp(ScriptFile, StrBuffer, 0, "council_sm.pcx", 0);
                }
            }
        }
    }
}

void __cdecl intervention(int faction_id_def, int faction_id_atk) {
    for (int i = 1; i < MaxPlayerNum; i++) {
        MFaction* m = &MFactions[i];
        Faction* plr = &Factions[i];
        if (i != faction_id_def && i != faction_id_atk && is_alive(i)
        && plr->diplo_status[faction_id_def] & DIPLO_PACT
        && !(plr->diplo_status[faction_id_atk] & (DIPLO_VENDETTA|DIPLO_PACT))) {
            *plurality_default = m->is_noun_plural;
            *gender_default = m->noun_gender;
            parse_says(0, m->noun_faction, -1, -1);
            *plurality_default = MFactions[faction_id_def].is_noun_plural;
            *gender_default = MFactions[faction_id_def].noun_gender;
            parse_says(1, MFactions[faction_id_def].noun_faction, -1, -1);
            *plurality_default = 0;
            *gender_default = m->is_leader_female;
            parse_says(2, m->title_leader, -1, -1);
            parse_says(3, m->name_leader, -1, -1);
            *plurality_default = 0;
            *gender_default = MFactions[faction_id_atk].is_leader_female;
            parse_says(4, MFactions[faction_id_atk].title_leader, -1, -1);
            parse_says(5, MFactions[faction_id_atk].name_leader, -1, -1);
            *plurality_default = MFactions[faction_id_atk].is_noun_plural;
            *gender_default = MFactions[faction_id_atk].noun_gender;
            parse_says(6, MFactions[faction_id_atk].noun_faction, -1, -1);
            if (is_human(faction_id_atk) || is_human(faction_id_def)) {
                if (is_human(faction_id_atk)) {
                    Factions[faction_id_atk].diplo_spoke[i] = *CurrentTurn;
                }
                treaty_on(i, faction_id_atk, DIPLO_UNK_80000000|DIPLO_VENDETTA|DIPLO_COMMLINK);
                set_treaty(i, faction_id_atk, DIPLO_UNK_40, 1);
            }
            if (faction_id_atk == *CurrentPlayerFaction) {
                X_pop2("AIDSTHEM", 0);
            } else if (faction_id_def == *CurrentPlayerFaction) {
                X_pop2("AIDSUS", 0);
            }
        }
    }
}

void __cdecl double_cross(int faction_id_atk, int faction_id_def, int faction_id_other) {
    MFaction* m_atk = &MFactions[faction_id_atk];
    Faction* plr_atk = &Factions[faction_id_atk];
    Faction* plr_def = &Factions[faction_id_def];

    if (*MultiplayerActive) {
        if (faction_id_def != *CurrentPlayerFaction || faction_id_other <= 0) {
            if (faction_id_def == *CurrentPlayerFaction && is_human(faction_id_atk)) {
                *plurality_default = 0;
                *gender_default = m_atk->is_leader_female;
                parse_says(0, m_atk->title_leader, -1, -1);
                parse_says(1, m_atk->name_leader, -1, -1);
                *plurality_default = m_atk->is_noun_plural;
                *gender_default = m_atk->noun_gender;
                parse_says(2, m_atk->noun_faction, -1, -1);
                NetMsg_pop(NetMsg, "VENDETTAWARNING", 5000, 0, 0);
            }
        } else {
            MFaction* m_other = &MFactions[faction_id_other];
            *plurality_default = 0;
            *gender_default = m_other->is_leader_female;
            parse_says(0, m_other->title_leader, -1, -1);
            parse_says(1, m_other->name_leader, -1, -1);
            *plurality_default = m_other->is_noun_plural;
            *gender_default = m_other->noun_gender;
            parse_says(2, m_other->noun_faction, -1, -1);
            *plurality_default = 0;
            *gender_default = m_atk->is_leader_female;
            parse_says(3, m_atk->title_leader, -1, -1);
            parse_says(4, m_atk->name_leader, -1, -1);
            *plurality_default = m_atk->is_noun_plural;
            *gender_default = m_atk->noun_gender;
            parse_says(5, m_atk->noun_faction, -1, -1);
            NetMsg_pop(NetMsg, "INCITED", 5000, 0, 0);
        }
    }
    plr_def->loan_balance[faction_id_atk] = 0;
    int is_allowed = 0;
    int is_victim = 0;
    if (has_treaty(faction_id_atk, faction_id_def, DIPLO_ATROCITY_VICTIM) || plr_def->major_atrocities) {
        is_victim = 1;
    }
    if (faction_id_other >= 0) {
        if (has_treaty(faction_id_atk, faction_id_other, DIPLO_PACT)) {
            if (!has_treaty(faction_id_atk, faction_id_def, DIPLO_PACT)) {
                is_allowed = 1;
            }
        } else {
            if (!has_treaty(faction_id_atk, faction_id_other, DIPLO_TREATY)) {
                if (has_treaty(faction_id_atk, faction_id_other, DIPLO_TRUCE)
                && !has_treaty(faction_id_atk, faction_id_def, DIPLO_TRUCE)) {
                    is_allowed = 1;
                }
            } else if (!has_treaty(faction_id_atk, faction_id_def, DIPLO_TREATY)) {
                is_allowed = 1;
            }
        }
        plr_atk->diplo_gifts[faction_id_other]++;
    }
    if (has_treaty(faction_id_atk, faction_id_def, DIPLO_PACT)) {
        if (!has_agenda(faction_id_atk, faction_id_def, AGENDA_UNK_20)) {
            plr_atk->diplo_reputation = max(0, plr_atk->diplo_reputation - 1);
            if (plr_atk->diff_level && !is_victim && !is_allowed) {
                plr_atk->integrity_blemishes++;
                plr_def->diplo_betrayed[faction_id_atk]++;
            }
            if (faction_id_other >= 0) {
                cause_friction(faction_id_other, faction_id_atk, -4);
            } else {
                if (!is_victim) {
                    plr_atk->integrity_blemishes++;
                }
                plr_atk->diplo_wrongs[faction_id_def]++;
                plr_def->diplo_betrayed[faction_id_atk]++;
            }
            if (is_human(faction_id_atk)) {
                if (has_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_400)) {
                    set_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_800, 1);
                }
                if (has_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_REVENGE)) {
                    set_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_400, 1);
                }
                set_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_REVENGE, 1);
            }
        }
        pact_ends(faction_id_atk, faction_id_def);
        treaty_on(faction_id_atk, faction_id_def, DIPLO_VENDETTA);
        return; // Skip other cases
    }
    if (!has_agenda(faction_id_atk, faction_id_def, AGENDA_UNK_20)) {
        bool has_truce = has_treaty(faction_id_atk, faction_id_def, DIPLO_TRUCE|DIPLO_TREATY);
        if (has_truce) {
            if (!has_treaty(faction_id_atk, faction_id_def, DIPLO_UNK_10000)) {
                plr_atk->diplo_reputation = max(0, plr_atk->diplo_reputation - 1);
                if ((great_satan(faction_id_atk, 0) || (plr_atk->ranking >= 6 && plr_def->ranking <= 4))
                && faction_id_other < 0 && !is_victim) {
                    plr_atk->integrity_blemishes++;
                    plr_def->diplo_betrayed[faction_id_atk]++;
                }
            }
            if (plr_atk->diff_level && !is_victim && !is_allowed) {
                plr_atk->integrity_blemishes++;
                plr_def->diplo_betrayed[faction_id_atk]++;
            }
            if (faction_id_other < 0) {
                plr_atk->diplo_wrongs[faction_id_def]++;
            }
        }
        if (has_truce || faction_id_other >= 0) {
            if (is_human(faction_id_atk)) {
                if (has_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_400)) {
                    set_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_800, 1);
                }
                if (has_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_REVENGE)) {
                    set_agenda(faction_id_def, faction_id_atk, AGENDA_UNK_400, 1);
                }
                set_treaty(faction_id_def, faction_id_atk, DIPLO_WANT_REVENGE, 1);
            }
            if (faction_id_other >= 0) {
                cause_friction(faction_id_other, faction_id_atk, -2);
            }
        }
    }
    treaty_on(faction_id_atk, faction_id_def, DIPLO_VENDETTA);
    if (!has_agenda(faction_id_atk, faction_id_def, AGENDA_UNK_20)) {
        if (is_human(faction_id_atk) != is_human(faction_id_def)) {
            set_treaty(faction_id_atk, faction_id_def, DIPLO_UNK_40, 1);
            plr_atk->diplo_spoke[faction_id_def] = *CurrentTurn;
        }
        intervention(faction_id_def, faction_id_atk);
    }
}

int __cdecl act_of_aggression(int faction_id_atk, int faction_id_def) {
    int value = 0;
    if (has_treaty(faction_id_def, faction_id_atk, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        double_cross(faction_id_atk, faction_id_def, -1);
        value = 1;
    }
    treaty_on(faction_id_atk, faction_id_def, DIPLO_UNK_8000|DIPLO_VENDETTA);
    return value;
}

int __cdecl steal_tech(int faction_id, int faction_id_tgt, int is_steal) {
    if (!faction_id || !faction_id_tgt
    || (*MultiplayerActive && is_human(faction_id) && faction_id != *CurrentPlayerFaction)) {
        return 0;
    }
    int tech_id = mod_tech_pick(faction_id, 0, faction_id_tgt, is_steal ? "STEALTECH" : "ACQUIRETECH");
    if (tech_id < 0) {
        tech_id = 9999;
    }
    if (*MultiplayerActive && faction_id == *CurrentPlayerFaction) {
        message_data(0x244C, 0, faction_id, tech_id, faction_id_tgt, 0);
    } else {
        if (faction_id_tgt == *CurrentPlayerFaction && tech_id != 9999) {
            MFaction* m = &MFactions[faction_id];
            *plurality_default = m->is_noun_plural;
            *gender_default = m->noun_gender;
            parse_says(0, m->noun_faction, -1, -1);
            StrBuffer[0] = '\0';
            say_tech(StrBuffer, tech_id, 0);
            parse_says(1, StrBuffer, -1, -1);
            NetMsg_pop(NetMsg, "STOLETECH", 5000, 0, 0);
        }
        tech_achieved(faction_id, tech_id, faction_id_tgt, 0);
        if (!is_human(faction_id) && tech_id != 9999) {
            bases_reset(-1, faction_id, 0);
        }
    }
    return tech_id == 9999;
}

/*
Calculate faction's vote count. Used for Planetary Governor and Supreme Leader.
Return Value: Faction vote count
*/
int __cdecl council_votes(int faction_id) {
    if (is_alien(faction_id)) {
        return 0;
    }
    int votes = 0;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id ) {
            votes += Bases[i].pop_size;
        }
    }
    if (has_project(FAC_EMPATH_GUILD, faction_id)) {
        votes += votes / 2; // +50% votes
    }
    if (has_project(FAC_CLINICAL_IMMORTALITY, faction_id)) {
        votes *= 2; // Doubles votes
    }
    int bonus_count = MFactions[faction_id].faction_bonus_count;
    for (int i = 0; i < bonus_count; i++) {
        if (MFactions[faction_id].faction_bonus_id[i] == RULE_VOTES) {
            int votes_bonus = MFactions[faction_id].faction_bonus_val1[i];
            if (votes_bonus >= 0) {
                votes *= votes_bonus; // Peacekeeper bonus
            }
        }
    }
    return votes;
}

/*
Check whether a faction's leader is eligible to be a Planetary Governor candidate.
Return Value: Is the leader eligible (top two vote totals)? true/false
*/
int __cdecl eligible(int faction_id) {
    if (is_alien(faction_id)) {
        return false;
    }
    int votes = council_votes(faction_id);
    int faction_count = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id != i && is_alive(i) && council_votes(i) > votes) {
            faction_count++;
        }
    }
    return faction_count < 2;
}

/*
Determine if the overall dominant human faction is a minor threat based on base count.
Return Value: Is faction minor threat? true/false
*/
int __cdecl great_beelzebub(int faction_id, int is_aggressive) {
    if (is_human(faction_id) && FactionRankings[7] == faction_id) {
        int bases_threat = (*CurrentTurn + 25) / 50;
        if (bases_threat < 4) {
            bases_threat = 4;
        }
        if (Factions[faction_id].base_count > bases_threat
        && (Factions[faction_id].diff_level > DIFF_SPECIALIST
        || *GameRules & RULES_INTENSE_RIVALRY || is_aggressive)) {
            return true;
        }
    }
    return false;
}

/*
Determine if the specified faction is considered a threat based on the game state and ranking.
Return Value: Is the specified faction a threat? true/false
*/
int __cdecl great_satan(int faction_id, int is_aggressive) {
    if (great_beelzebub(faction_id, is_aggressive)) {
        bool intense_rivalry = (*GameRules & RULES_INTENSE_RIVALRY);
        if (*CurrentTurn <= ((intense_rivalry ? 0
        : (DIFF_TRANSCEND - Factions[faction_id].diff_level) * 50) + 100)) {
            return false;
        }
        if (climactic_battle() && aah_ooga(faction_id, -1) == faction_id) {
            return true;
        }
        int diff_factor;
        int factor;
        if (intense_rivalry) {
            factor = 4;
            diff_factor = DIFF_TRANSCEND;
        } else if (Factions[faction_id].diff_level >= DIFF_LIBRARIAN
            || *GameRules & RULES_VICTORY_CONQUEST || *ObjectiveReqVictory <= 1000) {
            factor = 2;
            diff_factor = DIFF_LIBRARIAN;
        } else {
            factor = 1;
            diff_factor = DIFF_TALENT;
        }
        return (factor * FactionRankingsUnk[FactionRankings[7]]
            >= diff_factor * FactionRankingsUnk[FactionRankings[6]]);
    }
    return false;
}

/*
Check whether the specified faction is nearing the diplomatic victory requirements to be
able to call a Supreme Leader vote. Optional 2nd parameter (0/-1 to disable) that specifies
a faction to skip if they have a pact with faction from the 1st parameter.
Return Value: faction_id if nearing diplomatic victory or zero
*/
int __cdecl aah_ooga(int faction_id, int pact_faction_id) {
    if (!(*GameRules & RULES_VICTORY_DIPLOMATIC)) {
        return 0; // Diplomatic Victory not allowed
    }
    int votes_total = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        votes_total += council_votes(i);
    }
    int value = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != pact_faction_id
        && (pact_faction_id <= 0 || !has_treaty(i, pact_faction_id, DIPLO_PACT)
        || !(*GameRules & RULES_VICTORY_COOPERATIVE))) {
            int proposal_preq = Proposal[PROP_UNITE_SUPREME_LEADER].preq_tech;
            if ((has_tech(proposal_preq, faction_id)
            || (proposal_preq >= 0
            && (has_tech(Tech[proposal_preq].preq_tech1, faction_id)
            || has_tech(Tech[proposal_preq].preq_tech2, faction_id))))
            && council_votes(i) * 2 >= votes_total && (!value || i == faction_id)) {
                value = i;
            }
        }
    }
    return value;
}

/*
Check if the human controlled player is nearing the endgame.
Return Value: Is human player nearing endgame? true/false
*/
int __cdecl climactic_battle() {
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_human(i) && Factions[i].corner_market_turn > *CurrentTurn) {
            return true;
        }
    }
    // Removed following code since this will always return false.
    // This may have been disabled as a design decision rather than a bug.
    // if (aah_ooga(0, -1)) { return true; }
    assert(!aah_ooga(0, -1));

    if (voice_of_planet()) { // Replace function ascending(0)
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (is_human(i) && (has_tech(Facility[FAC_PSI_GATE].preq_tech, i)
            || has_tech(Facility[FAC_VOICE_OF_PLANET].preq_tech, i))) {
                return true;
            }
        }
    }
    return false;
}

/*
Determine if the specified AI faction is at the end game based on certain conditions.
Return Value: Is the AI faction near victory? true/false
*/
int __cdecl at_climax(int faction_id) {
    if (is_human(faction_id) || *GameState & STATE_GAME_DONE
    || *DiffLevel == DIFF_CITIZEN || !climactic_battle()) {
        return false;
    }
    if (aah_ooga(faction_id, faction_id)) {
        return true;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id && Factions[faction_id].corner_market_turn > *CurrentTurn
        && (!has_treaty(faction_id, i, DIPLO_PACT) || !(*GameRules & RULES_VICTORY_COOPERATIVE))) {
            return true;
        }
    }
    int sp_mins_them = 0;
    int sp_mins_own = 0;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].queue_items[0] == -FAC_ASCENT_TO_TRANSCENDENCE) {
            int min_accum = Bases[i].minerals_accumulated;
            if (Bases[i].faction_id == faction_id) {
                if (sp_mins_own <= min_accum) {
                    sp_mins_own = min_accum;
                }
            } else if (sp_mins_them <= min_accum) {
                sp_mins_them = min_accum;
            }
        }
    }
    // Removed redundant ascending() check here
    return sp_mins_them && sp_mins_them > sp_mins_own;
}

/*
Add friction between the two specified factions.
*/
void __cdecl cause_friction(int faction_id, int faction_id_with, int friction) {
    Faction* f = &Factions[faction_id];
    f->diplo_friction[faction_id_with] = clamp(f->diplo_friction[faction_id_with] + friction, 0, 20);
    if (*DiploFrictionFactionID == faction_id && *DiploFrictionFactionIDWith == faction_id_with) {
        *DiploFriction = clamp(*DiploFriction + friction, 0, 20); // Fix: add variable bounding
    }
}

/*
Normalize the diplomatic friction value into a mood offset.
Return Value: Mood (0-8)
*/
int __cdecl get_mood(int friction) {
    if (friction <= 0) {
        return MOOD_MAGNANIMOUS; // can be also displayed as "Submissive"
    }
    if (friction <= 2) {
        return MOOD_SOLICITOUS;
    }
    if (friction <= 4) {
        return MOOD_COOPERATIVE;
    }
    if (friction <= 8) {
        return MOOD_NONCOMMITTAL;
    }
    if (friction <= 12) {
        return MOOD_AMBIVALENT;
    }
    if (friction <= 15) {
        return MOOD_OBSTINATE;
    }
    if (friction <= 17) {
        return MOOD_QUARRELSOME;
    }
    return (friction > 19) + MOOD_BELLIGERENT; // or MOOD_SEETHING if condition is true
}

/*
Calculate the negative reputation the specified faction has with another.
Return Value: Reputation (higher values are worse)
*/
int __cdecl reputation(int faction_id, int faction_id_with) {
    return clamp(Factions[faction_id].integrity_blemishes
        - Factions[faction_id].diplo_gifts[faction_id_with], 0, 99);
}

/*
Calculate the amount of patience the specified faction has with another.
Return Value: Patience
*/
int __cdecl get_patience(int faction_id_with, int faction_id) {
    if (DEBUG && conf.diplo_patience > 0) {
        return conf.diplo_patience;
    }
    if (has_treaty(faction_id, faction_id_with, DIPLO_VENDETTA)) {
        return 1;
    }
    if (has_treaty(faction_id, faction_id_with, DIPLO_PACT)) {
        return has_treaty(faction_id, faction_id_with, DIPLO_HAVE_SURRENDERED) ? 500 : 6;
    }
    return (has_treaty(faction_id, faction_id_with, DIPLO_TREATY) != 0)
        - ((*DiploFriction + 3) / 8) + 3;
}

/*
Calculate the amount of goodwill a loan will generate. This is used to reduce friction.
Return Value: Goodwill (friction reduction amount)
*/
int __cdecl energy_value(int loan_principal) {
    int goodwill = 0;
    int divisor = 2;
    for (int weight = 10, energy = loan_principal / 5; energy > 0; energy -= weight, weight = 20) {
        goodwill += ((weight >= 0) ? ((energy > weight) ? weight : energy) : 0) / divisor++;
    }
    return (goodwill + 4) / 5;
}

/*
Calculate the social engineering effect modifiers for the specified faction.
*/
void __cdecl social_calc(CSocialCategory* category, CSocialEffect* effect,
int faction_id, int UNUSED(toggle), int is_quick_calc) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    memset(effect, 0, sizeof(CSocialEffect));
    for (int cat = 0; cat < MaxSocialCatNum; cat++) {
        int model = *(&category->politics + cat);
        assert(model >= 0 && model < MaxSocialModelNum);
        for (int eff = 0; eff < MaxSocialEffectNum; eff++) {
            int effect_val = *(&SocialField[cat].soc_effect[model].economy + eff);
            if (effect_val < 0) {
                if (cat == SOCIAL_C_FUTURE) {
                    if (model == SOCIAL_M_CYBERNETIC) {
                        if (has_project(FAC_NETWORK_BACKBONE, faction_id)) {
                            effect_val = 0;
                        }
                    } else if (model == SOCIAL_M_THOUGHT_CONTROL) {
                        if (has_project(FAC_CLONING_VATS, faction_id)) {
                            effect_val = 0;
                        }
                    }
                } else if (cat == SOCIAL_C_VALUES && model == SOCIAL_M_POWER
                && has_project(FAC_CLONING_VATS, faction_id)) {
                    effect_val = 0;
                }
                if (effect_val < 0) {
                    for (int i = 0; i < m->faction_bonus_count; i++) {
                        if (m->faction_bonus_val1[i] == cat
                        && m->faction_bonus_val2[i] == model) {
                            if (m->faction_bonus_id[i] == RULE_IMPUNITY) {
                                *(&effect->economy + eff) -= effect_val; // negates neg effects
                            } else if (m->faction_bonus_id[i] == RULE_PENALTY) {
                                *(&effect->economy + eff) += effect_val; // doubles neg effects
                            }
                        }
                    }
                }
            }
            *(&effect->economy + eff) += effect_val;
        }
    }
    if (!is_quick_calc) {
        if (has_project(FAC_ASCETIC_VIRTUES, faction_id)) {
            effect->police++;
        }
        if (has_project(FAC_LIVING_REFINERY, faction_id)) {
            effect->support += 2;
        }
        if (has_temple(faction_id)) {
            effect->planet++;
            if (is_alien(faction_id)) {
                effect->research++; // bonus documented in conceptsx.txt but not manual
            }
        }
        CSocialEffect* effect_base = (CSocialEffect*)&f->SE_economy_base;
        for (int eff = 0; eff < MaxSocialEffectNum; eff++) {
            *(&effect->economy + eff) += *(&effect_base->economy + eff);
        }
        for (int i = 0; i < m->faction_bonus_count; i++) {
            int bonus_id = m->faction_bonus_id[i];
            int bonus_val = m->faction_bonus_val1[i];
            if (bonus_id == RULE_IMMUNITY || bonus_id == RULE_ROBUST) {
                int32_t* effect_value = (&effect->economy + bonus_val);
                assert(bonus_val >= 0 && bonus_val < MaxSocialEffectNum);
                if (bonus_id == RULE_IMMUNITY) { // cancels neg effects
                    *effect_value = clamp(*effect_value, 0, 999);
                } else if (bonus_id == RULE_ROBUST) { // halves neg effects
                    if (*effect_value < 0) {
                        *effect_value /= 2;
                    }
                }
            }
        }
    }
}

/*
Handle the social engineering turn upkeep for the specified faction.
*/
void __cdecl social_upkeep(int faction_id) {
    Faction* f = &Factions[faction_id];
    CSocialCategory* pending = (CSocialCategory*)&f->SE_Politics_pending;
    CSocialCategory* current = (CSocialCategory*)&f->SE_Politics;
    memcpy(current, pending, sizeof(CSocialCategory));
    social_calc(pending, (CSocialEffect*)&f->SE_economy_pending, faction_id, false, false);
    social_calc(pending, (CSocialEffect*)&f->SE_economy, faction_id, false, false);
    social_calc(pending, (CSocialEffect*)&f->SE_economy_2, faction_id, true, false);
    f->SE_upheaval_cost_paid = 0;
}

/*
Calculate the cost of the social upheaval for the specified faction.
Return Value: Social upheaval cost
*/
int __cdecl social_upheaval(int faction_id, CSocialCategory* choices) {
    Faction* f = &Factions[faction_id];
    int changes = 0;
    for (int i = 0; i < MaxSocialCatNum; i++) {
        if (*(&choices->politics + i) != (&f->SE_Politics)[i]) {
            changes++;
        }
    }
    if (!changes) {
        return 0;
    }
    changes++;
    int diff_lvl = is_human(faction_id) ? f->diff_level : DIFF_LIBRARIAN;
    int cost = changes * changes * changes * diff_lvl;
    if (is_alien(faction_id)) {
        cost += cost / 2;
    }
    return cost;
}

/*
Check to see whether the faction can utilize a specific social category and model.
Return Value: Is social category/model available to faction? true/false
*/
int __cdecl society_avail(int soc_category, int soc_model, int faction_id) {
    if (MFactions[faction_id].soc_opposition_category == soc_category
    && MFactions[faction_id].soc_opposition_model == soc_model) {
        return false;
    }
    return has_tech(SocialField[soc_category].soc_preq_tech[soc_model], faction_id);
}

/*
Improved social engineering AI choices feature.
*/
static int social_score(int faction_id, int sf, int sm, bool pop_boom) {
    assert(sf >= 0 && sf < 4 && sm >= 0 && sm < 4);
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    int def_val = plans[faction_id].defense_modifier;
    int base_val = max(1, f->base_count);
    int base_ratio = min(10, 10 * f->base_count / clamp(*MapAreaSqRoot / 2, 8, 40));
    int morale_mod = (has_project(FAC_COMMAND_NEXUS, faction_id) ? 2 : 0)
        + (has_project(FAC_CYBORG_FACTORY, faction_id) ? 2 : 0);
    int probe_mod = (*CurrentTurn - m->thinker_last_mc_turn < 10 ? def_val + 1 : 0);
    int sc = 0;
    CSocialEffect vals;
    CSocialCategory soc;
    memcpy(&soc, &f->SE_Politics, sizeof(soc));

    if ((&soc.politics)[sf] == sm) {
        // Evaluate the current active social model.
        memcpy(&vals, &f->SE_economy, sizeof(vals));
    } else {
        // Take the faction base social values and apply all modifiers.
        (&soc.politics)[sf] = sm;
        memcpy(&vals, &f->SE_economy_base, sizeof(vals));
        social_calc(&soc, &vals, faction_id, 0, 0);
    }
    if (m->soc_priority_category >= 0 && m->soc_priority_model >= 0) {
        if (sf == m->soc_priority_category) {
            if (sm == m->soc_priority_model) {
                sc += conf.social_ai_bias;
            } else if (sm != SOCIAL_M_FRONTIER) {
                sc -= conf.social_ai_bias;
            }
        } else {
            if ((&f->SE_Politics)[m->soc_priority_category] == m->soc_priority_model) {
                sc += conf.social_ai_bias;
            } else if ((&f->SE_Politics)[m->soc_priority_category] != SOCIAL_M_FRONTIER) {
                sc -= conf.social_ai_bias;
            }
        }
    }
    // AIs also take into account Social Effect priorities whenever social_ai_bias >= 10
    if (m->soc_priority_effect >= 0 && m->soc_priority_effect < MaxSocialEffectNum) {
        sc += clamp((&vals.economy)[m->soc_priority_effect], -4, 4)
            * clamp(conf.social_ai_bias / 10, 0, 2);
    }
    if (vals.economy >= 2) {
        sc += (vals.economy >= 4 ? 16 : 12);
    }
    if (vals.efficiency < -2) {
        sc -= (vals.efficiency < -3 ? 16 : 8);
    }
    if (vals.support < -3) {
        sc -= 16;
    }
    if (vals.morale >= 1 && vals.morale + morale_mod >= 4) {
        sc += 10;
    }
    if (vals.probe >= 3 && !has_project(FAC_HUNTER_SEEKER_ALGORITHM, faction_id)) {
        sc += 4 * def_val;
    }
    sc += max(2, 2 + 4*f->AI_wealth + 3*f->AI_tech - f->AI_fight)
        * clamp(vals.economy, -3, 5);
    sc += max(2, 2*f->AI_wealth + 2*f->AI_tech - f->AI_fight + base_ratio/2)
        * clamp(vals.efficiency, -4, 6);
    sc += max(2, 3 + 2*f->AI_power + 2*f->AI_fight - base_ratio/4 + def_val/2)
        * clamp(vals.support, -4, 3);
    sc += max(2, def_val + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals.morale, -4, 4);
    bool creche = has_tech(Facility[FAC_CHILDREN_CRECHE].preq_tech, faction_id)
        || has_free_facility(FAC_CHILDREN_CRECHE, faction_id);
    bool sphere = has_tech(Facility[FAC_PUNISHMENT_SPHERE].preq_tech, faction_id);
    bool nodrones = has_free_facility(FAC_PUNISHMENT_SPHERE, faction_id);
    bool skipdrones = has_project(FAC_TELEPATHIC_MATRIX, faction_id);

    if (skipdrones && !nodrones) {
        sc += 4*clamp(vals.talent, 0, 5); // consider only extra talents
    } else if (!nodrones) {
        sc += 4*clamp(vals.talent, -5, 5);
        sc += (vals.police >= 0 || def_val > 2 ? 4 : 2)
            * clamp(vals.police, (def_val > 2 ? -10 : -5), 3);
        if (vals.police < -2) {
            sc -= (vals.police < -3 ? 2 : 1) * def_val
                * (has_aircraft(faction_id) ? 2 : 1);
        }
        if (has_project(FAC_LONGEVITY_VACCINE, faction_id) && sf == SOCIAL_C_ECONOMICS) {
            sc += (sm == SOCIAL_M_PLANNED ? 10 : 0);
            sc += (sm == SOCIAL_M_SIMPLE || sm == SOCIAL_M_GREEN ? 5 : 0);
        }
        int drone_score = 3 + (m->rule_drone > 0) - (m->rule_talent > 0) - (sphere ? 1 : 0);
        if (*SunspotDuration > 1 && *DiffLevel >= DIFF_LIBRARIAN
        && un_charter() && vals.police >= 0) {
            sc += 3*drone_score;
        }
        if (!un_charter() && vals.police >= 0) {
            sc += 2*drone_score;
        }
    }
    if (!has_project(FAC_CLONING_VATS, faction_id)) {
        if (pop_boom && vals.growth + (creche ? 2 : 0) >= GrowthPopBoom) {
            sc += 20;
        }
        if (vals.growth < -2) {
            sc -= 20;
        }
        sc += ((def_val < 3 ? 5 : 3) + (pop_boom ? 3 : 0)) * clamp(vals.growth, -3, GrowthPopBoom);
    }
    if (plans[faction_id].keep_fungus) {
        sc += 3*clamp(vals.planet, -3, 0); // penalty for reduced fungus yield
    }
    sc += max(2, (f->SE_planet_base > 0 ? 5 : 2) + m->rule_psi/10
        + (has_project(FAC_MANIFOLD_HARMONICS, faction_id) ? 6 : 0)) * clamp(vals.planet, -3, 3);
    sc += max(2, 1 + def_val + probe_mod + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals.probe, -2, 3);
    sc += (2*clamp(vals.industry, -3, 5) - 8*mineral_factor(faction_id, vals.industry));

    if (!(*GameRules & RULES_SCN_NO_TECH_ADVANCES)) {
        sc += max(2, 3 + 4*f->AI_tech + 2*(f->AI_wealth - f->AI_fight))
            * clamp(vals.research, -5, 5);
    }

    int psy_val = f->social_psych[clamp(vals.talent + 3, 0, 7)][clamp(vals.police + 5, 0, 8)];
    int psy_mod = psy_val / base_val;
    int sup_val = f->social_support[clamp(vals.support + 4, 0, 7)];
    int sup_mod = 4 * sup_val / base_val;
    int eff_val = f->social_effic[clamp(8 - vals.efficiency, 0, 8)];
    int eff_mod = 2 * eff_val / base_val;
    sc += ((skipdrones || nodrones ? 0 : psy_mod) - sup_mod - eff_mod);

    debug("social_values %d %d %8s psy: %d sup: %d eff: %d score: %d %s\n",
        *CurrentTurn, faction_id, m->filename, psy_mod, sup_mod, eff_mod, sc, SocialField[sf].soc_name[sm]);
    return sc;
}

int __cdecl SocialWin_social_ai(int faction_id,
int UNUSED(a2), int UNUSED(a3), int UNUSED(a4), int UNUSED(a5), int UNUSED(a6))
{
    Faction* f = &Factions[faction_id];
    if (f->SE_upheaval_cost_paid > 0) {
        social_set(faction_id);
    }
    return 0;
}

int __cdecl mod_social_ai(int faction_id, int a2, int a3, int a4, int a5, int a6) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    AIPlans* p = &plans[faction_id];

    if (is_human(faction_id) || !is_alive(faction_id)) {
        return 0;
    }
    if (!thinker_enabled(faction_id) || !conf.social_ai) {
        design_units(faction_id);
        return social_ai(faction_id, a2, a3, a4, a5, a6);
    }
    if (f->SE_upheaval_cost_paid > 0) {
        social_set(faction_id);
        return 0;
    }
    int def_value = p->defense_modifier;
    int want_pop = 0;
    int pop_total = 0;
    bool pop_boom = 0;
    bool has_creche = has_tech(Facility[FAC_CHILDREN_CRECHE].preq_tech, faction_id)
        || has_free_facility(FAC_CHILDREN_CRECHE, faction_id);
    assert(!memcmp(&f->SE_Politics, &f->SE_Politics_pending, 16));

    if (has_project(FAC_CLONING_VATS, faction_id)) {
        // skipped
    } else if (has_creche || f->SE_growth >= GrowthPopBoom-4) {
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id) {
                want_pop += (pop_goal(i) - b->pop_size)
                    * (b->nutrient_surplus > 1 && has_facility(FAC_CHILDREN_CRECHE, i) ? 4 : 1);
                pop_total += b->pop_size;
            }
        }
        if (pop_total > 0) {
            pop_boom = ((f->SE_growth < GrowthPopBoom-2 ? 1 : 2) * want_pop) >= pop_total;
        }
    }
    debug("social_params %d %d %8s defense: %d creche: %d pop_boom: %d want_pop: %3d pop_total: %3d\n",
        *CurrentTurn, faction_id, m->filename, def_value, has_creche, pop_boom, want_pop, pop_total);
    int score_diff = 1 + (*CurrentTurn + 11*faction_id) % 6;
    int sf = -1;
    int sm2 = -1;
    CSocialCategory soc;

    for (int i = 0; i < MaxSocialCatNum; i++) {
        int sm1 = (&f->SE_Politics)[i];
        int sc1 = social_score(faction_id, i, sm1, pop_boom);
        for (int j = 0; j < MaxSocialModelNum; j++) {
            if (j == sm1 || !society_avail(i, j, faction_id)) {
                continue;
            }
            int sc2 = social_score(faction_id, i, j, pop_boom);
            if (sc2 - sc1 > score_diff) {
                sf = i;
                sm2 = j;
                score_diff = sc2 - sc1;
                memcpy(&soc, &f->SE_Politics, sizeof(soc));
                *(&soc.politics + sf) = sm2;
            }
        }
    }
    int cost;
    if (sf >= 0 && f->energy_credits > (cost = social_upheaval(faction_id, &soc))) {
        int sm1 = (&f->SE_Politics)[sf];
        (&f->SE_Politics_pending)[sf] = sm2;
        f->energy_credits -= cost;
        f->SE_upheaval_cost_paid += cost;
        debug("social_change %d %d %8s cost: %d score: %d %s -> %s\n",
            *CurrentTurn, faction_id, m->filename,
            cost, score_diff, SocialField[sf].soc_name[sm1], SocialField[sf].soc_name[sm2]);
    }
    social_set(faction_id);
    design_units(faction_id);
    consider_designs(faction_id);
    return 0;
}

/*
Determine if the specified faction wants to attack the target faction.
Lower values of modifier will make the faction more likely to attack.
*/
static int __cdecl evaluate_attack(int faction_id, int faction_id_tgt, int faction_id_unk) {
    Faction* plr = &Factions[faction_id];
    Faction* plr_tgt = &Factions[faction_id_tgt];
    int32_t peace_faction_id = 0;
    bool common_enemy = false; // Target faction is at war with a human faction we are allied with
    if (MFactions[faction_id].is_alien() && MFactions[faction_id_tgt].is_alien()) {
        return true;
    }
    if (has_treaty(faction_id, faction_id_tgt, DIPLO_WANT_REVENGE | DIPLO_UNK_40 | DIPLO_ATROCITY_VICTIM)) {
        return true;
    }
    if (plr->major_atrocities && plr_tgt->major_atrocities) {
        return true;
    }
    if (has_treaty(faction_id, faction_id_tgt, DIPLO_UNK_4000000)) {
        return false;
    }
    if (!is_human(faction_id_tgt) && plr->player_flags & PFLAG_TEAM_UP_VS_HUMAN) {
        return false;
    }
    int32_t modifier = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id && i != faction_id_tgt) {
            bool has_surrender = has_treaty(faction_id, i, DIPLO_HAVE_SURRENDERED);
            if (has_surrender && has_treaty(faction_id, i, DIPLO_PACT)) {
                peace_faction_id = i;
            }
            if (has_treaty(faction_id, i, DIPLO_VENDETTA)
            && !has_treaty(faction_id_tgt, i, DIPLO_PACT)) {
                modifier++;
                if (Factions[i].mil_strength_1 > ((plr_tgt->mil_strength_1 * 3) / 2)) {
                    modifier++;
                }
            }
            if (great_beelzebub(i, false)
            && (*CurrentTurn >= 100 || *GameRules & RULES_INTENSE_RIVALRY)) {
                if (has_treaty(faction_id_tgt, i, DIPLO_VENDETTA)) {
                    modifier++;
                }
                if (has_treaty(faction_id_tgt, i, DIPLO_COMMLINK)
                && has_treaty(faction_id, i, DIPLO_COMMLINK)) {
                    modifier++;
                }
            }
            if (has_treaty(faction_id, i, DIPLO_PACT) && is_human(i)) {
                if (has_treaty(faction_id_tgt, i, DIPLO_PACT)) {
                    modifier += 2;
                }
                if (has_surrender && has_treaty(i, faction_id_tgt, DIPLO_PACT | DIPLO_TREATY)) {
                    return false;
                }
                if (has_treaty(faction_id_tgt, i, DIPLO_VENDETTA)) {
                    common_enemy = true;
                    modifier -= (has_surrender ? 4 : 2);
                }
            }
        }
    }
    if (peace_faction_id) {
        if (has_treaty(faction_id_tgt, peace_faction_id, DIPLO_VENDETTA)) {
            return true;
        }
        if (has_treaty(faction_id_tgt, peace_faction_id, DIPLO_PACT | DIPLO_TREATY)) {
            return false;
        }
    }
    if (plr->AI_fight < 0 && !common_enemy && FactionRankings[7] != faction_id_tgt) {
        return false;
    }
    // Modify attacks to be less likely when AI has only few bases
    int32_t score = plr->base_count
        + (plr->best_armor_value > 1 ? 5 : 0)
        + (plr->best_weapon_value > 1 ? 5 : 0);
    if (score <= ((*CurrentTurn + faction_id) & 15)) {
        return false;
    }
    // Fix: initialize array to zero, original doesn't and compares arbitrary data on stack
    uint32_t region_top_base_count[MaxPlayerNum] = {};
    for (int region = 1; region < MaxRegionLandNum; region++) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            uint32_t total_bases = Factions[i].region_total_bases[region];
            if (total_bases > region_top_base_count[i]) {
                region_top_base_count[i] = total_bases;
            }
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        region_top_base_count[i] -= (region_top_base_count[i] / 4);
    }
    int32_t region_target_hq = -1;
    int32_t region_hq = -1;
    for (int i = 0; i < *BaseCount; i++) {
        if (has_fac_built(FAC_HEADQUARTERS, i)) {
            if (Bases[i].faction_id == faction_id) {
                region_hq = region_at(Bases[i].x, Bases[i].y);
            } else if (Bases[i].faction_id == faction_id_tgt) {
                region_target_hq = region_at(Bases[i].x, Bases[i].y);
            }
        }
    }
    int32_t factor_force_rating = 0;
    int32_t factor_count = 0;
    int32_t factor_unk = 1;
    for (int region = 1; region < MaxRegionLandNum; region++) {
        uint32_t force_rating = plr->region_force_rating[region];
        if (!bad_reg(region) && force_rating) {
            uint32_t total_cmbt_vehs = plr_tgt->region_total_combat_units[region];
            uint32_t total_bases_tgt = plr_tgt->region_total_bases[region];

            if (total_cmbt_vehs || total_bases_tgt) {
                if (plr->region_total_bases[region]
                >= ((region_top_base_count[faction_id] / 4) * 3) || region == region_hq) {
                    int32_t compare = force_rating +
                        plr->region_total_combat_units[region] +
                        (faction_id_unk > 0
                            ? Factions[faction_id_unk].region_force_rating[region] / 4 : 0);
                    if (plr_tgt->region_force_rating[region] > compare) {
                        return false;
                    }
                }
                if (total_bases_tgt) {
                    factor_force_rating += force_rating + (faction_id_unk > 0
                        ? Factions[faction_id_unk].region_force_rating[region] / 2 : 0);
                }
                if ((total_bases_tgt >= ((region_top_base_count[faction_id_tgt] / 4) * 3)
                || region == region_target_hq) && force_rating > total_cmbt_vehs) {
                    factor_force_rating += force_rating + (faction_id_unk > 0
                        ? Factions[faction_id_unk].region_force_rating[region] / 2 : 0);
                }
                factor_unk += total_cmbt_vehs + (plr->region_total_bases[region]
                    ? plr_tgt->region_force_rating[region] / 2 : 0);
                if (plr->region_total_bases[region]) {
                    factor_count++;
                }
            }
        }
    }
    modifier -= plr->AI_fight * 2;
    if (plr->tech_commerce_bonus > ((plr_tgt->tech_commerce_bonus * 3) / 2)) {
        modifier++;
    }
    if (plr->tech_commerce_bonus < ((plr_tgt->tech_commerce_bonus * 2) / 3)) {
        modifier--;
    }
    if (plr->best_weapon_value > (plr_tgt->best_armor_value * 2)) {
        modifier--;
    }
    if (plr->best_weapon_value <= plr_tgt->best_armor_value) {
        modifier++;
    }
    if (!has_treaty(faction_id, faction_id_tgt, DIPLO_VENDETTA)) {
        modifier++;
    }
    if (!has_treaty(faction_id, faction_id_tgt, DIPLO_PACT)) {
        modifier++;
    }
    if (faction_id_unk > 0 && !great_satan(faction_id_unk, false)) {
        modifier--;
    }
    if (has_agenda(faction_id, faction_id_tgt, AGENDA_UNK_200)
    && *GameRules & RULES_INTENSE_RIVALRY) {
        modifier--;
    }
    modifier -= clamp((plr_tgt->integrity_blemishes
        - plr->integrity_blemishes + 2) / 3, 0, 2);
    int32_t morale_factor = clamp(plr->SE_morale_pending, -4, 4)
        + MFactions[faction_id].rule_morale + 16;
    int32_t morale_divisor = factor_unk * (clamp(plr_tgt->SE_morale_pending, -4, 4)
        + MFactions[faction_id_tgt].rule_morale + 16);

    if (!morale_divisor) {
        assert(0);
        return false;
    }
    if ((factor_count || modifier > 0 || has_treaty(faction_id, faction_id_tgt, DIPLO_UNK_20000000))
    && ((morale_factor * factor_force_rating * 6) / morale_divisor) < (modifier + 6)) {
        return false;
    }
    return true;
}

int __cdecl mod_wants_to_attack(int faction_id, int faction_id_tgt, int faction_id_unk) {
    int value = evaluate_attack(faction_id, faction_id_tgt, faction_id_unk);

    debug("wants_to_attack turn: %d factions: %d %d %d value: %d\n",
        *CurrentTurn, faction_id, faction_id_tgt, faction_id_unk, value);
    return value;
}

static int veh_init_free(int unit_id, int faction_id, int x, int y) {
    int veh_id = veh_init(unit_id, faction_id, x, y);
    if (veh_id >= 0) {
        Vehs[veh_id].home_base_id = -1;
        if (thinker_enabled(faction_id)) {
            Vehs[veh_id].state |= VSTATE_UNK_40000;
            Vehs[veh_id].state &= ~VSTATE_UNK_2000;
        }
    }
    return veh_id;
}

static int veh_init_last(int unit_id, int faction_id, int x, int y) {
    int veh_id = veh_init_free(unit_id, faction_id, x, y);
    if (veh_id >= 0) {
        veh_demote(veh_id);
    }
    return veh_id;
}

static void veh_init_stack(int unit_id, int faction_id, int x, int y, int num) {
    for (int i = 0; i < num; i++) {
        veh_init_last(unit_id, faction_id, x, y);
    }
}

static int __cdecl setup_values(int faction_id, int a2, int a3) {
    Faction* f = &Factions[faction_id];
    int value = setup_player(faction_id, a2, a3);
    if (!faction_id) {
        return value;
    }
    int num_colony = is_human(faction_id) ? conf.player_colony_pods : conf.computer_colony_pods;
    int num_former = is_human(faction_id) ? conf.player_formers : conf.computer_formers;

    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id) {
            MAP* sq = mapsq(veh->x, veh->y);
            int colony = (is_ocean(sq) ? BSC_SEA_ESCAPE_POD : BSC_COLONY_POD);
            int former = (is_ocean(sq) ? BSC_SEA_FORMERS : BSC_FORMERS);
            for (int j = 0; j < num_colony; j++) {
                veh_init_free(colony, faction_id, veh->x, veh->y);
            }
            for (int j = 0; j < num_former; j++) {
                veh_init_free(former, faction_id, veh->x, veh->y);
            }
            break;
        }
    }
    if (is_human(faction_id)) {
        f->satellites_nutrient = conf.player_satellites[0];
        f->satellites_mineral = conf.player_satellites[1];
        f->satellites_energy = conf.player_satellites[2];
    } else {
        f->satellites_nutrient = conf.computer_satellites[0];
        f->satellites_mineral = conf.computer_satellites[1];
        f->satellites_energy = conf.computer_satellites[2];
    }
    if (!*CurrentTurn) {
        // Update default governor settings
        f->base_governor_adv &= ~(GOV_MAY_PROD_SP|GOV_MAY_HURRY_PRODUCTION);
        f->base_governor_adv |= GOV_MAY_PROD_NATIVE;
    }
    return value;
}

static bool has_suffix(const char* str) {
    int len = strlen(str);
    return len >= 2 && str[len-2] == '-' && isdigit(str[len-1]);
}

int __cdecl mod_setup_player(int faction_id, int setup_id, int is_probe) {
    Faction* plr = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    const bool special_spawn = setup_id == -282;
    const bool initial_spawn = !*CurrentTurn || special_spawn;
    const bool is_player = faction_id == *CurrentPlayerFaction;
    const bool plr_alien = m->is_alien();
    const bool plr_aquatic = m->is_aquatic();
    debug("setup_player %d %d %d %d %s\n", *CurrentTurn, faction_id, setup_id, is_probe, m->filename);

    if (!conf.faction_placement) {
        return setup_values(faction_id, setup_id, is_probe);
    }
    // Modify the original version to check by filename if multiple similar factions are active.
    // If any duplicates are found, each one will have their faction_id as name suffix.
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id && initial_spawn && i != faction_id
        && !_stricmp(MFactions[i].filename, m->filename)) {
            char buf[StrBufLen];
            if (!has_suffix(m->name_leader)) {
                strcpy_n(buf, 22, m->name_leader);
                snprintf(m->name_leader, 24, "%s-%d", buf, faction_id);
            }
            if (!has_suffix(m->noun_faction)) {
                strcpy_n(buf, 22, m->noun_faction);
                snprintf(m->noun_faction, 24, "%s-%d", buf, faction_id);
            }
            if (!has_suffix(m->formal_name_faction)) {
                strcpy_n(buf, 38, m->formal_name_faction);
                snprintf(m->formal_name_faction, 40, "%s-%d", buf, faction_id);
            }
            break;
        }
    }
    for (int i = *BaseCount - 1; i >= 0; --i) {
        if (Bases[i].faction_id == faction_id) {
            base_kill(i); // Skip find_relocate_base check
        }
    }
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id && (faction_id || veh->unit_id != BSC_FUNGAL_TOWER)) {
            kill(i);
        }
    }
    plr->target_y = -1;
    plr->target_x = -1;
    plr->base_id_attack_target = -1;
    if (initial_spawn) {
        clear_goals(faction_id);
        clear_bunglist(faction_id);
        plr->player_flags = 0;
        plr->base_governor_adv = (GOV_MAY_PROD_TRANSPORT|GOV_MAY_PROD_EXPLORE_VEH|GOV_MAY_PROD_PROBES|\
            GOV_MAY_PROD_PROTOTYPE|GOV_MAY_PROD_SP|GOV_MAY_PROD_COLONY_POD|GOV_MAY_PROD_FACILITIES|\
            GOV_MAY_PROD_TERRAFORMERS|GOV_MAY_PROD_AIR_DEFENSE|GOV_MAY_PROD_LAND_DEFENSE|GOV_MAY_PROD_AIR_COMBAT|\
            GOV_MAY_PROD_NAVAL_COMBAT|GOV_MAY_PROD_LAND_COMBAT|GOV_MANAGE_CITIZENS|GOV_MANAGE_PRODUCTION);
        if (!*DiffLevel) {
            plr->base_governor_adv |= (GOV_ACTIVE|GOV_UNK_40000000);
        }
        plr->base_name_offset = 0;
        plr->base_sea_name_offset = 0;
        plr->AI_fight = m->AI_fight;
        plr->AI_power = m->AI_power;
        plr->AI_tech = m->AI_tech;
        plr->AI_wealth = m->AI_wealth;
        plr->AI_growth = m->AI_growth;
        plr->unk_37 = 0;
        for (int i = 0; i < 8; i++) {
            plr->saved_queue_size[i] = 0;
            snprintf(&plr->saved_queue_name[i][0], 24, "%s %d", label_get(609), i+1); // Template
        }
        memset(&plr->SE_Politics_pending, 0, 32u); // pending / current
        memset(&plr->SE_economy_pending, 0, 44u); // only pending
        memset(&plr->secret_project_intel, 0, 32u);
        plr->SE_upheaval_cost_paid = 0;
        plr->diff_level = is_human(faction_id) ? *DiffLevel : 3;
        plr->integrity_blemishes = 0;
        plr->diplo_reputation = 0;
        plr->traded_maps = 0;
        plr->energy_credits = 0;
        plr->atrocities = 0;
        plr->major_atrocities = 0;
        plr->clean_minerals_modifier = 0;
        plr->sanction_turns = 0;
        plr->council_call_turn = -999;
        TectonicDetonationCount[faction_id] = 0;
        // Fix issue with randomized faction agendas where they might be given agendas that are
        // their opposition social models making the choice unusable. Future social models can be
        // also selected but much less often. Additionally randomized leader personalities
        // always selects at least one AI priority.
        if (*GameState & STATE_RAND_FAC_LEADER_SOCIAL_AGENDA) {
            for (int i = 0; i < 1000; i++) {
                int val = game_randv(4) ? 3 : 4;
                int sfield = game_randv(val);
                int smodel = game_randv(3) + 1;
                if (SocialField[sfield].soc_preq_tech[smodel] == TECH_Disable
                || (sfield == m->soc_opposition_category && smodel == m->soc_opposition_model)) {
                    continue;
                }
                bool valid = true;
                for (int j = 1; j < MaxPlayerNum; j++) {
                    if (faction_id != j && is_alive(j)
                    && MFactions[j].soc_priority_category == sfield
                    && MFactions[j].soc_priority_model == smodel) {
                        valid = false;
                    }
                }
                if (valid) {
                    m->soc_priority_category = sfield;
                    m->soc_priority_model = smodel;
                    debug("setup_player_agenda %s %d %d\n",  m->filename, sfield, smodel);
                    break;
                }
            }
        }
        if (*GameState & STATE_RAND_FAC_LEADER_PERSONALITIES) {
            plr->AI_fight = game_randv(3) - 1;
            int val = 0;
            while (!val) {
                val = game_randv(4096);
                val &= (val >> 8) & (val >> 4);
                if (val & 1) plr->AI_growth = 1;
                if (val & 2) plr->AI_tech = 1;
                if (val & 4) plr->AI_wealth = 1;
                if (val & 8) plr->AI_power = 1;
            }
        }
        if (*GameRules & RULES_INTENSE_RIVALRY) {
            plr->AI_fight = 1;
        }
        plr->total_combat_units = 0;
        plr->base_count = 0;
        plr->mil_strength_1 = 0;
        plr->mil_strength_2 = 0;
        plr->pop_total = 0;
        plr->unk_70 = 0;
        plr->mind_control_total = 0;
        memset(&plr->diplo_mind_control, 0, 32u);
        memset(&plr->diplo_stolen_techs, 0, 32u);
        plr->net_random_event = 0;
        for (int i = 0; i < MaxRegionNum; i++) {
            plr->region_total_combat_units[i] = 0;
            plr->region_total_bases[i] = 0;
            plr->region_total_offensive_units[i] = 0;
            plr->region_force_rating[i] = 0;
            plr->region_flags[i] = 0;
        }
        memset(&plr->unk_29, 0, 44u);
        memset(&plr->unk_30, 0, 44u);
        memset(&plr->corner_market_turn, 0, 40u);
        plr->unk_93 = 0;
        plr->goody_opened = 0;
        plr->goody_artifact = 0;
        plr->goody_earthquake = 0;
        plr->goody_tech = 0;
        plr->tech_achieved = 0;
        plr->time_bonus_count = 1;
    }
    plr->energy_credits += (*MultiplayerActive ? 100 : Rules->starting_energy_reserve) + m->rule_energy;
    plr->hurry_cost_total = 0;
    plr->tech_cost = -1;
    plr->last_base_turn = *CurrentTurn;
    for (int i = 0; i < MaxPlayerNum; i++) {
        if (!initial_spawn) {
            treaty_off(faction_id, i,
                DIPLO_UNK_80000000|DIPLO_UNK_10000000|DIPLO_UNK_8000|DIPLO_UNK_4000|DIPLO_WANT_TO_TALK|\
                DIPLO_HAVE_INFILTRATOR|DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_UNK_200|DIPLO_UNK_100|DIPLO_UNK_40|DIPLO_COMMLINK);
        } else {
            plr->diplo_status[i] = 0; // seems not to reset diplo_agenda?
            plr->diplo_spoke[i] = -1;
            plr->diplo_merc[i] = 0;
            plr->diplo_patience[i] = 0;
            int val = 3 * (*GameRules & RULES_INTENSE_RIVALRY ? 5 : *DiffLevel) + 8;
            plr->diplo_friction[i] = clamp(game_randv(val), 1, 16);
            // Fix: the game may not properly reset all these fields.
            plr->diplo_gifts[i] = 0;
            plr->diplo_wrongs[i] = 0;
            plr->diplo_betrayed[i] = 0;
            plr->diplo_unk_3[i] = 0;
            plr->diplo_unk_4[i] = 0;
        }
        plr->loan_balance[i] = 0;
        plr->loan_payment[i] = 0;
        plr->unk_1[i] = 0;
    }
    for (int i = 0; i < MaxProtoNum; i++) {
        plr->units_active[i] = 0;
        plr->units_queue[i] = 0;
        if (!*CurrentTurn) {
            plr->units_lost[i] = 0;
        }
    }
    if (initial_spawn) {
        memset(&plr->tech_pact_shared_goals, 0, 48u);
        memset(&plr->tech_trade_source, 0, 88u);
        plr->unk_27 = 0;
        plr->unk_28 = 0;
        plr->unk_33 = 0;
        plr->facility_announced[0] = 0;
        plr->facility_announced[1] = 0;
        plr->satellites_nutrient = 0;
        plr->satellites_mineral = 0;
        plr->satellites_energy = 0;
        plr->satellites_ODP = 0;
        plr->ODP_deployed = 0;
        plr->tech_count_transcendent = 0;
        plr->tech_ranking = 0;
        plr->tech_research_id = -1;
        plr->tech_accumulated = 0;
        plr->earned_techs_saved = 0;
        plr->unk_102 = 0;
        if (faction_id) {
            if (is_human(faction_id)) {
                plr->satellites_nutrient = conf.player_satellites[0];
                plr->satellites_mineral = conf.player_satellites[1];
                plr->satellites_energy = conf.player_satellites[2];
            } else {
                plr->satellites_nutrient = conf.computer_satellites[0];
                plr->satellites_mineral = conf.computer_satellites[1];
                plr->satellites_energy = conf.computer_satellites[2];
            }
            // Update default governor settings
            plr->base_governor_adv &= ~(GOV_MAY_PROD_SP|GOV_MAY_HURRY_PRODUCTION);
            plr->base_governor_adv |= GOV_MAY_PROD_NATIVE;
        }
    }
    plr->unk_17 = 0;
    plr->unk_18 = 0;
    plr->best_mineral_output = 0;
    plr->energy_surplus_total = 0;
    plr->facility_maint_total = 0;
    plr->tech_commerce_bonus = 0;
    plr->tech_fungus_nutrient = 0;
    plr->tech_fungus_mineral = 0;
    plr->tech_fungus_energy = 0;
    plr->tech_fungus_unused = 0;
    plr->turn_commerce_income = 0;
    plr->best_weapon_value = 0;
    plr->best_armor_value = 0;
    plr->best_land_speed = 0;
    plr->best_psi_land_offense = 0;
    plr->best_psi_land_defense = 0;
    plr->enemy_best_weapon_value = 0;
    plr->enemy_best_armor_value = 0;
    plr->enemy_best_land_speed = 0;
    plr->enemy_best_psi_land_offense = 0;
    plr->enemy_best_psi_land_defense = 0;
    if (!faction_id) {
        return 0;
    }
    if (!is_alive(faction_id)) {
        return 0;
    }
    if (setup_id < -1 && !special_spawn) {
        set_alive(faction_id, false);
        return 0;
    }
    bool spawn_skip = *CurrentTurn > 100 || (!initial_spawn && *GameRules & RULES_DO_OR_DIE);
    bool spawn_flag = !is_probe && (plr->player_flags & PFLAG_UNK_1000);
    MAP* sq = NULL;
    int region = 0;
    int x = -1;
    int y = -1;
    // This replaces the entire old faction placement method
    if (is_probe || !spawn_flag) {
        find_start(faction_id, &x, &y);
        if (!(sq = mapsq(x, y))) {
            // skipped
        } else {
            region = sq->region;
        }
    }
    if (!sq || (!is_probe && (spawn_flag || spawn_skip))) {
        if (!is_human(faction_id) || *MultiplayerActive) {
            plr->unk_102 = 0;
            set_alive(faction_id, false);
            int other_id = FactionRankings[7];
            if (faction_id == other_id) {
                other_id = FactionRankings[6];
            }
            int num_active = 0;
            int num_allied = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (is_alive(i)) {
                    num_active++;
                    if (!*MultiplayerActive && Factions[i].diplo_status[*CurrentPlayerFaction] & DIPLO_PACT
                    && Factions[i].diplo_status[*CurrentPlayerFaction] & DIPLO_HAVE_SURRENDERED) {
                        num_active--;
                    } else if (i != other_id && Factions[i].diplo_status[other_id] & DIPLO_PACT
                    && *GameRules & RULES_VICTORY_COOPERATIVE && num_allied < 2) {
                        num_allied++;
                    }
                }
            }
            if (is_player) {
                int k = (setup_id < 0 ? 0 : setup_id);
                *gender_default = MFactions[k].noun_gender;
                *plurality_default = MFactions[k].is_noun_plural;
                parse_says(0, &MFactions[k].noun_faction[0], -1, -1);
                if (*MultiplayerActive) {
                    net_game_close();
                    *ControlTurnA = 1;
                }
                if (setup_id || *GameLanguage) {
                    X_pop2("YOULOSE", 0);
                } else {
                    X_pop2("YOULOSE2", 0);
                }
                *GameState |= STATE_GAME_DONE;
                *dword_9B206C = setup_id ? 7 : 15; // TODO investigate values
            }
            if (!is_player) {
                if (num_active - num_allied < 2) {
                    if (num_allied) {
                        popp(ScriptFile, "CONQCOOP", 0, "conq_sm.pcx", 0);
                        if (!*dword_9B206C) {
                            *dword_9B206C = 5;
                        }
                    } else if (other_id == *CurrentPlayerFaction) {
                        popp(ScriptFile, "CONQSING", 0, "conq_sm.pcx", 0);
                        if (!*dword_9B206C) {
                            *dword_9B206C = 4;
                        }
                    }
                    *GameState |= (STATE_GAME_DONE | STATE_VICTORY_CONQUER);
                }
            }
        } else {
            *dword_9B206C = 7;
            *GameState |= STATE_GAME_DONE;
            set_alive(faction_id, false);
        }
        return 0; // Spawn failed, always return zero
    } else {
        assert(sq && sq->anything_at() < 0);
        assert(region > 0);
        plr->SE_alloc_labs = 5;
        plr->SE_alloc_psych = 0;
        if (initial_spawn && *DiffLevel >= 2) {
            if (plr->AI_wealth || plr->AI_tech) {
                if (!game_randv(4)) {
                    plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_PROBE_TEAMS;
                }
            }
            if (!_stricmp(m->filename, "ANGELS")) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_PROBE_TEAMS;
            }
            if (plr_aquatic) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_SEA_BASES;
                plr->player_flags |= PFLAG_EMPHASIZE_SEA_POWER;
            }
            if (plr_alien) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_MISSILES;
                plr->player_flags |= PFLAG_EMPHASIZE_AIR_POWER;
            }
            if (!_stricmp(m->filename, "DRONE")) {
                plr->player_flags_ext &= ~PFLAG_EXT_STRAT_LOTS_PROBE_TEAMS;
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_TERRAFORMERS;
            }
            if (Continents[region].tile_count < 250 && !game_randv(3)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_SEA_BASES;
            }
            if (plr->AI_fight >= 0 && !game_randv(4)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_MISSILES;
            }
            if (!game_randv(6)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_TERRAFORMERS;
            }
            if (!game_randv(6)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_COLONY_PODS;
            }
            if (!game_randv(5)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_ARTILLERY;
            }
            if ((*GameRules & RULES_INTENSE_RIVALRY || *DiffLevel >= 4)
            && plr->AI_fight > -((*GameRules & RULES_INTENSE_RIVALRY) != 0)) {
                if (!game_randv(4)) {
                    plr->player_flags_ext |= PFLAG_EXT_SHAMELESS_BETRAY_HUMANS;
                }
                if (!game_randv(4)) {
                    plr->player_flags |= PFLAG_COMMIT_ATROCITIES_WANTONLY;
                    if (!game_randv(2)) {
                        plr->player_flags |= PFLAG_OBLITERATE_CAPTURED_BASES;
                    }
                }
            }
            if (plr->AI_power || plr->AI_fight > 0) {
                if (Continents[region].tile_count >= 400 && !game_randv(4)) {
                    plr->player_flags |= PFLAG_EMPHASIZE_LAND_POWER;
                }
                int val = Continents[region].tile_count > 250 ? 6 : 4;
                if (!game_randv(val)) {
                    plr->player_flags |= PFLAG_EMPHASIZE_SEA_POWER;
                }
                if (!game_randv(4)) {
                    plr->player_flags |= PFLAG_EMPHASIZE_AIR_POWER;
                }
            }
        }
        for (auto& p : iterate_tiles(x, y, 0, 21)) {
            p.sq->visibility |= (1 << faction_id);
            synch_bit(p.x, p.y, faction_id);
        }
        int recon_unit_id = BSC_SCOUT_PATROL;
        if (initial_spawn) {
            int bonus_count = m->faction_bonus_count;
            for (int i = 0; i < bonus_count; i++) {
                if (m->faction_bonus_id[i] == RULE_TECH) {
                    tech_achieved(faction_id, m->faction_bonus_val1[i], 0, 0);
                }
            }
            if (*ExpansionEnabled && plr_alien && conf.alien_guaranteed_techs) {
                tech_achieved(faction_id, TECH_PrPsych, 0, 0);
                tech_achieved(faction_id, TECH_FldMod, 0, 0);
            }
            if (*MultiplayerActive) {
                for (int i = 0; ; ++i) {
                    int v1 = plr->SE_research_base <= 0 ? (plr->SE_research_base >= 0) - 1 : 1;
                    int v2 = plr->SE_morale_base <= 0 ? (plr->SE_morale_base >= 0) - 1 : 1;
                    if (i >= v1 - v2 + 1) {
                        break;
                    }
                    tech_achieved(faction_id, mod_tech_ai(faction_id), 0, 0);
                }
                plr->energy_credits += m->rule_energy / 2;
            }
            consider_designs(faction_id);
            for (int i = 0; i < MaxProtoFactionNum; i++) {
                int unit_id = i + faction_id * MaxProtoFactionNum;
                UNIT* u = &Units[unit_id];
                if (u->unit_flags & UNIT_ACTIVE) {
                    u->unit_flags |= UNIT_PROTOTYPED;
                    if (u->plan == PLAN_RECON) {
                        recon_unit_id = unit_id;
                    }
                }
            }
        }
        // First spawn additional units specified by config
        // These prototypes also use the same fixed choices as in the original game
        const int num_colony = is_human(faction_id) ? conf.player_colony_pods : conf.computer_colony_pods;
        const int num_former = is_human(faction_id) ? conf.player_formers : conf.computer_formers;
        const int colony_unit = plr_aquatic ? BSC_SEA_ESCAPE_POD : BSC_COLONY_POD;
        const int former_unit = plr_aquatic ? BSC_SEA_FORMERS : BSC_FORMERS;
        veh_init_stack(colony_unit, faction_id, x, y, num_colony);
        veh_init_stack(former_unit, faction_id, x, y, num_former);
        // For consistency these colony/former are always set as independent units
        // Colony pods spawned may change based on balance function (deprecated, skipped by default)
        if (!initial_spawn) {
            veh_init_free(colony_unit, faction_id, x, y);
            veh_init_last(former_unit, faction_id, x, y);
        } else {
            // Fix: original game always enabled Look First during MultiplayerActive if Time Warp is disabled
            if (*GameRules & RULES_LOOK_FIRST && !(*GameRules & RULES_TIME_WARP)) {
                veh_init_free(colony_unit, faction_id, x, y);
            } else {
                int base_id = base_init(faction_id, x, y);
                if (base_id >= 0 && !_stricmp(m->filename, "FUNGBOY") && special_spawn) {
                    Bases[base_id].pop_size = 3;
                }
                if (base_id >= 0 && plr_alien && special_spawn) {
                    Bases[base_id].pop_size = 3;
                }
            }
            if (!plr_aquatic) {
                // Original game only spawns an extra recon unit
                veh_init_stack(BSC_COLONY_POD, faction_id, x, y, conf.spawn_free_units[0]);
                veh_init_stack(BSC_FORMERS, faction_id, x, y, conf.spawn_free_units[1]);
                veh_init_stack(recon_unit_id, faction_id, x, y, conf.spawn_free_units[2]);
            }
            if (*MultiplayerActive && !plr_aquatic) {
                veh_init_last(BSC_UNITY_ROVER, faction_id, x, y);
                veh_init_last(BSC_FORMERS, faction_id, x, y);
            }
            int coast = is_coast(x, y, 0);
            if (coast && (*DiffLevel > 1 || *MultiplayerActive || Continents[region].tile_count <= 32)) {
                // Calculation for nearby opponents is slightly modified from the original to exclude native units
                int nearby = 0;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (Factions[i].region_total_bases[region]) {
                        nearby |= (1 << Bases[i].faction_id);
                    }
                }
                for (int i = 0; i < *VehCount; i++) {
                    if (Vehs[i].faction_id && region_at(Vehs[i].x, Vehs[i].y) == region) {
                        nearby |= (1 << Vehs[i].faction_id);
                    }
                }
                if ((*MultiplayerActive && is_human(faction_id)
                && Continents[region].tile_count / (__builtin_popcount(nearby) + 1) <= 75)
                || (Continents[region].tile_count / (__builtin_popcount(nearby) + 1) <= 50)) {
                    int x2 = wrap(x + TableOffsetX[coast]);
                    int y2 = y + TableOffsetY[coast];
                    int veh_id = veh_init(BSC_UNITY_FOIL, faction_id, x2, y2);
                    if (veh_id >= 0) {
                        Vehs[veh_id].home_base_id = -1;
                        Vehs[veh_id].morale = 2;
                        veh_demote(veh_id);
                    }
                }
            }
            memset(&plr->SE_economy_base, 0, 44u);
            int bonus_count = m->faction_bonus_count;
            for (int i = 0; i < bonus_count; i++) {
                if (m->faction_bonus_id[i] == RULE_SOCIAL) {
                    assert(m->faction_bonus_val1[i] >= 0 && m->faction_bonus_val1[i] < 11);
                    (&plr->SE_economy_base)[m->faction_bonus_val1[i]] += m->faction_bonus_val2[i];
                } else if (m->faction_bonus_id[i] == RULE_UNIT) {
                    veh_init_last(m->faction_bonus_val1[i], faction_id, x, y);
                    if (m->faction_bonus_val1[i] == BSC_COLONY_POD) {
                        plr->player_flags |= PFLAG_UNK_100;
                    }
                }
            }
            if (*ExpansionEnabled && plr_alien) {
                // Original game only spawns an extra colony pod and battle ogre
                veh_init_stack(BSC_COLONY_POD, faction_id, x, y, conf.spawn_free_units[6]);
                veh_init_stack(BSC_FORMERS, faction_id, x, y, conf.spawn_free_units[7]);
                veh_init_stack(BSC_BATTLE_OGRE_MK1, faction_id, x, y, conf.spawn_free_units[8]);
            }
            if (plr_aquatic) {
                if (!is_human(faction_id)) {
                    int unit_id = -1; // Fix: possibly uninitialized default value
                    for (int i = 0; i < MaxProtoFactionNum; i++) {
                        int k = i + faction_id * MaxProtoFactionNum;
                        if (!(Units[k].unit_flags & UNIT_ACTIVE)) {
                            unit_id = k;
                            break;
                        }
                    }
                    if (unit_id >= 0) {
                        char buf[StrBufLen] = {};
                        mod_make_proto(unit_id, CHS_FOIL, WPN_COLONY_MODULE, ARM_NO_ARMOR, ABL_NONE, REC_FISSION);
                        mod_name_proto(&buf[0], unit_id, faction_id,
                            CHS_FOIL, WPN_COLONY_MODULE, ARM_NO_ARMOR, ABL_NONE, REC_FISSION);
                        Units[unit_id].unit_flags |= UNIT_PROTOTYPED;
                        Units[unit_id].unit_flags &= ~UNIT_CUSTOM_NAME_SET;
                        strcpy_n(&Units[unit_id].name[0], MaxProtoNameLen, buf);
                    }
                }
                // Original game only spawns an extra colony pod and unity gunship
                veh_init_stack(BSC_SEA_ESCAPE_POD, faction_id, x, y, conf.spawn_free_units[3]);
                veh_init_stack(BSC_SEA_FORMERS, faction_id, x, y, conf.spawn_free_units[4]);
                veh_init_stack(BSC_UNITY_GUNSHIP, faction_id, x, y, conf.spawn_free_units[5]);
            }
        }
        if (!spawn_flag && !initial_spawn) {
            int spawn_id_a = BSC_SCOUT_PATROL; // Garrison unit
            int spawn_id_b = -1; // Mostly attacker unit
            int spawn_id_c = -1; // Fast recon unit
            int spawn_val_a = -1;
            int spawn_val_b = -1;
            int spawn_val_c = -1;
            for (int unit_id = 0; unit_id < MaxProtoNum; unit_id++) {
                UNIT* u = &Units[unit_id];
                if (mod_veh_avail(unit_id, faction_id, -1) && u->offense_value() != 0) {
                    if ((plr_aquatic && u->triad() == TRIAD_SEA) || (!plr_aquatic && u->triad() == TRIAD_LAND)) {
                        int v1 = 8 * armor_val(unit_id, faction_id) - weap_val(unit_id, faction_id);
                        if (v1 > spawn_val_a) {
                            spawn_val_a = v1;
                            spawn_id_a = unit_id;
                        }
                        int v2 = u->speed() + 2 * armor_val(unit_id, faction_id) + 4 * weap_val(unit_id, faction_id);
                        if (v2 > spawn_val_b) {
                            spawn_val_b = v2;
                            spawn_id_b = unit_id;
                        }
                        int v3 = 4 * u->speed() - weap_val(unit_id, faction_id) - armor_val(unit_id, faction_id);
                        if (v3 > spawn_val_c && u->speed() > 1) {
                            spawn_val_c = v3;
                            spawn_id_c = unit_id;
                        }
                    }
                }
            }
            int veh_id = veh_init(colony_unit, faction_id, x, y);
            veh_init(spawn_id_a, faction_id, x, y);
            if (*CurrentTurn > 20 && spawn_id_c >= 0) {
                veh_init(spawn_id_c, faction_id, x, y);
            }
            if (*CurrentTurn > 40 && spawn_id_b >= 0) {
                veh_init(spawn_id_b, faction_id, x, y);
            }
            if (*CurrentTurn > 60 && spawn_id_a >= 0) {
                veh_init(spawn_id_a, faction_id, x, y);
            }
            if (*CurrentTurn > 80 && spawn_id_b >= 0) {
                veh_init(spawn_id_b, faction_id, x, y);
            }
            if (*CurrentTurn > 100) {
                veh_init(colony_unit, faction_id, x, y);
            }
            veh_promote(veh_id);
            plr->unk_27 = 3 * plr->tech_ranking / 4;
        }
        if (faction_id == *CurrentPlayerFaction) {
            MapWin->iTileX = x;
            MapWin->iTileY = y;
            for (int i = 0; i < 32; i++) {
                MapWin->aiCursorPositionsX[i] = x;
                MapWin->aiCursorPositionsY[i] = y;
            }
        }
        return 1; // Valid spawn, return value non-zero
    }
}

int __cdecl mod_eliminate_player(int faction_id, int setup_id) {
    Faction* plr = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    const bool is_player = faction_id == *CurrentPlayerFaction;
    const bool plr_alien = m->is_alien();
    if (!faction_id) {
        return 0;
    }
    for (int i = *BaseCount - 1; i >= 0; --i) {
        if (Bases[i].faction_id == faction_id) {
            return 0;
        }
    }
    for (int i = *VehCount - 1; i >= 0; --i) {
        if (Vehs[i].faction_id == faction_id) {
            veh_kill(i); // does not call kill() instead?
        }
    }
    plr->base_count = 0;
    int setup_val = mod_setup_player(faction_id, setup_id, 0);
    if (!setup_val && setup_id > 0 && !(*GameState & (STATE_FINAL_SCORE_DONE|STATE_GAME_DONE))) {
        Factions[setup_id].eliminated_count++;
    }
    if (setup_val || !is_player) {
        if (setup_id >= 0) {
            if (setup_id > 0) {
                set_treaty(faction_id, setup_id, DIPLO_WANT_TO_TALK|DIPLO_WANT_REVENGE|DIPLO_VENDETTA, 1);
            }
        } else {
            setup_id = 0;
        }
        draw_map(1);
        *gender_default = m->noun_gender;
        *plurality_default = m->is_noun_plural;
        parse_says(0, m->noun_faction, -1, -1);
        *gender_default = MFactions[setup_id].noun_gender;
        *plurality_default = MFactions[setup_id].is_noun_plural;
        parse_says(1, MFactions[setup_id].noun_faction, -1, -1);
        *gender_default = m->is_leader_female;
        *plurality_default = 0;
        parse_says(2, m->title_leader, -1, -1);
        parse_says(3, m->name_leader, -1, -1);
        parse_says(4, (char*)get_his_her(faction_id, 0), -1, -1);
        char name[StrBufLen] = {};
        char file[StrBufLen] = {};
        snprintf(name, StrBufLen, "WIPEOUT%s%d",
            (setup_id && setup_id == *CurrentPlayerFaction) ? "YOU" : "",
            setup_id ? (setup_val != 0) : 2);
        if (setup_val) { // The faction has escaped in a colony pod
            snprintf(file, StrBufLen, "%s", plr_alien ? "al_pod_sm.pcx" : "escpod_sm.pcx");
        } else { // The faction has been eradicated
            snprintf(file, StrBufLen, "%s2.pcx", m->filename);
        }
        popp(ScriptFile, name, 0, file, 0);
        if (!setup_val) {
            mon_killed_faction(setup_id, faction_id);
        }
        if (!*MultiplayerActive && !setup_val && setup_id == *CurrentPlayerFaction
        && (*GamePreferences & PREF_AV_SECRET_PROJECT_MOVIES)) {
            mod_amovie_project(&m->filename[0]);
        }
        if (!*MultiplayerActive && setup_val && setup_id == *CurrentPlayerFaction) {
            // TRACKED event, escape location may be revealed by random chance
            bool infiltrate = Factions[setup_id].diplo_status[faction_id] & DIPLO_HAVE_INFILTRATOR;
            int veh_id;
            if (!game_randv(infiltrate != 0 ? 2 : 4)
            && (veh_id = veh_find(0, 0, faction_id, -1)) >= 0) {
                int x = Vehs[veh_id].x;
                int y = Vehs[veh_id].y;
                for (auto& p : iterate_tiles(x, y, 0, 9)) {
                    spot_loc(p.x, p.y, setup_id);
                }
                bool found = false;
                veh_id = veh_top(veh_id);
                while (veh_id >= 0) {
                    Vehs[veh_id].visibility = 0;
                    if (!found && Vehs[veh_id].is_colony()) {
                        Vehs[veh_id].visibility = (1 << setup_id);
                        found = true;
                    }
                    veh_id = Vehs[veh_id].next_veh_id_stack;
                }
                draw_tiles(x, y, 2);
                MapWin_set_center(MapWin, x, y, 1);
                parse_says(6, m->adj_name_faction, -1, -1);
                if (!plr_alien) {
                    popp(ScriptFile, "TRACKED", 0, "escpod_sm.pcx", 0);
                } else {
                    popp(ScriptFile, "TRACKED", 0, "al_pod_sm.pcx", 0);
                }
            }
        }
        if (faction_id != *CurrentPlayerFaction && plr_alien && MFactions[*CurrentPlayerFaction].is_alien()) {
            interlude(42, 0, 1, 0);
        }
    }
    return 1;
}

