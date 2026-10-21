#include "core/worldState.h"

#include <stdlib.h>

static WorldFlag *flags;
static int flagCount, flagCapacity;

void worldReset(void) {
    flagCount = 0;
}

bool worldHas(WorldFlagType type, int layer, int map, int x, int y, int index) {
    for (int i = 0; i < flagCount; i++) {
        const WorldFlag *f = &flags[i];
        if (f->type == type && f->layer == layer && f->map == map && f->x == x && f->y == y && f->index == index)
            return true;
    }
    return false;
}

void worldMark(WorldFlagType type, int layer, int map, int x, int y, int index) {
    if (worldHas(type, layer, map, x, y, index)) return;

    if (flagCount == flagCapacity) {
        int capacity = flagCapacity ? flagCapacity * 2 : 32;
        WorldFlag *grown = realloc(flags, (size_t)capacity * sizeof(WorldFlag));
        if (!grown) return;
        flags = grown;
        flagCapacity = capacity;
    }
    flags[flagCount++] = (WorldFlag){type, layer, map, x, y, index};
}

int worldCount(void) { return flagCount; }

const WorldFlag *worldAt(int i) {
    return (i >= 0 && i < flagCount) ? &flags[i] : NULL;
}
