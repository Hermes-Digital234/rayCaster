// main.c - window and main loop. Everything else happens inside states.
#include "core/stateManager.h"
#include "core/game.h"
#include "core/states/battleMain.h"
#include "core/states/devMapSelect.h"
#include "core/states/dungeonExplore.h"
#include "core/states/gameOver.h"
#include "core/states/interactPrompt.h"
#include "core/states/mainMenu.h"
#include "core/states/pauseMenu.h"
#include "core/states/saveLoadMenu.h"
#include "core/states/statusMenu.h"
#include "core/states/inventoryMenu.h"
#include "raylib.h"
#include <stdlib.h>
#include <time.h>

#define WIN_W 960
#define WIN_H 600

int main(void) {
    // assets/ is copied next to the executable, so run from there no matter where we were launched
    ChangeDirectory(GetApplicationDirectory());

    InitWindow(WIN_W, WIN_H, "rpg engine");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Esc means "back" in the game, not "close the window"

    srand((unsigned)time(NULL)); // random events

    stateManagerInit();
    mainMenuRegister();
    devMapSelectRegister();
    dungeonExploreRegister();
    interactPromptRegister();
    battleMainRegister();
    pauseMenuRegister();
    statusMenuRegister();
    inventoryMenuRegister();
    saveLoadMenuRegister();
    gameOverRegister();

    gameLoadData(); // items and enemies from assets/data
    gameNewGame();  // the starting party from assets/data/party.txt

    stateManagerPush(STATE_MAIN_MENU);
    stateManagerFlush(); // so stateManagerDepth() is accurate before the loop starts

    // The game ends when the window closes or the last state is popped (Quit in the main menu)
    while (!WindowShouldClose() && stateManagerDepth() > 0) {
        stateManagerUpdate(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        stateManagerDraw();
        EndDrawing();
    }

    stateManagerShutdown();
    CloseWindow();
    return 0;
}