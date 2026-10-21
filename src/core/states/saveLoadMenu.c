#include "core/states/saveLoadMenu.h"

#include "core/input.h"
#include "core/menuList.h"
#include "core/stateManager.h"
#include "core/states/interactPrompt.h"
#include "raylib.h"

#include <stdio.h>
#include <time.h>

typedef enum { MODE_SAVE, MODE_LOAD } Mode;

static Mode mode;
static SaveLocation saveHere;     // MODE_SAVE: what to write
static int selected;
static int pendingSlot = -1;      // slot waiting for the "overwrite?" answer
static bool loaded;
static SaveLocation loadedLocation;

static char labelText[SAVE_SLOTS][112];
static const char *labels[SAVE_SLOTS];

static void refreshLabels(void) {
    for (int i = 0; i < SAVE_SLOTS; i++) {
        SaveInfo info = saveInfo(i);
        if (!info.exists) {
            snprintf(labelText[i], sizeof(labelText[i]), "Slot %d   - empty -", i + 1);
        } else {
            char when[32] = "";
            time_t t = (time_t)info.savedAt;
            struct tm *local = localtime(&t);
            if (local) strftime(when, sizeof(when), "%Y-%m-%d %H:%M", local);
            snprintf(labelText[i], sizeof(labelText[i]), "Slot %d   Layer %d - Map %d   %s", i + 1, info.layer, info.map, when);
        }
        labels[i] = labelText[i];
    }
}

void saveLoadMenuOpenSave(const SaveLocation *here) {
    mode = MODE_SAVE;
    saveHere = *here;
    stateManagerPush(STATE_SAVE_LOAD);
}

void saveLoadMenuOpenLoad(void) {
    mode = MODE_LOAD;
    loaded = false;
    stateManagerPush(STATE_SAVE_LOAD);
}

bool saveLoadMenuTakeLoaded(SaveLocation *out) {
    if (!loaded) return false;
    loaded = false;
    *out = loadedLocation;
    return true;
}

static void doSave(int slot) {
    if (saveWrite(slot, &saveHere)) {
        refreshLabels();
        interactPromptOpen(TextFormat("Game saved to slot %d.", slot + 1), PROMPT_OK);
    } else {
        interactPromptOpen("The save file could not be written.", PROMPT_OK);
    }
}

static void init(void) {
    selected = 0;
    pendingSlot = -1;
    refreshLabels();
}

static void resume(void) {
    if (pendingSlot >= 0) {
        int slot = pendingSlot;
        pendingSlot = -1;
        if (interactPromptAccepted()) doSave(slot);
    }
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();
    selected = menuListMove(selected, SAVE_SLOTS, action);

    if (action == MENU_BACK) {
        stateManagerPop();
        return;
    }
    if (action != MENU_CONFIRM) return;

    SaveInfo info = saveInfo(selected);

    if (mode == MODE_LOAD) {
        if (!info.exists) {
            interactPromptOpen("This slot is empty.", PROMPT_OK);
        } else if (saveRead(selected, &loadedLocation)) {
            loaded = true;
            stateManagerPop();
        } else {
            interactPromptOpen("This save could not be read.", PROMPT_OK);
        }
        return;
    }

    if (info.exists) {
        pendingSlot = selected;
        interactPromptOpen(TextFormat("Overwrite slot %d?", selected + 1), PROMPT_YES_NO);
    } else {
        doSave(selected);
    }
}

static void draw(void) {
    menuListDraw(mode == MODE_SAVE ? "Save Game" : "Load Game", labels, SAVE_SLOTS, selected, NULL,
                 "Up/Down: select    Enter: confirm    Esc: back");
}

void saveLoadMenuRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
        .resume = resume,
    };
    stateManagerRegister(STATE_SAVE_LOAD, h);
}
