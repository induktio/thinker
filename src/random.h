#pragma once

#include "main.h"

void random_reseed(uint32_t value);
uint32_t pair_hash(uint32_t a, uint32_t b);
uint32_t random_next(uint32_t value);
uint32_t random_state();
int32_t random(int32_t n);

