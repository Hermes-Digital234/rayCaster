// enemy.h - enemy blueprints, loaded from assets/data/enemies.txt. An EnemyDef is read-only data; a battle keeps
// its own current health.
//
// File format (# for comments):
//
//   enemy <id> health=<n> damage=<n> armor=<n> texture=<png path> <display name>
//   attack <enemyId> [multiplier=<x>] [weight=<n>] <attack name>
//
//   id           a one-word name used by the map data files (`battle <id>`)
//   damage       the enemy's base damage; an attack deals base damage * its multiplier
//   texture      PNG relative to the executable (transparent backgrounds are supported)
//   multiplier   defaults to 1.0
//   weight       how often the attack is picked compared to the enemy's other attacks (default 1)
//   The names are the rest of the line and may contain spaces. `attack` lines must come after their `enemy` line.
//   An enemy with no attack lines gets one plain attack (1x damage).
//
//   enemy basic health=100 damage=10 armor=100 texture=assets/textures/enemies/basic.png Basic Enemy
//   attack basic multiplier=1.0 weight=70 Strike
//   attack basic multiplier=1.5 weight=30 Heavy Swing
#ifndef ENEMY_H
#define ENEMY_H

#define ENEMIES_FILE "assets/data/enemies.txt"
#define MAX_ENEMIES 64
#define MAX_ENEMY_ATTACKS 4

typedef struct {
    char name[48];
    float multiplier; // damage = the enemy's baseDamage * multiplier, before the target's armor
    int weight;
} EnemyAttack;

typedef struct {
    char id[32];
    char name[48];
    int maxHealth;
    int baseDamage;
    int armor;
    char texturePath[128];
    EnemyAttack attacks[MAX_ENEMY_ATTACKS];
    int attackCount;
} EnemyDef;

// Replaces the enemy table with the contents of the file. Returns the number of enemies loaded, or -1 if the
// file could not be read. Bad lines are reported on stderr and skipped.
int enemyDbLoad(const char *path);

int enemyDbCount(void);

// Looks an enemy up by id. NULL if there is none.
const EnemyDef *enemyDefFind(const char *id);

// The enemy used when a data file names one that does not exist: the first one in the file. NULL if none loaded.
const EnemyDef *enemyDefDefault(void);

// Picks one of the enemy's attacks at random, by weight.
const EnemyAttack *enemyPickAttack(const EnemyDef *def);

// Damage of an attack before the target's armor.
float enemyAttackDamage(const EnemyDef *def, const EnemyAttack *attack);

#endif
