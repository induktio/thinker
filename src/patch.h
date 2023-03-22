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

void write_jump(int addr, int func);
void short_jump(int addr);
void write_call(int addr, int func);
void write_offset(int addr, const void* offset);
void write_bytes(int addr, const byte* old_bytes, const byte* new_bytes, int len);
void remove_call(int addr);
bool patch_setup(Config* cf);

