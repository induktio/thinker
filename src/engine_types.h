#pragma once
#pragma pack(push, 1)

struct MAP {
    uint8_t climate; // 000 00 000 | altitude (3 bit) ; rainfall (2 bit) ; temperature (3 bit)
    uint8_t contour; // altitude details
    /*
    val2 & 0xF0:
    AI colonization priority returned by world_site()
    This is written to the map struct by site_set()
    0 = unknown/not available, 1..15 higher numbers indicate a better location.
    When AltGr+4 debug overlay is enabled, MapWin_gen_terrain_poly renders
    this number on land tiles where value > 0.
    val2 & 0xF:
    Faction ID of the unit occupying this tile. 0xF = unoccupied.
    Sometimes faction ID of a deleted unit persists on the tile.
    */
    uint8_t val2;
    /*
    The game keeps track of disjoint land/water areas and assigns each of them an ID number
    which is used to index the [128] planning variable arrays in Faction struct.
    Valid ranges: 1-62 (land), 65-126 (sea).
    */
    uint8_t region;
    uint8_t visibility;
    uint8_t val3; // 00 000 000 | rocky (2 bit); lock faction_id (3 bit); using faction_id (3 bit)
    uint8_t unk_1; // flags? bitfield
    int8_t owner;
    uint32_t items;
    uint32_t landmarks;
    uint32_t visible_items[7];

    int alt_level() {
        return climate >> 5;
    }
    int code_at() {
        return landmarks >> 24;
    }
    int lm_items() {
        return landmarks & 0xFFFF;
    }
    bool is_visible(int faction) {
        return visibility & (1 << faction);
    }
    bool is_owned() {
        return owner >= 0;
    }
    bool is_land_region() {
        return region < 63; // Skip pole tiles
    }
    bool is_pole_tile() {
        return region == 63 || region == 127;
    }
    bool is_base() {
        return items & BIT_BASE_IN_TILE && veh_owner() >= 0;
    }
    bool is_bunker() {
        return items & BIT_BUNKER;
    }
    bool is_airbase() {
        return is_base() || (items & BIT_AIRBASE);
    }
    bool is_base_radius() {
        return items & BIT_BASE_RADIUS;
    }
    bool is_base_or_bunker() {
        return is_base() || (items & BIT_BUNKER);
    }
    bool is_fungus() {
        return items & BIT_FUNGUS && alt_level() >= ALT_OCEAN_SHELF;
    }
    bool is_rocky() {
        return val3 & TILE_ROCKY && alt_level() >= ALT_SHORE_LINE;
    }
    bool is_rolling() {
        return val3 & TILE_ROLLING;
    }
    bool is_arid() {
        return !(climate & (TILE_MOIST | TILE_RAINY));
    }
    bool is_rainy() {
        return climate & TILE_RAINY;
    }
    bool is_moist() {
        return climate & TILE_MOIST;
    }
    bool is_rainy_or_moist() {
        return climate & (TILE_MOIST | TILE_RAINY);
    }
    bool volcano_center() { // Volcano also counts as rocky
        return landmarks & LM_VOLCANO && code_at() == 0;
    }
    bool veh_in_tile() {
        return items & BIT_VEH_IN_TILE;
    }
    bool allow_base() {
        return !(items & (BIT_BASE_IN_TILE|BIT_MONOLITH)) && !is_fungus() && !is_rocky();
    }
    bool allow_spawn() {
        return allow_base() && veh_who() < 0;
    }
    bool allow_supply() {
        return !(items & (BIT_BASE_IN_TILE|BIT_VEH_IN_TILE|BIT_MONOLITH|BIT_SUPPLY_REMOVE));
    }
    int veh_owner() {
        if ((val2 & 0xF) >= MaxPlayerNum) {
            return -1; // No vehicles in this tile
        }
        return val2 & 0xF;
    }
    int veh_who() {
        if (items & BIT_VEH_IN_TILE) {
            return veh_owner();
        }
        return -1;
    }
    int base_who() {
        if (items & BIT_BASE_IN_TILE) {
            return veh_owner();
        }
        return -1;
    }
    int anything_at() {
        if (items & (BIT_VEH_IN_TILE | BIT_BASE_IN_TILE)) {
            return veh_owner();
        }
        return -1;
    }
};

struct FileFindPath {
    char cd_path[256];
    char alt_path[256];
    char last_path[256];
    char exe_dir[256];
    char relative_path[256];
};

struct AlphaIniPref {
    int32_t preferences;
    int32_t more_preferences;
    int32_t announce;
    int32_t rules;
    int32_t semaphore;
    int32_t time_controls;
    int32_t customize;
    int32_t custom_world[7];
};

struct DefaultPref {
    int32_t difficulty;
    int32_t faction_id;
    int32_t pad; // unused
    int32_t map_type;
    int32_t top_menu;
};

struct Label {
    char** labels;
    int32_t label_count;
};

struct Landmark {
    int32_t x;
    int32_t y;
    char name[32];
};

struct Continent {
    int32_t tile_count; // count of tiles in region
    int32_t open_terrain; // count of non-rocky, non-fungus tiles (only 1 movement point to travel)
    int32_t world_site; // highest world_site value (0-15)
    int32_t pods; // current count of supply and unity pods in region
    int32_t unk_5; // padding?
    uint8_t sea_coasts[8]; // sea specific regions, connections to land regions? bitmask
};

struct Path {
    int32_t* mapTable;
    int16_t* xTable;
    int16_t* yTable;
    int32_t index1; // specific territory count
    int32_t index2; // overall territory count
    int32_t faction_id1;
    int32_t xDst;
    int32_t yDst;
    int32_t field_20;
    int32_t faction_id2;
    int32_t unit_id;
};

struct Goal {
    int16_t type;
    int16_t priority;
    int32_t x;
    int32_t y;
    int32_t base_id;
};

struct Monument {
    int32_t data1[205];
    int32_t data2[8][14];
};

struct ReplayEvent {
    int8_t event;
    int8_t faction_id;
    int16_t turn;
    int16_t x;
    int16_t y;
};

struct MFaction {
    int32_t is_leader_female;
    char filename[24];
    char search_key[24];
    char name_leader[24];
    char title_leader[24];
    char adj_leader[128];
    char adj_insult_leader[128];
    char adj_faction[128];
    char adj_insult_faction[128];
    /*
    Thinker-specific save game variables.
    */
    int32_t thinker_unused[3];
    int16_t thinker_probe_lost;
    int16_t thinker_last_mc_turn;
    int16_t thinker_probe_end_turn[8];
    char pad_1[96];
    /*
    End of block
    */
    char noun_faction[24];
    int32_t noun_gender;
    int32_t is_noun_plural;
    char adj_name_faction[128]; // drops 2nd entry on line (abbreviation value?)
    char formal_name_faction[40];
    char insult_leader[24];
    char desc_name_faction[24];
    char assistant_name[24];
    char scientist_name[24];
    char assistant_city[24];
    char pad_2[176];
    int32_t rule_tech_selected;
    int32_t rule_morale;
    int32_t rule_research;
    int32_t rule_drone;
    int32_t rule_talent;
    int32_t rule_energy;
    int32_t rule_interest;
    int32_t rule_population;
    int32_t rule_hurry;
    int32_t rule_techcost;
    int32_t rule_psi;
    int32_t rule_sharetech;
    int32_t rule_commerce;
    int32_t rule_flags;
    int32_t faction_bonus_count;
    int32_t faction_bonus_id[8];
    int32_t faction_bonus_val1[8];
    int32_t faction_bonus_val2[8];
    int32_t AI_fight;
    int32_t AI_growth;
    int32_t AI_tech;
    int32_t AI_wealth;
    int32_t AI_power;
    int32_t soc_priority_category;
    int32_t soc_opposition_category;
    int32_t soc_priority_model;
    int32_t soc_opposition_model;
    int32_t soc_priority_effect;
    int32_t soc_opposition_effect;

    bool is_aquatic() {
        return rule_flags & RFLAG_AQUATIC;
    }
    bool is_alien() {
        return rule_flags & RFLAG_ALIEN;
    }
};

struct Faction {
    int32_t player_flags;
    int32_t ranking; // 0 (lowest) to 7 (highest)
    int32_t diff_level; // 3 for AIs, equal to diff_level setting for humans
    int32_t base_name_offset; // Keep track which base names have been used
    int32_t base_sea_name_offset; // Keep track which sea base names have been used
    int32_t last_base_turn; // Turn for last built, captured or acquired (drone riot) base
    int32_t diplo_status[8]; // Contains all formal treaties
    int32_t diplo_agenda[8];
    int32_t diplo_friction[8];
    int32_t diplo_spoke[8]; // Turn for the last player-to-AI communication; -1 for never
    int32_t diplo_merc[8]; // Possibly higher values indicate willingness for deal-making
    int8_t diplo_patience[8]; // AI-to-player
    int32_t sanction_turns; // Turns left for economic sanctions imposed by other factions for atrocities
    int32_t loan_balance[8]; // Loan balance remaining this faction owes another to be paid over term
    int32_t loan_payment[8]; // The per turn payment amount this faction owes another faction
    int32_t unk_1[8]; // unused
    int32_t integrity_blemishes;
    int32_t diplo_reputation;
    int32_t diplo_gifts[8]; // ? Gifts and bribes we have received
    int32_t diplo_wrongs[8]; // Number of times we double crossed this faction
    int32_t diplo_betrayed[8]; // Number of times double crossed by this faction
    int32_t diplo_unk_3[8]; // ? combat related
    int32_t diplo_unk_4[8]; // ? combat related
    int32_t traded_maps; // bitfield of other factions that have traded maps with faction
    int32_t base_governor_adv; // default advanced Governor settings
    int32_t atrocities; // count committed by faction
    int32_t major_atrocities; // count committed by faction
    int32_t mind_control_total; // ? probe: mind control base (+4) / subvert unit (+1) total
    int32_t diplo_mind_control[8]; // ? probe: mind control base (+4) / subvert unit (+1) per faction
    int32_t diplo_stolen_techs[8]; // probe: successfully procured research data (tech/map) per faction
    int32_t energy_credits;
    int32_t hurry_cost_total; // Net MP: Total paid energy to hurry production (current turn)
    int32_t SE_Politics_pending;
    int32_t SE_Economics_pending;
    int32_t SE_Values_pending;
    int32_t SE_Future_pending;
    int32_t SE_Politics;
    int32_t SE_Economics;
    int32_t SE_Values;
    int32_t SE_Future;
    int32_t SE_upheaval_cost_paid;
    int32_t SE_economy_pending;
    int32_t SE_effic_pending;
    int32_t SE_support_pending;
    int32_t SE_talent_pending;
    int32_t SE_morale_pending;
    int32_t SE_police_pending;
    int32_t SE_growth_pending;
    int32_t SE_planet_pending;
    int32_t SE_probe_pending;
    int32_t SE_industry_pending;
    int32_t SE_research_pending;
    int32_t SE_economy;
    int32_t SE_effic;
    int32_t SE_support;
    int32_t SE_talent;
    int32_t SE_morale;
    int32_t SE_police;
    int32_t SE_growth;
    int32_t SE_planet;
    int32_t SE_probe;
    int32_t SE_industry;
    int32_t SE_research;
    int32_t SE_economy_2;
    int32_t SE_effic_2;
    int32_t SE_support_2;
    int32_t SE_talent_2;
    int32_t SE_morale_2;
    int32_t SE_police_2;
    int32_t SE_growth_2;
    int32_t SE_planet_2;
    int32_t SE_probe_2;
    int32_t SE_industry_2;
    int32_t SE_research_2;
    int32_t SE_economy_base;
    int32_t SE_effic_base;
    int32_t SE_support_base;
    int32_t SE_talent_base;
    int32_t SE_morale_base;
    int32_t SE_police_base;
    int32_t SE_growth_base;
    int32_t SE_planet_base;
    int32_t SE_probe_base;
    int32_t SE_industry_base;
    int32_t SE_research_base;
    int32_t energy_surplus_total;
    int32_t facility_maint_total;
    int32_t tech_commerce_bonus; // Increases commerce income
    int32_t turn_commerce_income;
    int32_t unk_17;
    int32_t unk_18;
    int32_t tech_fungus_nutrient;
    int32_t tech_fungus_mineral;
    int32_t tech_fungus_energy;
    int32_t tech_fungus_unused;
    int32_t SE_alloc_psych;
    int32_t SE_alloc_labs;
    int32_t tech_pact_shared_goals[12]; // Bitfield; Suggested tech_id goals from pacts (TEAMTECH)
    int32_t tech_ranking; // Twice the number of techs discovered
    int32_t unk_27;
    int32_t ODP_deployed;
    int32_t tech_count_transcendent; // Transcendent Thoughts achieved
    int8_t tech_trade_source[88];
    int32_t unk_28;
    int32_t tech_accumulated;
    int32_t tech_research_id;
    int32_t tech_cost;
    int32_t earned_techs_saved;
    int32_t net_random_event;
    int32_t AI_fight;
    int32_t AI_growth;
    int32_t AI_tech;
    int32_t AI_wealth;
    int32_t AI_power;
    int32_t target_x;
    int32_t target_y;
    int32_t best_mineral_output; // For faction bases highest mineral_intake_2 + 2 * mineral_surplus
    int32_t council_call_turn;
    int32_t unk_29[11]; // Council related
    int32_t unk_30[11]; // Council related
    int32_t facility_announced[2]; // bitfield - used to determine one time play of fac audio blurb
    int32_t unk_33;
    int32_t clean_minerals_modifier; // Starts from zero and increases by one after each fungal pop
    int32_t base_id_attack_target; // Battle planning of base to attack, -1 if not set
    int32_t unk_37;
    char saved_queue_name[8][24];
    int32_t saved_queue_size[8];
    int32_t saved_queue_items[8][10];
    int32_t unk_40[8]; // From possible SE support -4 to 3 minerals spent on unit support
    int32_t unk_41[40]; // base_psych
    int32_t unk_42[32]; // base_psych
    int32_t unk_43[9]; // From possible SE effic 4 to -4 all energy lost to inefficieny
    int32_t unk_45;
    int32_t unk_46;
    int32_t unk_47;
    int32_t nutrient_surplus_total;
    int32_t labs_total;
    int32_t satellites_nutrient;
    int32_t satellites_mineral;
    int32_t satellites_energy;
    int32_t satellites_ODP;
    int32_t best_weapon_value;
    int32_t best_psi_land_offense;
    int32_t best_psi_land_defense;
    int32_t best_armor_value;
    int32_t best_land_speed;
    int32_t enemy_best_weapon_value; // Enemy refers here to any non-pact faction
    int32_t enemy_best_armor_value;
    int32_t enemy_best_land_speed;
    int32_t enemy_best_psi_land_offense;
    int32_t enemy_best_psi_land_defense;
    int32_t unk_64;
    int32_t unk_65;
    int32_t unk_66;
    int32_t unk_67;
    int32_t unk_68;
    int32_t unk_69;
    uint8_t units_active[512];
    uint8_t units_queue[512];
    uint16_t units_lost[512];
    int32_t total_combat_units;
    int32_t base_count;
    int32_t mil_strength_1;
    int32_t mil_strength_2;
    int32_t pop_total;
    int32_t unk_70;
    int32_t planet_busters;
    int32_t unk_71;
    int32_t unk_72;
    /*
    AI planning variables that relate to faction units in specific disjoint land/water areas.
    All of these are indexed by the region value in MAP struct.
    These are updated in enemy_strategy and mainly only used by the legacy vanilla AI.
    */
    uint16_t region_total_combat_units[128];
    uint8_t region_total_bases[128];
    uint8_t region_total_offensive_units[128];
    uint16_t region_force_rating[128]; // Combined offensive/morale rating of all units in the area
    uint16_t region_flags[128]; // Movement planning flags
    uint16_t region_territory_tiles[128];
    uint16_t region_visible_tiles[128];
    uint16_t region_good_tiles[128];
    uint16_t region_unk_5[128]; // Unknown reset_territory/enemy_move counter
    uint8_t region_unk_6[128]; // Unknown enemy_strategy state
    uint8_t region_territory_pods[128];
    uint8_t region_base_plan[128]; // visible in map UI with omni view + debug mode under base name
    /* End of block */
    Goal goals[75];
    Goal sites[25];
    int32_t unk_93;
    int32_t goody_opened;
    int32_t goody_artifact;
    int32_t goody_earthquake;
    int32_t goody_tech;
    int32_t tech_achieved; // count of technology faction has discovered/achieved
    int32_t time_bonus_count; // Net MP: Each is worth amount in seconds under Time Controls Extra
    int32_t unk_99; // unused?
    uint32_t secret_project_intel[8]; // Bitfield; News of SPs other factions are working on
    int32_t corner_market_turn;
    int32_t corner_market_active;
    int32_t unk_101;
    int32_t unk_102;
    int32_t unk_103;
    int32_t player_flags_ext;
    int32_t unk_105;
    int32_t unk_106;
    int32_t unk_107;
    int32_t eliminated_count; // may decrease if some faction respawns
    int32_t unk_109;
    int32_t unk_110;
    int32_t unk_111;
    int32_t unk_112;
    int32_t unk_113;
    int32_t unk_114;
    int32_t unk_115;
    int32_t unk_116;
    int32_t unk_117;
    int32_t unk_118;
};

struct CRules {
    int32_t move_rate_roads;
    int32_t nutrient_intake_req_citizen;
    int32_t max_airdrop_rng_wo_orbital_insert;
    int32_t artillery_max_rng;
    int32_t artillery_dmg_numerator;
    int32_t artillery_dmg_denominator;
    int32_t nutrient_cost_multi;
    int32_t mineral_cost_multi;
    int32_t tech_discovery_rate;
    int32_t limit_mineral_inc_for_mine_wo_road;
    int32_t nutrient_effect_mine_sq;
    int32_t min_base_size_specialists;
    int32_t drones_induced_genejack_factory;
    int32_t pop_limit_wo_hab_complex;
    int32_t pop_limit_wo_hab_dome;
    int32_t tech_preq_improv_fungus;
    int32_t tech_preq_ease_fungus_mov;
    int32_t tech_preq_build_road_fungus;
    int32_t tech_preq_allow_2_spec_abil;
    int32_t tech_preq_allow_3_nutrients_sq;
    int32_t tech_preq_allow_3_minerals_sq;
    int32_t tech_preq_allow_3_energy_sq;
    int32_t extra_cost_prototype_land;
    int32_t extra_cost_prototype_sea;
    int32_t extra_cost_prototype_air;
    int32_t psi_combat_ratio_atk[3]; // Numerator: LAND, SEA, AIR
    int32_t psi_combat_ratio_def[3]; // Denominator: LAND, SEA, AIR
    int32_t starting_energy_reserve;
    int32_t combat_bonus_intrinsic_base_def;
    int32_t combat_bonus_atk_road;
    int32_t combat_bonus_atk_higher_elevation;
    int32_t combat_penalty_atk_lower_elevation;
    int32_t tech_preq_orb_insert_wo_space;
    int32_t min_turns_between_councils;
    int32_t minerals_harvesting_forest;
    int32_t territory_max_dist_base;
    int32_t turns_corner_global_energy_market;
    int32_t tech_preq_mining_platform_bonus;
    int32_t tech_preq_economic_victory;
    int32_t combat_penalty_atk_airdrop;
    int32_t combat_bonus_fanatic;
    int32_t combat_land_vs_sea_artillery;
    int32_t combat_artillery_bonus_altitude;
    int32_t combat_mobile_open_ground;
    int32_t combat_mobile_def_in_rough;
    int32_t combat_bonus_trance_vs_psi;
    int32_t combat_bonus_empath_song_vs_psi;
    int32_t combat_infantry_vs_base;
    int32_t combat_penalty_air_supr_vs_ground;
    int32_t combat_bonus_air_supr_vs_air;
    int32_t combat_penalty_non_combat_vs_combat;
    int32_t combat_comm_jammer_vs_mobile;
    int32_t combat_bonus_vs_ship_port;
    int32_t combat_aaa_bonus_vs_air;
    int32_t combat_defend_sensor;
    int32_t combat_psi_bonus_per_planet;
    int32_t retool_strictness;
    int32_t retool_penalty_prod_change;
    int32_t retool_exemption;
    int32_t tgl_probe_steal_tech;
    int32_t tgl_humans_always_contact_net;
    int32_t tgl_humans_always_contact_pbem;
    int32_t max_dmg_percent_arty_base_bunker;
    int32_t max_dmg_percent_arty_open;
    int32_t max_dmg_percent_arty_sea;
    int32_t freq_global_warming_numerator;
    int32_t freq_global_warming_denominator;
    int32_t normal_start_year;
    int32_t normal_end_year_low_three_diff;
    int32_t normal_end_year_high_three_diff;
    int32_t tgl_oblit_base_atrocity;
    int32_t base_size_subspace_gen;
    int32_t subspace_gen_req;
};

struct ResValue {
    int nutrient;
    int mineral;
    int energy;
    int unused;
};

struct CResourceInfo {
    ResValue ocean_sq;
    ResValue base_sq;
    ResValue bonus_sq;
    ResValue forest_sq;
    ResValue recycling_tanks;
    ResValue improved_land;
    ResValue improved_sea;
    ResValue monolith_sq;
    ResValue borehole_sq;
};

struct CResourceName {
    char* name_singular;
    char* name_plural;
};

struct CEnergy {
    char* abbrev;
    char* name;
};

struct CFacility {
    char* name;
    char* effect;
    int32_t pad;
    int32_t cost;
    int32_t maint;
    int32_t preq_tech;
    int32_t free_tech;
    int32_t AI_fight;
    int32_t AI_growth;
    int32_t AI_tech;
    int32_t AI_wealth;
    int32_t AI_power;
};

struct CTech {
    int32_t flags;
    char* name;
    char short_name[8]; // up to 7 characters in length
    int32_t padding; // unused
    int32_t AI_growth;
    int32_t AI_tech;
    int32_t AI_wealth;
    int32_t AI_power;
    int32_t preq_tech1;
    int32_t preq_tech2;
};

struct CCitizen {
    char* singular_name;
    char* plural_name;
    int32_t preq_tech;
    int32_t obsol_tech;
    int32_t econ_bonus;
    int32_t psych_bonus;
    int32_t labs_bonus;
};

struct CAbility {
    char* name;
    char* description;
    char* abbreviation;
    int32_t cost;
    int32_t unk_1; // only referenced in NetDaemon::synch
    int32_t flags;
    int16_t preq_tech;
    int16_t pad;

    bool cost_increase_with_weapon() {
        return cost == -2 || cost == -5 || cost == -6;
    }
    bool cost_increase_with_armor() {
        return cost == -3 || cost == -5 || cost == -7;
    }
    bool cost_increase_with_speed() {
        return cost == -4 || cost == -6 || cost == -7;
    }
};

struct CChassis {
    char* offsv1_name;
    char* offsv2_name;
    char* offsv_name_lrg;
    char* defsv1_name;
    char* defsv2_name;
    char* defsv_name_lrg;
    int32_t offsv1_gender;
    int32_t offsv2_gender;
    int32_t offsv_gender_lrg;
    int32_t defsv1_gender;
    int32_t defsv2_gender;
    int32_t defsv_gender_lrg;
    int32_t offsv1_is_plural;
    int32_t offsv2_is_plural;
    int32_t offsv_is_plural_lrg;
    int32_t defsv1_is_plural;
    int32_t defsv2_is_plural;
    int32_t defsv_is_plural_lrg;
    int8_t speed;
    int8_t triad;
    int8_t range;
    int8_t cargo;
    int8_t cost;
    int8_t missile;
    char sprite_flag_x_coord[8];
    char sprite_flag_y_coord[8];
    char sprite_unk1_x_coord[8];
    char sprite_unk1_y_coord[8];
    char sprite_unk2_x_coord[8];
    char sprite_unk2_y_coord[8];
    char sprite_unk3_x_coord[8];
    char sprite_unk3_y_coord[8];
    int16_t preq_tech;
};

struct CArmor {
    char* name;
    char* name_short;
    int8_t defense_value;
    int8_t mode;
    int8_t cost;
    int8_t padding1;
    int16_t preq_tech;
    int16_t padding2;
};

struct CReactor {
    char* name;
    char* name_short;
    int16_t preq_tech;
    int16_t power; // Fix: this value is not originally set
};

struct CWeapon {
    char* name;
    char* name_short;
    int8_t offense_value;
    int8_t icon;
    int8_t mode;
    int8_t cost;
    int16_t preq_tech;
    int16_t padding;
};

struct CTerraform {
    char* name;
    char* name_sea;
    int32_t preq_tech;
    int32_t preq_tech_sea;
    uint32_t bit;
    uint32_t bit_incompatible;
    int32_t rate;
    char* shortcuts;
};

struct CMorale {
    char* name;
    char* name_lifecycle;
};

struct CCombatMode {
    char* name;
    char* hyphened;
    char* abbrev;
    char* letter;
};

struct COrder {
    char* order;
    char* order_sea;
    char* letter;
};

struct CTimeControl {
    char* name;
    int32_t turn;
    int32_t base;
    int32_t unit;
    int32_t event;
    int32_t extra;
    int32_t refresh;
    int32_t accumulated;
};

struct CSocialCategory {
    int32_t politics;
    int32_t economics;
    int32_t values;
    int32_t future;
};

struct CSocialEffect {
    int32_t economy;
    int32_t efficiency;
    int32_t support;
    int32_t talent;
    int32_t morale;
    int32_t police;
    int32_t growth;
    int32_t planet;
    int32_t probe;
    int32_t industry;
    int32_t research;
};

struct CSocialParam {
    char set1[24];
    char set2[24];
    char padding[56];
};

struct CSocialField {
    char* field_name;
    int32_t soc_preq_tech[4];
    char* soc_name[4];
    CSocialEffect soc_effect[4];
};

struct CMandate {
    char* name;
    char* name_caps;
};

struct CMight {
    char* adj_start;
    char* adj;
};

struct CBonusName {
    char key[24];
};

struct CProposal {
    char* name;
    char* description;
    int32_t preq_tech;
};

struct CNatural {
    char* name;
    char* name_short;
};

struct CWorldbuilder {
    // Seeded land size of a standard world (allowed: 50 to 4000, default: 384)
    int32_t land_base;
    // Additional land from LAND selection: x0, x1, x2 (allowed: 0 to 2000, default: 256)
    int32_t land_mod;
    // Base size of a land mass seed (allowed: 5 to 1000, default: 12)
    int32_t continent_base;
    // Increased size from LAND selection: x0, x1, x2 (allowed: 5 to 1000, default: 24)
    int32_t continent_mod;
    // Base # of extra hills (allowed: 0 to 100, default: 1)
    int32_t hills_base;
    // Additional hills from TIDAL selection: x0, x1, x2 (allowed: 0 to 100, default: 2)
    int32_t hills_mod;
    // Basic plateau size (allowed: 0 to 500, default: 4)
    int32_t plateau_base;
    // Plateau modifier based on LAND selection: x0, x1, x2 (allowed: 0 to 500, default: 8)
    int32_t plateau_mod;
    // Basic # of rivers (allowed: 0 to 100, default: 8)
    int32_t rivers_base;
    // Additional rivers based on RAIN selection (allowed: 0 to 100, default: 12)
    int32_t rivers_rain_mod;
    // Latitude DIVISOR for temperature based on HEAT
    // Smaller # increases effect of HEAT selection (allowed: 1 to 64, default: 14)
    int32_t solar_energy;
    // Latitude DIVISOR for thermal banding; Smaller # widens hot bands (allowed: 1 to 64, default: 14)
    int32_t thermal_band;
    // Latitude DIVISOR for thermal deviance; Smaller # increases randomness (allowed: 1 to 64, default: 8)
    int32_t thermal_deviance;
    // Latitude DIVISOR for global warming; Smaller # increases effect of warming (allowed: 1 to 64, default: 8)
    int32_t global_warming;
    // Magnitude of sea level changes from ice cap melting/freezing (allowed: 1 to 100, default: 5)
    int32_t sea_level_rises;
    // Size of cloud mass trapped by peaks (allowed: 0 to 20, default: 5)
    int32_t cloudmass_peaks;
    // Size of cloud mass trapped by hills (allowed: 0 to 20, default: 3)
    int32_t cloudmass_hills;
    // Multiplier for rainfall belts (allowed: 0 to 8, default: 1)
    int32_t rainfall_coeff;
    // Encourages fractal to grow deep water (allowed: -100 to 100, default: 15)
    int32_t deep_water;
    // Encourages fractal to grow shelf (allowed: -100 to 100, default: 10)
    int32_t shelf;
    // Encourages highland plains (allowed: -100 to 100, default: 15)
    int32_t plains;
    // Encourages wider beaches (allowed: -100 to 100, default: 10)
    int32_t beach;
    // Encourages hills x TIDAL selection (allowed: 0 to 100, default: 10)
    int32_t hills;
    // Encourages peaks (allowed: -100 to 100, default: 25)
    int32_t peaks;
    // Fungus coefficient based on LIFE selection (allowed: 0 to 5, default: 1)
    int32_t fungus;
    // Continent size ratios #1 (allowed: n/a, default: 3)
    int32_t cont_size_ratio1;
    // Continent size ratios #2 (allowed: n/a, default: 6)
    int32_t cont_size_ratio2;
    // Continent size ratios #3 (allowed: n/a, default: 12)
    int32_t cont_size_ratio3;
    // Continent size ratios #4 (allowed: n/a, default: 18)
    int32_t cont_size_ratio4;
    // Continent size ratios #5 (allowed: n/a, default: 24)
    int32_t cont_size_ratio5;
    // Higher # increases island count (allowed: 1 to 500, default: 36)
    int32_t islands;
};


#pragma pack(pop)

