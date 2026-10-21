// raycaster.c - all rendering. Reads the map, never changes it.
#include "raycaster.h"
#include <math.h>
#include <stdlib.h>

#define FOG_DIST 16.0f // distance at which things get dark; raise it for less fog
#define CHEST_SIZE 0.6f // sprite width/height as a fraction of a cell
#define MAX_CHESTS 256

#define CHEST_CLOSED_FILE "assets/textures/chestClosed.png"
#define CHEST_OPEN_FILE   "assets/textures/chestOpen.png"

// Colors used when drawing the floor under each tile type
static const Color floorColor[T_COUNT] = {
    [T_FLOOR] = {235, 235, 235, 255},
    [T_WALL]  = {30, 30, 30, 255},
    [T_WATER] = {40, 90, 220, 255},
    [T_ITEM]  = {235, 235, 235, 255},
    [T_DOOR]  = {30, 30, 30, 255},
    [T_HEAL]  = {70, 200, 90, 255},
    [T_EVENT] = {255, 130, 190, 255},
    [T_STAIRS_UP]   = {30, 30, 30, 255},
    [T_STAIRS_DOWN] = {30, 30, 30, 255},
    [T_SKIP]        = {30, 30, 30, 255},
    [T_CHEST_OPEN]  = {235, 235, 235, 255},
};

static const Color ceilingColor = {45, 45, 65, 255};

static Color shade(Color c, float dist) {
    float f = 1.0f - dist / FOG_DIST;
    if (f < 0.15f) f = 0.15f;
    return (Color){(unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), 255};
}

static Color wallColor(int tile, bool side, float dist) {
    Color c;
    switch (tile) {
        case T_DOOR:
            c = side ? (Color){150, 30, 30, 255} : (Color){210, 45, 45, 255};
            break;
        case T_STAIRS_UP:
            c = side ? (Color){105, 48, 160, 255} : (Color){150, 70, 230, 255};
            break;
        case T_STAIRS_DOWN:
            c = side ? (Color){168, 105, 20, 255} : (Color){240, 150, 30, 255};
            break;
        default: // T_WALL, and T_SKIP on purpose: a hidden skip must look exactly like a wall
            c = side ? (Color){105, 105, 105, 255} : (Color){150, 150, 150, 255};
            break;
    }
    return shade(c, dist);
}

static void loadChestTexture(Raycaster *rc, int index, const char *path) {
    rc->chestPx[index] = NULL;
    rc->chestW[index] = rc->chestH[index] = 0;

    Image img = LoadImage(path);
    if (img.data == NULL) {
        TraceLog(LOG_WARNING, "raycaster: could not load '%s', drawing a plain box instead", path);
        return;
    }
    Color *colors = LoadImageColors(img);
    size_t bytes = (size_t)img.width * img.height * sizeof(Color);
    rc->chestPx[index] = malloc(bytes);
    if (rc->chestPx[index]) {
        for (int i = 0; i < img.width * img.height; i++) rc->chestPx[index][i] = colors[i];
        rc->chestW[index] = img.width;
        rc->chestH[index] = img.height;
    }
    UnloadImageColors(colors);
    UnloadImage(img);
}

void rcInit(Raycaster *rc) {
    rc->px = malloc(RW * RH * sizeof(Color));
    for (int i = 0; i < RW * RH; i++) rc->px[i] = BLACK;
    loadChestTexture(rc, 0, CHEST_CLOSED_FILE);
    loadChestTexture(rc, 1, CHEST_OPEN_FILE);

    Image fb = {.data = rc->px, .width = RW, .height = RH, .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    rc->tex = LoadTextureFromImage(fb);
    SetTextureFilter(rc->tex, TEXTURE_FILTER_POINT);
}

void rcFree(Raycaster *rc) {
    for (int i = 0; i < 2; i++) {
        free(rc->chestPx[i]);
        rc->chestPx[i] = NULL;
    }
    UnloadTexture(rc->tex);
    free(rc->px);
    rc->px = NULL;
}

static void renderFloorAndCeiling(Raycaster *rc, const Map *m, float posX, float posY,
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
            px[y * RW + x] = rowCol[mapTile(m, (int)floorf(fx), (int)floorf(fy))];
            fx += sx;
            fy += sy;
        }
    }
}

// DDA, one ray per column. Water is NOT hit, so you can see past it.
static void renderWalls(Raycaster *rc, const Map *m, float posX, float posY,
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
        while (!mapBlocksRay(tile)) {
            if (sideX < sideY) { sideX += deltaX; mapX += stepX; sideHit = false; }
            else               { sideY += deltaY; mapY += stepY; sideHit = true;  }
            tile = mapTile(m, mapX, mapY);
        }

        float dist = sideHit ? (sideY - deltaY) : (sideX - deltaX);
        if (dist < 0.01f) dist = 0.01f;
        rc->zbuf[x] = dist;

        int lineH = (int)(RH / dist);
        int y0 = RH / 2 - lineH / 2, y1 = RH / 2 + lineH / 2;
        if (y0 < 0) y0 = 0;
        if (y1 > RH - 1) y1 = RH - 1;
        Color c = wallColor(tile, sideHit, dist);
        for (int y = y0; y <= y1; y++) rc->px[y * RW + x] = c;
    }
}

// The color of the chest sprite at (u, v), both 0..1 with v = 0 at the top.
// Returns false where the sprite is transparent.
static bool chestPixel(const Raycaster *rc, bool open, float u, float v, Color *out) {
    int i = open ? 1 : 0;
    if (rc->chestPx[i]) {
        int tx = (int)(u * rc->chestW[i]), ty = (int)(v * rc->chestH[i]);
        if (tx < 0) tx = 0;
        if (ty < 0) ty = 0;
        if (tx >= rc->chestW[i]) tx = rc->chestW[i] - 1;
        if (ty >= rc->chestH[i]) ty = rc->chestH[i] - 1;
        Color c = rc->chestPx[i][ty * rc->chestW[i] + tx];
        if (c.a < 128) return false;
        *out = c;
        return true;
    }

    // Fallback when the texture file is missing: a plain box, gold when closed and dark when open
    Color c = open ? (Color){110, 80, 40, 255} : (Color){255, 215, 0, 255};
    if (u < 0.08f || u > 0.92f || v > 0.92f) c = (Color){170, 130, 0, 255};
    *out = c;
    return true;
}

typedef struct {
    int cx, cy;
    float depth;
    bool open;
} ChestSprite;

static int compareFarFirst(const void *a, const void *b) {
    float da = ((const ChestSprite *)a)->depth, db = ((const ChestSprite *)b)->depth;
    return (da < db) - (da > db);
}

// Chests (closed and open): billboard sprites, depth-tested against the walls and drawn far to near
static void renderChests(Raycaster *rc, const Map *m, float posX, float posY,
                          float dirX, float dirY, float planeX, float planeY) {
    float invDet = 1.0f / (planeX * dirY - dirX * planeY);

    ChestSprite sprites[MAX_CHESTS];
    int count = 0;
    for (int cy = 0; cy < m->h && count < MAX_CHESTS; cy++) {
        for (int cx = 0; cx < m->w && count < MAX_CHESTS; cx++) {
            int tile = mapTile(m, cx, cy);
            if (tile != T_ITEM && tile != T_CHEST_OPEN) continue;

            float sx = cx + 0.5f - posX, sy = cy + 0.5f - posY;
            float depth = invDet * (-planeY * sx + planeX * sy);
            if (depth < 0.1f) continue;
            sprites[count++] = (ChestSprite){cx, cy, depth, tile == T_CHEST_OPEN};
        }
    }
    qsort(sprites, (size_t)count, sizeof(ChestSprite), compareFarFirst);

    for (int i = 0; i < count; i++) {
        float sx = sprites[i].cx + 0.5f - posX, sy = sprites[i].cy + 0.5f - posY;
        float tx = invDet * (dirY * sx - dirX * sy);
        float ty = sprites[i].depth;

        int screenX = (int)((RW / 2) * (1.0f + tx / ty));
        int size = (int)(RH / ty * CHEST_SIZE);
        if (size < 1) continue;
        int bottom = RH / 2 + (int)(RH / (2.0f * ty)); // where the floor is at this depth
        int top = bottom - size;
        int left = screenX - size / 2, right = left + size;

        for (int x = left; x < right; x++) {
            if (x < 0 || x >= RW || ty >= rc->zbuf[x]) continue;
            float u = (float)(x - left) / (float)size;
            for (int y = top; y < bottom; y++) {
                if (y < 0 || y >= RH) continue;
                float v = (float)(y - top) / (float)size;
                Color c;
                if (chestPixel(rc, sprites[i].open, u, v, &c)) rc->px[y * RW + x] = shade(c, ty);
            }
        }
    }
}

void rcRender(Raycaster *rc, const Map *m, float posX, float posY, float angle) {
    // Camera: direction vector plus a perpendicular plane (length 0.66 gives ~66 degree FOV)
    float dirX = cosf(angle), dirY = sinf(angle);
    float planeX = sinf(angle) * 0.66f, planeY = -cosf(angle) * 0.66f;

    renderFloorAndCeiling(rc, m, posX, posY, dirX, dirY, planeX, planeY);
    renderWalls(rc, m, posX, posY, dirX, dirY, planeX, planeY);
    renderChests(rc, m, posX, posY, dirX, dirY, planeX, planeY);
}

void rcDraw(const Raycaster *rc, int winW, int winH) {
    UpdateTexture(rc->tex, rc->px);
    DrawTexturePro(rc->tex, (Rectangle){0, 0, RW, RH}, (Rectangle){0, 0, winW, winH},
                   (Vector2){0, 0}, 0.0f, WHITE);
}
