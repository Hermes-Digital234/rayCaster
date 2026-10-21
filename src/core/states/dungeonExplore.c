// STATE_DUNGEON_EXPLORE: first-person grid exploration.
// Owns the loaded map, its tile data, the player and the renderer for as long as it is on the stack.
// Esc opens the pause menu (party, save, load, leave the dungeon).
//
// How interactions work:
//   1. Each frame findOpportunity() looks at what the player could do right now: the tile they are
//      facing (door, stairs, skip, closed chest), or the event tile they are standing on.
//   2. If there is something, draw() shows an "interact" button and pressing it activates it.
//   3. Doors, stairs, skips and chests open a Yes/No popup first; event tiles start their event directly.
//      Events can ask a question with a menu of answers (see tileData.h for the data file format).
//   4. When the popup closes, resume() runs and carries out the answer.
//   5. Doors, skips and stairs fade to black; at full black resolveTransition teleports or swaps the floor.
#include "core/states/dungeonExplore.h"

#include "core/input.h"
#include "core/save.h"
#include "core/stateManager.h"
#include "core/states/battleMain.h"
#include "core/states/interactPrompt.h"
#include "core/worldState.h"
#include "dungeon/interaction.h"
#include "dungeon/map.h"
#include "dungeon/mapCatalog.h"
#include "dungeon/raycaster.h"
#include "dungeon/tileData.h"
#include "core/game.h"
#include "party/inventory.h"
#include "party/item.h"
#include "party/party.h"
#include "player/player.h"
#include "raylib.h"

// Which map to load on the next init (set through dungeonExploreSelectMap).
// Layer = which set of floors (and, later, which textures); map = the floor within that layer.
static int currentLayer = 1;
static int currentMap = 1;

// Zero-initialised, so unload() is safe even if init() bailed out early
static Map map;
static TileData tileData;
static Player player;
static Raycaster rc;

// The tile an interaction is aimed at: (tileX, tileY) is the target, (dirX, dirY) the direction the player faces
typedef struct {
    int tileX, tileY;
    int dirX, dirY;
} Target;

// What the player could do right now (see findOpportunity)
typedef enum { OPPORTUNITY_NONE, OPPORTUNITY_FACING, OPPORTUNITY_EVENT, OPPORTUNITY_HEAL } OpportunityType;
typedef struct {
    OpportunityType type;
    InteractKind kind;  // OPPORTUNITY_FACING
    Target target;      // OPPORTUNITY_FACING
    const char *label;  // text on the interact button
} Opportunity;

// What the popup that is currently open is waiting for
typedef enum { PENDING_NONE, PENDING_INTERACT, PENDING_CHOICE } PendingType;
static struct {
    PendingType type;
    InteractKind kind;        // PENDING_INTERACT
    Target target;            // PENDING_INTERACT
    const EventInfo *event;   // PENDING_CHOICE: the "ask" event whose menu is open
} pending;

// What to do once the screen is fully black
typedef enum { TRANSITION_NONE, TRANSITION_CROSS, TRANSITION_STAIRS } TransitionType;
static struct {
    TransitionType type;
    int destX, destY;         // TRANSITION_CROSS: the cell to appear in
    int mapDelta;             // TRANSITION_STAIRS: -1 = up (previous map), +1 = down (next map)
    InteractKind stairsKind;
} transition;

// After a popup closes, ignore movement keys until they are released, so holding "forward" against a
// door does not immediately reopen the popup.
static bool waitForRelease = false;

// Where to put the player on the next init instead of the map's start tile (set when loading a save)
static bool hasStartOverride = false;
static SaveLocation startOverride;

void dungeonExploreSelectMap(int layer, int mapNumber) {
    currentLayer = layer;
    currentMap = mapNumber;
    hasStartOverride = false;
}

void dungeonExploreStartAt(const SaveLocation *location) {
    currentLayer = location->layer;
    currentMap = location->map;
    startOverride = *location;
    hasStartOverride = true;
}

bool dungeonExploreGetLocation(SaveLocation *out) {
    if (!stateManagerIsActive(STATE_DUNGEON_EXPLORE) || map.tiles == NULL) return false;
    out->layer = currentLayer;
    out->map = currentMap;
    out->x = player.gx;
    out->y = player.gy;
    playerFacing(&player, &out->dirX, &out->dirY);
    return true;
}

// Re-applies what the player already changed on this map: opened chests, discovered skips, once-only events
// that already happened.
static void applyWorldState(int layer, int mapNumber, Map *m, TileData *data) {
    for (int i = 0; i < worldCount(); i++) {
        const WorldFlag *f = worldAt(i);
        if (f->layer != layer || f->map != mapNumber) continue;

        switch (f->type) {
            case WORLD_CHEST_OPENED:
                if (mapTile(m, f->x, f->y) == T_ITEM) mapSet(m, f->x, f->y, T_CHEST_OPEN);
                break;
            case WORLD_SKIP_DISCOVERED: {
                SkipInfo *skip = tileDataFindSkip(data, f->x, f->y);
                if (skip) skip->discovered = true;
                break;
            }
            case WORLD_EVENT_FIRED:
                // the index is only trusted if the data file still has that event at that position
                if (f->index >= 0 && f->index < data->eventCount && data->events[f->index].x == f->x &&
                    data->events[f->index].y == f->y)
                    data->events[f->index].fired = true;
                break;
        }
    }
}

// Loads a map plus its data file into the given structs. Nothing is changed unless it succeeds.
static bool loadFloor(int layer, int mapNumber, Map *outMap, TileData *outData) {
    char path[256], dataPath[256];
    mapCatalogPath(path, sizeof(path), layer, mapNumber);
    mapCatalogDataPath(dataPath, sizeof(dataPath), layer, mapNumber);

    Map loaded;
    if (!mapLoad(&loaded, path)) {
        TraceLog(LOG_ERROR, "dungeonExplore: could not load map '%s'", path);
        return false;
    }

    int startX, startY;
    if (!mapFindStart(&loaded, &startX, &startY)) {
        TraceLog(LOG_ERROR, "dungeonExplore: '%s' has no white floor tile to start on", path);
        mapFree(&loaded);
        return false;
    }

    *outMap = loaded;
    tileDataLoad(outData, outMap, dataPath);
    applyWorldState(layer, mapNumber, outMap, outData);
    return true;
}

static void init(void) {
    pending.type = PENDING_NONE;
    transition.type = TRANSITION_NONE;
    waitForRelease = false;

    if (!loadFloor(currentLayer, currentMap, &map, &tileData)) {
        stateManagerPop(); // nothing to explore; go back to the menu
        return;
    }

    int startX, startY;
    mapFindStart(&map, &startX, &startY);

    rcInit(&rc);
    playerInit(&player, &map, startX, startY);

    if (hasStartOverride) { // loaded from a save: stand where the save says, if that is still a walkable cell
        if (mapWalkable(mapTile(&map, startOverride.x, startOverride.y)))
            playerPlace(&player, startOverride.x, startOverride.y, startOverride.dirX, startOverride.dirY);
        hasStartOverride = false;
    }
}

// Jumps to a saved location while the state is already running (loading from the pause or game-over menu).
// The map swap is instant; the caller closes its own menu afterwards.
static bool jumpToLocation(const SaveLocation *location) {
    Map newMap;
    TileData newData;
    if (!loadFloor(location->layer, location->map, &newMap, &newData)) return false;

    mapFree(&map);
    tileDataFree(&tileData);
    map = newMap;
    tileData = newData;
    currentLayer = location->layer;
    currentMap = location->map;

    pending.type = PENDING_NONE;
    transition.type = TRANSITION_NONE;
    player.phase = PH_IDLE;
    player.fade = 0.0f;

    if (mapWalkable(mapTile(&map, location->x, location->y))) {
        playerPlace(&player, location->x, location->y, location->dirX, location->dirY);
    } else {
        int startX, startY;
        mapFindStart(&map, &startX, &startY);
        playerPlace(&player, startX, startY, 1, 0);
    }
    return true;
}

bool dungeonExploreLoadLocation(const SaveLocation *location) {
    if (stateManagerIsActive(STATE_DUNGEON_EXPLORE)) return jumpToLocation(location);
    dungeonExploreStartAt(location);
    stateManagerPush(STATE_DUNGEON_EXPLORE);
    return true;
}

// --- Stairs: swap to the next/previous map and appear next to the matching staircase ---

static bool changeFloor(int mapDelta, InteractKind stairsKind) {
    int newMapNumber = currentMap + mapDelta;

    Map newMap;
    TileData newData;
    if (!loadFloor(currentLayer, newMapNumber, &newMap, &newData)) return false;

    mapFree(&map);
    tileDataFree(&tileData);
    map = newMap;
    tileData = newData;
    currentMap = newMapNumber;

    // Going up the stairs arrives at the new floor's down-stairs, and the other way round
    int counterpart = (stairsKind == INTERACT_STAIRS_UP) ? T_STAIRS_DOWN : T_STAIRS_UP;
    static const int dirs[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};

    int stairsX, stairsY;
    if (mapFindTile(&map, counterpart, &stairsX, &stairsY)) {
        for (int i = 0; i < 4; i++) {
            int cx = stairsX + dirs[i][0], cy = stairsY + dirs[i][1];
            if (mapWalkable(mapTile(&map, cx, cy))) {
                playerPlace(&player, cx, cy, dirs[i][0], dirs[i][1]); // face away from the stairs
                return true;
            }
        }
    }

    TraceLog(LOG_WARNING, "dungeonExplore: layer %d map %d has no matching stairs, starting at the entrance",
             currentLayer, currentMap);
    int startX, startY;
    mapFindStart(&map, &startX, &startY);
    playerPlace(&player, startX, startY, 1, 0);
    return true;
}

// Called while the screen is fully black
static void resolveTransition(void) {
    switch (transition.type) {
        case TRANSITION_CROSS:
            playerTeleport(&player, transition.destX, transition.destY);
            break;
        case TRANSITION_STAIRS:
            if (!changeFloor(transition.mapDelta, transition.stairsKind))
                playerTeleport(&player, player.gx, player.gy); // could not load: stay put
            break;
        case TRANSITION_NONE:
            break;
    }
    transition.type = TRANSITION_NONE;
    playerEndBlack(&player);
}

// --- Interactions ---

static void executeInteraction(InteractKind kind, Target target) {
    switch (kind) {
        case INTERACT_DOOR:
            transition.type = TRANSITION_CROSS;
            transition.destX = target.tileX + target.dirX; // the cell on the far side of the door
            transition.destY = target.tileY + target.dirY;
            playerBeginFade(&player, target.dirX, target.dirY);
            break;

        case INTERACT_SKIP: {
            SkipInfo *skip = tileDataFindSkip(&tileData, target.tileX, target.tileY);
            if (skip && !tileDataSkipCanCross(skip, target.dirX, target.dirY)) {
                // Wrong side: the popup was still offered, but the wall holds
                interactPromptOpen("The wall doesn't budge.", PROMPT_OK);
                break;
            }
            if (skip) { // from now on it works from both sides
                skip->discovered = true;
                worldMark(WORLD_SKIP_DISCOVERED, currentLayer, currentMap, skip->x, skip->y, 0);
            }
            transition.type = TRANSITION_CROSS;
            transition.destX = target.tileX + target.dirX;
            transition.destY = target.tileY + target.dirY;
            playerBeginFade(&player, target.dirX, target.dirY);
            break;
        }

        case INTERACT_STAIRS_UP:
        case INTERACT_STAIRS_DOWN: {
            int delta = (kind == INTERACT_STAIRS_UP) ? -1 : 1;
            if (!mapCatalogExists(currentLayer, currentMap + delta)) {
                interactPromptOpen(delta < 0 ? "There is no floor above." : "There is no floor below.", PROMPT_OK);
                break;
            }
            transition.type = TRANSITION_STAIRS;
            transition.mapDelta = delta;
            transition.stairsKind = kind;
            playerBeginFade(&player, target.dirX, target.dirY);
            break;
        }

        case INTERACT_CHEST: {
            if (inventoryAdd(DROPPED_ITEM_ID, 1) < 1) {
                interactPromptOpen("You can't carry any more of these.", PROMPT_OK); // only with INVENTORY_STACK_LIMIT; the chest stays closed
                break;
            }
            const ItemDef *dropped = itemDefFind(DROPPED_ITEM_ID);
            mapSet(&map, target.tileX, target.tileY, T_CHEST_OPEN); // stays in place, drawn open, still blocks you
            worldMark(WORLD_CHEST_OPENED, currentLayer, currentMap, target.tileX, target.tileY, 0);
            interactPromptOpen(TextFormat("You found a %s!", dropped ? dropped->name : DROPPED_ITEM_ID), PROMPT_OK);
            break;
        }

        case INTERACT_NONE:
            break;
    }
}

// --- Events ---

// `enemyId` comes from the map data file (`battle <enemyId>`). An unknown id is a data mistake: say so on the
// terminal and fight the default enemy instead of silently doing nothing.
static void startBattle(const char *enemyId) {
    const EnemyDef *enemy = enemyDefFind(enemyId);
    if (!enemy) {
        TraceLog(LOG_WARNING, "dungeonExplore: no enemy called '%s' (check the data file), using the default", enemyId);
        enemy = enemyDefDefault();
    }
    if (!enemy) {
        TraceLog(LOG_ERROR, "dungeonExplore: no enemies are defined, check assets/data/enemies.txt");
        interactPromptOpen("No enemies are defined. (Check assets/data/enemies.txt.)", PROMPT_OK);
        return;
    }
    if (partyGet()->count == 0) {
        TraceLog(LOG_ERROR, "dungeonExplore: the party is empty, check assets/data/party.txt");
        interactPromptOpen("The party is empty. (Check assets/data/party.txt.)", PROMPT_OK);
        return;
    }
    battleMainStart(enemy);
}

static void applyOutcome(const EventOutcome *outcome) {
    if (!outcome) return; // none of the outcomes rolled: nothing happens

    switch (outcome->type) {
        case OUTCOME_MESSAGE:
            interactPromptOpen(outcome->text, PROMPT_OK);
            break;
        case OUTCOME_BATTLE:
            startBattle(outcome->text);
            break;
        case OUTCOME_ITEM:
            inventoryAdd(DROPPED_ITEM_ID, outcome->amount);
            if (outcome->text[0])
                interactPromptOpen(outcome->text, PROMPT_OK);
            else
                interactPromptOpen(TextFormat("You received %d item%s.", outcome->amount,
                                              outcome->amount == 1 ? "" : "s"), PROMPT_OK);
            break;
        case OUTCOME_NOTHING:
            break;
    }
}

// The player pressed interact on an event tile
static void startEvent(void) {
    const EventInfo *event = tileDataRollEvent(&tileData, player.gx, player.gy);
    if (event && event->once)
        worldMark(WORLD_EVENT_FIRED, currentLayer, currentMap, event->x, event->y, (int)(event - tileData.events));
    if (!event) {
        interactPromptOpen("You look around but find nothing.", PROMPT_OK);
        return;
    }

    switch (event->type) {
        case EVENT_MESSAGE:
            interactPromptOpen(event->text, PROMPT_OK);
            break;
        case EVENT_BATTLE:
            startBattle(event->text);
            break;
        case EVENT_ASK: {
            const char *labels[MAX_EVENT_OPTIONS];
            for (int i = 0; i < event->optionCount; i++) labels[i] = event->options[i].label;
            pending.type = PENDING_CHOICE;
            pending.event = event;
            interactPromptOpenChoice(event->text, labels, event->optionCount);
            break;
        }
    }
}

// --- The interact button ---

// What the interact button would do right now. Facing something beats standing on something.
static Opportunity findOpportunity(void) {
    Opportunity none = {OPPORTUNITY_NONE, INTERACT_NONE, {0, 0, 0, 0}, ""};
    if (player.phase != PH_IDLE) return none; // not while moving, turning or fading

    int dirX, dirY;
    playerFacing(&player, &dirX, &dirY);
    Target ahead = {player.gx + dirX, player.gy + dirY, dirX, dirY};

    InteractKind kind = interactionKind(&map, ahead.tileX, ahead.tileY, dirX, dirY);
    if (kind != INTERACT_NONE)
        return (Opportunity){OPPORTUNITY_FACING, kind, ahead, interactionLabel(kind)};

    if (mapTile(&map, player.gx, player.gy) == T_HEAL)
        return (Opportunity){OPPORTUNITY_HEAL, INTERACT_NONE, ahead, "Rest"};

    if (mapTile(&map, player.gx, player.gy) == T_EVENT && tileDataHasEvent(&tileData, player.gx, player.gy))
        return (Opportunity){OPPORTUNITY_EVENT, INTERACT_NONE, ahead, "Investigate"};

    return none;
}

static void restAtHealingSpot(void);

static void activate(Opportunity opportunity) {
    if (opportunity.type == OPPORTUNITY_FACING) {
        pending.type = PENDING_INTERACT;
        pending.kind = opportunity.kind;
        pending.target = opportunity.target;
        interactPromptOpen(interactionPrompt(opportunity.kind), PROMPT_YES_NO);
    } else if (opportunity.type == OPPORTUNITY_EVENT) {
        startEvent();
    } else if (opportunity.type == OPPORTUNITY_HEAL) {
        restAtHealingSpot();
    }
}

// Standing on a healing spot: everyone (the fallen included) goes back to full health
static void restAtHealingSpot(void) {
    const Party *party = partyGet();
    bool needed = false;
    for (int i = 0; i < party->count; i++)
        if (party->members[i].health < party->members[i].maxHealth) needed = true;

    if (!needed) {
        interactPromptOpen("Everyone is already in perfect health.", PROMPT_OK);
        return;
    }
    partyRestore();
    interactPromptOpen("A soothing light washes over the party. Everyone is fully healed.", PROMPT_OK);
}

// The popup above us closed. Act on the answer if there was something waiting for it.
static void resume(void) {
    waitForRelease = true; // W/S were just menu keys; do not walk off because they are still held

    PendingType type = pending.type;
    InteractKind kind = pending.kind;
    Target target = pending.target;
    const EventInfo *event = pending.event;
    pending.type = PENDING_NONE;

    if (type == PENDING_INTERACT) {
        if (interactPromptAccepted()) executeInteraction(kind, target);
    } else if (type == PENDING_CHOICE) {
        int choice = interactPromptChoice();
        if (event && choice >= 0 && choice < event->optionCount)
            applyOutcome(tileDataRollOption(&event->options[choice]));
    }
}

static void update(float dt) {
    if (player.phase == PH_IDLE && inputBackPressed()) {
        stateManagerPush(STATE_PAUSE_MENU);
        return;
    }

    Action action = inputPoll();
    if (waitForRelease) {
        if (action == ACT_NONE) waitForRelease = false;
        action = ACT_NONE;
    }

    playerUpdate(&player, &map, action, dt);

    if (player.phase == PH_BLACK) resolveTransition();

    if (inputInteractPressed()) activate(findOpportunity());
}

// The "interact" button: a key cap with the key name, then what pressing it does
static void drawInteractHint(int w, int h) {
    if (stateManagerCurrent() != STATE_DUNGEON_EXPLORE) return; // a popup is open on top of us

    Opportunity opportunity = findOpportunity();
    if (opportunity.type == OPPORTUNITY_NONE) return;

    const int fontSize = 24, padding = 14, keyPadding = 10, height = 44;
    const char *key = "SPACE";
    int keyWidth = MeasureText(key, fontSize) + 2 * keyPadding;
    int labelWidth = MeasureText(opportunity.label, fontSize);
    int totalWidth = padding + keyWidth + padding + labelWidth + padding;
    int x = (w - totalWidth) / 2, y = h - 100;

    DrawRectangle(x - 2, y - 2, totalWidth + 4, height + 4, (Color){225, 225, 235, 255});
    DrawRectangle(x, y, totalWidth, height, (Color){18, 18, 32, 235});
    DrawRectangle(x + padding, y + 7, keyWidth, height - 14, (Color){225, 225, 235, 255});
    DrawText(key, x + padding + keyPadding, y + (height - fontSize) / 2, fontSize, (Color){18, 18, 32, 255});
    DrawText(opportunity.label, x + padding + keyWidth + padding, y + (height - fontSize) / 2, fontSize,
             (Color){255, 215, 0, 255});
}

static void draw(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();

    rcRender(&rc, &map, player.posX, player.posY, player.angle);
    rcDraw(&rc, w, h);

    if (player.fade > 0.0f)
        DrawRectangle(0, 0, w, h, (Color){0, 0, 0, (unsigned char)(player.fade * 255)});

    drawInteractHint(w, h);

    DrawText(TextFormat("Layer %d - Map %d", currentLayer, currentMap), 12, 12, 24, WHITE);
    DrawText(TextFormat("Items: %d", inventoryItems()), 12, 42, 24, WHITE);
    DrawText("Esc: back", 12, h - 30, 20, (Color){200, 200, 200, 255});
    DrawFPS(w - 90, 8);
}

static void unload(void) {
    rcFree(&rc);
    tileDataFree(&tileData);
    mapFree(&map);
}

void dungeonExploreRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
        .unload = unload,
        .resume = resume,
        .drawBelow = false,
        .updateBelow = false,
    };
    stateManagerRegister(STATE_DUNGEON_EXPLORE, h);
}
