
#include "net.h"


void __cdecl net_treaty_on(int a1, int a2, int a3, int wait_diplo) {
    debug_ver("net_treaty_on %d %d %d\n", a1, a2, a3);
    if (*MultiplayerActive) {
        message_data(0x2441, 0, a1, a2, a3, 0);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x441);
        }
    } else {
        treaty_on(a1, a2, a3);
    }
}

void __cdecl net_treaty_off(int a1, int a2, int a3, int wait_diplo) {
    debug_ver("net_treaty_off %d %d %d\n", a1, a2, a3);
    if (*MultiplayerActive) {
        message_data(0x2442, 0, a1, a2, a3, 0);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x442);
        }
    } else {
        treaty_off(a1, a2, a3);
    }
}

void __cdecl net_set_treaty(int a1, int a2, int a3, int a4, int wait_diplo) {
    debug_ver("net_set_treaty %d %d %d %d\n", a1, a2, a3, a4);
    if (*MultiplayerActive) {
        message_data(0x2443, 0, a1, a2, a3, a4);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x443);
        }
    } else {
        set_treaty(a1, a2, a3, a4);
    }
}

void __cdecl net_agenda_off(int a1, int a2, int a3, int wait_diplo) {
    debug_ver("net_agenda_off %d %d %d\n", a1, a2, a3);
    if (*MultiplayerActive) {
        message_data(0x2445, 0, a1, a2, a3, 0);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x445);
        }
    } else {
        agenda_off(a1, a2, a3);
    }
}

void __cdecl net_set_agenda(int a1, int a2, int a3, int a4, int wait_diplo) {
    debug_ver("net_set_agenda %d %d %d %d\n", a1, a2, a3, a4);
    if (*MultiplayerActive) {
        message_data(0x2446, 0, a1, a2, a3, a4);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x446);
        }
    } else {
        set_agenda(a1, a2, a3, a4);
    }
}

void __cdecl net_energy(int faction_id_1, int energy_val_1, int faction_id_2, int energy_val_2, int wait_diplo) {
    debug_ver("net_energy %d %d %d %d\n", faction_id_1, energy_val_1, faction_id_2, energy_val_2);
    if (*MultiplayerActive) {
        message_data(0x2447, 0, faction_id_1, energy_val_1, faction_id_2, energy_val_2);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x447);
        }
    } else {
        if (faction_id_1) {
            Factions[faction_id_1].energy_credits += energy_val_1;
        }
        if (faction_id_2) {
            Factions[faction_id_2].energy_credits += energy_val_2;
        }
        Console_update_data(MapWin, 1);
    }
}

void __cdecl net_loan(int faction_id_1, int faction_id_2, int loan_total, int loan_payment) {
    debug_ver("net_loan %d %d %d %d\n", faction_id_1, faction_id_2, loan_total, loan_payment);
    if (*MultiplayerActive) {
        message_data(0x244E, 0, faction_id_1, faction_id_2, loan_total, loan_payment);
    } else {
        Factions[faction_id_1].loan_balance[faction_id_2] = loan_total;
        Factions[faction_id_1].loan_payment[faction_id_2] = loan_payment;
    }
}

void __cdecl net_maps(int a1, int a2, int wait_diplo) {
    debug_ver("net_maps %d %d\n", a1, a2);
    if (*MultiplayerActive) {
        message_data(0x2448, 0, a1, a2, 0, 0);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x448);
        }
    } else {
        trade_maps(a1, a2);
    }
}

void __cdecl net_tech(int a1, int a2, int a3, int wait_diplo) {
    debug_ver("net_tech %d %d %d\n", a1, a2, a3);
    if (*MultiplayerActive) {
        message_data(0x244B, 0, a1, a2, a3, 0);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x44B);
        }
    } else {
        tech_achieved(a1, a2, a3, 0);
    }
}

void __cdecl net_pact_ends(int a1, int a2, int wait_diplo) {
    debug_ver("net_pact_ends %d %d\n", a1, a2);
    if (*MultiplayerActive) {
        message_data(0x244D, 0, a1, a2, 0, 0);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x44D);
        }
    } else {
        pact_ends(a1, a2);
    }
}

void __cdecl net_cede_base(int a1, int a2, int wait_diplo) {
    debug_ver("net_cede_base %d %d\n", a1, a2);
    if (*MultiplayerActive) {
        message_data(0x2449, 0, a1, a2, 0, 0);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x449);
        }
    } else {
        give_a_base(a1, a2);
    }
}

void __cdecl net_double_cross(int a1, int a2, int a3, int wait_diplo) {
    debug_ver("net_double_cross %d %d %d\n", a1, a2, a3);
    if (*MultiplayerActive) {
        message_data(0x2440, 0, a1, a2, a3, 0);
        if (wait_diplo) {
            NetDaemon_await_diplo(NetState, 0x440);
        }
    } else {
        double_cross(a1, a2, a3);
    }
}

