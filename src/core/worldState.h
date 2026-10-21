// worldState.h - what the player has changed in the dungeon, remembered per map for the whole run.
//
// Maps are reloaded from their image every time they are entered, so changes (an opened chest, a discovered skip,
// an event that can only happen once) are recorded here and re-applied on load. Saving writes this list to the
// save file; loading puts it back.
#ifndef WORLD_STATE_H
#define WORLD_STATE_H

#include <stdbool.h>

typedef enum {
    WORLD_CHEST_OPENED,   // (x, y) of the chest
    WORLD_SKIP_DISCOVERED, // (x, y) of the skip
    WORLD_EVENT_FIRED,    // (x, y) of the tile, index = position of the event in the map's data file
} WorldFlagType;

typedef struct {
    WorldFlagType type;
    int layer, map;
    int x, y;
    int index;
} WorldFlag;

void worldReset(void);

// Records a flag. Recording the same flag twice does nothing.
void worldMark(WorldFlagType type, int layer, int map, int x, int y, int index);

bool worldHas(WorldFlagType type, int layer, int map, int x, int y, int index);

int worldCount(void);
const WorldFlag *worldAt(int i);

#endif
