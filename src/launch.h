// MIT License
//
// Copyright (c) Thinker Mod authors
// https://github.com/induktio/thinker/
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#define MOD_VERSION "Thinker Mod"

#ifdef BUILD_DEBUG
#define MOD_DATE __DATE__ " " __TIME__
#define DEBUG 1
#define debug(...) fprintf(debug_log, __VA_ARGS__);
#else
#define MOD_DATE __DATE__
#define DEBUG 0
#ifndef NDEBUG
#define NDEBUG /* Disable assertions */
#endif
#define debug(...) /* Nothing */
#endif

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <windows.h>
#include <winerror.h>
#include <strsafe.h>

#ifdef __GNUC__
#pragma GCC poison strcat_s strcpy_s
#pragma GCC diagnostic ignored "-Wconversion-null"
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_ ## x
#endif

#define GameExeFile "terranx.exe"
#define ModDllFile "thinker.dll"

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740
#endif

const int StrBufSize = 4096;

const int HookBufSize = strlen(ModDllFile)+1;

int main(int argc, char* argv[]);


