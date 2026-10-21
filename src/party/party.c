#include "party/party.h"

#include "core/textParse.h"

#include <string.h>

static Party party;

Party *partyGet(void) {
    return &party;
}

static const char *const slotKeys[SLOT_COUNT] = {"weapon", "armor", "misc1", "misc2", "misc3"};

bool partyParseMember(char *line, Character *out, const char *path, int lineNumber) {
    char token[96];
    char className[24] = "Adventurer";
    int maxHealth = 100, health = -1;
    const ItemDef *gear[SLOT_COUNT] = {0};

    for (;;) {
        char *save = line;
        if (!textNextToken(&line, token, sizeof(token))) break;
        const char *value;
        bool matched = false;

        if (textKeyValue(token, "class", &value)) {
            snprintf(className, sizeof(className), "%s", value);
            matched = true;
        } else if (textKeyValue(token, "health", &value)) {
            if (!textParseInt(value, &maxHealth) || maxHealth < 1) { textWarn("party", path, lineNumber, "health must be at least 1"); return false; }
            matched = true;
        } else if (textKeyValue(token, "hp", &value)) {
            if (!textParseInt(value, &health) || health < 0) { textWarn("party", path, lineNumber, "hp must be 0 or more"); return false; }
            matched = true;
        } else {
            for (int slot = 0; slot < SLOT_COUNT && !matched; slot++) {
                if (!textKeyValue(token, slotKeys[slot], &value)) continue;
                matched = true;
                if (!strcmp(value, "-") || *value == '\0') continue;
                const ItemDef *item = itemDefFind(value);
                if (!item) {
                    fprintf(stderr, "party: %s:%d: unknown item '%s' for %s (left empty)\n", path, lineNumber, value, slotKeys[slot]);
                } else if (!itemFitsSlot(item, (EquipSlot)slot)) {
                    fprintf(stderr, "party: %s:%d: item '%s' does not fit the %s slot (left empty)\n", path, lineNumber, value, slotKeys[slot]);
                } else {
                    gear[slot] = item;
                }
            }
        }

        if (!matched) { line = save; break; } // the rest of the line is the name
    }

    char *name = textSkipSpaces(line);
    if (*name == '\0') { textWarn("party", path, lineNumber, "member needs a name at the end of the line"); return false; }

    characterInit(out, name, className, maxHealth);
    if (health >= 0) out->health = health > maxHealth ? maxHealth : health;
    for (int slot = 0; slot < SLOT_COUNT; slot++) characterEquip(out, (EquipSlot)slot, gear[slot]);
    return true;
}

void partyWriteMember(FILE *file, const Character *c) {
    fprintf(file, "member class=%s health=%d hp=%d", c->className, c->maxHealth, c->health);
    for (int slot = 0; slot < SLOT_COUNT; slot++)
        fprintf(file, " %s=%s", slotKeys[slot], c->equipment[slot] ? c->equipment[slot]->id : "-");
    fprintf(file, " %s\n", c->name);
}

typedef struct { const char *path; Party *target; } LoadContext;

static void parseLine(char *line, int lineNumber, void *user) {
    LoadContext *context = user;
    char kind[32];
    textNextToken(&line, kind, sizeof(kind));
    if (strcmp(kind, "member") != 0) { textWarn("party", context->path, lineNumber, "unknown entry (expected member)"); return; }
    if (context->target->count >= MAX_PARTY) { textWarn("party", context->path, lineNumber, "the party is full"); return; }

    Character c;
    if (partyParseMember(line, &c, context->path, lineNumber)) context->target->members[context->target->count++] = c;
}

int partyLoadFile(const char *path) {
    memset(&party, 0, sizeof(party));
    LoadContext context = {path, &party};
    if (!textForEachLine(path, parseLine, &context)) {
        fprintf(stderr, "party: could not read %s\n", path);
        return -1;
    }
    return party.count;
}

void partyInitDefault(void) {
    if (partyLoadFile(PARTY_FILE) <= 0) fprintf(stderr, "party: the party is empty, check %s\n", PARTY_FILE);
}

void partyRestore(void) {
    for (int i = 0; i < party.count; i++) party.members[i].health = party.members[i].maxHealth;
}

int partyLivingCount(const Party *p) {
    int n = 0;
    for (int i = 0; i < p->count; i++)
        if (characterIsAlive(&p->members[i])) n++;
    return n;
}

int partyFirstLiving(const Party *p, int from) {
    for (int i = from < 0 ? 0 : from; i < p->count; i++)
        if (characterIsAlive(&p->members[i])) return i;
    return -1;
}
