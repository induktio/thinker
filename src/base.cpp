
#include "base.h"

static bool delay_base_riot = false;


bool governor_enabled(int base_id) {
    return conf.manage_player_bases && Bases[base_id].governor_flags & GOV_MANAGES_PRODUCTION;
}

void __cdecl mod_base_reset(int base_id, bool has_gov) {
    BASE& base = Bases[base_id];
    assert(base_id >= 0 && base_id < *BaseCount);
    assert(base.defend_goal >= 0 && base.defend_goal <= 5);
    print_base(base_id);

    if (base.plr_owner() && !governor_enabled(base_id)) {
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
Performs nearly the same thing as vanilla base_build but the last 3 parameters
have been replaced with the governor force recalculate flag.
*/
int __cdecl mod_base_build(int base_id, bool has_gov) {
    BASE& base = Bases[base_id];
    int choice = 0;
    set_base(base_id);
    base_compute(1);

    if (!base.plr_owner()) {
        consider_staple(base_id);
    }
    if (base.state_flags & BSTATE_PRODUCTION_DONE || has_gov) {
        debug("BUILD NEW\n");
        choice = select_build(base_id);
    } else if (base.item() >= 0 && !can_build_unit(base_id, base.item())) {
        debug("BUILD FACILITY\n");
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
    if (update_prev || *ComputeBaseID != *CurrentBaseID) {
        *ComputeBaseID = *CurrentBaseID;
        base_support();
        base_yield();
        mod_base_nutrient();
        mod_base_minerals();
        mod_base_energy();
    }
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
    debug("base_production %d %d base: %3d credits: %d stockpile: %d %s / %s\n",
    *CurrentTurn, base->faction_id, base_id, f->energy_credits,
    (value ? output : 0), base->name, prod_name(item_id));
    return value;
}

void __cdecl mod_base_nutrient() {
    BASE* base = *CurrentBase;
    int faction_id = base->faction_id;

    *BaseGrowthRate = Factions[faction_id].SE_growth_pending;
    if (has_fac_built(FAC_CHILDREN_CRECHE, *CurrentBaseID)) {
        *BaseGrowthRate += 2; // +2 on growth scale
    }
    if (base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
        *BaseGrowthRate += 2;
    }
    base->nutrient_intake_2 += BaseResourceConvoyTo[BRT_NUTRIENT];
    base->nutrient_consumption = BaseResourceConvoyFrom[BRT_NUTRIENT]
        + base->pop_size * Rules->nutrient_intake_req_citizen;
    base->nutrient_surplus = base->nutrient_intake_2
        - base->nutrient_consumption;
    if (base->nutrient_surplus >= 0) {
        if (base->nutrients_accumulated < 0) {
            base->nutrients_accumulated = 0;
        }
    } else if (!(base->nutrients_accumulated)) {
        base->nutrients_accumulated = -1;
    }
    if (*BaseUpkeepState == 1) {
        Factions[faction_id].nutrient_surplus_total
            += clamp(base->nutrient_surplus, 0, 99);
    }
}

void __cdecl mod_base_minerals() {
    BASE* base = *CurrentBase;
    int faction_id = base->faction_id;

    base->mineral_intake_2 += BaseResourceConvoyTo[BRT_MINERAL];
    base->mineral_intake_2 = (base->mineral_intake_2
        * (mineral_output_modifier(*CurrentBaseID) + 2)) / 2;
    base->mineral_consumption = *BaseForcesMaintCost
        + BaseResourceConvoyFrom[BRT_MINERAL];
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
    int eco_dmg_reduction = (has_fac_built(FAC_NANOREPLICATOR, *CurrentBaseID)
        || has_project(FAC_SINGULARITY_INDUCTOR, faction_id)) ? 2 : 1;
    if (has_fac_built(FAC_CENTAURI_PRESERVE, *CurrentBaseID)) {
        eco_dmg_reduction++;
    }
    if (has_fac_built(FAC_TEMPLE_OF_PLANET, *CurrentBaseID)) {
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
        - Factions[faction_id].theory_of_everything)
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

    base->energy_intake_2 += BaseResourceConvoyTo[BRT_ENERGY];
    base->energy_consumption = BaseResourceConvoyFrom[BRT_ENERGY];

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
    base->energy_inefficiency = black_market(base->energy_intake_2 - base->energy_consumption);
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
        if ( i < 16 ) {
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
    && Factions[faction_id].SE_economy_pending == SOCIAL_M_FREE_MARKET) {
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
    base_psych();
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
        tech_research(faction_id, v1 / 100);
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
    int nutrient_cost = (base->pop_size + 1) * mod_cost_factor(faction_id, 0, base_id);
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
                    parse_say(1, Facility[FAC_HAB_COMPLEX].name, -1, -1);
                } else if (!has_dome) {
                    parse_say(1, Facility[FAC_HABITATION_DOME].name, -1, -1);
                }
                if (!has_complex || !has_dome) {
                    if (!is_alien(faction_id)) {
                        popb("POPULATIONLIMIT", 2048, 16, "pop_sm.pcx", 0);
                    } else {
                        popb("POPULATIONLIMIT", 2048, 16, "al_pop_sm.pcx", 0);
                    }
                }
                base->nutrients_accumulated = nutrient_cost / 2;
            }
        }
        base->nutrients_accumulated += base->nutrient_surplus;
        if (base->nutrient_surplus >= 0
        || base->nutrients_accumulated < base->nutrient_surplus
        || base->nutrients_accumulated + 4 * base->nutrient_surplus + base->nutrient_surplus >= 0
        || ((base->governor_flags & GOV_ACTIVE) && (base->governor_flags & GOV_MANAGES_CITIZENS_SPECS))) {
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
                popb("LOWNUTRIENT", 128, 16, imagefile, 0);
            }
        }
        return 0; // Always return here
    }
    // Reduce base population when nutrients_accumulated becomes negative
    if (!(*GameState & STATE_GAME_DONE) || *GameState & STATE_FINAL_SCORE_DONE) {
        if (BaseResourceConvoyFrom[BRT_NUTRIENT]) {
            for (int i = 0; i < *VehCount; i++) {
                VEH* veh = &Vehs[i];
                if (veh->home_base_id == base_id && veh->is_supply()
                && veh->order == ORDER_CONVOY && !veh->moves_spent) {
                    MAP* sq = mapsq(veh->x, veh->y);
                    if (sq && sq->is_base() && sq->veh_owner() >= 0) {
                        veh->order = ORDER_NONE;
                    }
                }
            }
        }
        if (!is_alien(faction_id)) {
            popb("STARVE", 0x4000, 14, "starv_sm.pcx", 0);
        } else {
            popb("STARVE", 0x4000, 14, "al_starv_sm.pcx", 0);
        }
        if (base->pop_size > 1 || !is_objective(base_id)) {
            base->pop_size--;
            if (base->pop_size <= 0) {
                mod_base_kill(base_id);
                draw_map(1);
                eliminate_player(faction_id, 0);
                return 1;
            }
        }
    }
    base->nutrients_accumulated = base->nutrient_surplus;
    draw_tile(base->x, base->y, 2);
    return 0;
}

void __cdecl mod_base_drones() {
    BASE* base = *CurrentBase;

    if (base->golden_age()) {
        if (!(base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE)) {
            base->state_flags |= BSTATE_GOLDEN_AGE_ACTIVE;
            if (!is_alien(base->faction_id)) {
                popb("GOLDENAGE", 32, -1, "talent_sm.pcx", 0);
            } else {
                popb("GOLDENAGE", 32, -1, "Alopdir.pcx", 0);
            }
        }
    } else if (base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE) {
        base->state_flags &= ~BSTATE_GOLDEN_AGE_ACTIVE;
        if (base->drone_total <= base->talent_total) {
            // Notify end of golden age without drone riots
            if (!is_alien(base->faction_id)) {
                popb("GOLDENAGEOVER", 64, -1, "talent_sm.pcx", 0);
            } else {
                popb("GOLDENAGEOVER", 64, -1, "Alopdir.pcx", 0);
            }
        }
    }

    if (base->drone_total <= base->talent_total || base->nerve_staple_turns_left
    || has_project(FAC_TELEPATHIC_MATRIX, base->faction_id)) {
        if (base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE) {
            base->state_flags &= ~BSTATE_DRONE_RIOTS_ACTIVE;

            if (!(base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE)) {
                bool manage = base->governor_flags & GOV_ACTIVE
                    && base->governor_flags & GOV_MANAGES_CITIZENS_SPECS;

                if (!is_alien(base->faction_id)) {
                    if (manage) {
                        popb("DRONERIOTSOVER2", 0, 51, "talent_sm.pcx", 0);
                    } else {
                        popb("DRONERIOTSOVER", 16, 51, "talent_sm.pcx", 0);
                    }
                } else if (manage) {
                    popb("DRONERIOTSOVER2", 0, 51, "Alopdir.pcx", 0);
                } else {
                    popb("DRONERIOTSOVER", 16, 51, "Alopdir.pcx", 0);
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
        drone_riot();
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
    mod_base_hurry();
    assert(base == *CurrentBase);
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
        base_doctors();
        if ((*CurrentBase)->faction_id != faction_id) {
            return 1; // drone_riot may change base ownership if revolt is active
        }
        base_energy_costs();
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
        debug("base_upkeep %d %d base: %3d trade: %d %d %d %d %d %d %d commerce: %d grid: %d total: %d %s\n",
        *CurrentTurn, base->faction_id, base_id, c[1], c[2], c[3], c[4], c[5], c[6], c[7],
        commerce, energygrid, f->turn_commerce_income, base->name);
    }
    if (*CurrentTurn > 1) {
        base_maint();
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
            MFaction* m = &MFactions[base->faction_id];
            *gender_default = m->noun_gender;
            *plurality_default = m->is_noun_plural;
            parse_says(0, m->noun_faction, -1, -1);
            parse_says(1, base->name, -1, -1);

            if (conf.new_base_names) {
                mod_name_base(base->faction_id, base->name, 1, is_ocean(base));
            } else {
                name_base(base->faction_id, base->name, 1, is_ocean(base));
            }
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
            base->state_flags |= BSTATE_UNK_80;
        }
    }
    *ComputeBaseID = -1;
    if (*CurrentBaseID == *BaseUpkeepDrawID) {
        BaseWin_on_redraw(BaseWin);
    }
    if (*BaseUpkeepFlag && (!(*GameState & STATE_GAME_DONE) || *GameState & STATE_FINAL_SCORE_DONE)) {
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
    return 0;
}

bool valid_relocate_base(int base_id) {
    int faction = Bases[base_id].faction_id;
    int best_id = -1;
    int best_score = 0;

    if (conf.auto_relocate_hq || Bases[base_id].mineral_surplus < 2) {
        return false;
    }
    if (!has_tech(Facility[FAC_HEADQUARTERS].preq_tech, faction)) {
        return false;
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* b = &Bases[i];
        if (b->faction_id == faction) {
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

void find_relocate_base(int faction) {
    if (conf.auto_relocate_hq && find_hq(faction) < 0) {
        int best_score = INT_MIN;
        int best_id = -1;
        Points bases;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction) {
                bases.insert({b->x, b->y});
            }
        }
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction) {
                int score = 4 * b->pop_size - (int)(10 * avg_range(bases, b->x, b->y))
                    - 2 * b->assimilation_turns_left;
                debug("relocate_base %s %4d %s\n",
                    MFactions[faction].filename, score, b->name);
                if (score > best_score) {
                    best_id = i;
                    best_score = score;
                }
            }
        }
        if (best_id >= 0) {
            BASE* b = &Bases[best_id];
            set_fac(FAC_HEADQUARTERS, best_id, 1);
            draw_tile(b->x, b->y, 0);
        }
    }
}

int __cdecl mod_base_kill(int base_id) {
    int old_faction = Bases[base_id].faction_id;
    assert(base_id >= 0 && base_id < *BaseCount);
    base_kill(base_id);
    find_relocate_base(old_faction);
    return 0;
}

int __cdecl mod_capture_base(int base_id, int faction, int is_probe) {
    BASE* base = &Bases[base_id];
    int old_faction = base->faction_id;
    int prev_owner = base->faction_id;
    int last_spoke = *CurrentTurn - Factions[faction].diplo_spoke[old_faction];
    bool vendetta = at_war(faction, old_faction);
    bool alien_fight = is_alien(faction) != is_alien(old_faction);
    bool destroy_base = base->pop_size < 2;
    assert(base_id >= 0 && base_id < *BaseCount);
    assert(valid_player(faction) && faction != old_faction);
    /*
    Fix possible issues with inconsistent captured base facilities
    when some of the factions have free facilities defined for them.
    */
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
    Modify captured base extra drone effect to take into account the base size.
    */
    if (!destroy_base) {
        int num = 0;
        for (int i = Fac_ID_First; i <= Fac_ID_Last; i++) {
            if (has_fac_built((FacilityId)i, base_id)) {
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
    debug("capture_base %3d %3d old_owner: %d new_owner: %d last_spoke: %d v1: %d v2: %d\n",
        *CurrentTurn, base_id, old_faction, faction, last_spoke, *diplo_value_93FA98, *diplo_value_93FA24);
    return 0;
}

/*
Calculate the amount of content population before psych modifiers for the current faction.
*/
int __cdecl base_psych_content_pop() {
    int faction = (*CurrentBase)->faction_id;
    assert(valid_player(faction));
    if (is_human(faction)) {
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
    *base_limit = (((*content_pop + 2) * (Factions[faction_id].SE_effic_pending < 0 ? 4
        : Factions[faction_id].SE_effic_pending + 4) * *MapAreaSqRoot) / 56) / 2;
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
    assert(base_id >= 0 && base_id < *BaseCount);
    // Take possible prototype costs into account in veh_cost
    if (item_id >= 0) {
        return mod_veh_cost(item_id, base_id, 0) * mod_cost_factor(Bases[base_id].faction_id, 1, -1);
    } else {
        return Facility[-item_id].cost * mod_cost_factor(Bases[base_id].faction_id, 1, -1);
    }
}

int hurry_cost(int base_id, int item_id, int hurry_mins) {
    BASE* b = &Bases[base_id];
    MFaction* m = &MFactions[b->faction_id];
    assert(base_id >= 0 && base_id < *BaseCount);
    int mins = max(0, mineral_cost(base_id, item_id) - b->minerals_accumulated);
    int cost = (item_id < 0 ? 2*mins : mins*mins/20 + 2*mins);

    if (!conf.simple_hurry_cost && b->minerals_accumulated < Rules->retool_exemption) {
        cost *= 2;
    }
    if (item_id <= -SP_ID_First) {
        cost *= (b->minerals_accumulated < 4*mod_cost_factor(b->faction_id, 1, -1) ? 4 : 2);
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

    if (!has_facility(FAC_HAB_COMPLEX, base_id)) {
        return max(0, Rules->pop_limit_wo_hab_complex + limit_mod - base->pop_size);
    }
    if (!has_facility(FAC_HABITATION_DOME, base_id)) {
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
    for (const auto& m : iterate_tiles(base->x, base->y, 0, 21)) {
        int num = __builtin_popcount(m.sq->items & (BIT_THERMAL_BORE|BIT_ECH_MIRROR|
            BIT_CONDENSER|BIT_SOIL_ENRICHER|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD));
        if ((1 << m.i) & base->worked_tiles) {
            num *= 2;
        }
        if (m.sq->items & BIT_THERMAL_BORE) {
            num += 8;
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

int __cdecl mod_best_specialist() {
    int current_bonus = INT_MIN;
    int citizen_id = 0;
    for (int i = 0; i < MaxSpecialistNum; i++) {
        if (has_tech(Citizen[i].preq_tech, (*CurrentBase)->faction_id)) {
            int bonus = Citizen[i].psych_bonus * 3;
            if ((*CurrentBase)->pop_size >= Rules->min_base_size_specialists) {
                bonus += Citizen[i].econ_bonus + Citizen[i].labs_bonus;
            }
            if (bonus > current_bonus) {
                current_bonus = bonus;
                citizen_id = i;
            }
        }
    }
    return citizen_id;
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
    int faction = base->faction_id;
    Faction* f = &Factions[faction];

    // First check strict facility availability by the original games rules
    // Then check various other usefulness conditions to avoid unnecessary builds
    if (!mod_facility_avail((FacilityId)item_id, faction, base_id, 0)) {
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
        if (is_alien(faction) && has_tech(Facility[FAC_SUBSPACE_GENERATOR].preq_tech, faction)) {
            return false;
        }
    }
    if (item_id == FAC_HEADQUARTERS && !valid_relocate_base(base_id)) {
        return false;
    }
    if ((item_id == FAC_RECREATION_COMMONS || item_id == FAC_HOLOGRAM_THEATRE
    || item_id == FAC_PUNISHMENT_SPHERE || item_id == FAC_PARADISE_GARDEN)
    && !base_can_riot(base_id, false)) {
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
    && (base->nutrient_surplus < 2 || base_unused_space(base_id) > 1)) {
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
    if (item_id == FAC_PSI_GATE && 4*facility_count(FAC_PSI_GATE, faction) >= f->base_count) {
        return false;
    }
    if (item_id == FAC_SUBSPACE_GENERATOR) {
        if (!is_alien(faction) || base->pop_size < Rules->base_size_subspace_gen) {
            return false;
        }
        int n = 0;
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction && has_facility(FAC_SUBSPACE_GENERATOR, i)
            && b->pop_size >= Rules->base_size_subspace_gen
            && ++n >= Rules->subspace_gen_req) {
                return false;
            }
        }
    }
    if (item_id >= FAC_SKY_HYDRO_LAB && item_id <= FAC_ORBITAL_DEFENSE_POD) {
        int prod_num = prod_count(-item_id, faction, base_id);
        int goal_num = satellite_goal(faction, item_id);
        int built_num = satellite_count(faction, item_id);
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
            if (b->faction_id == faction && i != base_id
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
    if (unit_id >= 0 && u->triad() == TRIAD_SEA
    && !adjacent_region(b->x, b->y, -1, 10, TRIAD_SEA)) {
        return false;
    }
    return (*VehCount + 128 < MaxVehNum) || (*VehCount + random(128) < MaxVehNum);
}

bool can_staple(int base_id) {
    int faction_id = Bases[base_id].faction_id;
    return base_id >= 0 && conf.nerve_staple > Bases[base_id].plr_owner()
        && Factions[faction_id].SE_police + 2 * has_fac_built(FAC_BROOD_PIT, base_id) >= 0;
}

/*
Check if the base might riot on the next turn, usually after a population increase.
Used only for rendering additional details about base psych status.
*/
bool base_maybe_riot(int base_id) {
    BASE* b = &Bases[base_id];

    if (!base_can_riot(base_id, true)) {
        return false;
    }
    if (!conf.delay_drone_riots && base_unused_space(base_id) > 0 && b->drone_total <= b->talent_total) {
        int cost = (b->pop_size + 1) * mod_cost_factor(b->faction_id, 0, base_id);
        return b->drone_total + 1 > b->talent_total && (base_pop_boom(base_id)
            || (b->nutrients_accumulated + b->nutrient_surplus >= cost));
    }
    return b->drone_total > b->talent_total;
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
        && (m.faction_bonus_id[i] == FCB_FREEFAC
        || (m.faction_bonus_id[i] == FCB_FREEFAC_PREQ
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
        && Bases[base_id].facilities_built[item_id/8] & (1 << (item_id % 8));
}

/*
Original set_fac does not check variable bounds.
*/
void __cdecl set_fac(FacilityId item_id, int base_id, bool add) {
    if(base_id >= 0 && item_id >= 0 && item_id <= Fac_ID_Last) {
        if (add) {
            Bases[base_id].facilities_built[item_id/8] |= (1 << (item_id % 8));
        } else {
            Bases[base_id].facilities_built[item_id/8] &= ~(1 << (item_id % 8));
        }
    }
}

/*
Calculate facility maintenance cost taking into account faction bonus facilities.
Vanilla game calculates Command Center maintenance based on the current highest
reactor level which is unnecessary to implement here.
*/
int __cdecl fac_maint(int facility_id, int faction_id) {
    CFacility& facility = Facility[facility_id];
    MFaction& meta = MFactions[faction_id];

    for (int i = 0; i < meta.faction_bonus_count; i++) {
        if (meta.faction_bonus_val1[i] == facility_id
        && (meta.faction_bonus_id[i] == FCB_FREEFAC
        || (meta.faction_bonus_id[i] == FCB_FREEFAC_PREQ
        && has_tech(facility.preq_tech, faction_id)))) {
            return 0;
        }
    }
    return facility.maint;
}

int satellite_output(int satellites, int pop_size, bool full_value) {
    if (full_value) {
        return max(0, min(pop_size, satellites));
    }
    return max(0, min(pop_size, (satellites + 1) / 2));
}

/*
Calculate satellite bonuses and return true if any satellite is active.
*/
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

int consider_staple(int base_id) {
    BASE* b = &Bases[base_id];
    if (can_staple(base_id)
    && (!un_charter() || (*SunspotDuration > 1 && *DiffLevel >= DIFF_LIBRARIAN))
    && b->nerve_staple_count < 3
    && base_can_riot(base_id, true)
    && (b->drone_total > b->talent_total || b->state_flags & BSTATE_DRONE_RIOTS_ACTIVE)
    && b->pop_size / 4 >= b->nerve_staple_count
    && (b->faction_id == b->faction_id_former || want_revenge(b->faction_id, b->faction_id_former))) {
        action_staple(base_id);
    }
    return 0;
}

int need_psych(int base_id) {
    BASE* b = &Bases[base_id];
    bool drone_riots = b->drone_total > b->talent_total || b->state_flags & BSTATE_DRONE_RIOTS_ACTIVE;
    int turns = b->faction_id != b->faction_id_former ? b->assimilation_turns_left : 0;
    if (!base_can_riot(base_id, false)) {
        return 0;
    }
    if ((drone_riots || (turns && b->drone_total > 0))
    && can_build(base_id, FAC_PUNISHMENT_SPHERE)
    && prod_turns(base_id, -FAC_PUNISHMENT_SPHERE) < 6 + b->drone_total + turns/4
    && project_base(FAC_SUPERCOLLIDER) != base_id
    && project_base(FAC_THEORY_OF_EVERYTHING) != base_id) {
        if ((turns > 10) + (turns > 20)
        + (b->energy_surplus < 4 + 2*b->pop_size)
        + (b->drone_total > 1 + b->talent_total)
        + (b->drone_total > 3 + b->talent_total)
        + (b->energy_inefficiency > b->energy_surplus)
        + (b->energy_inefficiency > 2*b->energy_surplus) >= 3) {
            return -FAC_PUNISHMENT_SPHERE;
        }
    }
    if (drone_riots) {
        if (can_build(base_id, FAC_RECREATION_COMMONS)) {
            return -FAC_RECREATION_COMMONS;
        }
        if (has_project(FAC_VIRTUAL_WORLD, b->faction_id) && can_build(base_id, FAC_NETWORK_NODE)) {
            return -FAC_NETWORK_NODE;
        }
        if (can_build(base_id, FAC_HOLOGRAM_THEATRE)
        && prod_turns(base_id, -FAC_HOLOGRAM_THEATRE) < 8 + b->drone_total) {
            return -FAC_HOLOGRAM_THEATRE;
        }
        if (can_build(base_id, FAC_PARADISE_GARDEN)
        && 2*b->energy_inefficiency < b->energy_surplus
        && prod_turns(base_id, -FAC_PARADISE_GARDEN) < 8 + b->drone_total) {
            return -FAC_PARADISE_GARDEN;
        }
    }
    return 0;
}

bool need_scouts(int x, int y, int faction, int scouts) {
    Faction* f = &Factions[faction];
    MAP* sq = mapsq(x, y);
    if (is_ocean(sq)) {
        return false;
    }
    int nearby_pods = Continents[sq->region].pods
        * min(2*f->region_territory_tiles[sq->region], Continents[sq->region].tile_count)
        / max(1, Continents[sq->region].tile_count);
    int score = max(-2, 5 - *CurrentTurn/10)
        + 2*(*MapNativeLifeForms) - 3*scouts
        + min(4, nearby_pods / 6);

    bool val = random(16) < score;
    debug("need_scouts %2d %2d value: %d score: %d scouts: %d goodies: %d native: %d\n",
        x, y, val, score, scouts, Continents[sq->region].pods, *MapNativeLifeForms);
    return val;
}

int hurry_item(int base_id, int mins, int cost) {
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    debug("hurry_item %3d %3d mins: %d cost: %d credits: %d %s / %s\n",
        *CurrentTurn, base_id, mins, cost, f->energy_credits, b->name, prod_name(b->item()));
    f->energy_credits -= cost;
    b->minerals_accumulated += mins;
    b->state_flags |= BSTATE_HURRY_PRODUCTION;

    // This also enables "zoom to base" feature in the message window
    if (b->faction_id == MapWin->cOwner) {
        parse_says(0, b->name, -1, -1);
        parse_says(1, prod_name(b->item()), -1, -1);
        popb("GOVHURRY", 0, -1, "facblt_sm.pcx", 0);
    }
    return 1;
}

int mod_base_hurry() {
    const int base_id = *CurrentBaseID;
    BASE* b = &Bases[base_id];
    Faction* f = &Factions[b->faction_id];
    AIPlans* p = &plans[b->faction_id];
    assert(b == *CurrentBase);
    const int t = b->item();
    bool is_cheap = conf.simple_hurry_cost || b->minerals_accumulated >= Rules->retool_exemption;
    bool is_project = t <= -SP_ID_First && t >= -SP_ID_Last;
    bool player_gov = is_human(b->faction_id);
    int hurry_option;

    if (player_gov) {
        if (!conf.manage_player_bases) {
            return base_hurry();
        }
        hurry_option = (b->governor_flags & GOV_ACTIVE) &&
            (b->governor_flags & GOV_MAY_HURRY_PRODUCTION) ?
            ((b->governor_flags & GOV_MAY_PROD_SP) ? 2 : 1) : 0;
    } else {
        if (!thinker_enabled(b->faction_id)) {
            return base_hurry();
        }
        hurry_option = conf.base_hurry;
    }
    if (hurry_option < (is_project ? 2 : 1)
    || t == -FAC_STOCKPILE_ENERGY || t < -SP_ID_Last
    || b->state_flags & (BSTATE_PRODUCTION_DONE | BSTATE_HURRY_PRODUCTION)) {
        return 0;
    }
    int mins = mineral_cost(base_id, t) - b->minerals_accumulated;
    int cost = hurry_cost(base_id, t, mins);
    int turns = prod_turns(base_id, t);
    int reserve = clamp(*CurrentTurn * f->base_count / 8, 20, 500)
        * (!p->contacted_factions || is_project ? 1 : 4)
        * (b->defend_goal > 3 && p->enemy_factions > 0 ? 1 : 2)
        * (has_fac_built(FAC_HEADQUARTERS, base_id) ? 1 : 2) / 16;

    if (!is_cheap || mins < 1 || cost < 1 || f->energy_credits - cost < reserve) {
        return 0;
    }
    if (is_project) {
        int delay = player_gov ? 0 : clamp((*DiffLevel < DIFF_THINKER) + random(4), 0, 3);
        int threshold = 4*mod_cost_factor(b->faction_id, 1, -1);
        WItem Wgov;
        governor_priorities(Bases[base_id], Wgov);

        if (has_project((FacilityId)-t) || turns < 2 + delay
        || (b->defend_goal > 3 && p->enemy_factions > 0)
        || (delay > 0 && b->mineral_surplus < 4)
        || b->minerals_accumulated < threshold) { // Avoid double cost threshold
            return 0;
        }
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == b->faction_id
            && Bases[i].item() == t
            && Bases[i].minerals_accumulated > b->minerals_accumulated) {
                return 0;
            }
        }
        int num = 0;
        int proj_score = 0;
        int values[256];
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            if (Facility[i].preq_tech != TECH_Disable) {
                values[num] = facility_score((FacilityId)i, Wgov) + random(8);
                if (i == -t) {
                    proj_score = values[num];
                }
                num++;
            }
        }
        std::sort(values, values+num);
        if (proj_score < max(4, values[num/2])) {
            return 0;
        }
        mins = mineral_cost(base_id, t) - b->minerals_accumulated
            - b->mineral_surplus/2 - delay*b->mineral_surplus;
        cost = hurry_cost(base_id, t, mins);

        if (cost > 0 && cost < f->energy_credits && mins > 0 && mins > b->mineral_surplus) {
            hurry_item(base_id, mins, cost);
            if (DEBUG && conf.minimal_popups) {
                return 1;
            }
            if (!(*GameState & STATE_GAME_DONE) && b->faction_id != MapWin->cOwner
            && has_treaty(b->faction_id, MapWin->cOwner, DIPLO_COMMLINK)) {
                parse_says(0, MFactions[b->faction_id].adj_name_faction, -1, -1);
                parse_says(1, Facility[-t].name, -1, -1);
                popp(ScriptFile, "DONEPROJECT", 0, "secproj_sm.pcx", 0);
            }
            return 1;
        }
        return 0;
    }
    if (b->drone_total + b->specialist_total > b->talent_total
    && t < 0 && t == need_psych(base_id)
    && cost < f->energy_credits/4) {
        return hurry_item(base_id, mins, cost);
    }
    if (t < 0 && turns > 1 && cost < f->energy_credits/8) {
        if (t == -FAC_RECYCLING_TANKS || t == -FAC_PRESSURE_DOME
        || t == -FAC_RECREATION_COMMONS || t == -FAC_TREE_FARM
        || t == -FAC_PUNISHMENT_SPHERE || t == -FAC_HEADQUARTERS) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_CHILDREN_CRECHE && base_unused_space(base_id) > 2) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_HAB_COMPLEX && base_unused_space(base_id) == 0) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_PERIMETER_DEFENSE && b->defend_goal > 2 && p->enemy_factions > 0) {
            return hurry_item(base_id, mins, cost);
        }
        if (t == -FAC_AEROSPACE_COMPLEX && has_tech(TECH_Orbital, b->faction_id)) {
            return hurry_item(base_id, mins, cost);
        }
    }
    if (t >= 0 && turns > 1 && cost < f->energy_credits/4) {
        if (Units[t].is_combat_unit()) {
            int val = (b->defend_goal > 2)
                + (b->defend_goal > 3)
                + (p->enemy_bases > 0)
                + (p->enemy_factions > 0)
                + (at_war(b->faction_id, MapWin->cOwner) > 0)
                + (cost < f->energy_credits/16)
                + 2*(!has_defenders(b->x, b->y, b->faction_id));
            if (val > 3) {
                return hurry_item(base_id, mins, cost);
            }
        }
        if (Units[t].is_former() && turns > (*CurrentTurn + base_id) % 16
        && (cost < f->energy_credits/16 || b->mineral_surplus < p->median_limit)) {
            return hurry_item(base_id, mins, cost);
        }
        if (Units[t].is_colony() && b->pop_size > 1
        && (cost < f->energy_credits/16 || turns > (*CurrentTurn + base_id) % 16)
        && (b->state_flags & BSTATE_DRONE_RIOTS_ACTIVE
        || (!base_unused_space(base_id) && b->nutrient_surplus > 1)
        || (base_can_riot(base_id, true)
        && b->specialist_total + b->drone_total > b->talent_total))) {
            return hurry_item(base_id, mins, cost);
        }
    }
    return 0;
}

bool redundant_project(int faction, int fac_id) {
    Faction* f = &Factions[faction];
    if (fac_id == FAC_PLANETARY_DATALINKS) {
        int n = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (Factions[i].base_count > 0) {
                n++;
            }
        }
        return n < 4;
    }
    if (fac_id == FAC_CITIZENS_DEFENSE_FORCE) {
        return Facility[FAC_PERIMETER_DEFENSE].maint == 0
            && facility_count(FAC_PERIMETER_DEFENSE, faction) > f->base_count/2 + 2;
    }
    if (fac_id == FAC_MARITIME_CONTROL_CENTER) {
        int n = 0;
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehicles[i];
            if (veh->faction_id == faction && veh->triad() == TRIAD_SEA) {
                n++;
            }
        }
        return n < 8 && n < f->base_count/3;
    }
    if (fac_id == FAC_HUNTER_SEEKER_ALGORITHM) {
        return f->SE_probe >= 3;
    }
    if (fac_id == FAC_LIVING_REFINERY) {
        return f->SE_support >= 3;
    }
    return false;
}

int find_satellite(int base_id) {
    const int satellites[] = {
        FAC_ORBITAL_DEFENSE_POD,
        FAC_NESSUS_MINING_STATION,
        FAC_ORBITAL_POWER_TRANS,
        FAC_SKY_HYDRO_LAB,
    };
    BASE& base = Bases[base_id];
    Faction& f = Factions[base.faction_id];
    AIPlans& p = plans[base.faction_id];
    bool has_complex = has_facility(FAC_AEROSPACE_COMPLEX, base_id)
        || has_project(FAC_SPACE_ELEVATOR, base.faction_id);

    if (!has_complex && ((base_id + *CurrentTurn)/8) & 1) {
        return 0;
    }
    if (!has_complex && !can_build(base_id, FAC_AEROSPACE_COMPLEX)) {
        return 0;
    }
    bool defense_only = clamp(p.enemy_odp - f.satellites_ODP + 7, 0, 14) > random(16);
    for (const int fac_id : satellites) {
        if (!has_tech(Facility[fac_id].preq_tech, base.faction_id)) {
            continue;
        }
        if (fac_id != FAC_ORBITAL_DEFENSE_POD && defense_only) {
            continue;
        }
        int prod_num = prod_count(-fac_id, base.faction_id, base_id);
        int built_num = satellite_count(base.faction_id, fac_id);
        int goal_num = satellite_goal(base.faction_id, fac_id);

        if (built_num + prod_num >= goal_num) {
            continue;
        }
        if (!has_complex && can_build(base_id, FAC_AEROSPACE_COMPLEX)) {
            if (min(12, prod_turns(base_id, -FAC_AEROSPACE_COMPLEX)) < 2+random(16)) {
                return -FAC_AEROSPACE_COMPLEX;
            }
            return 0;
        }
        if (has_complex && min(12, prod_turns(base_id, -fac_id)) < 2+random(16)) {
            return -fac_id;
        }
    }
    return 0;
}

int find_planet_buster(int faction) {
    int best_id = -1;
    for (int unit_id = 0; unit_id < MaxProtoNum; unit_id++) {
        UNIT* u = &Units[unit_id];
        if ((unit_id < MaxProtoFactionNum || unit_id / MaxProtoFactionNum == faction)
        && u->is_planet_buster() && mod_veh_avail(unit_id, faction, -1)
        && (best_id < 0 || u->std_offense_value() > Units[best_id].std_offense_value())) {
            best_id = unit_id;
        }
    }
    return best_id;
}

int find_project(int base_id, WItem& Wgov) {
    BASE* base = &Bases[base_id];
    Faction* f = &Factions[base->faction_id];
    int gov = base->gov_config();
    int faction = base->faction_id;
    int projs = 0;
    int nukes = 0;
    int works = 0;
    int bases = f->base_count;
    int diplo = plans[faction].diplo_flags;
    int similar_limit = (base->minerals_accumulated >= 80 ? 2 : 1);
    int unit_id = (gov & (GOV_MAY_PROD_AIR_COMBAT | GOV_MAY_PROD_AIR_DEFENSE)
        ? find_planet_buster(faction) : -1);
    int built_nukes = 0;
    int enemy_nukes = 0;
    if (unit_id >= 0) {
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->is_planet_buster()) {
                if (faction == veh->faction_id) {
                    built_nukes++;
                } else if (at_war(faction, veh->faction_id)) {
                    enemy_nukes++;
                }
            }
        }
    }
    int nuke_score = (un_charter() ? 0 : 4) + 2*f->AI_power + 2*f->AI_fight
        + clamp(enemy_nukes - f->satellites_ODP, -2, 4)
        + (diplo & DIPLO_ATROCITY_VICTIM ? 4 : 0)
        + (diplo & DIPLO_WANT_REVENGE ? 4 : 0)
        + min(4, base->mineral_surplus / 32);
    int nuke_limit = (unit_id >= 0 && bases >= 8 && nuke_score > 3 &&
        built_nukes < clamp(1 + (f->AI_fight > 0) + nuke_score/8 + bases/32, 1, 3)
        ? (bases < 32 ? 1 : 2) : 0);
    debug("find_project %d score: %d limit: %d built: %d enemy: %d\n",
        faction, nuke_score, nuke_limit, built_nukes, enemy_nukes);

    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction) {
            int t = Bases[i].item();
            if (t <= -SP_ID_First || t == -FAC_SUBSPACE_GENERATOR) {
                projs++;
            } else if (t == -FAC_SKUNKWORKS) {
                works++;
            } else if (t >= 0 && Units[t].is_planet_buster()) {
                nukes++;
            }
        }
    }
    if (unit_id >= 0 && nukes < nuke_limit
    && (!un_charter() || !random(4) || has_fac_built(FAC_SKUNKWORKS, base_id))) {
        debug("find_project %d %d %s\n", faction, unit_id, Units[unit_id].name);
        int extra_cost = !Units[unit_id].is_prototyped()
            && unit_id >= MaxProtoFactionNum ? prototype_factor(unit_id) : 0;
        bool free_protos = !extra_cost || has_fac_built(FAC_SKUNKWORKS, base_id);
        if (gov & GOV_MAY_PROD_PROTOTYPE || free_protos) {
            if (extra_cost >= 50 && gov & GOV_MAY_PROD_FACILITIES
            && can_build(base_id, FAC_SKUNKWORKS) && works < 2) {
                return -FAC_SKUNKWORKS;
            }
            if ((extra_cost <= (works ? 0 : 50)
            || has_fac_built(FAC_SKUNKWORKS, base_id))) {
                return unit_id;
            }
        }
    }
    if (projs + nukes < min(3 + nuke_limit, bases/4)) {
        if (can_build(base_id, FAC_SUBSPACE_GENERATOR)) {
            return -FAC_SUBSPACE_GENERATOR;
        }
        int best_score = INT_MIN;
        int choice = 0;
        for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
            if (can_build(base_id, i) && prod_count(-i, faction, base_id) < similar_limit
            && (similar_limit > 1 || !redundant_project(faction, i))) {
                int score = facility_score((FacilityId)i, Wgov);
                if (score > best_score) {
                    choice = i;
                    best_score = score;
                }
                debug("find_project %d %d %d %s\n", faction, i, score, Facility[i].name);
            }
        }
        return (projs > 0 || best_score > 3 ? -choice : 0);
    }
    return 0;
}

/*
Return true if unit2 is strictly better than unit1 in all circumstances (non PSI).
Disable random chance in prototype choices in these instances.
*/
bool unit_is_better(int unit_id1, int unit_id2) {
    assert(unit_id1 >= 0 && unit_id2 >= 0);
    UNIT* u1 = &Units[unit_id1];
    UNIT* u2 = &Units[unit_id2];
    bool val = (u1->cost >= u2->cost
        && offense_value(unit_id1) >= 0
        && offense_value(unit_id1) <= offense_value(unit_id2)
        && defense_value(unit_id1) <= defense_value(unit_id2)
        && Chassis[u1->chassis_id].speed <= Chassis[u2->chassis_id].speed
        && (Chassis[u2->chassis_id].triad != TRIAD_AIR || u1->chassis_id == u2->chassis_id)
        && !((u1->ability_flags & u2->ability_flags) ^ u1->ability_flags))
        && (u1->ability_flags & ABL_SLOW) <= (u2->ability_flags & ABL_SLOW);
    if (val) {
        debug("unit_is_better %s -> %s\n", u1->name, u2->name);
    }
    return val;
}

int unit_score(BASE* base, int unit_id, int psi_score, bool defend) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    const int specials[][2] = {
        {ABL_AAA, 4},
        {ABL_AIR_SUPERIORITY, 2},
        {ABL_ALGO_ENHANCEMENT, 5},
        {ABL_AMPHIBIOUS, -2},
        {ABL_DROP_POD, 4},
        {ABL_EMPATH, 2},
        {ABL_TRANCE, 3},
        {ABL_SLOW, -4},
        {ABL_TRAINED, 2},
        {ABL_COMM_JAMMER, 3},
        {ABL_CLEAN_REACTOR, 2},
        {ABL_ANTIGRAV_STRUTS, 3},
        {ABL_BLINK_DISPLACER, 3},
        {ABL_DEEP_PRESSURE_HULL, 2},
        {ABL_SUPER_TERRAFORMER, 8},
    };
    UNIT* u = &Units[unit_id];
    int v = 16 * (defend ? defense_value(unit_id) : offense_value(unit_id));
    if (v < 0) {
        v = 12 * (defend ? Armor[best_armor(base->faction_id, -1)].defense_value
            : Weapon[best_weapon(base->faction_id)].offense_value)
            * (conf.ignore_reactor_power ? REC_FISSION : best_reactor(base->faction_id))
            + psi_score * 16;
    }
    if (u->triad() != TRIAD_AIR) {
        v += (defend ? 12 : 32) * u->speed();
        if (u->triad() == TRIAD_SEA && u->is_combat_unit()
        && defense_value(unit_id) > offense_value(unit_id)) {
            v -= 24;
        }
    }
    if (u->ability_flags & ABL_ARTILLERY) {
        if (conf.long_range_artillery > 0 && Rules->artillery_max_rng <= 4
        && !*MultiplayerActive && u->triad() == TRIAD_SEA && u->offense_value() > 0) {
            v += (conf.long_range_artillery > 1 ? 24 : 16);
        } else {
            v -= 8;
        }
    }
    if (u->ability_flags & ABL_POLICE_2X && need_police(base->faction_id)) {
        v += (u->speed() > 1 ? 16 : 32);
    }
    if (u->is_missile()) {
        v -= 8 * plans[base->faction_id].missile_units;
    }
    for (const int* s : specials) {
        if (u->ability_flags & s[0]) {
            v += 8 * s[1];
        }
    }
    // More detailed prototype cost calculations are skipped here
    int turns = max(0, u->cost*10 - base->minerals_accumulated) / max(2, base->mineral_surplus);
    int score = v - turns * (u->is_colony() ? 6 : 3)
        * (max(2, 7 - *CurrentTurn/16) + max(0, 2 - base->mineral_surplus/4));
    debug("unit_score %3d psi: %d cost: %d turns: %d score: %d %s\n",
        unit_id, psi_score, u->cost, turns, score, u->name);
    return score;
}

/*
Find the best prototype for base production when weighted against cost given the triad
and type constraints. For any combat-capable unit, mode is set to WMODE_COMBAT.
This assumes only Scout Patrol as the fallback choice for land combat units.
For all other unit types requirements for building the prototype are always checked.
*/
int find_proto(int base_id, Triad triad, VehWeaponMode mode, bool defend) {
    assert(base_id >= 0 && base_id < *BaseCount);
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    debug("find_proto faction: %d triad: %d mode: %d defend: %d\n", faction, triad, mode, defend);

    int gov = base->gov_config();
    int psi_score = plans[faction].psi_score
        + 2*(has_fac_built(FAC_BROOD_PIT, base_id)
        + has_fac_built(FAC_BIOLOGY_LAB, base_id)
        + has_fac_built(FAC_CENTAURI_PRESERVE, base_id)
        + has_fac_built(FAC_TEMPLE_OF_PLANET, base_id));
    bool prototypes = (gov & GOV_MAY_PROD_PROTOTYPE) || has_fac_built(FAC_SKUNKWORKS, base_id);
    bool combat = (mode == WMODE_COMBAT);
    bool pacifism = combat && triad == TRIAD_AIR
        && base_can_riot(base_id, true)
        && Factions[faction].SE_police + 2*has_fac_built(FAC_BROOD_PIT, base_id) < -3
        && base->drone_total+2 >= base->talent_total;
    int best_id;
    int best_val;

    if (combat && triad == TRIAD_LAND) {
        best_id = BSC_SCOUT_PATROL;
        best_val = (mod_veh_avail(best_id, faction, base_id) ? 0 : -50)
            + unit_score(base, best_id, psi_score, defend);
    } else {
        best_id = -FAC_STOCKPILE_ENERGY;
        best_val = -10000;
    }
    for (int id = 0; id < MaxProtoNum; id++) {
        UNIT* u = &Units[id];
        if ((id < MaxProtoFactionNum || id / MaxProtoFactionNum == faction)
        && mod_veh_avail(id, faction, base_id) && u->triad() == triad && id != best_id) {
            if (!prototypes && id >= MaxProtoFactionNum && !u->is_prototyped()
            && prototype_factor(id) > 0) {
                continue;
            }
            if ((!combat && Weapon[u->weapon_id].mode != mode)
            || (combat && Weapon[u->weapon_id].offense_value == 0)
            || (combat && defend && u->chassis_id != CHS_INFANTRY)
            || (u->is_psi_unit() && psi_score <= 0)
            || u->is_planet_buster()) {
                continue;
            }
            if (combat && best_id >= 0 && best_id != BSC_SCOUT_PATROL
            && ((defend && offense_value(id) > defense_value(id))
            || (!defend && offense_value(id) < defense_value(id)))) {
                continue;
            }
            if (combat && triad == TRIAD_AIR) {
                bool intercept = has_abil(id, ABL_AIR_SUPERIORITY);
                if ((!intercept && pacifism)
                || (!intercept && !(gov & GOV_MAY_PROD_AIR_COMBAT))
                || (intercept && !(gov & GOV_MAY_PROD_AIR_DEFENSE))) {
                    continue;
                }
            }
            int val = unit_score(base, id, psi_score, defend);
            if (best_id < 0 || unit_is_better(best_id, id) || random(128) > 64 + best_val - val) {
                best_id = id;
                best_val = val;
            }
        }
    }
    return best_id;
}

Triad select_colony(int base_id, int pods, bool build_ships) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    bool land = has_base_sites(base->x, base->y, faction, TRIAD_LAND);
    bool sea = has_base_sites(base->x, base->y, faction, TRIAD_SEA);
    int limit = (*CurrentTurn < 60 || (!random(3) && (land || (build_ships && sea))) ? 2 : 1);

    if (pods >= limit) {
        return TRIAD_NONE;
    }
    if (is_ocean(base)) {
        for (const auto& m : iterate_tiles(base->x, base->y, 1, 9)) {
            if (land && (!m.sq->is_owned() || (m.sq->owner == faction && !random(6)))
            && (m.sq->veh_owner() < 0 || m.sq->veh_owner() == faction)) {
                return TRIAD_LAND;
            }
        }
        if (sea) {
            return TRIAD_SEA;
        }
    } else {
        if (build_ships && sea && (!land || (*CurrentTurn > 50 && !random(3)))) {
            return TRIAD_SEA;
        } else if (land) {
            return TRIAD_LAND;
        }
    }
    return TRIAD_NONE;
}

int select_combat(int base_id, int num_probes, bool sea_base, bool build_ships) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    AIPlans* p = &plans[faction];
    int gov = base->gov_config();
    int choice;
    int w_air = 4*p->air_combat_units < f->base_count ? 2 : 5;
    int w_sea = (p->transport_units < 2 || 5*p->transport_units < f->base_count
        ? 2 : (3*p->transport_units < f->base_count ? 5 : 8));
    bool need_ships = 6*p->sea_combat_units < p->land_combat_units;
    bool reserve = base->mineral_surplus >= base->mineral_intake_2/2;
    bool probes = has_wmode(faction, WMODE_PROBE) && gov & GOV_MAY_PROD_PROBES;
    bool transports = has_wmode(faction, WMODE_TRANSPORT) && gov & GOV_MAY_PROD_TRANSPORT;
    bool land = gov & (GOV_MAY_PROD_LAND_COMBAT | GOV_MAY_PROD_LAND_DEFENSE);
    bool sea = gov & (GOV_MAY_PROD_NAVAL_COMBAT | GOV_MAY_PROD_TRANSPORT);
    bool air = gov & (GOV_MAY_PROD_AIR_COMBAT | GOV_MAY_PROD_AIR_DEFENSE);

    if (probes && (!(land || sea || air) || !random(num_probes*2 + 4) || !reserve)
    && (choice = find_proto(base_id, (build_ships ? TRIAD_SEA : TRIAD_LAND), WMODE_PROBE, DEF)) >= 0) {
        return choice;
    }
    if (air && (!(land || sea) || !random(w_air))
    && (choice = find_proto(base_id, TRIAD_AIR, WMODE_COMBAT, ATT)) >= 0) {
        return choice;
    }
    if (build_ships && sea) {
        int min_dist = INT_MAX;
        bool sea_enemy = false;

        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (faction != b->faction_id && !has_pact(faction, b->faction_id)) {
                int dist = map_range(base->x, base->y, b->x, b->y)
                    * (at_war(faction, b->faction_id) ? 1 : 4);
                if (dist < min_dist) {
                    sea_enemy = is_ocean(b);
                    min_dist = dist;
                }
            }
        }
        if (!land || (random(4) < (sea_base ? 3 : 1 + (need_ships || sea_enemy)))) {
            VehWeaponMode mode;
            if (!transports) {
                mode = WMODE_COMBAT;
            } else if (!(gov & GOV_MAY_PROD_NAVAL_COMBAT)) {
                mode = WMODE_TRANSPORT;
            } else {
                mode = (!random(w_sea) ? WMODE_TRANSPORT : WMODE_COMBAT);
            }
            if ((choice = find_proto(base_id, TRIAD_SEA, mode, ATT)) >= 0) {
                return choice;
            }
        }
    }
    return find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, (sea_base || !random(5) ? DEF : ATT));
}

void push_item(BASE* base, score_max_queue_t& builds, int item_id, int score, int modifier) {
    if (item_id >= 0) {
        score -= 2*Units[item_id].cost;
    } else if (item_id >= -FAC_ORBITAL_DEFENSE_POD) {
        int turns = max(0, 10*Facility[-item_id].cost - base->minerals_accumulated)
            / max(2, base->mineral_surplus);
        score -= turns*turns/4;
        score -= 2*Facility[-item_id].cost;
        score -= 8*Facility[-item_id].maint;
    }
    if (modifier > 0) {
        score += 40*modifier;
    }
    builds.push({item_id, score});
    debug("push_item %3d %3d %s\n", item_id, score, prod_name(item_id));
}

int select_build(int base_id) {
    BASE* base = &Bases[base_id];
    int faction = base->faction_id;
    Faction* f = &Factions[faction];
    AIPlans* p = &plans[faction];
    if (base->plr_owner()) {
        plans_upkeep(faction);
    }
    int gov = base->gov_config();
    int minerals = base->mineral_surplus + base->minerals_accumulated/10;
    int reserve = max(2, base->mineral_intake_2 / 2);
    int content_pop_value = (base->plr_owner() ? conf.content_pop_player[*DiffLevel]
        : conf.content_pop_computer[*DiffLevel]);
    int base_reg = region_at(base->x, base->y);
    int defend_range = (base->defend_range > 0 ?  base->defend_range : MaxEnemyRange/2);
    bool sea_base = base_reg >= MaxRegionLandNum;
    bool core_base = minerals >= p->project_limit;
    bool project_change = base->item_is_project()
        && !can_build(base_id, -base->item())
        && !(base->state_flags & BSTATE_PRODUCTION_DONE)
        && base->minerals_accumulated > Rules->retool_exemption;
    bool allow_units = can_build_unit(base_id, -1) && !project_change;
    bool allow_supply = !sea_base && gov & GOV_MAY_PROD_TERRAFORMERS;
    bool allow_build = gov & GOV_MAY_PROD_FACILITIES;
    bool allow_ships = has_ships(faction)
        && adjacent_region(base->x, base->y, -1, *MapAreaSqRoot + 16, TRIAD_SEA);
    bool allow_pods = allow_expand(faction) && (base->pop_size > 1 || base->nutrient_surplus > 1);
    int all_crawlers = 0;
    int near_formers = 0;
    int need_ferry = 0;
    int transports = 0;
    int landprobes = 0;
    int seaprobes = 0;
    int defenders = 0;
    int formers = 0;
    int scouts = 0;
    int pods = 0;

    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehicles[i];
        if (veh->faction_id != faction) {
            continue;
        }
        if (veh->home_base_id == base_id) {
            if (veh->is_former()) {
                formers++;
            } else if (veh->is_colony()) {
                pods++;
            } else if (veh->is_probe()) {
                if (veh->triad() == TRIAD_LAND) {
                    landprobes++;
                } else {
                    seaprobes++;
                }
            } else if (veh->is_transport()) {
                transports++;
            } else if (veh->is_supply() && veh->order != ORDER_CONVOY) {
                allow_supply = false;
            }
        }
        if (veh->is_combat_unit() && veh->triad() == TRIAD_LAND) {
            if (base->x == veh->x && base->y == veh->y) {
                defenders += 2;
            } else if (map_range(base->x, base->y, veh->x, veh->y) <= 1) {
                defenders += 1;
            }
            if (veh->home_base_id == base_id) {
                scouts++;
            }
        } else if (veh->is_former() && veh->home_base_id != base_id) {
            if (map_range(base->x, base->y, veh->x, veh->y) <= 1) {
                near_formers++;
            }
        } else if (veh->is_supply()) {
            all_crawlers++;
        }
        if (sea_base && veh->x == base->x && veh->y == base->y && veh->triad() == TRIAD_LAND) {
            if (veh->is_colony() || veh->is_former() || veh->is_supply()) {
                need_ferry++;
            }
        }
    }
    WItem Wgov;
    governor_priorities(Bases[base_id], Wgov);
    need_ferry = need_ferry && !transports
        && adjacent_region(base->x, base->y, faction, 16, TRIAD_LAND);

    float Wbase = clamp(1.0f * minerals / p->project_limit, 0.4f, 1.0f)
        * (defend_range > 0 && defend_range < 8 ? 4 : 1)
        * max(0.05f, 2.0f * p->enemy_mil_factor / (p->enemy_base_range * 0.1f + 0.1f)
        + min(1.0f, 1.5f * f->base_count / *MapAreaSqRoot)
        + 0.8f * p->enemy_bases + 0.2f * Wgov.AI_fight);

    if (base->plr_owner()) {
        Wbase *= (gov & GOV_PRIORITY_CONQUER ? 4 : 1);
    } else {
        Wbase *= (base_reg != p->main_region && base_reg == p->target_land_region ? 4 : 1);
    }
    float Wthreat = 1 - (1 / (1 + Wbase));

    debug("select_build %d %d %2d %2d def: %d frm: %d prb: %d crw: %d pods: %d expand: %d "\
        "scouts: %d min: %2d res: %2d limit: %2d mil: %.4f threat: %.4f\n",
        *CurrentTurn, faction, base->x, base->y,
        defenders, formers, landprobes+seaprobes, all_crawlers, pods, allow_pods,
        scouts, minerals, reserve, p->project_limit, p->enemy_mil_factor, Wthreat);

    const int SecretProject = -1;
    const int Satellites = -2;
    const int MorePsych = -3;
    const int DefendUnit = -4;
    const int CombatUnit = -5;
    const int ColonyUnit = -6;
    const int FormerUnit = -7;
    const int FerryUnit = -8;
    const int CrawlerUnit = -9;
    const int SeaProbeUnit = -10;

    const PItem build_order[] = { // E  D  B  C  W
        {DefendUnit,                 0, 0, 0, 0, 0},
        {MorePsych,                  0, 0, 0, 0, 0},
        {FAC_PRESSURE_DOME,          4, 4, 4, 4, 0},
        {FAC_HEADQUARTERS,           4, 4, 4, 4, 0},
        {CombatUnit,                 0, 0, 0, 4, 0},
        {Satellites,                 2, 2, 2, 2, 0},
        {FormerUnit,                 2, 0, 2, 0, 0},
        {FAC_RECYCLING_TANKS,        4, 4, 4, 0, 0},
        {SeaProbeUnit,               2, 0, 0, 3, 0},
        {CrawlerUnit,                0, 3, 3, 0, 0},
        {FerryUnit,                  2, 0, 0, 2, 0},
        {ColonyUnit,                 4, 1, 1, 0, 0},
        {FAC_RECREATION_COMMONS,     4, 4, 4, 0, 0},
        {SecretProject,              3, 3, 3, 3, 0},
        {FAC_CHILDREN_CRECHE,        2, 2, 2, 0, 0},
        {FAC_HAB_COMPLEX,            4, 4, 4, 0, 0},
        {FAC_NETWORK_NODE,           2, 4, 2, 0, 2},
        {FAC_PERIMETER_DEFENSE,      2, 2, 2, 4, 0},
        {FAC_HOLOGRAM_THEATRE,       3, 3, 3, 0, 1},
        {FAC_AEROSPACE_COMPLEX,      0, 0, 3, 3, 0},
        {FAC_TREE_FARM,              2, 2, 2, 0, 3},
        {FAC_GENEJACK_FACTORY,       0, 1, 3, 1, 0},
        {FAC_ROBOTIC_ASSEMBLY_PLANT, 0, 1, 3, 1, 0},
        {FAC_NANOREPLICATOR,         0, 1, 3, 1, 0},
        {FAC_QUANTUM_CONVERTER,      0, 1, 3, 1, 0},
        {FAC_HABITATION_DOME,        4, 4, 4, 0, 0},
        {FAC_TACHYON_FIELD,          0, 0, 3, 4, 0},
        {FAC_GEOSYNC_SURVEY_POD,     0, 0, 3, 4, 0},
        {FAC_FLECHETTE_DEFENSE_SYS,  0, 0, 3, 4, 0},
        {FAC_BIOENHANCEMENT_CENTER,  0, 0, 0, 3, 0},
        {FAC_COMMAND_CENTER,         0, 0, 0, 3, 0},
        {FAC_NAVAL_YARD,             0, 0, 0, 3, 0},
        {FAC_PSI_GATE,               0, 0, 3, 3, 0},
        {FAC_FUSION_LAB,             2, 4, 2, 0, 4},
        {FAC_QUANTUM_LAB,            2, 4, 2, 0, 4},
        {FAC_ENERGY_BANK,            0, 2, 2, 0, 2},
        {FAC_RESEARCH_HOSPITAL,      0, 3, 2, 0, 3},
        {FAC_NANOHOSPITAL,           0, 3, 2, 0, 3},
        {FAC_HYBRID_FOREST,          2, 2, 2, 0, 3},
        {FAC_BIOLOGY_LAB,            3, 2, 0, 0, 0},
        {FAC_PARADISE_GARDEN,        2, 2, 0, 0, 0},
        {FAC_CENTAURI_PRESERVE,      3, 0, 0, 0, 0},
        {FAC_COVERT_OPS_CENTER,      0, 0, 0, 3, 0},
        {FAC_EMPTY_FACILITY_42,      0, 2, 2, 0, 0},
        {FAC_EMPTY_FACILITY_43,      0, 2, 2, 0, 0},
        {FAC_EMPTY_FACILITY_44,      0, 2, 2, 0, 0},
        {FAC_EMPTY_FACILITY_45,      0, 2, 2, 0, 0},
    };
    score_max_queue_t builds;
    int Wenergy = has_fac_built(FAC_PUNISHMENT_SPHERE, base_id) ? 1 : 2;
    int Wt = 6;

    for (const auto& item : build_order) {
        const int t = item.item_id;
        int choice = 0;
        int score = random(32)
            + 4*(Wgov.AI_growth * item.explore + Wgov.AI_tech * item.discover
            + Wgov.AI_wealth * item.build + Wgov.AI_power * item.conquer);

        if (t >= 0 && !(allow_build && can_build(base_id, t))) {
            continue;
        }
        if (t <= DefendUnit && !allow_units) {
            continue;
        }
        if (t == MorePsych && gov & GOV_MAY_PROD_FACILITIES) {
            if ((choice = need_psych(base_id)) != 0) {
                return choice;
            }
        }
        if (t == Satellites && allow_build && minerals >= p->median_limit) {
            if ((choice = find_satellite(base_id)) != 0) {
                score += 2*min(40, f->base_count - 10);
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == SecretProject && core_base && gov & GOV_MAY_PROD_SP) {
            if ((choice = find_project(base_id, Wgov)) != 0) {
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == DefendUnit && gov & GOV_ALLOW_COMBAT && minerals > 1 + (defenders > 0)) {
            if (((gov & GOV_MAY_PROD_LAND_DEFENSE && defenders < 2)
            || (gov & GOV_MAY_PROD_EXPLORE_VEH && need_scouts(base->x, base->y, faction, scouts)))
            && (choice = find_proto(base_id, TRIAD_LAND, WMODE_COMBAT, DEF)) >= 0) {
                return choice;
            }
        }
        if (t == CombatUnit && gov & GOV_ALLOW_COMBAT && minerals > reserve) {
            if ((choice = select_combat(base_id, landprobes+seaprobes, sea_base, allow_ships)) >= 0) {
                if (random(256) < (int)(256 * Wthreat)) {
                    return choice;
                }
                score += 4*base->defend_goal - defend_range;
                push_item(base, builds, choice, score, 0);
                continue;
            }
        }
        if (t == FormerUnit && gov & GOV_MAY_PROD_TERRAFORMERS) {
            if (has_wmode(faction, WMODE_TERRAFORM)
            && formers + near_formers/2 < (base->pop_size < 4 ? 1 : 2)) {
                int num = 0;
                int sea = 0;
                for (const auto& m : iterate_tiles(base->x, base->y, 1, 21)) {
                    if (m.sq->owner == faction
                    && select_item(m.x, m.y, faction, FM_Auto_Full, m.sq) >= 0) {
                        num++;
                        sea += is_ocean(m.sq);
                    }
                }
                if (num < 4) {
                    continue;
                }
                score += 8 * (num-6);
                if (has_chassis(faction, CHS_GRAVSHIP)
                && (choice = find_proto(base_id, TRIAD_AIR, WMODE_TERRAFORM, DEF)) >= 0
                && Units[choice].triad() == TRIAD_AIR) {
                    push_item(base, builds, choice, score, --Wt);
                }
                if ((sea*2 >= num || sea_base) && has_ships(faction)
                && (choice = find_proto(base_id, TRIAD_SEA, WMODE_TERRAFORM, DEF)) >= 0) {
                    push_item(base, builds, choice, score, --Wt);
                    continue;
                }
                if (!sea_base
                && (choice = find_proto(base_id, TRIAD_LAND, WMODE_TERRAFORM, DEF)) >= 0) {
                    push_item(base, builds, choice, score, --Wt);
                    continue;
                }
            }
        }
        if (t == SeaProbeUnit && gov & GOV_MAY_PROD_PROBES) {
            if (allow_ships && has_wmode(faction, WMODE_PROBE)
            && p->unknown_factions > 1 && p->contacted_factions < 2
            && adjacent_region(base->x, base->y, -1, *MapAreaTiles/16, TRIAD_SEA)
            && (choice = find_proto(base_id, TRIAD_SEA, WMODE_PROBE, DEF)) >= 0) {
                score += 32*(p->unknown_factions - seaprobes) - 2*p->probe_units;
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == CrawlerUnit && allow_supply && has_wmode(faction, WMODE_SUPPLY)) {
            if (all_crawlers < min(f->base_count, *MapAreaTiles/20)
            && ((choice = find_proto(base_id, TRIAD_LAND, WMODE_SUPPLY, DEF)) >= 0)) {
                score += max(0, 40 - base->mineral_surplus - base->nutrient_surplus);
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == FerryUnit && gov & GOV_MAY_PROD_TRANSPORT && allow_ships && need_ferry) {
            if ((choice = find_proto(base_id, TRIAD_SEA, WMODE_TRANSPORT, DEF)) >= 0) {
                score += (p->target_land_region > 0 || p->transport_units < 4 ? 40 : 0);
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t == ColonyUnit && allow_pods && pods < 2 && gov & GOV_MAY_PROD_COLONY_POD) {
            Triad type = select_colony(base_id, pods, allow_ships);
            if ((type == TRIAD_LAND || type == TRIAD_SEA)
            && (choice = find_proto(base_id, type, WMODE_COLONY, DEF)) >= 0) {
                score += clamp(*MapAreaSqRoot*2 - f->base_count, 0, 80);
                push_item(base, builds, choice, score, --Wt);
                continue;
            }
        }
        if (t < 0) { // Skip facility evaluation
            continue;
        }
        if (item.energy > 0) {
            if (base->energy_surplus < 4) {
                continue;
            }
            score += Wenergy * item.energy * base->energy_surplus / 4;
            score -= 2*base->energy_inefficiency;
        }
        if (t == FAC_NETWORK_NODE) {
            if (has_project(FAC_VIRTUAL_WORLD, faction) && base_can_riot(base_id, false)) {
                score += 80;
            }
            if (facility_count(FAC_NETWORK_NODE, faction) < f->base_count/8) {
                score += 40;
            }
        }
        if (t == FAC_RECYCLING_TANKS) {
            score += 16*(ResInfo->recycling_tanks_nutrient
                + ResInfo->recycling_tanks_mineral
                + ResInfo->recycling_tanks_energy);
        }
        if (t == FAC_CHILDREN_CRECHE) {
            score += 4*base->energy_inefficiency + 16*min(4, base_unused_space(base_id));
            score += (f->SE_growth < -1 || f->SE_growth == 4 ? 40 : 0);
            score += (f->SE_growth > 5 || base->nutrient_surplus < 2
                || has_project(FAC_CLONING_VATS, faction) ? -40 : 0);
        }
        if (t == FAC_BIOLOGY_LAB) {
            score += 2*Wenergy*conf.biology_lab_bonus;
            score -= 2*base->energy_surplus;
        }
        if (t == FAC_BIOLOGY_LAB || t == FAC_CENTAURI_PRESERVE) {
            score += 8*min(4, p->psi_score);
            score -= 40*(!base->eco_damage + (Wgov.AI_fight > 0) + (Wgov.AI_power > 1));
        }
        if (t == FAC_GENEJACK_FACTORY || t == FAC_ROBOTIC_ASSEMBLY_PLANT
        || t == FAC_NANOREPLICATOR || t == FAC_QUANTUM_CONVERTER) {
            if (3*base->mineral_intake_2 > 2*(conf.clean_minerals + f->clean_minerals_modifier)) {
                score -= 100;
            }
            score += 2*max(-80, (core_base ? 80 : 60) - base->mineral_intake_2);
            score += 8*min(0, base->mineral_intake_2 - 16);
            score -= (Wgov.AI_fight > 0 || Wgov.AI_power > 1 ? 4 : 8)*base->eco_damage;
        }
        if (t == FAC_TREE_FARM || t == FAC_HYBRID_FOREST) {
            if (!base->eco_damage) {
                score -= (sea_base || Wgov.AI_fight > 0 ? 8 : 4)*Facility[t].cost;
            }
            score += (Wgov.AI_fight > 0 || Wgov.AI_power > 1 ? 2 : 4)*min(40, base->eco_damage);
            score += 8*(nearby_items(base->x, base->y, 1, BIT_FOREST));
            score -= 2*base->nutrient_surplus;
        }
        if (t == FAC_RECREATION_COMMONS || t == FAC_HOLOGRAM_THEATRE
        || t == FAC_RESEARCH_HOSPITAL || t == FAC_PARADISE_GARDEN) {
            if ((base->pop_size <= content_pop_value && !base->drone_total && base->specialist_total < 2)
            || (base->talent_total > 0 && !base->drone_total && !base->specialist_total)) {
                continue;
            }
            score += 8*base->drone_total;
            score += 16*min(5, base->drone_total + base->specialist_total/2 - base->talent_total);
        }
        if ((t == FAC_COMMAND_CENTER && sea_base) || (t == FAC_NAVAL_YARD && !allow_ships)) {
            continue;
        }
        if (t == FAC_COMMAND_CENTER || t == FAC_NAVAL_YARD || t == FAC_BIOENHANCEMENT_CENTER) {
            if (minerals < reserve || base->defend_goal < 3) {
                continue;
            }
            score -= 4*(Facility[t].cost + Facility[t].maint);
            score -= defend_range;
        }
        if (t == FAC_PERIMETER_DEFENSE || t == FAC_TACHYON_FIELD
        || t == FAC_GEOSYNC_SURVEY_POD || t == FAC_FLECHETTE_DEFENSE_SYS) {
            score += 4*(MaxEnemyRange - 4*Facility[t].maint - defend_range);
        }
        if (t == FAC_TACHYON_FIELD || t == FAC_GEOSYNC_SURVEY_POD || t == FAC_FLECHETTE_DEFENSE_SYS) {
            if (allow_units && base->defend_goal < 3 && defend_range > MaxEnemyRange/2) {
                continue;
            }
            score += 16*clamp(base->defend_goal-3, -2, 2);
        }
        assert(t > 0 && t <= SP_ID_Last);
        push_item(base, builds, -t, score, --Wt);
    }
    if (builds.size()) {
        return builds.top().item_id;
    }
    if (!allow_units || !(gov & GOV_ALLOW_COMBAT)) {
        return -FAC_STOCKPILE_ENERGY;
    }
    debug("BUILD OFFENSE\n");
    return select_combat(base_id, landprobes+seaprobes, sea_base, allow_ships);
}






