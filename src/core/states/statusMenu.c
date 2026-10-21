#include "core/states/statusMenu.h"

#include "combat/damage.h"
#include "core/input.h"
#include "core/menuList.h"
#include "core/stateManager.h"
#include "party/inventory.h"
#include "party/party.h"
#include "raylib.h"

static int selected;

static const Color panelBorder = {225, 225, 235, 255};
static const Color panelFill = {18, 18, 32, 245};
static const Color gold = {255, 215, 0, 255};
static const Color dim = {150, 150, 165, 255};

static void init(void) {
    selected = 0;
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();
    selected = menuListMove(selected, partyGet()->count, action);
    if (action == MENU_BACK || action == MENU_CONFIRM) stateManagerPop();
}

static void drawPanel(int x, int y, int w, int h) {
    DrawRectangle(x - 3, y - 3, w + 6, h + 6, panelBorder);
    DrawRectangle(x, y, w, h, panelFill);
}

static void drawBar(int x, int y, int w, int h, int value, int maxValue) {
    DrawRectangle(x, y, w, h, (Color){50, 50, 65, 255});
    if (maxValue > 0 && value > 0) DrawRectangle(x, y, w * value / maxValue, h, (Color){70, 190, 90, 255});
}

static const char *itemSummary(const ItemDef *item) {
    if (!item) return "-";
    if (item->damage > 0 && item->armor > 0) return TextFormat("%s (+%d damage, +%d armor)", item->name, item->damage, item->armor);
    if (item->damage > 0)                    return TextFormat("%s (+%d damage)", item->name, item->damage);
    if (item->armor > 0)                     return TextFormat("%s (+%d armor)", item->name, item->armor);
    return item->name;
}

static void draw(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    Party *party = partyGet();

    DrawRectangle(0, 0, w, h, (Color){24, 22, 36, 255});
    DrawText("Party", 40, 24, 36, WHITE);

    // Left: the members
    drawPanel(40, 90, 340, 20 + MAX_PARTY * 64);
    for (int i = 0; i < party->count; i++) {
        const Character *c = &party->members[i];
        int y = 104 + i * 64;
        Color color = i == selected ? gold : (characterIsAlive(c) ? WHITE : dim);
        DrawText(i == selected ? TextFormat("> %s", c->name) : c->name, 58, y, 24, color);
        DrawText(c->className, 58, y + 28, 16, dim);
        drawBar(190, y + 8, 150, 12, c->health, c->maxHealth);
        DrawText(TextFormat("%d/%d", c->health, c->maxHealth), 190, y + 26, 18, color);
    }
    if (party->count == 0) DrawText("The party is empty.", 58, 104, 22, dim);

    // Right: details of the selected member
    drawPanel(420, 90, 500, 20 + MAX_PARTY * 64);
    if (party->count > 0) {
        const Character *c = &party->members[selected];
        int armor = characterArmor(c);
        int x = 440, y = 106;

        DrawText(TextFormat("%s  -  %s", c->name, c->className), x, y, 26, gold);
        y += 44;
        DrawText(TextFormat("Health          %d / %d", c->health, c->maxHealth), x, y, 22, WHITE); y += 30;
        DrawText(TextFormat("Attack damage   %d", characterWeaponDamage(c)), x, y, 22, WHITE); y += 30;
        DrawText(TextFormat("Armor           %d", armor), x, y, 22, WHITE); y += 30;
        DrawText(TextFormat("Damage reduction  %d%%", (int)(armorDamageReduction((float)armor) * 100.0f + 0.5f)), x, y, 22, WHITE); y += 30;
        DrawText(TextFormat("Effective health  %d", (int)(armorEffectiveHealth((float)c->maxHealth, (float)armor) + 0.5f)), x, y, 22, WHITE);
        y += 50;

        for (int slot = 0; slot < SLOT_COUNT; slot++) {
            DrawText(TextFormat("%-7s", equipSlotName((EquipSlot)slot)), x, y, 20, dim);
            DrawText(itemSummary(c->equipment[slot]), x + 100, y, 20, WHITE);
            y += 28;
        }
    }

    DrawText(TextFormat("Items collected: %d", inventoryItems()), 40, h - 70, 22, WHITE);
    DrawText("Up/Down: choose    Esc: back", 40, h - 36, 20, dim);
}

void statusMenuRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
    };
    stateManagerRegister(STATE_STATUS_MENU, h);
}
