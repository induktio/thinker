#include "basewin.h"

static int hurry_minimal_cost = 0;
static int base_zoom_factor = -14;


void __cdecl action_staple(int base_id) {
    const int player_id = *CurrentPlayerFaction;
    BASE* const base = &Bases[base_id];
    const char* image = MFactions[base->faction_id].is_alien() ? "ALdrone_sm.pcx" : "drone_sm.pcx";
    bool failed = true;
    if (conf.nerve_staple_turns > 0) {
        ++base->nerve_staple_count;
        failed = (base->nerve_staple_count > 8) || (base->nerve_staple_count > 2 && !(game_rand() & 1));
    }
    if (failed) {
        if (base->faction_id == player_id) {
            parse_says(0, base->name, -1, -1);
            popp(ScriptFile, "NERVESTAPLEFAIL", 0, image, 0);
        }
    } else {
        base->state_flags &= ~BSTATE_DRONE_RIOTS_ACTIVE;
        base->nerve_staple_turns_left += conf.nerve_staple_turns;
        draw_tile(base->x, base->y, 2);
        if (base->faction_id == player_id) {
            parse_says(0, base->name, -1, -1);
            popp(ScriptFile, "NERVESTAPLEDONE", 0, image, 0);
        } else {
            parse_says(0, base->name, -1, -1);
            *PluralDefault = 0;
            *GenderDefault = MFactions[base->faction_id].is_leader_female;
            parse_says(1, MFactions[base->faction_id].title_leader, -1, -1);
            parse_says(2, MFactions[base->faction_id].name_leader, -1, -1);
            NetMsg_pop(NetMsg, "NERVESTAPLETHEM", 5000, 0, 0);
        }
    }
    if (conf.nerve_staple_turns > 0) {
        atrocity(base->faction_id, base->faction_id_former,
            base->faction_id_former != base->faction_id, 1);
    }
}

void __cdecl action_sat_attack(int faction_id, int faction_id_tgt, int target_id, int base_id) {
    int* const dword_7AE778 = (int*)0x7AE778;
    int* const dword_7D392C = (int*)0x7D392C;
    const int player_id = *CurrentPlayerFaction;
    assert(target_id >= 0 && target_id <= 4);
    if (!(Factions[faction_id].diplo_status[faction_id_tgt] & DIPLO_PACT)) {
        double_cross(faction_id, faction_id_tgt, -1);
    }
    if (faction_id == player_id || faction_id_tgt == player_id) {
        *PluralDefault = 0;
        *GenderDefault = MFactions[faction_id].is_leader_female;
        parse_says(0, MFactions[faction_id].title_leader, -1, -1);
        parse_says(1, MFactions[faction_id].name_leader, -1, -1);
        *PluralDefault = MFactions[faction_id].is_noun_plural;
        *GenderDefault = MFactions[faction_id].noun_gender;
        parse_says(2, MFactions[faction_id].noun_faction, -1, -1);
        *PluralDefault = 0;
        *GenderDefault = MFactions[faction_id_tgt].is_leader_female;
        parse_says(3, MFactions[faction_id_tgt].title_leader, -1, -1);
        parse_says(4, MFactions[faction_id_tgt].name_leader, -1, -1);
        *PluralDefault = MFactions[faction_id_tgt].is_noun_plural;
        *GenderDefault = MFactions[faction_id_tgt].noun_gender;
        parse_says(5, MFactions[faction_id_tgt].noun_faction, -1, -1);
        if (target_id >= 0 && target_id < 4) {
            parse_says(6, Facility[FAC_SKY_HYDRO_LAB + target_id].name, -1, -1);
        } else {
            parse_says(6, Facility[FAC_GEOSYNC_SURVEY_POD].name, -1, -1);
        }
        parse_says(7, MFactions[faction_id_tgt].adj_name_faction, -1, -1);
    }
    if (game_rand() % 100 >= OrbitalAttackChance) {
        --Factions[faction_id].satellites_ODP;
        if (faction_id == player_id) {
            popp(ScriptFile, "WEMUFFEDIT", 0, "satbat_sm.pcx", 0);
        } else if (faction_id_tgt == player_id) {
            popp(ScriptFile, "THEYMUFFEDIT", 0, "satbat_sm.pcx", 0);
        }
    } else {
        switch (target_id) {
        case 0: --Factions[faction_id_tgt].satellites_nutrient; break;
        case 1: --Factions[faction_id_tgt].satellites_mineral; break;
        case 2: --Factions[faction_id_tgt].satellites_energy; break;
        case 3: --Factions[faction_id_tgt].satellites_ODP; break;
        case 4: set_fac(FAC_GEOSYNC_SURVEY_POD, base_id, 0); break;
        }
        ++Factions[faction_id].ODP_deployed;
        if (faction_id == player_id) {
            popp(ScriptFile, "WEKILLEDIT", 0, "satbat_sm.pcx", 0);
        } else if (faction_id_tgt == player_id) {
            popp(ScriptFile, "THEYKILLEDIT", 0, "satbat_sm.pcx", 0);
        }
    }
    if (ReportIf->field_8 == 6 && dword_7AE778[*dword_7D392C] == 5) {
        ReportIf_on_redraw(ReportIf);
    }
}

void BaseWin_support_zoom(bool zoom_in) {
    base_zoom_factor = clamp(base_zoom_factor + (zoom_in ? 2 : -2), -14, 0);
    GraphicWin_redraw(BaseWin);
}

int __thiscall BaseWin_hurry_popup_start(
Popup* This, const char* UNUSED(filename), const char* UNUSED(label), int a4, char* a5, int a6, GraphicWin* a7) {
    BASE* base = *CurrentBase;
    Faction* f = &Factions[base->faction_id];
    int item_cost = mineral_cost(*CurrentBaseID, base->queue_items[0]);
    int minerals = item_cost - base->minerals_accumulated - max(0, base->mineral_surplus);
    int credits = max(0, f->energy_credits - f->hurry_cost_total);
    int cost = hurry_cost(*CurrentBaseID, base->queue_items[0], minerals);
    hurry_minimal_cost = min(credits, cost);
    if (item_cost <= base->minerals_accumulated) {
        ParseNumTable[0] = 0;
    }
    ParseNumTable[1] = cost;
    ParseNumTable[2] = credits;
    return Popup_start(This, "modmenu", "HURRY", a4, a5, a6, a7);
}

int __cdecl BaseWin_hurry_ask_number(const char* label, int value, fp_none fn) {
    ParseNumTable[0] = value;
    return pop_ask_number_4(ScriptFile, label, hurry_minimal_cost, fn);
}

/*
Fix issue where hurry production flag will not be set after
completely hurrying the current production "Spend $NUM0 energy credits."
*/
void __thiscall BaseWin_hurry_unlock_base(AlphaNet* This, int base_id) {
    if (base_id >= 0) {
        Bases[base_id].state_flags |= BSTATE_HURRY_PRODUCTION;
    }
    return NetDaemon_unlock_base(This, base_id);
}

void __thiscall BaseWin_draw_support(BaseWindow* This) {
    RECT& rc = This->oRender.rResWindow;
    Buffer_set_clip(&This->oCanvas, &rc);
    GraphicWin_fill_2(This, &rc, 0);

    MapWin_init_2((Console*)&This->oRender, 2, 0);
    This->oRender.iZoomFactor = base_zoom_factor;
    This->oRender.iWhatToDrawFlags = MAPWIN_SUPPORT_VIEW|MAPWIN_DRAW_BONUS_RES|\
        MAPWIN_DRAW_RIVERS|MAPWIN_DRAW_IMPROVEMENTS|MAPWIN_DRAW_TRANSLUCENT;

    This->oRender.iTileX = (*CurrentBase)->x;
    This->oRender.iTileY = (*CurrentBase)->y;
    GraphicWin_redraw(&This->oRender.oBufWin);

    Buffer_copy_4(&This->oRender.oBufWin.oCanvas, &This->oCanvas,
        0, 0, rc.left + 11, rc.top + 31, rc.right, rc.bottom);
    GraphicWin_soft_update_3(This, &rc);
    Buffer_set_clip(&This->oCanvas, &This->oCanvas.stRect[0]);
}

void __thiscall BaseWin_draw_misc_eco_damage(Buffer* This, char* buf, int x, int y, int len) {
    BASE* base = *CurrentBase;
    Faction* f = &Factions[base->faction_id];
    if (!conf.render_base_info || !strlen(label_eco_damage)) {
        Buffer_write_l_3(This, buf, x, y, len);
    } else {
        int clean_mins = conf.clean_minerals + f->clean_minerals_modifier
            + clamp(f->satellites_mineral, 0, (int)base->pop_size);
        int damage = terraform_eco_damage(*CurrentBaseID);
        int mins = base->mineral_intake_2 + damage/8;
        int pct;
        if (base->eco_damage > 0) {
            pct = 100 + base->eco_damage;
        } else {
            pct = (clean_mins > 0 ? 100 * clamp(mins, 0, clean_mins) / clean_mins : 0);
        }
        snprintf(buf, StrBufLen, label_eco_damage, pct);
        Buffer_write_l_3(This, buf, x, y, strlen(buf));
    }
}

void __thiscall BaseWin_draw_farm_set_font(Buffer* This, Font* a2, Font* a3, Font* a4, Font* a5) {
    char buf[StrBufLen] = {};
    // Base resource window coordinates including button row
    RECT* rc = &BaseWin->oRender.rResWindow;
    int x1 = rc->left;
    int y1 = rc->top;
    int x2 = rc->right;
    int y2 = rc->bottom;
    int N = 0;
    int M = 0;
    int E = 0;
    int SE = 0;
    Buffer_set_font(This, a2, a3, a4, a5);

    if (*CurrentBaseID < 0 || x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0) {
        assert(0);
    } else if (conf.render_base_info) {
        if (satellite_bonus(*CurrentBaseID, &N, &M, &E)) {
            snprintf(buf, StrBufLen, label_sat_nutrient, N);
            Buffer_set_text_color(This, ColorNutrient, 0, 1, 1);
            Buffer_write_l_3(This, buf, x1 + 5, y2 - 36 - 28, LineBufLen);

            snprintf(buf, StrBufLen, label_sat_mineral, M);
            Buffer_set_text_color(This, ColorMineral, 0, 1, 1);
            Buffer_write_l_3(This, buf, x1 + 5, y2 - 36 - 14, LineBufLen);

            snprintf(buf, StrBufLen, label_sat_energy, E);
            Buffer_set_text_color(This, ColorEnergy, 0, 1, 1);
            Buffer_write_l_3(This, buf, x1 + 5, y2 - 36     , LineBufLen);
        }
        if ((SE = stockpile_energy_active(*CurrentBaseID)) > 0) {
            snprintf(buf, StrBufLen, label_stockpile_energy, SE);
            Buffer_set_text_color(This, ColorEnergy, 0, 1, 1);
            Buffer_write_right_l_5(This, buf, x2 - 5, y2 - 36, LineBufLen);
        }
    }
}

void __thiscall BaseWin_draw_energy_set_text_color(Buffer* This, int a2, int a3, int a4, int a5) {
    BASE* base = &Bases[*CurrentBaseID];
    char buf[StrBufLen] = {};
    if (conf.render_base_info && *CurrentBaseID >= 0) {
        int workers = base->pop_size - base->talent_total - base->drone_total - base->specialist_total;
        int color;

        if (base_maybe_riot(*CurrentBaseID)) {
            color = ColorRed;
        } else if (base->golden_age()) {
            color = ColorEnergy;
        } else {
            color = ColorIntakeSurplus;
        }
        Buffer_set_text_color(This, color, a3, a4, a5);
        snprintf(buf, StrBufLen, label_pop_size,
            base->talent_total, workers, base->drone_total, base->specialist_total);
        if (DEBUG) {
            strncat(buf, conf.base_psych ? " / B" : " / A", 32);
        }
        Buffer_write_right_l_5(This, buf, 690, 423 - 42, LineBufLen);

        if (base_pop_boom(*CurrentBaseID) && base_unused_space(*CurrentBaseID) > 0) {
            Buffer_set_text_color(This, ColorNutrient, a3, a4, a5);
            snprintf(buf, StrBufLen, "%s", label_pop_boom);
            Buffer_write_right_l_5(This, buf, 690, 423 - 21, LineBufLen);
        }

        if (base->nerve_staple_turns_left > 0) {
            snprintf(buf, StrBufLen, label_nerve_staple, base->nerve_staple_turns_left);
            Buffer_set_text_color(This, ColorEnergy, a3, a4, a5);
            Buffer_write_right_l_5(This, buf, 690, 423, LineBufLen);
        }
    }
    Buffer_set_text_color(This, a2, a3, a4, a5);
}

void __cdecl BaseWin_draw_psych_strcat(char* buffer, char* source) {
    BASE* base = &Bases[*CurrentBaseID];
    if (conf.render_base_info && *CurrentBaseID >= 0) {
        if (base->nerve_staple_turns_left > 0
        || has_fac_built(FAC_PUNISHMENT_SPHERE, *CurrentBaseID)) {
            if (!strcmp(source, label_get(971))) { // Stapled Base
                strncat(buffer, label_get(322), StrBufLen); // Unmodified
                return;
            }
            if (!strcmp(source, label_get(327))) { // Secret Projects
                strncat(buffer, label_get(971), StrBufLen); // Stapled Base
                return;
            }
        }
        int turns = base->assimilation_turns_left;
        if (turns > 0 && !strcmp(source, label_get(970))) { // Captured Base
            snprintf(buffer, StrBufLen, label_captured_base, turns);
            return;
        }
    }
    strncat(buffer, source, StrBufLen);
}

void __cdecl mod_base_draw(Buffer* buffer, int base_id, int x, int y, int zoom, int opts) {
    int color = -1;
    int width = 1;
    BASE* base = &Bases[base_id];
    base_draw(buffer, base_id, x, y, zoom, opts);

    if (conf.render_base_info && zoom >= -8) {
        if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
            color = ColorWhite;
            width = 2;
        }
        if (has_fac_built(FAC_GEOSYNC_SURVEY_POD, base_id)
        || has_fac_built(FAC_FLECHETTE_DEFENSE_SYS, base_id)) {
            color = ColorCyan;
        }
        if (base->faction_id == MapWin->cOwner && base->golden_age()) {
            color = ColorEnergy;
        }
        if (base->faction_id == MapWin->cOwner && base_maybe_riot(base_id)) {
            color = ColorRed;
        }
        if (color < 0) {
            return;
        }
        // Game engine uses this way to determine the population label width
        int w = Font_width(*MapLabelFont, (base->pop_size >= 10 ? "88" : "8")) + 5;
        int h = (*MapLabelFont)->iHeight + 4;

        for (int i = 1; i <= width; i++) {
            RECT rr = {x-i, y-i, x+w+i, y+h+i};
            Buffer_box_2(buffer, &rr, color, color);
        }
    }
}

int __cdecl BaseWin_staple_popp(
const char* filename, const char* label, int a3, const char* imagefile, fp_none fn) {
    BASE* base = *CurrentBase;
    if (base && base->assimilation_turns_left
    && base->faction_id_former != MapWin->cOwner && is_alive(base->faction_id_former)) {
        return popp("modmenu", "NERVESTAPLE2", a3, imagefile, fn);
    }
    return popp(filename, label, a3, imagefile, fn);
}

/*
Refresh base window workers properly after nerve staple is done.
*/
void __cdecl BaseWin_action_staple(int base_id) {
    if (can_staple(base_id)) {
        set_base(base_id);
        action_staple(base_id);
        base_compute(1);
        BaseWin_on_redraw(BaseWin);
    }
}

/*
Separate case where nerve stapling is done from another popup.
*/
void __cdecl popb_action_staple(int base_id) {
    if (can_staple(base_id)) {
        action_staple(base_id);
    }
}

int __thiscall BaseWin_click_staple(BaseWindow* This) {
    int base_id = This->oRender.base_id;
    if (can_staple(base_id)) {
        return BaseWin_nerve_staple(This);
    }
    return 0;
}

int __thiscall BaseWin_gov_options(BaseWindow* This, int flag) {
    int base_id = *CurrentBaseID;
    if (base_id < 0 || base_id != This->oRender.base_id) {
        assert(0);
        return 1;
    }
    BASE* base = &Bases[base_id];
    int worked_tiles = base->worked_tiles;
    set_base(base_id);
    base_compute(1);
    if (*MultiplayerActive && !*ControlTurnC
    && worked_tiles != base->worked_tiles
    && !NetDaemon_lock_base(NetState, *CurrentBaseID, 0, -1, -1)) {
        NetDaemon_unlock_base(NetState, *CurrentBaseID);
    }
    if (base->faction_id != MapWin->cOwner && !(*GameState & STATE_OMNISCIENT_VIEW)) {
        return 1;
    }
    *DialogChoices = 0;
    for (auto& p : BaseGovOptions) {
        if (base->governor_flags & p[1]) {
            *DialogChoices |= p[0];
        }
    }
    parse_says(0, base->name, -1, -1);
    if (NetDaemon_lock_base(NetState, base_id, 0, -1, -1)) {
        return 1;
    }
    if (X_pop_9("modmenu", "GOVOPTIONS", -1, 0, 65, 0) >= 0) {
        base->governor_flags &= (GOV_PRIORITY_CONQUER|GOV_PRIORITY_BUILD|GOV_PRIORITY_DISCOVER|GOV_PRIORITY_EXPLORE);
        for (auto& p : BaseGovOptions) {
            if (*DialogChoices & p[0]) {
                base->governor_flags |= p[1];
            }
        }
        Factions[base->faction_id].base_governor_adv = base->governor_flags;
        if (!flag) {
            if (base->governor_flags & GOV_ACTIVE) {
                if (base->governor_flags & GOV_MANAGE_PRODUCTION) {
                    base->state_flags &= ~BSTATE_UNK_80000000;
                    base->queue_size = 0;
                    mod_base_reset(base_id, 1);
                }
                if (base->governor_flags & GOV_MANAGE_CITIZENS) {
                    base->worked_tiles = 0;
                    base->specialist_total = 0;
                    base->specialist_adjust = 0;
                    base_compute(1);
                    // base_doctors() removed as obsolete
                }
            }
            if (!(base->governor_flags & GOV_ACTIVE) || !(base->governor_flags & GOV_MANAGE_PRODUCTION)) {
                draw_radius(base->x, base->y, 2, 2);
            }
        }
        NetDaemon_unlock_base(NetState, base_id);
        GraphicWin_redraw(BaseWin);
        GraphicWin_redraw(MainWin);
        return 0;
    } else {
        NetDaemon_unlock_base(NetState, base_id);
        return 1;
    }
}


