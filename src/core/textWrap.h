// textWrap.h - word-wrapped text, shared by the popup and the battle screen
#ifndef TEXT_WRAP_H
#define TEXT_WRAP_H

#include "raylib.h"
#include <stdbool.h>

// Wraps `text` to fit maxWidth pixels at the given font size. With draw = true it draws the lines from
// (x, y) downward, lineHeight pixels apart; with draw = false it only measures. Returns the number of lines.
int textWrap(const char *text, int x, int y, int maxWidth, int fontSize, int lineHeight, bool draw, Color color);

#endif
