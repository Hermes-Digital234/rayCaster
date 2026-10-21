#include "party/item.h"
#include <string.h>

static const ItemDef items[] = {
    {"trainingSword", "Training Sword", ITEM_KIND_WEAPON, 20, 0},
    {"trainingArmor", "Training Armor", ITEM_KIND_ARMOR, 0, 300},
};

const ItemDef *itemDefFind(const char *id) {
    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++)
        if (!strcmp(items[i].id, id)) return &items[i];
    return NULL;
}

bool itemFitsSlot(const ItemDef *item, EquipSlot slot) {
    if (!item) return false;
    switch (item->kind) {
        case ITEM_KIND_WEAPON: return slot == SLOT_WEAPON;
        case ITEM_KIND_ARMOR:  return slot == SLOT_MAIN_ARMOR || (slot >= SLOT_MISC_1 && slot <= SLOT_MISC_3);
        case ITEM_KIND_CHARM:  return slot >= SLOT_MISC_1 && slot <= SLOT_MISC_3;
    }
    return false;
}

const char *equipSlotName(EquipSlot slot) {
    switch (slot) {
        case SLOT_WEAPON:     return "Weapon";
        case SLOT_MAIN_ARMOR: return "Armor";
        case SLOT_MISC_1:     return "Misc 1";
        case SLOT_MISC_2:     return "Misc 2";
        case SLOT_MISC_3:     return "Misc 3";
        case SLOT_COUNT:      break;
    }
    return "";
}
