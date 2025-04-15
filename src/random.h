#pragma once

#include "main.h"

uint32_t game_rand_state();
int32_t game_randv(int32_t value);
void random_reseed(uint32_t value);
uint32_t pair_hash(uint32_t a, uint32_t b);
uint32_t random_state();
int32_t random(int32_t limit);

class GameRandom {
    private:
    uint32_t state = 0;
    public:
    void reseed(uint32_t value);
    uint32_t get_state();
    int32_t get(int32_t limit);
    int32_t get(int32_t low, int32_t high);
};
extern GameRandom map_rand;

template <class T, class C>
const T& pick_random(const std::set<T,C>& s) {
    auto it = std::begin(s);
    std::advance(it, random(s.size()));
    return *it;
}

template <class T>
const T& pick_random(const std::set<T>& s) {
    auto it = std::begin(s);
    std::advance(it, random(s.size()));
    return *it;
}

#ifdef BUILD_DEBUG
uint64_t hash64(const void* input, size_t len, uint64_t seed);
uint32_t hash32(const void* input, size_t len, uint32_t seed);
#endif

