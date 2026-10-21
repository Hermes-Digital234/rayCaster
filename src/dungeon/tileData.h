// tileData.h - per-map data file that says what special tiles DO
//
// Each map l<layer>m<map>.png may have a text file assets/data/l<layer>m<map>.txt. A missing file is fine:
// skips then work from both sides and event tiles do nothing.
//
// File format: one entry per line. Blank lines and lines starting with # are ignored.
//
//   skip  <x> <y> [from=<north|east|south|west|any>]
//       Which side of the magenta wall tile the player must stand on to push through it FIRST.
//       "from=west" means standing west of the wall and walking east. After the first successful
//       crossing the skip is "discovered" and works from both sides. Default: any.
//
//   event <x> <y> [chance=<0-100>] [once=<0|1>] <message|battle|ask> <text>
//       Something that can happen on the pink tile at x,y. The player steps onto the tile freely; when
//       they press the interact button there, the tile's events are tried in file order and the first
//       one whose chance roll succeeds happens (if none does, they find nothing). chance defaults to 100,
//       once=1 means it can only happen once per map load.
//         message <text>   shows the text in a popup
//         battle <group>   starts a battle with that enemy group (placeholder popup until battles exist)
//         ask <question>   shows the question with a menu of options, defined by the option lines below it
//
//   option <label> => [chance=<0-100>] <message|battle|item|nothing> [text]
//       One answer to the "ask" event written just above it (up to 4 answers per question). Several lines
//       with the SAME label are the possible outcomes of that answer: they are tried in order and the
//       first whose chance succeeds happens. A line without chance always succeeds, so put it last.
//         message <text>       shows the text
//         battle <group>       starts a battle
//         item <count> [text]  gives the player items, and shows the text (or a default one)
//         nothing              nothing happens
//
//   Example:
//       event 4 11 once=1 ask You see a bag left behind by another adventurer. Pick it up?
//       option Pick it up => chance=25 battle goblins
//       option Pick it up => item 1 Inside the bag you find a healing potion.
//       option Leave it   => message You leave the bag where it lies.
//
// x and y are the pixel coordinates in the map image (0,0 is the top-left pixel).
// Discovered skips and fired events are kept in memory while the map is loaded (not saved yet).
#ifndef TILE_DATA_H
#define TILE_DATA_H

#include "dungeon/map.h"
#include <stdbool.h>

typedef enum { SIDE_ANY, SIDE_NORTH, SIDE_EAST, SIDE_SOUTH, SIDE_WEST } Side;
typedef enum { EVENT_MESSAGE, EVENT_BATTLE, EVENT_ASK } EventType;
typedef enum { OUTCOME_NOTHING, OUTCOME_MESSAGE, OUTCOME_BATTLE, OUTCOME_ITEM } OutcomeType;

#define MAX_EVENT_OPTIONS 4  // answers per "ask" event
#define MAX_OPTION_OUTCOMES 4 // possible outcomes per answer

typedef struct {
    OutcomeType type;
    int chance;       // percent
    int amount;       // OUTCOME_ITEM: how many items
    char text[160];   // message text, enemy group, or item text (may be empty for items)
} EventOutcome;

typedef struct {
    char label[48];
    EventOutcome outcomes[MAX_OPTION_OUTCOMES];
    int outcomeCount;
} EventOption;

typedef struct {
    int x, y;
    Side from;        // which side may be used for the first crossing
    bool discovered;  // true after a successful crossing; then both sides work
} SkipInfo;

typedef struct {
    int x, y;
    EventType type;
    char text[160];   // message, enemy group, or the question of an "ask" event
    int chance;       // percent
    bool once;
    bool fired;
    EventOption options[MAX_EVENT_OPTIONS]; // EVENT_ASK only
    int optionCount;
} EventInfo;

typedef struct {
    SkipInfo *skips;  // one per magenta tile in the map
    int skipCount;
    EventInfo *events;
    int eventCount;
} TileData;

// Builds the runtime data for a loaded map from the file at path. Never fails: problems in the file
// are reported on stderr and the offending line is skipped.
void tileDataLoad(TileData *data, const Map *map, const char *path);
void tileDataFree(TileData *data);

SkipInfo *tileDataFindSkip(TileData *data, int x, int y); // NULL if there is no skip there

// May the player cross this skip while travelling in direction (dirX, dirY)?
bool tileDataSkipCanCross(const SkipInfo *skip, int dirX, int dirY);

// Can anything still happen on the tile at x,y? (false if it has no events, or all of them were "once" and
// already happened.) Use it to decide whether to offer the interact button.
bool tileDataHasEvent(const TileData *data, int x, int y);

// Rolls the events of the tile at x,y. Returns the one that happens, or NULL if none does.
// The pointer stays valid until tileDataFree.
const EventInfo *tileDataRollEvent(TileData *data, int x, int y);

// Rolls the outcomes of an answer to an "ask" event. Returns the one that happens, or NULL if none does.
const EventOutcome *tileDataRollOption(const EventOption *option);

#endif