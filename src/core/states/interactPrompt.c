// STATE_INTERACT_PROMPT: a popup drawn over the state beneath it, which is frozen while it is open.
#include "core/states/interactPrompt.h"

#include "core/input.h"
#include "core/menuList.h"
#include "core/stateManager.h"
#include "core/textWrap.h"
#include "raylib.h"

#include <stdio.h>

#define PANEL_WIDTH 640
#define PANEL_MARGIN 24
#define TEXT_SIZE 26
#define LINE_HEIGHT 34
#define OPTION_ROW 40

static char message[256];
static PromptKind kind = PROMPT_YES_NO;
static int selected;  // Yes/No: 0 = Yes, 1 = No. Choice menu: the highlighted option.
static bool accepted;
static int chosen;    // choice menu: the option that was confirmed

static char choiceLabels[PROMPT_MAX_CHOICES][64];
static int choiceCount;

void interactPromptOpen(const char *text, PromptKind promptKind) {
    snprintf(message, sizeof(message), "%s", text);
    kind = promptKind;
    stateManagerPush(STATE_INTERACT_PROMPT);
}

void interactPromptOpenChoice(const char *text, const char *const *labels, int count) {
    if (count < 1) count = 1;
    if (count > PROMPT_MAX_CHOICES) count = PROMPT_MAX_CHOICES;
    choiceCount = count;
    for (int i = 0; i < count; i++) snprintf(choiceLabels[i], sizeof(choiceLabels[i]), "%s", labels[i]);

    snprintf(message, sizeof(message), "%s", text);
    kind = PROMPT_CHOICE;
    stateManagerPush(STATE_INTERACT_PROMPT);
}

bool interactPromptAccepted(void) {
    return accepted;
}

int interactPromptChoice(void) {
    return chosen;
}

static void init(void) {
    selected = 0; // Yes (or the first option) is highlighted first
    accepted = false;
    chosen = 0;
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();

    switch (action) {
        case MENU_LEFT:
        case MENU_RIGHT:
            if (kind == PROMPT_YES_NO) selected = 1 - selected;
            break;
        case MENU_UP:
        case MENU_DOWN:
            if (kind == PROMPT_YES_NO) selected = 1 - selected;
            else if (kind == PROMPT_CHOICE) selected = menuListMove(selected, choiceCount, action);
            break;
        case MENU_CONFIRM:
            if (kind == PROMPT_CHOICE) {
                chosen = selected;
                accepted = true;
            } else {
                accepted = (kind == PROMPT_OK) || selected == 0;
            }
            stateManagerPop();
            break;
        case MENU_BACK:
            if (kind == PROMPT_CHOICE) break; // must pick an option
            accepted = false;
            stateManagerPop();
            break;
        case MENU_NONE:
            break;
    }
}

static void drawOption(const char *label, int centerX, int y, bool isSelected) {
    const char *text = isSelected ? TextFormat("> %s <", label) : label;
    Color color = isSelected ? (Color){255, 215, 0, 255} : (Color){170, 170, 185, 255};
    DrawText(text, centerX - MeasureText(text, TEXT_SIZE) / 2, y, TEXT_SIZE, color);
}

static void draw(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    int textWidth = PANEL_WIDTH - 2 * PANEL_MARGIN;

    int lines = textWrap(message, 0, 0, textWidth, TEXT_SIZE, LINE_HEIGHT, false, WHITE);
    int optionRows = (kind == PROMPT_CHOICE) ? choiceCount : 1;
    int panelHeight = 2 * PANEL_MARGIN + lines * LINE_HEIGHT + optionRows * OPTION_ROW + 10;
    int panelX = (w - PANEL_WIDTH) / 2;
    int panelY = h - panelHeight - 50;

    DrawRectangle(panelX - 3, panelY - 3, PANEL_WIDTH + 6, panelHeight + 6, (Color){225, 225, 235, 255});
    DrawRectangle(panelX, panelY, PANEL_WIDTH, panelHeight, (Color){18, 18, 32, 245});

    textWrap(message, panelX + PANEL_MARGIN, panelY + PANEL_MARGIN, textWidth, TEXT_SIZE, LINE_HEIGHT, true, WHITE);

    int optionsY = panelY + PANEL_MARGIN + lines * LINE_HEIGHT + 10;
    int centerX = panelX + PANEL_WIDTH / 2;
    if (kind == PROMPT_YES_NO) {
        drawOption("Yes", centerX - 90, optionsY, selected == 0);
        drawOption("No", centerX + 90, optionsY, selected == 1);
    } else if (kind == PROMPT_CHOICE) {
        for (int i = 0; i < choiceCount; i++)
            drawOption(choiceLabels[i], centerX, optionsY + i * OPTION_ROW, selected == i);
    } else {
        drawOption("OK", centerX, optionsY, true);
    }
}

void interactPromptRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
        .drawBelow = true,   // the dungeon stays visible behind the popup
        .updateBelow = false, // ...but frozen
    };
    stateManagerRegister(STATE_INTERACT_PROMPT, h);
}
