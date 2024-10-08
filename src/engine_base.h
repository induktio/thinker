#pragma once
#pragma pack(push, 1)


enum BaseState {
    BSTATE_UNK_1 = 0x1,
    BSTATE_DRONE_RIOTS_ACTIVE = 0x2,
    BSTATE_GOLDEN_AGE_ACTIVE = 0x4,
    BSTATE_COMBAT_LOSS_LAST_TURN = 0x8, // Reset on upkeep
    BSTATE_UNK_10 = 0x10,
    BSTATE_UNK_20 = 0x20, // enemy_strategy, former units
    BSTATE_RESEARCH_DATA_STOLEN = 0x40,
    BSTATE_SKIP_RENAME = 0x80, // prevents capture_base from adding flag BSTATE_RENAME_BASE
    BSTATE_UNK_100 = 0x100,
    BSTATE_FACILITY_SCRAPPED = 0x200, // Only one facility can be scrapped/recycled per turn
    BSTATE_ARTIFACT_LINKED = 0x400, // Alien Artifact linked to Network Node
    BSTATE_ARTIFACT_ALREADY_LINKED = 0x800, // Only show already linked alert once
    BSTATE_UNK_1000 = 0x1000,
    BSTATE_UNK_2000 = 0x2000,
    BSTATE_UNK_4000 = 0x4000,
    BSTATE_UNK_8000 = 0x8000,
    BSTATE_RENAME_BASE = 0x10000, // set in capture_base, this base can be renamed in base_upkeep
    BSTATE_GENETIC_PLAGUE_INTRO = 0x20000,
    BSTATE_ASSISTANT_KILLER_HOME = 0x40000, // Veh home base (or closest) of Assistant worm killer
    BSTATE_UNK_80000 = 0x80000, // enemy_strategy, former units
    BSTATE_UNK_100000 = 0x100000,
    BSTATE_UNK_200000 = 0x200000,
    BSTATE_ENERGY_RESERVES_DRAINED = 0x400000,
    BSTATE_PRODUCTION_DONE = 0x800000, // cleared in base_upkeep > base_production
    BSTATE_UNK_1000000 = 0x1000000,
    BSTATE_UNK_2000000 = 0x2000000,
    BSTATE_UNK_4000000 = 0x4000000, // enemy_strategy
    BSTATE_UNK_8000000 = 0x8000000, // enemy_strategy
    BSTATE_NET_LOCKED = 0x10000000,
    BSTATE_PSI_GATE_USED = 0x20000000,
    BSTATE_HURRY_PRODUCTION = 0x40000000,
    BSTATE_UNK_80000000 = 0x80000000,
};

enum BaseEvent {
    BEVENT_UNK_100 = 0x100,
    BEVENT_BUMPER = 0x200,
    BEVENT_FAMINE = 0x400,
    BEVENT_INDUSTRY = 0x800,
    BEVENT_BUST = 0x1000,
    BEVENT_HEAT_WAVE = 0x2000,
    BEVENT_CLOUD_COVER = 0x4000,
    BEVENT_OBJECTIVE = 0x8000,
};

enum BaseGovernor {
    GOV_MANAGE_PRODUCTION = 0x1,
    GOV_MAY_FORCE_PSYCH = 0x2, // Thinker variable
    GOV_UNK_4 = 0x4, // maybe unused
    GOV_UNK_8 = 0x8, // maybe unused
    GOV_UNK_10 = 0x10, // maybe unused
    GOV_MAY_HURRY_PRODUCTION = 0x20,
    GOV_MANAGE_CITIZENS = 0x40,
    GOV_NEW_VEH_FULLY_AUTO = 0x80,
    GOV_MAY_PROD_NATIVE = 0x100, // Thinker variable
    GOV_MAY_PROD_LAND_COMBAT = 0x200,
    GOV_MAY_PROD_NAVAL_COMBAT = 0x400,
    GOV_MAY_PROD_AIR_COMBAT = 0x800,
    GOV_MAY_PROD_LAND_DEFENSE = 0x1000,
    GOV_MAY_PROD_AIR_DEFENSE = 0x2000,
    GOV_UNK_4000 = 0x4000, // maybe unused
    GOV_MAY_PROD_TERRAFORMERS = 0x8000,
    GOV_MAY_PROD_FACILITIES = 0x10000,
    GOV_MAY_PROD_COLONY_POD = 0x20000,
    GOV_MAY_PROD_SP = 0x40000,
    GOV_MAY_PROD_PROTOTYPE = 0x80000,
    GOV_MAY_PROD_PROBES = 0x100000,
    GOV_MULTI_PRIORITIES = 0x200000,
    GOV_MAY_PROD_EXPLORE_VEH = 0x400000,
    GOV_MAY_PROD_TRANSPORT = 0x800000,
    GOV_PRIORITY_EXPLORE = 0x1000000,
    GOV_PRIORITY_DISCOVER = 0x2000000,
    GOV_PRIORITY_BUILD = 0x4000000,
    GOV_PRIORITY_CONQUER = 0x8000000,
    GOV_UNK_10000000 = 0x10000000, // maybe unused
    GOV_UNK_20000000 = 0x20000000, // maybe unused
    GOV_UNK_40000000 = 0x40000000, // used on lowest difficulty
    GOV_ACTIVE = 0x80000000,
};

const uint32_t BaseGovOptions[][2] = {
    {0x1, GOV_ACTIVE},
    {0x2, GOV_MULTI_PRIORITIES},
    {0x4, GOV_NEW_VEH_FULLY_AUTO},
    {0x8, GOV_MANAGE_CITIZENS},
    {0x10, GOV_MANAGE_PRODUCTION},
    {0x20, GOV_MAY_PROD_EXPLORE_VEH},
    {0x40, GOV_MAY_PROD_LAND_COMBAT},
    {0x80, GOV_MAY_PROD_NAVAL_COMBAT},
    {0x100, GOV_MAY_PROD_AIR_COMBAT},
    {0x200, GOV_MAY_PROD_NATIVE},
    {0x400, GOV_MAY_PROD_LAND_DEFENSE},
    {0x800, GOV_MAY_PROD_AIR_DEFENSE},
    {0x1000, GOV_MAY_PROD_PROTOTYPE},
    {0x2000, GOV_MAY_PROD_TRANSPORT},
    {0x4000, GOV_MAY_PROD_PROBES},
    {0x8000, GOV_MAY_PROD_TERRAFORMERS},
    {0x10000, GOV_MAY_PROD_COLONY_POD},
    {0x20000, GOV_MAY_PROD_FACILITIES},
    {0x40000, GOV_MAY_FORCE_PSYCH},
    {0x80000, GOV_MAY_PROD_SP},
    {0x100000, GOV_MAY_HURRY_PRODUCTION},
};

enum BaseRadius {
    BR_NOT_AVAILABLE = 1,
    BR_NOT_VISIBLE = 2,
    BR_BASE_IN_TILE = 4,
    BR_VEH_IN_TILE = 8, // convoy active or non-pact/non-treaty vehicle
    BR_WORKER_ACTIVE = 16,
    BR_FOREIGN_TILE = 32,
};

enum SEType { // used only as boolean value
    SE_Current = 0,
    SE_Pending = 1,
};

struct BASE {
    int16_t x;
    int16_t y;
    int8_t faction_id;
    int8_t faction_id_former;
    int8_t pop_size;
    int8_t assimilation_turns_left;
    int8_t nerve_staple_turns_left;
    int8_t ai_plan_status;
    uint8_t visibility;
    int8_t factions_pop_size_intel[8];
    char name[25];
    int16_t unk_x; // base_terraform
    int16_t unk_y; // base_terraform
    int32_t state_flags;
    int32_t event_flags;
    int32_t governor_flags;
    int32_t nutrients_accumulated;
    int32_t minerals_accumulated;
    int32_t production_id_last;
    int32_t eco_damage;
    int32_t queue_size;
    int32_t queue_items[10];
    int32_t worked_tiles;
    int32_t specialist_total;
    int32_t specialist_adjust;
    int32_t specialist_types[2];
    uint8_t facilities_built[12];
    int32_t mineral_surplus_final;
    int32_t minerals_accumulated_2;
    int32_t pad_1;
    int32_t pad_2;
    int32_t pad_3;
    int32_t pad_4;
    int32_t nutrient_intake;
    int32_t mineral_intake;
    int32_t energy_intake;
    int32_t unused_intake;
    int32_t nutrient_intake_2;
    int32_t mineral_intake_2;
    int32_t energy_intake_2;
    int32_t unused_intake_2;
    int32_t nutrient_surplus;
    int32_t mineral_surplus;
    int32_t energy_surplus;
    int32_t unused_surplus;
    int32_t nutrient_inefficiency;
    int32_t mineral_inefficiency;
    int32_t energy_inefficiency;
    int32_t unused_inefficiency;
    int32_t nutrient_consumption;
    int32_t mineral_consumption;
    int32_t energy_consumption;
    int32_t unused_consumption;
    int32_t economy_total;
    int32_t psych_total;
    int32_t labs_total;
    int32_t unk_total;
    int16_t autoforward_land_base_id;
    int16_t autoforward_sea_base_id;
    int16_t autoforward_air_base_id;
    int8_t defend_goal; // Thinker variable
    int8_t defend_range; // Thinker variable
    int32_t talent_total;
    int32_t drone_total;
    int32_t superdrone_total;
    int32_t random_event_turns;
    int32_t nerve_staple_count;
    int32_t pad_7;
    int32_t pad_8;

    int item() {
        return queue_items[0];
    }
    bool item_is_project() {
        return queue_items[0] <= -SP_ID_First;
    }
    bool item_is_unit() {
        return queue_items[0] >= 0;
    }
    bool drone_riots_active() {
        return state_flags & BSTATE_DRONE_RIOTS_ACTIVE;
    }
    bool drone_riots() {
        return drone_total > talent_total;
    }
    bool golden_age_active() {
        return state_flags & BSTATE_GOLDEN_AGE_ACTIVE;
    }
    bool golden_age() {
        return !drone_total && pop_size > 2 && talent_total >= (pop_size + 1) / 2;
    }
    bool plr_owner() {
        return is_human(faction_id);
    }
    /*
    AI bases are not limited by any governor settings.
    */
    uint32_t gov_config() {
        return is_human(faction_id) ? governor_flags : ~0u;
    }
    /*
    This implementation is simplified to skip additional checks for these rules:
    RULES_SCN_VICT_SP_COUNT_OBJ, STATE_SCN_VICT_BASE_FACIL_COUNT_OBJ.
    */
    bool is_objective() {
        return (*GameRules & RULES_SCN_VICT_ALL_BASE_COUNT_OBJ ) || (event_flags & BEVENT_OBJECTIVE);
    }
    /*
    Specialist types (CCitizen, 4 bits per id) for the first 16 specialists in the base.
    These are assigned in base_yield and base_energy and chosen by best_specialist.
    */
    int specialist_type(int index) {
        if (index < 0 || index >= MaxBaseSpecNum) {
            return 0;
        }
        return (specialist_types[index/8] >> 4 * (index & 7)) & 0xF;
    }
    void specialist_modify(int index, int citizen_id) {
        if (index < 0 || index >= MaxBaseSpecNum) {
            return;
        }
        specialist_types[index/8] &= ~(0xF << (4 * (index & 7)));
        specialist_types[index/8] |= ((citizen_id & 0xF) << (4 * (index & 7)));
    }
    bool has_fac_built(FacilityId item_id) {
        return item_id >= 0 && item_id <= Fac_ID_Last
            && facilities_built[item_id/8] & (1 << (item_id % 8));
    }
    int SE_economy(bool pending) {
        return (pending ? Factions[faction_id].SE_economy_pending : Factions[faction_id].SE_economy)
            + golden_age_active(); // +1 on economy scale when flag set
    }
    int SE_effic(bool pending) {
        return (pending ? Factions[faction_id].SE_effic_pending : Factions[faction_id].SE_effic)
            + 2*has_fac_built(FAC_CHILDREN_CRECHE); // +2 on efficiency (actual value used in the game)
    }
    int SE_growth(bool pending) {
        return (pending ? Factions[faction_id].SE_growth_pending : Factions[faction_id].SE_growth)
            + 2*has_fac_built(FAC_CHILDREN_CRECHE) // +2 on growth scale
            + 2*golden_age_active(); // +2 on growth scale when flag set
    }
    int SE_police(bool pending) {
        return (pending ? Factions[faction_id].SE_police_pending : Factions[faction_id].SE_police)
            + 2*has_fac_built(FAC_BROOD_PIT);
    }
    int SE_probe(bool pending) {
        return (pending ? Factions[faction_id].SE_probe_pending : Factions[faction_id].SE_probe)
            + 2*has_fac_built(FAC_COVERT_OPS_CENTER);
    }
};


#pragma pack(pop)
