#ifndef DUNGEON_EXPLORE_H
#define DUNGEON_EXPLORE_H

#include "core/save.h"
#include <stdbool.h>

// Registers STATE_DUNGEON_EXPLORE with the state manager.
// Call after stateManagerInit and before pushing the state.
void dungeonExploreRegister(void);

// Chooses which map the NEXT push of STATE_DUNGEON_EXPLORE loads: assets/maps/l<layer>m<map>.png.
// Defaults to layer 1, map 1. The player starts on the map's first floor tile.
void dungeonExploreSelectMap(int layer, int map);

// Like dungeonExploreSelectMap, but the next push starts the player at a saved spot.
void dungeonExploreStartAt(const SaveLocation *location);

// Where the player is right now. False if the dungeon state is not running.
bool dungeonExploreGetLocation(SaveLocation *out);

// Takes the player to a saved spot. If the dungeon is already running (loading from the pause or game-over
// menu) it jumps there instantly; otherwise it pushes the dungeon state. False if the map cannot be loaded.
bool dungeonExploreLoadLocation(const SaveLocation *location);

#endif
