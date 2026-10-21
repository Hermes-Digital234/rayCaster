// character.h - one member of the party: name, class label, health and what they are wearing.
//
// A character stores only its own base values. Everything derived from gear (attack damage, armor) is
// computed from the equipped items on demand, so changing equipment in a menu later just works.
// The class is only a label for now (any word the data file uses); it has no stat effect yet.
#ifndef CHARACTER_H
#define CHARACTER_H

#include "party/item.h"
#include <stdbool.h>

typedef struct {
    char name[24];
    char className[24];
    int maxHealth;
    int health;
    const ItemDef *equipment[SLOT_COUNT]; // NULL = empty slot
} Character;

// Restores up to `amount` health to a living character. Returns how much was really restored (0 if dead or full).
int characterHeal(Character *c, int amount);
void characterInit(Character *c, const char *name, const char *className, int maxHealth);

// Puts an item in a slot (NULL empties it). Returns false and changes nothing if the item does not fit there.
bool characterEquip(Character *c, EquipSlot slot, const ItemDef *item);

// Damage of one attack before the target's armor: the weapon's damage, or 1 when bare-handed.
int characterWeaponDamage(const Character *c);

// Total armor of everything equipped.
int characterArmor(const Character *c);

bool characterIsAlive(const Character *c);

#endif
