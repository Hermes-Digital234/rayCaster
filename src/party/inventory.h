// inventory.h - what the party carries: a list of item stacks (item id + count).
//
// There is no cap on how many different items or how many of each you can hold. If your game wants one, define
// INVENTORY_STACK_LIMIT at compile time (e.g. -DINVENTORY_STACK_LIMIT=99); 0 means unlimited.
#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdbool.h>

#ifndef INVENTORY_STACK_LIMIT
#define INVENTORY_STACK_LIMIT 0
#endif

typedef struct {
    char id[32];  // an id from items.txt (kept even if the item is later removed from the file)
    int count;    // always at least 1
} InventoryStack;

int inventoryStackCount(void);                    // number of different items held
const InventoryStack *inventoryStackAt(int index);
int inventoryCount(const char *id);               // how many of this item (0 if none)
int inventoryItems(void);                         // total number of items, all stacks together

// Adds up to `count` items and returns how many fit (all of them unless INVENTORY_STACK_LIMIT is set).
int inventoryAdd(const char *id, int count);
// Removes `count` items. False (and nothing removed) if there are fewer than that.
bool inventoryRemove(const char *id, int count);
void inventoryReset(void);

#endif
