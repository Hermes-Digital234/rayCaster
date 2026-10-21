// enemy.h - enemy blueprints. An EnemyDef is read-only data; a battle keeps its own current health.
//
// Enemies are referred to by id in the map data files (`battle basic`). The table lives in enemy.c.
#ifndef ENEMY_H
#define ENEMY_H

#define MAX_ENEMY_ATTACKS 4

typedef struct {
    const char *name;
    float multiplier; // damage = the enemy's baseDamage * multiplier, before the target's armor
    int weight;       // how often it is picked compared to the enemy's other attacks
} EnemyAttack;

typedef struct {
    const char *id;
    const char *name;
    int maxHealth;
    int baseDamage;
    int armor;
    const char *texturePath;  // PNG, relative to the executable. Transparent backgrounds are supported.
    EnemyAttack attacks[MAX_ENEMY_ATTACKS];
    int attackCount;
} EnemyDef;

// Looks an enemy up by id. NULL if there is none.
const EnemyDef *enemyDefFind(const char *id);

// The enemy used when a data file names one that does not exist.
const EnemyDef *enemyDefDefault(void);

// Picks one of the enemy's attacks at random, by weight.
const EnemyAttack *enemyPickAttack(const EnemyDef *def);

// Damage of an attack before the target's armor.
float enemyAttackDamage(const EnemyDef *def, const EnemyAttack *attack);

#endif
