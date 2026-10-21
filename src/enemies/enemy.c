#include "enemies/enemy.h"

#include "core/textParse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static EnemyDef enemies[MAX_ENEMIES];
static int enemyCount;

static EnemyDef *findMutable(const char *id) {
    for (int i = 0; i < enemyCount; i++)
        if (!strcmp(enemies[i].id, id)) return &enemies[i];
    return NULL;
}

typedef struct { const char *path; } LoadContext;

static void parseEnemy(char *line, const char *path, int lineNumber) {
    char token[160];
    EnemyDef enemy;
    memset(&enemy, 0, sizeof(enemy));
    enemy.maxHealth = 1;

    if (!textNextToken(&line, enemy.id, sizeof(enemy.id))) { textWarn("enemies", path, lineNumber, "enemy needs an id"); return; }
    if (findMutable(enemy.id)) { textWarn("enemies", path, lineNumber, "an enemy with this id already exists"); return; }

    for (;;) {
        char *save = line;
        if (!textNextToken(&line, token, sizeof(token))) break;
        const char *value;
        if (textKeyValue(token, "health", &value)) {
            if (!textParseInt(value, &enemy.maxHealth) || enemy.maxHealth < 1) { textWarn("enemies", path, lineNumber, "health must be at least 1"); return; }
        } else if (textKeyValue(token, "damage", &value)) {
            if (!textParseInt(value, &enemy.baseDamage) || enemy.baseDamage < 0) { textWarn("enemies", path, lineNumber, "damage must be 0 or more"); return; }
        } else if (textKeyValue(token, "armor", &value)) {
            if (!textParseInt(value, &enemy.armor) || enemy.armor < 0) { textWarn("enemies", path, lineNumber, "armor must be 0 or more"); return; }
        } else if (textKeyValue(token, "texture", &value)) {
            snprintf(enemy.texturePath, sizeof(enemy.texturePath), "%s", value);
        } else {
            line = save; // not an option: the rest of the line is the name
            break;
        }
    }

    char *name = textSkipSpaces(line);
    snprintf(enemy.name, sizeof(enemy.name), "%s", *name ? name : enemy.id);

    if (enemyCount >= MAX_ENEMIES) { textWarn("enemies", path, lineNumber, "too many enemies"); return; }
    enemies[enemyCount++] = enemy;
}

static void parseAttack(char *line, const char *path, int lineNumber) {
    char token[160];
    if (!textNextToken(&line, token, sizeof(token))) { textWarn("enemies", path, lineNumber, "attack needs an enemy id"); return; }
    EnemyDef *enemy = findMutable(token);
    if (!enemy) { textWarn("enemies", path, lineNumber, "no enemy with this id (attack lines come after their enemy line)"); return; }
    if (enemy->attackCount >= MAX_ENEMY_ATTACKS) { textWarn("enemies", path, lineNumber, "too many attacks for this enemy"); return; }

    EnemyAttack attack;
    memset(&attack, 0, sizeof(attack));
    attack.multiplier = 1.0f;
    attack.weight = 1;

    for (;;) {
        char *save = line;
        if (!textNextToken(&line, token, sizeof(token))) break;
        const char *value;
        if (textKeyValue(token, "multiplier", &value)) {
            if (!textParseFloat(value, &attack.multiplier) || attack.multiplier < 0.0f) { textWarn("enemies", path, lineNumber, "multiplier must be 0 or more"); return; }
        } else if (textKeyValue(token, "weight", &value)) {
            if (!textParseInt(value, &attack.weight) || attack.weight < 1) { textWarn("enemies", path, lineNumber, "weight must be at least 1"); return; }
        } else {
            line = save;
            break;
        }
    }

    char *name = textSkipSpaces(line);
    if (*name == '\0') { textWarn("enemies", path, lineNumber, "attack needs a name"); return; }
    snprintf(attack.name, sizeof(attack.name), "%s", name);
    enemy->attacks[enemy->attackCount++] = attack;
}

static void parseLine(char *line, int lineNumber, void *user) {
    const char *path = ((LoadContext *)user)->path;
    char kind[32];
    textNextToken(&line, kind, sizeof(kind));
    if (!strcmp(kind, "enemy"))       parseEnemy(line, path, lineNumber);
    else if (!strcmp(kind, "attack")) parseAttack(line, path, lineNumber);
    else                              textWarn("enemies", path, lineNumber, "unknown entry (expected enemy or attack)");
}

int enemyDbLoad(const char *path) {
    enemyCount = 0;
    LoadContext context = {path};
    if (!textForEachLine(path, parseLine, &context)) {
        fprintf(stderr, "enemies: could not read %s\n", path);
        return -1;
    }

    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].attackCount == 0) {
            fprintf(stderr, "enemies: %s: enemy '%s' has no attack lines, giving it a plain attack\n", path, enemies[i].id);
            snprintf(enemies[i].attacks[0].name, sizeof(enemies[i].attacks[0].name), "Attack");
            enemies[i].attacks[0].multiplier = 1.0f;
            enemies[i].attacks[0].weight = 1;
            enemies[i].attackCount = 1;
        }
    }
    return enemyCount;
}

int enemyDbCount(void) { return enemyCount; }

const EnemyDef *enemyDefFind(const char *id) {
    return findMutable(id);
}

const EnemyDef *enemyDefDefault(void) {
    return enemyCount > 0 ? &enemies[0] : NULL;
}

const EnemyAttack *enemyPickAttack(const EnemyDef *def) {
    int total = 0;
    for (int i = 0; i < def->attackCount; i++) total += def->attacks[i].weight;
    if (total <= 0) return &def->attacks[0];

    int roll = rand() % total;
    for (int i = 0; i < def->attackCount; i++) {
        if (roll < def->attacks[i].weight) return &def->attacks[i];
        roll -= def->attacks[i].weight;
    }
    return &def->attacks[0];
}

float enemyAttackDamage(const EnemyDef *def, const EnemyAttack *attack) {
    return (float)def->baseDamage * attack->multiplier;
}
