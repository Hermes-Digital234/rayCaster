#include "core/textParse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *textSkipSpaces(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

bool textNextToken(char **cursor, char *out, size_t size) {
    char *s = textSkipSpaces(*cursor);
    if (*s == '\0') return false;

    size_t n = 0;
    while (*s != '\0' && *s != ' ' && *s != '\t') {
        if (n + 1 < size) out[n++] = *s;
        s++;
    }
    out[n] = '\0';
    *cursor = s;
    return true;
}

bool textParseInt(const char *s, int *out) {
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return false;
    *out = (int)v;
    return true;
}

bool textParseFloat(const char *s, float *out) {
    char *end;
    float v = strtof(s, &end);
    if (end == s || *end != '\0') return false;
    *out = v;
    return true;
}

void textTrimEnd(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = '\0';
}

bool textKeyValue(const char *token, const char *key, const char **value) {
    size_t n = strlen(key);
    if (strncmp(token, key, n) != 0 || token[n] != '=') return false;
    *value = token + n + 1;
    return true;
}

void textWarn(const char *module, const char *path, int lineNumber, const char *message) {
    fprintf(stderr, "%s: %s:%d: %s (line ignored)\n", module, path, lineNumber, message);
}

bool textForEachLine(const char *path, void (*handler)(char *line, int lineNumber, void *user), void *user) {
    FILE *file = fopen(path, "r");
    if (!file) return false;

    char line[512];
    int lineNumber = 0;
    while (fgets(line, sizeof(line), file)) {
        lineNumber++;
        textTrimEnd(line);
        char *start = textSkipSpaces(line);
        if (*start == '\0' || *start == '#') continue;
        handler(start, lineNumber, user);
    }
    fclose(file);
    return true;
}
