#include "party/character.h"
#include <stdio.h>
#include <string.h>

void characterInit(Character *c, const char *name, CharacterClass cls, int maxHealth) {
    memset(c, 0, sizeof(*c));
    snprintf(c->name, sizeof(c->name), "%s", name);
    c->cls = cls;
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

const char *characterClassName(CharacterClass cls) {
    switch (cls) {
        case CLASS_WARRIOR: return "Warrior";
        case CLASS_ROGUE:   return "Rogue";
        case CLASS_MAGE:    return "Mage";
        case CLASS_HEALER:  return "Healer";
        case CLASS_COUNT:   break;
    }
    return "";
}
