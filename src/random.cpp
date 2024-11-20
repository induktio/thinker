
#include "random.h"

GameRandom map_rand;
static uint32_t random_seed = 0;


uint32_t pair_hash(uint32_t a, uint32_t b) {
    uint32_t x;
    x = (a ^ (b << 16) ^ (~b >> 16));
    x *= 0x604baa5d;
    x ^= (x >> 16);
    x *= 0x43d6ce97;
    x ^= (x >> 16);
    return x;
}

void random_reseed(uint32_t value) {
    random_seed = value;
}

uint32_t random_state() {
    return random_seed;
}

/*
Returns same values as the game engine function random(0, n) and Random::get(0, n).
*/
int32_t random(int32_t limit) {
    random_seed = 1664525 * random_seed + 1013904223;
    return ((random_seed & 0xffff) * limit) >> 16;
}

void GameRandom::reseed(uint32_t value) {
    state = value;
}

uint32_t GameRandom::get_state() {
    return state;
}

int32_t GameRandom::get(int32_t low, int32_t high) {
    state = 1664525 * state + 1013904223;
    if (low > high) {
        return ((state & 0xffff) * (low - high)) >> 16;
    }
    return ((state & 0xffff) * (high - low)) >> 16;
}

int32_t GameRandom::get(int32_t limit) {
    state = 1664525 * state + 1013904223;
    return ((state & 0xffff) * limit) >> 16;
}

