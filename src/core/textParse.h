// textParse.h - tiny helpers for the line-based data files (items, enemies, party, map data, saves).
//
// File rules shared by all of them: one entry per line, blank lines and lines starting with # are ignored,
// words are separated by spaces or tabs, options look like key=value, and free text (a display name) is
// whatever is left of the line.
#ifndef TEXT_PARSE_H
#define TEXT_PARSE_H

#include <stdbool.h>
#include <stddef.h>

char *textSkipSpaces(char *s);

// Copies the next whitespace-separated word into out and advances *cursor. False at the end of the line.
bool textNextToken(char **cursor, char *out, size_t size);

// Whole-string number parsing: "12" works, "12x" and "" do not.
bool textParseInt(const char *s, int *out);
bool textParseFloat(const char *s, float *out);

// Removes trailing spaces, tabs and line breaks in place.
void textTrimEnd(char *s);

// If token looks like "key=value" for this key, sets *value to the text after '=' and returns true.
bool textKeyValue(const char *token, const char *key, const char **value);

// Prints "<file>:<line>: <message> (line ignored)" to stderr, prefixed with the module name.
void textWarn(const char *module, const char *path, int lineNumber, const char *message);

// Reads a text file line by line. Blank lines and # comments are skipped; the handler gets the line with
// leading spaces removed and trailing spaces trimmed, and may modify it. Returns false if the file
// cannot be opened (which callers usually treat as "no data").
bool textForEachLine(const char *path, void (*handler)(char *line, int lineNumber, void *user), void *user);

#endif
