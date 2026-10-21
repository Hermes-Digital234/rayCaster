#include "dungeon/tileData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 512

static char *skipSpaces(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

// Copies the next whitespace-separated word into out and advances the cursor. False at end of line.
static bool nextToken(char **cursor, char *out, size_t size) {
    char *s = skipSpaces(*cursor);
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

static bool parseInt(const char *s, int *out) {
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return false;
    *out = (int)v;
    return true;
}

static bool parseSide(const char *s, Side *out) {
    if (!strcmp(s, "any"))   { *out = SIDE_ANY;   return true; }
    if (!strcmp(s, "north")) { *out = SIDE_NORTH; return true; }
    if (!strcmp(s, "east"))  { *out = SIDE_EAST;  return true; }
    if (!strcmp(s, "south")) { *out = SIDE_SOUTH; return true; }
    if (!strcmp(s, "west"))  { *out = SIDE_WEST;  return true; }
    return false;
}

static void warn(const char *path, int lineNumber, const char *message) {
    fprintf(stderr, "tileData: %s:%d: %s (line ignored)\n", path, lineNumber, message);
}

// "skip <x> <y> [from=...]"
static void parseSkip(TileData *data, const Map *map, char *rest, const char *path, int lineNumber) {
    char token[64];
    int x, y;
    if (!nextToken(&rest, token, sizeof(token)) || !parseInt(token, &x) ||
        !nextToken(&rest, token, sizeof(token)) || !parseInt(token, &y)) {
        warn(path, lineNumber, "skip needs two numbers: skip <x> <y>");
        return;
    }

    SkipInfo *skip = tileDataFindSkip(data, x, y);
    if (!skip || mapTile(map, x, y) != T_SKIP) {
        warn(path, lineNumber, "there is no magenta skip tile at that position");
        return;
    }

    Side from = skip->from;
    while (nextToken(&rest, token, sizeof(token))) {
        if (strncmp(token, "from=", 5) == 0 && parseSide(token + 5, &from)) continue;
        warn(path, lineNumber, "unknown or invalid option (expected from=north|east|south|west|any)");
        return;
    }
    skip->from = from;
}

// "event <x> <y> [chance=N] [once=0|1] <message|battle|ask> <text>"
// Returns the index of the new event, or -1 if the line was ignored.
static int parseEvent(TileData *data, const Map *map, char *rest, const char *path, int lineNumber) {
    char token[64];
    int x, y;
    if (!nextToken(&rest, token, sizeof(token)) || !parseInt(token, &x) ||
        !nextToken(&rest, token, sizeof(token)) || !parseInt(token, &y)) {
        warn(path, lineNumber, "event needs two numbers: event <x> <y> ...");
        return -1;
    }
    if (mapTile(map, x, y) != T_EVENT) {
        warn(path, lineNumber, "there is no pink event tile at that position");
        return -1;
    }

    int chance = 100;
    bool once = false;
    bool haveType = false;
    EventType type = EVENT_MESSAGE;

    while (nextToken(&rest, token, sizeof(token))) {
        if (strncmp(token, "chance=", 7) == 0) {
            if (!parseInt(token + 7, &chance) || chance < 0 || chance > 100) {
                warn(path, lineNumber, "chance must be a number from 0 to 100");
                return -1;
            }
        } else if (strncmp(token, "once=", 5) == 0) {
            int v;
            if (!parseInt(token + 5, &v) || (v != 0 && v != 1)) {
                warn(path, lineNumber, "once must be 0 or 1");
                return -1;
            }
            once = (v == 1);
        } else if (!strcmp(token, "message")) {
            type = EVENT_MESSAGE; haveType = true; break;
        } else if (!strcmp(token, "battle")) {
            type = EVENT_BATTLE; haveType = true; break;
        } else if (!strcmp(token, "ask")) {
            type = EVENT_ASK; haveType = true; break;
        } else {
            warn(path, lineNumber, "expected chance=, once=, or the event type (message, battle or ask)");
            return -1;
        }
    }

    char *text = skipSpaces(rest);
    if (!haveType || *text == '\0') {
        warn(path, lineNumber, "event needs a type and text: message <text>, battle <group> or ask <question>");
        return -1;
    }

    EventInfo *grown = realloc(data->events, (size_t)(data->eventCount + 1) * sizeof(EventInfo));
    if (!grown) return -1;
    data->events = grown;

    EventInfo *event = &data->events[data->eventCount++];
    memset(event, 0, sizeof(*event));
    event->x = x;
    event->y = y;
    event->type = type;
    snprintf(event->text, sizeof(event->text), "%s", text);
    event->chance = chance;
    event->once = once;
    event->fired = false;
    return data->eventCount - 1;
}

// "option <label> => [chance=N] <message|battle|item|nothing> [text]"  (belongs to the last "ask" event)
static void parseOption(TileData *data, int askIndex, char *rest, const char *path, int lineNumber) {
    if (askIndex < 0) {
        warn(path, lineNumber, "option must come directly after an \"event ... ask\" line");
        return;
    }
    EventInfo *event = &data->events[askIndex];

    char *arrow = strstr(rest, "=>");
    if (!arrow) {
        warn(path, lineNumber, "option needs \"=>\" between its label and its outcome");
        return;
    }
    *arrow = '\0';

    char *label = skipSpaces(rest);
    size_t n = strlen(label);
    while (n > 0 && (label[n - 1] == ' ' || label[n - 1] == '\t')) label[--n] = '\0';
    if (*label == '\0') {
        warn(path, lineNumber, "option needs a label before the \"=>\"");
        return;
    }

    char *after = arrow + 2;
    char token[64];
    EventOutcome outcome;
    memset(&outcome, 0, sizeof(outcome));
    outcome.chance = 100;

    bool haveType = false;
    while (nextToken(&after, token, sizeof(token))) {
        if (strncmp(token, "chance=", 7) == 0) {
            if (!parseInt(token + 7, &outcome.chance) || outcome.chance < 0 || outcome.chance > 100) {
                warn(path, lineNumber, "chance must be a number from 0 to 100");
                return;
            }
        } else if (!strcmp(token, "message")) {
            outcome.type = OUTCOME_MESSAGE; haveType = true; break;
        } else if (!strcmp(token, "battle")) {
            outcome.type = OUTCOME_BATTLE; haveType = true; break;
        } else if (!strcmp(token, "item")) {
            outcome.type = OUTCOME_ITEM; haveType = true; break;
        } else if (!strcmp(token, "nothing")) {
            outcome.type = OUTCOME_NOTHING; haveType = true; break;
        } else {
            warn(path, lineNumber, "expected chance= or the outcome type (message, battle, item or nothing)");
            return;
        }
    }
    if (!haveType) {
        warn(path, lineNumber, "option needs an outcome: message <text>, battle <group>, item <count> [text] or nothing");
        return;
    }

    if (outcome.type == OUTCOME_ITEM) {
        if (!nextToken(&after, token, sizeof(token)) || !parseInt(token, &outcome.amount) || outcome.amount < 1) {
            warn(path, lineNumber, "item needs a count of at least 1: item <count> [text]");
            return;
        }
    }
    char *text = skipSpaces(after);
    if ((outcome.type == OUTCOME_MESSAGE || outcome.type == OUTCOME_BATTLE) && *text == '\0') {
        warn(path, lineNumber, "this outcome needs text (the message, or the enemy group)");
        return;
    }
    if (outcome.type != OUTCOME_NOTHING) snprintf(outcome.text, sizeof(outcome.text), "%s", text);

    // Find the answer with this label, or start a new one
    EventOption *option = NULL;
    for (int i = 0; i < event->optionCount; i++)
        if (!strcmp(event->options[i].label, label)) option = &event->options[i];
    if (!option) {
        if (event->optionCount >= MAX_EVENT_OPTIONS) {
            warn(path, lineNumber, "an ask event can have at most 4 different answers");
            return;
        }
        option = &event->options[event->optionCount++];
        snprintf(option->label, sizeof(option->label), "%s", label);
    }
    if (option->outcomeCount >= MAX_OPTION_OUTCOMES) {
        warn(path, lineNumber, "an answer can have at most 4 outcomes");
        return;
    }
    option->outcomes[option->outcomeCount++] = outcome;
}

void tileDataLoad(TileData *data, const Map *map, const char *path) {
    memset(data, 0, sizeof(*data));

    // Every magenta tile is a skip, whether or not the file mentions it
    int count = 0;
    for (int i = 0; i < map->w * map->h; i++)
        if (map->tiles[i] == T_SKIP) count++;

    if (count > 0) {
        data->skips = calloc((size_t)count, sizeof(SkipInfo));
        for (int i = 0; i < map->w * map->h && data->skips; i++) {
            if (map->tiles[i] != T_SKIP) continue;
            SkipInfo *skip = &data->skips[data->skipCount++];
            skip->x = i % map->w;
            skip->y = i / map->w;
            skip->from = SIDE_ANY;
            skip->discovered = false;
        }
    }

    FILE *file = fopen(path, "r");
    if (!file) return; // no data file: defaults only

    char line[LINE_SIZE];
    int lineNumber = 0;
    int askIndex = -1; // the "event ... ask" that option lines currently belong to
    while (fgets(line, sizeof(line), file)) {
        lineNumber++;

        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r' || line[n - 1] == ' ' || line[n - 1] == '\t'))
            line[--n] = '\0';

        char *cursor = skipSpaces(line);
        if (*cursor == '\0' || *cursor == '#') continue;

        char kind[32];
        nextToken(&cursor, kind, sizeof(kind));

        if (!strcmp(kind, "option")) {
            parseOption(data, askIndex, cursor, path, lineNumber);
            continue;
        }

        askIndex = -1; // any other line ends the list of options
        if (!strcmp(kind, "skip")) {
            parseSkip(data, map, cursor, path, lineNumber);
        } else if (!strcmp(kind, "event")) {
            int index = parseEvent(data, map, cursor, path, lineNumber);
            if (index >= 0 && data->events[index].type == EVENT_ASK) askIndex = index;
        } else {
            warn(path, lineNumber, "unknown entry type (expected skip, event or option)");
        }
    }
    fclose(file);

    // A question without answers would be a dead end: show it as a plain message instead
    for (int i = 0; i < data->eventCount; i++) {
        EventInfo *event = &data->events[i];
        if (event->type == EVENT_ASK && event->optionCount == 0) {
            fprintf(stderr, "tileData: %s: the ask event at %d,%d has no option lines (shown as a message)\n",
                    path, event->x, event->y);
            event->type = EVENT_MESSAGE;
        }
    }
}

void tileDataFree(TileData *data) {
    free(data->skips);
    free(data->events);
    memset(data, 0, sizeof(*data));
}

SkipInfo *tileDataFindSkip(TileData *data, int x, int y) {
    for (int i = 0; i < data->skipCount; i++)
        if (data->skips[i].x == x && data->skips[i].y == y) return &data->skips[i];
    return NULL;
}

bool tileDataSkipCanCross(const SkipInfo *skip, int dirX, int dirY) {
    if (skip->discovered || skip->from == SIDE_ANY) return true;

    // "from=west" = standing west of the wall, i.e. travelling east, and so on
    switch (skip->from) {
        case SIDE_WEST:  return dirX == 1 && dirY == 0;
        case SIDE_EAST:  return dirX == -1 && dirY == 0;
        case SIDE_NORTH: return dirX == 0 && dirY == 1;
        case SIDE_SOUTH: return dirX == 0 && dirY == -1;
        default:         return true;
    }
}

bool tileDataHasEvent(const TileData *data, int x, int y) {
    for (int i = 0; i < data->eventCount; i++) {
        const EventInfo *event = &data->events[i];
        if (event->x == x && event->y == y && !(event->once && event->fired)) return true;
    }
    return false;
}

const EventOutcome *tileDataRollOption(const EventOption *option) {
    for (int i = 0; i < option->outcomeCount; i++)
        if ((rand() % 100) < option->outcomes[i].chance) return &option->outcomes[i];
    return NULL;
}

const EventInfo *tileDataRollEvent(TileData *data, int x, int y) {
    for (int i = 0; i < data->eventCount; i++) {
        EventInfo *event = &data->events[i];
        if (event->x != x || event->y != y) continue;
        if (event->once && event->fired) continue;

        if ((rand() % 100) < event->chance) {
            event->fired = true;
            return event;
        }
    }
    return NULL;
}