#include "enemies/enemy.h"
#include <stdlib.h>
#include <string.h>

static const EnemyDef enemies[] = {
    {
        .id = "basic",
        .name = "Basic Enemy",
        .maxHealth = 100,
        .baseDamage = 10,
        .armor = 100,
        .texturePath = "assets/textures/enemies/basic.png",
        .attacks = {
            {"Strike", 1.0f, 70},      // basic attack: 1x base damage
            {"Heavy Swing", 1.5f, 30}, // heavy attack: 1.5x base damage
        },
        .attackCount = 2,
    },
};

const EnemyDef *enemyDefFind(const char *id) {
    for (size_t i = 0; i < sizeof(enemies) / sizeof(enemies[0]); i++)
        if (!strcmp(enemies[i].id, id)) return &enemies[i];
    return NULL;
}

const EnemyDef *enemyDefDefault(void) {
    return &enemies[0];
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
