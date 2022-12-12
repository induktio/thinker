#pragma once
#pragma pack(push, 1)


enum Triad {
    TRIAD_NONE = -1, // Thinker variable
    TRIAD_LAND = 0,
    TRIAD_SEA = 1,
    TRIAD_AIR = 2,
};

enum VehMorale {
    MORALE_VERY_GREEN = 0,
    MORALE_GREEN = 1,
    MORALE_DISCIPLINED = 2,
    MORALE_HARDENED = 3,
    MORALE_VETERAN = 4,
    MORALE_COMMANDO = 5,
    MORALE_ELITE = 6,
};

enum VehBasicUnit {
    BSC_COLONY_POD = 0,
    BSC_FORMERS = 1,
    BSC_SCOUT_PATROL = 2,
    BSC_TRANSPORT_FOIL = 3,
    BSC_SEA_FORMERS = 4,
    BSC_SUPPLY_CRAWLER = 5,
    BSC_PROBE_TEAM = 6,
    BSC_ALIEN_ARTIFACT = 7,
    BSC_MIND_WORMS = 8,
    BSC_ISLE_OF_THE_DEEP = 9,
    BSC_LOCUSTS_OF_CHIRON = 10,
    BSC_UNITY_ROVER = 11,
    BSC_UNITY_SCOUT_CHOPPER = 12,
    BSC_UNITY_FOIL = 13,
    BSC_SEALURK = 14,
    BSC_SPORE_LAUNCHER = 15,
    BSC_BATTLE_OGRE_MK1 = 16,
    BSC_BATTLE_OGRE_MK2 = 17,
    BSC_BATTLE_OGRE_MK3 = 18,
    BSC_FUNGAL_TOWER = 19,
    BSC_UNITY_MINING_LASER = 20,
    BSC_SEA_ESCAPE_POD = 21,
    BSC_UNITY_GUNSHIP = 22,
};

enum VehChassis {
    CHS_INFANTRY = 0,
    CHS_SPEEDER = 1,
    CHS_HOVERTANK = 2,
    CHS_FOIL = 3,
    CHS_CRUISER = 4,
    CHS_NEEDLEJET = 5,
    CHS_COPTER = 6,
    CHS_GRAVSHIP = 7,
    CHS_MISSILE = 8,
};

enum VehReactor {
    REC_FISSION = 1,
    REC_FUSION = 2,
    REC_QUANTUM = 3,
    REC_SINGULARITY = 4,
};

enum VehArmor {
    ARM_NO_ARMOR = 0,
    ARM_SYNTHMETAL_ARMOR = 1,
    ARM_PLASMA_STEEL_ARMOR = 2,
    ARM_SILKSTEEL_ARMOR = 3,
    ARM_PHOTON_WALL = 4,
    ARM_PROBABILITY_SHEATH = 5,
    ARM_NEUTRONIUM_ARMOR = 6,
    ARM_ANTIMATTER_PLATE = 7,
    ARM_STASIS_GENERATOR = 8,
    ARM_PSI_DEFENSE = 9,
    ARM_PULSE_3_ARMOR = 10,
    ARM_RESONANCE_3_ARMOR = 11,
    ARM_PULSE_8_ARMOR = 12,
    ARM_RESONANCE_8_ARMOR = 13,
};

enum VehWeapon {
    WPN_HAND_WEAPONS = 0,
    WPN_LASER = 1,
    WPN_PARTICLE_IMPACTOR = 2,
    WPN_GATLING_LASER = 3,
    WPN_MISSILE_LAUNCHER = 4,
    WPN_CHAOS_GUN = 5,
    WPN_FUSION_LASER = 6,
    WPN_TACHYON_BOLT = 7,
    WPN_PLASMA_SHARD = 8,
    WPN_QUANTUM_LASER = 9,
    WPN_GRAVITON_GUN = 10,
    WPN_SINGULARITY_LASER = 11,
    WPN_RESONANCE_LASER = 12,
    WPN_RESONANCE_BOLT = 13,
    WPN_STRING_DISRUPTOR = 14,
    WPN_PSI_ATTACK = 15,
    WPN_PLANET_BUSTER = 16,
    WPN_COLONY_MODULE = 17,
    WPN_TERRAFORMING_UNIT = 18,
    WPN_TROOP_TRANSPORT = 19,
    WPN_SUPPLY_TRANSPORT = 20,
    WPN_PROBE_TEAM = 21,
    WPN_ALIEN_ARTIFACT = 22,
    WPN_CONVENTIONAL_PAYLOAD = 23,
    WPN_TECTONIC_PAYLOAD = 24,
    WPN_FUNGAL_PAYLOAD = 25,
};

enum VehWeaponMode {
    WMODE_COMBAT = 0, // Thinker variable
    WMODE_PROJECTILE = 0,
    WMODE_ENERGY = 1,
    WMODE_MISSILE = 2,
    WMODE_TRANSPORT = 7,
    WMODE_COLONIST = 8,
    WMODE_TERRAFORMER = 9,
    WMODE_CONVOY = 10,
    WMODE_INFOWAR = 11,
    WMODE_ARTIFACT = 12,
};

enum VehPlan {
    PLAN_OFFENSIVE = 0,
    PLAN_COMBAT = 1,
    PLAN_DEFENSIVE = 2,
    PLAN_RECONNAISANCE = 3,
    PLAN_AIR_SUPERIORITY = 4,
    PLAN_PLANET_BUSTER = 5,
    PLAN_NAVAL_SUPERIORITY = 6,
    PLAN_NAVAL_TRANSPORT = 7,
    PLAN_COLONIZATION = 8,
    PLAN_TERRAFORMING = 9,
    PLAN_SUPPLY_CONVOY = 10,
    PLAN_INFO_WARFARE = 11,
    PLAN_ALIEN_ARTIFACT = 12,
    PLAN_TECTONIC_MISSILE = 13,
    PLAN_FUNGAL_MISSILE = 14,
    PLAN_AUTO_CALCULATE = -1,
};

enum VehAbility {
    ABL_ID_SUPER_TERRAFORMER = 0,
    ABL_ID_DEEP_RADAR = 1,
    ABL_ID_CLOAKED = 2,
    ABL_ID_AMPHIBIOUS = 3,
    ABL_ID_DROP_POD = 4,
    ABL_ID_AIR_SUPERIORITY = 5,
    ABL_ID_DEEP_PRESSURE_HULL = 6,
    ABL_ID_CARRIER = 7,
    ABL_ID_AAA = 8,
    ABL_ID_COMM_JAMMER = 9,
    ABL_ID_ANTIGRAV_STRUTS = 10,
    ABL_ID_EMPATH = 11,
    ABL_ID_POLY_ENCRYPTION = 12,
    ABL_ID_FUNGICIDAL = 13,
    ABL_ID_TRAINED = 14,
    ABL_ID_ARTILLERY = 15,
    ABL_ID_CLEAN_REACTOR = 16,
    ABL_ID_BLINK_DISPLACER = 17,
    ABL_ID_TRANCE = 18,
    ABL_ID_HEAVY_TRANSPORT = 19,
    ABL_ID_NERVE_GAS = 20,
    ABL_ID_REPAIR = 21,
    ABL_ID_POLICE_2X = 22,
    ABL_ID_SLOW = 23,
    ABL_ID_SOPORIFIC_GAS = 24,
    ABL_ID_DISSOCIATIVE_WAVE = 25,
    ABL_ID_MARINE_DETACHMENT = 26,
    ABL_ID_FUEL_NANOCELLS = 27,
    ABL_ID_ALGO_ENHANCEMENT = 28,
};

enum VehAbilityFlag {
    ABL_NONE = 0,
    ABL_SUPER_TERRAFORMER = 0x1,
    ABL_DEEP_RADAR = 0x2,
    ABL_CLOAKED = 0x4,
    ABL_AMPHIBIOUS = 0x8,
    ABL_DROP_POD = 0x10,
    ABL_AIR_SUPERIORITY = 0x20,
    ABL_DEEP_PRESSURE_HULL = 0x40,
    ABL_CARRIER = 0x80,
    ABL_AAA = 0x100,
    ABL_COMM_JAMMER = 0x200,
    ABL_ANTIGRAV_STRUTS = 0x400,
    ABL_EMPATH = 0x800,
    ABL_POLY_ENCRYPTION = 0x1000,
    ABL_FUNGICIDAL = 0x2000,
    ABL_TRAINED = 0x4000,
    ABL_ARTILLERY = 0x8000,
    ABL_CLEAN_REACTOR = 0x10000,
    ABL_BLINK_DISPLACER = 0x20000,
    ABL_TRANCE = 0x40000,
    ABL_HEAVY_TRANSPORT = 0x80000,
    ABL_NERVE_GAS = 0x100000,
    ABL_REPAIR = 0x200000,
    ABL_POLICE_2X = 0x400000,
    ABL_SLOW = 0x800000,
    ABL_SOPORIFIC_GAS = 0x1000000,
    ABL_DISSOCIATIVE_WAVE = 0x2000000,
    ABL_MARINE_DETACHMENT = 0x4000000,
    ABL_FUEL_NANOCELLS = 0x8000000,
    ABL_ALGO_ENHANCEMENT = 0x10000000,
};

enum VehAbilityRules {
    AFLAG_ALLOWED_LAND_UNIT = 0x1,
    AFLAG_ALLOWED_SEA_UNIT = 0x2,
    AFLAG_ALLOWED_AIR_UNIT = 0x4,
    AFLAG_ALLOWED_COMBAT_UNIT = 0x8,
    AFLAG_ALLOWED_TERRAFORM_UNIT = 0x10,
    AFLAG_ALLOWED_NONCOMBAT_UNIT = 0x20,
    AFLAG_NOT_ALLOWED_PROBE_TEAM = 0x40,
    AFLAG_NOT_ALLOWED_PSI_UNIT = 0x80,
    AFLAG_TRANSPORT_ONLY_UNIT = 0x100,
    AFLAG_NOT_ALLOWED_FAST_UNIT = 0x200,
    AFLAG_COST_INC_LAND_UNIT = 0x400,
    AFLAG_ONLY_PROBE_TEAM = 0x800,
};

enum UnitPrototypeFlags {
    UNIT_ACTIVE = 0x1, // if this bit is zero, prototype has been retired
    UNIT_CUSTOM_NAME_SET = 0x2,
    UNIT_PROTOTYPED = 0x4,
};

enum VehFlags {
    VFLAG_PROBE_PACT_OPERATIONS = 0x4,
    VFLAG_IS_OBJECTIVE = 0x20,
    VFLAG_LURKER = 0x40,
    VFLAG_START_RAND_LOCATION = 0x80,
    VFLAG_START_RAND_MONOLITH = 0x100,
    VFLAG_START_RAND_FUNGUS = 0x200,
    VFLAG_INVISIBLE = 0x400,
};

enum VehState {
    VSTATE_IN_TRANSPORT = 0x1,
    VSTATE_UNK_2 = 0x2,
    VSTATE_HAS_MOVED = 0x4, // set after first movement attempt (even if failed) on each turn
    VSTATE_UNK_8 = 0x8,
    VSTATE_REQUIRES_SUPPORT = 0x10,
    VSTATE_MADE_AIRDROP = 0x20,
    VSTATE_UNK_40 = 0x40,
    VSTATE_DESIGNATE_DEFENDER = 0x80,
    VSTATE_MONOLITH_UPGRADED = 0x100,
    VSTATE_UNK_200 = 0x200,
    VSTATE_UNK_400 = 0x400,
    VSTATE_UNK_800 = 0x800,
    VSTATE_UNK_1000 = 0x1000,
    VSTATE_UNK_2000 = 0x2000,
    VSTATE_EXPLORE = 0x4000,
    VSTATE_UNK_8000 = 0x8000,
    VSTATE_UNK_10000 = 0x10000,
    VSTATE_UNK_20000 = 0x20000,
    VSTATE_UNK_40000 = 0x40000,
    VSTATE_USED_NERVE_GAS = 0x80000, // set/reset on attacking Veh after each attack
    VSTATE_UNK_100000 = 0x100000,
    VSTATE_PACIFISM_DRONE = 0x200000,
    VSTATE_PACIFISM_FREE_SKIP = 0x400000,
    VSTATE_ASSISTANT_WORM = 0x800000, // Int: Brood Trainer; Human player's 1st spawned Mind Worm
    VSTATE_UNK_1000000 = 0x1000000,
    VSTATE_UNK_2000000 = 0x2000000,
    VSTATE_CRAWLING = 0x4000000, // more than just crawling, terraforming also?
    VSTATE_UNK_8000000 = 0x8000000,
    VSTATE_UNK_10000000 = 0x10000000,
    VSTATE_UNK_20000000 = 0x20000000,
    VSTATE_UNK_40000000 = 0x40000000,
    VSTATE_UNK_80000000 = 0x80000000,
};

enum FormerItem {
    FORMER_NONE = -1, // Thinker variable
    FORMER_FARM = 0,
    FORMER_SOIL_ENR = 1,
    FORMER_MINE = 2,
    FORMER_SOLAR = 3,
    FORMER_FOREST = 4,
    FORMER_ROAD = 5,
    FORMER_MAGTUBE = 6,
    FORMER_BUNKER = 7,
    FORMER_AIRBASE = 8,
    FORMER_SENSOR = 9,
    FORMER_REMOVE_FUNGUS = 10,
    FORMER_PLANT_FUNGUS = 11,
    FORMER_CONDENSER = 12,
    FORMER_ECH_MIRROR = 13,
    FORMER_THERMAL_BORE = 14,
    FORMER_AQUIFER = 15,
    FORMER_RAISE_LAND = 16,
    FORMER_LOWER_LAND = 17,
    FORMER_LEVEL_TERRAIN = 18,
    FORMER_MONOLITH = 19,
};

enum VehOrder {
    ORDER_NONE = 0,              //  -
    ORDER_SENTRY_BOARD = 1,      // (L)
    ORDER_HOLD = 2,              // (H); Hold (set 1st waypoint (-1, 0)), Hold 10 (-1, 10), On Alert
    ORDER_CONVOY = 3,            // (O)
    ORDER_FARM = 4,              // (f)
    ORDER_SOIL_ENRICHER = 5,     // (f)
    ORDER_MINE = 6,              // (M)
    ORDER_SOLAR_COLLECTOR = 7,   // (S)
    ORDER_PLANT_FOREST = 8,      // (F)
    ORDER_ROAD = 9,              // (R)
    ORDER_MAGTUBE = 10,          // (R)
    ORDER_BUNKER = 11,           // (K)
    ORDER_AIRBASE = 12,          // (.)
    ORDER_SENSOR_ARRAY = 13,     // (O)
    ORDER_REMOVE_FUNGUS = 14,    // (F)
    ORDER_PLANT_FUNGUS = 15,     // (F)
    ORDER_CONDENSER = 16,        // (N)
    ORDER_ECHELON_MIRROR = 17,   // (E)
    ORDER_THERMAL_BOREHOLE = 18, // (B)
    ORDER_DRILL_AQUIFER = 19,    // (Q)
    ORDER_TERRAFORM_UP = 20,     // (])
    ORDER_TERRAFORM_DOWN = 21,   // ([)
    ORDER_TERRAFORM_LEVEL = 22,  // (_)
    ORDER_PLACE_MONOLITH = 23,   // (?)
    ORDER_MOVE_TO = 24,          // (G); Move unit to here, Go to Base, Group go to, Patrol
    ORDER_MOVE = 25,             // (>); Only used in a few places, seems to be buggy mechanic
    ORDER_EXPLORE = 26,          // (/); not set via shortcut, AI related?
    ORDER_ROAD_TO = 27,          // (r)
    ORDER_MAGTUBE_TO = 28,       // (t)
    ORDER_AI_GO_TO = 88,         //  - ; ORDER_GO_TO (0x18) | 0x40 > 0x58 ? only used by AI funcs
};

enum VehOrderAutoType {
    ORDERA_TERRA_AUTO_FULL = 0,
    ORDERA_TERRA_AUTO_ROAD = 1,
    ORDERA_TERRA_AUTO_MAGTUBE = 2,
    ORDERA_TERRA_AUTOIMPROVE_BASE = 3,
    ORDERA_TERRA_FARM_SOLAR_ROAD = 4,
    ORDERA_TERRA_FARM_MINE_ROAD = 5, // displayed incorrectly as 'Mine+Solar+Road' (labels.txt:L411)
    ORDERA_TERRA_AUTO_FUNGUS_REM = 6,
    ORDERA_TERRA_AUTOMATIC_SENSOR = 7,
    // 8 unused?
    // 9 unused?
    ORDERA_BOMBING_RUN = 10, // air units only
    ORDERA_ON_ALERT = 11,
    ORDERA_AUTOMATE_AIR_DEFENSE = 12,
};

struct UNIT {
    char name[32];
    uint32_t ability_flags;
    int8_t chassis_type;
    int8_t weapon_type;
    int8_t armor_type;
    int8_t reactor_type;
    int8_t carry_capacity;
    uint8_t cost;
    int8_t plan;
    int8_t unk_1; // some kind of internal prototype category?
    uint8_t obsolete_factions;// faction bitfield of those who marked this prototype obsolete
    uint8_t combat_factions; // faction bitfield for those that have seen this unit in combat (atk/def)
    int8_t icon_offset;
    int8_t pad_1; // unused
    uint16_t unit_flags;
    int16_t preq_tech;

    int triad() {
        return Chassis[chassis_type].triad;
    }
    uint8_t speed() {
        return Chassis[chassis_type].speed;
    }
    uint8_t armor_cost() {
        return Armor[armor_type].cost;
    }
    uint8_t weapon_cost() {
        return Weapon[weapon_type].cost;
    }
    int std_offense_value() {
        return Weapon[weapon_type].offense_value * reactor_type;
    }
    bool is_active() {
        return unit_flags & UNIT_ACTIVE;
    }
    bool is_prototyped() {
        return unit_flags & UNIT_PROTOTYPED;
    }
    bool is_armored() {
        return armor_type != ARM_NO_ARMOR;
    }
    bool is_combat_unit() {
        return weapon_type <= WPN_PSI_ATTACK;
    }
    bool is_defend_unit() {
        return triad() == TRIAD_LAND && (armor_type != ARM_NO_ARMOR || weapon_type <= WPN_PSI_ATTACK);
    }
    bool is_colony() {
        return weapon_type == WPN_COLONY_MODULE;
    }
    bool is_former() {
        return weapon_type == WPN_TERRAFORMING_UNIT;
    }
    bool is_supply() {
        return weapon_type == WPN_SUPPLY_TRANSPORT;
    }
};

struct VEH {
    int16_t x;
    int16_t y;
    uint32_t state;
    uint16_t flags;
    int16_t unit_id;
    int16_t pad_0; // unused
    uint8_t faction_id;
    uint8_t year_end_lurking;
    uint8_t damage_taken;
    int8_t order;
    uint8_t waypoint_count;
    uint8_t patrol_current_point;
    int16_t waypoint_1_x; // doubles as transport veh_id if unit is sentry/board
    int16_t waypoint_2_x;
    int16_t waypoint_3_x;
    int16_t waypoint_4_x;
    int16_t waypoint_1_y;
    int16_t waypoint_2_y;
    int16_t waypoint_3_y;
    int16_t waypoint_4_y;
    uint8_t morale;
    uint8_t terraforming_turns;
    uint8_t order_auto_type;
    uint8_t visibility; // faction bitfield of who can currently see Veh excluding owner
    uint8_t road_moves_spent;
    int8_t rotate_angle;
    uint8_t iter_count;
    uint8_t status_icon;
    // 000 00 000 : framed faction : secondary options : primary action id (0-7)
    // secondary options: THOUGHTMENU, ADVVIRUS, DECIPHER, SUBVERTMENU
    // 000 0 0000 : framed faction : n/a : probe action id (8) ; Freeing Captured Leaders only
    uint8_t probe_action; // see above and ProbePrimaryAction, last action taken by probe team
    uint8_t probe_sabotage_id; // for targeted sabotage: production: 0, abort: 99, or facility id
    int16_t home_base_id;
    int16_t next_unit_id_stack;
    int16_t prev_unit_id_stack;

    const char* name() {
        return Units[unit_id].name;
    }
    int triad() {
        return Chassis[Units[unit_id].chassis_type].triad;
    }
    int speed() {
        return Chassis[Units[unit_id].chassis_type].speed;
    }
    int cost() {
        return Units[unit_id].cost;
    }
    int chassis_type() {
        return Units[unit_id].chassis_type;
    }
    int reactor_type() {
        return Units[unit_id].reactor_type;
    }
    int armor_type() {
        return Units[unit_id].armor_type;
    }
    int weapon_type() {
        return Units[unit_id].weapon_type;
    }
    int weapon_mode() {
        return Weapon[Units[unit_id].weapon_type].mode;
    }
    int offense_value() {
        return Weapon[Units[unit_id].weapon_type].offense_value;
    }
    int defense_value() {
        return Armor[Units[unit_id].armor_type].defense_value;
    }
    bool is_armored() {
        return armor_type() != ARM_NO_ARMOR;
    }
    bool is_combat_unit() {
        return Units[unit_id].weapon_type <= WPN_PSI_ATTACK && unit_id != BSC_FUNGAL_TOWER;
    }
    bool is_native_unit() {
        return unit_id == BSC_MIND_WORMS || unit_id == BSC_ISLE_OF_THE_DEEP
            || unit_id == BSC_LOCUSTS_OF_CHIRON || unit_id == BSC_SEALURK
            || unit_id == BSC_SPORE_LAUNCHER || unit_id == BSC_FUNGAL_TOWER;
    }
    bool is_probe() {
        return Units[unit_id].weapon_type == WPN_PROBE_TEAM;
    }
    bool is_colony() {
        return Units[unit_id].weapon_type == WPN_COLONY_MODULE;
    }
    bool is_supply() {
        return Units[unit_id].weapon_type == WPN_SUPPLY_TRANSPORT;
    }
    bool is_former() {
        return Units[unit_id].weapon_type == WPN_TERRAFORMING_UNIT;
    }
    bool is_transport() {
        return Units[unit_id].weapon_type == WPN_TROOP_TRANSPORT;
    }
    bool is_artifact() {
        return Units[unit_id].weapon_type == WPN_ALIEN_ARTIFACT;
    }
    bool is_visible(int faction) {
        return visibility & (1 << faction);
    }
    bool at_target() {
        return order == ORDER_NONE || (waypoint_1_x < 0 && waypoint_1_y < 0)
            || (x == waypoint_1_x && y == waypoint_1_y);
    }
    bool mid_damage() {
        return damage_taken > 2*Units[unit_id].reactor_type;
    }
    bool high_damage() {
        return damage_taken > 4*Units[unit_id].reactor_type;
    }
    bool need_heals() {
        return mid_damage() && unit_id != BSC_BATTLE_OGRE_MK1
            && unit_id != BSC_BATTLE_OGRE_MK2 && unit_id != BSC_BATTLE_OGRE_MK3;
    }
    bool need_monolith() {
        return need_heals() || (morale < MORALE_ELITE
            && ~state & VSTATE_MONOLITH_UPGRADED && offense_value() != 0);
    }
};

#pragma pack(pop)

