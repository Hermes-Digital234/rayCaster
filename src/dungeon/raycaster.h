// raycaster.h - software renderer: floor, walls/doors and chest sprites into a framebuffer
#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "map.h"
#include "raylib.h"

#define RW 480 // internal render resolution (scaled up when drawn)
#define RH 300

typedef struct {
    Color *px;         // RW*RH framebuffer
    float zbuf[RW];    // wall distance per column, for sprite occlusion
    Texture2D tex;     // framebuffer uploaded for drawing

    // Chest textures (assets/textures/chestClosed.png and chestOpen.png) as CPU pixels.
    // NULL when a file is missing; the renderer then draws a plain colored box instead.
    Color *chestPx[2]; // [0] = closed, [1] = open
    int chestW[2], chestH[2];
} Raycaster;

// Call after InitWindow (it creates a texture). Loads the chest textures relative to the working directory.
void rcInit(Raycaster *rc);
void rcFree(Raycaster *rc);

// Render the view from (posX, posY) looking along `angle` (radians) into the framebuffer
void rcRender(Raycaster *rc, const Map *m, float posX, float posY, float angle);

// Draw the framebuffer scaled to the given size. Call between BeginDrawing/EndDrawing.
void rcDraw(const Raycaster *rc, int winW, int winH);

#endif