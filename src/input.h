// Keys: W/Up forward, S/Down back, Q/E strafe, A/Left turn left, D/Right turn right
#ifndef INPUT_H
#define INPUT_H

typedef enum {
    ACT_NONE,
    ACT_FORWARD,
    ACT_BACK,
    ACT_STRAFE_LEFT,
    ACT_STRAFE_RIGHT,
    ACT_TURN_LEFT,
    ACT_TURN_RIGHT,
} Action;

// Returns the action currently requested (held keys count, so holding a key repeats it).
Action input_poll(void);

#endif
