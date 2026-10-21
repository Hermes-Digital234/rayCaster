// item.h - equipment definitions, loaded from assets/data/items.txt. An ItemDef is a read-only blueprint;
// characters point at the ones they wear.
//
// File format (one item per line, # for comments):
//
//   item <id> <weapon|armor|charm|consumable> [damage=<n>] [armor=<n>] [heal=<n|full>] <display name>
//
//   id       a one-word name used in other data files and in saves, e.g. trainingSword
//   damage   base damage added to the wearer's attacks (weapons)
//   armor    armor points added to the wearer (see combat/damage.h)
//   heal     health restored when a consumable is used on a party member (full = everything)
//   The display name is the rest of the line and may contain spaces.
//
//   item trainingSword weapon damage=20 Training Sword
//   item trainingArmor armor armor=300 Training Armor
//   item healingPotion consumable heal=50 Healing Potion
#ifndef ITEM_H
#define ITEM_H

#include <stdbool.h>

#define ITEMS_FILE "assets/data/items.txt"
#define MAX_ITEMS 256

typedef enum {
    ITEM_KIND_WEAPON,
    ITEM_KIND_ARMOR,
    ITEM_KIND_CHARM,
    ITEM_KIND_CONSUMABLE, // not worn: used from the inventory menu
} ItemKind;

#define ITEM_HEAL_FULL 1000000

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
    char id[32];
    char name[48];  // what the player sees
    ItemKind kind;
    int damage;
    int armor;
    int heal;       // consumables: health restored (ITEM_HEAL_FULL = all of it)
} ItemDef;

// Replaces the item table with the contents of the file. Returns the number of items loaded, or -1 if the
// file could not be read. Bad lines are reported on stderr and skipped. Pointers returned by itemDefFind stay
// valid until the next load.
int itemDbLoad(const char *path);

int itemDbCount(void);
const ItemDef *itemDbAt(int index);

// Looks an item up by id. NULL if there is no such item.
const ItemDef *itemDefFind(const char *id);

// Weapons go in the weapon slot, armor in the main slot or a misc slot, charms in misc slots only.
bool itemFitsSlot(const ItemDef *item, EquipSlot slot);

const char *equipSlotName(EquipSlot slot);

// Reads "weapon", "armor", "charm" or "consumable"
bool itemKindFromName(const char *name, ItemKind *out);

#endif
