
#include "base.h"

static bool delay_base_riot = false;


static bool valid_relocate_base(int base_id) {
    int faction_id = Bases[base_id].faction_id;
    int best_id = -1;
    int best_score = 0;

    if (conf.auto_relocate_hq || Bases[base_id].mineral_surplus < 2) {
        return false;
    }
    if (!has_tech(Facility[FAC_HEADQUARTERS].preq_tech, faction_id)) {
        return false;
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* b = &Bases[i];
        if (b->faction_id == faction_id) {
            if (has_fac_built(FAC_HEADQUARTERS, i)) {
                return false;
            }
            if (b->item() == -FAC_HEADQUARTERS) {
                return base_id == i;
            }
            int score = 8*(i == base_id) + 2*b->pop_size
                + b->mineral_surplus - b->assimilation_turns_left;
            if (best_id < 0 || score > 2 * best_score) {
                best_id = i;
                best_score = score;
            }
        }
    }
    return best_id == base_id;
}

static void find_relocate_base(int faction_id) {
    if (conf.auto_relocate_hq && find_hq(faction_id) < 0) {
        int best_score = INT_MIN;
        int best_id = -1;
        Points bases;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id) {
                bases.insert({b->x, b->y});
            }
        }
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id) {
                int score = 4*b->pop_size - (int)(10 * avg_range(bases, b->x, b->y))
                    - 2*b->assimilation_turns_left;
                debug("relocate_base %s %4d %s\n",
                    MFactions[faction_id].filename, score, b->name);
                if (score > best_score) {
                    best_id = i;
                    best_score = score;
                }
            }
        }
        if (best_id >= 0) {
            BASE* b = &Bases[best_id];
            set_fac(FAC_HEADQUARTERS, best_id, 1);
            if (!is_human(faction_id)) {
                mod_base_reset(best_id, 0);
            }
            draw_tile(b->x, b->y, 2);
            parse_says(1, Bases[best_id].name, -1, -1);
            parse_says(2, parse_set(faction_id), -1, -1);
            NetMsg_pop(NetMsg, "ESCAPED", 5000, 0, 0);
        }
    }
}

int __cdecl mod_base_init(int faction_id, int x, int y) {
    MAP* sq;
    if (*BaseCount >= MaxBaseNum) {
        return -1;
    }
    if (faction_id < 0 || faction_id >= MaxPlayerNum
    || !(sq = mapsq(x, y)) || sq->is_base()) { // Added validity checks
        assert(0);
        return -1;
    }
    Faction* plr = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    bit_set(x, y, BIT_BASE_IN_TILE, 1);
    owner_set(x, y, faction_id);
    synch_bit(x, y, faction_id);
    int base_id = (*BaseCount)++;
    BASE* base = &Bases[base_id];
    plr->base_count++;
    plr->region_total_bases[sq->region]++;
    mod_replay_base(0, x, y, faction_id);
    base->x = x;
    base->y = y;
    base->faction_id_former = faction_id;
    base->faction_id = faction_id;
    base->nerve_staple_turns_left = 0;
    base->assimilation_turns_left = 0;
    base->ai_plan_status = 0;
    base->pop_size = has_project(FAC_PLANETARY_TRANSIT_SYSTEM, faction_id) ? 3 : 1;
    base->visibility = (1 << faction_id) | sq->visibility;
    base->name[0] = '\0';
    mod_name_base(faction_id, &base->name[0], 1, is_ocean(sq));
    base->state_flags = 0;
    base->event_flags = 0;
    base->governor_flags = plr->base_governor_adv;
    base->minerals_accumulated = 0;
    base->nutrients_accumulated = 0;
    base->nerve_staple_count = 0;
    base->autoforward_land_base_id = -1;
    base->autoforward_sea_base_id = -1;
    base->autoforward_air_base_id = -1;
    base->queue_size = 0;
    base_first(base_id);

    if (*CurrentTurn) {
        base->minerals_accumulated = plr->SE_support < -1 ? 0 : Rules->retool_exemption;
    } else {
        int unit_id;
        if (has_tech(Units[BSC_FORMERS].preq_tech, faction_id)) {
            unit_id = BSC_FORMERS;
        } else {
            unit_id = plr->diff_level > 1 ? BSC_SCOUT_PATROL : BSC_COLONY_POD;
        }
        base->queue_items[0] = unit_id;
        base->minerals_accumulated = Units[unit_id].cost * mod_cost_factor(faction_id, RSC_MINERAL, -1) / 2;
    }
    base->worked_tiles = 0;
    base->specialist_total = 0;
    base->specialist_adjust = 0;
    base->eco_damage = 0;
    base->unk_y = -1;
    base->unk_x = -1;
    memset(&base->facilities_built, 0, 12);
    base->random_event_turns = 0;
    base->nerve_staple_count = 0;
    base->defend_goal = 0;
    base->defend_range = 0;
    base->pad_6 = 0;
    base->pad_7 = 0;
    base->pad_8 = 0;

    if (plr->base_count == 1) {
        set_fac(FAC_HEADQUARTERS, base_id, 1);
        if (*MultiplayerActive) {
            base->pop_size = 3;
            if (m->rule_talent || plr->SE_planet_base > 0) {
                base->pop_size = 4;
            }
            if (m->rule_drone || m->rule_population) {
                base->pop_size--;
            }
            set_fac(FAC_RECYCLING_TANKS, base_id, 1);
            if (*DiffLevel >= 3) {
                set_fac(FAC_RECREATION_COMMONS, base_id, 1);
            }
            if (plr->SE_economy_base > 0) {
                set_fac(FAC_ENERGY_BANK, base_id, 1);
            }
        } else {
            base->pop_size = clamp((*CurrentTurn + 49) / 50, 1, 5);
            if (*CurrentTurn >= 75) {
                set_fac(FAC_RECYCLING_TANKS, base_id, 1);
            }
            if (*CurrentTurn >= 100) {
                set_fac(FAC_RECREATION_COMMONS, base_id, 1);
            }
            if (*CurrentTurn >= 150) {
                set_fac(FAC_ENERGY_BANK, base_id, 1);
            }
        }
    }
    if (is_ocean(sq)) {
        set_fac(FAC_PRESSURE_DOME, base_id, 1);
    }
    for (int i = 0; i < MaxPlayerNum; i++) {
        base->factions_pop_size_intel[i] =
            (i == faction_id || (1 << i) & base->visibility ? base->pop_size : 0);
    }
    // Original function accepts IDs including satellites and stockpile energy but these
    // cannot be built on the base normally and will be ignored by additional set_fac checks.
    for (int i = 0; i < m->faction_bonus_count; i++) {
        if (m->faction_bonus_id[i] == RULE_FACILITY) {
            int item_id = clamp(m->faction_bonus_val1[i], 1, Fac_All_ID_Last);
            set_fac((FacilityId)item_id, base_id, 1);
        }
    }
    for (int i = 0; i < m->faction_bonus_count; i++) {
        if (m->faction_bonus_id[i] == RULE_FREEFAC) {
            int item_id = clamp(m->faction_bonus_val1[i], 1, Fac_All_ID_Last);
            if (has_tech(Facility[item_id].preq_tech, faction_id)) {
                set_fac((FacilityId)item_id, base_id, 1);
            }
        }
    }
    for (int i = 1; i <= Fac_All_ID_Last; i++) {
        if (has_tech(Facility[i].free_tech, faction_id)) {
            set_fac((FacilityId)i, base_id, 1);
        }
    }
    base->production_id_last = base->queue_items[0];
    base->minerals_accumulated_2 = base->minerals_accumulated;
    base->mineral_surplus_final = 0;
    mod_base_mark(base_id);
    reset_territory();
    set_base(base_id);
    base_compute(1); // Always update
    *GameDrawState |= 2u;
    return base_id;
}

void __cdecl mod_base_kill(int base_id) {
    assert(base_id >= 0 && base_id < *BaseCount);
    int prev_faction = Bases[base_id].faction_id;
    base_kill(base_id);
    find_relocate_base(prev_faction);
}

void __cdecl mod_base_reset(int base_id, bool has_gov) {
    BASE& base = Bases[base_id];
    bool manage_prod = conf.manage_player_bases
        && Bases[base_id].governor_flags & GOV_MANAGE_PRODUCTION;
    assert(base_id >= 0 && base_id < *BaseCount);
    assert(base.defend_goal >= 0 && base.defend_goal <= 5);
    print_base(base_id);

    if (base.plr_owner() && !manage_prod) {
        debug("skipping human base\n");
        base_reset(base_id, has_gov);
    } else if (!base.plr_owner() && !thinker_enabled(base.faction_id)) {
        debug("skipping computer base\n");
        base_reset(base_id, has_gov);
    } else {
        int choice = mod_base_build(base_id, has_gov);
        base_change(base_id, choice);
    }
}

/*
Performs nearly the same thing as original base_build except the last three parameters
have been replaced with the governor force recalculate flag to replace current production.
*/
int __cdecl mod_base_build(int base_id, bool has_gov) {
    BASE& base = Bases[base_id];
    int choice = 0;
    set_base(base_id);
    base_compute(1);

    if (!base.plr_owner() && *ControlUpkeepA) {
        consider_staple(base_id);
    }
    if (base.state_flags & BSTATE_PRODUCTION_DONE || has_gov) {
        debug("BUILD NEW\n");
        choice = select_build(base_id);
    } else if (base.item() >= 0 && !can_build_unit(base_id, base.item())) {
        debug("BUILD CHANGE\n");
        choice = select_build(base_id);
    } else if (base.item() < 0 && !can_build(base_id, abs(base.item()))) {
        debug("BUILD CHANGE\n");
        choice = select_build(base_id);
    } else if ((base.item() < 0 || !Units[base.item()].is_garrison_unit())
    && !has_defenders(base.x, base.y, base.faction_id)) {
        debug("BUILD DEFENSE\n");
        choice = find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, DEF);
    } else {
        debug("BUILD OLD\n");
        choice = base.item();
    }
    if (!has_gov && base.governor_flags & GOV_ACTIVE && base.plr_owner()
    && choice <= -SP_ID_First && base.item() > -SP_ID_First
    && *GamePreferences & PREF_BSC_TUTORIAL_MSGS && !*MultiplayerActive && !victory_done()) {
        int value = 0;
        while (choice < 0) {
            parse_says(0, base.name, -1, -1);
            parse_says(1, Facility[-choice].name, -1, -1);
            value = popp(ScriptFile, "GOVPROJECT", 0, "secproj_sm.pcx", 0);
            if (value != 3) {
                break;
            }
            help_topic(11, -choice);
        }
        if (value) {
            base.governor_flags &= ~GOV_MAY_PROD_SP;
            choice = select_build(base_id);
            if (value > 1) {
                Factions[base.faction_id].base_governor_adv &= ~GOV_MAY_PROD_SP;
            }
        }
    }
    debug("choice: %d %s\n", choice, prod_name(choice));
    flushlog();
    return choice;
}

void __cdecl base_first(int base_id) {
    BASE& base = Bases[base_id];
    Faction& f = Factions[base.faction_id];
    base.queue_items[0] = find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, DEF);

    if (base.plr_owner()) {
        int num = f.saved_queue_size[0];
        if (num > 0 && num < 10) {
            for (int i = 0; i < num; i++) {
                // Get only the lower 16 bits *with* sign extension
                base.queue_items[i] = (int16_t)f.saved_queue_items[0][i];
            }
            base.queue_size = num - 1;
        }
    }
}

void __cdecl set_base(int base_id) {
    *CurrentBaseID = base_id;
    *CurrentBase = &Bases[base_id];
}

void __cdecl base_compute(bool update_prev) {
    if (*CurrentBase) {
        // Fix rare issue that caused the base build queue to be saved with
        // incorrect entries resulting in crashes during turn upkeep.
        BASE* base = *CurrentBase;
        assert(base->queue_size < 10);
        assert(!(base->governor_flags &
            (GOV_UNK_4|GOV_UNK_8|GOV_UNK_10|GOV_UNK_4000|GOV_UNK_10000000|GOV_UNK_20000000)));
        for (int i = base->queue_size; i > 0; --i) {
            if (i > 9 || base->queue_items[i] < -SP_ID_Last
            || base->queue_items[i] >= MaxProtoNum) {
                base->queue_size = 0;
                memset(&base->queue_items[1], 0, 36);
                assert(base->worked_tiles);
                break;
            }
        }
    }
    if (update_prev || *ComputeBaseID != *CurrentBaseID) {
        *ComputeBaseID = *CurrentBaseID;
        mod_base_support();
        mod_base_yield();
        mod_base_nutrient();
        mod_base_minerals();
        mod_base_energy();
    }
}

void __cdecl mod_base_mark(int base_id) {
    int x = Bases[base_id].x;
    int y = Bases[base_id].y;
    int faction_id = Bases[base_id].faction_id;
    for (int i = 0; i < TableRange[3]; i++) {
        int x2 = wrap(x + TableOffsetX[i]);
        int y2 = y + TableOffsetY[i];
        if (on_map(x2, y2)) {
            if (i < 21) {
                bit_set(x2, y2, BIT_BASE_RADIUS, 1);
                using_set(x2, y2, faction_id);
            } else {
                site_set(x2, y2, 0);
            }
        }
    }
    for (int i = 0; i < MaxPlayerNum; i++) {
        // Fix most likely error that used offset location here instead of base location
        del_site(i, AI_GOAL_COLONIZE, x, y, 3);
    }
}

/*
Calculate nutrient/mineral cost factors for base production.
If the player faction is ranked first in the original game, the AI factions will get
additional growth/industry bonuses. This can be optionally skipped with simple_cost_factor option.
*/
int __cdecl mod_cost_factor(int faction_id, BaseResType type, int base_id) {
    int value;
    int multiplier = (type == RSC_NUTRIENT ?
        Rules->nutrient_cost_multi : Rules->mineral_cost_multi);

    if (is_human(faction_id)) {
        value = multiplier;
    } else {
        value = CostRatios[*DiffLevel];
        if (!conf.simple_cost_factor) {
            value -= (great_satan(FactionRankings[7], 0) != 0);
            value -= (!*MultiplayerActive && is_human(FactionRankings[7]));
        }
        value = multiplier * value / 10;
    }
    if (*MapSizePlanet == 0) {
        value = 8 * value / 10;
    } else if (*MapSizePlanet == 1) {
        value = 9 * value / 10;
    }
    if (type == RSC_MINERAL) {
        switch (Factions[faction_id].SE_industry_pending) {
            case -7:
            case -6:
            case -5:
            case -4:
            case -3:
                value = (13 * value + 9) / 10;
                break;
            case -2:
                value = (6 * value + 4) / 5;
                break;
            case -1:
                value = (11 * value + 9) / 10;
                break;
            case 0:
                break;
            case 1:
                value = (9 * value + 9) / 10;
                break;
            case 2:
                value = (4 * value + 4) / 5;
                break;
            case 3:
                value = (7 * value + 9) / 10;
                break;
            case 4:
                value = (3 * value + 4) / 5;
                break;
            default: // +5 Industry or better
                value = (value + 1) / 2;
        }
    } else if (type == RSC_NUTRIENT) {
        int growth = Factions[faction_id].SE_growth_pending;
        if (base_id >= 0) {
            growth = Bases[base_id].SE_growth(SE_Pending);
        }
        value = (value * (10 - clamp(growth, -2, 5)) + 9) / 10;
    }
    return value;
}

/*
Determine if the specified base has any restrictions around production item retooling.
Return Value: Fixed value (-1, 0, 1, 2, 3, -70) or item_id
*/
int __cdecl mod_base_making(int item_id, int base_id) {
    assert(base_id >= 0 && base_id < *BaseCount);
    int retool = Rules->retool_strictness;
    int faction_id = Bases[base_id].faction_id;
    if ((has_fac_built(FAC_SKUNKWORKS, base_id)
    || (MFactions[faction_id].rule_flags & RFLAG_FREEPROTO
    && has_tech(Facility[FAC_SKUNKWORKS].preq_tech, faction_id)))
    && retool >= RETOOL_FREE_CATEGORY) {
        // Do not override if retool strictness is already set to Always Free
        // Skunkworks or FREEPROTO + prerequisite tech > 'Free in Category'
        retool = RETOOL_FREE_CATEGORY;
    }
    int value = 1; // Default return value if no other condition applies
    if (item_id < 0) {
        int queue_id = Bases[base_id].queue_items[0];
        if (queue_id < 0 && queue_id >= -Fac_ID_Last
        && has_fac_built((FacilityId)-queue_id, base_id)) {
            value = -1; // Facility already completed, no retool penalty to change
        }
    }
    if (value < 0) {
        // No retool penalty
    } else if (retool == RETOOL_ALWAYS_FREE) { // Always Free
        value = 0;
    } else if (retool == RETOOL_FREE_CATEGORY) { // Free in Category
        if (item_id >= 0) {
            value = 0;
        } else {
            value = (item_id > -SP_ID_First ? (item_id >= -Fac_ID_Last) + 2 : 1);
        }
    } else if (retool == RETOOL_FREE_PROJECT) { // Free switching between SPs (default behavior)
        value = (item_id <= -SP_ID_First ? -SP_ID_First : item_id);
    } else if (retool == RETOOL_NEVER_FREE) { // Never Free
        value = item_id;
    }
    assert(value == base_making(item_id, base_id));
    return value;
}

/*
Calculate the mineral loss if the production were changed at the specified base.
Return Value: Minerals that would be lost or 0 if not applicable.
*/
int __cdecl mod_base_lose_minerals(int base_id, int UNUSED(item_id)) {
    BASE* base = &Bases[base_id];
    int min_accum = base->minerals_accumulated_2;
    if (Rules->retool_penalty_prod_change
    && is_human(base->faction_id)
    && min_accum > Rules->retool_exemption
    && mod_base_making(base->production_id_last, base_id)
    != mod_base_making(base->queue_items[0], base_id)) {
        return min_accum - (100 - Rules->retool_penalty_prod_change)
            * (min_accum - Rules->retool_exemption) / 100 - Rules->retool_exemption;
    }
    return 0;
}

int __cdecl mod_base_production() {
    int base_id = *CurrentBaseID;
    BASE* base = &Bases[base_id];
    Faction* f = &Factions[base->faction_id];
    int item_id = base->item();
    int output = stockpile_energy_active(base_id);
    int value = base_production();
    if (!value) { // Non-zero indicates production was stopped
        f->energy_credits += output;
    }
    debug("base_production %d %d credits: %d stockpile: %d %s / %s\n",
    *CurrentTurn, base_id, f->energy_credits,
    (!value ? output : 0), base->name, prod_name(item_id));
    return value;
}

void __cdecl mod_base_support() {
    BASE* base = *CurrentBase;
    int base_id = *CurrentBaseID;
    Faction* f = &Factions[base->faction_id];

    const int SupportCosts[8][2]  = {
        {2, 0}, // -4, Each unit costs 2 to support; no free minerals for new base.
        {1, 0}, // -3, Each unit costs 1 to support; no free minerals for new base.
        {1, 1}, // -2, Support 1 unit free per base; no free minerals for new base.
        {1, 1}, // -1, Support 1 unit free per base
        {1, 2}, //  0, Support 2 units free per base
        {1, 3}, //  1, Support 3 units free per base
        {1, 4}, //  2, Support 4 units free per base!
        {1, max((int)base->pop_size, 4)}, // 3, Support 4 units OR up to base size for free!!
    };
    const int support_val = clamp(f->SE_support_pending + 4, 0, 7);
    const int support_mod = (is_human(base->faction_id) ? 0 : conf.unit_support_bonus[*DiffLevel]);
    const int support_type = support_plan();
    const int SE_police = base->SE_police(SE_Pending);

    BaseResourceConvoyTo[0] = 0;
    BaseResourceConvoyTo[1] = 0;
    BaseResourceConvoyTo[2] = 0;
    BaseResourceConvoyTo[3] = 0;
    BaseResourceConvoyFrom[0] = 0;
    BaseResourceConvoyFrom[1] = 0;
    BaseResourceConvoyFrom[2] = 0;
    BaseResourceConvoyFrom[3] = 0;
    *BaseVehPacifismCount = 0;
    *BaseForcesSupported = 0;
    *BaseForcesMaintCost = 0;
    *BaseForcesMaintCount = 0;

    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id != base->faction_id) {
            continue;
        }
        MAP* sq = mapsq(veh->x, veh->y);
        if (veh->is_supply() && veh->order == ORDER_CONVOY) {
            BaseResType type = (BaseResType)veh->order_auto_type;
            assert(type <= RSC_UNUSED);
            if (veh->home_base_id == base_id && type <= RSC_UNUSED) {
                if (!sq->is_base()) {
                    int value = resource_yield(type, veh->faction_id, base_id, veh->x, veh->y);
                    BaseResourceConvoyTo[type] += value;
                }
                if (sq->is_base()) {
                    BaseResourceConvoyFrom[type]++;
                }
            } else if (veh->x == base->x && veh->y == base->y
            && veh->home_base_id >= 0 && type <= RSC_UNUSED) {
                BaseResourceConvoyTo[type]++;
            }
        }
        if (veh->home_base_id == base_id) {
            veh->state &= ~(VSTATE_PACIFISM_FREE_SKIP|VSTATE_PACIFISM_DRONE|VSTATE_REQUIRES_SUPPORT);

            if (veh->offense_value() && (veh->offense_value() > 0 || veh->unit_id >= MaxProtoFactionNum)
            && veh->plan() != PLAN_RECON && veh->plan() != PLAN_PLANET_BUSTER) {
                f->unk_46 += 1 + (veh->plan() == PLAN_OFFENSE || veh->plan() == PLAN_COMBAT);
            }
            // Exclude probe, supply, artifact, fungal and tectonic missiles
            // Native units do not require support on fungus
            // However fungus is not rendered on tiles deeper than ocean shelf
            if (veh->plan() <= support_type) {
                if (!has_abil(veh->unit_id, ABL_CLEAN_REACTOR)) {
                    if (!veh->is_native_unit() || !sq->is_fungus()) {
                        (*BaseForcesSupported)++;
                        if (*BaseForcesSupported > support_mod + SupportCosts[support_val][1]) {
                            veh->state |= VSTATE_REQUIRES_SUPPORT;
                            (*BaseForcesMaintCount)++;
                            (*BaseForcesMaintCost) += SupportCosts[support_val][0];
                        }
                    }
                    if (*BaseUpkeepState == 1) {
                        for (int j = 0; j < 8; j++) {
                            if (*BaseForcesSupported > SupportCosts[j][1]) {
                                f->unk_40[j] += SupportCosts[j][0];
                            }
                        }
                    }
                }
                // Pacifism drones are only created for combat units that match either of these rules.
                // 1) located on any foreign owned territory outside bases
                // 2) any aircraft without plan set to Air Superiority (applied with the ability)
                if (veh->offense_value() != 0 && ((!sq->is_base()
                && mod_whose_territory(base->faction_id, veh->x, veh->y, 0, 0) != base->faction_id)
                || (veh->triad() == TRIAD_AIR && veh->plan() != PLAN_AIR_SUPERIORITY))) {
                    (*BaseVehPacifismCount)++;
                    if (SE_police == -3 && *BaseVehPacifismCount == 1) {
                        veh->state |= VSTATE_PACIFISM_FREE_SKIP;
                    } else if (SE_police <= -3) {
                        veh->state |= VSTATE_PACIFISM_DRONE;
                    }
                }
            }
        }
    }
}

static int32_t base_radius(int base_id, std::vector<TileValue>& tiles) {
    Points reserved;
    BASE* base = &Bases[base_id];
    int faction_id = base->faction_id;
    bool has_map = Factions[faction_id].player_flags & PFLAG_MAP_REVEALED;
    int32_t usedtiles = 0;

    // Fix: In rare cases bases might have incorrect worked tiles set on foreign territory.
    // To avoid reserving these tiles the function checks actual territory ownership.
    for (int i = 0; i < *BaseCount; i++) {
        BASE* b = &Bases[i];
        if (base_id == i || map_range(base->x, base->y, b->x, b->y) > 4) {
            continue;
        }
        for (auto& m : iterate_tiles(b->x, b->y, 1, 21)) {
            if (b->worked_tiles & (1 << m.i)) {
                if (m.sq->owner < 0 || m.sq->owner == b->faction_id
                || mod_whose_territory(b->faction_id, m.x, m.y, 0, 0) < 0) {
                    reserved.insert({m.x, m.y});
                }
            }
        }
    }
    for (int i = 0; i < 25; i++) {
        int x = wrap(base->x + TableOffsetX[i]);
        int y = base->y + TableOffsetY[i];
        MAP* sq = mapsq(x, y);
        if (i == 0) {
            BaseTileFlags[i] = BR_BASE_IN_TILE;
        } else if (i >= 21) {
            BaseTileFlags[i] = BR_NOT_AVAILABLE;
        } else if (!sq || (!has_map && !sq->is_visible(faction_id))) {
            BaseTileFlags[i] = BR_NOT_VISIBLE;
        } else {
            BaseTileFlags[i] = 0;
            if (sq->is_base()) {
                BaseTileFlags[i] |= BR_BASE_IN_TILE;
            }
            for (int j = 0; j < *VehCount; j++) {
                VEH* veh = &Vehs[j];
                if (veh->x == x && veh->y == y
                && (veh->order == ORDER_CONVOY
                || (veh->faction_id != faction_id && veh->is_visible(faction_id)
                && !has_treaty(faction_id, veh->faction_id, DIPLO_TREATY|DIPLO_PACT)))) {
                    BaseTileFlags[i] |= BR_VEH_IN_TILE;
                }
            }
            // Do not display worker status for foreign tiles
            if (sq->owner >= 0 && faction_id != mod_whose_territory(faction_id, x, y, 0, 0)) {
                BaseTileFlags[i] |= BR_FOREIGN_TILE;
            } else if (reserved.count({x, y})) {
                BaseTileFlags[i] |= BR_WORKER_ACTIVE;
            }
        }
        if (i < 21) {
            bool valid = !BaseTileFlags[i];
            assert(sq || !(i == 0 || valid));
            if (!valid) {
                usedtiles |= (1 << i);
            }
            if (sq && (i == 0 || valid)) {
                int N = mod_crop_yield(faction_id, base_id, x, y, 0);
                int M = mod_mine_yield(faction_id, base_id, x, y, 0);
                int E = mod_energy_yield(faction_id, base_id, x, y, 0);
                tiles.push_back({x, y, i, sq, N, M, E});
            }
        }
    }
    return usedtiles;
}

static void base_update(BASE* base, std::vector<TileValue>& tiles) {
    for (auto& m : tiles) {
        if ((1 << m.i) & base->worked_tiles) {
            base->nutrient_intake += m.nutrient;
            base->mineral_intake += m.mineral;
            base->energy_intake += m.energy;
        }
    }
    base->nutrient_intake_2 = base->nutrient_intake;
    base->mineral_intake_2 = base->mineral_intake;
    base->energy_intake_2 = base->energy_intake;
    base->unused_intake_2 = base->unused_intake;
    assert(tiles.front().x == base->x && tiles.front().y == base->y);
    assert(base->pop_size + 1 == __builtin_popcount(base->worked_tiles) + base->specialist_total);
    assert(base->pop_size >= base->specialist_total && base->specialist_total >= 0);
}

void __cdecl mod_base_yield() {
    BASE* base = *CurrentBase;
    int base_id = *CurrentBaseID;
    int faction_id = base->faction_id;
    Faction* f = &Factions[base->faction_id];
    std::vector<TileValue> tiles;
    uint32_t gov = base->gov_config();
    int32_t reserved = base_radius(base_id, tiles);
    bool manage_workers = !base->worked_tiles || (gov & GOV_ACTIVE && gov & GOV_MANAGE_CITIZENS);
    bool pre_upkeep = *BaseUpkeepState != 2 || (base_id == *BaseUpkeepDrawID && Win_is_visible(BaseWin));
    base->worked_tiles &= (~reserved) | 1;
    base->state_flags &= ~BSTATE_UNK_8000;
    assert(f->SE_alloc_labs + f->SE_alloc_psych <= 10);
    assert((int)&base->nutrient_intake + 96 == (int)&base->autoforward_land_base_id);
    memset(&base->nutrient_intake, 0, 96);

    int Ns = 0, Ms = 0, Es = 0;
    satellite_bonus(base_id, &Ns, &Ms, &Es);
    base->nutrient_intake = Ns;
    base->mineral_intake = Ms;
    base->energy_intake = Es;

    int SE_police = base->SE_police(SE_Pending);
    bool can_grow = base_unused_space(base_id) > 0;
    bool can_riot = base_can_riot(base_id, true);
    bool need_labs = !has_fac_built(FAC_PUNISHMENT_SPHERE, base_id);
    bool pacifism = can_riot && SE_police <= -3 && *BaseVehPacifismCount > 0;
    int threshold = Rules->nutrient_intake_req_citizen * (base_pop_boom(base_id) ? 1 : 2);
    int effic_val = 16 - energy_intake_lost(base_id, 16, 0);
    int alloc_econ = 10 - f->SE_alloc_labs - f->SE_alloc_psych;
    int econ_val = 4 + (alloc_econ > f->SE_alloc_labs);
    int labs_val = 2 + 2*(need_labs && f->SE_alloc_labs > 0) + (alloc_econ <= f->SE_alloc_labs);
    int psych_val = 2 + 2*can_riot + 4*pacifism;
    int best_spc_id = best_specialist(base, econ_val, labs_val, psych_val);
    int psych_spc_id = (can_riot ? best_specialist(base, econ_val, labs_val, 3*psych_val) : best_spc_id);
    CCitizen& spc = Citizen[best_spc_id];

    // Initial production without intake from any tiles (incl. base tile)
    int Nv = Ns - base->pop_size * Rules->nutrient_intake_req_citizen
        + BaseResourceConvoyTo[RSC_NUTRIENT] - BaseResourceConvoyFrom[RSC_NUTRIENT];
    int Mv = Ms - *BaseForcesMaintCost
        + BaseResourceConvoyTo[RSC_MINERAL] - BaseResourceConvoyFrom[RSC_MINERAL];
    int Ev = Es + BaseResourceConvoyTo[RSC_ENERGY] - BaseResourceConvoyFrom[RSC_ENERGY];
    int total = base->pop_size + 1 - __builtin_popcount(base->worked_tiles)
        - base->specialist_total;
    int specialist_adjust = 0;
    int specialist_total = base->specialist_total;
    int worked_tiles = base->worked_tiles;
    std::vector<TileValue> choices;

    int Wenergy = 1 + (effic_val >= 6);
    int Wmineral = 3;
    if (base->defend_range > 0) {
        Wmineral = 3 + (base->defend_goal > 2) + max(0, 2 - base->defend_range/16);
    }
    if (is_human(faction_id)) {
        Wmineral = 3 + (gov & GOV_PRIORITY_BUILD ? 1 : 0) + (gov & GOV_PRIORITY_CONQUER ? 1 : 0);
    }

    if (pre_upkeep) {
        for (int i = 0; i < MaxBaseSpecNum; i++) {
            int spc_id = base->specialist_type(i);
            // Replace incorrect specialist types if any used
            if (manage_workers || has_tech(Citizen[spc_id].obsol_tech, faction_id)
            || !has_tech(Citizen[spc_id].preq_tech, faction_id)
            || (Citizen[spc_id].psych_bonus < 2 && base->pop_size < Rules->min_base_size_specialists)) {
                base->specialist_modify(i, best_spc_id);
            }
        }
    }
    if (total != 0 || (manage_workers && pre_upkeep)) {
        int workers = total;
        if (workers < 0 || manage_workers) {
            worked_tiles = 1;
            specialist_total = 0;
            workers = base->pop_size;
        }
        for (auto& m : tiles) {
            if ((1 << m.i) & worked_tiles) {
                Nv += m.nutrient;
                Mv += m.mineral;
                Ev += m.energy;
            }
        }
        choices.push_back(tiles.front());
        while (workers > 0) {
            TileValue* choice = NULL;
            int best_score = 0;
            for (auto& m : tiles) {
                int score;
                int energy_val = (can_riot && effic_val >= 12 && f->SE_alloc_psych > 0
                    && (Ev * f->SE_alloc_psych + 4) / 10 < 2) ? 6 : 4;
                if (can_grow) {
                    score = m.nutrient * max(4 + 4*(Nv < 0) + 4*(Nv < threshold)
                        + max(0, 5 - base->pop_size) - max(0, Nv - 1)/4, 2)
                        + m.mineral * (5 + 5*(Mv < 0) + 4*(Mv < 2))
                        + (m.energy * effic_val + 15) / 16 * energy_val;
                } else {
                    score = m.nutrient * (Nv < 0 ? 8 : 2)
                        + m.mineral * (5 + 5*(Mv < 0) + 4*(Mv < 2))
                        + (m.energy * effic_val + 15) / 16 * energy_val;
                }
                score = 32*score - m.i; // Select adjacent tiles first
                if (!((1 << m.i) & worked_tiles) && score > best_score) {
                    choice = &m;
                    best_score = score;
                }
            }
            if (!choice) {
                break;
            }
            int Wnutrient = 1 + (can_grow || Nv < 0 || Mv < 2)
                + (base->pop_size < Rules->min_base_size_specialists + 2);
            if (2*(spc.labs_bonus + spc.econ_bonus) + (can_riot ? 2 : 1) * spc.psych_bonus
            > Wnutrient*choice->nutrient + Wmineral*choice->mineral + Wenergy*choice->energy
            && Nv >= (can_grow ? threshold : 0) && Mv >= 2
            && Mv + *BaseForcesMaintCost >= (base->pop_size * Wmineral + 5) / 2) {
                break;
            }
            worked_tiles |= (1 << choice->i);
            Nv += choice->nutrient;
            Mv += choice->mineral;
            Ev += choice->energy;
            workers--;
            choices.push_back(*choice);
        }
        // Convert unallocated workers to specialists
        specialist_total += workers;
    }
    if ((total != 0 || manage_workers) && pre_upkeep) {
        base->worked_tiles = worked_tiles;
        base->specialist_total = specialist_total;
        BASE initial;
        memcpy(&initial, base, sizeof(BASE));
        bool valid = !can_riot;
        while (!valid && choices.size() > 1) {
            base_update(base, choices);
            mod_base_minerals();
            mod_base_energy();
            // Take into account possible delayed drone riots after pop growth
            valid = base->talent_total + delay_base_riot >= base->drone_total;
            if (!valid && manage_workers && best_spc_id != psych_spc_id
            && base->drone_total - base->talent_total > (base->pop_size + 3)/4) {
                best_spc_id = psych_spc_id;
                for (int i = 0; i < MaxBaseSpecNum; i++) {
                    base->specialist_modify(i, best_spc_id);
                    initial.specialist_modify(i, best_spc_id);
                }
                mod_base_minerals();
                mod_base_energy();
                valid = base->talent_total + delay_base_riot >= base->drone_total;
            }
            if (base->mineral_surplus - choices.back().mineral < 0) {
                // Priority for mineral support costs
                valid = true;
            }
            memcpy(base, &initial, sizeof(BASE));
            if (!valid) {
                worked_tiles &= ~(1 << choices.back().i);
                choices.pop_back();
                specialist_total++;
                specialist_adjust++;
            }
            base->worked_tiles = worked_tiles;
            base->specialist_total = specialist_total;
        };
        base->specialist_adjust = specialist_adjust;

    } else if (total != 0 && !pre_upkeep) {
        while (base->pop_size + 1 < (int)choices.size()) {
            choices.pop_back();
        }
        base->specialist_total = base->pop_size + 1 - choices.size();
        base->worked_tiles = 0;
        for (auto& m : choices) {
            base->worked_tiles |= (1 << m.i);
        }
    }
    debug_ver("base_yield %d %d state: %d total: %d pop: %d spc: %d adjust: %d\n",
    *CurrentTurn, base_id, *BaseUpkeepState, total, base->pop_size,
    base->specialist_total, base->specialist_adjust);

    base_update(base, tiles);
    base->state_flags &= ~BSTATE_UNK_100;
    base->eco_damage = terraform_eco_damage(base_id);

    if (faction_id == MapWin->cOwner && *ControlUpkeepA
    && f->SE_alloc_psych < 2 && effic_val >= 4 && 2*base->energy_surplus >= 3*base->pop_size
    && base->specialist_adjust > 2 && 2*base->specialist_adjust >= base->pop_size) {
        if (!is_alien(faction_id)) {
            popb("PSYCHREQUEST", 0, -1, "talent_sm.pcx", 0);
        } else {
            popb("PSYCHREQUEST", 0, -1, "Alopdir.pcx", 0);
        }
    }
}

void __cdecl mod_base_nutrient() {
    BASE* base = *CurrentBase;
    int faction_id = base->faction_id;

    *BaseGrowthRate = base->SE_growth(SE_Pending);
    base->nutrient_intake_2 += BaseResourceConvoyTo[RSC_NUTRIENT];
    base->nutrient_consumption = BaseResourceConvoyFrom[RSC_NUTRIENT]
        + base->pop_size * Rules->nutrient_intake_req_citizen;
    base->nutrient_surplus = base->nutrient_intake_2
        - base->nutrient_consumption;
    if (base->nutrient_surplus >= 0) {
        if (base->nutrients_accumulated < 0) {
            base->nutrients_accumulated = 0;
        }
    } else if (!base->nutrients_accumulated) {
        base->nutrients_accumulated = -1;
    }
    if (*BaseUpkeepState == 1) {
        Factions[faction_id].nutrient_surplus_total
            += clamp(base->nutrient_surplus, 0, 99);
    }
}

void __cdecl mod_base_minerals() {
    BASE* base = *CurrentBase;
    int base_id = *CurrentBaseID;
    int faction_id = base->faction_id;

    base->mineral_intake_2 += BaseResourceConvoyTo[RSC_MINERAL];
    base->mineral_intake_2 = (base->mineral_intake_2
        * (mineral_output_modifier(base_id) + 2)) / 2;
    base->mineral_consumption = *BaseForcesMaintCost
        + BaseResourceConvoyFrom[RSC_MINERAL];
    base->mineral_surplus = base->mineral_intake_2
        - base->mineral_consumption;
    base->mineral_inefficiency = 0; // unused
    base->mineral_surplus -= base->mineral_inefficiency; // unused
    base->mineral_surplus_final = base->mineral_surplus;

    base->eco_damage /= 8;
    int clean_minerals_total = conf.clean_minerals
        + Factions[faction_id].clean_minerals_modifier;
    if (base->eco_damage > 0) {
        int damage_modifier = min(base->eco_damage, clean_minerals_total);
        clean_minerals_total -= damage_modifier;
        base->eco_damage -= damage_modifier;
    }
    // Fix: original game also checks here for Nanoreplicator when
    // Singularity Inductor would grant Quantum Converter facility instead
    bool converter = conf.eco_damage_fix ? has_facility(FAC_QUANTUM_CONVERTER, base_id)
        : (has_fac_built(FAC_NANOREPLICATOR, base_id)
        || has_project(FAC_SINGULARITY_INDUCTOR, faction_id));
    int eco_dmg_reduction = converter ? 2 : 1;
    if (has_fac_built(FAC_CENTAURI_PRESERVE, base_id)) {
        eco_dmg_reduction++;
    }
    if (has_fac_built(FAC_TEMPLE_OF_PLANET, base_id)) {
        eco_dmg_reduction++;
    }
    if (has_project(FAC_PHOLUS_MUTAGEN, faction_id)) {
        eco_dmg_reduction++;
    }
    base->eco_damage += (base->mineral_intake_2 - clean_minerals_total
        - clamp(Factions[faction_id].satellites_mineral, 0, (int)base->pop_size))
        / eco_dmg_reduction;
    if (is_human(faction_id)) {
        base->eco_damage += ((Factions[faction_id].major_atrocities
            + TectonicDetonationCount[faction_id]) * 5) / (clamp(*MapSeaLevel, 0, 100)
            / clamp(WorldBuilder->sea_level_rises, 1, 100) + eco_dmg_reduction);
    }
    if (base->eco_damage < 0) {
        base->eco_damage = 0;
    }
    if (voice_of_planet() && *GameRules & RULES_VICTORY_TRANSCENDENCE) {
        base->eco_damage *= 2;
    }
    if (*GameState & STATE_PERIHELION_ACTIVE) {
        base->eco_damage *= 2;
    }
    int diff_factor;
    if (conf.eco_damage_fix || is_human(faction_id)) {
        int diff_lvl = Factions[faction_id].diff_level;
        diff_factor = !diff_lvl ? DIFF_TALENT
            : ((diff_lvl <= DIFF_LIBRARIAN) ? DIFF_LIBRARIAN : DIFF_TRANSCEND);
    } else {
        diff_factor = clamp(6 - *DiffLevel, (int)DIFF_SPECIALIST, (int)DIFF_LIBRARIAN);
    }
    base->eco_damage = ((Factions[faction_id].tech_ranking
        - Factions[faction_id].tech_count_transcendent)
        * (3 - clamp(Factions[faction_id].SE_planet_pending, -3, 2))
        * (*MapNativeLifeForms + 1) * base->eco_damage * diff_factor) / 6;
    base->eco_damage = (base->eco_damage + 50) / 100;

    // Orbital facilities double mineral production rate
    int item_id = base->queue_items[0];
    if (has_project(FAC_SPACE_ELEVATOR, faction_id)
    && (item_id == -FAC_SKY_HYDRO_LAB || item_id == -FAC_NESSUS_MINING_STATION
    || item_id == -FAC_ORBITAL_POWER_TRANS || item_id == -FAC_ORBITAL_DEFENSE_POD)) {
        base->mineral_intake_2 *= 2;
        // doesn't update mineral_surplus_final?
        base->mineral_surplus = base->mineral_intake_2 - base->mineral_consumption;
    }
}

void __cdecl mod_base_energy() {
    BASE* base = *CurrentBase;
    Faction* f = &Factions[base->faction_id];
    int base_id = *CurrentBaseID;
    int faction_id = base->faction_id;
    int our_rank = own_base_rank(base_id);
    int commerce = 0;
    int energygrid = 0;

    base->energy_intake_2 += BaseResourceConvoyTo[RSC_ENERGY];
    base->energy_consumption = BaseResourceConvoyFrom[RSC_ENERGY];

    for (int i = 1; i < MaxPlayerNum; i++) {
        int their_rank;
        BaseCommerceImport[i] = 0;
        BaseCommerceExport[i] = 0;
        if (i != faction_id && !is_alien(faction_id) && !is_alien(i)
        && !f->sanction_turns && !Factions[i].sanction_turns && Factions[i].base_count
        && has_treaty(faction_id, i, DIPLO_TREATY)
        && (their_rank = mod_base_rank(i, our_rank)) >= 0) {
            assert(has_treaty(i, faction_id, DIPLO_TREATY));
            int tech_count = (*TechCommerceCount + 1);
            int base_value = (base->energy_intake + Bases[their_rank].energy_intake + 7) / 8;
            if (global_trade_pact()) {
                base_value *= 2;
            }
            int commerce_import = (tech_count/2 + base_value
                * (f->tech_commerce_bonus + 1)) / tech_count;
            int commerce_export = (tech_count/2 + base_value
               * (Factions[i].tech_commerce_bonus + 1)) / tech_count;
            if (!has_pact(faction_id, i)) {
                commerce_import /= 2;
                commerce_export /= 2;
            }
            if (*GovernorFaction == faction_id) {
                commerce_import++;
            }
            if (*GovernorFaction == i) {
                commerce_export++;
            }
            commerce += commerce_import;
            BaseCommerceImport[i] = commerce_import;
            BaseCommerceExport[i] = commerce_export;
        }
    }
    if (!f->sanction_turns && is_alien(faction_id)) {
        energygrid = energy_grid_output(base_id);
    }
    base->energy_intake_2 += commerce;
    base->energy_intake_2 += energygrid;

    base->energy_inefficiency = energy_intake_lost(base_id, base->energy_intake_2 - base->energy_consumption,
        (*BaseUpkeepState == 1 ? f->unk_43 : NULL));
    base->energy_surplus = base->energy_intake_2 - base->energy_consumption - base->energy_inefficiency;

    if (*BaseUpkeepState == 1) {
        f->energy_surplus_total += clamp(base->energy_surplus, 0, 99999);
    }
    // Non-multiplied energy intake is always limited to this range
    int total_energy = clamp(base->energy_surplus, 0, 9999);
    int val_psych = (total_energy * f->SE_alloc_psych + 4) / 10;
    base->psych_total = max(0, min(val_psych, total_energy));

    int val_econ = (total_energy * (10 - f->SE_alloc_labs - f->SE_alloc_psych) + 4) / 10;
    base->economy_total = max(0, min(total_energy - base->psych_total, val_econ));

    int val_unk = (total_energy * (9 - f->SE_alloc_labs - f->SE_alloc_psych) + 4) / 10;
    base->unk_total = max(0, min(total_energy - base->psych_total, val_unk));
    base->labs_total = total_energy - base->psych_total - base->economy_total;

    for (int i = 0; i < base->specialist_total; i++) {
        int citizen_id;
        if (i < MaxBaseSpecNum) {
            citizen_id = clamp(base->specialist_type(i), 0, MaxSpecialistNum-1);
        } else {
            citizen_id = mod_best_specialist();
        }
        if (has_tech(Citizen[citizen_id].obsol_tech, faction_id)) {
            for (int j = 0; j < MaxSpecialistNum; j++) {
                if (has_tech(Citizen[j].preq_tech, faction_id)
                && !has_tech(Citizen[j].obsol_tech, faction_id)
                && Citizen[j].econ_bonus >= Citizen[citizen_id].econ_bonus
                && Citizen[j].psych_bonus >= Citizen[citizen_id].psych_bonus
                && Citizen[j].labs_bonus >= Citizen[citizen_id].labs_bonus) {
                    citizen_id = j;
                    break; // Original game picks first here regardless of other choices
                }
            }
        }
        base->specialist_modify(i, citizen_id);
        base->economy_total += Citizen[citizen_id].econ_bonus;
        base->psych_total += Citizen[citizen_id].psych_bonus;
        base->labs_total += Citizen[citizen_id].labs_bonus;
    }

    int coeff_labs = 4;
    int coeff_econ = 4;
    int coeff_psych = 4;

    if (has_fac_built(FAC_BIOLOGY_LAB, base_id)) {
        base->labs_total += conf.biology_lab_bonus;
    }
    if (has_facility(FAC_ENERGY_BANK, base_id)) {
        coeff_econ += 2;
    }
    if (has_fac_built(FAC_NETWORK_NODE, base_id)) {
        coeff_labs += 2;
    }
    if (has_fac_built(FAC_HOLOGRAM_THEATRE, base_id)
    || (has_project(FAC_VIRTUAL_WORLD, faction_id)
    && has_fac_built(FAC_NETWORK_NODE, base_id))) {
        coeff_psych += 2;
    }
    if (has_fac_built(FAC_RESEARCH_HOSPITAL, base_id)) {
        coeff_labs += 2;
        coeff_psych += 1;
    }
    if (has_fac_built(FAC_NANOHOSPITAL, base_id)) {
        coeff_labs += 2;
        coeff_psych += 1;
    }
    if (has_fac_built(FAC_TREE_FARM, base_id)) {
        coeff_econ += 2;
        coeff_psych += 2;
    }
    if (has_fac_built(FAC_HYBRID_FOREST, base_id)) {
        coeff_econ += 2;
        coeff_psych += 2;
    }
    if (has_fac_built(FAC_FUSION_LAB, base_id)) {
        coeff_labs += 2;
        coeff_econ += 2;
    }
    if (has_fac_built(FAC_QUANTUM_LAB, base_id)) {
        coeff_labs += 2;
        coeff_econ += 2;
    }
    base->labs_total = (coeff_labs * base->labs_total + 3) / 4;
    base->economy_total = (coeff_econ * base->economy_total + 3) / 4;
    base->psych_total = (coeff_psych * base->psych_total + 3) / 4;
    base->unk_total = (coeff_econ * base->unk_total + 3) / 4;

    if (project_base(FAC_SUPERCOLLIDER) == base_id) {
        base->labs_total *= 2;
    }
    if (project_base(FAC_THEORY_OF_EVERYTHING) == base_id) {
        base->labs_total *= 2;
    }
    if (project_base(FAC_SPACE_ELEVATOR) == base_id) {
        base->economy_total *= 2;
    }
    if (project_base(FAC_LONGEVITY_VACCINE) == base_id
    && Factions[faction_id].SE_Economics_pending == SOCIAL_M_FREE_MARKET) {
        base->economy_total += base->economy_total / 2;
    }
    if (project_base(FAC_NETWORK_BACKBONE) == base_id) {
        for (int i = 0; i < *BaseCount; i++) {
            if (has_fac_built(FAC_NETWORK_NODE, i)) {
                base->labs_total++;
            }
        }
        // Sanctions also prevent this bonus since no commerce would be occurring
        if (is_alien(faction_id)) {
            base->labs_total += energygrid;
        } else {
            base->labs_total += commerce;
        }
    }
    // Apply slider imbalance penalty if SE Efficiency is less than 4
    int alloc_labs = f->SE_alloc_labs;
    int alloc_psych = f->SE_alloc_psych;
    int effic_val = 4 - clamp(f->SE_effic_pending, -4, 4);
    int psych_val;
    if (2 * alloc_labs + alloc_psych - 10 < 0) {
        psych_val = (2 * (5 - alloc_labs) - alloc_psych) / 2;
    } else {
        psych_val = (2 * alloc_labs + alloc_psych - 10) / 2;
    }
    if (psych_val && effic_val) {
        int penalty = psych_val * effic_val * 2;
        if (2 * alloc_labs + alloc_psych <= 10) {
            base->labs_total = (base->labs_total * (100 - clamp(2 * penalty, 0, 80)) + 50) / 100;
            base->economy_total = (base->economy_total * (100 - clamp(penalty, 0, 40)) + 50) / 100;
        } else {
            base->labs_total = (base->labs_total * (100 - clamp(penalty, 0, 40)) + 50) / 100;
            base->economy_total = (base->economy_total * (100 - clamp(2 * penalty, 0, 80)) + 50) / 100;
        }
    }
    // Normally Stockpile Energy output would be applied here on base->economy_total
    // To avoid double production issues instead it is calculated in mod_base_production
    if (conf.base_psych) {
        mod_base_psych(base_id);
    } else {
        base_psych();
    }
}

static void add_psych_row(BASE* base, int num) {
    if (num >= 0 && num <= 4) {
        BasePsychTalents[num] = base->talent_total;
        BasePsychNDrones[num] = base->drone_total;
        BasePsychSDrones[num] = base->superdrone_total;
    }
}

static void adjust_psych(BASE* base, int talent_val, bool force) {
    const int pop_size = base->pop_size - base->specialist_total;
    while (force && talent_val > 0) {
        talent_val--;
        if (base->talent_total < pop_size) {
            base->talent_total++;
        }
        if (base->talent_total + base->drone_total + base->superdrone_total > pop_size) {
            if (base->superdrone_total > 0) {
                base->superdrone_total--;
            } else if (base->drone_total > 0) {
                base->drone_total--;
            }
        }
    }
    if (talent_val > 0) {
        base->talent_total += talent_val;
    }
    while (base->talent_total + base->drone_total + base->superdrone_total > pop_size) {
        if (base->talent_total > 0) {
            base->talent_total--;
            if (base->superdrone_total > 0) {
                base->superdrone_total--;
            } else if (base->drone_total > 0) {
                base->drone_total--;
            }
        } else {
            if (base->drone_total <= pop_size) {
                base->superdrone_total = min(base->drone_total, base->superdrone_total);
                break;
            }
            base->drone_total--;
        }
    }
    assert(base->pop_size >= base->talent_total + base->drone_total + base->specialist_total);
}

static void adjust_drone(BASE* base, int drone_val) {
    const int pop_size = base->pop_size - base->specialist_total;
    while (drone_val > 0) {
        drone_val--;
        if (base->drone_total < pop_size) {
            base->drone_total++;
        } else if (base->superdrone_total < pop_size) {
            base->superdrone_total++;
        }
        if (base->talent_total + base->drone_total > pop_size && base->talent_total > 0) {
            base->talent_total--;
        }
    }
    if (drone_val < 0) {
        base->drone_total = max(0, base->drone_total + drone_val);
    }
    base->superdrone_total = min(base->drone_total, base->superdrone_total);
    assert(base->pop_size >= base->talent_total + base->drone_total + base->specialist_total);
}

void __cdecl mod_base_psych(int base_id) {
    BASE* base = &Bases[base_id];
    Faction* f = &Factions[base->faction_id];
    MFaction* m = &MFactions[base->faction_id];
    int faction_id = base->faction_id;
    base->talent_total = 0;
    base->drone_total = 0;
    base->superdrone_total = 0;

    if (base->specialist_total >= base->pop_size) {
        for (int i = 0; i < 5; i++) add_psych_row(base, i);
        return;
    }

    // -5, Two extra drones for each military unit away from territory
    // -4, Extra drone for each military unit away from territory
    // -3, Extra drone if more than one military unit away from territory
    // -2, Cannot use military units as police. No nerve stapling.
    // -1, One police unit allowed. No nerve stapling.
    //  0, Can use one military unit as police
    //  1, Can use up to 2 military units as police
    //  2, Can use up to 3 military units as police!
    //  3, 3 units as police. Police effect doubled!!
    const int SE_police = base->SE_police(SE_Pending);
    const int num_police = clamp((SE_police == -1) + SE_police + 1, 0, 3);
    const int val_police = 1 + (SE_police >= 3);

    int rule_drone = 0;
    int rule_talent = 0;
    int drone_value = 0;
    int police_total = 0;
    int effic_drones = 0;
    int capture_drones = 0;
    int facility_value = 0;
    int content_pop, base_limit;
    mod_psych_check(faction_id, &content_pop, &base_limit);
    drone_value = max(0, base->pop_size - content_pop);

    if (base_limit) {
        int drone_limit = (base_id % base_limit + f->base_count - base_limit) / base_limit;
        effic_drones = max(0, min(drone_limit, (int)base->pop_size));
    }
    if (base->assimilation_turns_left > 0) {
        // Former faction_id can be the same but this can be also used for scenarios
        int v1 = (base->pop_size + (is_human(faction_id) ? f->diff_level : 3) - 2) / 4;
        int v2 = (base->assimilation_turns_left + 9) / 10;
        capture_drones = max(0, min(v1, v2));
        drone_value += capture_drones;
    }
    if (m->rule_drone) {
        // Extra drone at base (per "param" citizens, rounded down)
        rule_drone = base->pop_size / m->rule_drone;
        drone_value += rule_drone;
    }
    if (m->rule_talent) {
        // Extra talent at base (per "param" citizens, rounded up)
        rule_talent = (m->rule_talent + base->pop_size - 1) / m->rule_talent;
    }
    for (int i = 0; i < m->faction_bonus_count; i++) {
        // Number of drones per base made content
        if (m->faction_bonus_id[i] == RULE_NODRONE) {
            drone_value = max(0, drone_value - m->faction_bonus_val1[i]);
            break;
        }
    }
    base->drone_total = drone_value;
    base->talent_total = rule_talent;
    if (f->SE_talent_pending >= 0) {
        base->talent_total += f->SE_talent_pending;
    } else {
        base->drone_total -= f->SE_talent_pending;
    }
    base->drone_total = max(0, min(base->drone_total, (int)base->pop_size));
    base->drone_total += effic_drones;
    base->superdrone_total = base->drone_total - base->pop_size;
    base->superdrone_total = max(0, min(base->superdrone_total, (int)base->pop_size));
    base->drone_total = max(0, min(base->drone_total, (int)base->pop_size));

    adjust_psych(base, 0, 0); // while (talent_total + drone_total + superdrone_total > pop_size)
    add_psych_row(base, 0); // Unmodified / Captured Base

    // Original version (psych_val) limits psych allocation effects at twice the base size
    // Additional psych energy (addon_val) is used here with diminishing returns
    int psych_val = max(0, min(base->psych_total/2, base->pop_size - base->talent_total));
    int addon_val = base->psych_total/2 - psych_val;
    int addon_cost = 2;
    while (addon_val >= addon_cost) {
        psych_val++;
        addon_val -= addon_cost;
        addon_cost += 2;
    }
    adjust_psych(base, psych_val, 0);
    add_psych_row(base, 1); // Psych

    if (has_fac_built(FAC_GENEJACK_FACTORY, base_id)) {
        facility_value += Rules->drones_induced_genejack_factory;
    }
    if (has_fac_built(FAC_RECREATION_COMMONS, base_id)) {
        facility_value -= 2;
    }
    if (has_fac_built(FAC_HOLOGRAM_THEATRE, base_id)
    || (has_project(FAC_VIRTUAL_WORLD, faction_id)
    && has_fac_built(FAC_NETWORK_NODE, base_id))) {
        facility_value -= 2;
    }
    if (has_project(FAC_PLANETARY_TRANSIT_SYSTEM, faction_id) && base->pop_size <= 3) {
        facility_value -= 1;
    }
    if (has_fac_built(FAC_RESEARCH_HOSPITAL, base_id)) {
        facility_value -= 1;
    }
    if (has_fac_built(FAC_NANOHOSPITAL, base_id)) {
        facility_value -= 1;
    }
    adjust_drone(base, facility_value);
    if (has_fac_built(FAC_PARADISE_GARDEN, base_id)) {
        adjust_psych(base, 2, 1);
    }
    add_psych_row(base, 2); // Facilities

    // Allied units on the same tile can also apply police effects
    // These modifiers may stack and will result in three drones suppressed by one unit
    // when more than one ability is used: SE_police >= 3, ABL_POLICE_2X, RFLAG_WORMPOLICE
    std::priority_queue<int> units;
    if (SE_police >= -1) {
        if (has_project(FAC_SELF_AWARE_COLONY, faction_id)) {
            units.push({val_police});
        }
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->x == base->x && veh->y == base->y
            && veh->triad() != TRIAD_SEA && veh->plan() <= PLAN_RECON) {
                int value = val_police + (has_abil(veh->unit_id, ABL_POLICE_2X)
                    || (veh->is_native_unit() && m->rule_flags & RFLAG_WORMPOLICE));
                units.push({value});
            }
        }
        for (int i = 0; i < num_police && units.size() > 0; i++) {
            police_total += units.top();
            units.pop();
        }
        adjust_drone(base, -police_total);
    }

    if (SE_police == -3 && *BaseVehPacifismCount > 1) {
        base->drone_total = min((int)base->pop_size, base->drone_total + *BaseVehPacifismCount - 1);
    } else if (SE_police == -4 && *BaseVehPacifismCount > 0) {
        base->drone_total = min((int)base->pop_size, base->drone_total + *BaseVehPacifismCount);
    } else if (SE_police <= -5 && *BaseVehPacifismCount > 0) {
        base->drone_total = min((int)base->pop_size, base->drone_total + *BaseVehPacifismCount * 2);
    }
    adjust_psych(base, 0, 0); // Pacifism should not create additional superdrones
    add_psych_row(base, 3); // Police / Pacifism

    // Always increase the talent count when any non-specialists are available
    if (has_project(FAC_HUMAN_GENOME_PROJECT, faction_id)) {
        adjust_psych(base, 1, 1);
    }
    if (has_project(FAC_CLINICAL_IMMORTALITY, faction_id)) {
        adjust_psych(base, 1, 1);
    }
    // Planned reduces drones by two, while Simple and Green reduces by one
    if (has_project(FAC_LONGEVITY_VACCINE, faction_id)) {
        int value = (f->SE_Economics_pending == SOCIAL_M_PLANNED)
            + (f->SE_Economics_pending != SOCIAL_M_FREE_MARKET);
        adjust_drone(base, -value);
    }
    if (base->nerve_staple_turns_left > 0 || has_fac_built(FAC_PUNISHMENT_SPHERE, base_id)) {
        base->talent_total = 0;
        base->drone_total = 0;
        base->superdrone_total = 0;
    }
    add_psych_row(base, 4); // Secret Projects / Stapled Base

    debug_ver("base_psych %3d %3d pop: %2d tal: %2d dro: %2d spc: %2d eff: %d cap: %d pol: %d psy: %d\n",
    *CurrentTurn, base_id, base->pop_size, base->talent_total, base->drone_total, base->specialist_total,
    effic_drones, capture_drones, police_total, psych_val);
}

void __cdecl mod_base_energy_costs() {
    BASE* base = *CurrentBase;
    if (base && base->energy_surplus < 0) {
        MAP* sq;
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->faction_id == base->faction_id
            && veh->plan() == PLAN_SUPPLY
            && veh->order == ORDER_CONVOY
            && veh->order_auto_type == RSC_ENERGY
            && (sq = mapsq(veh->x, veh->y))
            && sq->is_base()) {
                veh->order = ORDER_NONE;
            }
        }
    }
}

/*
Remove labs start delay from factions with negative SE Research value
or when playing on two lowest difficulty levels.
*/
void __cdecl mod_base_research() {
    if (!(*GameRules & RULES_SCN_NO_TECH_ADVANCES)) {
        int faction_id = Bases[*CurrentBaseID].faction_id;
        int labs_total = Bases[*CurrentBaseID].labs_total;
        Faction* f = &Factions[faction_id];

        if (has_fac_built(FAC_PUNISHMENT_SPHERE, *CurrentBaseID)) {
            labs_total /= 2;
        }
        if (!conf.early_research_start) {
            if (*CurrentTurn < -5 * f->SE_research_pending) {
                labs_total = 0;
            }
            if (*CurrentTurn < 5 && f->diff_level < 2) {
                labs_total = 0;
            }
        }
        f->labs_total += labs_total;
        int v1 = 10 * labs_total * (clamp(f->SE_research_pending, -5, 5) + 10);
        int v2 = v1 % 100 + f->net_random_event;
        if (v2 >= 100) {
            v1 += 100;
            f->net_random_event = v2 - 100;
        } else {
            f->net_random_event = v2;
        }
        mod_tech_research(faction_id, v1 / 100);
    }
}

/*
These functions (first base_growth and then base_drones)
will only get called from production_phase > base_upkeep.
*/
int __cdecl mod_base_growth() {
    BASE* base = *CurrentBase;
    int base_id = *CurrentBaseID;
    int faction_id = base->faction_id;
    bool has_complex = has_fac_built(FAC_HAB_COMPLEX, base_id);
    bool has_dome = has_fac_built(FAC_HABITATION_DOME, base_id);
    int nutrient_cost = (base->pop_size + 1) * mod_cost_factor(faction_id, RSC_NUTRIENT, base_id);
    int pop_modifier = (has_project(FAC_ASCETIC_VIRTUES, faction_id) ? 2 : 0)
        - MFactions[faction_id].rule_population; // Positive rule_population decreases the limit
    int pop_limit = has_complex
        ? Rules->pop_limit_wo_hab_dome + pop_modifier
        : Rules->pop_limit_wo_hab_complex + pop_modifier;
    bool allow_growth = base->pop_size < pop_limit || has_dome;

    if (base->nutrients_accumulated >= 0) {
        if ((*BaseGrowthRate >= 6 || has_project(FAC_CLONING_VATS, faction_id))
        && Rules->nutrient_intake_req_citizen
        && base->nutrient_surplus >= Rules->nutrient_intake_req_citizen) {
            if (allow_growth) {
                if (base->pop_size < MaxBasePopSize) {
                    base->pop_size++;
                }
                base_compute(1);
                draw_tile(base->x, base->y, 2);
                if (base->nutrients_accumulated > nutrient_cost) {
                    base->nutrients_accumulated = nutrient_cost;
                }
                return 0;
            } // Display population limit notifications later
        }
        if (base->nutrients_accumulated >= nutrient_cost) {
            if (*BaseGrowthRate <= -3) {
                base->nutrients_accumulated = nutrient_cost;
                return 0;
            }
            if (allow_growth) {
                if (base->pop_size < MaxBasePopSize) {
                    base->pop_size++;
                }
                base->nutrients_accumulated = 0;
                base_compute(1);
                draw_tile(base->x, base->y, 2);
            } else if (base->pop_size >= pop_limit) {
                if (!has_complex) {
                    parse_says(1, Facility[FAC_HAB_COMPLEX].name, -1, -1);
                } else if (!has_dome) {
                    parse_says(1, Facility[FAC_HABITATION_DOME].name, -1, -1);
                }
                if (!has_complex || !has_dome) {
                    if (!is_alien(faction_id)) {
                        popb("POPULATIONLIMIT", WARN_STOP_POP_LIMIT_REACHED, 16, "pop_sm.pcx", 0);
                    } else {
                        popb("POPULATIONLIMIT", WARN_STOP_POP_LIMIT_REACHED, 16, "al_pop_sm.pcx", 0);
                    }
                }
                base->nutrients_accumulated = nutrient_cost / 2;
            }
        }
        base->nutrients_accumulated += base->nutrient_surplus;
        if (base->nutrient_surplus >= 0
        || base->nutrients_accumulated < base->nutrient_surplus
        || base->nutrients_accumulated + 5 * base->nutrient_surplus >= 0
        || ((base->governor_flags & GOV_ACTIVE) && (base->governor_flags & GOV_MANAGE_CITIZENS))) {
            return 0;
        }
        // Base has nutrient deficit but the population will not be reduced yet
        // These tutorial messages are adjusted to better check for various game conditions
        bool tree_farm = has_fac_built(FAC_TREE_FARM, base_id);
        int worked_farms = 0;
        int farms = 0;
        for (const auto& m : iterate_tiles(base->x, base->y, 1, 21)) {
            if (m.sq->owner != faction_id) {
                continue;
            }
            if (m.sq->items & BIT_FARM
            || (m.sq->items & BIT_FOREST && tree_farm && !is_ocean(m.sq))
            || (m.sq->items & BIT_FUNGUS && mod_crop_yield(faction_id, base_id, m.x, m.y, 0)
            >= Rules->nutrient_intake_req_citizen)) {
                farms++;
                if (base->worked_tiles & (1 << m.i)) {
                    worked_farms++;
                }
            }
        }
        const char* imagefile = is_alien(faction_id) ? "al_starv_sm.pcx" : "starv_sm.pcx";
        if ((farms < clamp(base->pop_size/2, 2, 6) || !worked_farms)
        && has_wmode(faction_id, WMODE_TERRAFORM)) {
            if (has_active_veh(faction_id, PLAN_TERRAFORM)) {
                popb("LOWNUTRIENTFARMS", 0, -1, imagefile, 0);
            } else {
                popb("LOWNUTRIENTFORMERS", 0, -1, imagefile, 0);
            }
        } else {
            if (base->specialist_total > (base->pop_size+1)/4) {
                if (base->drone_total) {
                    popb("LOWNUTRIENTSPEC1", 0, -1, imagefile, 0);
                } else {
                    popb("LOWNUTRIENTSPEC2", 0, -1, imagefile, 0);
                }
            } else {
                popb("LOWNUTRIENT", WARN_STOP_NUTRIENT_SHORTAGE, 16, imagefile, 0);
            }
        }
        return 0; // Always return here
    }
    // Reduce base population when nutrients_accumulated becomes negative
    if (full_game_turn()) {
        if (BaseResourceConvoyFrom[RSC_NUTRIENT]) {
            for (int i = 0; i < *VehCount; i++) {
                VEH* veh = &Vehs[i];
                if (veh->home_base_id == base_id && veh->is_supply()
                && veh->order == ORDER_CONVOY && !veh->moves_spent) {
                    MAP* sq = mapsq(veh->x, veh->y);
                    if (sq && sq->is_base()) {
                        veh->order = ORDER_NONE;
                    }
                }
            }
        }
        if (!is_alien(faction_id)) {
            popb("STARVE", WARN_STOP_STARVATION, 14, "starv_sm.pcx", 0);
        } else {
            popb("STARVE", WARN_STOP_STARVATION, 14, "al_starv_sm.pcx", 0);
        }
        if (base->pop_size > 1 || !is_objective(base_id)) {
            base->pop_size--;
            if (base->pop_size <= 0) {
                mod_base_kill(base_id);
                draw_map(1);
                mod_eliminate_player(faction_id, 0);
                return 1;
            }
        }
    }
    base->nutrients_accumulated = base->nutrient_surplus;
    draw_tile(base->x, base->y, 2);
    return 0;
}

void __cdecl mod_drone_riot() {
    BASE* base = *CurrentBase;
    Faction* f = &Factions[base->faction_id];
    MFaction* m = &MFactions[base->faction_id];
    bool manage_workers = base->governor_flags & GOV_ACTIVE
        && base->governor_flags & GOV_MANAGE_CITIZENS;
    bool start_riots = !(base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE);
    base->state_flags |= BSTATE_DRONE_RIOTS_ACTIVE;

    if (start_riots) {
        draw_tile(base->x, base->y, 2);
    }
    if (!start_riots || !manage_workers) {
        if (!is_alien(base->faction_id)) {
            popb("DRONERIOTS", WARN_STOP_DRONE_RIOTS, 17, "drone_sm.pcx", 0);
        } else {
            popb("DRONERIOTS", WARN_STOP_DRONE_RIOTS, 17, "ALdrone_sm.pcx", 0);
        }
    } else {
        if (!is_alien(base->faction_id)) {
            popb("DRONERIOTS2", 0, 17, "drone_sm.pcx", 0);
        } else {
            popb("DRONERIOTS2", 0, 17, "ALdrone_sm.pcx", 0);
        }
    }
    if (start_riots) {
        if (!is_human(base->faction_id)) {
            mod_base_reset(*CurrentBaseID, 0);
        }
        return; // Facilities cannot be destroyed on the first drone riot turn
    }
    int SE_police = base->SE_police(SE_Pending);
    if ((SE_police + 6 <= 0 || !(game_rand() % (SE_police + 7)))
    && !has_fac_built(FAC_HEADQUARTERS, *CurrentBaseID)
    && base->assimilation_turns_left < 45 && f->base_count > 1) {
        if (base->pop_size < 4 && game_rand() % 3) {
            return;
        }
        // Remove drone revolt event due to issues with the original code
        // This modifier is adjusted because the code assumed drone revolts as an alternative
        int modifier = base->pop_size + f->diff_level + f->ranking + 8;
        int iter = 0;
        int facility_id;
        while (true) {
            bool allow_remove = true;
            facility_id = game_rand() % 70;
            for (int i = 0; i < m->faction_bonus_count; i++) {
                if (m->faction_bonus_val1[i] == facility_id
                && (m->faction_bonus_id[i] == RULE_FACILITY
                || (m->faction_bonus_id[i] == RULE_FREEFAC
                && has_tech(Facility[facility_id].preq_tech, base->faction_id)))) {
                    allow_remove = false;
                }
            }
            if (facility_id > FAC_HEADQUARTERS && facility_id < FAC_SKY_HYDRO_LAB) {
                if (allow_remove && has_fac_built((FacilityId)facility_id, *CurrentBaseID)) {
                    set_fac((FacilityId)facility_id, *CurrentBaseID, false);
                    parse_says(1, Facility[facility_id].name, -1, -1);
                    if (!is_alien(base->faction_id)) {
                        popb("DRONERIOT", WARN_STOP_DRONE_RIOTS, -1, "drone_sm.pcx", 0);
                    } else {
                        popb("DRONERIOT", WARN_STOP_DRONE_RIOTS, -1, "ALdrone_sm.pcx", 0);
                    }
                    return;
                }
            }
            if (++iter > modifier) {
                // Skip drone revolt event here if no facility gets destroyed
                return;
            }
        }
    }
}

void __cdecl mod_base_drones() {
    BASE* base = *CurrentBase;

    if (base->golden_age()) {
        if (!(base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE)) {
            base->state_flags |= BSTATE_GOLDEN_AGE_ACTIVE;
            if (!is_alien(base->faction_id)) {
                popb("GOLDENAGE", WARN_STOP_GOLDEN_AGE, -1, "talent_sm.pcx", 0);
            } else {
                popb("GOLDENAGE", WARN_STOP_GOLDEN_AGE, -1, "Alopdir.pcx", 0);
            }
        }
    } else if (base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
        base->state_flags &= ~BSTATE_GOLDEN_AGE_ACTIVE;
        if (base->drone_total <= base->talent_total) {
            // Notify end of golden age without drone riots
            if (!is_alien(base->faction_id)) {
                popb("GOLDENAGEOVER", WARN_STOP_GOLDEN_AGE_END, -1, "talent_sm.pcx", 0);
            } else {
                popb("GOLDENAGEOVER", WARN_STOP_GOLDEN_AGE_END, -1, "Alopdir.pcx", 0);
            }
        }
    }

    if (base->drone_total <= base->talent_total || base->nerve_staple_turns_left
    || has_project(FAC_TELEPATHIC_MATRIX, base->faction_id)) {
        if (base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE) {
            base->state_flags &= ~BSTATE_DRONE_RIOTS_ACTIVE;

            if (!(base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE)) {
                bool manage = base->governor_flags & GOV_ACTIVE
                    && base->governor_flags & GOV_MANAGE_CITIZENS;

                if (!is_alien(base->faction_id)) {
                    if (manage) {
                        popb("DRONERIOTSOVER2", 0, 51, "talent_sm.pcx", 0);
                    } else {
                        popb("DRONERIOTSOVER", WARN_STOP_DRONE_RIOTS_END, 51, "talent_sm.pcx", 0);
                    }
                } else if (manage) {
                    popb("DRONERIOTSOVER2", 0, 51, "Alopdir.pcx", 0);
                } else {
                    popb("DRONERIOTSOVER", WARN_STOP_DRONE_RIOTS_END, 51, "Alopdir.pcx", 0);
                }
            }
            if (!is_human(base->faction_id)) {
                mod_base_reset(*CurrentBaseID, 0);
            }
            draw_tile(base->x, base->y, 2);
        }
    } else if (!delay_base_riot) {
        // Normally drone riots would always happen here if there are sufficient drones
        // but delay_drone_riots option can postpone it for one turn.
        mod_drone_riot();
    }
}

/*
Calculate overall maintenance cost for the currently selected base.
*/
void __cdecl mod_base_maint() {
    BASE* base = *CurrentBase;
    Faction* f = &Factions[base->faction_id];
    int faction_id = base->faction_id;

    for (int fac = 1; fac < SP_ID_First; fac++) {
        if (has_fac_built((FacilityId)fac, *CurrentBaseID)) {
            int maint = fac_maint(fac, faction_id);
            if (has_project(FAC_SELF_AWARE_COLONY, faction_id)) {
                if (f->player_flags & PFLAG_SELF_AWARE_COLONY_LOST_MAINT) {
                    maint++; // attempt to even out maintenance costs from lossy integer division
                }
                if (maint & 1) {
                    f->player_flags |= PFLAG_SELF_AWARE_COLONY_LOST_MAINT;
                } else {
                    f->player_flags &= ~PFLAG_SELF_AWARE_COLONY_LOST_MAINT;
                }
                maint /= 2;
            }
            f->energy_credits -= maint;
            f->facility_maint_total += maint;
            if (f->energy_credits < 0) {
                if (f->diff_level <= DIFF_SPECIALIST || base->queue_items[0] == -fac) {
                    f->energy_credits = 0;
                } else {
                    set_fac((FacilityId)fac, *CurrentBaseID, false);
                    f->energy_credits = Facility[fac].cost
                        * mod_cost_factor(faction_id, RSC_MINERAL, -1);
                    parse_says(1, Facility[fac].name, -1, -1);
                    popb("POWERSHORT", WARN_STOP_ENERGY_SHORTAGE, 14, "genwarning_sm.pcx", 0);
                }
            }
        }
    }
}

/*
Main base production management function during production_phase.
For the most part this follows the original logic flow as closely
as possible except when noted by comments or config options.
*/
int __cdecl mod_base_upkeep(int base_id) {
    BASE* base = &Bases[base_id];
    Faction* f = &Factions[base->faction_id];
    int faction_id = base->faction_id;

    set_base(base_id);
    *BaseUpkeepFlag = 0;
    base->state_flags &= ~(BSTATE_PSI_GATE_USED|BSTATE_FACILITY_SCRAPPED);
    base_compute(1); // Always update
    if (mod_base_production()) {
        return 1; // Current base was removed
    }
    set_base(base_id); // Fix CurrentBase sometimes being reset in game functions
    mod_base_hurry();
    base->minerals_accumulated_2 = base->minerals_accumulated;
    base->production_id_last = base->queue_items[0];

    int cur_pop = base->pop_size;
    delay_base_riot = conf.delay_drone_riots && base->talent_total >= base->drone_total
        && !(base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE);
    if (mod_base_growth()) {
        delay_base_riot = false;
        return 1; // Current base was removed
    }
    delay_base_riot = delay_base_riot && base->pop_size > cur_pop;

    if (*CurrentTurn > 1) {
        base_check_support();
    }
    *BaseUpkeepState = 1;
    assert(base == *CurrentBase);
    base_compute(1); // Always update

    int plan = f->region_base_plan[region_at(base->x, base->y)];
    int plan_value;
    if (plan != 7 || f->AI_fight > 0) {
        plan_value = (plan != 2) + 1;
    } else {
        plan_value = 4;
    }
    f->unk_45 += plan_value * __builtin_popcount(base->worked_tiles);
    *BaseUpkeepState = 2;

    if (*CurrentTurn > 0) {
        mod_base_drones();
        // base_doctors() removed as obsolete with the updated worker allocation
        if ((*CurrentBase)->faction_id != faction_id) {
            delay_base_riot = false;
            return 1; // drone_riot may change base ownership if revolt is active
        }
        mod_base_energy_costs();
        if (conf.early_research_start || *CurrentTurn > 5
        || *MultiplayerActive || !(*GamePreferences & PREF_BSC_TUTORIAL_MSGS)) {
            mod_base_research();
        }
    }
    set_base(base_id);
    base_compute(0); // Only update when the base changed
    base_ecology();
    f->energy_credits += base->economy_total;

    int commerce = 0;
    int energygrid = 0;
    if (!f->sanction_turns && !is_alien(base->faction_id)) {
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != base->faction_id && !is_alien(i) && !Factions[i].sanction_turns) {
                commerce += BaseCommerceImport[i];
            }
        }
    }
    if (!f->sanction_turns && is_alien(faction_id)) {
        energygrid = energy_grid_output(base_id);
    }
    f->turn_commerce_income += commerce;
    f->turn_commerce_income += energygrid;

    if (DEBUG) {
        int* c = BaseCommerceImport;
        debug("base_upkeep %d %d trade: %d %d %d %d %d %d %d commerce: %d grid: %d total: %d %s\n",
        *CurrentTurn, base_id, c[1], c[2], c[3], c[4], c[5], c[6], c[7],
        commerce, energygrid, f->turn_commerce_income, base->name);
    }
    if (*CurrentTurn > 1) {
        mod_base_maint();
    }
    // This is only used by the original AI terraformers
    if (!thinker_enabled(base->faction_id)) {
        base_terraform(0);
    }
    assert(base == *CurrentBase);
    assert(*ComputeBaseID == *CurrentBaseID);

    // Instead of random delay the AI always renames captured bases when they are assimilated
    if (base->state_flags & BSTATE_RENAME_BASE) {
        if (base->assimilation_turns_left <= 1 && !(*GameState & STATE_GAME_DONE)) {
            if ((base->faction_id_former == MapWin->cOwner
            || base->visibility & (1 << MapWin->cOwner))
            && !Console_focus(MapWin, base->x, base->y, MapWin->cOwner)) {
                draw_tiles(base->x, base->y, 2);
            }
            parse_says(0, parse_set(base->faction_id), -1, -1);
            parse_says(1, base->name, -1, -1);
            mod_name_base(base->faction_id, base->name, 1, is_ocean(base));
            parse_says(2, base->name, -1, -1);

            if (base->faction_id_former == MapWin->cOwner
            || base->visibility & (1 << MapWin->cOwner)) {
                draw_map(1);
                if (is_alien(base->faction_id)) {
                    popp(ScriptFile, "RENAMEBASE", 0, "al_col_sm.pcx", 0);
                } else {
                    popp(ScriptFile, "RENAMEBASE", 0, "colfoun_sm.pcx", 0);
                }
            }
            base->state_flags &= ~(BSTATE_RENAME_BASE|BSTATE_UNK_1);
            base->state_flags |= BSTATE_SKIP_RENAME;
        }
    }
    *ComputeBaseID = -1;
    if (*CurrentBaseID == *BaseUpkeepDrawID) {
        BaseWin_on_redraw(BaseWin);
    }
    if (*BaseUpkeepFlag && full_game_turn()) {
        BaseWin_zoom(BaseWin, *CurrentBaseID, 1);
    }
    *BaseUpkeepState = 0;
    if (base->assimilation_turns_left) {
        base->assimilation_turns_left--;
        if (base->faction_id_former == base->faction_id) {
            if (base->assimilation_turns_left) {
                base->assimilation_turns_left--;
            }
        }
        if (!base->assimilation_turns_left) {
            base->faction_id_former = base->faction_id;
        }
    }
    if (base->nerve_staple_turns_left) {
        base->nerve_staple_turns_left--;
    }
    // Nerve staple expires twice as fast when SE Police becomes insufficient
    if (base->nerve_staple_turns_left && f->SE_police_pending < conf.nerve_staple_mod) {
        base->nerve_staple_turns_left--;
    }
    base->state_flags &= ~(BSTATE_UNK_200000|BSTATE_COMBAT_LOSS_LAST_TURN);
    if (!((*CurrentTurn + *CurrentBaseID) & 7)) {
        base->state_flags &= ~BSTATE_UNK_100000;
    }
    delay_base_riot = false;
    return 0;
}

/*
Note that when a base is captured and either the former or current owner has
free facilities defined for the faction, all of these facilities will be added
on the base when it is captured, for example Hive bases always get Perimeter Defense.
*/
int __cdecl mod_capture_base(int base_id, int faction, int is_probe) {
    BASE* base = &Bases[base_id];
    assert(base_id >= 0 && base_id < *BaseCount);
    assert(faction >= 0 && faction < MaxPlayerNum && base->faction_id != faction);
    if (base_id < 0 || base_id >= *BaseCount || faction < 0 || faction >= MaxPlayerNum) {
        return 0;
    }
    int old_faction = base->faction_id;
    int prev_owner = base->faction_id;
    int last_spoke = *CurrentTurn - Factions[faction].diplo_spoke[old_faction];
    bool vendetta = at_war(faction, old_faction);
    bool alien_fight = is_alien(faction) != is_alien(old_faction);
    bool destroy_base = base->pop_size < 2 && !is_objective(base_id);
    set_base(base_id);

    if (!destroy_base && alien_fight) {
        base->pop_size = max(2, base->pop_size / 2);
    }
    if (!destroy_base && base->faction_id_former >= 0
    && is_alive(base->faction_id_former)
    && base->assimilation_turns_left > 0) {
        prev_owner = base->faction_id_former;
    }
    base->defend_goal = 0;
    capture_base(base_id, faction, is_probe);
    find_relocate_base(old_faction);
    if (is_probe) {
        MFactions[old_faction].thinker_last_mc_turn = *CurrentTurn;
        for (int i = *VehCount-1; i >= 0; i--) {
            if (Vehs[i].x == base->x && Vehs[i].y == base->y
            && Vehs[i].faction_id != faction && !has_pact(faction, Vehs[i].faction_id)) {
                veh_kill(i);
            }
        }
    }
    /*
    Fix issue after the faction capturing the base with Cloudbase Academy has their aircraft speed
    altered during the turn resulting in some aircraft crashing when they should not.
    */
    if (!destroy_base && project_base(FAC_CLOUDBASE_ACADEMY) == base_id) {
        for (int i = *VehCount-1; i >= 0; i--) {
            VEH* veh = &Vehs[i];
            if (veh->faction_id == faction && veh->triad() == TRIAD_AIR
            && veh->unit_id / MaxProtoFactionNum == faction) {
                int moves = veh_speed(i, 0);
                if (moves == veh->moves_spent + 2 * Rules->move_rate_roads) {
                    veh->moves_spent = moves;
                }
            }
        }
    }
    /*
    Modify captured base extra drone effect to take into account the base size.
    */
    if (!destroy_base) {
        int num = 0;
        for (int i = Fac_ID_First; i <= Fac_ID_Last; i++) {
            if (has_fac_built((FacilityId)i, base_id)) {
                num++;
            }
        }
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            if (project_base((FacilityId)i) == base_id) {
                num++;
            }
        }
        base->assimilation_turns_left = clamp((num + base->pop_size) * 5 + 10, 20, 50);
        base->faction_id_former = prev_owner;
    }
    /*
    Prevent AIs from initiating diplomacy once every turn after losing a base.
    Allow dialog if surrender is possible given the diplomacy check values.
    */
    if (!*MultiplayerActive && vendetta && is_human(faction) && !is_human(old_faction)
    && last_spoke < 10 && !*diplo_value_93FA98 && !*diplo_value_93FA24) {
        int lost_bases = 0;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && b->faction_id_former == old_faction) {
                lost_bases++;
            }
        }
        int value = max(2, 6 - last_spoke) + max(0, 6 - lost_bases)
            + (want_revenge(old_faction, faction) ? 4 : 0);
        if (random(value) > 0) {
            set_treaty(faction, old_faction, DIPLO_WANT_TO_TALK, 0);
            set_treaty(old_faction, faction, DIPLO_WANT_TO_TALK, 0);
        }
    }
    debug("capture_base %d %d old_owner: %d new_owner: %d last_spoke: %d v1: %d v2: %d\n",
        *CurrentTurn, base_id, old_faction, faction, last_spoke, *diplo_value_93FA98, *diplo_value_93FA24);
    return 0;
}

/*
Calculate the amount of content population before psych modifiers for the current faction.
*/
int __cdecl base_psych_content_pop() {
    if (*CurrentBase && is_human((*CurrentBase)->faction_id)) {
        return conf.content_pop_player[*DiffLevel];
    }
    return conf.content_pop_computer[*DiffLevel];
}

/*
Calculate the base count threshold for possible bureaucracy notifications in Console::new_base.
*/
void __cdecl mod_psych_check(int faction_id, int32_t* content_pop, int32_t* base_limit) {
    *content_pop = (is_human(faction_id) ?
        conf.content_pop_player[*DiffLevel] : conf.content_pop_computer[*DiffLevel]);
    *base_limit = (((*content_pop + 2) * max(4, 4 + Factions[faction_id].SE_effic_pending)
        * *MapAreaSqRoot) / 56) / 2;
}

char* prod_name(int item_id) {
    if (item_id >= 0) {
        return Units[item_id].name;
    } else {
        return Facility[-item_id].name;
    }
}

int prod_turns(int base_id, int item_id) {
    BASE* b = &Bases[base_id];
    assert(base_id >= 0 && base_id < *BaseCount);
    if (item_id >= 0) {
        assert(strlen(Units[item_id].name) > 0);
    } else {
        assert(item_id >= -SP_ID_Last);
    }
    int minerals = max(0, mineral_cost(base_id, item_id) - b->minerals_accumulated);
    int surplus = max(1, 10 * b->mineral_surplus);
    return 10 * minerals / surplus + ((10 * minerals) % surplus != 0);
}

int mineral_cost(int base_id, int item_id) {
    bool valid = base_id >= 0 && base_id < *BaseCount
        && item_id >= -SP_ID_Last && item_id < MaxProtoNum;
    if (valid) {
        // Take possible prototype costs into account in veh_cost
        int factor = mod_cost_factor(Bases[base_id].faction_id, RSC_MINERAL, -1);
        if (item_id >= 0) {
            return mod_veh_cost(item_id, base_id, 0) * factor;
        } else {
            return Facility[-item_id].cost * factor;
        }
    }
    assert(valid);
    return 0;
}

int hurry_cost(int base_id, int item_id, int hurry_mins) {
    BASE* b = &Bases[base_id];
    MFaction* m = &MFactions[b->faction_id];
    int mins = max(0, mineral_cost(base_id, item_id) - b->minerals_accumulated);
    int cost = (item_id < 0 ? 2*mins : mins*mins/20 + 2*mins);

    if (!conf.simple_hurry_cost && b->minerals_accumulated < Rules->retool_exemption) {
        cost *= 2;
    }
    if (item_id <= -SP_ID_First) {
        cost *= (b->minerals_accumulated < 4*mod_cost_factor(b->faction_id, RSC_MINERAL, -1) ? 4 : 2);
    }
    if (has_project(FAC_VOICE_OF_PLANET)) {
        cost *= 2;
    }
    cost = cost * m->rule_hurry / 100;
    if (hurry_mins > 0 && mins > 0) {
        return hurry_mins * cost / mins + (((hurry_mins * cost) % mins) != 0);
    }
    return 0;
}

int base_unused_space(int base_id) {
    BASE* base = &Bases[base_id];
    int limit_mod = (has_project(FAC_ASCETIC_VIRTUES, base->faction_id) ? 2 : 0)
        - MFactions[base->faction_id].rule_population;

    if (!has_fac_built(FAC_HAB_COMPLEX, base_id)) {
        return max(0, Rules->pop_limit_wo_hab_complex + limit_mod - base->pop_size);
    }
    if (!has_fac_built(FAC_HABITATION_DOME, base_id)) {
        return max(0, Rules->pop_limit_wo_hab_dome + limit_mod - base->pop_size);
    }
    return max(0, MaxBasePopSize - base->pop_size);
}

int base_growth_goal(int base_id) {
    BASE* base = &Bases[base_id];
    return clamp(24 - base->pop_size, 0, base_unused_space(base_id));
}

int stockpile_energy(int base_id) {
    BASE* base = &Bases[base_id];
    if (base_id >= 0 && base->mineral_surplus > 0) {
        if (has_project(FAC_PLANETARY_ENERGY_GRID, base->faction_id)) {
            return (5 * ((base->mineral_surplus + 1) / 2) + 3) / 4;
        } else {
            return (base->mineral_surplus + 1) / 2;
        }
    }
    return 0;
}

int stockpile_energy_active(int base_id) {
    BASE* base = &Bases[base_id];
    if (base_id >= 0 && base->item() == -FAC_STOCKPILE_ENERGY) {
        return stockpile_energy(base_id);
    }
    return 0;
}

int terraform_eco_damage(int base_id) {
    BASE* base = &Bases[base_id];
    int modifier = (has_fac_built(FAC_TREE_FARM, base_id) != 0)
        + (has_fac_built(FAC_HYBRID_FOREST, base_id) != 0);
    int value = 0;
    if (modifier == 2) {
        return 0;
    }
    for (const auto& m : iterate_tiles(base->x, base->y, 0, 21)) {
        int num = __builtin_popcount(m.sq->items & (BIT_THERMAL_BORE|BIT_ECH_MIRROR|
            BIT_CONDENSER|BIT_SOIL_ENRICHER|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD));
        if ((1 << m.i) & base->worked_tiles) {
            num *= 2;
        }
        if (m.sq->items & BIT_THERMAL_BORE) {
            num += 8;
        }
        if (m.sq->items & BIT_ECH_MIRROR) {
            num += 6;
        }
        if (m.sq->items & BIT_CONDENSER) {
            num += 4;
        }
        if (m.sq->items & BIT_FOREST) {
            num -= 1;
        }
        value += num * (2 - modifier) / 2;
    }
    return value;
}

int mineral_output_modifier(int base_id) {
    int value = 0;
    if (has_facility(FAC_QUANTUM_CONVERTER, base_id)) {
        value++; // The Singularity Inductor also possible
    }
    if (has_fac_built(FAC_ROBOTIC_ASSEMBLY_PLANT, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_GENEJACK_FACTORY, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_NANOREPLICATOR, base_id)) {
        value++;
    }
    if (has_project(FAC_BULK_MATTER_TRANSMITTER, Bases[base_id].faction_id)) {
        value++;
    }
    return value;
}

/*
Calculate potential Energy Grid output for Progenitor factions.
The base cannot receive commerce income and this bonus at the same time.
*/
int energy_grid_output(int base_id) {
    int num = 0;
    for (int i = 1; i <= MaxFacilityNum; i++) {
        if (has_fac_built((FacilityId)i, base_id)) {
            num++;
        }
    }
    for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
        if (project_base((FacilityId)i) == base_id) {
            num += 5;
        }
    }
    return num/2;
}

/*
Calculate the energy loss/inefficiency for the given energy intake in the base.
This function replaces black_market and modifies the parameters to avoid writing on the game state.
Original version always used dist=16 when the faction does not have headquarters active.
*/
int energy_intake_lost(int base_id, int energy, int32_t* effic_energy_lost) {
    BASE* base = &Bases[base_id];
    int value;
    int dist_hq = 9999;
    bool found = false;
    if (energy <= 0) {
        value = 0;
    } else {
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == base->faction_id && has_fac_built(FAC_HEADQUARTERS, i)) {
                int dist = vector_dist(Bases[i].x, Bases[i].y, base->x, base->y);
                dist_hq = min(dist, dist_hq);
                found = true;
            }
        }
    }
    if (dist_hq == 0) {
        value = 0;
    } else if (energy > 0) {
        if (!found) {
            dist_hq = clamp(Factions[base->faction_id].base_count/4 + 8, 16, 32);
        }
        bool has_creche = has_fac_built(FAC_CHILDREN_CRECHE, base_id);
        if (effic_energy_lost) {
            for (int i = 0; i < 9; i++) {
                int factor;
                if (has_creche) {
                    factor = 10 - i; // +2 on efficiency scale
                } else {
                    factor = 8 - i;
                }
                if (factor <= 0) {
                    effic_energy_lost[i] += energy;
                } else {
                    effic_energy_lost[i] += energy * dist_hq / (8 * factor);
                }
            }
        }
        int factor = 4 + Factions[base->faction_id].SE_effic_pending
            + (has_creche ? 2 : 0); // +2 on efficiency scale
        value = (factor <= 0 ? energy : clamp(energy * dist_hq / (8 * factor), 0, energy));
    }
    if (found && !effic_energy_lost) {
        assert(value == black_market(energy));
    }
    return value;
}

int satellite_output(int satellites, int pop_size, bool full_value) {
    if (full_value) {
        return max(0, min(pop_size, satellites));
    }
    return max(0, min(pop_size, (satellites + 1) / 2));
}

bool satellite_bonus(int base_id, int* nutrient, int* mineral, int* energy) {
    BASE& base = Bases[base_id];
    Faction& f = Factions[base.faction_id];

    if (f.satellites_nutrient > 0 || f.satellites_mineral > 0 || f.satellites_mineral > 0) {
        bool full_value = has_facility(FAC_AEROSPACE_COMPLEX, base_id)
            || has_project(FAC_SPACE_ELEVATOR, base.faction_id);

        *nutrient += satellite_output(f.satellites_nutrient, base.pop_size, full_value);
        *mineral += satellite_output(f.satellites_mineral, base.pop_size, full_value);
        *energy += satellite_output(f.satellites_energy, base.pop_size, full_value);
        return true;
    }
    return f.satellites_ODP > 0;
}

/*
Calculate the non-native unit morale bonus modifier provided by the base and faction for a triad.
Return Value: Morale bonus modifier
*/
int __cdecl morale_mod(int base_id, int faction_id, int triad) {
    int value = 0;
    if (triad == TRIAD_LAND) {
        if (has_fac_built(FAC_COMMAND_CENTER, base_id)
        || has_project(FAC_COMMAND_NEXUS, faction_id)) {
            value = 2;
        }
    } else if (triad == TRIAD_SEA) {
        if (has_fac_built(FAC_NAVAL_YARD, base_id)
        || has_project(FAC_MARITIME_CONTROL_CENTER, faction_id)) {
            value = 2;
        }
    } else if (triad == TRIAD_AIR) {
        if (has_fac_built(FAC_AEROSPACE_COMPLEX, base_id)
        || has_project(FAC_CLOUDBASE_ACADEMY, faction_id)) {
            value = 2;
        }
    }
    if (has_fac_built(FAC_BIOENHANCEMENT_CENTER, base_id)
    || has_project(FAC_CYBORG_FACTORY, faction_id)) {
        value += 2;
    }
    if (Factions[faction_id].SE_morale_pending < -1) {
        value /= 2;
    }
    return value;
}

/*
Calculate the count of lifecycle/psi bonuses that are provided by a base and faction.
If this value is non-zero, native units at base will receive the facility bonus in repair_phase.
Return Value: Native life modifier count
*/
int __cdecl breed_mod(int base_id, int faction_id) {
    int value = 0;
    if (has_project(FAC_XENOEMPATHY_DOME, faction_id)) {
        value++;
    }
    if (has_project(FAC_PHOLUS_MUTAGEN, faction_id)) {
        value++;
    }
    if (has_project(FAC_VOICE_OF_PLANET, faction_id)) {
        value++;
    }
    if (has_fac_built(FAC_CENTAURI_PRESERVE, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_TEMPLE_OF_PLANET, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_BIOLOGY_LAB, base_id)) {
        value++;
    }
    if (has_fac_built(FAC_BROOD_PIT, base_id)) {
        value++; // Original game did not include this contrary to datalinks documentation
    }
    if (has_fac_built(FAC_BIOENHANCEMENT_CENTER, base_id)
    || has_project(FAC_CYBORG_FACTORY, faction_id)) {
        value++;
    }
    return value;
}

/*
Calculate the new native unit lifecycle bonus modifier provided by a base and faction.
Return Value: Lifecycle bonus
*/
int __cdecl worm_mod(int base_id, int faction_id) {
    int value = breed_mod(base_id, faction_id);
    if (MFactions[faction_id].rule_psi) {
        value++;
    }
    if (has_project(FAC_DREAM_TWISTER, faction_id)) {
        value++;
    }
    if (has_project(FAC_NEURAL_AMPLIFIER, faction_id)) {
        value++;
    }
    return value;
}

/*
Determine whether the base is considered an objective for scenario victory conditions.
Return Value: Is base an objective? true/false
*/
int __cdecl is_objective(int base_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        assert(0);
        return false;
    }
    if (*GameRules & RULES_SCN_VICT_ALL_BASE_COUNT_OBJ
    || Bases[base_id].event_flags & BEVENT_OBJECTIVE) {
        return true;
    }
    if (*GameRules & RULES_SCN_VICT_SP_COUNT_OBJ) {
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            if (project_base((FacilityId)i) == base_id) {
                return true;
            }
        }
    }
    if (*GameState & STATE_SCN_VICT_BASE_FACIL_COUNT_OBJ
    && *ScnVictFacilityObj >= 0 && *ScnVictFacilityObj <= 64
    && has_fac_built((FacilityId)(*ScnVictFacilityObj), base_id)) {
        return true;
    }
    return false;
}

int __cdecl own_base_rank(int base_id) {
    assert(base_id >= 0 && base_id < *BaseCount);
    int value = Bases[base_id].energy_intake*MaxBaseNum + base_id;
    int position = 0;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == Bases[base_id].faction_id
        && Bases[i].energy_intake*MaxBaseNum + i > value) {
            position++;
        }
    }
    return position;
}

/*
Determine the faction base_id for the specified position sorted by the highest energy intake.
Return Value: base_id for the specified rank position or -1 if position is unavailable
*/
int __cdecl mod_base_rank(int faction_id, int position) {
    score_max_queue_t intake;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            intake.push({i, Bases[i].energy_intake*MaxBaseNum + i});
        }
    }
    if (position < 0 || (int)intake.size() <= position) {
        return -1;
    }
    while (--position >= 0) {
        intake.pop();
    }
    return intake.top().item_id;
}

/*
Value comparisons start with INT_MIN to enable more generic code.
Fix: original version skips check for obsol_tech while this is checked elsewhere.
*/
int __cdecl best_specialist(BASE* base, int econ_val, int labs_val, int psych_val) {
    int best_score = INT_MIN;
    int citizen_id = 0;
    for (int i = 0; i < MaxSpecialistNum; i++) {
        if (has_tech(Citizen[i].preq_tech, base->faction_id)
        && !has_tech(Citizen[i].obsol_tech, base->faction_id)
        && (Citizen[i].psych_bonus >= 2 || base->pop_size >= Rules->min_base_size_specialists)) {
            int score = econ_val * Citizen[i].econ_bonus
                + labs_val * Citizen[i].labs_bonus
                + psych_val * Citizen[i].psych_bonus;
            if (score > best_score) {
                best_score = score;
                citizen_id = i;
            }
        }
    }
    return citizen_id;
}

/*
Find the best specialist available to the current base with more weight placed on psych.
This is mostly used when the base exceeds the limit for 16 chosen specialists.
*/
int __cdecl mod_best_specialist() {
    return best_specialist(*CurrentBase, 1, 1, 2);
}

/*
Determine if the faction can build a specific facility or Secret Project in the specified base.
Checks are included to prevent SMACX specific facilities from being built in SMAC mode.
*/
int __cdecl mod_facility_avail(FacilityId item_id, int faction_id, int base_id, int queue_count) {
    // initial checks
    if (!item_id || (item_id == FAC_SKUNKWORKS && *DiffLevel <= DIFF_SPECIALIST)
    || (item_id >= SP_ID_First && *GameRules & RULES_SCN_NO_BUILDING_SP)) {
        return false; // Skunkworks removed if there are no prototype costs
    }
    if (item_id == FAC_ASCENT_TO_TRANSCENDENCE) { // at top since anyone can build it
        return voice_of_planet() && *GameRules & RULES_VICTORY_TRANSCENDENCE
            && _stricmp(MFactions[faction_id].filename, "CARETAKE"); // bug fix for Caretakers
    }
    if (!has_tech(Facility[item_id].preq_tech, faction_id)) { // Check tech for facility + SP
        return false;
    }
    // Secret Project availability
    if (!*ExpansionEnabled && (item_id == FAC_MANIFOLD_HARMONICS || item_id == FAC_NETHACK_TERMINUS
    || item_id == FAC_CLOUDBASE_ACADEMY || item_id == FAC_PLANETARY_ENERGY_GRID)) {
        return false;
    }
    if (item_id == FAC_VOICE_OF_PLANET && !_stricmp(MFactions[faction_id].filename, "CARETAKE")) {
        return false; // shifted Caretaker Ascent check to top (never reached here)
    }
    if (item_id >= SP_ID_First) {
        return project_base(item_id) == SP_Unbuilt;
    }
    // Facility availability
    if (base_id < 0) {
        return true;
    }
    if (has_fac(item_id, base_id, queue_count)) {
        return false; // already built or in queue
    }
    if (redundant(item_id, faction_id)) {
        return false; // has SP that counts as facility
    }
    switch (item_id) {
    case FAC_RECYCLING_TANKS:
        return !has_fac(FAC_PRESSURE_DOME, base_id, queue_count); // count as Recycling Tank, skip
    case FAC_TACHYON_FIELD:
        return has_fac(FAC_PERIMETER_DEFENSE, base_id, queue_count)
            || has_project(FAC_CITIZENS_DEFENSE_FORCE, faction_id); // Cumulative
    case FAC_SKUNKWORKS:
        return !(MFactions[faction_id].rule_flags & RFLAG_FREEPROTO); // no prototype costs? skip
    case FAC_HOLOGRAM_THEATRE:
        return has_fac(FAC_RECREATION_COMMONS, base_id, queue_count) // not documented in manual
            && !has_project(FAC_VIRTUAL_WORLD, faction_id); // Network Nodes replaces Theater
    case FAC_HYBRID_FOREST:
        return has_fac(FAC_TREE_FARM, base_id, queue_count); // Cumulative
    case FAC_QUANTUM_LAB:
        return has_fac(FAC_FUSION_LAB, base_id, queue_count); // Cumulative
    case FAC_NANOHOSPITAL:
        return has_fac(FAC_RESEARCH_HOSPITAL, base_id, queue_count); // Cumulative
    case FAC_PARADISE_GARDEN: // bug fix: added check
        return !has_fac(FAC_PUNISHMENT_SPHERE, base_id, queue_count); // antithetical
    case FAC_PUNISHMENT_SPHERE:
        return !has_fac(FAC_PARADISE_GARDEN, base_id, queue_count); // antithetical
    case FAC_NANOREPLICATOR:
        return has_fac(FAC_ROBOTIC_ASSEMBLY_PLANT, base_id, queue_count) // Cumulative
            || has_fac(FAC_GENEJACK_FACTORY, base_id, queue_count);
    case FAC_HABITATION_DOME:
        return has_fac(FAC_HAB_COMPLEX, base_id, queue_count); // must have Complex
    case FAC_TEMPLE_OF_PLANET:
        return has_fac(FAC_CENTAURI_PRESERVE, base_id, queue_count); // must have Preserve
    case FAC_QUANTUM_CONVERTER:
        return has_fac(FAC_ROBOTIC_ASSEMBLY_PLANT, base_id, queue_count); // Cumulative
    case FAC_NAVAL_YARD:
        return is_coast(Bases[base_id].x, Bases[base_id].y, false); // needs ocean
    case FAC_AQUAFARM:
    case FAC_SUBSEA_TRUNKLINE:
    case FAC_THERMOCLINE_TRANSDUCER:
        return *ExpansionEnabled && is_coast(Bases[base_id].x, Bases[base_id].y, false);
    case FAC_COVERT_OPS_CENTER:
    case FAC_BROOD_PIT:
    case FAC_FLECHETTE_DEFENSE_SYS:
        return *ExpansionEnabled;
    case FAC_GEOSYNC_SURVEY_POD: // SMACX only & must have Aerospace Complex
        return *ExpansionEnabled && (has_fac(FAC_AEROSPACE_COMPLEX, base_id, queue_count)
            || has_project(FAC_CLOUDBASE_ACADEMY, faction_id)
            || has_project(FAC_SPACE_ELEVATOR, faction_id));
    case FAC_SKY_HYDRO_LAB:
    case FAC_NESSUS_MINING_STATION:
    case FAC_ORBITAL_POWER_TRANS:
    case FAC_ORBITAL_DEFENSE_POD:  // must have Aerospace Complex
        return has_fac(FAC_AEROSPACE_COMPLEX, base_id, queue_count)
            || has_project(FAC_CLOUDBASE_ACADEMY, faction_id)
            || has_project(FAC_SPACE_ELEVATOR, faction_id);
    case FAC_SUBSPACE_GENERATOR: // Progenitor factions only
        return is_alien(faction_id);
    default:
        break;
    }
    return true;
}

bool can_build(int base_id, int item_id) {
    assert(base_id >= 0 && base_id < *BaseCount);
    assert(item_id > 0 && item_id <= FAC_EMPTY_SP_64);
    BASE* base = &Bases[base_id];
    int faction_id = base->faction_id;
    Faction* f = &Factions[faction_id];

    // First check strict facility availability by the original games rules
    // Then check various other usefulness conditions to avoid unnecessary builds
    if (!mod_facility_avail((FacilityId)item_id, faction_id, base_id, 0)) {
        return false;
    }
    // Stockpile Energy is selected usually if the game engine reaches the global unit limit
    if (item_id == FAC_STOCKPILE_ENERGY) {
        return random(4);
    }
    if (item_id == FAC_ASCENT_TO_TRANSCENDENCE || item_id == FAC_VOICE_OF_PLANET) {
        if (victory_done()) {
            return false;
        }
        if (is_alien(faction_id) && has_tech(Facility[FAC_SUBSPACE_GENERATOR].preq_tech, faction_id)) {
            return false;
        }
    }
    if (item_id >= SP_ID_First && item_id <= SP_ID_Last) {
        int tech_id = Facility[item_id].preq_tech;
        return tech_id == TECH_None || *DiffLevel >= conf.limit_project_start
            || (tech_id >= 0 && TechOwners[tech_id] & FactionStatus[0]);
    }
    if (item_id == FAC_HEADQUARTERS && !valid_relocate_base(base_id)) {
        return false;
    }
    if ((item_id == FAC_RECREATION_COMMONS || item_id == FAC_HOLOGRAM_THEATRE
    || item_id == FAC_PUNISHMENT_SPHERE || item_id == FAC_PARADISE_GARDEN)
    && !base_can_riot(base_id, false)) {
        return false;
    }
    if (item_id == FAC_PUNISHMENT_SPHERE && (project_base(FAC_SUPERCOLLIDER) == base_id
    || project_base(FAC_THEORY_OF_EVERYTHING) == base_id)) {
        return false;
    }
    if (*GameRules & RULES_SCN_NO_TECH_ADVANCES) {
        if (item_id == FAC_RESEARCH_HOSPITAL || item_id == FAC_NANOHOSPITAL
        || item_id == FAC_FUSION_LAB || item_id == FAC_QUANTUM_LAB) {
            return false;
        }
    }
    if (item_id == FAC_GENEJACK_FACTORY && base_can_riot(base_id, false)
    && base->talent_total + 1 < base->drone_total + Rules->drones_induced_genejack_factory) {
        return false;
    }
    if ((item_id == FAC_HAB_COMPLEX || item_id == FAC_HABITATION_DOME)
    && (base->nutrient_surplus < 0 || base_unused_space(base_id) > 1)) {
        return false;
    }
    if (item_id == FAC_PRESSURE_DOME && (*ClimateFutureChange <= 0
    || !is_shore_level(mapsq(base->x, base->y)))) {
        return false;
    }
    if (item_id >= FAC_EMPTY_FACILITY_42 && item_id <= FAC_EMPTY_FACILITY_64
    && Facility[item_id].maint >= 0) { // Negative maintenance or income is possible
        return false;
    }
    if ((item_id == FAC_COMMAND_CENTER || item_id == FAC_NAVAL_YARD)
    && (!conf.repair_base_facility || f->SE_morale < -1)) {
        return false;
    }
    if (item_id == FAC_BIOENHANCEMENT_CENTER && f->SE_morale < -1)  {
        return false;
    }
    if (item_id == FAC_COVERT_OPS_CENTER && has_project(FAC_HUNTER_SEEKER_ALGORITHM, faction_id)) {
        return false;
    }
    if (item_id == FAC_PSI_GATE && 4*facility_count(FAC_PSI_GATE, faction_id) >= f->base_count) {
        return false;
    }
    if (item_id == FAC_SUBSPACE_GENERATOR) {
        if (!is_alien(faction_id) || base->pop_size < Rules->base_size_subspace_gen) {
            return false;
        }
        int n = 0;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id && has_facility(FAC_SUBSPACE_GENERATOR, i)
            && b->pop_size >= Rules->base_size_subspace_gen
            && ++n >= Rules->subspace_gen_req) {
                return false;
            }
        }
    }
    if (item_id >= FAC_SKY_HYDRO_LAB && item_id <= FAC_ORBITAL_DEFENSE_POD) {
        int prod_num = prod_count(-item_id, faction_id, base_id);
        int goal_num = satellite_goal(faction_id, item_id);
        int built_num = satellite_count(faction_id, item_id);
        if (built_num + prod_num >= goal_num) {
            return false;
        }
    }
    if (item_id == FAC_FLECHETTE_DEFENSE_SYS && f->satellites_ODP > 0) {
        return false;
    }
    if (item_id == FAC_GEOSYNC_SURVEY_POD || item_id == FAC_FLECHETTE_DEFENSE_SYS) {
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id && i != base_id
            && map_range(base->x, base->y, b->x, b->y) <= 3
            && (has_facility(FAC_GEOSYNC_SURVEY_POD, i)
            || has_facility(FAC_FLECHETTE_DEFENSE_SYS, i)
            || b->item() == -FAC_GEOSYNC_SURVEY_POD
            || b->item() == -FAC_FLECHETTE_DEFENSE_SYS)) {
                return false;
            }
        }
    }
    return true;
}

bool can_build_unit(int base_id, int unit_id) {
    assert(base_id >= 0 && base_id < *BaseCount && unit_id >= -1);
    UNIT* u = &Units[unit_id];
    BASE* b = &Bases[base_id];
    if (unit_id >= 0 && !(unit_id < MaxProtoFactionNum
    || (unit_id / MaxProtoFactionNum) == b->faction_id)) {
        return false;
    }
    if (unit_id >= 0 && u->triad() == TRIAD_SEA
    && !adjacent_region(b->x, b->y, -1, 10, TRIAD_SEA)) {
        return false;
    }
    return (*VehCount + 32 < conf.max_veh_num) || (*VehCount + random(32) < conf.max_veh_num);
}

bool can_staple(int base_id) {
    return base_id >= 0 && conf.nerve_staple > Bases[base_id].plr_owner()
        && Bases[base_id].SE_police(SE_Current) >= 0;
}

bool base_maybe_riot(int base_id) {
    BASE* b = &Bases[base_id];
    return b->drone_riots() && base_can_riot(base_id, true);
}

bool base_can_riot(int base_id, bool allow_staple) {
    BASE* b = &Bases[base_id];
    return (!allow_staple || !b->nerve_staple_turns_left)
        && !has_project(FAC_TELEPATHIC_MATRIX, b->faction_id)
        && !has_fac_built(FAC_PUNISHMENT_SPHERE, base_id);
}

bool base_pop_boom(int base_id) {
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    if (b->nutrient_surplus < Rules->nutrient_intake_req_citizen) {
        return false;
    }
    return has_project(FAC_CLONING_VATS, b->faction_id)
        || f->SE_growth_pending
        + (has_fac_built(FAC_CHILDREN_CRECHE, base_id) ? 2 : 0)
        + (b->golden_age() ? 2 : 0) > 5;
}

bool can_use_teleport(int base_id) {
    return has_fac_built(FAC_PSI_GATE, base_id)
        && !(Bases[base_id].state_flags & BSTATE_PSI_GATE_USED);
}

bool can_link_artifact(int base_id) {
    return base_id >= 0 && ((has_fac_built(FAC_NETWORK_NODE, base_id)
        && !(Bases[base_id].state_flags & BSTATE_ARTIFACT_LINKED))
        || project_base(FAC_UNIVERSAL_TRANSLATOR) == base_id);
}

bool has_facility(FacilityId item_id, int base_id) {
    assert(base_id >= 0 && base_id < *BaseCount);
    if (item_id <= 0) { // zero slot unused
        return false;
    }
    if (item_id >= SP_ID_First) {
        return project_base(item_id) == base_id;
    }
    return has_fac_built(item_id, base_id) || redundant(item_id, Bases[base_id].faction_id);
}

bool has_free_facility(FacilityId item_id, int faction_id) {
    MFaction& m = MFactions[faction_id];
    for (int i = 0; i < m.faction_bonus_count; i++) {
        if (m.faction_bonus_val1[i] == item_id
        && (m.faction_bonus_id[i] == RULE_FACILITY
        || (m.faction_bonus_id[i] == RULE_FREEFAC
        && has_tech(Facility[item_id].preq_tech, faction_id)))) {
            return true;
        }
    }
    return false;
}

int __cdecl redundant(FacilityId item_id, int faction_id) {
    FacilityId project_id;
    switch (item_id) {
      case FAC_NAVAL_YARD:
        project_id = FAC_MARITIME_CONTROL_CENTER;
        break;
      case FAC_PERIMETER_DEFENSE:
        project_id = FAC_CITIZENS_DEFENSE_FORCE;
        break;
      case FAC_COMMAND_CENTER:
        project_id = FAC_COMMAND_NEXUS;
        break;
      case FAC_BIOENHANCEMENT_CENTER:
        project_id = FAC_CYBORG_FACTORY;
        break;
      case FAC_QUANTUM_CONVERTER:
        project_id = FAC_SINGULARITY_INDUCTOR;
        break;
      case FAC_AEROSPACE_COMPLEX:
        project_id = FAC_CLOUDBASE_ACADEMY;
        break;
      case FAC_ENERGY_BANK:
        project_id = FAC_PLANETARY_ENERGY_GRID;
        break;
      default:
        return false;
    }
    return has_project(project_id, faction_id);
}

/*
Check if the base already has a particular facility built or if it's in the queue.
*/
int __cdecl has_fac(FacilityId item_id, int base_id, int queue_count) {
    assert(base_id >= 0 && base_id < *BaseCount);
    if (item_id >= FAC_SKY_HYDRO_LAB) {
        return false;
    }
    bool is_built = has_fac_built(item_id, base_id);
    if (is_built || !queue_count) {
        return is_built;
    }
    if (base_id < 0 || queue_count <= 0 || queue_count > 10) { // added bounds checking
        return false;
    }
    for (int i = 0; i < queue_count; i++) {
        if (Bases[base_id].queue_items[i] == -item_id) {
            return true;
        }
    }
    return false;
}

int __cdecl has_fac_built(FacilityId item_id, int base_id) {
    return base_id >= 0 && item_id >= 0 && item_id <= Fac_ID_Last
        && !!(Bases[base_id].facilities_built[item_id/8] & (1 << (item_id % 8)));
}

/*
Original set_fac does not check variable bounds.
*/
void __cdecl set_fac(FacilityId item_id, int base_id, bool add) {
    if (base_id >= 0 && base_id < MaxBaseNum && item_id >= 0 && item_id <= Fac_ID_Last) {
        if (add) {
            Bases[base_id].facilities_built[item_id/8] |= (1 << (item_id % 8));
        } else {
            Bases[base_id].facilities_built[item_id/8] &= ~(1 << (item_id % 8));
        }
    }
}

/*
Calculate facility maintenance cost taking into account faction bonus facilities.
Original game calculates Command Center maintenance based on the current highest
reactor level which is unnecessary to implement here.
*/
int __cdecl fac_maint(int facility_id, int faction_id) {
    CFacility& facility = Facility[facility_id];
    MFaction& meta = MFactions[faction_id];

    for (int i = 0; i < meta.faction_bonus_count; i++) {
        if (meta.faction_bonus_val1[i] == facility_id
        && (meta.faction_bonus_id[i] == RULE_FACILITY
        || (meta.faction_bonus_id[i] == RULE_FREEFAC
        && has_tech(facility.preq_tech, faction_id)))) {
            return 0;
        }
    }
    return facility.maint;
}



