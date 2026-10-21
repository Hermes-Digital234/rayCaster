// raycaster.h - software renderer: floor, walls/doors and item sprites into a framebuffer
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
} Raycaster;

// Call after InitWindow (it creates a texture)
void rc_init(Raycaster *rc);
void rc_free(Raycaster *rc);

// Render the view from (posX, posY) looking along `angle` (radians) into the framebuffer
void rc_render(Raycaster *rc, const Map *m, float posX, float posY, float angle);

// Draw the framebuffer scaled to the given size. Call between BeginDrawing/EndDrawing.
void rc_draw(const Raycaster *rc, int winW, int winH);

#endif
