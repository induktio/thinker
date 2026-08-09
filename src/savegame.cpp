
#include "savegame.h"

// Due to internal implementation differences, any FILE* pointers must
// never be passed directly from modified to original functions or vice versa.

const char* FileExtensionMap = "MP";
const char* FileExtensionSave = "SAV";

int* const dword_6FF69C = (int*)0x6FF69C;
int* const dword_6FF6A0 = (int*)0x6FF6A0;
int* const dword_6FF6A4 = (int*)0x6FF6A4;
int* const dword_6FF6A8 = (int*)0x6FF6A8;
int* const dword_6FF6D4 = (int*)0x6FF6D4;
int* const dword_6FF6F4 = (int*)0x6FF6F4;
int* const dword_93A9B8 = (int*)0x93A9B8;
int* const dword_93A9D8 = (int*)0x93A9D8;
int* const dword_93F798 = (int*)0x93F798;
int* const dword_94B558 = (int*)0x94B558;
int* const dword_9A67DC = (int*)0x9A67DC;
int* const dword_9A67E0 = (int*)0x9A67E0;
int* const dword_939E5C = (int*)0x939E5C;
int* const dword_939E58 = (int*)0x939E58;
int* const dword_93D4F8 = (int*)0x93D4F8;
void* const unk_93D4FC = (void*)0x93D4FC;
void* const unk_93E978 = (void*)0x93E978;
void* const unk_9B2178 = (void*)0x9B2178;
void* const unk_9B208D = (void*)0x9B208D;
char* const unk_945D80 = (char*)0x945D80;
char* const unk_9B2078 = (char*)0x9B2078;
Console* const UnkWin_8EB48C = (Console*)0x8EB48C;
StringStruct* const DiploTextTable = (StringStruct*)0x93A7B8; // [8]
StringStruct (*const DiploMessageTable)[8] = (StringStruct (*)[8])0x737CD8; // [8][8]


static const char* StringStruct_entry_text(StringStruct* cur) {
    assert(!IsBadReadPtr(cur, 1)
        && !IsBadReadPtr(cur->cursor, 1)
        && !IsBadReadPtr(cur->cursor->data, 1)
        && !IsBadReadPtr(cur->cursor->data->text, 1));
    if (!cur || !cur->cursor || !cur->cursor->data || !cur->cursor->data->text) return "";
    return cur->cursor->data->text;
}

static void StringStruct_add_node(StringStruct* cur, int id, char* text) {
    if (!cur || !text) return;
    // add() reads as the source text to copy into StringStructData
    cur->src_text = text;
    // add() also expects these cleared before every call
    cur->field_20 = 0;
    cur->field_24 = 0;
    // memory allocation related, should be set to zero
    cur->flags = 0;
    StringStruct_add(cur, id);
}

static int __cdecl file_feed(void* buf, size_t len, size_t cnt, FILE* fp) {
    debug_ver("file_feed ver=%d wrt=%d enc=%d pos=%X %X %X %X\n",
        *SaveFileVersion, *SaveFileWrite != 0, *SaveFileEncrypt != 0, (int)ftell(fp), (int)buf, len, cnt);
    flushlog();
    if (*SaveFileEncrypt) {
        return *SaveFileWrite ? encrypt_write(buf, len, cnt, fp) : encrypt_read(buf, len, cnt, fp);
    } else {
        return *SaveFileWrite ? fwrite(buf, len, cnt, fp) : fread(buf, len, cnt, fp);
    }
}

static void convert_faction_v10_to_current(const LegacyFactionV10* src, Faction* dst) {
    // Most important conversion method since the non-expansion game uses v10 savegames
    // Includes changes: facility_announced[8] -> [12] and secret_project_intel[5] -> [8]
    memcpy(dst, src, offsetof(LegacyFactionV10, facility_announced));

    memset(dst->facility_announced, 0, sizeof(dst->facility_announced));
    memcpy(dst->facility_announced, src->facility_announced, sizeof(src->facility_announced));

    memcpy(&dst->clean_minerals_modifier, &src->clean_minerals_modifier,
        offsetof(LegacyFactionV10, secret_project_intel)
        - offsetof(LegacyFactionV10, clean_minerals_modifier));

    memset(dst->secret_project_intel, 0, sizeof(dst->secret_project_intel));
    memcpy(dst->secret_project_intel, src->secret_project_intel, sizeof(src->secret_project_intel));

    memcpy(&dst->corner_market_turn, &src->corner_market_turn,
        offsetof(LegacyFactionV10, unk_118) + sizeof(src->unk_118)
        - offsetof(LegacyFactionV10, corner_market_turn));
}

static void convert_faction_v2_to_v10(const LegacyFactionV2* src, LegacyFactionV10* dst) {
    memcpy(dst, &src->common_head, sizeof(src->common_head));
    memset(dst->units_active, 0, sizeof(dst->units_active));
    memset(dst->units_queue,  0, sizeof(dst->units_queue));
    memset(dst->units_lost,   0, sizeof(dst->units_lost));
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 32; ++j) {
            dst->units_active[i * 64 + j] = src->units_active[i * 32 + j];
            dst->units_queue[i * 64 + j]  = src->units_queue[i * 32 + j];
            dst->units_lost[i * 64 + j]   = src->units_lost[i * 32 + j];
        }
    }
    memcpy(&dst->total_combat_units, src->common_tail, sizeof(src->common_tail));
}

static void convert_veh_v2_to_current(const LegacyVehV2* src, VEH* dst) {
    dst->x = src->x;
    dst->y = src->y;
    dst->state = src->state;
    dst->flags = src->flags;
    dst->unit_id = src->unit_id;
    dst->pad_0 = 0; // new field
    dst->faction_id = src->faction_id;
    dst->year_end_lurking = 0; // new field
    dst->damage_taken = src->damage_taken;
    dst->order = src->order;
    dst->waypoint_count = src->waypoint_count;
    dst->patrol_current_point = src->patrol_current_point;
    memcpy(dst->waypoint_x, src->waypoint_x, sizeof(dst->waypoint_x));
    memcpy(dst->waypoint_y, src->waypoint_y, sizeof(dst->waypoint_y));
    dst->morale = src->morale;
    dst->movement_turns = src->movement_turns;
    dst->order_auto_type = src->order_auto_type;
    dst->visibility = src->visibility;
    dst->moves_spent = src->moves_spent;
    dst->rotate_angle = src->rotate_angle;
    dst->iter_count = src->iter_count;
    dst->status_icon = src->status_icon;
    dst->probe_action = src->probe_action;
    dst->probe_sabotage_id = src->probe_sabotage_id;
    dst->home_base_id = src->home_base_id;
    dst->next_veh_id_stack = src->next_veh_id_stack;
    dst->prev_veh_id_stack = src->prev_veh_id_stack;
}

static void convert_base_v2_to_current(const LegacyBaseV2* src, BASE* dst) {
    memcpy(dst, src, offsetof(LegacyBaseV2, facilities_built));
    memset(dst->facilities_built, 0, sizeof(dst->facilities_built));
    memcpy(dst->facilities_built, src->facilities_built, sizeof(src->facilities_built));
    memcpy(&dst->mineral_surplus_final, &src->mineral_surplus_final,
        offsetof(LegacyBaseV2, pad_8) + sizeof(src->pad_8)
        - offsetof(LegacyBaseV2, mineral_surplus_final));
    for (int& item : dst->queue_items) {
        if (item <= -34) {
            item -= 31;
        }
    }
}

int __cdecl game_data(FILE* fp, int write_file) {
    // Fix: properly zero initialize the header. The SMACX version does not do this
    // and writes stack data directly to the savefile while corrupting the version header.
    LegacyGamePrefs prefs = {};
    prefs.SaveFileVersion = 12;
    *SaveFileWrite = write_file;
    *SaveFileEncrypt = 0;
    *SaveFileStatus = 0;
    if (!file_feed(&prefs, 0x31Cu, 1u, fp)) {
        return 1;
    }
    if (prefs.SaveFileVersion >= 11) {
        if (!file_feed(GamePreferences, 0x398u, 1u, fp)) {
            return 1;
        }
    } else {
        memcpy(GamePreferences, &prefs, 0x31Cu);
        memcpy(SecretProjects, prefs.SecretProjects, 0x84u);
        memset(&SecretProjects[FAC_MANIFOLD_HARMONICS - SP_ID_First], 0xFFu, 0x7Cu);
        *GovernorFaction = prefs.GovernorFaction;
        memcpy(ProposalPassCount, prefs.ProposalPassCount, 44u);
        memcpy(ProposalCallTurn, prefs.ProposalCallTurn, 44u);
        memcpy(TechOwners, prefs.TechOwners, 0x59u);
        memcpy(LastSavePath, prefs.LastSavePath, 0x100u);
        *ClimateValueA = prefs.ClimateValueA;
        *ClimateValueB = prefs.ClimateValueB;
        *dword_9A67DC = prefs.dword_9A67DC;
        *ClimateValueC = prefs.ClimateValueC;
        *ClimateFutureChange = prefs.ClimateFutureChange;
        memcpy(dword_9A67E0, prefs.dword_9A67E0, 0x20u);
        memcpy(SunspotDuration, &prefs.SunspotDuration, 0x28u);
    }
    if (*SaveFileVersion < 4) {
        *SaveFileEncrypt = 0;
    } else {
        *SaveFileEncrypt = *GameState & STATE_UNK_10000000;
        if (*SaveFileEncrypt) {
            if (!file_feed(GamePreferences, 0x398u, 1u, fp)) {
                return 1;
            }
            if (!file_feed(unk_93E978, 0xCE0u, 1u, fp)) {
                return 1;
            }
            if (!file_feed(dword_93F798, 4u, 1u, fp)) {
                return 1;
            }
            static_assert(sizeof(CTimeControl) * MaxTimeControlNum == 0xC0u, "");
            char* names[MaxTimeControlNum];
            if (!write_file) {
                for (int i = 0; i < MaxTimeControlNum; ++i) {
                    names[i] = TimeControl[i].name;
                }
            }
            if (!file_feed(TimeControl, 0xC0u, 1u, fp)) {
                return 1;
            }
            if (!write_file) {
                for (int i = 0; i < MaxTimeControlNum; ++i) {
                    TimeControl[i].name = names[i];
                }
            }
        }
    }
    debug_ver("game_data ver=%d wrt=%d enc=%d\n", *SaveFileVersion, *SaveFileWrite != 0, *SaveFileEncrypt != 0);
    int32_t player_id = MapWin->cOwner;
    if (!file_feed(&player_id, 4u, 1u, fp)) {
        return 1;
    }
    if (!*SaveFileWrite) {
        if (player_id >= 0 && player_id < MaxPlayerNum) {
            MapWin->cOwner = player_id;
        } else {
            debug("game_data error player=%d\n", player_id);
            return 1;
        }
        for (int i = 0; i < 64; ++i) {
            if (SecretProjects[i] >= MaxBaseNum) {
                debug("game_data error project=%d base=%d\n", i, SecretProjects[i]);
                return 1;
            }
        }
    }
    if (*SaveFileVersion >= 11) {
        if (!file_feed(Factions, 0x10660u, 1u, fp)) {
            return 1;
        }
    } else {
        static_assert(sizeof(LegacyFactionV10) * 8 == 0x105E0u, "");
        static_assert(sizeof(LegacyFactionV2) * 8 == 0xE5E0u, "");
        std::vector<uint8_t> legacyV10(0x105E0u);
        if (*SaveFileVersion < 3) {
            std::vector<uint8_t> legacyV2(0xE5E0u);
            if (!file_feed(legacyV2.data(), 0xE5E0u, 1u, fp)) {
                return 1;
            }
            const auto* src = (const LegacyFactionV2*)(legacyV2.data());
            auto* dst = (LegacyFactionV10*)(legacyV10.data());
            for (int i = 0; i < 8; ++i) {
                convert_faction_v2_to_v10(&src[i], &dst[i]);
            }
        }
        if (*SaveFileVersion >= 3) {
            if (!file_feed(legacyV10.data(), 0x105E0u, 1u, fp)) {
                return 1;
            }
        }
        const auto* src = (const LegacyFactionV10*)(legacyV10.data());
        for (int i = 0; i < MaxPlayerNum; ++i) {
            convert_faction_v10_to_current(&src[i], &Factions[i]);
        }
    }
    if (!file_feed(MFactions, 0x2CE0u, 1u, fp)) {
        return 1;
    }
    if (!file_feed(FactionRankingsUnk, 0x20u, 1u, fp)) {
        return 1;
    }
    if (!*SaveFileWrite && !*SaveFileMenu && !(*GameState & STATE_GAME_DONE)) {
        if (bit_count(FactionStatus[0] & FactionStatus[1]) > 1) {
            for (int i = 1; i < MaxPlayerNum; ++i) {
                if (is_alive(i) && is_human(i) && Factions[i].unk_102) {
                    *SaveFileStatus = 1;
                    return 1;
                }
            }
        }
    }
    if (*SaveFileWrite ? map_write(fp) : map_read(fp)) {
        return 1;
    }
    if (!file_feed(FactionTurnMight, 0x3E80u, 1u, fp)) {
        return 1;
    }
    if (!file_feed(ReplayEventSize, 4u, 1u, fp)) {
        return 1;
    }
    if (*ReplayEventSize < 0 || *ReplayEventSize > 8192
    || *VehCount < 0 || *VehCount > conf.max_veh_num
    || *BaseCount < 0 || *BaseCount > MaxBaseNum) {
        debug("game_data error bases=%d vehs=%d replay=%d\n", *BaseCount, *VehCount, *ReplayEventSize);
        return 1; // Fix: added bounds checking
    }
    if (*ReplayEventSize) {
        if (!file_feed(ReplayEvents, *ReplayEventSize, 1u, fp)) {
            return 1;
        }
    }
    if (!file_feed(Continents, 0xE00u, 1u, fp)) {
        return 1;
    }
    if (*SaveFileVersion < 3) {
        for (UNIT* block = Units; block < Units + MaxProtoNum; block += 64) {
            if (!file_feed(block, 32 * sizeof(UNIT), 1u, fp)) {
                return 1;
            }
            for (int i = 32; i < MaxProtoFactionNum; ++i) {
                memset(&block[i], 0, sizeof(UNIT));
            }
        }
    } else {
        if (!file_feed(Units, 0x6800u, 1u, fp)) {
            return 1;
        }
    }
    if (*SaveFileVersion < 6) {
        for (int i = 0; i < 8; ++i) {
            clear_bunglist(i);
        }
    } else {
        if (!file_feed(dword_94B558, 0x1000u, 1u, fp)) {
            return 1;
        }
    }
    if (*SaveFileVersion < 11) {
        for (int i = 0; i < MaxProtoNum; ++i) {
            if (Units[i].weapon_id >= 12) {
                Units[i].weapon_id += 3;
            }
        }
    }
    if (*VehCount) {
        if (*SaveFileVersion >= 3) {
            if (!file_feed(Vehs, sizeof(VEH) * *VehCount, 1u, fp)) {
                return 1;
            }
        } else {
            size_t len = sizeof(LegacyVehV2) * *VehCount;
            std::vector<uint8_t> legacy(len);
            const auto* src = (LegacyVehV2*)(legacy.data());
            if (!file_feed(legacy.data(), len, 1u, fp)) {
                return 1;
            }
            for (int i = 0; i < *VehCount; ++i) {
                convert_veh_v2_to_current(&src[i], &Vehs[i]);
            }
        }
    }
    if (*SaveFileVersion <= 7) {
        Units[BSC_MIND_WORMS].icon_offset = 3;
        Units[BSC_ALIEN_ARTIFACT].icon_offset = 2;
    }
    if (*SaveFileVersion < 11) {
        if (*BaseCount) {
            size_t len = sizeof(LegacyBaseV2) * *BaseCount;
            std::vector<uint8_t> legacy(len);
            const auto* src = (LegacyBaseV2*)(legacy.data());
            if (!file_feed(legacy.data(), len, 1u, fp)) {
                return 1;
            }
            for (int i = 0; i < *BaseCount; ++i) {
                convert_base_v2_to_current(&src[i], &Bases[i]);
            }
        }
    } else if (*BaseCount) {
        if (!file_feed(Bases, sizeof(BASE) * *BaseCount, 1u, fp)) {
            return 1;
        }
    }
    if (*SaveFileVersion < 3) {
        for (int i = 0; i < *VehCount; ++i) {
            int16_t& uid = Vehs[i].unit_id;
            uid = (uid % 32) + ((uid / 32) * MaxProtoFactionNum);
        }
        for (int i = 0; i < *BaseCount; ++i) {
            int num = Bases[i].queue_size;
            if (num < 0) {
                continue;
            }
            for (int q = 0; q <= num && q < 10; q++) { // Fix: upper bounds checking
                int& item = Bases[i].queue_items[q];
                if (item >= 0) {
                    item = (item % 32) + ((item / 32) * MaxProtoFactionNum);
                }
            }
        }
    }
    if (*GameState & STATE_IS_SCENARIO) {
        if (!file_feed(unk_9B2178, 0x100u, 1u, fp)) {
            return 1;
        }
    }
    if (*SaveFileVersion < 1) {
        strcpy(unk_9B2078, (const char*)unk_945D80);
    } else {
        if (!file_feed(unk_9B2078, 0x100u, 1u, fp)) {
            return 1;
        }
    }
    if (*SaveFileVersion < 2) {
        clear_scenario();
    } else {
        if (!file_feed(ObjectiveReqVictory, 0x98u, 1u, fp)) {
            return 1;
        }
    }
    if (*SaveFileVersion < 7) {
        clear_monuments();
    } else {
        if (!file_feed(Monuments, 0x27A0u, 1u, fp)) {
            return 1;
        }
    }
    if (*SaveFileVersion < 12) {
        memset(TectonicDetonationCount, 0, 0x20u);
    } else {
        if (!file_feed(TectonicDetonationCount, 0x20u, 1u, fp)) {
            return 1;
        }
    }
    if (!file_feed(&MapWin->fUnitNotViewMode, 4u, 1u, fp)) {
        return 1;
    }
    if (!file_feed(&MapWin->iUnit, 4u, 1u, fp)) {
        return 1;
    }
    if (!file_feed(&MapWin->aiCursorPositionsX[MapWin->iCursorPositionCurrent], 4u, 1u, fp)) {
        return 1;
    }
    if (!file_feed(&MapWin->aiCursorPositionsY[MapWin->iCursorPositionCurrent], 4u, 1u, fp)) {
        return 1;
    }
    if (*SaveFileWrite) {
        MapWin->field_23D20 = 0;
    }
    if (!file_feed(&MapWin->field_23D20, 4u, 1u, fp)) {
        return 1;
    }
    for (int i = 0; i < 8; ++i) {
        if (game_io(MapWinPtr[i], fp)) {
            return 1;
        }
    }
    if (game_io(UnkWin_8EB48C, fp)) {
        return 1;
    }
    if (*GameMoreRules & MRULES_UNK_20) {
        if (!file_feed(DiploStateC, 0x2400u, 1u, fp)) {
            return 1;
        }
        if (!file_feed(DiploStateB, 0x100u, 1u, fp)) {
            return 1;
        }
        if (!file_feed(FactionCombatWin, 0x20u, 1u, fp)) {
            return 1;
        }
        if (!file_feed(FactionCombatLoss, 0x20u, 1u, fp)) {
            return 1;
        }
        char sbuf[StrBufLen];
        size_t spos = 0;
        size_t snum = 0;
        size_t slen = 0;
        for (int i = 1; i < 8; ++i) {
            StringStruct* cur = &DiploTextTable[i];
            if (write_file) {
                snum = cur->count;
                if (!fwrite(&snum, 4u, 1u, fp)) {
                    return 1;
                }
                for (size_t j = 0; j < snum; ++j) {
                    StringStruct_seek_pos(cur, j);
                    spos = StringStruct_current_id(cur);
                    if (!fwrite(&spos, 4u, 1u, fp)) {
                        return 1;
                    }
                    strcpy_n(sbuf, StrBufLen, StringStruct_entry_text(cur));
                    slen = strlen(sbuf);
                    if (!fwrite(&slen, 4u, 1u, fp) || (slen && !fwrite(sbuf, slen, 1u, fp))) {
                        return 1;
                    }
                }
            } else {
                StringStruct_remove_all(cur);
                cur->flags = 0;
                if (!fread(&snum, 4u, 1u, fp)) {
                    return 1;
                }
                for (size_t j = 0; j < snum; ++j) {
                    if (!fread(&spos, 4u, 1u, fp) || !fread(&slen, 4u, 1u, fp)
                    || slen >= StrBufLen || (slen && !fread(sbuf, slen, 1u, fp))) {
                        return 1;
                    }
                    sbuf[slen] = 0;
                    StringStruct_add_node(cur, spos, sbuf);
                }
            }
        }
        for (int j = 1; j <= 7; ++j) {
            for (int i = 1; i < j; ++i) {
                StringStruct* cur = &DiploMessageTable[i][j];
                if (write_file) {
                    snum = cur->count;
                    if (!fwrite(&snum, 4u, 1u, fp)) {
                        return 1;
                    }
                    for (size_t k = 0; k < snum; ++k) {
                        StringStruct_seek_pos(cur, k);
                        spos = StringStruct_current_id(cur);
                        if (!fwrite(&spos, 4u, 1u, fp)) {
                            return 1;
                        }
                        strcpy_n(sbuf, StrBufLen, StringStruct_entry_text(cur));
                        slen = strlen(sbuf);
                        if (!fwrite(&slen, 4u, 1u, fp) || (slen && !fwrite(sbuf, slen, 1u, fp))) {
                            return 1;
                        }
                    }
                } else {
                    StringStruct_remove_all(cur);
                    cur->flags = 0;
                    if (!fread(&snum, 4u, 1u, fp)) {
                        return 1;
                    }
                    for (size_t k = 0; k < snum; ++k) {
                        if (!fread(&spos, 4u, 1u, fp) || !fread(&slen, 4u, 1u, fp)
                        || slen >= StrBufLen || (slen && !fread(sbuf, slen, 1u, fp))) {
                            return 1;
                        }
                        sbuf[slen] = 0;
                        StringStruct_add_node(cur, spos, sbuf);
                    }
                }
            }
        }
        if (!file_feed(dword_6FF69C, 4u, 1u, fp)) return 1;
        if (!file_feed(dword_6FF6A0, 4u, 1u, fp)) return 1;
        if (!file_feed(dword_6FF6A4, 4u, 1u, fp)) return 1;
        if (!file_feed(dword_6FF6A8, 4u, 1u, fp)) return 1;
        if (!file_feed(dword_6FF6D4, 0x20u, 1u, fp)) return 1;
        if (!file_feed(dword_6FF6F4, 0x20u, 1u, fp)) return 1;
        if (!file_feed(CouncilSessionPending, 4u, 1u, fp)) return 1;
        if (!file_feed(CouncilProposal, 0x20u, 1u, fp)) return 1;
        if (!file_feed(CouncilVoteState, 0x20u, 1u, fp)) return 1;
        if (!file_feed(dword_93A9AC, 4u, 1u, fp)) return 1;
        if (!file_feed(dword_93A9B0, 4u, 1u, fp)) return 1;
        if (!file_feed(dword_93A9B4, 4u, 1u, fp)) return 1;
        if (!file_feed(dword_93A9B8, 0x20u, 1u, fp)) return 1;
        if (!file_feed(dword_93A9D8, 0x20u, 1u, fp)) return 1;
    }
    if (write_file) {
        if (!fwrite(dword_939E5C, 4u, 1u, fp)) {
            return 1;
        }
        if (!fwrite(dword_939E58, 4u, 1u, fp)) {
            return 1;
        }
    } else {
        int val1 = 0, val2 = 0;
        if (!fread(&val1, 4u, 1u, fp)) {
            return 1;
        }
        if (!fread(&val2, 4u, 1u, fp)) {
            return 1;
        }
        if (val1 != *dword_939E5C || val2 != *dword_939E58) {
            for (int i = 1; i < 8; ++i) {
                if (MapWinPtr[i]) {
                    MapWinPtr[i]->iDrawToggleA = 0;
                }
            }
        }
    }
    return 0;
}

int __cdecl game_io(Console* state, FILE* fp) {
    if (!file_feed(&state->iDrawToggleB, 4u, 1u, fp)) return 1;
    if (!file_feed(&state->iDrawToggleA, 4u, 1u, fp)) return 1;
    if (!file_feed(&state->iWhatToDrawFlags, 4u, 1u, fp)) return 1;
    if (!file_feed(&state->iZoomFactor, 4u, 1u, fp)) return 1;
    if (!file_feed(&state->iTileX, 4u, 1u, fp)) return 1;
    return file_feed(&state->iTileY, 4u, 1u, fp) == 0;
}

int __cdecl encrypt_write(void* src_ptr, size_t len, size_t cnt, FILE* fp) {
    const uint8_t* src = static_cast<const uint8_t*>(src_ptr);
    size_t total_len = len * cnt;
    size_t alloc_len = total_len + (total_len & 1);
    std::vector<uint8_t> buf(alloc_len);
    uint8_t* dst = buf.data();
    uint8_t v1 = 0x80;
    uint8_t v2 = 0x80;
    size_t half_len = alloc_len / 2;
    for (size_t i = 0; i < half_len; ++i) {
        uint8_t v3 = *src++;
        uint8_t v4 = (2 * i + 1 >= total_len) ? 0 : *src++;
        ++v1;
        --v2;
        if (v3 && v1 != v3) {
            v3 ^= v1;
        }
        if (v4 && v2 != v4) {
            v4 ^= v2;
        }
        *dst++ = v4;
        *dst++ = v3;
    }
    return fwrite(buf.data(), alloc_len, 1u, fp);
}

int __cdecl encrypt_read(void* dst_ptr, size_t len, size_t cnt, FILE* fp) {
    uint8_t* dst = static_cast<uint8_t*>(dst_ptr);
    size_t total_len = len * cnt;
    size_t alloc_len = total_len + (total_len & 1);
    std::vector<uint8_t> buf(alloc_len);
    if (!fread(buf.data(), alloc_len, 1u, fp)) {
        return 0;
    }
    const uint8_t* src = static_cast<const uint8_t*>(buf.data());
    uint8_t v1 = 0x80;
    uint8_t v2 = 0x80;
    size_t half_len = alloc_len / 2;
    for (size_t i = 0; i < half_len; ++i) {
        uint8_t v3 = *src++;
        uint8_t v4 = *src++;
        ++v1;
        --v2;
        if (v4 && v1 != v4) {
            v4 ^= v1;
        }
        if (v3 && v2 != v3) {
            v3 ^= v2;
        }
        *dst++ = v4;
        if (2 * i + 1 < total_len) {
            *dst++ = v3;
        }
    }
    return 1;
}

void __cdecl map_shutdown() {
    if (*MapTiles) {
        mem_free(*MapTiles);
    }
    if (*MapAbstract) {
        mem_free(*MapAbstract);
    }
    *MapTiles = nullptr;
    *MapAbstract = nullptr;
}

/*
Reset the map to a blank state. Original doesn't wipe unk_1 and owner fields.
This is simplified by zeroing all fields first and then setting specific fields.
*/
void __cdecl map_wipe() {
    *MapSeaLevel = 0;
    *MapSeaLevelCouncil = 0;
    *MapLandmarkCount = 0;
    *MapRandomSeed = random(0x7FFF) + 1;
    memset(*MapTiles, 0, *MapAreaTiles * sizeof(MAP));
    for (int i = 0; i < *MapAreaTiles; ++i) {
        (*MapTiles)[i].climate = 0x20;
        (*MapTiles)[i].contour = 20;
        (*MapTiles)[i].val2 = 0xF;
        (*MapTiles)[i].owner = -1;
    }
    reset_state();
    mapdata.clear();
    mapnodes.clear();
}

int __cdecl map_init() {
    snprintf(MapFilePath, StrBufLen, "maps\\%s.%s", label_get(676), FileExtensionMap);
    *MapHalfX = *MapAreaX / 2;
    *MapAreaTiles = *MapAreaY * (*MapAreaX / 2);
    *MapAreaSqRoot = quick_root(*MapAreaY * (*MapAreaX / 2));
    *MapTiles = static_cast<MAP*>(mem_get(sizeof(MAP) * (*MapAreaTiles)));
    if (!*MapTiles) {
        if (*MapAbstract) {
            mem_free(*MapAbstract);
        }
        *MapTiles = nullptr;
        *MapAbstract = nullptr;
        return 1;
    }
    *MapAbstractX = (*MapAreaX + 4) / 5;
    *MapAbstractY = (*MapAreaY + 4) / 5;
    *MapAbstractCount = *MapAbstractY * ((*MapAbstractX + 1) / 2);
    *MapAbstract = static_cast<uint8_t*>(mem_get(*MapAbstractCount));
    if (*MapAbstract) {
        mapwin_terrain_fixup();
        return 0;
    }
    if (*MapTiles) {
        mem_free(*MapTiles);
    }
    if (*MapAbstract) {
        mem_free(*MapAbstract);
    }
    *MapTiles = nullptr;
    *MapAbstract = nullptr;
    return 1;
}

int __cdecl map_write(FILE* fp) {
    if (!fwrite(MapAreaX, 0xAA4u, 1u, fp)) {
        return 1;
    }
    if (!fwrite(*MapTiles, sizeof(MAP) * (*MapAreaTiles), 1u, fp)) {
        return 1;
    }
    return fwrite(*MapAbstract, *MapAbstractCount, 1u, fp) == 0;
}

int __cdecl map_read(FILE* fp) {
    if (*MapTiles) {
        mem_free(*MapTiles);
    }
    if (*MapAbstract) {
        mem_free(*MapAbstract);
    }
    *MapTiles = nullptr;
    *MapAbstract = nullptr;
    if (!fread(MapAreaX, 0xAA4u, 1u, fp)) {
        return 1;
    }
    *MapTiles = nullptr; // NOTE: fread overwrites these pointers
    *MapAbstract = nullptr;
    debug("map_read x=%d y=%d\n", *MapAreaX, *MapAreaY);
    if (*MapAreaX <= 0 || *MapAreaX > 0x1000
    || *MapAreaY <= 0 || *MapAreaY > 0x1000) {
        assert(0);
        return 1;
    }
    if (map_init()) {
        return 1;
    }
    if (!fread(*MapTiles, sizeof(MAP) * (*MapAreaTiles), 1u, fp)) {
        return 1;
    }
    if (!fread(*MapAbstract, *MapAbstractCount, 1u, fp)) {
        return 1;
    }
    fixup_landmarks();
    return 0;
}

int __cdecl map_data(FILE* fp, int save_flag, int extended_flag) {
    debug("map_data %d %d\n", save_flag, extended_flag);
    GamePrefs header_buf = {};
    uint8_t plr_buf[0x10660u];
    uint8_t fac_buf[0x2CE0u];
    uint8_t rank_buf[0x20u];
    const auto* header = (const LegacyGamePrefs*)(&header_buf);
    *SaveFileWrite = save_flag;
    *SaveFileEncrypt = 0;
    if (extended_flag) {
        if (save_flag) {
            assert(0);
            return 1;
        }
        if (!file_feed(&header_buf, sizeof(LegacyGamePrefs), 1u, fp)) {
            return 1;
        }
        int32_t version = header->SaveFileVersion;
        // Fix: original game was reading save file versions and offsets in incorrectly
        // resulting in crashes when any SAV file was opened in the map editor mode
        if (version >= 11) {
            if (!file_feed(&header_buf, sizeof(GamePrefs), 1u, fp)) {
                return 1;
            }
            version = header_buf.SaveFileVersion;
        }
        if (header_buf.GameState & STATE_UNK_10000000 || header_buf.GameMoreRules & MRULES_UNK_20) {
            return 1;
        }
        if (!file_feed(&save_flag, 4u, 1u, fp)) {
            return 1;
        }
        if (!(version >= 0 && version <= 12)) {
            return 1;
        }
        size_t plr_size = 0x10660u;
        if (version < 11) {
            plr_size = (version >= 3 ? 0x105E0u : 0xE5E0u);
        }
        if (!file_feed(plr_buf, plr_size, 1u, fp)) {
            return 1;
        }
        if (!file_feed(fac_buf, sizeof(fac_buf), 1u, fp)) {
            return 1;
        }
        if (!file_feed(rank_buf, sizeof(rank_buf), 1u, fp)) {
            return 1;
        }
    }
    return (*SaveFileWrite ? map_write(fp) : map_read(fp)) != 0;
}

int __cdecl header_check(char* dst, FILE* fp) {
    int cnt = 0;
    char c = static_cast<char>(fgetc(fp));
    *dst = c;
    char* out = dst + 1;
    while (c) {
        if (++cnt >= 256) {
            dst[255] = '\0';
            break;
        }
        c = static_cast<char>(fgetc(fp));
        *out++ = c;
    }
    return fgetc(fp);
}

int __cdecl header_write(const char* src, FILE* fp) {
    char c;
    do {
        c = *src++;
        fputc(c, fp);
    } while (c);
    return fputc(0x1A, fp);
}

/*
Original file headers use TERRAN (savegames) and TERRANMAP (maps without game state).
When unit count exceeds previous limit 2048, this extended version will use a modified
file header for savegames to prevent these files from being opened in the original game.
*/
int __cdecl mod_save_daemon(const char* filename) {
    if (*PbemActive) {
        ++*dword_93A9B4;
    }
    uint32_t seed = GetTickCount();
    if ((*GameRules & RULES_IRONMAN) && !*MultiplayerActive) {
        game_srand(seed);
    }
    *SaveFileVersion = 12;

    if (*GameRules & RULES_IRONMAN) {
        *GameState |= STATE_UNK_10000000;
        *dword_93F798 = 0;
    }
    if (*PbemActive) {
        *GameMoreRules |= MRULES_UNK_20;
        *GameState |= STATE_UNK_10000000;
        *dword_93F798 = 0;
    }
    if (*MultiplayerActive) {
        *GameState |= STATE_UNK_10000000;
        *dword_93F798 = *dword_93D4F8;
        memcpy(unk_93E978, unk_93D4FC, 0xCE0u);
    }
    char path[StrBufLen];
    if (strchr(filename, '.')) {
        snprintf(path, sizeof(path), "%s", filename);
    } else {
        snprintf(path, sizeof(path), "%s.%s", filename, FileExtensionSave);
    }
    debug("save_daemon %s\n", path);
    FILE* file = env_open(path, "wb");
    if (!file) {
        return 1;
    }
    int status = 2;
    if (*VehCount > MaxVehNum) {
        header_write("TERRAE", file);
    } else {
        header_write("TERRAN", file);
    }
    int32_t version = 0x56;
    if (fwrite(&version, 4u, 1u, file)) {
        status = 3;
        if (!game_data(file, 1)) {
            if (fwrite(&seed, sizeof(uint32_t), 1u, file)) {
                status = 0;
            }
        }
    }
    fclose(file);
    if (status) {
        remove(path);
    }
    return status;
}

int __cdecl mod_load_daemon(const char* filename, int flag) {
    debug("load_daemon flag=%d %s\n", flag, filename);
    const char* header_1 = "TERRAN";
    const char* header_2 = conf.modify_unit_limit ? "TERRAE" : "TERRAN";
    FILE* fp = env_open(filename, "rb");
    if (!fp) {
        return SAVE_LOAD_NONE;
    }
    reset_state();
    char buf[StrBufLen];
    header_check(buf, fp);
    if (strcmp(buf, header_1) != 0 && strcmp(buf, header_2) != 0) {
        fclose(fp);
        return SAVE_LOAD_NOT;
    }
    int32_t version = 0;
    if (!fread(&version, 4u, 1u, fp) || version < 0x56) {
        fclose(fp);
        return SAVE_LOAD_OLD;
    }
    for (int i = 2; i < 8; ++i) {
        Console* win = MapWinPtr[i];
        if (win && win->iDrawToggleA) {
            win->iDrawToggleA = 0;
            MapWin_close(win);
        }
    }
    MapWin->field_23BE4 = 0;
    MapWin->field_23BE8 = 0;
    if (game_data(fp, 0)) {
        fclose(fp);
        if (!*SaveFileStatus) {
            return SAVE_LOAD_ERROR;
        }
        return SAVE_LOAD_SECURITY;
    }
    uint32_t seed = 0;
    if ((*GameRules & RULES_IRONMAN) && fread(&seed, 4u, 1u, fp)) {
        game_srand(seed);
    }
    if (!*MultiplayerActive) {
        for (int i = 2; i < 8; ++i) {
            Console* ptr = MapWinPtr[i];
            if (ptr) {
                ptr->iDrawToggleA = 0;
            }
        }
    }
    *ComputeBaseID = -1;
    Path_init(Paths);
    if (flag) {
        mod_world_linearize_contours();
    }
    memcpy(AltNatural, ElevDetail, sizeof(ElevDetail));
    for (int fc = 1; fc < MaxPlayerNum; ++fc) {
        Faction* plr = &Factions[fc];
        plr->player_flags &= ~PFLAG_MAP_REVEALED;
        if (plr->satellites_nutrient || plr->satellites_mineral
        || plr->satellites_energy || plr->satellites_ODP) {
            plr->player_flags |= PFLAG_MAP_REVEALED;
        }
        for (int i = 0; i < MaxTechnologyNum; ++i) {
            if ((Tech[i].flags & TFLAG_REVEALS_MAP) && has_tech(i, fc)) {
                plr->player_flags |= PFLAG_MAP_REVEALED;
            }
        }
    }
    for (int fc = 1; fc < MaxPlayerNum; ++fc) {
        bool active = false;
        for (int i = 0; i < *BaseCount; ++i) {
            if (Bases[i].faction_id == fc) {
                active = true;
                break;
            }
        }
        if (!active) {
            for (int i = 0; i < *VehCount; ++i) {
                if (Vehs[i].faction_id == fc && Vehs[i].plan() == PLAN_COLONY) {
                    active = true;
                    break;
                }
            }
        }
        set_alive(fc, active);
    }
    // Fix: commented out since the original remove does not have any effect for opened files
    // if (*GameRules & RULES_IRONMAN) {
    //     remove(filename);
    // }
    int cursor_x = MapWin->aiCursorPositionsX[MapWin->iCursorPositionCurrent];
    int cursor_y = MapWin->aiCursorPositionsY[MapWin->iCursorPositionCurrent];
    for (int i = 0; i < 32; ++i) {
        MapWin->aiCursorPositionsX[i] = cursor_x;
        MapWin->aiCursorPositionsY[i] = cursor_y;
    }
    rebuild_vehicle_bits();
    rebuild_base_bits();
    // Fix: removed leftover original debug code that activated debug mode and scenario editor
    // when any save with the string "multi" in the filename was opened
    *ControlTurnMove = MapWin->cOwner;
    if (*MultiplayerActive) {
        *PbemActive = 0;
    } else if ((*GameMoreRules & MRULES_UNK_20) || bit_count(FactionStatus[0]) > 1) {
        *PbemActive = 1;
        check_tamper();
    } else {
        *PbemActive = 0;
    }
    *GameDrawState |= 4;
    fclose(fp);
    return SAVE_LOAD_VALID;
}

int __cdecl mod_save_map_daemon(const char* filename) {
    int status = 1;
    char path[StrBufLen];

    if (strchr(filename, '.')) {
        snprintf(path, sizeof(path), "%s", filename);
    } else {
        snprintf(path, sizeof(path), "%s.%s", filename, FileExtensionMap);
    }
    FILE* fp = env_open(path, "wb");
    if (fp) {
        status = 2;
        header_write("TERRANMAP", fp);
        int32_t version = 0x5;
        if (fwrite(&version, 4u, 1u, fp)) {
            status = 3;
            if (!map_data(fp, 1, 0)) {
                status = 0;
            }
        }
        fclose(fp);
    }
    if (status) {
        remove(path);
    }
    return status;
}

int __cdecl mod_load_map_daemon(const char* filename) {
    int is_save = 0;
    char* ext = strrchr(filename, '.'); // Fix: select the last dot from the filename
    if (ext && !_stricmp(ext + 1, FileExtensionSave)) {
        is_save = 1;
    }
    FILE* fp = env_open(filename, "rb");
    if (!fp) {
        return 1;
    }
    reset_state();
    int status = 2;
    char buf[StrBufLen];
    header_check(buf, fp);
    const char* header_1 = is_save ? "TERRAN" : "TERRANMAP";
    const char* header_2 = is_save ? "TERRAE" : "TERRANMAP";
    if (!strcmp(buf, header_1) || !strcmp(buf, header_2)) {
        status = 3;
        int32_t version = 0;
        if (fread(&version, 4u, 1u, fp)) {
            int min_ver = is_save ? 0x56 : 0x5;
            if (version >= min_ver) {
                for (int i = 1; i < 8; ++i) {
                    Console* win = MapWinPtr[i];
                    if (win && win->iDrawToggleA) {
                        win->iDrawToggleA = 0;
                        MapWin_close(win);
                    }
                }
                MapWin->field_23BE4 = 0;
                status = 4;
                if (!map_data(fp, 0, is_save)) {
                    *BaseCount = 0;
                    *VehCount = 0;
                    MapWin->fUnitNotViewMode = 0;
                    MapWin->iUnit = -1;
                    for (int i = 0; i < *MapAreaTiles; ++i) {
                        MAP* sq = &(*MapTiles)[i];
                        sq->visibility = 0;
                        sq->items &= ~(BIT_VEH_IN_TILE | BIT_BASE_IN_TILE);
                    }
                    status = 0;
                    *ComputeBaseID = -1;
                    memcpy(AltNatural, ElevDetail, sizeof(ElevDetail));
                    Path_init(Paths);
                }
            }
        }
    }
    fclose(fp);
    return status;
}

void __cdecl mod_auto_save() {
    if ((!*PbemActive || *MultiplayerActive)
    && (!(*GameRules & RULES_IRONMAN) || *GameState & STATE_SCENARIO_EDITOR)) {
        if (conf.autosave_interval > 0 && !(*CurrentTurn % conf.autosave_interval)) {
            char buf[256];
            snprintf(buf, sizeof(buf), "saves/auto/Autosave_%d.sav", game_year(*CurrentTurn));
            mod_save_daemon(buf);
        }
    }
}

