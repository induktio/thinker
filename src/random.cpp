
#include "random.h"

static uint32_t random_seed = 0;


uint32_t pair_hash(uint32_t a, uint32_t b) {
    uint32_t x;
    x = a ^ b ^ (~b << 16);
    x *= 0x604baa5d;
    x ^= x >> 15;
    x *= 0x43d6ce97;
    x ^= x >> 15;
    return x;
}

void random_reseed(uint32_t value) {
    random_seed = value;
}

uint32_t random_state() {
    return random_seed;
}

/*
Produces same values than the game engine function random(0, n) and Random::get(0, n).
*/
int32_t random(int32_t n) {
    random_seed = 1664525 * random_seed + 1013904223;
    return ((random_seed & 0xffff) * n) >> 16;
}

