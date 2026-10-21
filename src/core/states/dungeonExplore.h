#ifndef DUNGEON_EXPLORE_H
#define DUNGEON_EXPLORE_H

// Registers STATE_DUNGEON_EXPLORE with the state manager.
// Call after stateManagerInit and before pushing the state.
void dungeonExploreRegister(void);

// Chooses which map the NEXT push of STATE_DUNGEON_EXPLORE loads: assets/maps/l<layer>m<map>.png.
// Defaults to layer 1, map 1.
void dungeonExploreSelectMap(int layer, int map);

#endif
