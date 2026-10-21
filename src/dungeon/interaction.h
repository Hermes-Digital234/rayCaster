// interaction.h - what you can do with the special tile in front of you. Pure rules, no drawing or input.
//
// When the player faces a door, stairs, skip or closed chest, an "interact" button appears. Pressing it
// asks the player first (a popup), and the explore state only acts if they say yes.
#ifndef INTERACTION_H
#define INTERACTION_H

#include "dungeon/map.h"

typedef enum {
    INTERACT_NONE,
    INTERACT_DOOR,
    INTERACT_STAIRS_UP,
    INTERACT_STAIRS_DOWN,
    INTERACT_SKIP,
    INTERACT_CHEST,
} InteractKind;

// What the player can do with the tile at (tileX, tileY) when facing it in direction (dirX, dirY).
// INTERACT_NONE means it is just scenery (a plain wall, an opened chest, a door with a wall behind it).
// Skips always interact, from either side: whether you can pass is decided after you say yes.
InteractKind interactionKind(const Map *map, int tileX, int tileY, int dirX, int dirY);

// The question shown in the popup.
const char *interactionPrompt(InteractKind kind);

// The short text on the interact button, e.g. "Open door".
const char *interactionLabel(InteractKind kind);

#endif