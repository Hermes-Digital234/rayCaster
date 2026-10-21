// input.h - turns raw keyboard state into game actions. Key bindings live in input.c only.
//
// Exploring: W/Up forward, S/Down back, Q/E strafe, A/Left turn left, D/Right turn right
// Interact:  Space/Enter/Z (the "interact" button that appears next to something you can use)
// Menus:     W/S or Up/Down and A/D or Left/Right move, Enter/Space/Z confirm, Esc/X/Backspace back
#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

typedef enum {
    ACT_NONE,
    ACT_FORWARD,
    ACT_BACK,
    ACT_STRAFE_LEFT,
    ACT_STRAFE_RIGHT,
    ACT_TURN_LEFT,
    ACT_TURN_RIGHT,
} Action;

typedef enum {
    MENU_NONE,
    MENU_UP,
    MENU_DOWN,
    MENU_LEFT,
    MENU_RIGHT,
    MENU_CONFIRM,
    MENU_BACK,
} MenuAction;

// Movement while exploring. Held keys count, so holding a key repeats the action.
Action inputPoll(void);

// One menu action per key press (up/down also repeat while held).
MenuAction inputMenu(void);

// True on the frame the interact button is pressed.
bool inputInteractPressed(void);

// True on the frame "back" is pressed.
bool inputBackPressed(void);

#endif
