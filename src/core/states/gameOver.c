#include "core/states/gameOver.h"

#include "core/input.h"
#include "core/menuList.h"
#include "core/save.h"
#include "core/stateManager.h"
#include "core/states/dungeonExplore.h"
#include "core/states/saveLoadMenu.h"

#include <stddef.h>

typedef enum { OVER_LOAD, OVER_TITLE, OVER_COUNT } OverItem;
static const char *const labels[OVER_COUNT] = {"Load Game", "Return to Title"};

static int selected;

static void init(void) {
    selected = OVER_LOAD;
}

static void resume(void) {
    SaveLocation location;
    if (saveLoadMenuTakeLoaded(&location)) {
        if (dungeonExploreLoadLocation(&location)) stateManagerPop(); // close this screen, the dungeon is already there
    }
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();
    selected = menuListMove(selected, OVER_COUNT, action);
    if (action != MENU_CONFIRM) return; // Esc does nothing here: the player has to choose

    if (selected == OVER_LOAD) saveLoadMenuOpenLoad();
    else                       stateManagerPopTo(STATE_MAIN_MENU);
}

static void draw(void) {
    menuListDraw("GAME OVER", labels, OVER_COUNT, selected, NULL, "Up/Down: select    Enter: confirm");
}

void gameOverRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
        .resume = resume,
    };
    stateManagerRegister(STATE_GAME_OVER, h);
}
