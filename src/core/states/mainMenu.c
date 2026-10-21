// STATE_MAIN_MENU: Start Game / Load Game / Development / Quit
#include "core/states/mainMenu.h"

#include "core/game.h"
#include "core/input.h"
#include "core/menuList.h"
#include "core/save.h"
#include "core/stateManager.h"
#include "core/states/dungeonExplore.h"
#include "core/states/saveLoadMenu.h"

#include <stddef.h>

#define GAME_TITLE "RPG ENGINE"

typedef enum { MAIN_START, MAIN_LOAD, MAIN_DEVELOPMENT, MAIN_QUIT, MAIN_COUNT } MainItem;
static const char *const labels[MAIN_COUNT] = {"Start Game", "Load Game", "Development", "Quit"};

static int selected;

static void init(void) {
    selected = MAIN_START;
}

// Back from the load menu: if a save was loaded, enter the dungeon where it was saved
static void resume(void) {
    SaveLocation location;
    if (saveLoadMenuTakeLoaded(&location)) dungeonExploreLoadLocation(&location);
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();
    selected = menuListMove(selected, MAIN_COUNT, action);

    if (action != MENU_CONFIRM) return;

    switch (selected) {
        case MAIN_START:
            gameNewGame(); // fresh party, no items, untouched maps
            dungeonExploreSelectMap(1, 1); // layer 1, map 1 until there is a real new-game flow
            stateManagerPush(STATE_DUNGEON_EXPLORE);
            break;
        case MAIN_LOAD:
            saveLoadMenuOpenLoad();
            break;
        case MAIN_DEVELOPMENT:
            stateManagerPush(STATE_DEV_MAP_SELECT);
            break;
        case MAIN_QUIT:
            stateManagerPop(); // the last state leaving ends the game
            break;
    }
}

static void draw(void) {
    menuListDraw(GAME_TITLE, labels, MAIN_COUNT, selected, NULL, "Up/Down: select    Enter: confirm");
}

void mainMenuRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
        .resume = resume,
    };
    stateManagerRegister(STATE_MAIN_MENU, h);
}
