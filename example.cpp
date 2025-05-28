#include <stdio.h>

#include "src/beta_dungeons.hpp"

int main(void) {
    World world = new_world(46290ull);
    for (int cx = -5; cx < 5; cx++) {
        for (int cz = -5; cz < 5; cz++) {
            DungeonResult result = chunkHasDungeon(&world, cx ,cz);
            if (result.has_dungeon) {
                printf("%d %d\n", result.x, result.z);
            }
        }
    }
    free_world(world);
}