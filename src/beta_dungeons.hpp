#pragma once
#include <cstdint>

typedef struct {
    int cx, cz;
    uint8_t *blocks;
} Chunk;

#include <map>

typedef struct {
    uint64_t seed;
    std::map<std::tuple<int, int>, Chunk> chunks;
} World;

typedef struct {
    int x, z;
    bool has_dungeon;
} DungeonResult;

World new_world(uint64_t seed);
DungeonResult chunkHasDungeon(World *world, int chunkX, int chunkZ);
void free_world(World world);