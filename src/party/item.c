#include "party/item.h"

#include "core/textParse.h"

#include <stdio.h>
#include <string.h>

static ItemDef items[MAX_ITEMS];
static int itemCount;

bool itemKindFromName(const char *name, ItemKind *out) {
    if (!strcmp(name, "weapon")) { *out = ITEM_KIND_WEAPON; return true; }
    if (!strcmp(name, "armor"))  { *out = ITEM_KIND_ARMOR;  return true; }
    if (!strcmp(name, "charm"))  { *out = ITEM_KIND_CHARM;  return true; }
    if (!strcmp(name, "consumable")) { *out = ITEM_KIND_CONSUMABLE; return true; }
    return false;
}

typedef struct { const char *path; } LoadContext;

static void parseItemLine(char *line, int lineNumber, void *user) {
    const char *path = ((LoadContext *)user)->path;
    char token[64];

    textNextToken(&line, token, sizeof(token)); // "item"
    if (strcmp(token, "item") != 0) { textWarn("items", path, lineNumber, "unknown entry (expected item)"); return; }

    ItemDef item;
    memset(&item, 0, sizeof(item));

    if (!textNextToken(&line, item.id, sizeof(item.id))) { textWarn("items", path, lineNumber, "item needs an id"); return; }
    if (itemDefFind(item.id)) { textWarn("items", path, lineNumber, "an item with this id already exists"); return; }

    if (!textNextToken(&line, token, sizeof(token)) || !itemKindFromName(token, &item.kind)) {
        textWarn("items", path, lineNumber, "item needs a type: weapon, armor, charm or consumable");
        return;
    }

    // key=value options, then the display name
    for (;;) {
        char *save = line;
        if (!textNextToken(&line, token, sizeof(token))) break;
        const char *value;
        if (textKeyValue(token, "damage", &value)) {
            if (!textParseInt(value, &item.damage) || item.damage < 0) { textWarn("items", path, lineNumber, "damage must be a number from 0 up"); return; }
        } else if (textKeyValue(token, "armor", &value)) {
            if (!textParseInt(value, &item.armor) || item.armor < 0) { textWarn("items", path, lineNumber, "armor must be a number from 0 up"); return; }
        } else if (textKeyValue(token, "heal", &value)) {
            if (!strcmp(value, "full")) item.heal = ITEM_HEAL_FULL;
            else if (!textParseInt(value, &item.heal) || item.heal < 0) { textWarn("items", path, lineNumber, "heal must be a number from 0 up, or full"); return; }
        } else {
            line = save; // not an option: the rest of the line is the name
            break;
        }
    }

    char *name = textSkipSpaces(line);
    snprintf(item.name, sizeof(item.name), "%s", *name ? name : item.id);

    if (itemCount >= MAX_ITEMS) { textWarn("items", path, lineNumber, "too many items"); return; }
    items[itemCount++] = item;
}

int itemDbLoad(const char *path) {
    itemCount = 0;
    LoadContext context = {path};
    if (!textForEachLine(path, parseItemLine, &context)) {
        fprintf(stderr, "items: could not read %s\n", path);
        return -1;
    }
    return itemCount;
}

int itemDbCount(void) { return itemCount; }

const ItemDef *itemDbAt(int index) {
    return (index >= 0 && index < itemCount) ? &items[index] : NULL;
}

const ItemDef *itemDefFind(const char *id) {
    for (int i = 0; i < itemCount; i++)
        if (!strcmp(items[i].id, id)) return &items[i];
    return NULL;
}

bool itemFitsSlot(const ItemDef *item, EquipSlot slot) {
    if (!item) return false;
    switch (item->kind) {
        case ITEM_KIND_WEAPON: return slot == SLOT_WEAPON;
        case ITEM_KIND_ARMOR:  return slot == SLOT_MAIN_ARMOR || (slot >= SLOT_MISC_1 && slot <= SLOT_MISC_3);
        case ITEM_KIND_CHARM:  return slot >= SLOT_MISC_1 && slot <= SLOT_MISC_3;
        case ITEM_KIND_CONSUMABLE: return false;
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
