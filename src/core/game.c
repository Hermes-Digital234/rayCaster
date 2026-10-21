#include "core/game.h"

#include "core/worldState.h"
#include "enemies/enemy.h"
#include "party/inventory.h"
#include "party/item.h"
#include "party/party.h"

#include <stdio.h>

void gameLoadData(void) {
    int items = itemDbLoad(ITEMS_FILE);
    int enemies = enemyDbLoad(ENEMIES_FILE);
    fprintf(stderr, "game: loaded %d items, %d enemies\n", items < 0 ? 0 : items, enemies < 0 ? 0 : enemies);
}

void gameNewGame(void) {
    partyInitDefault(); // needs the item table, so gameLoadData comes first
    inventoryReset();
    worldReset();
}
