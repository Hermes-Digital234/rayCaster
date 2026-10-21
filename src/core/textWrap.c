#include "core/textWrap.h"

#include <stdio.h>
#include <string.h>

// Bounded copy that always terminates
static void copyText(char *dst, size_t size, const char *src) {
    size_t n = strlen(src);
    if (n >= size) n = size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

int textWrap(const char *text, int x, int y, int maxWidth, int fontSize, int lineHeight, bool draw, Color color) {
    char line[400] = "";
    char word[128];
    int lines = 0;
    const char *p = text;

    while (*p) {
        while (*p == ' ') p++;
        int n = 0;
        while (*p && *p != ' ' && n < (int)sizeof(word) - 1) word[n++] = *p++;
        word[n] = '\0';
        if (n == 0) break;

        char candidate[600]; // big enough for a full line plus one more word
        if (line[0]) snprintf(candidate, sizeof(candidate), "%s %s", line, word);
        else         snprintf(candidate, sizeof(candidate), "%s", word);

        if (line[0] && MeasureText(candidate, fontSize) > maxWidth) {
            if (draw) DrawText(line, x, y + lines * lineHeight, fontSize, color);
            lines++;
            copyText(line, sizeof(line), word);
        } else {
            copyText(line, sizeof(line), candidate);
        }
    }
    if (line[0]) {
        if (draw) DrawText(line, x, y + lines * lineHeight, fontSize, color);
        lines++;
    }
    return lines;
}
