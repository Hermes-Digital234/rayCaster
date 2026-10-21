// STATE_DEV_MAP_SELECT: development tool that lists every map in assets/maps and loads the chosen one.
// The list is rescanned each time this state is entered.
#include "core/states/devMapSelect.h"

#include "core/input.h"
#include "core/menuList.h"
#include "core/stateManager.h"
#include "core/states/dungeonExplore.h"
#include "dungeon/mapCatalog.h"

#include <stdlib.h>

static MapCatalog catalog;
static const char **labels; // points into catalog.entries[i].label, as menuListDraw expects
static int selected;

static void init(void) {
    mapCatalogScan(&catalog);

    labels = malloc((catalog.count > 0 ? catalog.count : 1) * sizeof(const char *));
    for (int i = 0; i < catalog.count; i++) labels[i] = catalog.entries[i].label;

    selected = 0;
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();
    selected = menuListMove(selected, catalog.count, action);

    if (action == MENU_CONFIRM && catalog.count > 0) {
        const MapEntry *entry = &catalog.entries[selected];
        dungeonExploreSelectMap(entry->layer, entry->map);
        stateManagerPush(STATE_DUNGEON_EXPLORE); // Esc in the map returns here
    } else if (action == MENU_BACK) {
        stateManagerPop();
    }
}

static void draw(void) {
    menuListDraw("Development: select map", labels, catalog.count, selected,
                 "No maps found in " MAP_DIR " (expected files like l1m1.png)",
                 "Up/Down: select    Enter: load map    Esc: back");
}

static void unload(void) {
    free(labels);
    labels = NULL;
    mapCatalogFree(&catalog);
}

void devMapSelectRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
        .unload = unload,
    };
    stateManagerRegister(STATE_DEV_MAP_SELECT, h);
}
