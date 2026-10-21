// player.c - movement rules and animation. Knows nothing about keys or drawing.
#include "player.h"
#include "raylib.h" // PI
#include <math.h>

#define STEP_TIME 0.18f // seconds per move/turn
#define DOOR_TIME 0.40f // seconds for each half of the door fade

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

void player_init(Player *p, const Map *m, int gx, int gy) {
    p->gx = gx;
    p->gy = gy;

    // Face the first open direction
    p->targetAngle = 0.0f;
    for (int k = 0; k < 4; k++) {
        float a = k * PI / 2;
        if (map_walkable(map_tile(m, gx + (int)lroundf(cosf(a)), gy + (int)lroundf(sinf(a))))) {
            p->targetAngle = a;
            break;
        }
    }

    p->posX = gx + 0.5f;
    p->posY = gy + 0.5f;
    p->angle = p->targetAngle;
    p->startX = p->posX;
    p->startY = p->posY;
    p->startAngle = p->angle;
    p->creepX = p->creepY = 0.0f;
    p->doorDestX = p->doorDestY = 0;
    p->phase = PH_IDLE;
    p->t = 0.0f;
    p->fade = 0.0f;
    p->itemsCollected = 0;
}

// Decide what an action does, and start the matching animation
static void begin_action(Player *p, const Map *m, Action act) {
    // Facing and right-hand directions as integer cell offsets
    int fx = (int)lroundf(cosf(p->targetAngle)), fy = (int)lroundf(sinf(p->targetAngle));
    int rx = (int)lroundf(sinf(p->targetAngle)), ry = (int)lroundf(-cosf(p->targetAngle));

    int mx = 0, my = 0;
    float turn = 0.0f;
    switch (act) {
        case ACT_FORWARD:      mx =  fx; my =  fy; break;
        case ACT_BACK:         mx = -fx; my = -fy; break;
        case ACT_STRAFE_RIGHT: mx =  rx; my =  ry; break;
        case ACT_STRAFE_LEFT:  mx = -rx; my = -ry; break;
        case ACT_TURN_LEFT:    turn =  PI / 2; break;
        case ACT_TURN_RIGHT:   turn = -PI / 2; break;
        case ACT_NONE:         return;
    }

    p->startX = p->posX;
    p->startY = p->posY;
    p->startAngle = p->angle;

    if (turn != 0.0f) {
        p->targetAngle += turn;
        p->phase = PH_MOVE;
        p->t = 0.0f;
        return;
    }

    int nx = p->gx + mx, ny = p->gy + my;
    int tile = map_tile(m, nx, ny);

    if (map_walkable(tile)) {
        p->gx = nx;
        p->gy = ny;
        p->phase = PH_MOVE;
        p->t = 0.0f;
    } else if (tile == T_DOOR) {
        // The player reappears on the far side of the door, in the same direction of travel
        int ox = nx + mx, oy = ny + my;
        if (map_walkable(map_tile(m, ox, oy))) {
            p->doorDestX = ox;
            p->doorDestY = oy;
            p->creepX = p->gx + 0.5f + mx * 0.3f;
            p->creepY = p->gy + 0.5f + my * 0.3f;
            p->phase = PH_DOOR_OUT;
            p->t = 0.0f;
        }
    }
    // water, walls, or a blocked door: nothing happens
}

// Things that happen when the player lands on a tile
static void on_arrive(Player *p, Map *m) {
    int here = map_tile(m, p->gx, p->gy);
    if (here == T_ITEM) {
        p->itemsCollected++;
        map_set(m, p->gx, p->gy, T_FLOOR);
    }
    // T_HEAL and T_EVENT: hook your game logic in here later
}

void player_update(Player *p, Map *m, Action act, float dt) {
    if (p->phase == PH_IDLE) begin_action(p, m, act);

    bool arrived = false;

    if (p->phase == PH_MOVE) {
        p->t += dt / STEP_TIME;
        if (p->t > 1.0f) p->t = 1.0f;
        float s = smooth(p->t);
        p->posX = lerpf(p->startX, p->gx + 0.5f, s);
        p->posY = lerpf(p->startY, p->gy + 0.5f, s);
        p->angle = lerpf(p->startAngle, p->targetAngle, s);
        if (p->t >= 1.0f) { p->phase = PH_IDLE; arrived = true; }

    } else if (p->phase == PH_DOOR_OUT) {
        p->t += dt / DOOR_TIME;
        if (p->t > 1.0f) p->t = 1.0f;
        float s = smooth(p->t);
        p->posX = lerpf(p->startX, p->creepX, s);
        p->posY = lerpf(p->startY, p->creepY, s);
        p->fade = s;
        if (p->t >= 1.0f) { // fully black: jump to the other side
            p->gx = p->doorDestX;
            p->gy = p->doorDestY;
            p->posX = p->gx + 0.5f;
            p->posY = p->gy + 0.5f;
            p->phase = PH_DOOR_IN;
            p->t = 0.0f;
            p->fade = 1.0f;
        }

    } else if (p->phase == PH_DOOR_IN) {
        p->t += dt / DOOR_TIME;
        if (p->t > 1.0f) p->t = 1.0f;
        p->fade = 1.0f - smooth(p->t);
        if (p->t >= 1.0f) { p->phase = PH_IDLE; p->fade = 0.0f; arrived = true; }
    }

    if (arrived) on_arrive(p, m);
}
