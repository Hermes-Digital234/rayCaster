#include "stateManager.h"
#include "raylib.h" // TraceLog

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static StateManager manager;
static StateHandlers registry[STATE_COUNT];
static bool registered[STATE_COUNT];

typedef enum {
    ACTION_PUSH,
    ACTION_POP,
    ACTION_REPLACE,
    ACTION_POP_TO
} PendingType;

typedef struct {
    PendingType type;
    GameState state;
} PendingAction;

#define MAX_PENDING 8
static PendingAction pending[MAX_PENDING];
static int pendingCount = 0;

static bool isRegistered(GameState state) {
    return state > STATE_NONE && state < STATE_COUNT && registered[state];
}

static void queue(PendingType type, GameState state) {
    if (pendingCount >= MAX_PENDING) {
        TraceLog(LOG_ERROR, "StateManager: too many transitions queued at once, dropping one");
        return;
    }
    pending[pendingCount].type = type;
    pending[pendingCount].state = state;
    pendingCount++;
}

// --- The actual stack operations. Only called while applying the queue. ---

static void doPush(GameState state) {
    if (!isRegistered(state)) {
        TraceLog(LOG_WARNING, "StateManager: state %d is not registered, push ignored", (int)state);
        return;
    }
    if (manager.stackTop + 1 >= MAX_STATE_STACK) {
        TraceLog(LOG_ERROR, "StateManager: state stack is full (%d), push ignored", MAX_STATE_STACK);
        return;
    }

    if (manager.stackTop >= 0) {
        StateHandlers *below = &manager.handlers[manager.stackTop];
        if (below->pause) below->pause();
    }

    manager.stackTop++;
    manager.stack[manager.stackTop] = state;
    manager.handlers[manager.stackTop] = registry[state];
    if (registry[state].init) registry[state].init();
}

static void doPop(void) {
    if (manager.stackTop < 0) return;

    StateHandlers top = manager.handlers[manager.stackTop];
    if (top.unload) top.unload();
    manager.stackTop--;

    if (manager.stackTop >= 0) {
        StateHandlers *now = &manager.handlers[manager.stackTop];
        if (now->resume) now->resume();
    }
}

static void doReplace(GameState state) {
    if (!isRegistered(state)) {
        TraceLog(LOG_WARNING, "StateManager: state %d is not registered, replace ignored", (int)state);
        return;
    }
    if (manager.stackTop < 0) { // nothing to replace
        doPush(state);
        return;
    }

    StateHandlers old = manager.handlers[manager.stackTop];
    if (old.unload) old.unload();

    manager.stack[manager.stackTop] = state;
    manager.handlers[manager.stackTop] = registry[state];
    if (registry[state].init) registry[state].init();
}

// Callbacks may queue more transitions while we run, so the loop re-reads pendingCount.
static void applyPending(void) {
    for (int i = 0; i < pendingCount; i++) {
        PendingAction a = pending[i];
        switch (a.type) {
            case ACTION_PUSH:    doPush(a.state);    break;
            case ACTION_POP:     doPop();            break;
            case ACTION_REPLACE: doReplace(a.state); break;
            case ACTION_POP_TO:
                // checked now, not when queued, because earlier queued actions may change the stack
                if (!stateManagerIsActive(a.state)) break;
                while (manager.stackTop >= 0 && manager.stack[manager.stackTop] != a.state) doPop();
                break;
        }
    }
    pendingCount = 0;
}

// --- Public API ---

void stateManagerInit(void) {
    memset(&manager, 0, sizeof(manager));
    memset(registry, 0, sizeof(registry));
    memset(registered, 0, sizeof(registered));
    manager.stackTop = -1;
    pendingCount = 0;
}

void stateManagerRegister(GameState state, StateHandlers handlers) {
    if (state <= STATE_NONE || state >= STATE_COUNT) {
        TraceLog(LOG_WARNING, "StateManager: cannot register invalid state %d", (int)state);
        return;
    }
    registry[state] = handlers;
    registered[state] = true;
}

void stateManagerPush(GameState state)    { queue(ACTION_PUSH, state); }
void stateManagerPop(void)                { queue(ACTION_POP, STATE_NONE); }
void stateManagerReplace(GameState state) { queue(ACTION_REPLACE, state); }
void stateManagerPopTo(GameState state)  { queue(ACTION_POP_TO, state); }

void stateManagerFlush(void) { applyPending(); }

GameState stateManagerCurrent(void) {
    return (manager.stackTop >= 0) ? manager.stack[manager.stackTop] : STATE_NONE;
}

int stateManagerDepth(void) { return manager.stackTop + 1; }

bool stateManagerIsActive(GameState state) {
    for (int i = 0; i <= manager.stackTop; i++)
        if (manager.stack[i] == state) return true;
    return false;
}

void stateManagerUpdate(float dt) {
    applyPending();
    if (manager.stackTop < 0) return;

    // Walk down while the state above says the one beneath it keeps updating
    int first = manager.stackTop;
    while (first > 0 && manager.handlers[first].updateBelow) first--;

    for (int i = first; i <= manager.stackTop; i++)
        if (manager.handlers[i].update) manager.handlers[i].update(dt);

    applyPending(); // so a state that just popped itself is never drawn
}

void stateManagerDraw(void) {
    if (manager.stackTop < 0) return;

    int first = manager.stackTop;
    while (first > 0 && manager.handlers[first].drawBelow) first--;

    for (int i = first; i <= manager.stackTop; i++) // bottom to top
        if (manager.handlers[i].draw) manager.handlers[i].draw();
}

void stateManagerShutdown(void) {
    while (manager.stackTop >= 0) {
        StateHandlers top = manager.handlers[manager.stackTop];
        if (top.unload) top.unload();
        manager.stackTop--;
    }
    pendingCount = 0;
}
