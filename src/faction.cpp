
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
Determine if the specified faction is alive or whether they've been eliminated.
*/
bool is_alive(int faction_id) {
    assert(faction_id >= 0);
    return FactionStatus[1] & (1 << faction_id);
}

/*
Determine if the specified faction is a Progenitor faction (Caretakers / Usurpers).
*/
bool is_alien(int faction_id) {
    assert(faction_id >= 0);
    return *ExpansionEnabled && MFactions[faction_id].rule_flags & RFLAG_ALIEN;
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
            VEH* veh = &Vehicles[i];
            if (veh->faction_id == faction_id && veh->is_colony()) {
                pods++;
            }
        }
        return Factions[faction_id].base_count + pods < max(bases, conf.expansion_limit);
    }
    return true;
}

bool has_transport(int x, int y, int faction_id) {
    assert(valid_player(faction_id));
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction_id && veh->x == x && veh->y == y
        && veh->is_transport()) {
            return true;
        }
    }
    return false;
}

bool has_defenders(int x, int y, int faction_id) {
    assert(valid_player(faction_id));
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction_id && veh->x == x && veh->y == y
        && (veh->is_combat_unit() || veh->is_armored())
        && veh->triad() == TRIAD_LAND) {
            return true;
        }
    }
    return false;
}

bool has_active_veh(int faction_id, VehPlan plan) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
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

void __cdecl set_treaty(int faction1, int faction2, uint32_t treaty, bool add) {
    if (add) {
        Factions[faction1].diplo_status[faction2] |= treaty;
        if (treaty & DIPLO_UNK_40) {
            Factions[faction1].diplo_merc[faction2] = 50;
        }
        if (treaty & DIPLO_HAVE_INFILTRATOR && conf.counter_espionage) {
            int turns = probe_active_turns(faction1, faction2);
            MFactions[faction1].thinker_probe_end_turn[faction2] = *CurrentTurn + turns;
            Factions[faction1].diplo_status[faction2] |= DIPLO_RENEW_INFILTRATOR;
            if (faction1 == MapWin->cOwner) {
                ParseNumTable[0] = turns;
                parse_says(0, parse_set(faction2), -1, -1);
                NetMsg_pop(NetMsg, "SPYRENEW", 5000, 0, 0);
            }
        }
    } else {
        Factions[faction1].diplo_status[faction2] &= ~treaty;
    }
}

void __cdecl set_agenda(int faction1, int faction2, uint32_t agenda, bool add) {
    if (add) {
        Factions[faction1].diplo_agenda[faction2] |= agenda;
    } else {
        Factions[faction1].diplo_agenda[faction2] &= ~agenda;
    }
}

int __cdecl has_treaty(int faction1, int faction2, uint32_t treaty) {
    assert(faction1 >= 0 && faction1 < MaxPlayerNum);
    assert(faction2 >= 0 && faction2 < MaxPlayerNum);
    return Factions[faction1].diplo_status[faction2] & treaty;
}

int __cdecl has_agenda(int faction1, int faction2, uint32_t agenda) {
    assert(faction1 >= 0 && faction1 < MaxPlayerNum);
    assert(faction2 >= 0 && faction2 < MaxPlayerNum);
    return Factions[faction1].diplo_agenda[faction2] & agenda;
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
Return Value: Bad reputation
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

int __cdecl mod_setup_player(int faction_id, int a2, int a3) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    debug("setup_player %d %d %d %d\n", *CurrentTurn, faction_id, a2, a3);
    if (!faction_id) {
        return setup_player(faction_id, a2, a3);
    }
    /*
    Fix issue with randomized faction agendas where they might be given agendas
    that are their opposition social models.
    */
    if (*GameState & STATE_RAND_FAC_LEADER_SOCIAL_AGENDA && !*CurrentTurn) {
        for (int i = 0; i < 1000; i++) {
            int sfield = random(3);
            int smodel = random(3) + 1;
            if (sfield == m->soc_opposition_category && smodel == m->soc_opposition_model) {
                continue;
            }
            for (int j = 1; j < MaxPlayerNum; j++) {
                if (faction_id != j && is_alive(j)) {
                    if (MFactions[j].soc_priority_category == sfield
                    && MFactions[j].soc_priority_model == smodel) {
                        continue;
                    }
                }
            }
            m->soc_priority_category = sfield;
            m->soc_priority_model = smodel;
            debug("setup_player_agenda %s %d %d\n", m->filename, sfield, smodel);
            break;
        }
    }
    if (*GameState & STATE_RAND_FAC_LEADER_PERSONALITIES && !*CurrentTurn) {
        m->AI_fight = random(3) - 1;
        m->AI_tech = 0;
        m->AI_power = 0;
        m->AI_growth = 0;
        m->AI_wealth = 0;

        for (int i = 0; i < 2; i++) {
            int val = random(4);
            switch (val) {
                case 0: m->AI_tech = 1; break;
                case 1: m->AI_power = 1; break;
                case 2: m->AI_growth = 1; break;
                case 3: m->AI_wealth = 1; break;
            }
        }
    }
    setup_player(faction_id, a2, a3);

    int num_colony = is_human(faction_id) ? conf.player_colony_pods : conf.computer_colony_pods;
    int num_former = is_human(faction_id) ? conf.player_formers : conf.computer_formers;

    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id == faction_id) {
            MAP* sq = mapsq(veh->x, veh->y);
            int colony = (is_ocean(sq) ? BSC_SEA_ESCAPE_POD : BSC_COLONY_POD);
            int former = (is_ocean(sq) ? BSC_SEA_FORMERS : BSC_FORMERS);
            for (int j = 0; j < num_colony; j++) {
                mod_veh_init(colony, faction_id, veh->x, veh->y);
            }
            for (int j = 0; j < num_former; j++) {
                mod_veh_init(former, faction_id, veh->x, veh->y);
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
    return 0;
}

/*
Improved social engineering AI choices feature.
*/
int social_score(int faction_id, int sf, int sm, int def_value, bool pop_boom, bool has_nexus,
int robust, int immunity, int impunity, int penalty) {
    enum {ECO, EFF, SUP, TAL, MOR, POL, GRW, PLA, PRO, IND, RES};
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    int base_ratio = min(10, 10 * f->base_count / min(40, *MapAreaSqRoot / 2));
    int w_morale = (has_project(FAC_COMMAND_NEXUS, faction_id) ? 2 : 0)
        + (has_project(FAC_CYBORG_FACTORY, faction_id) ? 2 : 0);
    int w_probe = (*CurrentTurn - m->thinker_last_mc_turn < 10 ? def_value + 1 : 0);
    int sc = 0;
    int vals[MaxSocialEffectNum];

    if ((&f->SE_Politics)[sf] == sm) {
        /* Evaluate the current active social model. */
        memcpy(vals, &f->SE_economy, sizeof(vals));
    } else {
        /* Take the faction base social values and apply all modifiers. */
        memcpy(vals, &f->SE_economy_base, sizeof(vals));

        for (int i = 0; i < MaxSocialCatNum; i++) {
            int j = (sf == i ? sm : (&f->SE_Politics)[i]);
            for (int k = 0; k < MaxSocialEffectNum; k++) {
                int val = ((int32_t*)&SocialField[i].soc_effect[j])[k];
                if ((1 << (i*4 + j)) & impunity) {
                    val = max(0, val);
                } else if ((1 << (i*4 + j)) & penalty) {
                    val = val * (val < 0 ? 2 : 1);
                }
                vals[k] += val;
            }
        }
        if (has_project(FAC_ASCETIC_VIRTUES, faction_id)) {
            vals[POL] += 1;
        }
        if (has_project(FAC_LIVING_REFINERY, faction_id)) {
            vals[SUP] += 2;
        }
        if (has_nexus) {
            if (is_alien(faction_id)) {
                vals[RES]++;
            }
            vals[PLA]++;
        }
        for (int k = 0; k < MaxSocialEffectNum; k++) {
            if ((1 << k) & immunity) {
                vals[k] = max(0, vals[k]);
            } else if ((1 << k) & robust && vals[k] < 0) {
                vals[k] /= 2;
            }
        }
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
        sc += clamp(vals[m->soc_priority_effect], -4, 4)
            * clamp(conf.social_ai_bias / 10, 0, 2);
    }
    if (vals[ECO] >= 2) {
        sc += (vals[ECO] >= 4 ? 16 : 12);
    }
    if (vals[EFF] < -2) {
        sc -= (vals[EFF] < -3 ? 20 : 14);
    }
    if (vals[SUP] < -3) {
        sc -= 16;
    }
    if (vals[MOR] >= 1 && vals[MOR] + w_morale >= 4) {
        sc += 10;
    }
    if (vals[PRO] >= 3 && !has_project(FAC_HUNTER_SEEKER_ALGORITHM, faction_id)) {
        sc += 4 * def_value;
    }
    sc += max(2, 2 + 4*f->AI_wealth + 3*f->AI_tech - f->AI_fight)
        * clamp(vals[ECO], -3, 5);
    sc += max(2, 2*(f->AI_wealth + f->AI_tech) - f->AI_fight + base_ratio/2)
        * (min(6, vals[EFF]) + (vals[EFF] >= 3 ? 2 : 0));
    sc += max(3, 4 + 2*f->AI_power + 2*f->AI_fight - base_ratio/4 + def_value/2)
        * clamp(vals[SUP], -4, 3);
    sc += max(2, def_value + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals[MOR], -4, 4);

    if (!has_project(FAC_TELEPATHIC_MATRIX, faction_id)) {
        sc += (vals[POL] < 0 ? 2 : 4) * clamp(vals[POL], -5, 3);
        if (vals[POL] < -2) {
            sc -= (vals[POL] < -3 ? 2 : 1) * def_value
                * (has_aircraft(faction_id) ? 2 : 1);
        }
        if (has_project(FAC_LONGEVITY_VACCINE, faction_id) && sf == SOCIAL_C_ECONOMICS) {
            sc += (sm == SOCIAL_M_PLANNED ? 10 : 0);
            sc += (sm == SOCIAL_M_SIMPLE || sm == SOCIAL_M_GREEN ? 5 : 0);
        }
        sc += 3*clamp(vals[TAL], -5, 5);

        int drone_score = 3 + (m->rule_drone > 0) - (m->rule_talent > 0)
            - (has_tech(Facility[FAC_PUNISHMENT_SPHERE].preq_tech, faction_id) ? 1 : 0);
        if (*SunspotDuration > 1 && *DiffLevel >= DIFF_LIBRARIAN
        && un_charter() && vals[POL] >= 0) {
            sc += 3*drone_score;
        }
        if (!un_charter() && vals[POL] >= 0) {
            sc += 2*drone_score;
        }
    }
    if (!has_project(FAC_CLONING_VATS, faction_id)) {
        if (pop_boom && vals[GRW] >= 4) {
            sc += 20;
        }
        if (vals[GRW] < -2) {
            sc -= 16;
        }
        sc += (pop_boom ? 6 : 3) * clamp(vals[GRW], -3, 6);
    }
    // Negative planet values reduce fungus yield
    if (plans[faction_id].keep_fungus) {
        sc += 3*clamp(vals[PLA], -3, 0);
    }
    sc += max(2, (f->SE_planet_base > 0 ? 5 : 2) + m->rule_psi/10
        + (has_project(FAC_MANIFOLD_HARMONICS, faction_id) ? 6 : 0)) * clamp(vals[PLA], -3, 3);
    sc += max(2, 1 + def_value + w_probe + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals[PRO], -2, 3);
    sc += 8 * clamp(vals[IND], -3, 8 - *DiffLevel);

    if (!(*GameRules & RULES_SCN_NO_TECH_ADVANCES)) {
        sc += max(2, 3 + 4*f->AI_tech + 2*(f->AI_wealth - f->AI_fight))
            * clamp(vals[RES], -5, 5);
    }

    debug("social_score %d %d %d %d %s\n", faction_id, sf, sm, sc, SocialField[sf].soc_name[sm]);
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
    bool pop_boom = 0;
    int want_pop = 0;
    int pop_total = 0;
    int robust = 0;
    int immunity = 0;
    int impunity = 0;
    int penalty = 0;

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
    bool has_nexus = has_temple(faction_id);
    assert(!memcmp(&f->SE_Politics, &f->SE_Politics_pending, 16));

    for (int i = 0; i < m->faction_bonus_count; i++) {
        if (m->faction_bonus_id[i] == RULE_ROBUST) {
            robust |= (1 << m->faction_bonus_val1[i]);
        }
        if (m->faction_bonus_id[i] == RULE_IMMUNITY) {
            immunity |= (1 << m->faction_bonus_val1[i]);
        }
        if (m->faction_bonus_id[i] == RULE_IMPUNITY) {
            impunity |= (1 << (m->faction_bonus_val1[i] * 4 + m->faction_bonus_val2[i]));
        }
        if (m->faction_bonus_id[i] == RULE_PENALTY) {
            penalty |= (1 << (m->faction_bonus_val1[i] * 4 + m->faction_bonus_val2[i]));
        }
    }
    if (has_project(FAC_NETWORK_BACKBONE, faction_id)) {
        /* Cybernetic */
        impunity |= (1 << (4*SOCIAL_C_FUTURE + SOCIAL_M_CYBERNETIC));
    }
    if (has_project(FAC_CLONING_VATS, faction_id)) {
        /* Power & Thought Control */
        impunity |= (1 << (4*SOCIAL_C_VALUES + SOCIAL_M_POWER))
            | (1 << (4*SOCIAL_C_FUTURE + SOCIAL_M_THOUGHT_CONTROL));

    } else if (has_tech(Facility[FAC_CHILDREN_CRECHE].preq_tech, faction_id)) {
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id) {
                want_pop += (pop_goal(i) - b->pop_size)
                    * (b->nutrient_surplus > 1 && has_facility(FAC_CHILDREN_CRECHE, i) ? 4 : 1);
                pop_total += b->pop_size;
            }
        }
        if (pop_total > 0) {
            pop_boom = ((f->SE_growth < 4 ? 1 : 2) * want_pop) >= pop_total;
        }
    }
    debug("social_params %d %d %8s defense: %d has_nexus: %d pop_boom: %d want_pop: %3d pop_total: %3d "\
        "robust: %04x immunity: %04x impunity: %04x penalty: %04x\n", *CurrentTurn, faction_id, m->filename,
        def_value, has_nexus, pop_boom, want_pop, pop_total, robust, immunity, impunity, penalty);
    int score_diff = 1 + (*CurrentTurn + 11*faction_id) % 6;
    int sf = -1;
    int sm2 = -1;
    CSocialCategory soc;

    for (int i = 0; i < MaxSocialCatNum; i++) {
        int sm1 = (&f->SE_Politics)[i];
        int sc1 = social_score(faction_id, i, sm1, def_value, pop_boom, has_nexus, robust, immunity, impunity, penalty);

        for (int j = 0; j < MaxSocialModelNum; j++) {
            if (j == sm1 || !society_avail(i, j, faction_id)) {
                continue;
            }
            int sc2 = social_score(faction_id, i, j, def_value, pop_boom, has_nexus, robust, immunity, impunity, penalty);
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
        debug("social_change %d %d %8s cost: %2d score_diff: %2d %s -> %s\n",
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
    int32_t peace_faction_id = 0;
    bool common_enemy = false; // Target faction is at war with a human faction we are allied with
    if (MFactions[faction_id].is_alien() && MFactions[faction_id_tgt].is_alien()) {
        return true;
    }
    if (has_treaty(faction_id, faction_id_tgt, DIPLO_WANT_REVENGE | DIPLO_UNK_40 | DIPLO_ATROCITY_VICTIM)) {
        return true;
    }
    if (Factions[faction_id_tgt].major_atrocities && Factions[faction_id].major_atrocities) {
        return true;
    }
    if (has_treaty(faction_id, faction_id_tgt, DIPLO_UNK_4000000)) {
        return false;
    }
    // Modify attacks to be less likely when AI has only few bases
    if (Factions[faction_id].base_count
    <= 1 + (*CurrentTurn + faction_id) % (*MapAreaTiles >= 3200 ? 8 : 4)) {
        return false;
    }
    if (!is_human(faction_id_tgt) && Factions[faction_id].player_flags & PFLAG_TEAM_UP_VS_HUMAN) {
        return false;
    }
    int32_t modifier = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id && i != faction_id_tgt) {
            if (has_treaty(faction_id, i, DIPLO_HAVE_SURRENDERED | DIPLO_PACT)) {
                peace_faction_id = i;
            }
            if (has_treaty(faction_id, i, DIPLO_VENDETTA)
            && !has_treaty(faction_id_tgt, i, DIPLO_PACT)) {
                modifier++;
                if (Factions[i].mil_strength_1
                    > ((Factions[faction_id_tgt].mil_strength_1 * 3) / 2)) {
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
                bool has_surrender = has_treaty(faction_id, i, DIPLO_HAVE_SURRENDERED);
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
    if (Factions[faction_id].AI_fight < 0 && !common_enemy && FactionRankings[7] != faction_id_tgt) {
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
        uint32_t force_rating = Factions[faction_id].region_force_rating[region];
        if (!bad_reg(region) && force_rating) {
            uint32_t total_cmbt_vehs = Factions[faction_id_tgt].region_total_combat_units[region];
            uint32_t total_bases_tgt = Factions[faction_id_tgt].region_total_bases[region];

            if (total_cmbt_vehs || total_bases_tgt) {
                if (Factions[faction_id].region_total_bases[region]
                >= ((region_top_base_count[faction_id] / 4) * 3) || region == region_hq) {
                    int32_t compare = force_rating +
                        Factions[faction_id].region_total_combat_units[region] +
                        (faction_id_unk > 0
                            ? Factions[faction_id_unk].region_force_rating[region] / 4 : 0);
                    if (Factions[faction_id_tgt].region_force_rating[region] > compare) {
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
                factor_unk += total_cmbt_vehs
                    + (Factions[faction_id].region_total_bases[region]
                    ? Factions[faction_id_tgt].region_force_rating[region] / 2 : 0);
                if (Factions[faction_id].region_total_bases[region]) {
                    factor_count++;
                }
            }
        }
    }
    modifier -= Factions[faction_id].AI_fight * 2;
    int32_t tech_comm_bonus = Factions[faction_id].tech_commerce_bonus;
    int32_t tech_comm_bonus_target = Factions[faction_id_tgt].tech_commerce_bonus;
    int32_t best_armor_target = Factions[faction_id_tgt].best_armor_value;
    int32_t best_weapon = Factions[faction_id_tgt].best_weapon_value;

    if (tech_comm_bonus > ((tech_comm_bonus_target * 3) / 2)) {
        modifier++;
    }
    if (tech_comm_bonus < ((tech_comm_bonus_target * 2) / 3)) {
        modifier--;
    }
    if (best_weapon > (best_armor_target * 2)) {
        modifier--;
    }
    if (best_weapon <= best_armor_target) {
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
    modifier -= clamp((Factions[faction_id_tgt].integrity_blemishes
        - Factions[faction_id].integrity_blemishes + 2) / 3, 0, 2);
    int32_t morale_factor = clamp(Factions[faction_id].SE_morale_pending, -4, 4)
        + MFactions[faction_id].rule_morale + 16;
    int32_t morale_divisor = factor_unk * (clamp(Factions[faction_id_tgt].SE_morale_pending, -4, 4)
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
    bool value = evaluate_attack(faction_id, faction_id_tgt, faction_id_unk);

    debug("wants_to_attack turn: %d factions: %d %d %d value: %d\n",
        *CurrentTurn, faction_id, faction_id_tgt, faction_id_unk, value);
    return value;
}


