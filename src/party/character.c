#include "party/character.h"
#include <stdio.h>
#include <string.h>

void characterInit(Character *c, const char *name, const char *className, int maxHealth) {
    memset(c, 0, sizeof(*c));
    snprintf(c->name, sizeof(c->name), "%s", name);
    snprintf(c->className, sizeof(c->className), "%s", className);
    c->maxHealth = maxHealth;
    c->health = maxHealth;
}

bool characterEquip(Character *c, EquipSlot slot, const ItemDef *item) {
    if (slot < 0 || slot >= SLOT_COUNT) return false;
    if (item && !itemFitsSlot(item, slot)) return false;
    c->equipment[slot] = item;
    return true;
}

int characterWeaponDamage(const Character *c) {
    const ItemDef *weapon = c->equipment[SLOT_WEAPON];
    return weapon ? weapon->damage : 1;
}

int characterArmor(const Character *c) {
    int total = 0;
    for (int i = 0; i < SLOT_COUNT; i++)
        if (c->equipment[i]) total += c->equipment[i]->armor;
    return total;
}

bool characterIsAlive(const Character *c) {
    return c->health > 0;
}

int characterHeal(Character *c, int amount) {
    if (!c || c->health <= 0 || amount <= 0) return 0;
    int room = c->maxHealth - c->health;
    int healed = amount < room ? amount : room;
    c->health += healed;
    return healed;
}
