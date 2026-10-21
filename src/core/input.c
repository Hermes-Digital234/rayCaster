// input.c - keyboard bindings live here and nowhere else
#include "input.h"
#include "raylib.h"

Action inputPoll(void) {
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    return ACT_FORWARD;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  return ACT_BACK;
    if (IsKeyDown(KEY_E))                         return ACT_STRAFE_RIGHT;
    if (IsKeyDown(KEY_Q))                         return ACT_STRAFE_LEFT;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  return ACT_TURN_LEFT;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) return ACT_TURN_RIGHT;
    return ACT_NONE;
}

bool inputInteractPressed(void) {
    return IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z);
}

bool inputBackPressed(void) {
    return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_X) || IsKeyPressed(KEY_BACKSPACE);
}

MenuAction inputMenu(void) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) ||
        IsKeyPressed(KEY_W)  || IsKeyPressedRepeat(KEY_W))   return MENU_UP;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN) ||
        IsKeyPressed(KEY_S)    || IsKeyPressedRepeat(KEY_S)) return MENU_DOWN;
    if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) return MENU_LEFT;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) return MENU_RIGHT;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z)) return MENU_CONFIRM;
    if (inputBackPressed()) return MENU_BACK;
    return MENU_NONE;
}
