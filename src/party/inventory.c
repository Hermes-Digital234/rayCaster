#include "party/inventory.h"

#include <stdlib.h>
#include <string.h>

static InventoryStack *stacks;
static int stackCount, stackCapacity;

static int findStack(const char *id) {
    for (int i = 0; i < stackCount; i++)
        if (!strcmp(stacks[i].id, id)) return i;
    return -1;
}

int inventoryStackCount(void) { return stackCount; }

const InventoryStack *inventoryStackAt(int index) {
    return (index >= 0 && index < stackCount) ? &stacks[index] : NULL;
}

int inventoryCount(const char *id) {
    int i = findStack(id);
    return i < 0 ? 0 : stacks[i].count;
}

int inventoryItems(void) {
    int total = 0;
    for (int i = 0; i < stackCount; i++) total += stacks[i].count;
    return total;
}

int inventoryAdd(const char *id, int count) {
    if (!id || !*id || count < 1) return 0;

    int i = findStack(id);
    int have = i < 0 ? 0 : stacks[i].count;
    if (INVENTORY_STACK_LIMIT > 0 && have + count > INVENTORY_STACK_LIMIT) {
        count = INVENTORY_STACK_LIMIT - have;
        if (count < 1) return 0;
    }

    if (i < 0) {
        if (stackCount == stackCapacity) {
            int capacity = stackCapacity ? stackCapacity * 2 : 16;
            InventoryStack *grown = realloc(stacks, (size_t)capacity * sizeof(InventoryStack));
            if (!grown) return 0;
            stacks = grown;
            stackCapacity = capacity;
        }
        i = stackCount++;
        memset(&stacks[i], 0, sizeof(stacks[i]));
        strncpy(stacks[i].id, id, sizeof(stacks[i].id) - 1);
    }
    stacks[i].count += count;
    return count;
}

bool inventoryRemove(const char *id, int count) {
    int i = findStack(id);
    if (i < 0 || count < 1 || stacks[i].count < count) return false;
    stacks[i].count -= count;
    if (stacks[i].count == 0) {
        memmove(&stacks[i], &stacks[i + 1], (size_t)(stackCount - i - 1) * sizeof(InventoryStack));
        stackCount--;
    }
    return true;
}

void inventoryReset(void) { stackCount = 0; }
