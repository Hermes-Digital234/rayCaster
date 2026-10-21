// map.h - tile map loaded from an image (1 pixel = 1 tile)
//
// Pixel colors (matched to the NEAREST palette color, so exact shades don't matter):
//   white = floor, black = wall, blue = water (not walkable, see-through),
//   yellow = item box, red = door, green = healing spot, pink = random event
#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

typedef enum { T_FLOOR, T_WALL, T_WATER, T_ITEM, T_DOOR, T_HEAL, T_EVENT, T_COUNT } Tile;

typedef struct {
    int w, h;
    unsigned char *tiles; // w*h entries, row-major
} Map;

bool map_load(Map *m, const char *path); // false if the image can't be loaded
void map_free(Map *m);

int  map_tile(const Map *m, int x, int y);          // out of bounds counts as T_WALL
void map_set(Map *m, int x, int y, int tile);

bool map_blocks_ray(int tile); // walls and doors are solid to the raycaster; water is not
bool map_walkable(int tile);   // tiles the player may step onto

// First floor tile in row-major order. Returns false if there is none.
bool map_find_start(const Map *m, int *x, int *y);

#endif
