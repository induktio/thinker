
#include "gameturn.h"


static bool territory_avail(int faction_id, int x, int y) {
    int owner = whose_territory(faction_id, x, y, 0, 0);
    return owner < 0 || owner == faction_id;
}

void __cdecl random_events(int flag) {
    const uint32_t BEVENT_VISIBLE =
        (BEVENT_CLOUD_COVER|BEVENT_HEAT_WAVE|BEVENT_BUST|BEVENT_INDUSTRY|BEVENT_FAMINE|BEVENT_BUMPER);
    if (!flag) {
        if (*SolarFlaresEvent & 2) {
            *SolarFlaresEvent = 0;
        } else if (*SolarFlaresEvent & 1) {
            *SolarFlaresEvent = 2;
        }
        if (*DustCloudDuration < 0) {
            *DustCloudDuration = 0;
        }
        *SunspotDuration = max(*SunspotDuration, -1000) - 1;
        if (!*SunspotDuration) {
            POP2("NOMORESPOTS", "space_sm.pcx", -1);
        }
        for (int i = 1; i < MaxPlayerNum; i++) {
            for (int j = 1; j < MaxPlayerNum; j++) {
                if (i != j && is_human(i) && !is_human(j)
                && !has_treaty(j, i, DIPLO_WANT_REVENGE|DIPLO_VENDETTA)) {
                    set_treaty(j, i, DIPLO_UNK_800|DIPLO_SHALL_BETRAY, 0);
                }
            }
        }
        if (*DustCloudDuration > 0 && !--(*DustCloudDuration)) {
            POP2("NOMOREDUST", "stars_sm.pcx", -1);
        }
        for (int i = *BaseCount - 1; i >= 0; --i) {
            BASE* base = &Bases[i];
            if (base->event_flags & BEVENT_VISIBLE) {
                // Skip decrement if turns value is already zero
                if (base->random_event_turns > 0) {
                    base->random_event_turns--;
                }
                if (!base->random_event_turns) {
                    base->event_flags &= ~BEVENT_VISIBLE;
                }
            }
        }
        if (*GameState & STATE_PERIHELION_ACTIVE && !((game_year(*CurrentTurn) - 2180) % 80)) {
            *GameState &= ~STATE_PERIHELION_ACTIVE;
            POP2("PERIHELIONENDS", "stars_sm.pcx", -1);
            return;
        }
        if (!(conf.skip_random_events & 1) && *CurrentTurn > 50) {
            if (!((game_year(*CurrentTurn) - 2160) % 80)) {
                *GameState |= STATE_PERIHELION_ACTIVE;
                POP2("PERIHELION", "heat_sm.pcx", -1);
                return;
            }
        }
    }
    if (*GameRules & RULES_BELL_CURVE || *CurrentTurn < 75 - *DiffLevel * 10) {
        return;
    }
    if (!flag) {
        // This replaces game_reseed / game_random to keep the states separate
        map_rand.reseed(*MapRandomSeed + *CurrentTurn * 13);
    }
    const int base_id = map_rand.get(0, max(100, *BaseCount));
    if (base_id >= *BaseCount) {
        return;
    }
    BASE* base = &Bases[base_id];
    Faction* plr = &Factions[base->faction_id];
    const int faction_id = base->faction_id;
    const bool is_player = faction_id == *CurrentPlayerFaction;
    const bool is_visible = base->visibility & (1 << *CurrentPlayerFaction);
    const int bx = base->x;
    const int by = base->y;
    if (base->pop_size <= 3 || plr->base_count <= 1 || base->event_flags & BEVENT_VISIBLE) {
        return;
    }
    const int event_value = map_rand.get(0, 22);
    if (conf.skip_random_events & (1 << (event_value + 1))) {
        return;
    }
    parse_num(0, conf.base_event_turns);
    parse_says(0, base->name, -1, -1); // Include initial base name by default

    switch (event_value) {
    case 0:
        if (FactionRankings[7] != faction_id
        || (is_human(faction_id) && plr->diff_level <= 1)
        || !map_rand.get(0, 4)) {
            base->event_flags |= BEVENT_BUMPER;
            if (base->event_flags & BEVENT_VISIBLE) {
                base->random_event_turns = conf.base_event_turns;
            }
            if (is_player || *PbemActive) {
                POP2("BUMPER", "bump_sm.pcx", base_id);
            }
        }
        return;
    case 1:
        if (plr->ranking >= (*CurrentTurn >= 150) + 4) {
            base->event_flags |= BEVENT_FAMINE;
            if (base->event_flags & BEVENT_VISIBLE) {
                base->random_event_turns = conf.base_event_turns;
            }
            if (is_player || *PbemActive) {
                if (!is_alien(faction_id)) {
                    POP2("FAMINE", "starv_sm.pcx", base_id);
                } else {
                    POP2("FAMINE", "al_starv_sm.pcx", base_id);
                }
            }
        }
        return;
    case 2:
        if (FactionRankings[7] != faction_id
        || (is_human(faction_id) && plr->diff_level <= 1)
        || !map_rand.get(0, 4)) {
            base->event_flags |= BEVENT_INDUSTRY;
            if (base->event_flags & BEVENT_VISIBLE) {
                base->random_event_turns = conf.base_event_turns;
            }
            if (is_player || *PbemActive) {
                POP2("INDUSTRY", "indbm_sm.pcx", base_id);
            }
        }
        return;
    case 3:
        if (plr->ranking >= (*CurrentTurn >= 150) + 4) {
            base->event_flags |= BEVENT_BUST;
            if (base->event_flags & BEVENT_VISIBLE) {
                base->random_event_turns = conf.base_event_turns;
            }
            if (is_player || *PbemActive) {
                POP2("BUST", "genwarning_sm.pcx", base_id);
            }
        }
        return;
    case 4:
        if (FactionRankings[7] != faction_id
        || (is_human(faction_id) && plr->diff_level <= 1)
        || !map_rand.get(0, 4)) {
            base->event_flags |= BEVENT_HEAT_WAVE;
            if (base->event_flags & BEVENT_VISIBLE) {
                base->random_event_turns = conf.base_event_turns;
            }
            if (is_player || *PbemActive) {
                POP2("HEATWAVE", "heat_sm.pcx", base_id);
            }
        }
        return;
    case 5:
        if (plr->ranking >= (*CurrentTurn >= 150) + 4) {
            base->event_flags |= BEVENT_CLOUD_COVER;
            if (base->event_flags & BEVENT_VISIBLE) {
                base->random_event_turns = conf.base_event_turns;
            }
            if (is_player || *PbemActive) {
                POP2("CLOUDCOVER", "cloud_sm.pcx", base_id);
            }
        }
        return;
    case 6:
        bool has_hosp, has_nano;
        has_hosp = has_fac_built(FAC_RESEARCH_HOSPITAL, base_id);
        has_nano = has_fac_built(FAC_NANOHOSPITAL, base_id);
        if (has_hosp || has_nano) {
            if (has_nano) {
                parse_says(1, Facility[FAC_NANOHOSPITAL].name, -1, -1);
            } else {
                parse_says(1, Facility[FAC_RESEARCH_HOSPITAL].name, -1, -1);
            }
            if (is_player) {
                Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                POP2("PROMETHEUS1", "biohazd_sm.pcx", base_id);
            } else if (*PbemActive) {
                POP2("PROMETHEUS1", "biohazd_sm.pcx", base_id);
            }
            return;
        }
        if (!has_project(FAC_HUMAN_GENOME_PROJECT, faction_id)
        && !has_project(FAC_LONGEVITY_VACCINE, faction_id)
        && !has_project(FAC_CLINICAL_IMMORTALITY, faction_id)) {
            if (plr->ranking >= (*CurrentTurn >= 150) + 4) {
                int base_id_tgt = -1;
                int num = (base->pop_size + 1) / 2;
                parse_num(1, num);
                if (is_player || is_visible) {
                    base_id_tgt = base_id;
                    Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                }
                for (int i = 0; i < *BaseCount; i++) {
                    BASE* b = &Bases[i];
                    if (map_range(base->x, base->y, b->x, b->y) <= num) {
                        if (!has_fac_built(FAC_RESEARCH_HOSPITAL, i)
                        && !has_fac_built(FAC_NANOHOSPITAL, i)
                        && !has_project(FAC_HUMAN_GENOME_PROJECT, b->faction_id)
                        && !has_project(FAC_LONGEVITY_VACCINE, b->faction_id)
                        && !has_project(FAC_CLINICAL_IMMORTALITY, b->faction_id)) {
                            b->pop_size = (b->pop_size + 1) / 2;
                            draw_tile(b->x, b->y, 2);
                            if (b->faction_id == *CurrentPlayerFaction) {
                                base_id_tgt = i;
                                Console_focus(MapWin, b->x, b->y, *CurrentPlayerFaction);
                            } else if (*PbemActive) {
                                base_id_tgt = i;
                            }
                        }
                    }
                }
                if (base_id_tgt >= 0) {
                    POP2("PROMETHEUS0", "biohazd_sm.pcx", base_id_tgt);
                }
            }
            return;
        }
        if (has_project(FAC_HUMAN_GENOME_PROJECT, faction_id)) {
            parse_says(1, Facility[FAC_HUMAN_GENOME_PROJECT].name, -1, -1);
        } else if (has_project(FAC_LONGEVITY_VACCINE, faction_id)) {
            parse_says(1, Facility[FAC_LONGEVITY_VACCINE].name, -1, -1);
        } else {
            parse_says(1, Facility[FAC_CLINICAL_IMMORTALITY].name, -1, -1);
        }
        if (is_player) {
            Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
            POP2("PROMETHEUS2", "biohazd_sm.pcx", base_id);
        } else if (*PbemActive) {
            POP2("PROMETHEUS2", "biohazd_sm.pcx", base_id);
        }
        return;
    case 7:
    case 8:
        if (*PbemActive) {
            return;
        }
        int offset, nearby, tx, ty;
        offset = map_rand.get(0, 20) + 1;
        nearby = 0;
        for (int i = 0; i < 9; i++) {
            MAP* sq = next_tile(base->x, base->y, i, &tx, &ty);
            if (sq && bonus_at(tx, ty) && is_known(tx, ty, faction_id)
            && (is_ocean(sq) || territory_avail(faction_id, tx, ty))) {
                offset = i;
                nearby = 1;
                break;
            }
        }
        MAP* bsq;
        bsq = next_tile(base->x, base->y, offset, &tx, &ty);
        if (bsq && is_known(tx, ty, faction_id)) {
            bool ocean = is_ocean(bsq);
            if (ocean || territory_avail(faction_id, tx, ty)) {
                int value = bonus_at(tx, ty);
                if (nearby && value) {
                    if (*CurrentTurn >= 75) {
                        parse_says(1, ResName[value - 1].name_plural, -1, -1);
                        bit_set(tx, ty, BIT_BONUS_RES, 0);
                        synch_bit(tx, ty, faction_id);
                        draw_tile(tx, ty, 2);
                        if (!bonus_at(tx, ty) && is_player) {
                            MapWin_set_center(MapWin, tx, ty, 1);
                            if (!shift_key_down()) {
                                for (int i = 0; i < 10; i++) {
                                    bit_set(tx, ty, BIT_BONUS_RES, 1);
                                    synch_bit(tx, ty, faction_id);
                                    draw_tile(tx, ty, 2);
                                    if (!shift_key_down() && !*MultiplayerActive) {
                                        clock_wait(20);
                                    }
                                    bit_set(tx, ty, BIT_BONUS_RES, 0);
                                    synch_bit(tx, ty, faction_id);
                                    draw_tile(tx, ty, 2);
                                    if (!shift_key_down() && !*MultiplayerActive) {
                                        clock_wait(20);
                                    }
                                }
                            }
                            if (!shift_key_down() && !*MultiplayerActive) {
                                clock_wait(100);
                            }
                            POP2("PETERSOUT", resource_icon(value, ocean, 0), -1);
                        }
                    }
                } else if (!value) { // Fix: always check that no resource exists before
                    bit_set(tx, ty, BIT_BONUS_RES, 1);
                    synch_bit(tx, ty, faction_id);
                    draw_tile(tx, ty, 2);
                    value = bonus_at(tx, ty);
                    if (value && is_player) {
                        MapWin_set_center(MapWin, tx, ty, 1);
                        if (!shift_key_down()) {
                            for (int i = 0; i < 10; i++) {
                                bit_set(tx, ty, BIT_BONUS_RES, 0);
                                synch_bit(tx, ty, faction_id);
                                draw_tile(tx, ty, 2);
                                if (!shift_key_down() && !*MultiplayerActive) {
                                    clock_wait(20);
                                }
                                bit_set(tx, ty, BIT_BONUS_RES, 1);
                                synch_bit(tx, ty, faction_id);
                                draw_tile(tx, ty, 2);
                                if (!shift_key_down() && !*MultiplayerActive) {
                                    clock_wait(20);
                                }
                            }
                        }
                        parse_says(1, ResName[value - 1].name_plural, -1, -1);
                        POP2("NEWRESOURCE", resource_icon(value, ocean, 1), -1);
                    }
                }
            }
        }
        return;
    case 9:
        if (!*SolarFlaresEvent && !map_rand.get(0, 5)) {
            bool found = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (Factions[i].satellites_energy || Factions[i].satellites_ODP) {
                    Factions[i].satellites_energy = 0;
                    Factions[i].satellites_ODP = 0;
                    found = 1;
                }
            }
            *SolarFlaresEvent = 1;
            if (found) {
                POP2("SOLARSTORM", "sun_sm.pcx", -1);
            } else {
                POP2("SOLARFLARE", "heat_sm.pcx", -1);
            }
        }
        return;
    case 10:
        if (!map_rand.get(0, 5)) {
            bool found = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (Factions[i].satellites_mineral) {
                    Factions[i].satellites_mineral = 0;
                    found = 1;
                }
            }
            if (found) {
                POP2("ASTEROID", "astm_sm.pcx", -1);
            }
        }
        return;
    case 11:
        if (*CurrentTurn >= 50 && *SunspotDuration <= -40
        && (*SunspotDuration <= -80 || *CurrentTurn < 80 || !map_rand.get(0, 3))) {
            *SunspotDuration = map_rand.get(0, 11) + 10;
            parse_num(0, 20);
            POP2("SUNSPOTS", "solar_sm.pcx", -1);
        }
        return;
    case 12:
        if (plr->energy_credits > 1000) {
            if (is_human(faction_id) || plr->energy_credits > 2000) {
                // Fix: use the suitable icon for energy market crash event
                // This also reduces reserves only by 1/2 instead of 3/4 like previously
                plr->energy_credits /= 2;
                if (is_player) {
                    parse_num(0, plr->energy_credits);
                    Console_update_data(MapWin, 0);
                    POP2("INFLATION", "genwarning_sm.pcx", -1);
                } else if (*PbemActive) {
                    parse_num(0, plr->energy_credits);
                    POP2("INFLATION", "genwarning_sm.pcx", -1 - faction_id);
                }
            }
        } else if (plr->ranking <= 4 && plr->energy_credits <= 500) {
            plr->energy_credits *= 2;
            if (is_player) {
                parse_num(0, plr->energy_credits);
                Console_update_data(MapWin, 0);
                POP2("ENERGYBOOM", "markbm_sm.pcx", -1);
            }
        }
        return;
    case 13:
        if (*CurrentTurn >= 75 && plr->ranking >= (*CurrentTurn >= 150) + 4) {
            bool show_event = 0;
            for (int i = 0; i < 21; i++) {
                MAP* sq = next_tile(base->x, base->y, i, &tx, &ty);
                if (sq && !is_ocean(sq) && sq->items & BIT_SOLAR && is_known(tx, ty, faction_id)) {
                    if (territory_avail(faction_id, tx, ty)) {
                        if (*PbemActive) {
                            show_event = 1;
                        } else if (is_player || is_visible) {
                            if (!show_event) {
                                Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                                if (!shift_key_down() && !*MultiplayerActive) {
                                    clock_wait(200);
                                }
                            }
                            show_event = 1;
                        }
                        bit_set(tx, ty, BIT_SOLAR, 0);
                        synch_bit(tx, ty, faction_id);
                        for (int j = 1; j < MaxPlayerNum; j++) {
                            sq->visible_items[j - 1] &= ~BIT_SOLAR;
                        }
                        if (show_event) {
                            draw_tile(tx, ty, 2);
                            if (!shift_key_down() && !*MultiplayerActive && !*PbemActive) {
                                clock_wait(50);
                            }
                        }
                    }
                }
            }
            if (show_event) {
                POP2("HAIL", "hail_sm.pcx", base_id);
            }
        }
        return;
    case 14:
        if (*PbemActive || *CurrentTurn < 75 || *GameState & STATE_IS_SCENARIO
        || plr->base_count < 8 || plr->ranking < 6) {
            return;
        }
        if (!mapsq(*MountPlanetX, *MountPlanetY)
        || (bit2_at(*MountPlanetX, *MountPlanetY) & (LM_DISABLE|LM_VOLCANO)) != LM_VOLCANO) {
            return;
        }
        if (map_range(bx, by, *MountPlanetX, *MountPlanetY) > 5
        || !is_known(*MountPlanetX, *MountPlanetY, faction_id)) {
            return;
        }
        bool visible;
        visible = is_known(*MountPlanetX, *MountPlanetY, *CurrentPlayerFaction);
        for (int i = 0; i < 81; i++) {
            MAP* sq = next_tile(bx, by, i, &tx, &ty);
            if (sq) {
                bit_set(tx, ty, BIT_SENSOR|BIT_THERMAL_BORE|BIT_ECH_MIRROR|BIT_CONDENSER|BIT_FOREST|\
                    BIT_SOIL_ENRICHER|BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_FUNGUS|BIT_MINE|BIT_MAGTUBE|BIT_ROAD, 0);
                synch_bit(tx, ty, *CurrentPlayerFaction);
                rocky_set(tx, ty, LEVEL_ROCKY);
                bool volcano = (sq->landmarks & (LM_DISABLE|LM_VOLCANO)) == LM_VOLCANO;
                for (int j = *VehCount - 1; j >= 0; j--) {
                    VEH* veh = &Vehs[j];
                    if (veh->x == tx && veh->y == ty) {
                        if (volcano) {
                            veh_kill(j);
                        } else {
                            veh->damage_taken += veh->cur_hitpoints() / 2;
                        }
                    }
                }
                int base_id_tgt = base_at(tx, ty);
                if (base_id_tgt >= 0) {
                    BASE* b = &Bases[base_id_tgt];
                    if (b->faction_id == *CurrentPlayerFaction && !is_player) {
                        parse_says(0, b->name, -1, -1);
                        visible = 1;
                    }
                    if (i < 9 && !is_objective(base_id_tgt)) {
                        mod_base_kill(base_id_tgt);
                    } else if (i < 25) {
                        b->pop_size = (b->pop_size + 3) / 4;
                    } else if (i < 49) {
                        b->pop_size = (b->pop_size + 1) / 2;
                    } else {
                        b->pop_size = (3 * (b->pop_size + 1)) / 4;
                    }
                }
            }
        }
        if (!visible || !Console_focus(MapWin, *MountPlanetX, *MountPlanetY, *CurrentPlayerFaction)) {
            draw_map(1);
        }
        *DustCloudDuration = 10;
        popp(ScriptFile, "BIGFATVOLCANO", 0, "volc_sm.pcx", 0);
        return;
    case 15:
        if (*PbemActive || *CurrentTurn < 75 || *GameState & STATE_IS_SCENARIO
        || plr->base_count < 8 || plr->ranking < 7 || near_landmark(bx, by)) {
            return;
        }
        visible = is_known(bx, by, *CurrentPlayerFaction);
        int sea_count;
        sea_count = 0;
        for (int i = 0; i < 49; i++) {
            MAP* sq = next_tile(bx, by, i, &tx, &ty);
            if (sq && is_ocean(sq)) {
                if (i < 25 || ++sea_count > 4) {
                    return;
                }
            }
        }
        for (int i = 0; i < 49; i++) {
            MAP* sq = next_tile(bx, by, i, &tx, &ty);
            if (sq) {
                bit_set(tx, ty, BIT_SENSOR|BIT_THERMAL_BORE|BIT_ECH_MIRROR|BIT_CONDENSER|BIT_FOREST|\
                    BIT_SOIL_ENRICHER|BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_FUNGUS|BIT_MINE|BIT_MAGTUBE|BIT_ROAD, 0);
                synch_bit(tx, ty, *CurrentPlayerFaction);
                int base_id_tgt = base_at(tx, ty);
                if (base_id_tgt >= 0) {
                    BASE* b = &Bases[base_id_tgt];
                    if (b->faction_id == *CurrentPlayerFaction && !is_player) {
                        parse_says(0, b->name, -1, -1);
                        visible = 1;
                    }
                    mod_base_kill(base_id_tgt);
                }
                for (int j = *VehCount - 1; j >= 0; j--) {
                    VEH* veh = &Vehs[j];
                    if (veh->x == tx && veh->y == ty) {
                        if (veh->faction_id == *CurrentPlayerFaction) {
                            visible = 1;
                        }
                        veh_kill(j);
                    }
                }
            }
        }
        mod_world_crater(bx, by);
        world_climate();
        if (!visible || !Console_focus(MapWin, bx, by, *CurrentPlayerFaction)) {
            draw_map(1);
        }
        *DustCloudDuration = 10;
        popp(ScriptFile, "EATTHIS", 0, "astp_sm.pcx", 0);
        return;
    case 16:
        if (has_tech(Facility[FAC_BIOLOGY_LAB].preq_tech, faction_id)) {
            bool has_lab = has_fac_built(FAC_BIOLOGY_LAB, base_id);
            bool show_event = 0;
            if (has_lab || plr->ranking >= (*CurrentTurn >= 150) + 4) {
                for (int i = 0; i < 21; i++) {
                    MAP* sq = next_tile(base->x, base->y, i, &tx, &ty);
                    if (sq && !is_ocean(sq) && sq->items & (BIT_FOREST|BIT_FARM) && is_known(tx, ty, faction_id)) {
                        if (territory_avail(faction_id, tx, ty)) {
                            if (!has_lab) {
                                if (!show_event) {
                                    Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                                    if (!shift_key_down() && !*MultiplayerActive) {
                                        clock_wait(200);
                                    }
                                }
                                bit_set(tx, ty, BIT_FOREST|BIT_SOIL_ENRICHER|BIT_FARM, 0);
                                synch_bit(tx, ty, faction_id);
                                for (int j = 1; j < MaxPlayerNum; j++) {
                                    sq->visible_items[j - 1] &= ~(BIT_FOREST|BIT_SOIL_ENRICHER|BIT_FARM);
                                }
                                draw_tile(tx, ty, 2);
                                if (!shift_key_down() && !*MultiplayerActive) {
                                    clock_wait(50);
                                }
                            }
                            show_event = 1;
                        }
                    }
                }
                if (show_event) {
                    if (has_lab) {
                        base->event_flags |= BEVENT_BUMPER;
                        base->random_event_turns = conf.base_event_turns;
                    }
                    if (is_player) {
                        if (!Console_focus(MapWin, base->x, base->y, faction_id)) {
                            draw_radius(base->x, base->y, 2, 2);
                        }
                    }
                    if (is_player || *PbemActive) {
                        POP2(has_lab ? "BLIGHT1" : "BLIGHT", "biohazd_sm.pcx", base_id);
                    }
                }
            }
        }
        return;
    case 17:
        if (has_tech(Facility[FAC_ENERGY_BANK].preq_tech, faction_id)) {
            bool has_bank = 0;
            bool show_event = 0;
            if (has_facility(FAC_ENERGY_BANK, base_id)) {
                has_bank = 1;
            } else if (plr->ranking < (*CurrentTurn >= 150) + 4) {
                return;
            }
            for (int i = 0; i < 21; i++) {
                MAP* sq = next_tile(base->x, base->y, i, &tx, &ty);
                if (sq && !is_ocean(sq) && sq->items & BIT_MINE && is_known(tx, ty, faction_id)) {
                    if (territory_avail(faction_id, tx, ty)) {
                        if (!has_bank) {
                            if (!show_event) {
                                Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                                if (!shift_key_down() && !*MultiplayerActive) {
                                    clock_wait(200);
                                }
                            }
                            bit_set(tx, ty, BIT_MINE, 0);
                            synch_bit(tx, ty, faction_id);
                            for (int j = 1; j < MaxPlayerNum; j++) {
                                sq->visible_items[j - 1] &= ~BIT_MINE;
                            }
                            draw_tile(tx, ty, 2);
                            if (!shift_key_down() && !*MultiplayerActive) {
                                clock_wait(50);
                            }
                        }
                        show_event = 1;
                    }
                }
            }
            if (show_event) {
                if (has_bank) {
                    plr->energy_credits += 50;
                }
                if (is_player) {
                    if (!Console_focus(MapWin, base->x, base->y, faction_id)) {
                        draw_radius(base->x, base->y, 2, 2);
                    }
                }
                if (is_player || *PbemActive) {
                    POP2(has_bank ? "SURGE1" : "SURGE", "markbm_sm.pcx", base_id);
                }
            }
        }
        return;
    case 18:
        if (has_tech(Facility[FAC_NETWORK_NODE].preq_tech, faction_id)) {
            if (plr->tech_accumulated >= mod_tech_rate(faction_id) / 4) {
                if (has_fac_built(FAC_NETWORK_NODE, base_id)) {
                    plr->tech_accumulated = mod_tech_rate(faction_id);
                    if (is_player) {
                        Console_focus(MapWin, base->x, base->y, faction_id);
                    }
                    if (is_player || *PbemActive) {
                        POP2("NETBONUS", "markbm_sm.pcx", base_id);
                    }
                } else if (plr->ranking >= (*CurrentTurn >= 150) + 4) {
                    plr->tech_accumulated = 0;
                    plr->net_random_event = 0;
                    if (is_player) {
                        Console_focus(MapWin, base->x, base->y, faction_id);
                    }
                    if (is_player || *PbemActive) {
                        POP2("NETCRASH", "netcr_sm.pcx", base_id);
                    }
                }
            }
        }
        return;
    case 19:
        if (has_tech(Facility[FAC_CHILDREN_CRECHE].preq_tech, faction_id)) {
            if (has_fac_built(FAC_CHILDREN_CRECHE, base_id)) {
                int added = 0;
                // Fix: check that maximum population is not exceeded
                while (base->nutrient_surplus > 0 && base->pop_size < MaxBasePopSize && ++added < 3) {
                    base->pop_size++;
                    set_base(base_id);
                    base_compute(1);
                }
                if (added) {
                    if (is_player) {
                        if (!Console_focus(MapWin, base->x, base->y, faction_id)) {
                            draw_tile(base->x, base->y, 2);
                        }
                    }
                    if (is_player || *PbemActive) {
                        if (!is_alien(faction_id)) {
                            POP2("CRECHE1", "pop_sm.pcx", base_id);
                        } else {
                            POP2("CRECHE1", "al_pop_sm.pcx", base_id);
                        }
                    }
                }
            } else if (plr->ranking >= (*CurrentTurn >= 150) + 4 && !base->assimilation_turns_left) {
                base->assimilation_turns_left = 5;
                if (is_player) {
                    Console_focus(MapWin, base->x, base->y, faction_id);
                }
                if (is_player || *PbemActive) {
                    if (!is_alien(faction_id)) {
                        POP2("CRECHE", "pop_sm.pcx", base_id);
                    } else {
                        POP2("CRECHE", "al_pop_sm.pcx", base_id);
                    }
                }
            }
        }
        return;
    case 20:
        if (!*PbemActive && *CurrentTurn >= 75 && plr->ranking >= (*CurrentTurn >= 150) + 4) {
            bool show_event = 0;
            for (int i = 0; i < 21; i++) {
                MAP* sq = next_tile(base->x, base->y, i, &tx, &ty);
                if (sq && is_ocean(sq) && sq->items & BIT_FARM && is_known(tx, ty, faction_id)) {
                    // Fix: remove another is_ocean check that prevented the event from happening
                    if (territory_avail(faction_id, tx, ty)) {
                        if (is_player || is_visible) {
                            if (!show_event) {
                                Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                                if (!shift_key_down() && !*MultiplayerActive) {
                                    clock_wait(200);
                                }
                            }
                            show_event = 1;
                        }
                        bit_set(tx, ty, BIT_FARM, 0);
                        synch_bit(tx, ty, faction_id);
                        for (int j = 1; j < MaxPlayerNum; j++) {
                            sq->visible_items[j - 1] &= ~BIT_FARM;
                        }
                        if (show_event) {
                            draw_tile(tx, ty, 2);
                            if (!shift_key_down() && !*MultiplayerActive) {
                                clock_wait(50);
                            }
                        }
                    }
                }
            }
            if (show_event) {
                // Added event icon since previously there was none defined
                POP2("KELPWIPE", "kelp_sm.pcx", is_player ? base_id : -1);
            }
        }
        return;
    case 21:
        if (!*PbemActive && *CurrentTurn >= 75 && plr->ranking >= (*CurrentTurn >= 150) + 4) {
            bool show_event = 0;
            for (int i = 0; i < 21; i++) {
                MAP* sq = next_tile(base->x, base->y, i, &tx, &ty);
                if (sq && is_ocean(sq) && sq->items & BIT_MINE && is_known(tx, ty, faction_id)) {
                    // Fix: remove another is_ocean check that prevented the event from happening
                    if (territory_avail(faction_id, tx, ty)) {
                        if (is_player || is_visible) {
                            if (!show_event) {
                                Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                                if (!shift_key_down() && !*MultiplayerActive) {
                                    clock_wait(200);
                                }
                            }
                            show_event = 1;
                        }
                        bit_set(tx, ty, BIT_MINE, 0);
                        synch_bit(tx, ty, faction_id);
                        for (int j = 1; j < MaxPlayerNum; j++) {
                            sq->visible_items[j - 1] &= ~BIT_MINE;
                        }
                        if (show_event) {
                            draw_tile(tx, ty, 2);
                            if (!shift_key_down() && !*MultiplayerActive) {
                                clock_wait(50);
                            }
                        }
                    }
                }
            }
            if (show_event) {
                // Added event icon since previously there was none defined
                POP2("PLATFORMWIPE", "subbase_sm.pcx", is_player ? base_id : -1);
            }
        }
        return;
    default:
        return;
    }
}

void __cdecl turn_upkeep() {
    debug("turn_upkeep %d bases: %d vehs: %d\n", (*CurrentTurn)+1, *BaseCount, *VehCount);
    snprintf(ThinkerVars->build_date, 12, MOD_DATE);
    if (*CurrentTurn == 0) {
        init_world_config();
    }
    if (DEBUG) {
        if (conf.debug_mode) {
            *GameState |= STATE_DEBUG_MODE;
            *GamePreferences |= PREF_ADV_FAST_BATTLE_RESOLUTION;
        } else {
            *GameState &= ~STATE_DEBUG_MODE;
        }
    }
    // Original game turn upkeep starts here
    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
        VEH* veh = &Vehs[veh_id];
        if (veh->triad() == TRIAD_AIR && veh->range() && veh_speed(veh_id, 0) - veh->moves_spent > 0) {
            MAP* sq = mapsq(veh->x, veh->y);
            if (sq && sq->base_who() < 0 && !(sq->items & BIT_AIRBASE)
            && !mod_stack_check(veh_id, 6, ABL_CARRIER, -1, -1)) {
                // Fix bug here that prevented the turn from advancing
                // while any needlejet in flight has moves left.
                if (veh->movement_turns < veh->range() - 1) {
                    ++veh->movement_turns;
                }
                veh->state |= VSTATE_HAS_MOVED;
                int dmg_val;
                if (veh->range() != 1 || veh->chassis_type() != CHS_COPTER) {
                    dmg_val = veh->cur_hitpoints();
                } else {
                    dmg_val = 3 * veh->reactor_type();
                }
                veh->damage_taken = clamp(veh->damage_taken + dmg_val, 0, 255);
                if (veh->max_hitpoints() - veh->damage_taken <= 0) {
                    kill(veh_id);
                }
            }
        }
    }
    *dword_90DB84 = 0; // action_terraform
    rankings(1);
    *CurrentMissionYear = game_year(++(*CurrentTurn));
    MapWin_main_caption(MapWin);

    if (*ClimateFutureChange) {
        int val = *ClimateValueA + *ClimateValueC;
        *ClimateValueC += *ClimateValueA;
        if (*ClimateValueC >= *ClimateValueB) {
            *ClimateValueC = val - *ClimateValueB;
            int alt_val = clamp(*ClimateFutureChange, -1, 1);
            *ClimateFutureChange -= alt_val;
            *MapSeaLevel += alt_val;
            world_climate();
            draw_map(1);
            if (alt_val > 0) {
                popp(ScriptFile, "SEARISING", 0, "searis_sm.pcx", 0);
            } else {
                popp(ScriptFile, "SEAFALLING", 0, "seafal_sm.pcx", 0);
            }
        }
    }
    mod_alien_fauna();
    mod_do_fungal_towers();

    if ((*CurrentTurn == 4 && !game_randv(4))
    || (*CurrentTurn == 5 && !game_randv(3))
    || (*CurrentTurn == 6 && !game_randv(2))
    || *CurrentTurn >= 7) {
        bool aliens_arrive = false;
        for (int i = 1; i < MaxPlayerNum; i++) {
            Faction& plr = Factions[i];
            if (is_alien(i) || (!plr.base_count && !_strcmpi(MFactions[i].filename, "FUNGBOY"))) {
                // TODO: investigate more consistent ways to mark factions for late spawns
                int flags = plr.player_flags | PFLAG_MAP_REVEALED;
                if (flags == -1 && plr.diff_level == -1 && !is_human(i) && !is_alive(i)) {
                    if (is_alien(i)) {
                        aliens_arrive = true;
                    }
                    set_alive(i, true);
                    mod_setup_player(i, -282, 0);
                }
            }
        }
        if (aliens_arrive) {
            popp(ScriptFile, "ALIENSARRIVE", 0, "al_land_sm.pcx", 0);
            interlude(21, 0, 1, 0);
        }
    }
    int caretake_id = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_alive(i) && !_strcmpi(MFactions[i].filename, "CARETAKE")) { // Added is_alive check
            caretake_id = i;
        }
    }
    if (caretake_id) {
        for (int i = 0; i < *BaseCount; i++) {
            int faction_id = Bases[i].faction_id;
            if (faction_id != caretake_id
            && Bases[i].queue_items[0] == -FAC_ASCENT_TO_TRANSCENDENCE
            && !has_treaty(caretake_id, faction_id, DIPLO_VENDETTA)) {
                if (has_treaty(faction_id, caretake_id, DIPLO_PACT)) {
                    pact_ends(faction_id, caretake_id);
                }
                treaty_on(caretake_id, faction_id, DIPLO_VENDETTA|DIPLO_COMMLINK);
                set_treaty(caretake_id, faction_id, DIPLO_UNK_40, 1);
                Factions[faction_id].diplo_spoke[caretake_id] = *CurrentTurn;
                if (faction_id == *CurrentPlayerFaction) {
                    X_pops_11("NOTRANSCEND", 0x100000, FactionPortraits[caretake_id], pop_wait);
                }
            }
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        for (int j = 1; j < MaxPlayerNum; j++) {
            if (i != j && is_alien(i) && is_alien(j)) {
                set_treaty(i, j, DIPLO_UNK_80000000|DIPLO_UNK_40000000|DIPLO_UNK_8000000|\
                    DIPLO_MAJOR_ATROCITY_VICTIM|DIPLO_ATROCITY_VICTIM|DIPLO_UNK_800|\
                    DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE|DIPLO_VENDETTA|DIPLO_COMMLINK, 1);
                set_agenda(i, j, AGENDA_PERMANENT|AGENDA_UNK_800|AGENDA_UNK_400|AGENDA_UNK_20|\
                    AGENDA_FIGHT_TO_DEATH|AGENDA_UNK_4|AGENDA_UNK_1, 1);
                Factions[i].diplo_spoke[j] = *CurrentTurn;
            }
        }
    }
    random_events(0);
    if (*CurrentTurn == 75 && has_tech(Facility[FAC_BIOLOGY_LAB].preq_tech, *CurrentPlayerFaction)) {
        interlude(1, 0, 1, 0);
    }
    if (voice_of_planet()) {
        Faction& plr = Factions[*CurrentPlayerFaction];
        if (!(plr.player_flags & PFLAG_UNK_2000)) {
            plr.player_flags |= PFLAG_UNK_2000;
            parse_says(0, get_title(*CurrentPlayerFaction), -1, -1);
            parse_says(1, get_name(*CurrentPlayerFaction), -1, -1);
            popp(ScriptFile, "ASCENT", 0, "asctran_sm.pcx", 0);
        }
    }
    if ((*CurrentTurn & 3) == 1) {
        reset_territory();
    }
    do_all_non_input();
}

void __cdecl faction_upkeep(int faction_id) {
    Faction* f = &Factions[faction_id];
    debug("faction_upkeep %d %d\n", *CurrentTurn, faction_id);

    init_save_game(faction_id);
    plans_upkeep(faction_id);
    reset_netmsg_status();
    *ControlUpkeepA = 1;
    social_upkeep(faction_id);
    do_all_non_input();
    repair_phase(faction_id);
    do_all_non_input();
    production_phase(faction_id);
    do_all_non_input();
    if (full_game_turn()) {
        allocate_energy(faction_id);
        do_all_non_input();
        enemy_diplomacy(faction_id);
        do_all_non_input();
        enemy_strategy(faction_id);
        do_all_non_input();
        /*
        Thinker-specific AI planning routines.
        Note that move_upkeep is only updated after all the production is done,
        so that the movement code can utilize up-to-date priority maps.
        This means we cannot use move_upkeep maps or variables in production phase.
        */
        mod_social_ai(faction_id, -1, -1, -1, -1, 0);
        probe_upkeep(faction_id);
        move_upkeep(faction_id, UM_Full);
        do_all_non_input();

        if (!is_human(faction_id) && *GameRules & RULES_VICTORY_ECONOMIC
        && has_tech(Rules->tech_preq_economic_victory, faction_id)) {
            int cost = corner_market(faction_id);
            if (!victory_done() && f->corner_market_cost <= 0 && f->energy_credits > cost) {
                f->corner_market_turn = *CurrentTurn + Rules->turns_corner_global_energy_market;
                f->corner_market_cost = cost;
                f->energy_credits -= cost;
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_noun(faction_id), -1, -1);
                parse_says(3, get_adjective(faction_id), -1, -1);
                ParseNumTable[0] = game_year(f->corner_market_turn);
                popp(ScriptFile, "CORNERWARNING", 0, "econwin_sm.pcx", 0);
            }
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id) {
            // Fix: clear hurry production flags moved to mod_base_upkeep
            base->state_flags &= ~BSTATE_UNK_1;
        }
    }
    f->energy_credits -= f->hurry_cost_total;
    f->hurry_cost_total = 0;
    if (f->energy_credits < 0) {
        f->energy_credits = 0;
    }
    if (!f->base_count && !has_active_veh(faction_id, PLAN_COLONY)) {
        mod_eliminate_player(faction_id, 0);
    }
    if (f->base_count && f->tech_research_id < 0 && *NetUpkeepState != 1
    && !(*GameRules & RULES_SCN_NO_TECH_ADVANCES)) {
        f->tech_research_id = mod_tech_selection(faction_id);
    }
    *ControlUpkeepA = 0;
    Paths->xDst = -1;
    Paths->yDst = -1;

    if (full_game_turn()) {
        if (faction_id == MapWin->cOwner
        && !(*GameState & (STATE_COUNCIL_HAS_CONVENED | STATE_DISPLAYED_COUNCIL_AVAIL_MSG))
        && can_call_council(faction_id, 0) && !(*GameState & STATE_GAME_DONE)) {
            *GameState |= STATE_DISPLAYED_COUNCIL_AVAIL_MSG;
            popp(ScriptFile, "COUNCILOPEN", 0, "council_sm.pcx", 0);
        }
        if (!is_human(faction_id)) {
            call_council(faction_id);
        }
    }
    if (!*MultiplayerActive && *GamePreferences & PREF_BSC_AUTOSAVE_EACH_TURN
    && faction_id == MapWin->cOwner) {
        auto_save();
    }
    flushlog();
}

void __cdecl repair_phase(int faction_id) {
    Faction* f = &Factions[faction_id];
    f->ODP_deployed = 0;
    Points tiles;

    for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
        VEH* veh = &Vehs[veh_id];
        if (veh->faction_id != faction_id) {
            continue;
        }
        MAP* sq = mapsq(veh->x, veh->y);
        const bool at_base = sq && sq->is_base();
        const int triad = veh->triad();
        veh->iter_count = 0;
        veh->moves_spent = 0;
        veh->flags &= ~VFLAG_UNK_1000;
        veh->state &= ~(VSTATE_WORKING|VSTATE_UNK_2000|VSTATE_UNK_2);

        if (conf.activate_skipped_units) {
            veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
        }
        if (sq && !at_base) {
            if (veh->faction_id == MapWin->cOwner || veh->is_visible(MapWin->cOwner)) {
                tiles.insert({veh->x, veh->y});
            }
        }
        if ( !((*CurrentTurn + veh_id) & 3) ) {
            veh->state &= ~VSTATE_UNK_800;
            if (veh->flags & VFLAG_UNK_2) {
                veh->flags &= ~VFLAG_UNK_2;
            } else {
                veh->flags &= ~VFLAG_UNK_1;
            }
        }
        if (veh->order == ORDER_SENTRY_BOARD || veh->order == ORDER_HOLD) {
            if (veh->waypoint_y[0]) {
                if (!--veh->waypoint_y[0]) {
                    veh->order = ORDER_NONE;
                }
            }
        }
        if (veh->state & VSTATE_UNK_8) {
            if (at_base
            || (triad == TRIAD_LAND && sq->is_bunker())
            || (triad == TRIAD_AIR && sq->is_airbase())) {
                veh->state &= ~VSTATE_UNK_8;
            }
        }
        if (veh->unit_id == BSC_FUNGAL_TOWER) {
            veh->faction_id = 0;
        }
        if (!sq || !veh->damage_taken
        || (veh->is_battle_ogre() && conf.repair_battle_ogre <= 0)
        || !(veh->unit_id == BSC_FUNGAL_TOWER || !(veh->state & VSTATE_HAS_MOVED))) {
            continue;
        }
        int value = conf.repair_minimal;
        int base_id = base_at(veh->x, veh->y);

        if (veh->unit_id != BSC_FUNGAL_TOWER) {
            if (veh->is_native_unit() && sq->is_fungus()) {
                assert(sq->alt_level() >= ALT_OCEAN_SHELF);
                value = conf.repair_fungus;
            }
            if (conf.repair_friendly > 0
            && whose_territory(faction_id, veh->x, veh->y, 0, 0) == faction_id) {
                value += conf.repair_friendly;
            }
            if (triad == TRIAD_AIR && sq->items & BIT_AIRBASE) {
                value += conf.repair_airbase;
            }
            if (triad == TRIAD_LAND && sq->items & BIT_BUNKER) {
                value += conf.repair_bunker;
            }
        }
        if (base_id >= 0 && !Bases[base_id].drone_riots_active()) {
            value += conf.repair_base;
            // Fix: consider secret projects built by the base owner instead of the veh owner
            if (!veh->is_native_unit()) {
                if ((triad == TRIAD_LAND && has_facility(FAC_COMMAND_CENTER, base_id))
                || (triad == TRIAD_SEA && has_facility(FAC_NAVAL_YARD, base_id))
                || (triad == TRIAD_AIR && has_facility(FAC_AEROSPACE_COMPLEX, base_id))) {
                    value += conf.repair_base_facility;
                }
            } else if (breed_mod(base_id, faction_id)) {
                value += conf.repair_base_native;
            }
        }
        bool repair_abl = false;
        if (triad == TRIAD_LAND) {
            if ((!veh_cargo(veh_id) && veh->order == ORDER_SENTRY_BOARD) || is_ocean(sq)) {
                int iter_id = veh_top(veh_id);
                while (iter_id >= 0) {
                    if (iter_id != veh_id && has_abil(Vehs[iter_id].unit_id, ABL_REPAIR)) {
                        repair_abl = true;
                    }
                    iter_id = Vehs[iter_id].next_veh_id_stack;
                }
                if (repair_abl) {
                    value *= 2;
                }
            }
        }
        if (has_project(FAC_NANO_FACTORY, faction_id)) {
            value += conf.repair_nano_factory;
        }
        if (veh->is_battle_ogre()) {
             value = min(value, conf.repair_battle_ogre);
        }
        int repair_limit = 0;
        int repair_value = value * veh->reactor_type();

        if (faction_id && !repair_abl && base_id < 0 && !has_project(FAC_NANO_FACTORY, faction_id)) {
            repair_limit = 2 * veh->reactor_type();
            if (sq->is_fungus()) {
                assert(sq->alt_level() >= ALT_OCEAN_SHELF);
                if (veh->is_native_unit()) {
                    repair_limit = 0;
                }
                if (has_project(FAC_XENOEMPATHY_DOME, faction_id)) {
                    repair_limit = 0;
                    repair_value += veh->reactor_type();
                }
            }
        }
        int damage = clamp(veh->damage_taken - repair_value, repair_limit, 255);
        if (damage < veh->damage_taken) {
            veh->damage_taken = damage;
            if (damage <= repair_limit && is_human(faction_id)
            && veh->order == ORDER_SENTRY_BOARD
            && (triad != TRIAD_LAND || !is_ocean(sq))) {
                veh->order = ORDER_NONE;
            }
        }
    }
    for (auto& p : tiles) {
        draw_tile(p.x, p.y, -1);
    }
    do_all_draws();
}

void __cdecl production_phase(int faction_id) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    debug("production_phase %d %d\n", *CurrentTurn, faction_id);
    f->best_mineral_output = 0;
    f->energy_surplus_total = 0;
    f->facility_maint_total = 0;
    f->turn_commerce_income = 0;
    tech_effects(faction_id);

    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id != i && is_alive(faction_id) && is_alive(i)) {
            assert(f->loan_balance[i] >= 0);
            if (f->loan_balance[i] && !Factions[i].sanction_turns) {
                assert(f->loan_payment[i] > 0);
                if (at_war(faction_id, i)) {
                    f->loan_balance[i] += f->loan_payment[i];
                } else {
                    int payment = clamp(f->energy_credits, 0, f->loan_payment[i]);
                    Factions[i].energy_credits += payment;
                    f->energy_credits -= payment;
                    f->loan_balance[i] -= payment;
                    if (payment < f->loan_payment[i]) {
                        f->loan_balance[i] += f->loan_payment[i] - payment;
                    }
                }
            }
        }
    }

    f->player_flags &= ~PFLAG_SELF_AWARE_COLONY_LOST_MAINT;
    *EnergyCredits = f->energy_credits;
    /*
    Reset all fields listed below.
    int32_t social_support[8];
    int32_t social_psych[8][9];
    int32_t social_effic[9];
    int32_t unk_45;
    int32_t unk_46;
    int32_t unk_47;
    */
    assert((int)&f->social_support + 0x170 == (int)&f->nutrient_surplus_total);
    memset(&f->social_support, 0, 0x170);

    f->nutrient_surplus_total = 0;
    f->labs_total = 0;

    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        if (Bases[base_id].faction_id == faction_id) {
            if (mod_base_upkeep(base_id)) {
                base_id--; // Base was removed for some reason
            }
            do_all_non_input();
        }
    }
    /*
    Apply the original AI facility maintenance discounts
    These modifiers are not displayed in budget screens, they are just applied here
    */
    if (!is_human(faction_id) && *DiffLevel >= DIFF_THINKER) {
        if (f->facility_maint_total) {
            int value;
            if (*DiffLevel == DIFF_THINKER) {
                value = f->facility_maint_total / 3;
                f->energy_credits += value;
            } else {
                value = f->facility_maint_total * 2 / 3;
                f->energy_credits += value;
            }
            f->facility_maint_total -= value;
        }
    }
    /*
    INTEREST = Energy reserves interest.
    Non-zero = constant percentage per turn (including negative)
    Zero     = +1/base each turn
    */
    if (m->rule_flags & RFLAG_INTEREST) {
        int rule_interest = m->rule_interest;
        if (rule_interest) {
            f->energy_credits += f->energy_credits * rule_interest / 100;
        } else {
            for (int base_id = 0; base_id < *BaseCount; base_id++) {
                if (Bases[base_id].faction_id == faction_id) {
                    f->energy_credits++;
                    f->energy_surplus_total++;
                }
            }
        }
    }
    f->unk_18 = f->energy_surplus_total;
    f->unk_17 = f->energy_surplus_total + f->turn_commerce_income - f->facility_maint_total;

    if (*CurrentTurn == 1) {
        int techs = m->rule_tech_selected;
        *SkipTechScreenA = 1;
        while (--techs >= 0) {
            tech_advance(faction_id);
        }
        *SkipTechScreenA = 0;
    }
    if (full_game_turn()) {
        if (f->sanction_turns) {
            f->sanction_turns--;
            if (!f->sanction_turns && faction_id == MapWin->cOwner) {
                if (!is_alien(faction_id)) {
                    popp(ScriptFile, "SANCTIONSEND", 0, "council_sm.pcx", 0);
                } else {
                    popp(ScriptFile, "SANCTIONSENDALIEN", 0, "Alopdir.pcx", 0);
                }
            }
        }
    }
}

void __cdecl allocate_energy(int faction_id) {
    Faction* plr = &Factions[faction_id];
    MFaction* fac = &MFactions[faction_id];
    int alloc_val;
    int psych_val;

    if (is_human(faction_id)) {
        if (plr->diff_level > 1 && faction_id == MapWin->cOwner && plr->base_count > 1) {
            if (plr->energy_credits < 100 && 11 * plr->energy_credits - 10 * *EnergyCredits < 0) {
                if (10 - plr->SE_alloc_labs - plr->SE_alloc_psych >= 5) {
                    if (has_tech(Units[BSC_FORMERS].preq_tech, faction_id) && !(*CurrentTurn % 5)) {
                        popp(ScriptFile, "INFRASTRUCTURE2", 0, "genwarning_sm.pcx", 0);
                    }
                } else if (popp(ScriptFile, "INFRASTRUCTURE", 0, "genwarning_sm.pcx", 0)) {
                    social_select(faction_id);
                }
            }
        }
        return;
    }
    alloc_val = 10 - plr->SE_alloc_psych - plr->SE_alloc_labs;
    if (plr->SE_police_pending >= -2) {
        if (plr->SE_police_pending == -2) {
            psych_val = 2;
        } else {
            psych_val = plr->SE_police_pending <= 1;
        }
    } else {
        psych_val = 4;
    }
    /*
    Modified psych energy allocation for rewritten governor specialist priorities
    where the scoring heuristic avoids situations in which the governor is forced
    to convert workers into specialists as counted by specialist_adjust.
    This replaces the original game version which used incorrect base iterators
    while the rest of this function uses previous decision methods.
    */
    if (has_project(FAC_TELEPATHIC_MATRIX, faction_id)) {
        plr->SE_alloc_psych = 0;
    } else {
        int pop_total = 0;
        int pop_score = 0;
        for (int base_id = 0; base_id < *BaseCount; base_id++) {
            BASE* b = &Bases[base_id];
            if (b->faction_id == faction_id && base_can_riot(base_id, true)) {
                int coeff_psych = 4;
                if (has_fac_built(FAC_HOLOGRAM_THEATRE, base_id)
                || (has_project(FAC_VIRTUAL_WORLD, faction_id)
                && has_fac_built(FAC_NETWORK_NODE, base_id))) {
                    coeff_psych += 2;
                }
                if (has_fac_built(FAC_RESEARCH_HOSPITAL, base_id)) {
                    coeff_psych += 1;
                }
                if (has_fac_built(FAC_NANOHOSPITAL, base_id)) {
                    coeff_psych += 1;
                }
                if (has_fac_built(FAC_TREE_FARM, base_id)) {
                    coeff_psych += 2;
                }
                if (has_fac_built(FAC_HYBRID_FOREST, base_id)) {
                    coeff_psych += 2;
                }
                pop_score += coeff_psych
                    * (b->drone_riots_active() ? 2 : 1)
                    * (b->assimilation_turns_left ? 1 : 2)
                    * (b->energy_surplus >= min(20, 2 + 2*b->pop_size) ? 2 : 1)
                    * max(0, (coeff_psych > 4 ? b->pop_size/4 : 0)
                    + 2*b->specialist_adjust + b->drone_total - b->talent_total);
                pop_total += b->pop_size;
            }
        }
        pop_score = pop_score * clamp(4 - plr->SE_police_pending, 1, 4) / 4;
        if (pop_score <= 3*pop_total) {
            if (plr->SE_alloc_psych > psych_val / 2) {
                if (pop_score <= 2*pop_total || plr->SE_alloc_psych - psych_val >= 2
                || plr->SE_police_pending >= (plr->SE_alloc_psych <= 1)) {
                    --plr->SE_alloc_psych;
                }
            }
        } else if (plr->SE_alloc_psych < psych_val) {
            if (++plr->SE_alloc_psych < psych_val && plr->SE_police_pending < 0) {
                plr->SE_alloc_psych += (pop_score >= 4*pop_total);
            }
        }
        plr->SE_alloc_psych = clamp(plr->SE_alloc_psych,
            (plr->SE_police_pending >= -1 || pop_score <= 2*pop_total ? 0 : 2), 4);
    }

    plr->SE_alloc_labs = (plr->AI_tech - plr->AI_power - plr->SE_alloc_psych + 11) / 2 - plr->AI_wealth;
    if (*CurrentTurn < 25 * (plr->AI_tech - plr->AI_power - plr->AI_wealth + 3)) {
        ++plr->SE_alloc_labs;
    }
    plr->SE_alloc_labs += plr->energy_credits
        / (2 * (*CurrentTurn + (plr->AI_power + plr->AI_wealth + 1) * fac->rule_techcost));
    bool defense = false;
    for (int i = 1; i < MaxRegionNum; i++) {
        if (plr->region_total_bases[i]) {
            if (plr->region_base_plan[i] == PLAN_DEFENSE) {
                --plr->SE_alloc_labs;
                defense = true;
                break;
            }
            if (plr->region_base_plan[i] == PLAN_OFFENSE && (plr->AI_wealth || plr->AI_power)) {
                --plr->SE_alloc_labs;
            }
        }
    }
    if (plr->AI_tech && !plr->AI_wealth && !plr->AI_power && !defense) {
        ++plr->SE_alloc_labs;
    }
    if (alloc_val && 10 - plr->SE_alloc_labs - plr->SE_alloc_psych >= alloc_val) {
        int sum = 0;
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == faction_id) {
                sum += Bases[i].unk_total;
                for (int fc = 0; fc <= Fac_ID_Last; fc++) {
                    if (has_fac_built((FacilityId)fc, i)) {
                        sum -= fac_maint(fc, faction_id);
                    }
                }
            }
        }
        int base_mul = plr->base_count * (2 * plr->AI_wealth + 4 - plr->AI_tech) / 4;
        if (sum <= base_mul) {
            if (sum < base_mul / 2) {
                --plr->SE_alloc_labs;
            }
        } else {
            while (10 - plr->SE_alloc_labs - plr->SE_alloc_psych >= alloc_val) {
                ++plr->SE_alloc_labs;
            }
        }
    }
    if (plr->tech_research_id == TECH_TranT) {
        --plr->SE_alloc_labs;
    }
    plr->SE_alloc_labs = max(0, min(plr->SE_alloc_labs,
        min(energy_limit(faction_id), 10 - plr->SE_alloc_psych)));
    while (10 - plr->SE_alloc_labs - plr->SE_alloc_psych > energy_limit(faction_id)) {
        ++plr->SE_alloc_labs;
    }
    int effic_val = clamp(plr->SE_effic_2, 0, 99) + clamp(3 - plr->base_count, 0, 3) + 1;
    plr->SE_alloc_labs = clamp(plr->SE_alloc_labs, 0, 10);
    while (plr->SE_alloc_labs > 10 + effic_val - plr->SE_alloc_psych - plr->SE_alloc_labs) {
        --plr->SE_alloc_labs;
    }
    while (10 - plr->SE_alloc_labs - plr->SE_alloc_psych > effic_val + plr->SE_alloc_labs) {
        ++plr->SE_alloc_labs;
    }
}

