// character.h - one member of the party: name, class, health and what they are wearing.
//
// A character stores only its own base values. Everything derived from gear (attack damage, armor) is
// computed from the equipped items on demand, so changing equipment in a menu later just works.
#ifndef CHARACTER_H
#define CHARACTER_H

#include "party/item.h"
#include <stdbool.h>

// Placeholder generic classes. They carry no stat differences yet.
typedef enum {
    CLASS_WARRIOR,
    CLASS_ROGUE,
    CLASS_MAGE,
    CLASS_HEALER,
    CLASS_COUNT
} CharacterClass;

typedef struct {
    char name[24];
    CharacterClass cls;
    int maxHealth;
    int health;
    const ItemDef *equipment[SLOT_COUNT]; // NULL = empty slot
} Character;

void characterInit(Character *c, const char *name, CharacterClass cls, int maxHealth);

// Puts an item in a slot (NULL empties it). Returns false and changes nothing if the item does not fit there.
bool characterEquip(Character *c, EquipSlot slot, const ItemDef *item);

// Damage of one attack before the target's armor: the weapon's damage, or 1 when bare-handed.
int characterWeaponDamage(const Character *c);

// Total armor of everything equipped.
int characterArmor(const Character *c);

bool characterIsAlive(const Character *c);
const char *characterClassName(CharacterClass cls);

#endif
