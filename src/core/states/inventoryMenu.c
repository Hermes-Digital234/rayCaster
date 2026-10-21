// STATE_INVENTORY_MENU: list of item stacks on the left. Enter on a consumable asks who should use it.
#include "core/states/inventoryMenu.h"

#include "core/input.h"
#include "core/menuList.h"
#include "core/stateManager.h"
#include "party/inventory.h"
#include "party/item.h"
#include "party/party.h"
#include "raylib.h"

#include <stdio.h>
#include <string.h>

#define VISIBLE_ROWS 11

typedef enum { MODE_LIST, MODE_TARGET } Mode;

static Mode mode;
static int selected;   // stack in the list
static int target;     // party member while choosing who uses the item
static int scroll;
static char message[96];

static const Color panelBorder = {225, 225, 235, 255};
static const Color panelFill = {18, 18, 32, 245};
static const Color gold = {255, 215, 0, 255};
static const Color dim = {150, 150, 165, 255};

static void init(void) {
    mode = MODE_LIST;
    selected = 0;
    target = 0;
    scroll = 0;
    message[0] = '\0';
}

static const ItemDef *selectedDef(void) {
    const InventoryStack *stack = inventoryStackAt(selected);
    return stack ? itemDefFind(stack->id) : NULL;
}

static void useOnTarget(void) {
    const InventoryStack *stack = inventoryStackAt(selected);
    const ItemDef *def = selectedDef();
    Character *who = &partyGet()->members[target];
    if (!stack || !def) return;

    if (!characterIsAlive(who)) {
        snprintf(message, sizeof(message), "%s has fallen and can't use that.", who->name);
        return;
    }
    int healed = characterHeal(who, def->heal);
    if (healed <= 0) {
        snprintf(message, sizeof(message), "%s is already at full health.", who->name);
        return;
    }
    char id[sizeof(stack->id)];
    strcpy(id, stack->id);
    inventoryRemove(id, 1);
    snprintf(message, sizeof(message), "%s recovers %d health.", who->name, healed);

    // the stack may be gone now
    if (selected >= inventoryStackCount()) selected = inventoryStackCount() - 1;
    if (selected < 0) selected = 0;
    if (!inventoryStackAt(selected) || !selectedDef() || selectedDef()->kind != ITEM_KIND_CONSUMABLE ||
        inventoryCount(id) == 0)
        mode = MODE_LIST;
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();
    int stacks = inventoryStackCount();

    if (mode == MODE_LIST) {
        if (action == MENU_BACK) { stateManagerPop(); return; }
        int before = selected;
        selected = menuListMove(selected, stacks, action);
        if (selected != before) message[0] = '\0';
        if (selected < scroll) scroll = selected;
        if (selected >= scroll + VISIBLE_ROWS) scroll = selected - VISIBLE_ROWS + 1;

        if (action == MENU_CONFIRM && stacks > 0) {
            const ItemDef *def = selectedDef();
            if (def && def->kind == ITEM_KIND_CONSUMABLE && partyGet()->count > 0) {
                mode = MODE_TARGET;
                target = 0;
                message[0] = '\0';
            } else {
                snprintf(message, sizeof(message), "That can't be used here.");
            }
        }
    } else {
        if (action == MENU_BACK) { mode = MODE_LIST; message[0] = '\0'; return; }
        target = menuListMove(target, partyGet()->count, action);
        if (action == MENU_CONFIRM) useOnTarget();
    }
}

static void drawPanel(int x, int y, int w, int h) {
    DrawRectangle(x - 3, y - 3, w + 6, h + 6, panelBorder);
    DrawRectangle(x, y, w, h, panelFill);
}

static const char *describe(const ItemDef *def) {
    if (!def) return "Unknown item (not in items.txt).";
    switch (def->kind) {
        case ITEM_KIND_CONSUMABLE:
            if (def->heal >= ITEM_HEAL_FULL) return "Fully restores one party member's health.";
            return TextFormat("Restores %d health to one party member.", def->heal);
        case ITEM_KIND_WEAPON: return TextFormat("Weapon: +%d damage.", def->damage);
        case ITEM_KIND_ARMOR:  return TextFormat("Armor: +%d armor.", def->armor);
        case ITEM_KIND_CHARM:  return "A charm.";
    }
    return "";
}

static void draw(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    Party *party = partyGet();

    DrawRectangle(0, 0, w, h, (Color){8, 8, 16, 255});
    DrawText("Inventory", 40, 24, 36, WHITE);

    int listX = 40, listY = 90, listW = w / 2 - 60, listH = VISIBLE_ROWS * 34 + 20;
    drawPanel(listX, listY, listW, listH);

    int stacks = inventoryStackCount();
    if (stacks == 0) DrawText("You are not carrying anything.", listX + 18, listY + 18, 22, dim);
    for (int row = 0; row < VISIBLE_ROWS; row++) {
        int i = scroll + row;
        if (i >= stacks) break;
        const InventoryStack *stack = inventoryStackAt(i);
        const ItemDef *def = itemDefFind(stack->id);
        const char *name = def ? def->name : stack->id;
        int y = listY + 12 + row * 34;
        Color color = (i == selected) ? gold : WHITE;
        if (i == selected) DrawText(">", listX + 10, y, 24, gold);
        DrawText(name, listX + 34, y, 24, color);
        const char *count = TextFormat("x%d", stack->count);
        DrawText(count, listX + listW - 18 - MeasureText(count, 24), y, 24, color);
    }
    if (scroll > 0) DrawText("^", listX + listW - 26, listY + 2, 18, dim);
    if (scroll + VISIBLE_ROWS < stacks) DrawText("v", listX + listW - 26, listY + listH - 24, 18, dim);

    // The party, so you can see who needs healing
    int partyX = w / 2 + 20, partyW = w / 2 - 60;
    drawPanel(partyX, listY, partyW, listH);
    for (int i = 0; i < party->count; i++) {
        const Character *c = &party->members[i];
        int y = listY + 14 + i * 56;
        Color color = !characterIsAlive(c) ? dim : WHITE;
        if (mode == MODE_TARGET && i == target) { color = gold; DrawText(">", partyX + 10, y, 24, gold); }
        DrawText(c->name, partyX + 34, y, 24, color);
        DrawText(TextFormat("%d/%d", c->health, c->maxHealth), partyX + partyW - 120, y, 22, color);
        DrawRectangle(partyX + 34, y + 32, partyW - 68, 8, (Color){50, 50, 65, 255});
        if (c->maxHealth > 0 && c->health > 0)
            DrawRectangle(partyX + 34, y + 32, (partyW - 68) * c->health / c->maxHealth, 8, (Color){70, 190, 90, 255});
    }

    int infoY = listY + listH + 24;
    if (stacks > 0) DrawText(describe(selectedDef()), 40, infoY, 22, WHITE);
    if (message[0]) DrawText(message, 40, infoY + 34, 22, gold);

    DrawText(mode == MODE_TARGET ? "Up/Down: choose who    Enter: use    Esc: cancel" : "Up/Down: choose    Enter: use    Esc: back",
             40, h - 36, 20, dim);
}

void inventoryMenuRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
    };
    stateManagerRegister(STATE_INVENTORY_MENU, h);
}