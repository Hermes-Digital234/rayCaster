#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <stdbool.h>

typedef enum {
    STATE_NONE = 0,

    STATE_SPLASH,
    STATE_MAIN_MENU,
    STATE_NEW_GAME_SETUP,
    STATE_LOAD_GAME,

    STATE_TOWN,
    STATE_SHOP,
    STATE_GUILD,
    STATE_INN,
    STATE_TAVERN,

    STATE_DUNGEON_EXPLORE,
    STATE_DUNGEON_EVENT,

    STATE_BATTLE_INTRO,
    STATE_BATTLE_MAIN,
    STATE_BATTLE_RESULT,
    STATE_BATTLE_DEFEAT,

    STATE_PAUSE_MENU,
    STATE_INVENTORY_MENU,
    STATE_STATUS_MENU,
    STATE_MAP_MENU,
    STATE_DIALOGUE,
    STATE_INTERACT_PROMPT, // yes/no or OK popup drawn over the dungeon
    STATE_SAVE_LOAD,

    STATE_DEV_MAP_SELECT, // development: pick any map from assets/maps

    STATE_LOADING,
    STATE_GAME_OVER,
    STATE_QUIT,

    STATE_COUNT
} GameState;

#define MAX_STATE_STACK 8

// Any callback may be NULL.
typedef struct {
    void (*init)(void);          // state was pushed or replaced in
    void (*update)(float dt);
    void (*draw)(void);
    void (*unload)(void);        // state was popped or replaced out
    void (*pause)(void);         // another state was pushed on top of this one
    void (*resume)(void);        // the state above this one was popped

    // What happens to the states BENEATH this one while it is on top.
    // A pause menu overlay: drawBelow = true, updateBelow = false.
    bool drawBelow;
    bool updateBelow;
} StateHandlers;

typedef struct {
    GameState stack [MAX_STATE_STACK];
    StateHandlers handlers [MAX_STATE_STACK];
    int stackTop; // index of the top entry, -1 when the stack is empty
} StateManager;

void stateManagerInit(void);

void stateManagerRegister(GameState state, StateHandlers handlers);

// Push/Pop/Replace are QUEUED and applied at the next safe point (the start and end of
// stateManagerUpdate, or stateManagerFlush). It is safe to call them from inside any
// state callback, including init().
void stateManagerPush(GameState state);
void stateManagerPop(void);
void stateManagerReplace(GameState state);

// Pops states until `state` is the top one (it stays). Does nothing if it is not on the stack.
void stateManagerPopTo(GameState state);

// Apply queued transitions right now. Useful for the very first Push before the main loop.
void stateManagerFlush(void);

GameState stateManagerCurrent(void);
int stateManagerDepth(void);
bool stateManagerIsActive(GameState state);

void stateManagerUpdate(float dt);
void stateManagerDraw(void);
void stateManagerShutdown(void);

#endif
