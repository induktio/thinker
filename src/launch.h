#pragma once

#define MOD_VERSION "Thinker Mod"

#ifdef BUILD_DEBUG
    #define MOD_DATE __DATE__ " " __TIME__
    #define DEBUG 1
    #define debug(...) fprintf(debug_log, __VA_ARGS__);
#else
    #define MOD_DATE __DATE__
    #define DEBUG 0
    #define NDEBUG /* Disable assertions */
    #define debug(...) /* Nothing */
    #ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    #endif
#endif

#ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wconversion-null"
    #define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
    #define UNUSED(x) UNUSED_ ## x
#endif

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <windows.h>

#define GameExeFile "terranx.exe"
#define ModDllFile "thinker.dll"

const int HookBufSize = strlen(ModDllFile)+1;

int main(int argc, char* argv[]);


