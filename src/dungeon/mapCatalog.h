// mapCatalog.h - finds and names the map images in assets/maps
//
// Map files are named l<layer>m<map>.png, e.g. l1m1.png = layer 1, map 1.
// "Layer" is the dungeon floor/stratum; later it will pick the texture set for that layer.
// Files that don't match the pattern are ignored.
#ifndef MAP_CATALOG_H
#define MAP_CATALOG_H

#include <stdbool.h>
#include <stddef.h>

#define MAP_DIR "assets/maps"
#define DATA_DIR "assets/data"

typedef struct {
    int layer;
    int map;
    char path[256];
    char label[64]; // e.g. "Layer 1 - Map 2  (l1m2.png)"
} MapEntry;

typedef struct {
    MapEntry *entries; // sorted by layer, then map; NULL when count == 0
    int count;
} MapCatalog;

// Scans MAP_DIR. Safe to call again later to pick up new files.
void mapCatalogScan(MapCatalog *catalog);
void mapCatalogFree(MapCatalog *catalog);

// Builds "assets/maps/l<layer>m<map>.png"
void mapCatalogPath(char *out, size_t size, int layer, int map);

// Builds "assets/data/l<layer>m<map>.txt" (the file that says what the map's special tiles do)
void mapCatalogDataPath(char *out, size_t size, int layer, int map);

// Does the map image for this layer and map exist?
bool mapCatalogExists(int layer, int map);

#endif
