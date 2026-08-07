#pragma once

#include "main.h"

// Always available settings for random maps started from the main menu
// Select only those settings that are not set in Special Scenario Rules
const uint32_t GAME_RULES_MASK = 0x7808FFFF;
const uint32_t GAME_MRULES_MASK = 0xFFFFFFF0;

const char* resource_icon(int res_type, bool ocean, bool add);
bool un_charter();
bool global_trade_pact();
bool victory_done();
bool full_game_turn();
bool voice_of_planet();
bool valid_player(int faction_id);
bool valid_triad(int triad);
char* label_get(size_t index);
void __cdecl clear();
char* __cdecl says(const char* buf);
char* __cdecl say_num(int value);
char* __cdecl say_year(char* buf);
char* __cdecl parse_set(int faction_id);
int __cdecl parse_num(size_t index, int value);
int __cdecl parse_says(size_t index, const char* src, int gender, int plural);
int __cdecl game_year(int turn);
int __cdecl in_box(int x, int y, RECT* rc);
void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask);

void show_rules_menu();
void init_world_config();
void init_save_game(int faction_id);
void __cdecl control_game();
int __cdecl custom_planet(int use_images, int use_defaults);
int __cdecl size_of_planet(int setup_mode);
int __cdecl map_menu(int flag);
int __cdecl top_menu(int flag);
int __cdecl game_init(int tgl_text, int tgl_rules);
int __cdecl game_reload(int tgl_init, int tgl_rules);
void __cdecl game_close(int);
void __cdecl alien_start();
void __cdecl scenario_setup();
int __cdecl config_game(int flag);
void __cdecl setup_game(int flag);
int __cdecl generators(int faction_id, int* pop_size_req);
int __cdecl end_of_game(int flag);
int __cdecl replay_base(int event, int x, int y, int faction_id);
void __cdecl mod_name_base(int faction_id, char* name, bool save_offset, bool water);
int __cdecl load_music_strcmpi(const char* active, const char* label);

template<typename T, typename... Args>
int net_show(T format, Args... vals) {
    char buf[StrBufLen];
    snprintf(buf, StrBufLen, format, vals...);
    parse_says(0, buf, -1, -1);
    return NetMsg_pop(NetMsg, "GENERIC", 5000, 0, 0);
}

