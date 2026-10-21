// STATE_BATTLE_MAIN. Damage rules live in combat/damage.c, stats in party/ and enemies/; this file only
// runs the turn order and draws it.
#include "core/states/battleMain.h"

#include "combat/damage.h"
#include "core/input.h"
#include "core/menuList.h"
#include "core/stateManager.h"
#include "party/party.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MESSAGES 8
#define MESSAGE_SIZE 160

typedef enum { MODE_COMMAND, MODE_MESSAGE } Mode;
typedef enum { NEXT_COMMAND, NEXT_ENEMY_TURN, NEXT_END } Next;
typedef enum { RESULT_NONE, RESULT_VICTORY, RESULT_DEFEAT, RESULT_FLED } Result;
typedef enum { COMMAND_ATTACK, COMMAND_RUN, COMMAND_COUNT } Command;
static const char *const commandLabels[COMMAND_COUNT] = {"Attack", "Run"};

// Layout (the window is 960x600)
#define SPRITE_TOP 80
#define SPRITE_BOTTOM 340
#define MESSAGE_Y 350
#define PANEL_Y 408
#define PARTY_ROW 30 // a full party of MAX_PARTY rows still fits above the bottom edge

static const EnemyDef *enemy;     // who we are fighting (set by battleMainStart, read in init)
static int enemyHealth;
static Texture2D enemyTexture;
static bool haveTexture;

static Mode mode;
static Next next;                 // what happens when the current messages have all been read
static Result result;
static int acting;                // party member choosing a command
static int commandSelected;

static char messages[MAX_MESSAGES][MESSAGE_SIZE];
static int messageCount, messageIndex;

void battleMainStart(const EnemyDef *def) {
    enemy = def;
    stateManagerPush(STATE_BATTLE_MAIN);
}

// --- Message queue ---

static void startMessages(void) {
    messageCount = 0;
    messageIndex = 0;
    mode = MODE_MESSAGE;
}

static void addMessage(const char *format, ...) {
    if (messageCount >= MAX_MESSAGES) return;
    va_list args;
    va_start(args, format);
    vsnprintf(messages[messageCount++], MESSAGE_SIZE, format, args);
    va_end(args);
}

// --- Turn logic ---

static void partyAttack(void) {
    Character *attacker = &partyGet()->members[acting];
    int dealt = damageAfterArmor((float)characterWeaponDamage(attacker), (float)enemy->armor);
    enemyHealth -= dealt;
    if (enemyHealth < 0) enemyHealth = 0;

    startMessages();
    addMessage("%s attacks! %d damage.", attacker->name, dealt);

    if (enemyHealth == 0) {
        addMessage("%s is defeated!", enemy->name);
        result = RESULT_VICTORY;
        next = NEXT_END;
        return;
    }

    int following = partyFirstLiving(partyGet(), acting + 1);
    if (following >= 0) {
        acting = following;
        next = NEXT_COMMAND;
    } else {
        next = NEXT_ENEMY_TURN;
    }
}

static void partyRun(void) {
    startMessages();
    addMessage("The party flees!");
    result = RESULT_FLED;
    next = NEXT_END;
}

static void enemyTurn(void) {
    Party *party = partyGet();
    startMessages();

    // Pick a random living target
    int target = -1;
    int pick = rand() % partyLivingCount(party);
    for (int i = 0; i < party->count; i++) {
        if (!characterIsAlive(&party->members[i])) continue;
        if (pick-- == 0) { target = i; break; }
    }
    Character *victim = &party->members[target];

    const EnemyAttack *attack = enemyPickAttack(enemy);
    int dealt = damageAfterArmor(enemyAttackDamage(enemy, attack), (float)characterArmor(victim));
    victim->health -= dealt;
    if (victim->health < 0) victim->health = 0;

    addMessage("%s uses %s on %s! %d damage.", enemy->name, attack->name, victim->name, dealt);
    if (!characterIsAlive(victim)) addMessage("%s falls!", victim->name);

    if (partyLivingCount(party) == 0) {
        addMessage("The party is wiped out...");
        result = RESULT_DEFEAT;
        next = NEXT_END;
    } else {
        acting = partyFirstLiving(party, 0);
        next = NEXT_COMMAND;
    }
}

// All queued messages were read: continue with whatever comes next
static void afterMessages(void) {
    switch (next) {
        case NEXT_COMMAND:
            mode = MODE_COMMAND;
            commandSelected = COMMAND_ATTACK;
            break;
        case NEXT_ENEMY_TURN:
            enemyTurn();
            break;
        case NEXT_END:
            if (result == RESULT_DEFEAT) stateManagerReplace(STATE_GAME_OVER); // the dungeon stays underneath
            else                         stateManagerPop();
            break;
    }
}

// --- State ---

static void init(void) {
    if (!enemy) enemy = enemyDefDefault();
    Party *party = partyGet();
    if (partyLivingCount(party) == 0) partyRestore(); // cannot fight with a dead party

    enemyHealth = enemy->maxHealth;
    result = RESULT_NONE;
    acting = partyFirstLiving(party, 0);

    enemyTexture = LoadTexture(enemy->texturePath);
    haveTexture = enemyTexture.id != 0;
    if (haveTexture) SetTextureFilter(enemyTexture, TEXTURE_FILTER_POINT);
    else TraceLog(LOG_WARNING, "battle: could not load '%s', drawing a placeholder", enemy->texturePath);

    startMessages();
    addMessage("%s appears!", enemy->name);
    next = NEXT_COMMAND;
}

static void unload(void) {
    if (haveTexture) UnloadTexture(enemyTexture);
    haveTexture = false;
}

static void update(float dt) {
    (void)dt;
    MenuAction action = inputMenu();

    if (mode == MODE_MESSAGE) {
        if (action == MENU_CONFIRM && ++messageIndex >= messageCount) afterMessages();
        return;
    }

    commandSelected = menuListMove(commandSelected, COMMAND_COUNT, action);
    if (action != MENU_CONFIRM) return;

    if (commandSelected == COMMAND_ATTACK) partyAttack();
    else                                    partyRun();
}

// --- Drawing ---

static const Color panelBorder = {225, 225, 235, 255};
static const Color panelFill = {18, 18, 32, 245};
static const Color gold = {255, 215, 0, 255};
static const Color dim = {150, 150, 165, 255};

static void drawPanel(int x, int y, int w, int h) {
    DrawRectangle(x - 3, y - 3, w + 6, h + 6, panelBorder);
    DrawRectangle(x, y, w, h, panelFill);
}

static void drawBar(int x, int y, int w, int h, int value, int maxValue, Color fill) {
    DrawRectangle(x, y, w, h, (Color){50, 50, 65, 255});
    if (maxValue > 0 && value > 0) DrawRectangle(x, y, w * value / maxValue, h, fill);
}

static void drawEnemy(int w) {
    // Name, health and armor
    DrawText(enemy->name, w / 2 - MeasureText(enemy->name, 28) / 2, 12, 28, WHITE);
    drawBar(w / 2 - 150, 44, 300, 10, enemyHealth, enemy->maxHealth, (Color){200, 60, 60, 255});
    const char *info = TextFormat("HP %d/%d    Armor %d (%d%% damage reduction)", enemyHealth, enemy->maxHealth,
                                  enemy->armor, (int)(armorDamageReduction((float)enemy->armor) * 100.0f + 0.5f));
    DrawText(info, w / 2 - MeasureText(info, 18) / 2 - 0, 58 - 2, 18, dim);

    // The picture: whole-number scale so pixels stay sharp, centered, standing on the ground line.
    // Drawn with normal alpha blending, so PNGs with transparent backgrounds show the scene behind them.
    if (haveTexture) {
        int scale = (SPRITE_BOTTOM - SPRITE_TOP) / enemyTexture.height;
        if (scale < 1) scale = 1;
        float dw = (float)(enemyTexture.width * scale), dh = (float)(enemyTexture.height * scale);
        Rectangle source = {0, 0, (float)enemyTexture.width, (float)enemyTexture.height};
        Rectangle dest = {w / 2 - dw / 2, SPRITE_BOTTOM - dh, dw, dh};
        DrawTexturePro(enemyTexture, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangle(w / 2 - 60, SPRITE_BOTTOM - 160, 120, 160, (Color){90, 90, 110, 255});
        DrawText("?", w / 2 - 8, SPRITE_BOTTOM - 100, 40, WHITE);
    }
}

static void drawParty(void) {
    Party *party = partyGet();
    drawPanel(30, PANEL_Y, 540, 22 + party->count * PARTY_ROW);

    for (int i = 0; i < party->count; i++) {
        const Character *c = &party->members[i];
        int y = PANEL_Y + 10 + i * PARTY_ROW;
        bool isActing = (mode == MODE_COMMAND && i == acting);
        Color nameColor = !characterIsAlive(c) ? dim : (isActing ? gold : WHITE);

        DrawText(isActing ? ">" : " ", 40, y, 22, gold);
        DrawText(c->name, 62, y, 22, nameColor);
        DrawText(c->className, 150, y + 3, 18, dim);
        drawBar(250, y + 4, 150, 14, c->health, c->maxHealth, (Color){70, 190, 90, 255});
        DrawText(TextFormat("%d/%d", c->health, c->maxHealth), 410, y, 22, nameColor);
        DrawText(TextFormat("AR %d", characterArmor(c)), 495, y + 3, 18, dim);
    }
}

static void drawCommands(void) {
    drawPanel(600, PANEL_Y, 330, 22 + COMMAND_COUNT * 40);
    for (int i = 0; i < COMMAND_COUNT; i++) {
        const char *text = (i == commandSelected) ? TextFormat("> %s", commandLabels[i]) : commandLabels[i];
        DrawText(text, 625, PANEL_Y + 12 + i * 40, 26, i == commandSelected ? gold : dim);
    }
}

static void draw(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();

    DrawRectangle(0, 0, w, h, (Color){24, 22, 36, 255});
    DrawRectangle(0, SPRITE_BOTTOM, w, h - SPRITE_BOTTOM, (Color){38, 34, 52, 255}); // ground

    drawEnemy(w);

    // Message line
    drawPanel(30, MESSAGE_Y, w - 60, 44);
    if (mode == MODE_MESSAGE) {
        DrawText(messages[messageIndex], 48, MESSAGE_Y + 10, 24, WHITE);
        DrawText("Enter", w - 110, MESSAGE_Y + 14, 18, dim);
    } else {
        const char *turn = TextFormat("%s's turn", partyGet()->members[acting].name);
        DrawText(turn, 48, MESSAGE_Y + 10, 24, WHITE);
    }

    drawParty();
    if (mode == MODE_COMMAND) drawCommands();
}

void battleMainRegister(void) {
    StateHandlers h = {
        .init = init,
        .update = update,
        .draw = draw,
        .unload = unload,
        .drawBelow = false,
        .updateBelow = false,
    };
    stateManagerRegister(STATE_BATTLE_MAIN, h);
}
