
#include "monument.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

int* const dword_7FD634 = (int*)0x7FD634;

void __cdecl monument(int is_new_event) {
    if (!MFactions[MapWin->cOwner].is_alien()
    && !*MultiplayerActive
    && (!is_new_event
    || (!(*GameMorePreferences & MPREF_AV_MONUMENTS_DISABLED)
    && !(*GameState & STATE_IS_SCENARIO)
    && (!(*GameState & STATE_GAME_DONE) || !(*GameState & STATE_FINAL_SCORE_DONE))))) {
        int achieved_count = 0;
        for (int i = 0; i < MON_KILLED_FACTION_1; i++) {
            if (Monuments[MapWin->cOwner].events[i].achieved) {
                achieved_count++;
            }
        }
        *dword_7FD634 = achieved_count;
        if (achieved_count) {
            if (!MonuWin_init(MonuWin)) {
                MonuWin->is_detail_open = is_new_event != 0;
                (*FWin_set_modal((DWORD*)MonuWin->vtable + 57))(MonuWin, 0, 0, 0);
                GraphicWin_close(MonuWin);
                do_all_draws();
            }
        }
    }
}

void __cdecl clear_monuments() {
    for (int i = 0; i < MaxPlayerNum; i++) {
        for (auto& event : Monuments[i].events) {
            event.achieved = 0;
        }
        Monuments[i].latest_event_id = -1;
    }
}

void __cdecl mon_colony_founded(int faction_id, char* base_name) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_COLONY_FOUNDED];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_COLONY_FOUNDED].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_COLONY_FOUNDED].achieved
                && Monuments[i].events[MON_COLONY_FOUNDED].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.faction_id = first_faction_id;
        event.achieved = 1;
        event.year = year;
        strncpy(Monuments[faction_id].base_name, base_name, sizeof(Monuments[faction_id].base_name));
        Monuments[faction_id].latest_event_id = MON_COLONY_FOUNDED;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_tech_discovered(int faction_id, int tech_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_TECH_DISCOVERED];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_TECH_DISCOVERED].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_TECH_DISCOVERED].achieved
                && Monuments[i].events[MON_TECH_DISCOVERED].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.achieved = 1;
        event.year = year;
        event.faction_id = first_faction_id;
        event.param2 = tech_id;
        Monuments[faction_id].latest_event_id = MON_TECH_DISCOVERED;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_secrets_of_tech(int faction_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_SECRETS_OF_TECH];
    if (!MFactions[faction_id].is_alien()) {
        if (event.achieved) {
            if (event.param2 == -1) {
                event.param2 = year;
            } else if (event.param1 == -1) {
                event.param1 = year;
            }
        } else {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_SECRETS_OF_TECH].achieved) {
                    event.prev_count++;
                }
            }
            int first_faction_id = faction_id;
            if (event.prev_count) {
                event.prev_count = 0;
                for (int i = 0; i < MaxPlayerNum; i++) {
                    if (i != faction_id && Monuments[i].events[MON_SECRETS_OF_TECH].achieved
                    && Monuments[i].events[MON_SECRETS_OF_TECH].prev_count) {
                        first_faction_id = i;
                    }
                }
            } else {
                event.prev_count = 1;
            }
            event.faction_id = first_faction_id;
            event.year = year;
            event.achieved = 1;
            event.param2 = -1;
            event.param1 = -1;
        }
        Monuments[faction_id].latest_event_id = MON_SECRETS_OF_TECH;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_prototype_built(int faction_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_PROTOTYPE_BUILT];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_PROTOTYPE_BUILT].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_PROTOTYPE_BUILT].achieved
                && Monuments[i].events[MON_PROTOTYPE_BUILT].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.achieved = 1;
        event.faction_id = first_faction_id;
        event.year = year;
        Monuments[faction_id].latest_event_id = MON_PROTOTYPE_BUILT;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_facility_built(int faction_id, char* facility_name) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_FACILITY_BUILT];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_FACILITY_BUILT].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_FACILITY_BUILT].achieved
                && Monuments[i].events[MON_FACILITY_BUILT].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.faction_id = first_faction_id;
        event.achieved = 1;
        event.year = year;
        strncpy(Monuments[faction_id].facility_name, facility_name, sizeof(Monuments[faction_id].facility_name));
        Monuments[faction_id].latest_event_id = MON_FACILITY_BUILT;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_secret_project(int faction_id, int project_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_SECRET_PROJECT];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_SECRET_PROJECT].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_SECRET_PROJECT].achieved
                && Monuments[i].events[MON_SECRET_PROJECT].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.achieved = 1;
        event.year = year;
        event.faction_id = first_faction_id;
        event.param2 = project_id;
        Monuments[faction_id].latest_event_id = MON_SECRET_PROJECT;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_enemy_destroyed(int faction_id, int enemy_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_ENEMY_DESTROYED];
    if (!MFactions[faction_id].is_alien() && !event.achieved && enemy_id) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_ENEMY_DESTROYED].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            first_faction_id = enemy_id;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_ENEMY_DESTROYED].achieved
                && Monuments[i].events[MON_ENEMY_DESTROYED].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
            first_faction_id = faction_id;
        }
        event.achieved = 1;
        event.year = year;
        event.faction_id = first_faction_id;
        event.param2 = enemy_id;
        Monuments[faction_id].latest_event_id = MON_ENEMY_DESTROYED;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_conquer_base(int faction_id, char* base_name) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_CONQUER_BASE];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_CONQUER_BASE].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_CONQUER_BASE].achieved
                && Monuments[i].events[MON_CONQUER_BASE].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.faction_id = first_faction_id;
        event.achieved = 1;
        event.year = year;
        strncpy(Monuments[faction_id].enemy_base_name, base_name, sizeof(Monuments[faction_id].enemy_base_name));
        Monuments[faction_id].latest_event_id = MON_CONQUER_BASE;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_naval_unit_built(int faction_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_NAVAL_UNIT_BUILT];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_NAVAL_UNIT_BUILT].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_NAVAL_UNIT_BUILT].achieved
                && Monuments[i].events[MON_NAVAL_UNIT_BUILT].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.achieved = 1;
        event.faction_id = first_faction_id;
        event.year = year;
        Monuments[faction_id].latest_event_id = MON_NAVAL_UNIT_BUILT;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_air_unit_built(int faction_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_AIR_UNIT_BUILT];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_AIR_UNIT_BUILT].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_AIR_UNIT_BUILT].achieved
                && Monuments[i].events[MON_AIR_UNIT_BUILT].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.achieved = 1;
        event.faction_id = first_faction_id;
        event.year = year;
        Monuments[faction_id].latest_event_id = MON_AIR_UNIT_BUILT;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_native_life_bred(int faction_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_NATIVE_LIFE_BRED];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_NATIVE_LIFE_BRED].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_NATIVE_LIFE_BRED].achieved
                && Monuments[i].events[MON_NATIVE_LIFE_BRED].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.achieved = 1;
        event.faction_id = first_faction_id;
        event.year = year;
        Monuments[faction_id].latest_event_id = MON_NATIVE_LIFE_BRED;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_first_in_space(int faction_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_FIRST_IN_SPACE];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_FIRST_IN_SPACE].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_FIRST_IN_SPACE].achieved
                && Monuments[i].events[MON_FIRST_IN_SPACE].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.achieved = 1;
        event.faction_id = first_faction_id;
        event.year = year;
        Monuments[faction_id].latest_event_id = MON_FIRST_IN_SPACE;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_built_preserve(int faction_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_BUILT_PRESERVE];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.prev_count = 0;
        for (int i = 0; i < MaxPlayerNum; i++) {
            if (i != faction_id && Monuments[i].events[MON_BUILT_PRESERVE].achieved) {
                event.prev_count++;
            }
        }
        int first_faction_id = faction_id;
        if (event.prev_count) {
            event.prev_count = 0;
            for (int i = 0; i < MaxPlayerNum; i++) {
                if (i != faction_id && Monuments[i].events[MON_BUILT_PRESERVE].achieved
                && Monuments[i].events[MON_BUILT_PRESERVE].prev_count) {
                    first_faction_id = i;
                }
            }
        } else {
            event.prev_count = 1;
        }
        event.achieved = 1;
        event.faction_id = first_faction_id;
        event.year = year;
        Monuments[faction_id].latest_event_id = MON_BUILT_PRESERVE;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_winning_unify(int faction_id, int winner_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_WINNING_UNIFY];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.year = year;
        event.faction_id = faction_id;
        event.prev_count = 1;
        event.achieved = 1;
        event.param2 = winner_id;
        Monuments[faction_id].latest_event_id = MON_WINNING_UNIFY;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_winning_trans(int faction_id, int winner_id) {
    int year = game_year(*CurrentTurn);
    auto& event = Monuments[faction_id].events[MON_WINNING_TRANS];
    if (!MFactions[faction_id].is_alien() && !event.achieved) {
        event.year = year;
        event.faction_id = faction_id;
        event.prev_count = 1;
        event.achieved = 1;
        event.param2 = winner_id;
        Monuments[faction_id].latest_event_id = MON_WINNING_TRANS;
        if (faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

void __cdecl mon_killed_faction(int faction_id, int killed_id) {
    int event_id = -1;
    int year = game_year(*CurrentTurn);
    for (int i = MON_KILLED_FACTION_1; i <= MON_KILLED_FACTION_6; i++) {
        if (!Monuments[faction_id].events[i].achieved && event_id == -1) {
            event_id = i;
        }
    }
    if (event_id != -1) {
        auto& event = Monuments[faction_id].events[event_id];
        event.year = year;
        event.param2 = killed_id;
        event.prev_count = 1;
        event.achieved = 1;
        event.faction_id = faction_id;
        Monuments[faction_id].latest_event_id = event_id;
        if (!MFactions[faction_id].is_alien() && faction_id == MapWin->cOwner) {
            monument(1);
        }
    }
}

#pragma GCC diagnostic pop
