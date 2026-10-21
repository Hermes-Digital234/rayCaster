// STATE_MAIN_MENU: Start Game / Development / Quit
#include "core/states/mainMenu.h"

#include "core/input.h"
#include "core/menuList.h"
#include "core/stateManager.h"
#include "core/states/dungeonExplore.h"

#include <stddef.h>

#define GAME_TITLE "RPG ENGINE"

typedef enum { ITEM_START, ITEM_DEVELOPMENT, ITEM_QUIT, ITEM_COUNT } MenuItem;
static const char *const labels[ITEM_COUNT] = {"Start Game", "Development", "Quit"};

static int selected;

static void init(void) {
    selected = ITEM_START;
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();
    selected = menuListMove(selected, ITEM_COUNT, action);

    if (action != MENU_CONFIRM) return;

    switch (selected) {
        case ITEM_START:
            dungeonExploreSelectMap(1, 1); // layer 1, map 1 until there is a real new-game flow
            stateManagerPush(STATE_DUNGEON_EXPLORE);
            break;
        case ITEM_DEVELOPMENT:
            stateManagerPush(STATE_DEV_MAP_SELECT);
            break;
        case ITEM_QUIT:
            stateManagerPop(); // the last state leaving ends the game
            break;
    }
}

static void draw(void) {
    menuListDraw(GAME_TITLE, labels, ITEM_COUNT, selected, NULL, "Up/Down: select    Enter: confirm");
}

void mainMenuRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
    };
    stateManagerRegister(STATE_MAIN_MENU, h);
}
