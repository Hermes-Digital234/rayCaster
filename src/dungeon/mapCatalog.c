#include "dungeon/mapCatalog.h"
#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Accepts exactly "l<number>m<number>.png" with both numbers >= 1
static bool parseMapName(const char *name, int *layer, int *map) {
    *layer = *map = 0;
    int consumed = 0; // %n is only reached if the whole ".png" literal matched
    int matched = sscanf(name, "l%dm%d.png%n", layer, map, &consumed);
    return matched == 2 && consumed > 0 && name[consumed] == '\0' && *layer >= 1 && *map >= 1;
}

static int compareEntries(const void *a, const void *b) {
    const MapEntry *x = a, *y = b;
    if (x->layer != y->layer) return x->layer < y->layer ? -1 : 1; // numeric, so l2 sorts before l10
    if (x->map != y->map)     return x->map < y->map ? -1 : 1;
    return 0;
}

void mapCatalogScan(MapCatalog *catalog) {
    catalog->entries = NULL;
    catalog->count = 0;

    FilePathList files = LoadDirectoryFilesEx(MAP_DIR, ".png", false);
    if (files.count > 0) catalog->entries = malloc(files.count * sizeof(MapEntry));

    for (unsigned i = 0; i < files.count && catalog->entries; i++) {
        const char *name = GetFileName(files.paths[i]);
        int layer, map;
        if (!parseMapName(name, &layer, &map)) {
            TraceLog(LOG_WARNING, "mapCatalog: ignoring '%s' (expected l<layer>m<map>.png)", name);
            continue;
        }

        MapEntry *entry = &catalog->entries[catalog->count++];
        entry->layer = layer;
        entry->map = map;
        snprintf(entry->path, sizeof(entry->path), "%s", files.paths[i]);
        snprintf(entry->label, sizeof(entry->label), "Layer %d - Map %d  (%s)", layer, map, name);
    }
    UnloadDirectoryFiles(files);

    if (catalog->count > 1) qsort(catalog->entries, catalog->count, sizeof(MapEntry), compareEntries);
}

void mapCatalogFree(MapCatalog *catalog) {
    free(catalog->entries);
    catalog->entries = NULL;
    catalog->count = 0;
}

void mapCatalogPath(char *out, size_t size, int layer, int map) {
    snprintf(out, size, MAP_DIR "/l%dm%d.png", layer, map);
}

void mapCatalogDataPath(char *out, size_t size, int layer, int map) {
    snprintf(out, size, DATA_DIR "/l%dm%d.txt", layer, map);
}

bool mapCatalogExists(int layer, int map) {
    char path[256];
    mapCatalogPath(path, sizeof(path), layer, map);
    return FileExists(path);
}
