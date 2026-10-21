// STATE_PAUSE_MENU: Resume / Party / Inventory / Save Game / Load Game / Leave Dungeon. Sits on top of the dungeon state.
#include "core/states/pauseMenu.h"

#include "core/input.h"
#include "core/menuList.h"
#include "core/save.h"
#include "core/stateManager.h"
#include "core/states/dungeonExplore.h"
#include "core/states/interactPrompt.h"
#include "core/states/saveLoadMenu.h"

#include <stddef.h>

typedef enum { PAUSE_RESUME, PAUSE_PARTY, PAUSE_INVENTORY, PAUSE_SAVE, PAUSE_LOAD, PAUSE_LEAVE, PAUSE_COUNT } PauseItem;
static const char *const labels[PAUSE_COUNT] = {"Resume", "Party", "Inventory", "Save Game", "Load Game", "Leave Dungeon"};

static int selected;
static bool askingToLeave;

static void init(void) {
    selected = PAUSE_RESUME;
    askingToLeave = false;
}

static void resume(void) {
    // Back from the load menu: if a save was loaded, move the dungeon there and close the pause menu
    SaveLocation location;
    if (saveLoadMenuTakeLoaded(&location)) {
        if (dungeonExploreLoadLocation(&location)) stateManagerPop();
        return;
    }

    // Back from the "leave?" question
    if (askingToLeave) {
        askingToLeave = false;
        if (interactPromptAccepted()) {
            stateManagerPop(); // this menu
            stateManagerPop(); // the dungeon
        }
    }
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();
    selected = menuListMove(selected, PAUSE_COUNT, action);

    if (action == MENU_BACK) {
        stateManagerPop();
        return;
    }
    if (action != MENU_CONFIRM) return;

    switch (selected) {
        case PAUSE_RESUME:
            stateManagerPop();
            break;
        case PAUSE_PARTY:
            stateManagerPush(STATE_STATUS_MENU);
            break;
        case PAUSE_INVENTORY:
            stateManagerPush(STATE_INVENTORY_MENU);
            break;
        case PAUSE_SAVE: {
            SaveLocation here;
            if (dungeonExploreGetLocation(&here)) saveLoadMenuOpenSave(&here);
            break;
        }
        case PAUSE_LOAD:
            saveLoadMenuOpenLoad();
            break;
        case PAUSE_LEAVE:
            askingToLeave = true;
            interactPromptOpen("Leave the dungeon? Progress since your last save is lost.", PROMPT_YES_NO);
            break;
    }
}

static void draw(void) {
    menuListDraw("Paused", labels, PAUSE_COUNT, selected, NULL, "Up/Down: select    Enter: confirm    Esc: resume");
}

void pauseMenuRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
        .resume = resume,
    };
    stateManagerRegister(STATE_PAUSE_MENU, h);
}