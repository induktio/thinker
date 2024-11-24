#pragma once

#include "main.h"

const LPVOID AC_IMAGE_BASE = (LPVOID)0x401000;
const SIZE_T AC_IMAGE_LEN = (SIZE_T)0x263000;
const LPVOID AC_IMPORT_BASE = (LPVOID)0x669000;
const SIZE_T AC_IMPORT_LEN = (SIZE_T)0x3A0;

const int PeekMessageImport = 0x669358;
const int RegisterClassImport = 0x66929C;
const int GetSystemMetricsImport = 0x669334;
const int GetPrivateProfileStringAImport = 0x669108;

const int8_t NetVersion = 11; // Network multiplayer

void write_jump(int32_t addr, int32_t func);
void short_jump(int32_t addr);
void long_jump(int32_t addr);
void write_call(int32_t addr, int32_t func);
void write_offset(int32_t addr, const void* offset);
void write_bytes(int32_t addr, const byte* old_bytes, const byte* new_bytes, int32_t len);
void write_byte(int32_t addr, byte old_byte, byte new_byte);
void write_word(int32_t addr, int32_t old_word, int32_t new_word);
void remove_call(int32_t addr);
bool patch_setup(Config* cf);

