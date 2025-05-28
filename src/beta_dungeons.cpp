#include <iostream>
#include <chrono>
#include <string>
#include <fstream>
#include <algorithm>
#include <memory.h>

#include "rng.h"
#include "beta_dungeons.hpp"

#define OFFSET 12
#define Random uint64_t
#define RANDOM_MULTIPLIER 0x5DEECE66DULL
#define RANDOM_ADDEND 0xBULL
#define RANDOM_MASK ((1ULL << 48u) - 1)
#define RANDOM_SCALE 0x1.0p-53
#define get_random(seed) ((Random)((seed ^ RANDOM_MULTIPLIER) & RANDOM_MASK))

static inline void advance4(Random *random) {
    *random = (*random * 0x32EB772C5F11LLU + 0x2D3873C4CD04LLU) & RANDOM_MASK;
}

static inline void advance6(Random *random) {
    *random = (*random * 0x45D73749A7F9LLU + 0x17617168255ELLU) & RANDOM_MASK;
}

static inline int32_t random_next(Random *random, int bits) {
    *random = (*random * RANDOM_MULTIPLIER + RANDOM_ADDEND) & RANDOM_MASK;
    return (int32_t) (*random >> (48u - bits));
}

static inline int32_t random_next_int(Random *random, const uint16_t bound) {
    int32_t r = random_next(random, 31);
    const uint16_t m = bound - 1u;
    if ((bound & m) == 0) {
        r = (int32_t) ((bound * (uint64_t) r) >> 31u);
    } else {
        for (int32_t u = r;
             u - (r = u % bound) + m < 0;
             u = random_next(random, 31));
    }
    return r;
}

static inline double next_double(Random *random) {
    return (double) ((((uint64_t) ((uint32_t) random_next(random, 26)) << 27u)) + random_next(random, 27)) * RANDOM_SCALE;
}

inline uint64_t random_next_long(Random *random) {
    return (((uint64_t) random_next(random, 32)) << 32u) + (int32_t) random_next(random, 32);
}

struct PermutationTable {
    double xo;
    double yo;
    double zo; // this actually never used in fixed noise aka 2d noise;)
    uint8_t permutations[256];
};

static inline void initOctaves(PermutationTable octaves[], Random *random, int nbOctaves) {
    for (int i = 0; i < nbOctaves; ++i) {
        octaves[i].xo = next_double(random) * 256.0;
        octaves[i].yo = next_double(random) * 256.0;
        octaves[i].zo = next_double(random) * 256.0;
        uint8_t *permutations = octaves[i].permutations;
        uint8_t j = 0;
        do {
            permutations[j] = j;
        } while (j++ != 255);
        uint8_t index = 0;
        do {
            uint32_t randomIndex = (uint32_t) random_next_int(random, 256u - index) + index;
            if (randomIndex != index) {
                // swap
                permutations[index] ^= permutations[randomIndex];
                permutations[randomIndex] ^= permutations[index];
                permutations[index] ^= permutations[randomIndex];
            }
        } while (index++ != 255);
    }
}

struct BiomeNoises {
    PermutationTable temperatureOctaves[4];
    PermutationTable humidityOctaves[4];
    PermutationTable precipitationOctaves[2];
};


enum Biomes {
    Rainforest,
    Swampland,
    Seasonal_forest,
    Forest,
    Savanna,
    Shrubland,
    Taiga,
    Desert,
    Plains,
    IceDesert,
    Tundra,
};

Biomes biomesTable[4096]={Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Desert, Desert, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Desert, Desert, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Savanna, Savanna, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Plains, Plains, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Shrubland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Seasonal_forest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Seasonal_forest, Rainforest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Rainforest, Rainforest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Rainforest, Rainforest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Rainforest, Rainforest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Rainforest, Rainforest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Forest, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Rainforest, Rainforest, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Tundra, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Taiga, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Swampland, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Forest, Rainforest, Rainforest,};

struct BiomeResult {
    double *temperature;
    double *humidity;
    Biomes *biomes;
};


#define F2 0.3660254037844386
#define G2 0.21132486540518713

struct TerrainNoises {
    PermutationTable minLimit[16];
    PermutationTable maxLimit[16];
    PermutationTable mainLimit[8];
    //PermutationTable shoresBottomComposition[4];
    PermutationTable surfaceElevation[4];
    PermutationTable scale[10];
    PermutationTable depth[16];
};

enum blocks {
    AIR,
    STONE,
    GRASS,
    DIRT,
    BEDROCK,
    MOVING_WATER,
    SAND,
    GRAVEL,
    ICE,
    LAVA
};

struct TerrainResult {
    BiomeResult *biomeResult;
    uint8_t *chunkHeights;
};


const char *biomesNames[] = {"Rainforest", "Swampland", "Seasonal Forest", "Forest", "Savanna", "Shrubland", "Taiga", "Desert", "Plains", "IceDesert", "Tundra"};

int grad2[12][2] = {{1,  1,},
                    {-1, 1,},
                    {1,  -1,},
                    {-1, -1,},
                    {1,  0,},
                    {-1, 0,},
                    {1,  0,},
                    {-1, 0,},
                    {0,  1,},
                    {0,  -1,},
                    {0,  1,},
                    {0,  -1,}};


static inline void simplexNoise(double **buffer, double chunkX, double chunkZ, int x, int z, double offsetX, double offsetZ, double octaveFactor, PermutationTable permutationTable) {
    int k = 0;
    uint8_t *permutations = permutationTable.permutations;
    for (int X = 0; X < x; X++) {
        double XCoords = (chunkX + (double) X) * offsetX + permutationTable.xo;
        for (int Z = 0; Z < z; Z++) {
            double ZCoords = (chunkZ + (double) Z) * offsetZ + permutationTable.yo;
            // Skew the input space to determine which simplex cell we're in
            double hairyFactor = (XCoords + ZCoords) * F2;
            auto tempX = static_cast<int32_t>(XCoords + hairyFactor);
            auto tempZ = static_cast<int32_t>(ZCoords + hairyFactor);
            int32_t xHairy = (XCoords + hairyFactor < tempX) ? (tempX - 1) : (tempX);
            int32_t zHairy = (ZCoords + hairyFactor < tempZ) ? (tempZ - 1) : (tempZ);
            double d11 = (double) (xHairy + zHairy) * G2;
            double X0 = (double) xHairy - d11; // Unskew the cell origin back to (x,y) space
            double Y0 = (double) zHairy - d11;
            double x0 = XCoords - X0; // The x,y distances from the cell origin
            double y0 = ZCoords - Y0;
            // For the 2D case, the simplex shape is an equilateral triangle.
            // Determine which simplex we are in.
            int offsetSecondCornerX, offsetSecondCornerZ; // Offsets for second (middle) corner of simplex in (i,j) coords

            if (x0 > y0) {  // lower triangle, XY order: (0,0)->(1,0)->(1,1)
                offsetSecondCornerX = 1;
                offsetSecondCornerZ = 0;
            } else { // upper triangle, YX order: (0,0)->(0,1)->(1,1)
                offsetSecondCornerX = 0;
                offsetSecondCornerZ = 1;
            }

            double x1 = (x0 - (double) offsetSecondCornerX) + G2; // Offsets for middle corner in (x,y) unskewed coords
            double y1 = (y0 - (double) offsetSecondCornerZ) + G2;
            double x2 = (x0 - 1.0) + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
            double y2 = (y0 - 1.0) + 2.0 * G2;

            // Work out the hashed gradient indices of the three simplex corners
            uint8_t ii = (uint8_t) xHairy & 0xffu;
            uint8_t jj = (uint8_t) zHairy & 0xffu;
            uint8_t gi0 = permutations[(uint8_t) (ii + permutations[jj]) & 0xffu] % 12u;
            uint8_t gi1 = permutations[(uint8_t) (ii + offsetSecondCornerX + permutations[(uint8_t) (jj + offsetSecondCornerZ) & 0xffu]) & 0xffu] % 12u;
            uint8_t gi2 = permutations[(uint8_t) (ii + 1 + permutations[(uint8_t) (jj + 1) & 0xffu]) & 0xffu] % 12u;

            // Calculate the contribution from the three corners
            double t0 = 0.5 - x0 * x0 - y0 * y0;
            double n0;
            if (t0 < 0.0) {
                n0 = 0.0;
            } else {
                t0 *= t0;
                n0 = t0 * t0 * ((double) grad2[gi0][0] * x0 + (double) grad2[gi0][1] * y0);  // (x,y) of grad2 used for 2D gradient
            }
            double t1 = 0.5 - x1 * x1 - y1 * y1;
            double n1;
            if (t1 < 0.0) {
                n1 = 0.0;
            } else {
                t1 *= t1;
                n1 = t1 * t1 * ((double) grad2[gi1][0] * x1 + (double) grad2[gi1][1] * y1);
            }
            double t2 = 0.5 - x2 * x2 - y2 * y2;
            double n2;
            if (t2 < 0.0) {
                n2 = 0.0;
            } else {
                t2 *= t2;
                n2 = t2 * t2 * ((double) grad2[gi2][0] * x2 + (double) grad2[gi2][1] * y2);
            }
            // Add contributions from each corner to get the final noise value.
            // The result is scaled to return values in the interval [-1,1].
            (*buffer)[k] = (*buffer)[k] + 70.0 * (n0 + n1 + n2) * octaveFactor;
            k++;

        }

    }
}


static inline void getFixedNoise(double *buffer, double chunkX, double chunkZ, int sizeX, int sizeZ, double offsetX, double offsetZ, double ampFactor, PermutationTable *permutationTable, uint8_t octaves) {
    offsetX /= 1.5;
    offsetZ /= 1.5;
    // cache should be created by the caller
    memset(buffer, 0, sizeof(double) * sizeX * sizeZ);

    double octaveDiminution = 1.0;
    double octaveAmplification = 1.0;
    for (uint8_t j = 0; j < octaves; ++j) {
        simplexNoise(&buffer, chunkX, chunkZ, sizeX, sizeZ, offsetX * octaveAmplification, offsetZ * octaveAmplification, 0.55000000000000004 / octaveDiminution, permutationTable[j]);
        octaveAmplification *= ampFactor;
        octaveDiminution *= 0.5;
    }

}


static inline BiomeNoises *initBiomeGen(uint64_t worldSeed) {
    auto *pBiomeNoises = new BiomeNoises;
    Random worldRandom;
    PermutationTable *octaves;
    worldRandom = get_random(worldSeed * 9871L);
    octaves = pBiomeNoises->temperatureOctaves;
    initOctaves(octaves, &worldRandom, 4);
    worldRandom = get_random(worldSeed * 39811L);
    octaves = pBiomeNoises->humidityOctaves;
    initOctaves(octaves, &worldRandom, 4);
    worldRandom = get_random(worldSeed * 543321L);
    octaves = pBiomeNoises->precipitationOctaves;
    initOctaves(octaves, &worldRandom, 2);
    return pBiomeNoises;
}


static inline BiomeResult *getBiomes(int posX, int posZ, int sizeX, int sizeZ, BiomeNoises *biomesOctaves) {
    auto *biomes = new Biomes[16 * 16];
    auto *biomeResult = new BiomeResult;
    auto *temperature = new double[sizeX * sizeZ];
    auto *humidity = new double[sizeX * sizeZ];
    auto *precipitation = new double[sizeX * sizeZ];
    getFixedNoise(temperature, posX, posZ, sizeX, sizeZ, 0.02500000037252903, 0.02500000037252903, 0.25, (*biomesOctaves).temperatureOctaves, 4);
    getFixedNoise(humidity, posX, posZ, sizeX, sizeZ, 0.05000000074505806, 0.05000000074505806, 0.33333333333333331, (*biomesOctaves).humidityOctaves, 4);
    getFixedNoise(precipitation, posX, posZ, sizeX, sizeZ, 0.25, 0.25, 0.58823529411764708, (*biomesOctaves).precipitationOctaves, 2);
    int index = 0;
    for (int X = 0; X < sizeX; X++) {
        for (int Z = 0; Z < sizeZ; Z++) {
            double preci = precipitation[index] * 1.1000000000000001 + 0.5;
            double temp = (temperature[index] * 0.14999999999999999 + 0.69999999999999996) * (1.0 - 0.01) + preci * 0.01;
            temp = 1.0 - (1.0 - temp) * (1.0 - temp);
            if (temp < 0.0) {
                temp = 0.0;
            }
            if (temp > 1.0) {
                temp = 1.0;
            }
            double humi = (humidity[index] * 0.14999999999999999 + 0.5) * (1.0 - 0.002) + preci * 0.002;
            if (humi < 0.0) {
                humi = 0.0;
            }
            if (humi > 1.0) {
                humi = 1.0;
            }
            temperature[index] = temp;
            humidity[index] = humi;
            biomes[index] = biomesTable[(int) (temp * 63) + (int) (humi * 63) * 64];
            index++;
        }
    }
    delete[] precipitation;
    biomeResult->biomes = biomes;
    biomeResult->temperature = temperature;
    biomeResult->humidity = humidity;
    return biomeResult;
}

void delete_biome_result(BiomeResult *biomeResult) {
    delete[] biomeResult->biomes;
    delete[] biomeResult->temperature;
    delete[] biomeResult->humidity;
    delete biomeResult;
}

BiomeResult *BiomeWrapper(uint64_t worldSeed, int32_t chunkX, int32_t chunkZ) {
    BiomeNoises *biomesOctaves = initBiomeGen(worldSeed);
    auto *biomes = getBiomes(chunkX * 16, chunkZ * 16, 16, 16, biomesOctaves);
    delete biomesOctaves;
    return biomes;
}

static inline double lerp(double x, double a, double b) {
    return a + x * (b - a);
}

static inline double grad(uint8_t hash, double x, double y, double z) {
    switch (hash & 0xFu) {
        case 0x0:
            return x + y;
        case 0x1:
            return -x + y;
        case 0x2:
            return x - y;
        case 0x3:
            return -x - y;
        case 0x4:
            return x + z;
        case 0x5:
            return -x + z;
        case 0x6:
            return x - z;
        case 0x7:
            return -x - z;
        case 0x8:
            return y + z;
        case 0x9:
            return -y + z;
        case 0xA:
            return y - z;
        case 0xB:
            return -y - z;
        case 0xC:
            return y + x;
        case 0xD:
            return -y + z;
        case 0xE:
            return y - x;
        case 0xF:
            return -y - z;
        default:
            return 0; // never happens
    }
}

static inline double grad2D(uint8_t hash, double x, double z) {
    return grad(hash, x, 0, z);
}

//we care only about 60-61, 77-78, 145-146, 162-163, 230-231, 247-248, 315-316, 332-333, 400-401, 417-418
static inline void generatePermutations(double **buffer, double x, double y, double z, double noiseFactorX, double noiseFactorY, double noiseFactorZ, double octaveSize, PermutationTable permutationTable) {
    uint8_t *permutations = permutationTable.permutations;
    double octaveWidth = 1.0 / octaveSize;
    int32_t i2 = -1;
    double x1 = 0.0;
    double x2 = 0.0;
    double xx1 = 0.0;
    double xx2 = 0.0;
    double t;
    double w;
    int columnIndex = 0; // possibleX[0]*5*17+3*17
    int possibleX[10] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4};
    int possibleZ[10] = {3, 4, 3, 4, 3, 4, 3, 4, 3, 4};
    for (int index = 0; index < 10; index++) {
        double xCoord = (x + (double) possibleX[index]) * noiseFactorX + permutationTable.xo;
        auto clampedXcoord = (int32_t) xCoord;
        if (xCoord < (double) clampedXcoord) {
            clampedXcoord--;
        }
        auto xBottoms = (uint8_t) ((uint32_t) clampedXcoord & 0xffu);
        xCoord -= clampedXcoord;
        t = xCoord * 6 - 15;
        w = (xCoord * t + 10);
        double fadeX = xCoord * xCoord * xCoord * w;
        double zCoord = (z + (double) possibleZ[index]) * noiseFactorZ + permutationTable.zo;
        auto clampedZCoord = (int32_t) zCoord;
        if (zCoord < (double) clampedZCoord) {
            clampedZCoord--;
        }
        auto zBottoms = (uint8_t) ((uint32_t) clampedZCoord & 0xffu);
        zCoord -= clampedZCoord;
        t = zCoord * 6 - 15;
        w = (zCoord * t + 10);
        double fadeZ = zCoord * zCoord * zCoord * w;
        for (int Y = 0; Y < 11; Y++) { // we cannot limit on lower bound without some issues later
            // ZCoord
            double yCoords = (y + (double) Y) * noiseFactorY + permutationTable.yo;
            auto clampedYCoords = (int32_t) yCoords;
            if (yCoords < (double) clampedYCoords) {
                clampedYCoords--;
            }
            auto yBottoms = (uint8_t) ((uint32_t) clampedYCoords & 0xffu);
            yCoords -= clampedYCoords;
            t = yCoords * 6 - 15;
            w = yCoords * t + 10;
            double fadeY = yCoords * yCoords * yCoords * w;
            // ZCoord

            if (Y == 0 || yBottoms != i2) { // this is wrong on so many levels, same ybottoms doesnt mean x and z were the same...
                i2 = yBottoms;
                uint16_t k2 = permutations[(uint8_t)((uint16_t)(permutations[(uint8_t)(xBottoms& 0xffu)] + yBottoms)& 0xffu)] + zBottoms;
                uint16_t l2 = permutations[(uint8_t)((uint16_t)(permutations[(uint8_t)(xBottoms& 0xffu)] + yBottoms + 1)& 0xffu)] + zBottoms;
                uint16_t k3 = permutations[(uint8_t)((uint16_t)(permutations[(uint8_t)((xBottoms + 1u)& 0xffu)] + yBottoms)& 0xffu)] + zBottoms;
                uint16_t l3 = permutations[(uint8_t)((uint16_t)(permutations[(uint8_t)((xBottoms + 1u)& 0xffu)] + yBottoms + 1)& 0xffu)] + zBottoms;
                x1 = lerp(fadeX, grad(permutations[(uint8_t)(k2& 0xffu)], xCoord, yCoords, zCoord), grad(permutations[(uint8_t)(k3& 0xffu)], xCoord - 1.0, yCoords, zCoord));
                x2 = lerp(fadeX, grad(permutations[(uint8_t)(l2& 0xffu)], xCoord, yCoords - 1.0, zCoord), grad(permutations[(uint8_t)(l3& 0xffu)], xCoord - 1.0, yCoords - 1.0, zCoord));
                xx1 = lerp(fadeX, grad(permutations[(uint8_t)((k2+1u)& 0xffu)], xCoord, yCoords, zCoord - 1.0), grad(permutations[(uint8_t)((k3+1u)& 0xffu)], xCoord - 1.0, yCoords, zCoord - 1.0));
                xx2 = lerp(fadeX, grad(permutations[(uint8_t)((l2+1u)& 0xffu)], xCoord, yCoords - 1.0, zCoord - 1.0), grad(permutations[(uint8_t)((l3+1u)& 0xffu)], xCoord - 1.0, yCoords - 1.0, zCoord - 1.0));
            }
            double y1 = lerp(fadeY, x1, x2);
            double y2 = lerp(fadeY, xx1, xx2);
            (*buffer)[columnIndex] = (*buffer)[columnIndex] + lerp(fadeZ, y1, y2) * octaveWidth;
            columnIndex++;
        }
    }
}

static inline void generateFixedPermutations(double **buffer, double x, double z, int sizeX, int sizeZ, double noiseFactorX, double noiseFactorZ, double octaveSize, PermutationTable permutationTable) {
    int index = 0;
    uint8_t *permutations = permutationTable.permutations;
    double octaveWidth = 1.0 / octaveSize;
    for (int X = 0; X < sizeX; X++) {
        double xCoord = (x + (double) X) * noiseFactorX + permutationTable.xo;
        int clampedXCoord = (int) xCoord;
        if (xCoord < (double) clampedXCoord) {
            clampedXCoord--;
        }
        auto xBottoms = (uint16_t) ((uint32_t) clampedXCoord & 0xffu);
        xCoord -= clampedXCoord;
        double fadeX = xCoord * xCoord * xCoord * (xCoord * (xCoord * 6.0 - 15.0) + 10.0);
        for (int Z = 0; Z < sizeZ; Z++) {
            double zCoord = (z + (double) Z) * noiseFactorZ + permutationTable.zo;
            int clampedZCoord = (int) zCoord;
            if (zCoord < (double) clampedZCoord) {
                clampedZCoord--;
            }
            auto zBottoms = (uint16_t) ((uint32_t) clampedZCoord & 0xffu);
            zCoord -= clampedZCoord;
            double fadeZ = zCoord * zCoord * zCoord * (zCoord * (zCoord * 6.0 - 15.0) + 10.0);
            uint16_t hhxz = ((permutations[permutations[xBottoms] & 0xffu] & 0xffu) + zBottoms) & 0xffu;
            uint16_t hhx1z = ((permutations[permutations[(xBottoms + 1u) & 0xffu] & 0xffu] & 0xffu) + zBottoms) & 0xffu;
            uint16_t Hhhxz = permutations[hhxz & 0xffu];
            uint16_t Hhhx1z = permutations[hhx1z & 0xffu];
            uint16_t Hhhxz1 = permutations[(hhxz + 1u) & 0xffu];
            uint16_t Hhhx1z1 = permutations[(hhx1z + 1u) & 0xffu];
            double x1 = lerp(fadeX, grad2D(Hhhxz, xCoord, zCoord), grad2D(Hhhx1z, xCoord - 1.0, zCoord));
            double x2 = lerp(fadeX, grad2D(Hhhxz1, xCoord, zCoord - 1.0), grad2D(Hhhx1z1, xCoord - 1.0, zCoord - 1.0));
            double y1 = lerp(fadeZ, x1, x2);
            (*buffer)[index] = (*buffer)[index] + y1 * octaveWidth;
            index++;
        }
    }
}

static inline void generateNormalPermutations(double **buffer, double x, double y, double z, int sizeX, int sizeY, int sizeZ, double noiseFactorX, double noiseFactorY, double noiseFactorZ, double octaveSize, PermutationTable permutationTable) {
    uint8_t *permutations = permutationTable.permutations;
    double octaveWidth = 1.0 / octaveSize;
    int32_t i2 = -1;
    double x1 = 0.0;
    double x2 = 0.0;
    double xx1 = 0.0;
    double xx2 = 0.0;
    double t;
    double w;
    int columnIndex = 0;
    for (int X = 0; X < sizeX; X++) {
        double xCoord = (x + (double) X) * noiseFactorX + permutationTable.xo;
        auto clampedXcoord = (int32_t) xCoord;
        if (xCoord < (double) clampedXcoord) {
            clampedXcoord--;
        }
        auto xBottoms = (uint8_t) ((uint32_t) clampedXcoord & 0xffu);
        xCoord -= clampedXcoord;
        t = xCoord * 6 - 15;
        w = (xCoord * t + 10);
        double fadeX = xCoord * xCoord * xCoord * w;
        for (int Z = 0; Z < sizeZ; Z++) {
            double zCoord = (z + (double) Z) * noiseFactorZ + permutationTable.zo;
            auto clampedZCoord = (int32_t) zCoord;
            if (zCoord < (double) clampedZCoord) {
                clampedZCoord--;
            }
            auto zBottoms = (uint8_t) ((uint32_t) clampedZCoord & 0xffu);
            zCoord -= clampedZCoord;
            t = zCoord * 6 - 15;
            w = (zCoord * t + 10);
            double fadeZ = zCoord * zCoord * zCoord * w;
            for (int Y = 0; Y < sizeY; Y++) {
                double yCoords = (y + (double) Y) * noiseFactorY + permutationTable.yo;
                auto clampedYCoords = (int32_t) yCoords;
                if (yCoords < (double) clampedYCoords) {
                    clampedYCoords--;
                }
                auto yBottoms = (uint8_t) ((uint32_t) clampedYCoords & 0xffu);
                yCoords -= clampedYCoords;
                t = yCoords * 6 - 15;
                w = yCoords * t + 10;
                double fadeY = yCoords * yCoords * yCoords * w;
                // ZCoord

                if (Y == 0 || yBottoms != i2) { // this is wrong on so many levels, same ybottoms doesnt mean x and z were the same...
                    i2 = yBottoms;
                    uint16_t k2 = permutations[(uint8_t)((uint16_t)(permutations[(uint8_t)(xBottoms& 0xffu)] + yBottoms)& 0xffu)] + zBottoms;
                    uint16_t l2 = permutations[(uint8_t)((uint16_t)(permutations[(uint8_t)(xBottoms& 0xffu)] + yBottoms + 1u )& 0xffu)] + zBottoms;
                    uint16_t k3 = permutations[(uint8_t)((uint16_t)(permutations[(uint8_t)((xBottoms + 1u)& 0xffu)] + yBottoms )& 0xffu)] + zBottoms;
                    uint16_t l3 = permutations[(uint8_t)((uint16_t)(permutations[(uint8_t)((xBottoms + 1u)& 0xffu)] + yBottoms + 1u) & 0xffu)] + zBottoms;
                    x1 = lerp(fadeX, grad(permutations[(uint8_t)(k2& 0xffu)], xCoord, yCoords, zCoord), grad(permutations[(uint8_t)(k3& 0xffu)], xCoord - 1.0, yCoords, zCoord));
                    x2 = lerp(fadeX, grad(permutations[(uint8_t)(l2& 0xffu)], xCoord, yCoords - 1.0, zCoord), grad(permutations[(uint8_t)(l3& 0xffu)], xCoord - 1.0, yCoords - 1.0, zCoord));
                    xx1 = lerp(fadeX, grad(permutations[(uint8_t)((k2+1u)& 0xffu)], xCoord, yCoords, zCoord - 1.0), grad(permutations[(uint8_t)((k3+1u)& 0xffu)], xCoord - 1.0, yCoords, zCoord - 1.0));
                    xx2 = lerp(fadeX, grad(permutations[(uint8_t)((l2+1u)& 0xffu)], xCoord, yCoords - 1.0, zCoord - 1.0), grad(permutations[(uint8_t)((l3+1u)& 0xffu)], xCoord - 1.0, yCoords - 1.0, zCoord - 1.0));
                }
                double y1 = lerp(fadeY, x1, x2);
                double y2 = lerp(fadeY, xx1, xx2);
                (*buffer)[columnIndex] = (*buffer)[columnIndex] + lerp(fadeZ, y1, y2) * octaveWidth;
                columnIndex++;
            }
        }
    }
}


static inline void generateNoise(double *buffer, double chunkX, double chunkY, double chunkZ, int sizeX, int sizeY, int sizeZ, double offsetX, double offsetY, double offsetZ, PermutationTable *permutationTable, int nbOctaves, int type) {
    memset(buffer, 0, sizeof(double) * sizeX * sizeZ * sizeY);
    double octavesFactor = 1.0;
    for (int octave = 0; octave < nbOctaves; octave++) {
        generateNormalPermutations(&buffer, chunkX, chunkY, chunkZ, sizeX, sizeY, sizeZ, offsetX * octavesFactor, offsetY * octavesFactor, offsetZ * octavesFactor, octavesFactor, permutationTable[octave]);
        octavesFactor /= 2.0;
    }
}

static inline void generateFixedNoise(double *buffer, double chunkX, double chunkZ, int sizeX, int sizeZ, double offsetX, double offsetZ, PermutationTable *permutationTable, int nbOctaves) {
    memset(buffer, 0, sizeof(double) * sizeX * sizeZ);
    double octavesFactor = 1.0;
    for (int octave = 0; octave < nbOctaves; octave++) {
        generateFixedPermutations(&buffer, chunkX, chunkZ, sizeX, sizeZ, offsetX * octavesFactor, offsetZ * octavesFactor, octavesFactor, permutationTable[octave]);
        octavesFactor /= 2.0;
    }
}


static inline void fillNoiseColumn(double **NoiseColumn, int chunkX, int chunkZ, const double *temperature, const double *humidity, TerrainNoises terrainNoises) {
    // we only need
    // (60, 77, 145, 162, 61, 78, 146, 163)
    // (145, 162, 230, 247, 146, 163, 231, 248)
    // (230, 247, 315, 332, 231, 248, 316, 333)
    // (315, 332, 400, 417, 316, 333, 401, 418)
    // which is only 60-61, 77-78, 145-146, 162-163, 230-231, 247-248, 315-316, 332-333, 400-401, 417-418 // so 20
    // or as cellCounter 3,4,8,9,13,14,18,19,23,24
    // or as x indices 1 2 3 4 5 and fixed z to 3-4
    // 5 is the cellsize here and 17 the column size, they are inlined constants
    double d = 684.41200000000003;
    double d1 = 684.41200000000003;
    // this is super fast (but we only care about 3,4,8,9,13,14,18,19,23,24)
    auto *surfaceNoise = new double[5 * 5];
    auto *depthNoise = new double[5 * 5];
    generateFixedNoise(surfaceNoise, chunkX, chunkZ, 5, 5, 1.121, 1.121, terrainNoises.scale, 10);
    generateFixedNoise(depthNoise, chunkX, chunkZ, 5, 5, 200.0, 200.0, terrainNoises.depth, 16);

    auto *mainLimitPerlinNoise = new double[425];
    auto *minLimitPerlinNoise = new double[425];
    auto *maxLimitPerlinNoise = new double[425];
    // use optimized noise
    generateNoise(mainLimitPerlinNoise, chunkX, 0, chunkZ, 5, 17, 5, d / 80, d1 / 160, d / 80, terrainNoises.mainLimit, 8, 0);
    generateNoise(minLimitPerlinNoise, chunkX, 0, chunkZ, 5, 17, 5, d, d1, d, terrainNoises.minLimit, 16, 0);
    generateNoise(maxLimitPerlinNoise, chunkX, 0, chunkZ, 5, 17, 5, d, d1, d, terrainNoises.maxLimit, 16, 0);

    int var5 = 5;
    int var7 = 5;
    int var6 = 17;

    int var14 = 0;
    int var15 = 0;
    int var16 = 16 / var5;

    for(int var17 = 0; var17 < var5; ++var17) {
        int var18 = var17 * var16 + var16 / 2;

        for(int var19 = 0; var19 < var7; ++var19) {
            int var20 = var19 * var16 + var16 / 2;
            double var21 = humidity[var18 * 16 + var20];
            double var23 = temperature[var18 * 16 + var20] * var21;
            double var25 = 1.0D - var23;
            var25 *= var25;
            var25 *= var25;
            var25 = 1.0D - var25;
            double var27 = (surfaceNoise[var15] + 256.0D) / 512.0D;
            var27 *= var25;
            if(var27 > 1.0D) {
                var27 = 1.0D;
            }

            double var29 = depthNoise[var15] / 8000.0D;
            if(var29 < 0.0D) {
                var29 = -var29 * 0.3D;
            }

            var29 = var29 * 3.0D - 2.0D;
            if(var29 < 0.0D) {
                var29 /= 2.0D;
                if(var29 < -1.0D) {
                    var29 = -1.0D;
                }

                var29 /= 1.4D;
                var29 /= 2.0D;
                var27 = 0.0D;
            } else {
                if(var29 > 1.0D) {
                    var29 = 1.0D;
                }

                var29 /= 8.0D;
            }

            if(var27 < 0.0D) {
                var27 = 0.0D;
            }

            var27 += 0.5D;
            var29 = var29 * (double)var6 / 16.0D;
            double var31 = (double)var6 / 2.0D + var29 * 4.0D;
            ++var15;

            for(int var33 = 0; var33 < var6; ++var33) {
                double var34 = 0.0D;
                double var36 = ((double)var33 - var31) * 12.0D / var27;
                if(var36 < 0.0D) {
                    var36 *= 4.0D;
                }

                double var38 = minLimitPerlinNoise[var14] / 512.0D;
                double var40 = maxLimitPerlinNoise[var14] / 512.0D;
                double var42 = (mainLimitPerlinNoise[var14] / 10.0D + 1.0D) / 2.0D;
                if(var42 < 0.0D) {
                    var34 = var38;
                } else if(var42 > 1.0D) {
                    var34 = var40;
                } else {
                    var34 = var38 + (var40 - var38) * var42;
                }

                var34 -= var36;
                if(var33 > var6 - 4) {
                    double var44 = (double)((float)(var33 - (var6 - 4)) / 3.0F);
                    var34 = var34 * (1.0D - var44) + -10.0D * var44;
                }

                (*NoiseColumn)[var14] = var34;
                ++var14;
            }
        }
    }

    delete[] surfaceNoise;
    delete[] depthNoise;
    delete[] mainLimitPerlinNoise;
    delete[] minLimitPerlinNoise;
    delete[] maxLimitPerlinNoise;
}

static inline void generateTerrain(int chunkX, int chunkZ, uint8_t **chunkCache, double *temperatures, double *humidity, TerrainNoises terrainNoises) {
    auto *NoiseColumn = new double[425];
    memset(NoiseColumn, 0, sizeof(double) * 425);
    fillNoiseColumn(&NoiseColumn, chunkX * 4, chunkZ * 4, temperatures, humidity, terrainNoises);
    
    int var6 = 4;
    int var7 = 64;
    int var8 = var6 + 1;
    int var9 = 17;
    int var10 = var6 + 1;

    for(int var11 = 0; var11 < var6; ++var11) {
        for(int var12 = 0; var12 < var6; ++var12) {
            for(int var13 = 0; var13 < 16; ++var13) {
                double var14 = 0.125D;
                double var16 = NoiseColumn[((var11 + 0) * var10 + var12 + 0) * var9 + var13];
                double var18 = NoiseColumn[((var11 + 0) * var10 + var12 + 1) * var9 + var13];
                double var20 = NoiseColumn[((var11 + 1) * var10 + var12 + 0) * var9 + var13];
                double var22 = NoiseColumn[((var11 + 1) * var10 + var12 + 1) * var9 + var13];
                double var24 = (NoiseColumn[((var11 + 0) * var10 + var12 + 0) * var9 + var13 + 1] - var16) * var14;
                double var26 = (NoiseColumn[((var11 + 0) * var10 + var12 + 1) * var9 + var13 + 1] - var18) * var14;
                double var28 = (NoiseColumn[((var11 + 1) * var10 + var12 + 0) * var9 + var13 + 1] - var20) * var14;
                double var30 = (NoiseColumn[((var11 + 1) * var10 + var12 + 1) * var9 + var13 + 1] - var22) * var14;

                for(int var32 = 0; var32 < 8; ++var32) {
                    double var33 = 0.25D;
                    double var35 = var16;
                    double var37 = var18;
                    double var39 = (var20 - var16) * var33;
                    double var41 = (var22 - var18) * var33;

                    for(int var43 = 0; var43 < 4; ++var43) {
                        int var44 = var43 + var11 * 4 << 11 | 0 + var12 * 4 << 7 | var13 * 8 + var32;
                        short var45 = 128;
                        double var46 = 0.25D;
                        double var48 = var35;
                        double var50 = (var37 - var35) * var46;

                        for(int var52 = 0; var52 < 4; ++var52) {
                            double var53 = temperatures[(var11 * 4 + var43) * 16 + var12 * 4 + var52];
                            int var55 = 0;
                            if(var13 * 8 + var32 < var7) {
                                if(var53 < 0.5D && var13 * 8 + var32 >= var7 - 1) {
                                    var55 = ICE;
                                } else {
                                    var55 = MOVING_WATER;
                                }
                            }

                            if(var48 > 0.0D) {
                                var55 = STONE;
                            }

                            (*chunkCache)[var44] = (uint8_t)var55;
                            var44 += var45;
                            var48 += var50;
                        }

                        var35 += var39;
                        var37 += var41;
                    }

                    var16 += var24;
                    var18 += var26;
                    var20 += var28;
                    var22 += var30;
                }
            }
        }
    }
    delete[]NoiseColumn;
}

static inline TerrainNoises *initTerrain(uint64_t worldSeed) {
    auto *terrainNoises = new TerrainNoises;
    Random worldRandom = get_random(worldSeed);
    PermutationTable *octaves = terrainNoises->minLimit;
    initOctaves(octaves, &worldRandom, 16);
    octaves = terrainNoises->maxLimit;
    initOctaves(octaves, &worldRandom, 16);
    octaves = terrainNoises->mainLimit;
    initOctaves(octaves, &worldRandom, 8);
    for (int j = 0; j < 4; j++) { //shore and river composition
        advance6(&worldRandom);
        // could be just advanced but i am afraid of nextInt
        uint8_t i = 0u;
        do {
            random_next_int(&worldRandom, 256u - i);
        } while (i++ != 255);
    }
    octaves = terrainNoises->surfaceElevation;
    initOctaves(octaves, &worldRandom, 4);
    octaves = terrainNoises->scale;
    initOctaves(octaves, &worldRandom, 10);
    octaves = terrainNoises->depth;
    initOctaves(octaves, &worldRandom, 16);
    return terrainNoises;
}

#define PI 3.14159265358
#include <math.h>

static int floor_double(double var0) {
    int var2 = (int)var0;
    return var0 < (double)var2 ? var2 - 1 : var2;
}

void releaseEntitySkin(int var1, int var2, uint8_t *var3, double var4, double var6, double var8, float var10, float var11, float var12, int var13, int var14, double var15, uint64_t *rng); 

void func_870_a(int var1, int var2, uint8_t *var3, double var4, double var6, double var8, uint64_t *rng) {
    releaseEntitySkin(var1, var2, var3, var4, var6, var8, 1.0F + nextFloat(rng) * 6.0F, 0.0F, 0.0F, -1, -1, 0.5D, rng);
}

void releaseEntitySkin(int var1, int var2, uint8_t *var3, double var4, double var6, double var8, float var10, float var11, float var12, int var13, int var14, double var15, uint64_t *rng) {
    double var17 = (double)(var1 * 16 + 8);
    double var19 = (double)(var2 * 16 + 8);
    float var21 = 0.0F;
    float var22 = 0.0F;
    uint64_t var23;
    uint64_t seed = nextLong(rng);
    setSeed(&var23, seed);
    if(var14 <= 0) {
        int var24 = 8 * 16 - 16;
        var14 = var24 - nextInt(&var23, var24 / 4);
    }

    bool var52 = false;
    if(var13 == -1) {
        var13 = var14 / 2;
        var52 = true;
    }

    int var25 = nextInt(&var23, var14 / 2) + var14 / 4;

    for(bool var26 = nextInt(&var23, 6) == 0; var13 < var14; ++var13) {
        double var27 = 1.5D + (double)(sinf((float)var13 * (float)PI / (float)var14) * var10 * 1.0F);
        double var29 = var27 * var15;
        float var31 = cosf(var12);
        float var32 = sinf(var12);
        var4 += (double)(cosf(var11) * var31);
        var6 += (double)var32;
        var8 += (double)(sinf(var11) * var31);
        if(var26) {
            var12 *= 0.92F;
        } else {
            var12 *= 0.7F;
        }

        var12 += var22 * 0.1F;
        var11 += var21 * 0.1F;
        var22 *= 0.9F;
        var21 *= 12.0F / 16.0F;
        var22 += (nextFloat(&var23) - nextFloat(&var23)) * nextFloat(&var23) * 2.0F;
        var21 += (nextFloat(&var23) - nextFloat(&var23)) * nextFloat(&var23) * 4.0F;
        if(!var52 && var13 == var25 && var10 > 1.0F) {
            releaseEntitySkin(var1, var2, var3, var4, var6, var8, nextFloat(&var23) * 0.5F + 0.5F, var11 - (float)PI * 0.5F, var12 / 3.0F, var13, var14, 1.0D, rng);
            releaseEntitySkin(var1, var2, var3, var4, var6, var8, nextFloat(&var23) * 0.5F + 0.5F, var11 + (float)PI * 0.5F, var12 / 3.0F, var13, var14, 1.0D, rng);
            return;
        }

        if(var52 || nextInt(&var23, 4) != 0) {
            double var33 = var4 - var17;
            double var35 = var8 - var19;
            double var37 = (double)(var14 - var13);
            double var39 = (double)(var10 + 2.0F + 16.0F);
            if(var33 * var33 + var35 * var35 - var37 * var37 > var39 * var39) {
                return;
            }

            if(var4 >= var17 - 16.0D - var27 * 2.0D && var8 >= var19 - 16.0D - var27 * 2.0D && var4 <= var17 + 16.0D + var27 * 2.0D && var8 <= var19 + 16.0D + var27 * 2.0D) {
                int var53 = floor_double(var4 - var27) - var1 * 16 - 1;
                int var34 = floor_double(var4 + var27) - var1 * 16 + 1;
                int var54 = floor_double(var6 - var29) - 1;
                int var36 = floor_double(var6 + var29) + 1;
                int var55 = floor_double(var8 - var27) - var2 * 16 - 1;
                int var38 = floor_double(var8 + var27) - var2 * 16 + 1;
                if(var53 < 0) {
                    var53 = 0;
                }

                if(var34 > 16) {
                    var34 = 16;
                }

                if(var54 < 1) {
                    var54 = 1;
                }

                if(var36 > 120) {
                    var36 = 120;
                }

                if(var55 < 0) {
                    var55 = 0;
                }

                if(var38 > 16) {
                    var38 = 16;
                }

                bool var56 = false;

                int var40;
                int var43;
                for(var40 = var53; !var56 && var40 < var34; ++var40) {
                    for(int var41 = var55; !var56 && var41 < var38; ++var41) {
                        for(int var42 = var36 + 1; !var56 && var42 >= var54 - 1; --var42) {
                            var43 = (var40 * 16 + var41) * 128 + var42;
                            if(var42 >= 0 && var42 < 128) {
                                if(var3[var43] == MOVING_WATER || var3[var43] == MOVING_WATER) {
                                    var56 = true;
                                }

                                if(var42 != var54 - 1 && var40 != var53 && var40 != var34 - 1 && var41 != var55 && var41 != var38 - 1) {
                                    var42 = var54;
                                }
                            }
                        }
                    }
                }

                if(!var56) {
                    for(var40 = var53; var40 < var34; ++var40) {
                        double var57 = ((double)(var40 + var1 * 16) + 0.5D - var4) / var27;

                        for(var43 = var55; var43 < var38; ++var43) {
                            double var44 = ((double)(var43 + var2 * 16) + 0.5D - var8) / var27;
                            int var46 = (var40 * 16 + var43) * 128 + var36;
                            bool var47 = false;
                            if(var57 * var57 + var44 * var44 < 1.0D) {
                                for(int var48 = var36 - 1; var48 >= var54; --var48) {
                                    double var49 = ((double)var48 + 0.5D - var6) / var29;
                                    if(var49 > -0.7D && var57 * var57 + var49 * var49 + var44 * var44 < 1.0D) {
                                        uint8_t var51 = var3[var46];
                                        if(var51 == GRASS) {
                                            var47 = true;
                                        }

                                        if(var51 == STONE || var51 == DIRT || var51 == GRASS) {
                                            if(var48 < 10) {
                                                var3[var46] = LAVA;
                                            } else {
                                                var3[var46] = AIR;
                                                if(var47 && var3[var46 - 1] == DIRT) {
                                                    var3[var46 - 1] = GRASS;
                                                }
                                            }
                                        }
                                    }

                                    --var46;
                                }
                            }
                        }
                    }

                    if(var52) {
                        break;
                    }
                }
            }
        }
    }

}

static inline void caves(int var2, int var3, int var4, int var5, uint8_t *chunkCache, uint64_t *rng) {
    int var7 = nextInt(rng, nextInt(rng, nextInt(rng, 40) + 1) + 1);

    if (nextInt(rng, 15) != 0) {
        var7 = 0;
    }

    for(int var8 = 0; var8 < var7; ++var8) {
        int v = nextInt(rng, 16);
        double var9 = (double)(var2 * 16 + v);
        double var11 = (double)nextInt(rng, nextInt(rng, 120) + 8);
        double var13 = (double)(var3 * 16 + nextInt(rng, 16));
        int var15 = 1;
        if(nextInt(rng, 4) == 0) {
            func_870_a(var4, var5, chunkCache, var9, var11, var13, rng);
            var15 += nextInt(rng, 4);
        }

        for(int var16 = 0; var16 < var15; ++var16) {
            float var17 = nextFloat(rng) * (float)PI * 2.0F;
            float var18 = (nextFloat(rng) - 0.5F) * 2.0F / 8.0F;
            float var19 = nextFloat(rng) * 2.0F + nextFloat(rng);
            releaseEntitySkin(var4, var5, chunkCache, var9, var11, var13, var19, var17, var18, 0, 0, 1.0D, rng);
        }
    }
}

static inline void generateCaves(uint64_t worldSeed, int var3, int var4, uint8_t *chunkCache) {
    uint64_t rng;
    int var6 = 8;
    setSeed(&rng, worldSeed);
    uint64_t var7 = ((int64_t)nextLong(&rng) / 2) * 2 + 1;
    uint64_t var9 = ((int64_t)nextLong(&rng) / 2) * 2 + 1;
    for(int var11 = var3 - var6; var11 <= var3 + var6; ++var11) {
        for(int var12 = var4 - var6; var12 <= var4 + var6; ++var12) {
            setSeed(&rng, (uint64_t)var11 * var7 + (uint64_t)var12 * var9 ^ worldSeed);
            caves(var11, var12, var3, var4, chunkCache, &rng);
        }
    }
}

Chunk TerrainWrapper(uint64_t worldSeed, int32_t chunkX, int32_t chunkZ);

uint8_t getBlockID(World *world, int x, int y, int z) {
    std::tuple<int, int> key = std::tuple<int, int>(x >> 4, z >> 4);
    if (world->chunks.count(key) < 1) {
        Chunk c = TerrainWrapper(world->seed, x >> 4, z >> 4);
        world->chunks[std::tuple<int, int>(x >> 4, z >> 4)] = c;
    }
    Chunk c = world->chunks.at(key); 
    int cx = x & 15;
    int cy = y;
    int cz = z & 15;
    return c.blocks[cx << 11 | cz << 7 | cy];
}

static inline bool generate_dungeons(World *world, int var3, int var4, int var5, uint64_t *rng, int *x, int *z) {
    uint8_t var6 = 3;
    int var7 = nextInt(rng, 2) + 2;
    int var8 = nextInt(rng, 2) + 2;
    int var9 = 0;

    int var10;
    int var11;
    int var12;
    for(var10 = var3 - var7 - 1; var10 <= var3 + var7 + 1; ++var10) {
        for(var11 = var4 - 1; var11 <= var4 + var6 + 1; ++var11) {
            for(var12 = var5 - var8 - 1; var12 <= var5 + var8 + 1; ++var12) {
                uint8_t var13 = getBlockID(world, var10, var11, var12);
                if(var11 == var4 - 1 && !(var13 != AIR)) {
                    return false;
                }

                if(var11 == var4 + var6 + 1 && !(var13 != AIR)) {
                    return false;
                }

                if((var10 == var3 - var7 - 1 || var10 == var3 + var7 + 1 || var12 == var5 - var8 - 1 || var12 == var5 + var8 + 1) && var11 == var4 && getBlockID(world, var10, var11, var12)==AIR && getBlockID(world, var10, var11 + 1, var12)==AIR) {
                    ++var9;
                }
            }
        }
    }
    if(var9 >= 1 && var9 <= 5) {
        *x = var3 - var7 - 1; 
        *z = var5 - var8 - 1;
        return true;
    }
    return false;
}

static inline uint8_t *provideChunk(uint64_t worldSeed, int chunkX, int chunkZ, BiomeResult *biomeResult, TerrainNoises *terrainNoises) {
    Random worldRandom = get_random((uint64_t) ((long) chunkX * 0x4f9939f508L + (long) chunkZ * 0x1ef1565bd5L));
    auto *chunkCache = new uint8_t[16 * 16 * 128];
    generateTerrain(chunkX, chunkZ, &chunkCache, biomeResult->temperature, biomeResult->humidity, *terrainNoises);
    generateCaves(worldSeed, chunkX, chunkZ, chunkCache);

    return chunkCache;    
}

DungeonResult chunkHasDungeon(World *world, int chunkX, int chunkZ) {
    DungeonResult result;
    result.has_dungeon = false;

    int var4 = chunkX * 16;
	int var5 = chunkZ * 16;

    uint64_t rng;
    setSeed(&rng, world->seed);
    uint64_t var7 = ((int64_t)nextLong(&rng) / 2L) * 2L + 1L;
    uint64_t var9 = ((int64_t)nextLong(&rng) / 2L) * 2L + 1L;
    setSeed(&rng, (long)chunkX * var7 + (long)chunkZ * var9 ^ world->seed);

    double var11 = 0.25D;
    int var13;
    int var14;
    int var15;
    if(nextInt(&rng, 4) == 0) {
        return result;
    }

    if(nextInt(&rng, 8) == 0) {
        return result;
    }
    int var16;
    for(var13 = 0; var13 < 8; ++var13) {
        var14 = var4 + nextInt(&rng, 16) + 8;
        var15 = nextInt(&rng, 128);
        var16 = var5 + nextInt(&rng, 16) + 8;
        int x, z;
        if (generate_dungeons(world, var14, var15, var16, &rng, &x, &z)) {
            result.has_dungeon = true;
            result.x = x;
            result.z = z;
            return result;
        }
    }
    return result;
}

void delete_terrain_result(TerrainResult *terrainResult) {
    delete[] terrainResult->chunkHeights;
    delete_biome_result(terrainResult->biomeResult);
    delete terrainResult;
}

uint8_t *TerrainInternalWrapper(uint64_t worldSeed, int32_t chunkX, int32_t chunkZ, BiomeResult *biomeResult) {
    TerrainNoises *terrainNoises = initTerrain(worldSeed);
    uint8_t *chunkCache = provideChunk(worldSeed, chunkX, chunkZ, biomeResult, terrainNoises);
    delete terrainNoises;
    return chunkCache;
}

Chunk TerrainWrapper(uint64_t worldSeed, int32_t chunkX, int32_t chunkZ) {
    BiomeResult *biomeResult = BiomeWrapper(worldSeed, chunkX, chunkZ);
    auto *chunkCache = TerrainInternalWrapper(worldSeed, chunkX, chunkZ, biomeResult);
    delete_biome_result(biomeResult);
    return (Chunk){.cx=chunkX, .cz=chunkZ, .blocks=chunkCache};
}

World new_world(uint64_t seed) {
    World w;
    w.seed = seed;
    return w;
}

void free_world(World world) {
    for (const auto& [position, chunk] : world.chunks) {
        delete chunk.blocks;
    }
}
