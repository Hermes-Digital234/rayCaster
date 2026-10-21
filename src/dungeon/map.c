// map.c - loading the map image and querying tiles
#include "map.h"
#include "raylib.h"
#include <stdlib.h>

// Colors the map image is matched against
static const Color palette[T_COUNT] = {
    [T_FLOOR]       = {255, 255, 255, 255},
    [T_WALL]        = {0, 0, 0, 255},
    [T_WATER]       = {0, 0, 255, 255},
    [T_ITEM]        = {255, 255, 0, 255},
    [T_DOOR]        = {255, 0, 0, 255},
    [T_HEAL]        = {0, 255, 0, 255},
    [T_EVENT]       = {255, 128, 192, 255},
    [T_STAIRS_UP]   = {128, 0, 255, 255},
    [T_STAIRS_DOWN] = {255, 165, 0, 255},
    [T_SKIP]        = {255, 0, 255, 255},
};

static int classify(Color c) {
    int best = 0, bestD = 1 << 30;
    for (int i = 0; i < T_CHEST_OPEN; i++) { // T_CHEST_OPEN has no pixel color
        int dr = c.r - palette[i].r, dg = c.g - palette[i].g, db = c.b - palette[i].b;
        int d = dr * dr + dg * dg + db * db;
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

bool mapLoad(Map *m, const char *path) {
    Image img = LoadImage(path);
    if (img.data == NULL) return false;

    m->w = img.width;
    m->h = img.height;
    m->tiles = malloc((size_t)m->w * m->h);

    Color *colors = LoadImageColors(img);
    for (int i = 0; i < m->w * m->h; i++) m->tiles[i] = (unsigned char)classify(colors[i]);
    UnloadImageColors(colors);
    UnloadImage(img);
    return true;
}

void mapFree(Map *m) {
    free(m->tiles);
    m->tiles = NULL;
    m->w = m->h = 0;
}

int mapTile(const Map *m, int x, int y) {
    if (x < 0 || y < 0 || x >= m->w || y >= m->h) return T_WALL;
    return m->tiles[y * m->w + x];
}

void mapSet(Map *m, int x, int y, int tile) {
    if (x < 0 || y < 0 || x >= m->w || y >= m->h) return;
    m->tiles[y * m->w + x] = (unsigned char)tile;
}

bool mapBlocksRay(int tile) {
    return tile == T_WALL || tile == T_DOOR || tile == T_STAIRS_UP || tile == T_STAIRS_DOWN || tile == T_SKIP;
}

bool mapWalkable(int tile) {
    return tile == T_FLOOR || tile == T_HEAL || tile == T_EVENT;
}

bool mapIsInteractable(int tile) {
    return tile == T_DOOR || tile == T_STAIRS_UP || tile == T_STAIRS_DOWN || tile == T_SKIP || tile == T_ITEM;
}

bool mapFindTile(const Map *m, int tile, int *x, int *y) {
    for (int i = 0; i < m->w * m->h; i++) {
        if (m->tiles[i] == tile) {
            *x = i % m->w;
            *y = i / m->w;
            return true;
        }
    }
    return false;
}

bool mapFindStart(const Map *m, int *x, int *y) {
    return mapFindTile(m, T_FLOOR, x, y);
}