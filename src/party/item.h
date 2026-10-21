// item.h - equipment definitions. An ItemDef is a read-only blueprint; characters point at the ones they wear.
//
// Today the items are a hardcoded table in item.c. Later the same ItemDef structs can be filled from a file
// in assets/data without touching anything that uses them.
#ifndef ITEM_H
#define ITEM_H

#include <stdbool.h>

typedef enum {
    ITEM_KIND_WEAPON,
    ITEM_KIND_ARMOR,
    ITEM_KIND_CHARM,
} ItemKind;

// A character has five equipment slots: one weapon, one main armor and three miscellaneous ones.
typedef enum {
    SLOT_WEAPON,
    SLOT_MAIN_ARMOR,
    SLOT_MISC_1,
    SLOT_MISC_2,
    SLOT_MISC_3,
    SLOT_COUNT
} EquipSlot;

typedef struct {
    char id[32];    // stable name used in data files and saves, e.g. "trainingSword"
    char name[48];  // what the player sees
    ItemKind kind;
    int damage;     // base damage added to the wearer's attacks (weapons)
    int armor;      // armor points added to the wearer
} ItemDef;

// Looks an item up by id. NULL if there is no such item.
const ItemDef *itemDefFind(const char *id);

// Weapons go in the weapon slot, armor in the main slot or a misc slot, charms in misc slots only.
bool itemFitsSlot(const ItemDef *item, EquipSlot slot);

const char *equipSlotName(EquipSlot slot);

#endif
