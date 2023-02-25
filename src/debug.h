#pragma once

#include "main.h"

int __cdecl mod_except_handler3(EXCEPTION_RECORD* rec, PVOID* frame, CONTEXT* ctx);
void __cdecl log_say(const char* a1, int a2, int a3, int a4);
void __cdecl log_say2(const char* a1, const char* a2, int a3, int a4, int a5);
void __cdecl log_say_hex(const char* a1, int a2, int a3, int a4);
void __cdecl log_say_hex2(const char* a1, const char* a2, int a3, int a4, int a5);

