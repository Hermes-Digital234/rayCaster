// player.h - grid movement, turning and the animation state machine.
// The player knows nothing about what tiles mean. Interacting is the owner's job (the explore state):
// it looks at the cell in front of the player (playerFacing) and decides what, if anything, can be done.
#ifndef PLAYER_H
#define PLAYER_H

#include "core/input.h"
#include "dungeon/map.h"

typedef enum {
    PH_IDLE,
    PH_MOVE,
    PH_FADE_OUT, // screen fading to black while the camera creeps toward the tile
    PH_BLACK,    // fully black: the owner may now teleport/swap the map, then call playerEndBlack
    PH_FADE_IN,
} Phase;

typedef struct {
    // Logical state: which cell the player is in and which way they face
    int gx, gy;
    float targetAngle; // always a multiple of 90 degrees

    // Visual state: what the camera shows (animated toward the logical state)
    float posX, posY, angle;

    // Animation bookkeeping
    Phase phase;
    float t;                          // 0..1 progress of the current animation
    float startX, startY, startAngle; // where the current animation began
    float creepX, creepY;             // where the camera drifts to while fading out

    float fade;         // 0 = clear, 1 = fully black (draw as a black overlay)
    int itemsCollected;

    bool arrived;  // true for ONE frame after landing on a tile (walking, or fading in)
} Player;

void playerInit(Player *p, const Map *m, int gx, int gy);

// The direction the player is facing as a cell offset, e.g. (1,0) = east, (0,-1) = north.
void playerFacing(const Player *p, int *dirX, int *dirY);

// Instantly puts the player in a cell, facing (faceDx, faceDy). Does not touch the animation phase.
void playerPlace(Player *p, int gx, int gy, int faceDx, int faceDy);

// Call once per frame. Pass ACT_NONE if there is no input. Only takes a new action while idle.
void playerUpdate(Player *p, const Map *m, Action act, float dt);

// --- Fade transitions (doors, skips, stairs). Only starts from PH_IDLE. ---
// 1. playerBeginFade: fade to black while creeping a little in (dirX, dirY).
// 2. When player.phase == PH_BLACK, do the teleport / map swap (playerTeleport or playerPlace).
// 3. playerEndBlack: fade back in. `arrived` is set once the fade-in finishes.
void playerBeginFade(Player *p, int dirX, int dirY);
void playerTeleport(Player *p, int gx, int gy); // new cell, keeps facing
void playerEndBlack(Player *p);

#endif