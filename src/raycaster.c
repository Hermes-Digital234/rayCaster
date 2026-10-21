// raycaster.c - all rendering. Reads the map, never changes it.
#include "raycaster.h"
#include <math.h>
#include <stdlib.h>

#define FOG_DIST 16.0f // distance at which things get dark; raise it for less fog

// Colors used when drawing the floor under each tile type
static const Color floorColor[T_COUNT] = {
    [T_FLOOR] = {235, 235, 235, 255},
    [T_WALL]  = {30, 30, 30, 255},
    [T_WATER] = {40, 90, 220, 255},
    [T_ITEM]  = {235, 235, 235, 255},
    [T_DOOR]  = {30, 30, 30, 255},
    [T_HEAL]  = {70, 200, 90, 255},
    [T_EVENT] = {255, 130, 190, 255},
};

static const Color ceilingColor = {45, 45, 65, 255};

static Color shade(Color c, float dist) {
    float f = 1.0f - dist / FOG_DIST;
    if (f < 0.15f) f = 0.15f;
    return (Color){(unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), 255};
}

static Color wall_color(int tile, bool side, float dist) {
    Color c;
    if (tile == T_DOOR) c = side ? (Color){150, 30, 30, 255} : (Color){210, 45, 45, 255};
    else                c = side ? (Color){105, 105, 105, 255} : (Color){150, 150, 150, 255};
    return shade(c, dist);
}

void rc_init(Raycaster *rc) {
    rc->px = malloc(RW * RH * sizeof(Color));
    for (int i = 0; i < RW * RH; i++) rc->px[i] = BLACK;

    Image fb = {.data = rc->px, .width = RW, .height = RH, .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    rc->tex = LoadTextureFromImage(fb);
    SetTextureFilter(rc->tex, TEXTURE_FILTER_POINT);
}

void rc_free(Raycaster *rc) {
    UnloadTexture(rc->tex);
    free(rc->px);
    rc->px = NULL;
}

static void render_floor_and_ceiling(Raycaster *rc, const Map *m, float posX, float posY,
                                     float dirX, float dirY, float planeX, float planeY) {
    Color *px = rc->px;
    for (int y = 0; y < RH; y++) {
        if (y < RH / 2) {
            for (int x = 0; x < RW; x++) px[y * RW + x] = ceilingColor;
            continue;
        }
        float rowDist = 0.5f * RH / (y - RH / 2 + 0.5f);
        float fx = posX + rowDist * (dirX - planeX);
        float fy = posY + rowDist * (dirY - planeY);
        float sx = rowDist * 2.0f * planeX / RW;
        float sy = rowDist * 2.0f * planeY / RW;

        Color rowCol[T_COUNT]; // shaded colors for this row, one per tile type
        for (int i = 0; i < T_COUNT; i++) rowCol[i] = shade(floorColor[i], rowDist);

        for (int x = 0; x < RW; x++) {
            px[y * RW + x] = rowCol[map_tile(m, (int)floorf(fx), (int)floorf(fy))];
            fx += sx;
            fy += sy;
        }
    }
}

// DDA, one ray per column. Water is NOT hit, so you can see past it.
static void render_walls(Raycaster *rc, const Map *m, float posX, float posY,
                         float dirX, float dirY, float planeX, float planeY) {
    for (int x = 0; x < RW; x++) {
        float camX = 2.0f * x / (float)RW - 1.0f;
        float rayX = dirX + planeX * camX;
        float rayY = dirY + planeY * camX;

        int mapX = (int)floorf(posX), mapY = (int)floorf(posY);
        float deltaX = (rayX == 0) ? 1e30f : fabsf(1.0f / rayX);
        float deltaY = (rayY == 0) ? 1e30f : fabsf(1.0f / rayY);

        int stepX, stepY;
        float sideX, sideY;
        if (rayX < 0) { stepX = -1; sideX = (posX - mapX) * deltaX; }
        else          { stepX =  1; sideX = (mapX + 1.0f - posX) * deltaX; }
        if (rayY < 0) { stepY = -1; sideY = (posY - mapY) * deltaY; }
        else          { stepY =  1; sideY = (mapY + 1.0f - posY) * deltaY; }

        bool sideHit = false;
        int tile = T_FLOOR;
        while (!map_blocks_ray(tile)) {
            if (sideX < sideY) { sideX += deltaX; mapX += stepX; sideHit = false; }
            else               { sideY += deltaY; mapY += stepY; sideHit = true;  }
            tile = map_tile(m, mapX, mapY);
        }

        float dist = sideHit ? (sideY - deltaY) : (sideX - deltaX);
        if (dist < 0.01f) dist = 0.01f;
        rc->zbuf[x] = dist;

        int lineH = (int)(RH / dist);
        int y0 = RH / 2 - lineH / 2, y1 = RH / 2 + lineH / 2;
        if (y0 < 0) y0 = 0;
        if (y1 > RH - 1) y1 = RH - 1;
        Color c = wall_color(tile, sideHit, dist);
        for (int y = y0; y <= y1; y++) rc->px[y * RW + x] = c;
    }
}

// Item boxes: billboard sprites, depth-tested against the wall distances
static void render_items(Raycaster *rc, const Map *m, float posX, float posY,
                         float dirX, float dirY, float planeX, float planeY) {
    float invDet = 1.0f / (planeX * dirY - dirX * planeY);

    for (int cy = 0; cy < m->h; cy++) {
        for (int cx = 0; cx < m->w; cx++) {
            if (map_tile(m, cx, cy) != T_ITEM) continue;

            float sx = cx + 0.5f - posX, sy = cy + 0.5f - posY;
            float tx = invDet * (dirY * sx - dirX * sy);
            float ty = invDet * (-planeY * sx + planeX * sy); // depth
            if (ty < 0.1f) continue;

            int screenX = (int)((RW / 2) * (1.0f + tx / ty));
            int size = (int)(RH / ty * 0.35f);             // box is 35% of a cell
            int bottom = RH / 2 + (int)(RH / (2.0f * ty)); // where the floor is at this depth
            int top = bottom - size;
            int left = screenX - size / 2, right = screenX + size / 2;

            for (int x = left; x < right; x++) {
                if (x < 0 || x >= RW || ty >= rc->zbuf[x]) continue;
                rc->zbuf[x] = ty;
                float u = (float)(x - left) / (float)size;
                for (int y = top; y < bottom; y++) {
                    if (y < 0 || y >= RH) continue;
                    float v = (float)(y - top) / (float)size;
                    Color c = {255, 215, 0, 255};
                    if (u < 0.08f || u > 0.92f || v > 0.92f) c = (Color){170, 130, 0, 255}; // edges
                    else if (fabsf(v - 0.3f) < 0.05f)        c = (Color){120, 80, 0, 255};  // lid line
                    rc->px[y * RW + x] = shade(c, ty);
                }
            }
        }
    }
}

void rc_render(Raycaster *rc, const Map *m, float posX, float posY, float angle) {
    // Camera: direction vector plus a perpendicular plane (length 0.66 gives ~66 degree FOV)
    float dirX = cosf(angle), dirY = sinf(angle);
    float planeX = sinf(angle) * 0.66f, planeY = -cosf(angle) * 0.66f;

    render_floor_and_ceiling(rc, m, posX, posY, dirX, dirY, planeX, planeY);
    render_walls(rc, m, posX, posY, dirX, dirY, planeX, planeY);
    render_items(rc, m, posX, posY, dirX, dirY, planeX, planeY);
}

void rc_draw(const Raycaster *rc, int winW, int winH) {
    UpdateTexture(rc->tex, rc->px);
    DrawTexturePro(rc->tex, (Rectangle){0, 0, RW, RH}, (Rectangle){0, 0, winW, winH},
                   (Vector2){0, 0}, 0.0f, WHITE);
}
