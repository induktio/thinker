#pragma once

#include "main.h"

#define AC_IMAGE_BASE (LPVOID)0x401000
#define AC_IMAGE_LEN  (SIZE_T)0x263000

extern const char* lm_params[];

bool patch_setup(Config* cf);

