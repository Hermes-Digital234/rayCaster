// game.h - loading the game's data files and starting a new game. Everything game-specific lives in those
// files (assets/data), so this is the only place that knows which ones there are.
#ifndef GAME_H
#define GAME_H

// The item chests (and "item" outcomes in map files) hand out. Hardcoded on purpose; override at compile time
// with -DDROPPED_ITEM_ID=\"otherItem\" if a game wants something else. Must be an id from items.txt.
#ifndef DROPPED_ITEM_ID
#define DROPPED_ITEM_ID "healingPotion"
#endif

// Loads items.txt and enemies.txt. Call once at startup (after the working directory is set).
void gameLoadData(void);

// Resets everything the player carries through a run: the party (from party.txt), the item count and the
// changes made to maps.
void gameNewGame(void);

#endif
