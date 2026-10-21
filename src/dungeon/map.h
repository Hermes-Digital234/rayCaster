// map.h - tile map loaded from an image (1 pixel = 1 tile)
//
// Pixel colors (matched to the NEAREST palette color, so exact shades don't matter):
//   white   = floor
//   black   = wall
//   blue    = water (not walkable, but see-through)
//   yellow  = treasure chest (face it and press the interact key; it stays in place as an open chest)
//   red     = door
//   purple  = stairs up   (goes to the previous map of the same layer)
//   orange  = stairs down (goes to the next map of the same layer)
//   magenta = hidden skip: looks like a wall, can be walked through (see tileData.h for the rules)
//   green   = healing spot (floor tile only, no effect yet)
//   pink    = random event (floor tile; what happens is defined in the map's data file)
#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

typedef enum {
    T_FLOOR,
    T_WALL,
    T_WATER,
    T_ITEM,
    T_DOOR,
    T_HEAL,
    T_EVENT,
    T_STAIRS_UP,
    T_STAIRS_DOWN,
    T_SKIP,
    T_CHEST_OPEN, // never comes from a pixel: a chest becomes this once opened (still blocks the player)
    T_COUNT
} Tile;

typedef struct {
    int w, h;
    unsigned char *tiles; // w*h entries, row-major
} Map;

bool mapLoad(Map *m, const char *path); // false if the image can't be loaded
void mapFree(Map *m);

int  mapTile(const Map *m, int x, int y);          // out of bounds counts as T_WALL
void mapSet(Map *m, int x, int y, int tile);

bool mapBlocksRay(int tile);      // drawn as a solid wall: walls, doors, stairs and skips. Water and chests are not.
bool mapWalkable(int tile);       // tiles the player may simply step onto
bool mapIsInteractable(int tile); // tiles you can interact with from the next cell (door, stairs, skip, closed chest)

// First tile of the given type in row-major order. Returns false if there is none.
bool mapFindTile(const Map *m, int tile, int *x, int *y);

// First floor tile in row-major order (where the player starts). Returns false if there is none.
bool mapFindStart(const Map *m, int *x, int *y);

#endif