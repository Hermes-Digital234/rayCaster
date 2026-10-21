// player.h - grid movement, turning, door transitions and the animation state machine
#ifndef PLAYER_H
#define PLAYER_H

#include "input.h"
#include "map.h"

typedef enum { PH_IDLE, PH_MOVE, PH_DOOR_OUT, PH_DOOR_IN } Phase;

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
    float creepX, creepY;             // where the camera drifts to while fading out at a door
    int doorDestX, doorDestY;         // cell the player appears in after the door

    float fade;         // 0 = clear, 1 = fully black (draw as a black overlay)
    int itemsCollected;
} Player;

void player_init(Player *p, const Map *m, int gx, int gy);

// Call once per frame. Pass ACT_NONE if there is no input.
// Only takes a new action while idle. May modify the map (picking up items).
void player_update(Player *p, Map *m, Action act, float dt);

#endif
