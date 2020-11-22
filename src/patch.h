#pragma once

#include "main.h"

const LPVOID AC_IMAGE_BASE = (LPVOID)0x401000;
const SIZE_T AC_IMAGE_LEN = (SIZE_T)0x263000;

extern const char* landmark_params[];

bool patch_setup(Config* cf);

