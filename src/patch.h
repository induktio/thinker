#pragma once

#include "main.h"

const LPVOID AC_IMAGE_BASE = (LPVOID)0x401000;
const SIZE_T AC_IMAGE_LEN = (SIZE_T)0x263000;
const LPVOID AC_IMPORT_BASE = (LPVOID)0x669000;
const SIZE_T AC_IMPORT_LEN = (SIZE_T)0x3A0;

const LPVOID PeekMessageImport = (LPVOID)0x669358;
const LPVOID RegisterClassImport = (LPVOID)0x66929C;
const LPVOID GetSystemMetricsImport = (LPVOID)0x669334;

extern const char* landmark_params[];

void write_jump(int addr, int func);
void write_call(int addr, int func);
void write_offset(int addr, const void* ofs);
void write_bytes(int addr, const byte* old_bytes, const byte* new_bytes, int len);
void remove_call(int addr);
bool patch_setup(Config* cf);

