#include "input.h"
#include "raylib.h"

Action input_poll(void) {
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    return ACT_FORWARD;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  return ACT_BACK;
    if (IsKeyDown(KEY_E))                         return ACT_STRAFE_RIGHT;
    if (IsKeyDown(KEY_Q))                         return ACT_STRAFE_LEFT;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  return ACT_TURN_LEFT;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) return ACT_TURN_RIGHT;
    return ACT_NONE;
}
