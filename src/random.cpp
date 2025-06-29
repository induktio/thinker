
#include "random.h"

uint32_t game_rand_state() {
    fp_none getptd = (fp_none)0x6491C3;
    return ((uint32_t*)getptd())[5];
}

int32_t game_randv(int32_t value) {
    // RNG state only advances when the value is within bounds
    return (value > 1 ? game_rand() % value : 0);
}

uint32_t pair_hash(uint32_t a, uint32_t b) {
    uint32_t x;
    x = (a ^ (b << 16) ^ (~b >> 16));
    x *= 0x603a32a7;
    x ^= (x >> 16);
    x *= 0x5a522677;
    x ^= (x >> 16);
    return x;
}

GameRandom map_rand;
static uint32_t random_seed = 0;

void random_reseed(uint32_t value) {
    random_seed = value;
}

uint32_t random_state() {
    return random_seed;
}

/*
Returns same values as the game engine function game_random(0, n) and Random::get(0, n).
*/
int32_t random(int32_t limit) {
    random_seed = 1664525 * random_seed + 1013904223;
    return ((random_seed & 0xffff) * limit) >> 16;
}

int32_t random_get(int32_t low, int32_t high) {
    random_seed = 1664525 * random_seed + 1013904223;
    if (low > high) {
        std::swap(low, high);
    }
    return low + (((random_seed & 0xffff) * (high - low)) >> 16);
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
        std::swap(low, high);
    }
    return low + (((state & 0xffff) * (high - low)) >> 16);
}

int32_t GameRandom::get(int32_t limit) {
    state = 1664525 * state + 1013904223;
    return ((state & 0xffff) * limit) >> 16;
}

#ifdef BUILD_DEBUG
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"

/* HalfSipHash reference C implementation */

#ifndef cROUNDS
#define cROUNDS 2
#endif
#ifndef dROUNDS
#define dROUNDS 4
#endif

#define ROTL(x, b) (uint32_t)(((x) << (b)) | ((x) >> (32 - (b))))

#define U32TO8_LE(p, v)                                                        \
    (p)[0] = (uint8_t)((v));                                                   \
    (p)[1] = (uint8_t)((v) >> 8);                                              \
    (p)[2] = (uint8_t)((v) >> 16);                                             \
    (p)[3] = (uint8_t)((v) >> 24);

#define U8TO32_LE(p)                                                           \
    (((uint32_t)((p)[0])) | ((uint32_t)((p)[1]) << 8) |                        \
     ((uint32_t)((p)[2]) << 16) | ((uint32_t)((p)[3]) << 24))

#define SIPROUND                                                               \
    do {                                                                       \
        v0 += v1;                                                              \
        v1 = ROTL(v1, 5);                                                      \
        v1 ^= v0;                                                              \
        v0 = ROTL(v0, 16);                                                     \
        v2 += v3;                                                              \
        v3 = ROTL(v3, 8);                                                      \
        v3 ^= v2;                                                              \
        v0 += v3;                                                              \
        v3 = ROTL(v3, 7);                                                      \
        v3 ^= v0;                                                              \
        v2 += v1;                                                              \
        v1 = ROTL(v1, 13);                                                     \
        v1 ^= v2;                                                              \
        v2 = ROTL(v2, 16);                                                     \
    } while (0)

/*
    Computes a SipHash value
    *in: pointer to input data (read-only)
    inlen: input data length in bytes (any size_t value)
    *k: pointer to the key data (read-only), must be 8 bytes
    *out: pointer to output data (write-only), outlen bytes must be allocated
    outlen: length of the output in bytes, must be 4 or 8
*/
static int halfsiphash(const void* in, const size_t inlen, const void* k, uint8_t* out, const size_t outlen) {
    const unsigned char* ni = (const unsigned char*)in;
    const unsigned char* kk = (const unsigned char*)k;
    assert((outlen == 4) || (outlen == 8));
    uint32_t v0 = 0;
    uint32_t v1 = 0;
    uint32_t v2 = UINT32_C(0x6c796765);
    uint32_t v3 = UINT32_C(0x74656462);
    uint32_t k0 = U8TO32_LE(kk);
    uint32_t k1 = U8TO32_LE(kk + 4);
    uint32_t m;
    int i;
    const unsigned char* end = ni + inlen - (inlen % sizeof(uint32_t));
    const int left = inlen & 3;
    uint32_t b = ((uint32_t)inlen) << 24;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    if (outlen == 8)
        v1 ^= 0xee;

    for (; ni != end; ni += 4) {
        m = U8TO32_LE(ni);
        v3 ^= m;

        for (i = 0; i < cROUNDS; ++i)
            SIPROUND;

        v0 ^= m;
    }
    switch (left) {
    case 3: b |= ((uint32_t)ni[2]) << 16; /* FALLTHRU */
    case 2: b |= ((uint32_t)ni[1]) << 8; /* FALLTHRU */
    case 1: b |= ((uint32_t)ni[0]); break;
    case 0: break;
    }
    v3 ^= b;

    for (i = 0; i < cROUNDS; ++i)
        SIPROUND;

    v0 ^= b;

    if (outlen == 8)
        v2 ^= 0xee;
    else
        v2 ^= 0xff;

    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v1 ^ v3;
    U32TO8_LE(out, b);

    if (outlen == 4)
        return 0;

    v1 ^= 0xdd;

    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v1 ^ v3;
    U32TO8_LE(out + 4, b);
    return 0;
}

uint64_t hash64(const void* input, size_t len, uint64_t seed) {
    uint64_t h = 0;
    halfsiphash((uint8_t*)input, len, &seed, (uint8_t*)&h, 8);
    return h;
}

uint32_t hash32(const void* input, size_t len, uint64_t seed) {
    uint32_t h = 0;
    halfsiphash((uint8_t*)input, len, &seed, (uint8_t*)&h, 4);
    return h;
}

#pragma GCC diagnostic pop
#endif

