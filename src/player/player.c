// player.c - movement rules and animation. Knows nothing about keys, drawing or what special tiles do.
#include "player.h"
#include "raylib.h" // PI
#include <math.h>

#define STEP_TIME 0.18f // seconds per move/turn
#define FADE_TIME 0.40f // seconds for each half of a fade transition

static float lerp(float a, float b, float t) { return a + (b - a) * t; }
static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

void playerInit(Player *p, const Map *m, int gx, int gy) {
    p->gx = gx;
    p->gy = gy;

    // Face the first open direction
    p->targetAngle = 0.0f;
    for (int k = 0; k < 4; k++) {
        float a = k * PI / 2;
        if (mapWalkable(mapTile(m, gx + (int)lroundf(cosf(a)), gy + (int)lroundf(sinf(a))))) {
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
    p->phase = PH_IDLE;
    p->t = 0.0f;
    p->fade = 0.0f;
    p->arrived = false;
}

void playerFacing(const Player *p, int *dirX, int *dirY) {
    *dirX = (int)lroundf(cosf(p->targetAngle));
    *dirY = (int)lroundf(sinf(p->targetAngle));
}

void playerPlace(Player *p, int gx, int gy, int faceDx, int faceDy) {
    p->gx = gx;
    p->gy = gy;
    p->posX = gx + 0.5f;
    p->posY = gy + 0.5f;
    p->targetAngle = atan2f((float)faceDy, (float)faceDx); // (1,0)=0, (0,1)=90deg, (-1,0)=180deg, (0,-1)=-90deg
    p->angle = p->targetAngle;
    p->startX = p->posX;
    p->startY = p->posY;
    p->startAngle = p->angle;
}

// Decide what an action does, and start the matching animation
static void beginAction(Player *p, const Map *m, Action act) {
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
    int tile = mapTile(m, nx, ny);

    if (mapWalkable(tile)) {
        p->gx = nx;
        p->gy = ny;
        p->phase = PH_MOVE;
        p->t = 0.0f;
    }
    // anything else (walls, doors, water, chests...) simply blocks the move
}

void playerUpdate(Player *p, const Map *m, Action act, float dt) {
    p->arrived = false;

    if (p->phase == PH_IDLE) beginAction(p, m, act);

    switch (p->phase) {
        case PH_MOVE:
            p->t += dt / STEP_TIME;
            if (p->t > 1.0f) p->t = 1.0f;
            {
                float s = smooth(p->t);
                p->posX = lerp(p->startX, p->gx + 0.5f, s);
                p->posY = lerp(p->startY, p->gy + 0.5f, s);
                p->angle = lerp(p->startAngle, p->targetAngle, s);
            }
            if (p->t >= 1.0f) { p->phase = PH_IDLE; p->arrived = true; }
            break;

        case PH_FADE_OUT:
            p->t += dt / FADE_TIME;
            if (p->t > 1.0f) p->t = 1.0f;
            {
                float s = smooth(p->t);
                p->posX = lerp(p->startX, p->creepX, s);
                p->posY = lerp(p->startY, p->creepY, s);
                p->fade = s;
            }
            if (p->t >= 1.0f) { p->phase = PH_BLACK; p->fade = 1.0f; }
            break;

        case PH_BLACK:
            break; // waiting for the owner to call playerEndBlack

        case PH_FADE_IN:
            p->t += dt / FADE_TIME;
            if (p->t > 1.0f) p->t = 1.0f;
            p->fade = 1.0f - smooth(p->t);
            if (p->t >= 1.0f) { p->phase = PH_IDLE; p->fade = 0.0f; p->arrived = true; }
            break;

        case PH_IDLE:
            break;
    }
}

void playerBeginFade(Player *p, int dirX, int dirY) {
    if (p->phase != PH_IDLE) return;
    p->startX = p->posX;
    p->startY = p->posY;
    p->creepX = p->gx + 0.5f + dirX * 0.3f;
    p->creepY = p->gy + 0.5f + dirY * 0.3f;
    p->phase = PH_FADE_OUT;
    p->t = 0.0f;
}

void playerTeleport(Player *p, int gx, int gy) {
    p->gx = gx;
    p->gy = gy;
    p->posX = gx + 0.5f;
    p->posY = gy + 0.5f;
    p->startX = p->posX;
    p->startY = p->posY;
}

void playerEndBlack(Player *p) {
    if (p->phase != PH_BLACK) return;
    p->phase = PH_FADE_IN;
    p->t = 0.0f;
    p->fade = 1.0f;
}
